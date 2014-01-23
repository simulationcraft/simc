// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace unique_gear;

#define maintenance_check( ilvl ) static_assert( ilvl >= 372, "unique item below min level, should be deprecated." )
#define DECLARE_CB(name) void name( special_effect_t& effect, const item_t& item, const special_effect_db_item_t& dbitem )

namespace { // UNNAMED NAMESPACE

/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace enchant
{
  DECLARE_CB( dancing_steel );
  DECLARE_CB( jade_spirit );
  DECLARE_CB( windsong );
  DECLARE_CB( elemental_force );
  DECLARE_CB( rivers_song );
  DECLARE_CB( colossus );
  DECLARE_CB( executioner );
  DECLARE_CB( hurricane_spell );
  DECLARE_CB( hurricane_weapon );
  DECLARE_CB( landslide );
  DECLARE_CB( mongoose );
  DECLARE_CB( power_torrent );
  DECLARE_CB( berserking );

  DECLARE_CB( gnomish_xray );
  DECLARE_CB( lord_blastingtons_scope_of_doom );
  DECLARE_CB( mirror_scope );
}

namespace profession
{
  DECLARE_CB( synapse_springs );
  DECLARE_CB( zen_alchemist_stone );
}

namespace item
{
  DECLARE_CB( flurry_of_xuen );
  DECLARE_CB( essence_of_yulon );
  DECLARE_CB( endurance_of_niuzao );
  DECLARE_CB( amplify_trinket );
  DECLARE_CB( cooldown_reduction_trinket );
  DECLARE_CB( multistrike_trinket );
  DECLARE_CB( cleave_trinket );
  DECLARE_CB( rune_of_reorigination );
  DECLARE_CB( spark_of_zandalar );
  DECLARE_CB( unerring_vision_of_leishen );
}

namespace gem
{
  DECLARE_CB( thundering_skyfire );
  DECLARE_CB( sinister_primal );
  DECLARE_CB( indomitable_primal );
  DECLARE_CB( capacitive_primal );
  DECLARE_CB( courageous_primal );
}

/**
 * Select attribute operator for buffs. Selects the attribute based on the
 * comparator given (std::greater for example), based on all defined attributes
 * that the stat buff is using. Note that this is for _ATTRIBUTES_ only. Using
 * it for any kind of stats will not work (for now).
 *
 * TODO: Generic way to get "composite" stat_e, so we can extend this class to
 * work on all Blizzard stats.
 */
template<template<typename> class CMP>
struct select_attr
{
  CMP<double> comparator;

  bool operator()( const stat_buff_t& buff ) const
  {
    // Comparing to 0 isn't exactly "correct", however the odds of an actor
    // having zero for all primary attributes is slim to none. If for some
    // reason all checked attributes are 0, the last checked attribute will be
    // the one selected. The order of stats checked is determined by the
    // stat_buff_creator add_stats() calls.
    double compare_to = 0;
    stat_e compare_stat = STAT_NONE, my_stat = STAT_NONE;

    for ( size_t i = 0, end = buff.stats.size(); i < end; i++ )
    {
      if ( this == buff.stats[ i ].check_func.target<select_attr<CMP> >() )
        my_stat = buff.stats[ i ].stat;

      attribute_e stat = static_cast<attribute_e>( buff.stats[ i ].stat );
      double val = buff.player -> get_attribute( stat );
      if ( ! compare_to || comparator( val, compare_to ) )
      {
        compare_to = val;
        compare_stat = buff.stats[ i ].stat;
      }
    }

    return compare_stat == my_stat;
  }
};

std::string suffix( const item_t& item )
{
  if ( item.slot == SLOT_OFF_HAND )
    return "_oh";
  return "";
}

std::string tokenized_name( const spell_data_t* data )
{
  std::string s = data -> name_cstr();
  util::tokenize( s );
  return s;
}

/**
 * Master list of special effects in Simulationcraft.
 *
 * Each line in the array corresponds to a specific spell (a proc driver spell,
 * or an "on use" spell) in World of Warcraft. There are several sources for
 * special effects:
 * 1) Items (Use, Equip, Chance on hit)
 * 2) Enchants, and profession specific enchants
 * 3) Engineering special effects (tinkers, ranged enchants)
 * 4) Gems
 *
 * Blizzard does not discriminate between the different types, nor do we
 * anymore. Each spell can be mapped to a special effect in the simc client.
 *
 * This list currently contains many entries, in the future we will also
 * determine most proc flags directly from the game client spells, thus
 * removing the need to specify lines here how an on-equip effect (or enchant
 * if it's a generic one) procs. This will remove most lines from this array,
 * what's left over are custom procs and procs where game client data is either
 * incorrect (so we can override values), or incomplete (so we can help the
 * automatic creation process on the simc side).
 *
 * Each entry contains three fields:
 * 1) The spell ID of the effect. You can find these from third party websites
 *    by clicking on the generated link in item tooltip.
 * 2) Currently a string of "additional options" given for a special effect.
 *    This includes the forementioned fixes of incorrect values, and "help" to
 *    drive the automatic special effect generation process. Case insensitive.
 * 3) A callback to a custom initialization function. The function is of the
 *    form: void custom_function_of_awesome( special_effect_t& effect,
 *                                           const item_t& item,
 *                                           const special_effect_db_item_t& dbitem )
 *    Where 'effect' is the effect being created, 'item' is the item that has 
 *    the special effect, and 'dbitem' is the database entry itself.
 *
 * Now, special effect creation in this new system is currently a two phase
 * process. First, the special_effect_t struct for the special effect is filled
 * with information from the game client data, and any options given in this
 * list (through the additional options). For custom special effects, the first
 * phase simply creates a stub special_effect_t object, and no game client data
 * is processed at this time.
 *
 * The second phase of the creation process is responsible for instantiating
 * the necessary action_callback_t object (simc procs), and whatever buffs, or
 * actions are required for the proc. This is also when custom callbacks get
 * called.
 *
 * Note: The special effect initialization process is now unified for all types
 * of special effects, we no longer discriminate between item, enchant, tinker,
 * or gem based special effects.
 *
 * Note2: Once we transition to the new "proc flags" system, all "simple"
 * on-equip procs will cease to exist in this list, and in addition, all future
 * "simple" trinkets will be readily usable by actors in simc, as long as we
 * have the necessary item and spell data from the game client.
 *
 * Note3: Enchants, addons, and possibly gems will have a separate translation
 * table in sc_enchant.cpp that maps "user given" names of enchants
 * (enchant=dancing_steel), to in game data, so we can properly initialize the
 * correct spells here.
 */
static const special_effect_db_item_t __special_effect_db[] = {
  /**
   * Items
   */

  /* Mists of Pandaria: 5.4 */
  { 146195, 0,                               item::flurry_of_xuen }, /* Melee legendary cloak */
  { 146197, 0,                             item::essence_of_yulon }, /* Caster legendary cloak */
  { 146193, 0,                          item::endurance_of_niuzao }, /* Tank legendary cloak */

  { 146311, "OnDirectDamage",                                   0 }, /* Ticking Ebon Detonator */           
  { 146183, "OnHarmfulSpellHit_10Stack",                        0 }, /* Black Blood of Y'Shaarj */
  { 146286, "OnAttackHit",                                      0 }, /* Skeer's Bloodsoaked Talisman */
  
  { 146313, "OnAttackHit",                                      0 }, /* Discipline of Xuen */
  { 146219, "OnSpellDamage",                                    0 }, /* Yu'lon's Bite */
  { 146295, "OnAttackHit",                                      0 }, /* Alacrity of Xuen */
  { 146047, "OnDirectDamage",                                   0 }, /* Purified Bindings of Immerseus (Int proc) */
  { 146251, "OnDirectDamage",                                   0 }, /* Thok's Tail Tip (Str proc) */
  { 146315, "OnHeal",                                           0 }, /* Prismatic Prison of Pride (Int proc) */
  { 146309, "OnDirectDamage",                                   0 }, /* Assurance of Consequence (Agi proc) */
  { 146247, "OnDirectDamage",                                   0 }, /* Evil Eye of Galakras (Str proc) */
  { 148904, "OnDirectDamage",                                   0 }, /* Haromm's Talisman (Agi proc) */
  { 148907, "OnDirectDamage",                                   0 }, /* Kardris' Toxic Talisman (Int proc) */
  { 148895, "OnDirectDamage",                                   0 }, /* Sigil of Rampage (Agi proc) */
  { 148898, "OnDirectDamage",                                   0 }, /* Frenzied Crystal of Rage (Int proc) */
  { 148901, "OnDirectDamage",                                   0 }, /* Fusion-Fire Core (Str proc) */

  { 146051, 0,                              item::amplify_trinket }, /* Amplification effect */
  { 146019, 0,                   item::cooldown_reduction_trinket }, /* Readiness effect */
  { 146059, 0,                          item::multistrike_trinket }, /* Multistrike effect */
  { 146136, 0,                               item::cleave_trinket }, /* Cleave effect */

  /* Mists of Pandaria: 5.2 */
  { 138728, "10Stack_Reverse_NoRefresh",                        0 }, /* Steadfast Talisman of the Shado-Pan Assault */
  { 138894, "OnDirectDamage",                                   0 }, /* Talisman of Bloodlust */
  { 138871, "OnDirectDamage",                                   0 }, /* Primordius' Talisman of Rage */
  { 139171, "OnAttackCrit",                                     0 }, /* Gaze of the Twins */
  { 138757, "OnDirectDamage_1Tick_138737Trigger",               0 }, /* Renataki's Soul Charm */
  { 138790, "OnSpellDamage_1Tick_138788Trigger",                0 }, /* Wushoolay's Final Choice */
  { 138758, "OnDirectDamage",                                   0 }, /* Fabled Feather of Ji-Kun */
  { 138896, "OnSpellTickDamage",                                0 }, /* Breath of the Hydra */
  { 139134, "OnHarmfulSpellCrit",                               0 }, /* Cha-Ye's Essence of Brilliance */
  { 138939, "OnDirectDamage",                                   0 }, /* Bad Juju */
  { 139116, 0,                        item::rune_of_reorigination }, /* Rune of Reorigination */
  { 138957, 0,                            item::spark_of_zandalar }, /* Spark of Zandalar */
  { 138964, 0,                   item::unerring_vision_of_leishen }, /* Unerring Vision of Lei Shen */

  /* Mists of Pandaria: 5.0 */
  { 126660, "OnHarmfulSpellHit",                                0 }, /* Essence of Terror */
  { 126650, "OnDirectDamage",                                   0 }, /* Terror in the Mists */
  { 126658, "OnDirectDamage",                                   0 }, /* Darkmist Vortex */
  { 126641, "OnHeal",                                           0 }, /* Spirits of the Sun */
  { 126647, "OnAttackHit",                                      0 }, /* Stuff of Nightmares */
  { 126579, "OnSpellTickDamage",                                0 }, /* Light of the Cosmos */
  { 126552, "OnAttackHit",                                      0 }, /* Bottle of Infinite Stars */
  { 126534, "OnAttackHit",                                      0 }, /* Vial of Dragon's Blood */
  { 126583, "OnAttackHit",                                      0 }, /* Lei Shen's Final Orders */
  { 126590, "OnHeal",                                           0 }, /* Qin-xi's Polarizing Seal */

  /* Mists of Pandaria: Dungeon */
  { 126473, "OnSpellDamage",                                    0 }, /* Vision of the Predator */
  { 126516, "OnDirectDamage",                                   0 }, /* Carbonic Carbuncle */
  { 126482, "OnDirectDamage",                                   0 }, /* Windswept Pages */
  { 126490, "OnDirectCrit",                                     0 }, /* Searing Words */
  { 126237, "OnAttackHit",                                      0 }, /* Iron Protector Talisman */

  /* Mists of Pandaria: Player versus Player */
  { 138704, "OnHarmfulSpellHit",                                0 }, /* Volatile Talisman of the Shado-Pan Assault */
  { 138701, "OnDirectDamage",                                   0 }, /* Brutal Talisman of the Shado-Pan Assault */
  { 138700, "OnDirectDamage",                                   0 }, /* Vicious Talisman of the Shado-Pan Assault */

  { 126702, "OnAttackHit",                                      0 }, /* Gladiator's Insignia of Victory */
  { 126706, "OnSpellDamage",                                    0 }, /* Gladiator's Insignia of Dominance */
  { 126708, "OnAttackHit",                                      0 }, /* Gladiator's Insignia of Conquest */

  /* Mists of Pandaria: Darkmoon Faire */ 
  { 128990, "OnSpellDamage",                                    0 }, /* Relic of Yu'lon */
  { 128991, "OnHeal",                                           0 }, /* Relic of Chi'ji */
  { 128445, "OnAttackCrit",                                     0 }, /* Relic of Xuen (agi) */
  { 128989, "OnAttackHit",                                      0 }, /* Relic of Xuen (str) */

  /**
   * Enchants
   */

  /* Mists of Pandaria */
  { 118333, 0,                             enchant::dancing_steel },
  { 142531, 0,                             enchant::dancing_steel }, /* Bloody Dancing Steel */
  { 141178, 0,                               enchant::jade_spirit },
  { 104561, 0,                                  enchant::windsong },
  { 104428, 0,                           enchant::elemental_force },
  { 104441, 0,                               enchant::rivers_song },
  { 118314, 0,                                  enchant::colossus },

  /* Cataclysm */
  {  94747, 0,                           enchant::hurricane_spell },
  {  74221, 0,                          enchant::hurricane_weapon },
  {  74245, 0,                                 enchant::landslide },
  {  94746, 0,                             enchant::power_torrent },

  /* Wrath of the Lich King */
  {  59620, 0,                                enchant::berserking },
  {  42976, 0,                               enchant::executioner },

  /* The Burning Crusade */
  {  28093, 0,                                  enchant::mongoose },

  /* Engineering enchants */
  { 109092, 0,                              enchant::mirror_scope },
  { 109085, 0,           enchant::lord_blastingtons_scope_of_doom },
  {  95713, 0,                              enchant::gnomish_xray },

  /* Profession perks */
  { 125484, "OnSpellDamageHeal",                                0 }, /* Lightweave Embroidery Rank 3 */
  {  75171, "OnSpellDamageHeal",                                0 }, /* Lightweave Embroidery Rank 2 */
  {  55640, "OnSpellDamageHeal",                                0 }, /* Lightweave Embroidery Rank 1 */
  { 125488, "OnSpellDamageHeal",                                0 }, /* Darkglow Embroidery Rank 3 */
  {  75174, "OnSpellDamageHeal",                                0 }, /* Darkglow Embroidery Rank 2 */
  {  55768, "OnSpellDamageHeal",                                0 }, /* Darkglow Embroidery Rank 1 */
  { 125486, "OnAttackHit",                                      0 }, /* Swordguard Embroidery Rank 3 */
  {  75177, "OnAttackHit",                                      0 }, /* Swordguard Embroidery Rank 2 */
  {  55776, "OnAttackHit",                                      0 }, /* Swordguard Embroidery Rank 1 */
  { 105574, 0,                    profession::zen_alchemist_stone }, /* Zen Alchemist Stone (stat proc) */
  { 141331, 0,                        profession::synapse_springs },
  { 126734, 0,                        profession::synapse_springs },

  /**
   * Gems
   */

  {  39958, 0,                            gem::thundering_skyfire },
  {  55380, 0,                            gem::thundering_skyfire }, /* Can use same callback for both */
  { 137592, 0,                               gem::sinister_primal }, /* Caster Legendary Gem */
  { 137594, 0,                            gem::indomitable_primal }, /* Tank Legendary Gem */
  { 137592, 0,                             gem::capacitive_primal }, /* Melee Legendary Gem */
  { 137248, 0,                             gem::courageous_primal }, /* Healer Legendary Gem */

  {      0, 0,                                                  0 }
};

struct stat_buff_proc_t : public buff_proc_callback_t<stat_buff_t>
{
  stat_buff_proc_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    buff_proc_callback_t<stat_buff_t>( p, data, 0, driver )
  {
    buff = stat_buff_creator_t( listener, proc_data.name_str )
           .activated( data.reverse || data.tick != timespan_t::zero() )
           .max_stack( proc_data.max_stacks )
           .duration( proc_data.duration )
           .cd( proc_data.cooldown )
           .reverse( proc_data.reverse )
           .refreshes( ! proc_data.no_refresh )
           .add_stat( proc_data.stat, proc_data.stat_amount );
  }
};

struct cost_reduction_buff_proc_t : public buff_proc_callback_t<cost_reduction_buff_t>
{
  cost_reduction_buff_proc_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    buff_proc_callback_t<cost_reduction_buff_t>( p, data, 0, driver )
  {
    buff = cost_reduction_buff_creator_t( listener, proc_data.name_str )
           .activated( false )
           .max_stack( proc_data.max_stacks )
           .duration( proc_data.duration )
           .cd( proc_data.cooldown )
           .reverse( proc_data.reverse )
           .amount( proc_data.discharge_amount )
           .refreshes( ! proc_data.no_refresh );
  }
};

struct discharge_spell_t : public spell_t
{
  discharge_spell_t( player_t* p, special_effect_t& data ) :
    spell_t( data.name_str, p, spell_data_t::nil() )
  {
    school = ( data.school == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : data.school;
    discharge_proc = true;
    item_proc = true;
    trigger_gcd = timespan_t::zero();
    base_dd_min = data.discharge_amount;
    base_dd_max = data.discharge_amount;
    direct_power_mod = data.discharge_scaling;
    may_crit = ( data.school != SCHOOL_DRAIN ) && ( ( data.override_result_es_mask & RESULT_CRIT_MASK ) ? ( data.result_es_mask & RESULT_CRIT_MASK ) : true ); // Default true
    may_miss = ( data.override_result_es_mask & RESULT_MISS_MASK ) ? ( data.result_es_mask & RESULT_MISS_MASK ) != 0 : may_miss;
    background  = true;
    aoe = data.aoe;
  }
};

struct discharge_attack_t : public attack_t
{
  discharge_attack_t(  player_t* p, special_effect_t& data ) :
    attack_t( data.name_str, p, spell_data_t::nil() )
  {
    school = ( data.school == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : data.school;
    discharge_proc = true;
    item_proc = true;
    trigger_gcd = timespan_t::zero();
    base_dd_min = data.discharge_amount;
    base_dd_max = data.discharge_amount;
    direct_power_mod = data.discharge_scaling;
    may_crit = ( data.school != SCHOOL_DRAIN ) && ( ( data.override_result_es_mask & RESULT_CRIT_MASK ) ? ( data.result_es_mask & RESULT_CRIT_MASK ) : true ); // Default true
    may_miss = ( data.override_result_es_mask & RESULT_MISS_MASK ) ? ( data.result_es_mask & RESULT_MISS_MASK ) != 0 : may_miss;
    may_dodge = ( data.school == SCHOOL_PHYSICAL ) && ( ( data.override_result_es_mask & RESULT_DODGE_MASK ) ? ( data.result_es_mask & RESULT_DODGE_MASK ) : may_dodge );
    may_parry = ( data.school == SCHOOL_PHYSICAL ) && ( ( data.override_result_es_mask & RESULT_PARRY_MASK ) ? ( data.result_es_mask & RESULT_PARRY_MASK ) : may_parry )
                && ( p -> position() == POSITION_FRONT || p ->position() == POSITION_RANGED_FRONT );
    //may_block = ( data.school == SCHOOL_PHYSICAL ) && ( ( data.override_result_es_mask & RESULT_BLOCK_MASK ) ? ( data.result_es_mask & RESULT_BLOCK_MASK ) : may_block )
    //            && ( p -> position() == POSITION_FRONT || p -> position() == POSITION_RANGED_FRONT );
    may_glance = false;
    background  = true;
    aoe = data.aoe;
  }
};

struct discharge_proc_callback_base_t : public discharge_proc_t<action_t>
{
  discharge_proc_callback_base_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    discharge_proc_t<action_t>( p, data, nullptr, driver )
  {
    if ( proc_data.discharge_amount > 0 )
    {
      discharge_action = new discharge_spell_t( p, proc_data );
    }
    else
    {
      discharge_action = new discharge_attack_t( p, proc_data );
    }
  }
};

// discharge_proc_callback ==================================================

struct discharge_proc_callback_t : public discharge_proc_callback_base_t
{

  discharge_proc_callback_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    discharge_proc_callback_base_t( p, data, driver )
  { }
};

// chance_discharge_proc_callback ===========================================

struct chance_discharge_proc_callback_t : public discharge_proc_callback_base_t
{

  chance_discharge_proc_callback_t( player_t* p, const special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    discharge_proc_callback_base_t( p, data, driver )
  { }

  virtual double proc_chance()
  {
    if ( discharge_stacks == this -> proc_data.max_stacks )
      return 1.0;

    return discharge_proc_callback_base_t::proc_chance();
  }
};

// stat_discharge_proc_callback =============================================

struct stat_discharge_proc_callback_t : public discharge_proc_t<action_t>
{
  stat_buff_t* buff;

  stat_discharge_proc_callback_t( player_t* p, special_effect_t& data, const spell_data_t* driver = spell_data_t::nil() ) :
    discharge_proc_t<action_t>( p, data, nullptr, driver )
  {
    if ( proc_data.max_stacks == 0 ) proc_data.max_stacks = 1;
    if ( proc_data.proc_chance == 0 ) proc_data.proc_chance = 1;

    buff = stat_buff_creator_t( p, proc_data.name_str )
           .max_stack( proc_data.max_stacks )
           .duration( proc_data.duration )
           .cd( proc_data.cooldown )
           .chance( proc_data.proc_chance )
           .activated( false /* proc_data.activated */ )
           .add_stat( proc_data.stat, proc_data.stat_amount );

    if ( proc_data.discharge_amount > 0 )
    {
      discharge_action = new discharge_spell_t( p, proc_data );
    }
    else
    {
      discharge_action = new discharge_attack_t( p, proc_data );
    }
  }

  virtual void deactivate()
  {
    action_callback_t::deactivate();
    buff -> expire();
  }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( buff -> trigger( a ) )
    {
      if ( ! allow_self_procs && ( a == discharge_action ) ) return;
      discharge_action -> execute();
    }
  }
};

// Weapon Proc Callback =====================================================

struct weapon_proc_callback_t : public proc_callback_t<action_state_t>
{
  typedef proc_callback_t<action_state_t> base_t;
  weapon_t* weapon;
  bool all_damage;

  weapon_proc_callback_t( player_t* p,
                          special_effect_t& e,
                          weapon_t* w,
                          bool all = false,
                          const spell_data_t* driver = spell_data_t::nil() ) :
    base_t( p, e, driver ),
    weapon( w ), all_damage( all )
  {
  }

  virtual void trigger( action_t* a, void* call_data )
  {
    if ( ! all_damage && a -> proc ) return;
    if ( a -> weapon && weapon && a -> weapon != weapon ) return;

    base_t::trigger( a, call_data );
  }
};


// Enchants ================================================================

void enchant::elemental_force( special_effect_t& effect,
                               const item_t& item,
                               const special_effect_db_item_t& dbitem )
{
  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* elemental_force_spell = item.player -> find_spell( 116616 );

  double amount = ( elemental_force_spell -> effectN( 1 ).min( item.player ) + elemental_force_spell -> effectN( 1 ).max( item.player ) ) / 2;

  effect.ppm = -1.0 * driver -> real_ppm();
  effect.school = SCHOOL_ELEMENTAL;
  effect.discharge_amount = amount;
  effect.rppm_scale = RPPM_HASTE;

  action_callback_t* cb  = new discharge_proc_callback_t( item.player, effect );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
}

void enchant::rivers_song( special_effect_t& effect,
                           const item_t& item,
                           const special_effect_db_item_t& dbitem )
{
  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = item.player -> find_spell( 116660 );

  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( item.player, "rivers_song" ) );
    
  if ( ! buff )
    buff = stat_buff_creator_t( item.player, "rivers_song", spell )
           .activated( false );

  effect.name_str = "rivers_song";
  effect.ppm = -1.0 * driver -> real_ppm();
  effect.cooldown = driver -> internal_cooldown();
  effect.rppm_scale = RPPM_HASTE;

  action_callback_t* cb = new buff_proc_callback_t<stat_buff_t>( item.player, effect, buff );

  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
  item.player -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
}

void enchant::colossus( special_effect_t& effect,
                        const item_t& item,
                        const special_effect_db_item_t& dbitem )
{
  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = item.player -> find_spell( 116631 );

  absorb_buff_t* buff = static_cast<absorb_buff_t*>( buff_t::find( item.player, "colossus" ) );
    
  if ( ! buff )
    buff = absorb_buff_creator_t( item.player, "colossus", spell )
           .source( item.player -> get_stats( "colossus" ) )
           .activated( false );

  effect.name_str = "colossus";
  effect.ppm = -1.0 * driver -> real_ppm();
  effect.cooldown = driver -> internal_cooldown();
  effect.rppm_scale = RPPM_HASTE;

  action_callback_t* cb = new buff_proc_callback_t<absorb_buff_t>( item.player, effect, buff );

  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK | RESULT_DODGE_MASK | RESULT_PARRY_MASK, cb );
  item.player -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
}

void enchant::dancing_steel( special_effect_t& effect, 
                             const item_t& item,
                             const special_effect_db_item_t& dbitem )
{
  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = item.player -> find_spell( 120032 );

  double value = spell -> effectN( 1 ).average( item.player );
  
  stat_buff_t* buff  = stat_buff_creator_t( item.player, effect.name_str, spell )
                       .activated( false )
                       .add_stat( STAT_STRENGTH, value, select_attr<std::greater>() )
                       .add_stat( STAT_AGILITY,  value, select_attr<std::greater>() );

  effect.ppm = -1.0 * driver -> real_ppm();

  buff_proc_callback_t<stat_buff_t>* cb = new buff_proc_callback_t<stat_buff_t>( item.player, effect, buff );

  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
}

struct jade_spirit_check_func
{
  bool operator()( const stat_buff_t& buff )
  {
    if ( buff.player -> resources.max[ RESOURCE_MANA ] <= 0.0 )
      return false;

    return buff.player -> resources.pct( RESOURCE_MANA ) < 0.25;
  }
};

void enchant::jade_spirit( special_effect_t& effect, 
                           const item_t& item,
                           const special_effect_db_item_t& dbitem )
{
  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = item.player -> find_spell( 104993 );

  double int_value = spell -> effectN( 1 ).average( item.player );
  double spi_value = spell -> effectN( 2 ).average( item.player );
  
  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( item.player, effect.name_str ) );
  if ( ! buff ) 
    buff = stat_buff_creator_t( item.player, effect.name_str, driver )
           .activated( false )
           .add_stat( STAT_INTELLECT, int_value )
           .add_stat( STAT_SPIRIT, spi_value, jade_spirit_check_func() );

  effect.name_str = "jade_spirit";
  effect.ppm = -1.0 * driver -> real_ppm();
  effect.cooldown = driver -> internal_cooldown();

  action_callback_t* cb = new buff_proc_callback_t<stat_buff_t>( item.player, effect, buff );

  item.player -> callbacks.register_tick_damage_callback  ( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_tick_heal_callback    ( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_direct_heal_callback  ( SCHOOL_ALL_MASK, cb );
}

struct windsong_callback_t : public proc_callback_t<action_state_t>
{
  stat_buff_t* haste, *crit, *mastery;

  windsong_callback_t( const special_effect_t& effect, 
                       stat_buff_t* hb,
                       stat_buff_t* cb,
                       stat_buff_t* mb,
                       const spell_data_t* driver ) :
    proc_callback_t<action_state_t>( hb -> player, effect, driver ),
    haste( hb ), crit( cb ), mastery( mb )
  { }

  void execute( action_t* /* a */, action_state_t* /* call_data */ )
  {
    stat_buff_t* buff;

    int p_type = ( int ) ( listener -> sim -> rng().real() * 3.0 );
    switch ( p_type )
    {
      case 0: buff = haste; break;
      case 1: buff = crit; break;
      case 2:
      default:
        buff = mastery; break;
    }

    buff -> trigger();
  }
};

void enchant::windsong( special_effect_t& effect, 
                        const item_t& item,
                        const special_effect_db_item_t& dbitem )
{
  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* mastery = item.player -> find_spell( 104510 );
  const spell_data_t* haste = item.player -> find_spell( 104423 );
  const spell_data_t* crit = item.player -> find_spell( 104509 );

  stat_buff_t* mastery_buff = stat_buff_creator_t( item.player, "windsong_mastery" + suffix( item ), mastery )
                              .activated( false );
  stat_buff_t* haste_buff   = stat_buff_creator_t( item.player, "windsong_haste" + suffix( item ), haste )
                              .activated( false );
  stat_buff_t* crit_buff    = stat_buff_creator_t( item.player, "windsong_crit" + suffix( item ), crit )
                              .activated( false );

  effect.ppm = -1.0 * driver -> real_ppm();

  action_callback_t* cb  = new windsong_callback_t( effect, haste_buff, crit_buff, mastery_buff, driver );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_spell_callback ( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_tick_callback  ( RESULT_HIT_MASK, cb );
  item.player -> callbacks.register_heal_callback  ( SCHOOL_ALL_MASK, cb );
}

void enchant::hurricane_weapon( special_effect_t& effect, 
                                const item_t& item,
                                const special_effect_db_item_t& dbitem )
{
  const spell_data_t* spell = item.player -> find_spell( dbitem.spell_id );
  stat_buff_t* buff = stat_buff_creator_t( item.player, tokenized_name( spell ) + suffix( item ), spell )
                      .activated( false );

  effect.name_str = tokenized_name( spell ) + suffix( item );
  effect.ppm = 1.0;

  action_callback_t* cb = new weapon_buff_proc_callback_t( item.player, effect, item.weapon(), buff );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

struct hurricane_spell_proc_t : public proc_callback_t<action_state_t>
{
  buff_t *mh_buff, *oh_buff, *s_buff;

  hurricane_spell_proc_t( player_t* p, const special_effect_t& effect, buff_t* mhb, buff_t* ohb, buff_t* sb ) :
    proc_callback_t<action_state_t>( p, effect ), 
    mh_buff( mhb ), oh_buff( ohb ), s_buff( sb )
  { }

  void execute( action_t* /* a */, action_state_t* /* call_data */ )
  {
    if ( mh_buff && mh_buff -> check() )
      mh_buff -> trigger();
    else if ( oh_buff && oh_buff -> check() )
      oh_buff -> trigger();
    else
      s_buff -> trigger();
  }
};

void enchant::hurricane_spell( special_effect_t& effect, 
                               const item_t& item,
                               const special_effect_db_item_t& dbitem )
{
  int n_hurricane_enchants = 0;

  if ( item.player -> items[ SLOT_MAIN_HAND ].parsed.enchant_id == 4083 ||
       util::str_compare_ci( item.player -> items[ SLOT_MAIN_HAND ].option_enchant_str, "hurricane" ) )
    n_hurricane_enchants++;

  if ( item.player -> items[ SLOT_OFF_HAND ].parsed.enchant_id == 4083 ||
       util::str_compare_ci( item.player -> items[ SLOT_OFF_HAND ].option_enchant_str, "hurricane" ) )
    n_hurricane_enchants++;

  buff_t* mh_buff = buff_t::find( item.player, "hurricane" );
  buff_t* oh_buff = buff_t::find( item.player, "hurricane_oh" );

  // If we have 2 hurricane enchants, and we're creating the first one
  // (opposite hand weapon buff has not been created), bail out early.
  // Note that this presumes that the spell item enchant has the procs in 
  // correct order (which they are, at the moment).
  if ( n_hurricane_enchants == 2 && ( ! mh_buff || ! oh_buff ) )
    return;

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();
  stat_buff_t* spell_buff = stat_buff_creator_t( item.player, "hurricane_s", spell )
                            .activated( false );

  effect.name_str = tokenized_name( spell );
  effect.proc_chance = driver -> proc_chance();
  effect.cooldown = driver -> internal_cooldown();

  action_callback_t* cb = new hurricane_spell_proc_t( item.player, effect, mh_buff, oh_buff, spell_buff );

  item.player -> callbacks.register_spell_direct_damage_callback( SCHOOL_SPELL_MASK, cb );
  item.player -> callbacks.register_spell_tick_damage_callback  ( SCHOOL_SPELL_MASK, cb );
  item.player -> callbacks.register_direct_heal_callback        ( SCHOOL_SPELL_MASK, cb );
  item.player -> callbacks.register_tick_heal_callback          ( SCHOOL_SPELL_MASK, cb );
}

void enchant::landslide( special_effect_t& effect, 
                         const item_t& item,
                         const special_effect_db_item_t& dbitem )
{
  const spell_data_t* spell = item.player -> find_spell( dbitem.spell_id );
  stat_buff_t* buff = stat_buff_creator_t( item.player, tokenized_name( spell ) + suffix( item ), spell )
                      .activated( false );

  effect.name_str = tokenized_name( spell ) + suffix( item );
  effect.ppm = 1.0;

  action_callback_t* cb = new weapon_buff_proc_callback_t( item.player, effect, item.weapon(), buff );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

void enchant::power_torrent( special_effect_t& effect, 
                             const item_t& item,
                             const special_effect_db_item_t& dbitem )
{
  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  stat_buff_t* buff = stat_buff_creator_t( item.player, tokenized_name( spell ) + suffix( item ), spell )
                      .activated( false );

  effect.name_str = tokenized_name( spell ) + suffix( item );
  effect.proc_chance = driver -> proc_chance();
  effect.cooldown = driver -> internal_cooldown();

  action_callback_t* cb = new buff_proc_callback_t<stat_buff_t>( item.player, effect, buff );
  
  item.player -> callbacks.register_spell_tick_damage_callback  ( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_spell_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_tick_heal_callback          ( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_direct_heal_callback        ( SCHOOL_ALL_MASK, cb );
}

void enchant::executioner( special_effect_t& effect, 
                           const item_t& item,
                           const special_effect_db_item_t& dbitem )
{
  const spell_data_t* spell = item.player -> find_spell( dbitem.spell_id );
  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( item.player, tokenized_name( spell ) ) );

  if ( ! buff )
    buff = stat_buff_creator_t( item.player, tokenized_name( spell ), spell )
           .activated( false );

  effect.name_str = tokenized_name( spell );
  effect.ppm = 1.0;

  action_callback_t* cb = new weapon_buff_proc_callback_t( item.player, effect, item.weapon(), buff );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

void enchant::berserking( special_effect_t& effect, 
                          const item_t& item,
                          const special_effect_db_item_t& dbitem )
{
  const spell_data_t* spell = item.player -> find_spell( dbitem.spell_id );
  stat_buff_t* buff = stat_buff_creator_t( item.player, tokenized_name( spell ) + suffix( item ), spell )
                      .activated( false );

  effect.name_str = tokenized_name( spell ) + suffix( item );
  effect.ppm = 1.0;

  action_callback_t* cb = new weapon_buff_proc_callback_t( item.player, effect, item.weapon(), buff );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

void enchant::mongoose( special_effect_t& effect, 
                        const item_t& item,
                        const special_effect_db_item_t& dbitem )
{
  const spell_data_t* spell = item.player -> find_spell( dbitem.spell_id );
  stat_buff_t* buff = stat_buff_creator_t( item.player, tokenized_name( spell ) + suffix( item ), spell )
                      .activated( false );

  effect.name_str = tokenized_name( spell ) + suffix( item );
  effect.ppm = 1.0;

  action_callback_t* cb = new weapon_buff_proc_callback_t( item.player, effect, item.weapon(), buff );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

void enchant::lord_blastingtons_scope_of_doom( special_effect_t& effect, 
                                               const item_t& item,
                                               const special_effect_db_item_t& dbitem )
{
  const spell_data_t* spell = item.player -> find_spell( dbitem.spell_id );

  stat_buff_t* buff = stat_buff_creator_t( item.player, tokenized_name( spell ), spell )
                      .activated( false );

  effect.name_str = tokenized_name( spell );
  effect.ppm = 1.0; // PPM

  action_callback_t* cb = new weapon_buff_proc_callback_t( item.player, effect, item.weapon(), buff );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

void enchant::mirror_scope( special_effect_t& effect, 
                            const item_t& item,
                            const special_effect_db_item_t& dbitem )
{
  const spell_data_t* spell = item.player -> find_spell( dbitem.spell_id );

  effect.name_str = tokenized_name( spell );
  effect.ppm = 1.0;

  stat_buff_t* buff = stat_buff_creator_t( item.player, tokenized_name( spell ), spell );

  action_callback_t* cb = new weapon_buff_proc_callback_t( item.player, effect, item.weapon(), buff );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

void enchant::gnomish_xray( special_effect_t& effect, 
                            const item_t& item,
                            const special_effect_db_item_t& dbitem )
{
  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  effect.name_str = tokenized_name( spell );
  effect.ppm = 1.0;
  effect.cooldown = driver -> internal_cooldown();

  stat_buff_t* buff = stat_buff_creator_t( item.player, tokenized_name( spell ), spell );

  action_callback_t* cb = new weapon_buff_proc_callback_t( item.player, effect, item.weapon(), buff );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

// Profession perks =========================================================

void profession::synapse_springs( special_effect_t& effect, 
                              const item_t& item,
                              const special_effect_db_item_t& dbitem )
{
  const spell_data_t* use_spell = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* buff_spell = item.player -> find_spell( 96228 );

  double value = use_spell -> effectN( 1 ).average( item.player );

  stat_buff_t* buff  = stat_buff_creator_t( item.player, effect.name_str, buff_spell )
                       .add_stat( STAT_STRENGTH, value, select_attr<std::greater>() )
                       .add_stat( STAT_INTELLECT, value, select_attr<std::greater>() )
                       .add_stat( STAT_AGILITY,  value, select_attr<std::greater>() );

  effect.cooldown = use_spell -> cooldown();

  effect.custom_buff = buff;
}

void profession::zen_alchemist_stone( special_effect_t& effect,
                                      const item_t& item,
                                      const special_effect_db_item_t& dbitem )
{
  struct zen_alchemist_stone_callback : public proc_callback_t<action_state_t>
  {
    stat_buff_t* buff_str;
    stat_buff_t* buff_agi;
    stat_buff_t* buff_int;

    zen_alchemist_stone_callback( const item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      const spell_data_t* spell = listener -> find_spell( 105574 );

      struct common_buff_creator : public stat_buff_creator_t
      {
        common_buff_creator( player_t* p, const std::string& n, const spell_data_t* spell ) :
          stat_buff_creator_t ( p, "zen_alchemist_stone_" + n, spell  )
        {
          duration( p -> find_spell( 60229 ) -> duration() );
          chance( 1.0 );
          activated( false );
        }
      };

      double value = spell -> effectN( 1 ).average( i );

      buff_str = common_buff_creator( listener, "str", spell )
                 .add_stat( STAT_STRENGTH, value );
      buff_agi = common_buff_creator( listener, "agi", spell )
                 .add_stat( STAT_AGILITY, value );
      buff_int = common_buff_creator( listener, "int", spell )
                 .add_stat( STAT_INTELLECT, value );
    }

    void execute( action_t* a, action_state_t* /* state */ )
    {
      player_t* p = a -> player;

      if ( p -> strength() > p -> agility() )
      {
        if ( p -> strength() > p -> intellect() )
          buff_str -> trigger();
        else
          buff_int -> trigger();
      }
      else if ( p -> agility() > p -> intellect() )
        buff_agi -> trigger();
      else
        buff_int -> trigger();
    }
  };

  maintenance_check( 450 );

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );

  effect.name_str    = "zen_alchemist_stone";
  effect.proc_chance = driver -> proc_chance();
  effect.cooldown    = driver -> internal_cooldown();

  zen_alchemist_stone_callback* cb = new zen_alchemist_stone_callback( item, effect );
  item.player -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_tick_damage_callback( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_direct_heal_callback( RESULT_ALL_MASK, cb );
}

// Gems ====================================================================

void gem::thundering_skyfire( special_effect_t& effect, 
                              const item_t& item,
                              const special_effect_db_item_t& dbitem )
{
  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  //FIXME: 0.2 ppm seems to roughly match in-game behavior, but we need to verify the exact mechanics
  stat_buff_t* buff = stat_buff_creator_t( item.player, tokenized_name( spell ), spell )
                      .activated( false );

  effect.name_str = tokenized_name( spell );
  effect.ppm = 0.2; // PPM

  // TODO: Procs only on auto attacks it seems. Make it proc on attacks for
  // now, regardless of handedness.
  action_callback_t* cb = new buff_proc_callback_t<stat_buff_t>( item.player, effect, buff );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

void gem::sinister_primal( special_effect_t& effect, 
                           const item_t& item,
                           const special_effect_db_item_t& dbitem )
{
  if ( item.sim -> challenge_mode )
    return;

  struct sinister_primal_proc_t : public buff_proc_callback_t<buff_t>
  {
    sinister_primal_proc_t( player_t* p, const special_effect_t& data, const spell_data_t* driver ) :
      buff_proc_callback_t<buff_t>( p, data, p -> buffs.tempus_repit, driver )
    { }
  };

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );

  effect.name_str = "tempus_repit";
  effect.ppm      = -1.0 * driver -> real_ppm();
  effect.cooldown = driver -> internal_cooldown(); 

  sinister_primal_proc_t* cb = new sinister_primal_proc_t( item.player, effect, driver );
  item.player -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_tick_damage_callback( SCHOOL_ALL_MASK, cb );
}

void gem::indomitable_primal( special_effect_t& effect, 
                              const item_t& item,
                              const special_effect_db_item_t& dbitem )
{
  if ( item.sim -> challenge_mode )
    return;

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
    
  effect.name_str = "fortitude";
  effect.ppm      = -1.0 * driver -> real_ppm();
  effect.cooldown = driver -> internal_cooldown();

  action_callback_t *cb = new buff_proc_callback_t<buff_t>( item.player, effect, item.player -> buffs.fortitude );
  item.player -> callbacks.register_incoming_attack_callback( RESULT_ALL_MASK, cb );
}

void gem::capacitive_primal( special_effect_t& effect, 
                             const item_t& item,
                             const special_effect_db_item_t& dbitem )
{
  if ( item.sim -> challenge_mode )
    return;

  struct lightning_strike_t : public attack_t
  {
    lightning_strike_t( player_t* p ) :
      attack_t( "lightning_strike", p, p -> find_spell( 137597 ) )
    {
      may_crit = special = background = true;
      may_parry = may_dodge = false;
      proc = false;
      direct_power_mod = data().extra_coeff();
    }
  };

  struct capacitive_primal_proc_t : public discharge_proc_t<action_t>
  {
    capacitive_primal_proc_t( player_t* p, const special_effect_t& data, action_t* a, const spell_data_t* spell ) :
      discharge_proc_t<action_t>( p, data, a, spell )
    {
      // Unfortunately the weapon-based RPPM modifiers have to be hardcoded,
      // as they will not show on the client tooltip data.
      if ( listener -> main_hand_weapon.group() != WEAPON_2H )
      {
        if ( listener -> specialization() == WARRIOR_FURY )
          rppm.set_modifier( 1.152 );
        else if ( listener -> specialization() == DEATH_KNIGHT_FROST )
          rppm.set_modifier( 1.134 );
      }
    }

    void trigger( action_t* action, void* call_data )
    {
      // Flurry of Xuen and Capacitance cannot proc Capacitance
      if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
        return;

      discharge_proc_t<action_t>::trigger( action, call_data );
    }
  };

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = item.player -> find_spell( 137596 );

  effect.name_str   = "lightning_strike";
  effect.max_stacks = spell -> max_stacks();
  effect.ppm        = -1.0 * driver -> real_ppm();
  effect.rppm_scale = RPPM_HASTE;
  effect.cooldown   = driver -> internal_cooldown();

  action_t* ls = item.player -> create_proc_action( "lightning_strike" );
  if ( ! ls )
    ls = new lightning_strike_t( item.player );
  action_callback_t* cb = new capacitive_primal_proc_t( item.player, effect, ls, driver );
  item.player -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

void gem::courageous_primal( special_effect_t& effect, 
                             const item_t& item,
                             const special_effect_db_item_t& dbitem )
{
  if ( item.sim -> challenge_mode )
    return;

  struct courageous_primal_proc_t : public buff_proc_callback_t<buff_t>
  {
    courageous_primal_proc_t( player_t* p, const special_effect_t& data ) :
      buff_proc_callback_t<buff_t>( p, data, p -> buffs.courageous_primal_diamond_lucidity )
    { }

    void trigger( action_t* action, void* call_data )
    {
      spell_base_t* spell = debug_cast<spell_base_t*>( action );
      if ( ! spell -> procs_courageous_primal_diamond )
        return;

      buff_proc_callback_t<buff_t>::trigger( action, call_data );
    }

    void execute( action_t* action, action_state_t* call_data )
    {
      if ( listener -> sim -> debug )
        listener -> sim -> out_debug.printf( "%s procs %s from action %s.",
            listener -> name(), buff -> name(), action -> name() );

      buff_proc_callback_t<buff_t>::execute( action, call_data );
    }
  };

    const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
    effect.name_str = "courageous_primal_diamond_lucidity";
    effect.ppm      = -1.0 * driver -> real_ppm();
    effect.cooldown = driver -> internal_cooldown();

    action_callback_t* cb = new courageous_primal_proc_t( item.player, effect );
    item.player -> callbacks.register_spell_callback( RESULT_ALL_MASK, cb );
}

// Items ====================================================================

void jikuns_rising_winds( item_t* item )
{
  maintenance_check( 502 );

  player_t* p = item -> player;

  item -> unique = true;

  const spell_data_t* spell = item -> player -> find_spell( 138973 );


  struct jikuns_rising_winds_heal_t : public heal_t
  {
    jikuns_rising_winds_heal_t( item_t& i, const spell_data_t  *spell ) :
      heal_t( "jikuns_rising_winds", i.player, spell )
    {
      trigger_gcd = timespan_t::zero();
      background  = true;
      may_miss    = false;
      may_crit    = true;
      callbacks   = false;
      const random_prop_data_t& budget = i.player -> dbc.random_property( i.item_level() );

      base_dd_min = budget.p_epic[ 0 ]  * spell  -> effectN( 1 ).m_average();
      base_dd_max = base_dd_min;
      init();
    }
  };

  struct jikuns_rising_winds_callback_t : public action_callback_t
  {
    heal_t* heal;
    cooldown_t* cd;
    proc_t* proc;

    jikuns_rising_winds_callback_t( player_t* p, heal_t* s ) :
      action_callback_t( p ), heal( s ), proc( 0 )
    {
      proc = p -> get_proc( "jikuns_rising_winds" );
      cd = p -> get_cooldown( "jikuns_rising_winds_callback" );
      cd -> duration = timespan_t::from_seconds( 30.0 );
    }

    virtual void trigger( action_t*, void* )
    {
      if ( cd -> remains() <= timespan_t::zero() && listener -> health_percentage() < 35 )
      {
        heal -> target = listener;
        heal -> execute();
        proc -> occur();
        cd -> start();
      }

    }
  };

  p -> callbacks.register_incoming_attack_callback( RESULT_HIT_MASK,  new jikuns_rising_winds_callback_t( p, new jikuns_rising_winds_heal_t( *item, spell ) )  );
}

// delicate_vial_of_the_sanguinaire =========================================

void delicate_vial_of_the_sanguinaire( item_t* item )
{
  maintenance_check( 502 );

  player_t* p = item -> player;

  item -> unique = true;


  const spell_data_t* spell = item -> player -> find_spell( 138865 );

  special_effect_t data;
  data.name_str    = "delicate_vial_of_the_sanguinaire";
  data.duration    = spell -> duration();
  data.max_stacks  = spell -> max_stacks();
  data.proc_chance = spell -> proc_chance();

  struct delicate_vial_of_the_sanguinaire_callback_t : public proc_callback_t<action_state_t>
  {
    stat_buff_t* buff;

    delicate_vial_of_the_sanguinaire_callback_t( item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data ),
      buff( nullptr )
    {
      const spell_data_t* spell = listener -> find_spell( 138864 );

      const random_prop_data_t& budget = listener -> dbc.random_property( i.item_level() );

      buff   = stat_buff_creator_t( listener, "blood_of_power" )
               .duration( spell -> duration() )
               .add_stat( STAT_MASTERY_RATING, budget.p_epic[ 0 ]  * spell  -> effectN( 1 ).m_average() );
    }

    void execute( action_t* /* action */, action_state_t* /* state */ )
    {
      buff -> trigger();
    }
  };

  p -> callbacks.register_incoming_attack_callback( RESULT_DODGE_MASK, new delicate_vial_of_the_sanguinaire_callback_t( *item, data ) );
}

void item::rune_of_reorigination( special_effect_t& effect,
                                  const item_t& item,
                                  const special_effect_db_item_t& dbitem )
{
  struct rune_of_reorigination_callback_t : public proc_callback_t<action_state_t>
  {
    enum
    {
      BUFF_CRIT = 0,
      BUFF_HASTE,
      BUFF_MASTERY
    };

    stat_buff_t* buff;

    rune_of_reorigination_callback_t( const item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      buff = stat_buff_creator_t( listener, proc_data.name_str )
             .activated( false )
             .duration( proc_data.duration )
             .add_stat( STAT_CRIT_RATING, 0 )
             .add_stat( STAT_HASTE_RATING, 0 )
             .add_stat( STAT_MASTERY_RATING, 0 );
    }

    virtual void execute( action_t* action, action_state_t* /* state */ )
    {
      // We can never allow this trinket to refresh, so force the trinket to 
      // always expire, before we proc a new one.
      buff -> expire();

      player_t* p = action -> player;

      // Determine highest stat based on rating multipliered stats
      double chr = p -> composite_melee_haste_rating();
      if ( p -> sim -> scaling -> scale_stat == STAT_HASTE_RATING )
        chr -= p -> sim -> scaling -> scale_value * p -> composite_rating_multiplier( RATING_MELEE_HASTE );

      double ccr = p -> composite_melee_crit_rating();
      if ( p -> sim -> scaling -> scale_stat == STAT_CRIT_RATING )
        ccr -= p -> sim -> scaling -> scale_value * p -> composite_rating_multiplier( RATING_MELEE_CRIT );

      double cmr = p -> composite_mastery_rating();
      if ( p -> sim -> scaling -> scale_stat == STAT_MASTERY_RATING )
        cmr -= p -> sim -> scaling -> scale_value * p -> composite_rating_multiplier( RATING_MASTERY );

      // Give un-multipliered stats so we don't double dip anywhere.
      chr /= p -> composite_rating_multiplier( RATING_MELEE_HASTE );
      ccr /= p -> composite_rating_multiplier( RATING_MELEE_CRIT );
      cmr /= p -> composite_rating_multiplier( RATING_MASTERY );
    
      if ( p -> sim -> debug )
        p -> sim -> out_debug.printf( "%s rune_of_reorigination procs crit=%.0f haste=%.0f mastery=%.0f",
                            p -> name(), ccr, chr, cmr );

      if ( ccr >= chr )
      {
        // I choose you, crit
        if ( ccr >= cmr )
        {
          buff -> stats[ BUFF_CRIT    ].amount = 2 * ( chr + cmr );
          buff -> stats[ BUFF_HASTE   ].amount = -chr;
          buff -> stats[ BUFF_MASTERY ].amount = -cmr;
        }
        // I choose you, mastery
        else
        {
          buff -> stats[ BUFF_CRIT    ].amount = -ccr;
          buff -> stats[ BUFF_HASTE   ].amount = -chr;
          buff -> stats[ BUFF_MASTERY ].amount = 2 * ( ccr + chr );
        }
      }
      // I choose you, haste
      else if ( chr >= cmr )
      {
        buff -> stats[ BUFF_CRIT    ].amount = -ccr;
        buff -> stats[ BUFF_HASTE   ].amount = 2 * ( ccr + cmr );
        buff -> stats[ BUFF_MASTERY ].amount = -cmr;
      }
      // I choose you, mastery
      else
      {
        buff -> stats[ BUFF_CRIT    ].amount = -ccr;
        buff -> stats[ BUFF_HASTE   ].amount = -chr;
        buff -> stats[ BUFF_MASTERY ].amount = 2 * ( ccr + chr );
      }

      buff -> trigger();
    }
  };

  maintenance_check( 502 );

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = item.player -> find_spell( 139120 );

  effect.name_str    = "rune_of_reorigination";
  effect.ppm         = -1.0 * driver -> real_ppm();
  effect.ppm        *= item_database::approx_scale_coefficient( 528, item.item_level() );
  effect.cooldown    = driver -> internal_cooldown(); 
  effect.duration    = spell -> duration();

  item.player -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, new rune_of_reorigination_callback_t( item, effect ) );
}

void item::spark_of_zandalar( special_effect_t& effect,
                              const item_t& item,
                              const special_effect_db_item_t& dbitem )
{
  maintenance_check( 502 );

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = item.player -> find_spell( 138958 );

  effect.name_str    = "spark_of_zandalar";
  effect.ppm         = -1.0 * driver -> real_ppm();
  effect.duration    = spell -> duration();
  effect.max_stacks  = spell -> max_stacks();

  struct spark_of_zandalar_callback_t : public proc_callback_t<action_state_t>
  {
    buff_t*      sparks;
    stat_buff_t* buff;

    spark_of_zandalar_callback_t( const item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      sparks = buff_creator_t( listener, proc_data.name_str )
               .activated( false )
               .duration( proc_data.duration )
               .max_stack( proc_data.max_stacks );

      const spell_data_t* spell = listener -> find_spell( 138960 );

      buff = stat_buff_creator_t( listener, "zandalari_warrior" )
             .duration( spell -> duration() )
             .add_stat( STAT_STRENGTH, spell -> effectN( 2 ).average( i ) );
    }

    void execute( action_t* /* action */, action_state_t* /* state */ )
    {
      sparks -> trigger();

      if ( sparks -> stack() == sparks -> max_stack() )
      {
        sparks -> expire();
        buff   -> trigger();
      }
    }
  };

  item.player -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, new spark_of_zandalar_callback_t( item, effect ) );
};

void item::unerring_vision_of_leishen( special_effect_t& effect,
                                       const item_t& item,
                                       const special_effect_db_item_t& dbitem )
{
  struct perfect_aim_buff_t : public buff_t
  {
    perfect_aim_buff_t( player_t* p, const spell_data_t* s ) :
      buff_t( buff_creator_t( p, "perfect_aim", s ).activated( false ) )
    { }

    void execute( int stacks, double value, timespan_t duration )
    {
      if ( current_stack == 0 )
      {
        player -> current.spell_crit  += data().effectN( 1 ).percent();
        player -> current.attack_crit += data().effectN( 1 ).percent();
        player -> invalidate_cache( CACHE_CRIT );
      }

      buff_t::execute( stacks, value, duration );
    }

    void expire_override()
    {
      buff_t::expire_override();

      player -> current.spell_crit  -= data().effectN( 1 ).percent();
      player -> current.attack_crit -= data().effectN( 1 ).percent();
      player -> invalidate_cache( CACHE_CRIT );
    }
  };

  struct unerring_vision_of_leishen_callback_t : public proc_callback_t<action_state_t>
  {
    perfect_aim_buff_t* buff;

    unerring_vision_of_leishen_callback_t( const item_t& i, const special_effect_t& data, const spell_data_t* driver ) :
      proc_callback_t<action_state_t>( i.player, data, driver )
    {
      buff = new perfect_aim_buff_t( listener, listener -> find_spell( 138963 ) );
      if ( i.player -> type == WARLOCK )
        rppm.set_modifier( 0.6 );
    }

    void execute( action_t* /* action */, action_state_t* /* state */ )
    { buff -> trigger(); }
  };

  maintenance_check( 502 );

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );

  effect.name_str  = "perfect_aim";
  effect.ppm       = -1.0 * driver -> real_ppm();
  effect.ppm      *= item_database::approx_scale_coefficient( 528, item.item_level() );
  effect.cooldown  = driver -> internal_cooldown();

  unerring_vision_of_leishen_callback_t* cb = new unerring_vision_of_leishen_callback_t( item, effect, driver );
  item.player -> callbacks.register_spell_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  item.player -> callbacks.register_spell_tick_damage_callback( SCHOOL_ALL_MASK, cb );
}

template <typename T>
struct cleave_t : public T
{
  cleave_t( const item_t& item, const char* name, school_e s ) :
    T( name, item.player )
  {
    this -> callbacks = false;
    this -> may_crit = false;
    this -> may_glance = false;
    this -> may_miss = true;
    this -> special = true;
    this -> proc = true;
    this -> background = true;
    this -> school = s;
    this -> aoe = 5;
    this -> snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA;
    if ( this -> type == ACTION_ATTACK )
    {
      this -> may_dodge = true;
      this -> may_parry = true;
      this -> may_block = true;
    }
  }

  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    tl.clear();

    for ( size_t i = 0, actors = this -> sim -> target_non_sleeping_list.size(); i < actors; i++ )
    {
      player_t* t = this -> sim -> target_non_sleeping_list[ i ];

      if ( t -> is_enemy() && ( t != this -> target ) )
        tl.push_back( t );
    }

    return tl.size();
  }

  double composite_target_multiplier( player_t* ) const
  { return 1.0; }

  double composite_da_multiplier() const
  { return 1.0; }

  double target_armor( player_t* ) const
  { return 0.0; }
};

void item::cleave_trinket( special_effect_t& effect,
                           const item_t& item,
                           const special_effect_db_item_t& dbitem )
{
  maintenance_check( 528 );

  struct cleave_callback_t : public proc_callback_t<action_state_t>
  {
    cleave_t<spell_t>* cleave_spell;
    cleave_t<attack_t>* cleave_attack;

    cleave_callback_t( const item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      cleave_spell = new cleave_t<spell_t>( i, "cleave_spell", SCHOOL_NATURE );
      cleave_attack = new cleave_t<attack_t>( i, "cleave_attack", SCHOOL_PHYSICAL );
    }

    void execute( action_t* action, action_state_t* state )
    {
      action_t* a = 0;

      if ( action -> type == ACTION_ATTACK )
        a = cleave_attack;
      else if ( action -> type == ACTION_SPELL )
        a = cleave_spell;
      // TODO: Heal

      if ( a )
      {
        a -> base_dd_min = a -> base_dd_max = state -> result_amount;
        a -> target = state -> target;
        a -> schedule_execute();
      }
    }
  };

  player_t* p = item.player;
  const spell_data_t* cleave_driver_spell = p -> find_spell( dbitem.spell_id );

  std::string name = cleave_driver_spell -> name_cstr();
  util::tokenize( name );

  effect.name_str = name;
  effect.proc_chance = cleave_driver_spell -> effectN( 1 ).average( item ) / 10000.0;

  cleave_callback_t* cb = new cleave_callback_t( item, effect );
  p -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  p -> callbacks.register_tick_damage_callback( SCHOOL_ALL_MASK, cb );
}

void item::multistrike_trinket( special_effect_t& effect,
                                const item_t& item,
                                const special_effect_db_item_t& dbitem )
{
  maintenance_check( 528 );

  // TODO: Healing multistrike

  struct multistrike_attack_t : public attack_t
  {
    multistrike_attack_t( const item_t& item ) :
      attack_t( "multistrike_attack", item.player )
    {
      callbacks = may_crit = may_glance = false;
      proc = background = special = true;
      school = SCHOOL_PHYSICAL;
      snapshot_flags |= STATE_MUL_DA;
    }

    double composite_target_multiplier( player_t* ) const
    { return 1.0; }

    double composite_da_multiplier() const
    { return 1.0 / 3.0; }

    double target_armor( player_t* ) const
    { return 0.0; }
  };

  struct multistrike_spell_t : public spell_t
  {
    multistrike_spell_t( const item_t& item ) :
      spell_t( "multistrike_spell", item.player )
    {
      callbacks = may_crit = false;
      proc = background = true;
      school = SCHOOL_NATURE; // Multiple schools in reality, but any school would work
      snapshot_flags |= STATE_MUL_DA;
    }

    double composite_target_multiplier( player_t* ) const
    { return 1.0; }

    double composite_da_multiplier() const
    { return 1.0 / 3.0; }

    double target_armor( player_t* ) const
    { return 0.0; }
  };

  struct multistrike_callback_t : public proc_callback_t<action_state_t>
  {
    action_t* strike_attack;
    action_t* strike_spell;

    multistrike_callback_t( const item_t& i, const special_effect_t& data ) :
      proc_callback_t<action_state_t>( i.player, data )
    {
      if ( ! ( strike_attack = listener -> create_proc_action( "multistrike_attack" ) ) )
      {
        strike_attack = new multistrike_attack_t( i );
        strike_attack -> init();
      }

      if ( ! ( strike_spell = listener -> create_proc_action( "multistrike_spell" ) ) )
      {
        strike_spell = new multistrike_spell_t( i );
        strike_spell -> init();
      }
    }

    void execute( action_t* action, action_state_t* state )
    {
      action_t* a = 0;

      if ( action -> type == ACTION_ATTACK )
        a = strike_attack;
      else if ( action -> type == ACTION_SPELL )
        a = strike_spell;
      // TODO: Heal

      if ( a )
      {
        a -> base_dd_min = a -> base_dd_max = state -> result_amount;
        a -> target = state -> target;
        a -> schedule_execute();
      }
    }
  };

  player_t* p = item.player;
  const spell_data_t* ms_driver_spell = p -> find_spell( dbitem.spell_id );

  std::string name = ms_driver_spell -> name_cstr();
  util::tokenize( name );
  effect.name_str = name;
  effect.proc_chance = ms_driver_spell -> effectN( 1 ).average( item ) / 1000.0;

  multistrike_callback_t* cb = new multistrike_callback_t( item, effect );
  p -> callbacks.register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
  p -> callbacks.register_tick_damage_callback( SCHOOL_ALL_MASK, cb );
}

void item::cooldown_reduction_trinket( special_effect_t& /* effect */,
                                       const item_t& item,
                                       const special_effect_db_item_t& dbitem )
{
  maintenance_check( 528 );

  const size_t MAX_COOLDOWNS = 8;
  const size_t MAX_SPECIALIZATIONS = 19;

  struct cooldowns_t
  {
    specialization_e spec;
    const char*      cooldowns[ MAX_COOLDOWNS ];
  };

  static const cooldowns_t __cd[] =
  {
    // NOTE: Spells that trigger buffs must have the cooldown of their buffs removed if they have one, or this trinket may cause undesirable results.
    { ROGUE_ASSASSINATION, { "evasion", "vanish", "cloak_of_shadows", "vendetta", "shadow_blades", 0, 0 } },
    { ROGUE_COMBAT,        { "evasion", "adrenaline_rush", "cloak_of_shadows", "killing_spree", "shadow_blades", 0, 0 } },
    { ROGUE_SUBTLETY,      { "evasion", "vanish", "cloak_of_shadows", "shadow_dance", "shadow_blades", 0, 0 } },
    { SHAMAN_ENHANCEMENT,  { "spiritwalkers_grace", "earth_elemental_totem", "fire_elemental_totem", "shamanistic_rage", "ascendance", "feral_spirit", 0 } },
    { DRUID_FERAL,         { "tigers_fury", "berserk", "barkskin", "survival_instincts", 0, 0, 0 } },
    { DRUID_GUARDIAN,      { "might_of_ursoc", "berserk", "barkskin", "survival_instincts", 0, 0, 0 } },
    { WARRIOR_FURY,        { "dragon_roar", "bladestorm", "shockwave", "avatar", "bloodbath", "recklessness", "storm_bolt", "heroic_leap" } },
    { WARRIOR_ARMS,        { "dragon_roar", "bladestorm", "shockwave", "avatar", "bloodbath", "recklessness", "storm_bolt", "heroic_leap" } },
    { WARRIOR_PROTECTION,  { "shield_wall", "demoralizing_shout", "last_stand", "recklessness", "heroic_leap", 0, 0 } },
    { DEATH_KNIGHT_BLOOD,  { "antimagic_shell", "dancing_rune_weapon", "icebound_fortitude", "outbreak", "vampiric_blood", "bone_shield", 0 } },
    { DEATH_KNIGHT_FROST,  { "antimagic_shell", "army_of_the_dead", "icebound_fortitude", "empower_rune_weapon", "outbreak", "pillar_of_frost", 0  } },
    { DEATH_KNIGHT_UNHOLY, { "antimagic_shell", "army_of_the_dead", "icebound_fortitude", "unholy_frenzy", "outbreak", "summon_gargoyle", 0 } },
    { MONK_BREWMASTER,	   { "fortifying_brew", "guard", "zen_meditation", 0, 0, 0, 0 } }, 
    { MONK_WINDWALKER,     { "energizing_brew", "fists_of_fury", "fortifying_brew", "zen_meditation", 0, 0, 0 } },
    { PALADIN_PROTECTION,  { "ardent_defender", "avenging_wrath", "divine_protection", "divine_shield", "guardian_of_ancient_kings", 0 } },
    { PALADIN_RETRIBUTION, { "avenging_wrath", "divine_protection", "divine_shield", "guardian_of_ancient_kings", 0, 0 } },
    { HUNTER_BEAST_MASTERY,{ "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", "bestial_wrath", 0 } },
    { HUNTER_MARKSMANSHIP, { "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", 0, 0 } },
    { HUNTER_SURVIVAL,     { "black_arrow", "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", 0 } },
    { SPEC_NONE,           { 0 } }
  };

  player_t* p = item.player;
  const spell_data_t* cdr_spell = p -> find_spell( dbitem.spell_id );
  double cdr = cdr_spell -> effectN( 1 ).average( item ) / 100.0;

  p -> buffs.cooldown_reduction -> s_data = cdr_spell;
  p -> buffs.cooldown_reduction -> default_value = cdr;
  p -> buffs.cooldown_reduction -> default_chance = 1;

  for ( size_t spec = 0; spec < MAX_SPECIALIZATIONS; spec++ )
  {
    const cooldowns_t& cd = __cd[ spec ];
    if ( p -> specialization() != cd.spec )
      continue;

    for ( size_t ability = 0; ability < MAX_COOLDOWNS; ability++ )
    {
      if ( cd.cooldowns[ ability ] == 0 )
        break;

      cooldown_t* ability_cd = p -> get_cooldown( cd.cooldowns[ ability ] );
      ability_cd -> set_recharge_multiplier( cdr );
    }
  }
}

void item::amplify_trinket( special_effect_t& /* effect */,
                            const item_t& item,
                            const special_effect_db_item_t& dbitem )
{
  maintenance_check( 528 );

  player_t* p = item.player;
  const spell_data_t* amplify_spell = p -> find_spell( dbitem.spell_id );

  p -> buffs.amplified -> default_value = amplify_spell -> effectN( 2 ).average( item ) / 100.0;
  p -> buffs.amplified -> default_chance = 1.0;
}

struct flurry_of_xuen_melee_t : public attack_t
{
  flurry_of_xuen_melee_t( player_t* player ) : 
    attack_t( "flurry_of_xuen", player, player -> find_spell( 147891 ) )
  {
    direct_power_mod = data().extra_coeff();
    background = true;
    proc = false;
    aoe = 5;
    special = may_crit = true;
  }
};

struct flurry_of_xuen_ranged_t : public ranged_attack_t
{
  flurry_of_xuen_ranged_t( player_t* player ) : 
    ranged_attack_t( "flurry_of_xuen", player, player -> find_spell( 147891 ) )
  {
    direct_power_mod = data().extra_coeff();
    background = true;
    proc = false;
    aoe = 5;
    special = may_crit = true;
  }
};

struct flurry_of_xuen_driver_t : public attack_t
{
  flurry_of_xuen_driver_t( player_t* player, action_t* action = 0 ) :
    attack_t( "flurry_of_xuen", player, player -> find_spell( 146194 ) )
  {
    hasted_ticks = may_miss = may_dodge = may_parry = may_crit = may_block = callbacks = false;
    proc = background = dual = true;

    if ( ! action )
    {
      if ( player -> type == HUNTER )
        action = new flurry_of_xuen_ranged_t( player );
      else
        action = new flurry_of_xuen_melee_t( player );
    }
    tick_action = action;
    dynamic_tick_action = true;
  }
};

struct flurry_of_xuen_cb_t : public proc_callback_t<action_state_t>
{
  action_t* action;

  flurry_of_xuen_cb_t( const item_t& item, const special_effect_t& effect, const spell_data_t* driver ) :
    proc_callback_t<action_state_t>( item.player, effect, driver ),
    action( new flurry_of_xuen_driver_t( listener, listener -> create_proc_action( effect.name_str ) ) )

  { }

  void trigger( action_t* action, void* call_data )
  {
    // Flurry of Xuen, and Lightning Strike cannot proc Flurry of Xuen
    if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
      return;

    proc_callback_t<action_state_t>::trigger( action, call_data );
  }

  void execute( action_t*, action_state_t* state )
  {
    action -> target = state -> target;
    action -> schedule_execute(); 
  }
};

void item::flurry_of_xuen( special_effect_t& effect,
                           const item_t& item,
                           const special_effect_db_item_t& dbitem )
{
  maintenance_check( 600 );

  if ( item.sim -> challenge_mode )
    return;

  player_t* p = item.player;
  const spell_data_t* driver = p -> find_spell( dbitem.spell_id );
  std::string name = driver -> name_cstr();
  util::tokenize( name );

  effect.name_str   = name;
  effect.ppm        = -1.0 * driver -> real_ppm();
  effect.ppm       *= item_database::approx_scale_coefficient( item.parsed.data.level, item.item_level() );
  effect.rppm_scale = RPPM_HASTE;
  effect.cooldown   = driver -> internal_cooldown();

  flurry_of_xuen_cb_t* cb = new flurry_of_xuen_cb_t( item, effect, driver );

  p -> callbacks.register_attack_callback( RESULT_HIT_MASK, cb );
}

struct essence_of_yulon_t : public spell_t
{
  essence_of_yulon_t( player_t* p, const spell_data_t& driver ) :
    spell_t( "essence_of_yulon", p, p -> find_spell( 148008 ) )
  {
    background = may_crit = true;
    proc = false;
    aoe = 5;
    direct_power_mod /= driver.duration().total_seconds() + 1;
  }
};

struct essence_of_yulon_driver_t : public spell_t
{
  essence_of_yulon_driver_t( player_t* player ) :
    spell_t( "essence_of_yulon", player, player -> find_spell( 146198 ) )
  {
    hasted_ticks = may_miss = may_dodge = may_parry = may_block = callbacks = false;
    tick_zero = proc = background = dual = true;
    travel_speed = 0;

    tick_action = new essence_of_yulon_t( player, data() );
    dynamic_tick_action = true;
  }
};

struct essence_of_yulon_cb_t : public proc_callback_t<action_state_t>
{
  action_t* action;

  essence_of_yulon_cb_t( const item_t& item, const special_effect_t& effect, const spell_data_t* driver ) :
    proc_callback_t<action_state_t>( item.player, effect, driver ),
    action( new essence_of_yulon_driver_t( item.player ) )
  { }

  void execute( action_t*, action_state_t* state )
  {
    action -> target = state -> target;
    action -> schedule_execute();
  }

  void trigger( action_t* action, void* call_data )
  {
    if ( action -> id == 148008 )
      return;

    proc_callback_t<action_state_t>::trigger( action, call_data );
  }
};

void item::essence_of_yulon( special_effect_t& effect,
                             const item_t& item,
                             const special_effect_db_item_t& dbitem )
{
  maintenance_check( 600 );

  if ( item.sim -> challenge_mode )
    return;

  player_t* p = item.player;
  const spell_data_t* driver = p -> find_spell( dbitem.spell_id );
  std::string name = driver -> name_cstr();
  util::tokenize( name );

  effect.name_str    = name;
  effect.ppm         = -1.0 * driver -> real_ppm();
  effect.ppm        *= item_database::approx_scale_coefficient( item.parsed.data.level, item.item_level() );
  effect.rppm_scale  = RPPM_HASTE;

  essence_of_yulon_cb_t* cb = new essence_of_yulon_cb_t( item, effect, driver );
  p -> callbacks.register_spell_direct_damage_callback( SCHOOL_ALL_MASK, cb );
}

// Qiang-Ying, Fortitude of Niuzao and Qian-Le, Courage of Niuzao

void item::endurance_of_niuzao( special_effect_t& /* effect */,
                                const item_t& item, 
                                const special_effect_db_item_t& dbitem )
{
  maintenance_check( 600 );

  if ( item.sim -> challenge_mode )
    return;

  player_t* p = item.player;

  p -> legendary_tank_cloak_cd = p -> get_cooldown( "endurance_of_niuzao" );
  p -> legendary_tank_cloak_cd -> duration = p -> find_spell( dbitem.spell_id ) -> duration();
}

/*


  // DK Runeforges
  else if ( name == "rune_of_cinderglacier"               ) e = "custom";
  else if ( name == "rune_of_razorice"                    ) e = "custom";
  else if ( name == "rune_of_the_fallen_crusader"         ) e = "custom";
*/

} // UNNAMED NAMESPACE

/* 
 * Initialize a special effect, based on a spell id. Returns true if the first
 * phase initialization succeeded, false otherwise. If the spell id points to a
 * spell that our system cannot support, also sets the special effect type to
 * SPECIAL_EFFECT_NONE.
 * 
 * Note that the first phase initialization simply fills up special_effect_t
 * with relevant information for non-custom special effects. Second phase of
 * the initialization (performed by unique_gear::init) will instantiate the
 * proc callback, and relevant actions/buffs, or call a custom function to
 * perform the initialization.
 */
bool unique_gear::initialize_special_effect( special_effect_t& effect,
                                             const item_t&     item,
                                             unsigned          spell_id )
{
  bool ret = true;

  const special_effect_db_item_t& dbitem = find_special_effect_db_item( spell_id );

  // Figure out first phase options from our special effect database
  if ( dbitem.spell_id == spell_id )
  {
    // Custom special effect initialization is deferred, and no parsing from 
    // spell data is done automatically.
    if ( dbitem.custom_cb != 0 )
      effect.type = SPECIAL_EFFECT_CUSTOM;
    
    // Parse auxilary effect options before doing spell data based parsing
    if ( dbitem.encoded_options != 0 )
    {
      std::string encoded_options = dbitem.encoded_options;
      util::tokenize( encoded_options );
      // Note, if the encoding parse fails (this should never ever happen), 
      // we don't parse game client data either.
      if ( ! proc::parse_special_effect_encoding( effect, item, encoded_options ) )
        return false;
    }
  }

  // Generic procs will go through game client data based parsing, or be 
  // discarded, if there's nothing that simc can use to generate the proc
  // automatically
  if ( effect.type != SPECIAL_EFFECT_CUSTOM )
  {
    // A "parseable" special effect in either the spell_id given, or the user 
    // defined trigger_spell_id.
    if ( proc::usable_effects( item.player, spell_id ) > 0 )
      ret = effect.parse_spell_data( item, spell_id );
    else if ( effect.trigger_spell_id > 0 && proc::usable_effects( item.player, effect.trigger_spell_id ) )
      ret = effect.parse_spell_data( item, spell_id );
    // No special effects, so no real point in making a special_effect_t. The
    // item_t decode_x functions will not push the effect to the item's
    // parsed special effect vector, unless effect.type is of the correct
    // type.
    else
      effect.type = SPECIAL_EFFECT_NONE;
  }
  // Custom procs will put the spell id in the special_effect_t, so second 
  // phase initialization can find the custom callback that needs to be 
  // called.
  else
    effect.spell_id = spell_id;

  return ret;
}

const special_effect_db_item_t& unique_gear::find_special_effect_db_item( unsigned spell_id )
{
  for ( size_t i = 0, end = sizeof_array( __special_effect_db ); i < end; i++ )
  {
    const special_effect_db_item_t& dbitem = __special_effect_db[ i ];
    if ( dbitem.spell_id == spell_id )
      return dbitem;
  }

  return __special_effect_db[ sizeof_array( __special_effect_db ) - 1 ];
}

// ==========================================================================
// unique_gear::init
// ==========================================================================

void unique_gear::init( player_t* p )
{
  if ( p -> is_pet() ) return;

  for ( size_t i = 0; i < p -> items.size(); i++ )
  {
    item_t& item = p -> items[ i ];

    for ( size_t j = 0; j < item.parsed.special_effects.size(); j++ )
    {
      special_effect_t& effect = item.parsed.special_effects[ j ];

      if ( effect.type == SPECIAL_EFFECT_EQUIP )
      {
        if ( effect.stat && effect.school )
        {
          register_stat_discharge_proc( p, effect );
        }
        else if ( effect.stat )
        {
          register_stat_proc( p, effect );
        }
        else if ( effect.cost_reduction && effect.school )
        {
          register_cost_reduction_proc( p, effect );
        }
        else if ( effect.school && effect.proc_chance && effect.chance_to_discharge )
        {
          register_chance_discharge_proc( p, effect );
        }
        else if ( effect.school )
        {
          register_discharge_proc( p, effect );
        }
      }
      else if ( effect.type == SPECIAL_EFFECT_CUSTOM )
      {
        assert( effect.spell_id > 0 );
        const special_effect_db_item_t& dbitem = find_special_effect_db_item( effect.spell_id );
        assert( dbitem.custom_cb != 0 );
        dbitem.custom_cb( effect, item, dbitem );
      }
    }

    if ( ! strcmp( item.name(), "delicate_vial_of_the_sanguinaire"    ) ) delicate_vial_of_the_sanguinaire  ( &item );
    if ( ! strcmp( item.name(), "jikuns_rising_winds"                 ) ) jikuns_rising_winds               ( &item );
  }
}

// ==========================================================================
// unique_gear::stat_proc
// ==========================================================================

action_callback_t* unique_gear::register_stat_proc( player_t* player,
                                                    const special_effect_t& effect )
{
  action_callback_t* cb = new stat_buff_proc_t( player, effect );

  if ( effect.trigger_type == PROC_DAMAGE || effect.trigger_type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( effect.trigger_mask, cb );
    player -> callbacks.register_direct_damage_callback( effect.trigger_mask, cb );
  }
  if ( effect.trigger_type == PROC_HEAL || effect.trigger_type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( effect.trigger_mask, cb );
    player -> callbacks.register_direct_heal_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( effect.trigger_mask, cb );
    player -> callbacks.register_tick_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HARMFUL_SPELL_LANDING )
  {
    player -> callbacks.register_harmful_spell_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DAMAGE_HEAL_SPELL )
  {
    player -> callbacks.register_spell_callback( effect.trigger_mask, cb );
    player -> callbacks.register_heal_callback( effect.trigger_mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear::cost_reduction_proc
// ==========================================================================

action_callback_t* unique_gear::register_cost_reduction_proc( player_t* player,
                                                              const special_effect_t& effect )
{
  action_callback_t* cb = new cost_reduction_buff_proc_t( player, effect );

  if ( effect.trigger_type == PROC_DAMAGE || effect.trigger_type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( effect.trigger_mask, cb );
    player -> callbacks.register_direct_damage_callback( effect.trigger_mask, cb );
  }
  if ( effect.trigger_type == PROC_HEAL || effect.trigger_type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( effect.trigger_mask, cb );
    player -> callbacks.register_direct_heal_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( effect.trigger_mask, cb );
    player -> callbacks.register_tick_callback( effect.trigger_mask, cb );
  }
  else if ( effect.trigger_type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( effect.trigger_mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear::discharge_proc
// ==========================================================================

action_callback_t* unique_gear::register_discharge_proc( player_t* player,
                                                         const special_effect_t& effect )
{
  action_callback_t* cb = new discharge_proc_callback_t( player, effect );

  const proc_e& type = effect.trigger_type;
  const int64_t& mask = effect.trigger_mask;

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( mask, cb );
    player -> callbacks.register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_AND_TICK )
  {
    player -> callbacks.register_spell_callback( mask, cb );
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear::chance_discharge_proc
// ==========================================================================

action_callback_t* unique_gear::register_chance_discharge_proc( player_t* player,
                                                                const special_effect_t& effect )
{
  action_callback_t* cb = new chance_discharge_proc_callback_t( player, effect );

  const proc_e& type = effect.trigger_type;
  const int64_t& mask = effect.trigger_mask;

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL  || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( mask, cb );
    player -> callbacks.register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_AND_TICK )
  {
    player -> callbacks.register_spell_callback( mask, cb );
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear::stat_discharge_proc
// ==========================================================================

action_callback_t* unique_gear::register_stat_discharge_proc( player_t* player,
                                                              const special_effect_t& effect )
{
  action_callback_t* cb = new discharge_proc_callback_t( player, effect );

  const proc_e& type = effect.trigger_type;
  const int64_t& mask = effect.trigger_mask;

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL  || type == PROC_DAMAGE_HEAL )
  {
    player -> callbacks.register_tick_heal_callback( mask, cb );
    player -> callbacks.register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> callbacks.register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> callbacks.register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_CRIT )
  {
    player -> callbacks.register_direct_crit_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_TICK_DAMAGE )
  {
    player -> callbacks.register_spell_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT_DAMAGE )
  {
    player -> callbacks.register_spell_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> callbacks.register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> callbacks.register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> callbacks.register_spell_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> callbacks.register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> callbacks.register_heal_callback( mask, cb );
  }

  return cb;
}

