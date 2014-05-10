// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

using namespace unique_gear;

#define maintenance_check( ilvl ) static_assert( ilvl >= 372, "unique item below min level, should be deprecated." )

namespace { // UNNAMED NAMESPACE

/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace enchant
{
  void dancing_steel( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void jade_spirit( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void windsong( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void elemental_force( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void rivers_song( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void colossus( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void executioner( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void hurricane_spell( special_effect_t&, const item_t&, const special_effect_db_item_t& );
}

namespace profession
{
  void nitro_boosts( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void zen_alchemist_stone( special_effect_t&, const item_t&, const special_effect_db_item_t& );
}

namespace item
{
  void flurry_of_xuen( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void essence_of_yulon( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void endurance_of_niuzao( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void skeers_bloodsoaked_talisman( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void black_blood_of_yshaarj( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void rune_of_reorigination( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void spark_of_zandalar( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void unerring_vision_of_leishen( special_effect_t&, const item_t&, const special_effect_db_item_t& );
}

namespace gem
{
  void sinister_primal( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void indomitable_primal( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void capacitive_primal( special_effect_t&, const item_t&, const special_effect_db_item_t& );
  void courageous_primal( special_effect_t&, const item_t&, const special_effect_db_item_t& );
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
 * This list currently contains custom procs and procs where game client data
 * is either incorrect (so we can override values), or incomplete (so we can
 * help the automatic creation process on the simc side).
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
 * Each special effect is fed to a new proc callback object
 * (dbc_proc_callback_t) that handles the initialization of the proc, and in
 * generic proc cases, the initialization of the buffs/actions.
 *
 * Each entry contains three fields:
 * 1) The spell ID of the effect. You can find these from third party websites
 *    by clicking on the generated link in item tooltip.
 * 2) Currently a c-string of "additional options" given for a special effect.
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
 * with enough information to initialize the proc (for generic procs, a driver
 * spell id is sufficient), and any options given in this list (through the
 * additional options). For custom special effects, the first phase simply
 * creates a stub special_effect_t object, and no game client data is processed
 * at this time.
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
 * Note2: Enchants, addons, and possibly gems will have a separate translation
 * table in sc_enchant.cpp that maps "user given" names of enchants
 * (enchant=dancing_steel), to in game data, so we can properly initialize the
 * correct spells here. Most of the enchants etc., are automatically
 * identified.  The table will only have the "non standard" user strings we
 * currently use, and whatever else we will use in the future.
 */
static const special_effect_db_item_t __special_effect_db[] = {
  /**
   * TRINKET-PROC-TYPE-TODO:
   * - Alacrity of Xuen (procs on damage or landing an ability?)
   * - Evil Eye of Galakras (damage/landing?)
   * - Kadris Toxic Totem (damage/landing?)
   * - Frenzied Crystal of Rage (damage/landing?)
   * - Fusion-Fire Core (damage/landing?)
   * - Talisman of Bloodlust (damage/landing?)
   * - Primordius' Talisman of Rage (damage/landing?)
   * - Fabled Feather of Ji-Kun (damage/landing?)
   */

  /**
   * Items
   */

  /* Mists of Pandaria: 5.4 */
  { 146195, 0,                               item::flurry_of_xuen }, /* Melee legendary cloak */
  { 146197, 0,                             item::essence_of_yulon }, /* Caster legendary cloak */
  { 146193, 0,                          item::endurance_of_niuzao }, /* Tank legendary cloak */

  { 146219, "ProcOn/Hit",                                       0 }, /* Yu'lon's Bite */
  { 146251, "ProcOn/Hit",                                       0 }, /* Thok's Tail Tip (Str proc) */


  { 146183, 0,                       item::black_blood_of_yshaarj }, /* Black Blood of Y'Shaarj */
  { 146286, 0,                  item::skeers_bloodsoaked_talisman }, /* Skeer's Bloodsoaked Talisman */

  /* Mists of Pandaria: 5.2 */
  { 139116, 0,                        item::rune_of_reorigination }, /* Rune of Reorigination */
  { 138957, 0,                            item::spark_of_zandalar }, /* Spark of Zandalar */
  { 138964, 0,                   item::unerring_vision_of_leishen }, /* Unerring Vision of Lei Shen */

  { 138728, "Reverse",                                          0 }, /* Steadfast Talisman of the Shado-Pan Assault */
  { 139171, "ProcOn/Crit_RPPMAttackCrit",                       0 }, /* Gaze of the Twins */
  { 138757, "1Tick_138737Trigger",                              0 }, /* Renataki's Soul Charm */
  { 138790, "ProcOn/Hit_1Tick_138788Trigger",                   0 }, /* Wushoolay's Final Choice */
  { 138758, "1Tick_138760Trigger",                              0 }, /* Fabled Feather of Ji-Kun */
  { 139134, "ProcOn/Crit_RPPMSpellCrit",                        0 }, /* Cha-Ye's Essence of Brilliance */

  { 138865, "ProcOn/Dodge",                                     0 }, /* Delicate Vial of the Sanguinaire */
  // TODO: Ji-Kun's rising winds (heal action support)
  // TODO: Soul barrier (absorb with individual absorb event cap)
  // TODO: Rook's unlucky Talisman (aoe damage reduction)

  /* Mists of Pandaria: 5.0 */
  { 126660, "OnHarmfulSpellHit",                                0 }, /* Essence of Terror */
  { 126650, "OnDirectDamage",                                   0 }, /* Terror in the Mists */
  { 126658, "OnDirectDamage",                                   0 }, /* Darkmist Vortex */
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
  { 120033, 0,                               enchant::jade_spirit },
  { 141178, 0,                               enchant::jade_spirit },
  { 104561, 0,                                  enchant::windsong },
  { 104428, "rppmhaste",                                        0 }, /* Elemental Force */
  { 104441, 0,                               enchant::rivers_song },
  { 118314, 0,                        enchant::colossus }, /* Colossus */

  /* Cataclysm */
  {  94747, 0,                           enchant::hurricane_spell },
  {  74221, "1PPM",                                             0 }, /* Hurricane Weapon */
  {  74245, "1PPM",                                             0 }, /* Landslide */
  {  94746, 0,                                                  0 }, /* Power Torrent */

  /* Wrath of the Lich King */
  {  59620, "1PPM",                                             0 }, /* Berserking */
  {  42976, 0,                               enchant::executioner },

  /* The Burning Crusade */
  {  28093, "1PPM",                                             0 }, /* Mongoose */

  /* Engineering enchants */
  { 109092, "1PPM",                                             0 }, /* Mirror Scope */
  { 109085, "1PPM",                                             0 }, /* Lord Blastingtons Scope of Doom */
  {  95713, "1PPM",                                             0 }, /* Gnomish XRay */

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
  {  55004, 0,                           profession::nitro_boosts },

  /**
   * Gems
   */

  {  39958, "0.2PPM", 0 }, /* Thundering Skyfire */ // TODO: check why 20% PPM and not 100% from spell data?
  {  55380, "0.2PPM", 0 }, /* Thundering Skyfire */ /* Can use same callback for both */ // TODO: check why 20% PPM and not 100% from spell data?
  { 137592, 0,                               gem::sinister_primal }, /* Caster Legendary Gem */
  { 137594, 0,                            gem::indomitable_primal }, /* Tank Legendary Gem */
  { 137595, 0,                             gem::capacitive_primal }, /* Melee Legendary Gem */
  { 137248, 0,                             gem::courageous_primal }, /* Healer Legendary Gem */

  {      0, 0,                                                  0 }
};


// Enchants ================================================================

void enchant::colossus( special_effect_t& effect, const item_t& item,
                        const special_effect_db_item_t& /* dbitem */)
{
  const spell_data_t* spell = item.player->find_spell( 116631 );

  absorb_buff_t* buff =
      absorb_buff_creator_t( item.player,
                             tokenized_name( spell ) + suffix( item ) )
      .spell( spell )
      .source( item.player->get_stats( tokenized_name( spell ) + suffix( item ) ) )
      .activated( false );

  effect.rppm_scale = RPPM_HASTE;
  effect.custom_buff = buff;

  new dbc_proc_callback_t( item, effect );
}

void enchant::rivers_song( special_effect_t& effect,
                           const item_t& item,
                           const special_effect_db_item_t& /* dbitem */ )
{
  const spell_data_t* spell = item.player -> find_spell( 116660 );

  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( item.player, tokenized_name( spell ) ) );
    
  if ( ! buff )
    buff = stat_buff_creator_t( item.player, tokenized_name( spell ), spell )
           .activated( false );

  effect.rppm_scale = RPPM_HASTE;
  effect.custom_buff = buff;

  new dbc_proc_callback_t( item, effect );
}

void enchant::dancing_steel( special_effect_t& effect, 
                             const item_t& item,
                             const special_effect_db_item_t& dbitem )
{
  // Account for Bloody Dancing Steel and Dancing Steel buffs
  const spell_data_t* spell = item.player -> find_spell( dbitem.spell_id == 142531 ? 142530 : 120032 );

  double value = spell -> effectN( 1 ).average( item.player );
  
  stat_buff_t* buff  = stat_buff_creator_t( item.player, tokenized_name( spell ) + suffix( item ), spell )
                       .activated( false )
                       .add_stat( STAT_STRENGTH, value, select_attr<std::greater>() )
                       .add_stat( STAT_AGILITY,  value, select_attr<std::greater>() );

  effect.custom_buff = buff;
  
  new dbc_proc_callback_t( item, effect );
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
                           const special_effect_db_item_t& )
{
  const spell_data_t* spell = item.player -> find_spell( 104993 );

  double int_value = spell -> effectN( 1 ).average( item.player );
  double spi_value = spell -> effectN( 2 ).average( item.player );

  // Set trigger spell here, so special_effect_t::name() returns a pretty name
  // for the custom buff.
  effect.trigger_spell_id = 104993;
  
  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( item.player, effect.name() ) );
  if ( ! buff ) 
    buff = stat_buff_creator_t( item.player, effect.name(), spell )
           .activated( false )
           .add_stat( STAT_INTELLECT, int_value )
           .add_stat( STAT_SPIRIT, spi_value, jade_spirit_check_func() );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( item.player, effect );
}

struct windsong_callback_t : public dbc_proc_callback_t
{
  stat_buff_t* haste, *crit, *mastery;

  windsong_callback_t( const item_t& i,
                       const special_effect_t& effect,
                       stat_buff_t* hb,
                       stat_buff_t* cb,
                       stat_buff_t* mb ) :
                 dbc_proc_callback_t( i, effect ),
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
                        const special_effect_db_item_t& /* dbitem */ )
{
  const spell_data_t* mastery = item.player -> find_spell( 104510 );
  const spell_data_t* haste = item.player -> find_spell( 104423 );
  const spell_data_t* crit = item.player -> find_spell( 104509 );

  stat_buff_t* mastery_buff = stat_buff_creator_t( item.player, "windsong_mastery" + suffix( item ), mastery )
                              .activated( false );
  stat_buff_t* haste_buff   = stat_buff_creator_t( item.player, "windsong_haste" + suffix( item ), haste )
                              .activated( false );
  stat_buff_t* crit_buff    = stat_buff_creator_t( item.player, "windsong_crit" + suffix( item ), crit )
                              .activated( false );

  //effect.name_str = tokenized_name( mastery ) + suffix( item );

  new windsong_callback_t( item, effect, haste_buff, crit_buff, mastery_buff );

}

struct hurricane_spell_proc_t : public dbc_proc_callback_t
{
  buff_t *mh_buff, *oh_buff, *s_buff;

  hurricane_spell_proc_t( player_t* p, const special_effect_t& effect, buff_t* mhb, buff_t* ohb, buff_t* sb ) :
    dbc_proc_callback_t( p, effect ),
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
  // (opposite hand weapon buff has not been created), bail out early.  Note
  // that this presumes that the spell item enchant has the procs spell ids in
  // correct order (which they are, at the moment).
  if ( n_hurricane_enchants == 2 && ( ! mh_buff || ! oh_buff ) )
    return;

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();
  stat_buff_t* spell_buff = stat_buff_creator_t( item.player, "Hurricane_spell", spell )
                            .activated( false );

  new hurricane_spell_proc_t( item.player, effect, mh_buff, oh_buff, spell_buff );

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
  effect.ppm_ = 1.0;

  effect.custom_buff = buff;

  new dbc_proc_callback_t( item, effect );
}

// Profession perks =========================================================

struct nitro_boosts_action_t : public action_t
{
  buff_t* buff;
  
  nitro_boosts_action_t( player_t* p ) : action_t( ACTION_USE, "nitro_boosts", p )
  { background = true; }

  virtual void execute()
  {
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    player -> buffs.nitro_boosts-> trigger();
  }

};

void profession::nitro_boosts( special_effect_t& effect, 
                               const item_t& item,
                               const special_effect_db_item_t& )
{
  // TODO-WOD: This is stupid, need to find a better way to implement custom on-use items
  effect.type = SPECIAL_EFFECT_USE;
  effect.execute_action = new nitro_boosts_action_t( item.player );
};

void profession::zen_alchemist_stone( special_effect_t& effect,
                                      const item_t& item,
                                      const special_effect_db_item_t& /* dbitem */ )
{
  struct zen_alchemist_stone_callback : public dbc_proc_callback_t
  {
    stat_buff_t* buff_str;
    stat_buff_t* buff_agi;
    stat_buff_t* buff_int;

    zen_alchemist_stone_callback( const item_t& i, const special_effect_t& data ) :
      dbc_proc_callback_t( i.player, data )
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

    virtual void execute( action_t* a, action_state_t* /* state */ ) override
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

  new zen_alchemist_stone_callback( item, effect );
}

void gem::sinister_primal( special_effect_t& effect, 
                           const item_t& item,
                           const special_effect_db_item_t& /* dbitem */ )
{
  if ( item.sim -> challenge_mode )
    return;

  effect.custom_buff = item.player->buffs.tempus_repit;

  new dbc_proc_callback_t( item, effect );
}

void gem::indomitable_primal( special_effect_t& effect, 
                              const item_t& item,
                              const special_effect_db_item_t& /* dbitem */ )
{
  if ( item.sim -> challenge_mode )
    return;

  effect.custom_buff = item.player -> buffs.fortitude;

  new dbc_proc_callback_t( item, effect );
}

void gem::capacitive_primal( special_effect_t& effect, 
                             const item_t& item,
                             const special_effect_db_item_t& /* dbitem */ )
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
    }
  };

  struct capacitive_primal_proc_t : public dbc_proc_callback_t
  {
    capacitive_primal_proc_t( const item_t& i, const special_effect_t& data ) :
      dbc_proc_callback_t( i, data )
    {

    }

    virtual void initialize() override
    {
      dbc_proc_callback_t::initialize();
      // Unfortunately the weapon-based RPPM modifiers have to be hardcoded,
      // as they will not show on the client tooltip data.
      if ( listener->main_hand_weapon.group() != WEAPON_2H )
      {
        if ( listener->specialization() == WARRIOR_FURY )
          rppm.set_modifier( 1.152 );
        else if ( listener->specialization() == DEATH_KNIGHT_FROST )
          rppm.set_modifier( 1.134 );
      }
    }

    void trigger( action_t* action, void* call_data )
    {
      // Flurry of Xuen and Capacitance cannot proc Capacitance
      if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
        return;

      dbc_proc_callback_t::trigger( action, call_data );
    }
  };

  player_t* p = item.player;

  // Stacking Buff
  effect.custom_buff = buff_creator_t( p, "capacitance", p -> find_spell( 137596 ) );
  effect.rppm_scale = RPPM_HASTE;


  // Execute Action
  action_t* ls = p -> create_proc_action( "lightning_strike" );
  if ( ! ls )
    ls = new lightning_strike_t( p );
  effect.execute_action = ls;

  new capacitive_primal_proc_t( item, effect );
}

void gem::courageous_primal( special_effect_t& effect, 
                             const item_t& item,
                             const special_effect_db_item_t& /* dbitem */ )
{
  if ( item.sim -> challenge_mode )
    return;

  struct courageous_primal_proc_t : public dbc_proc_callback_t
  {
    courageous_primal_proc_t( player_t* p, const special_effect_t& data ) :
      dbc_proc_callback_t( p, data )
    { }

    virtual void trigger( action_t* action, void* call_data ) override
    {
      spell_base_t* spell = debug_cast<spell_base_t*>( action );
      if ( ! spell -> procs_courageous_primal_diamond )
        return;

      dbc_proc_callback_t::trigger( action, call_data );
    }
  };

  effect.custom_buff = item.player -> buffs.courageous_primal_diamond_lucidity;

  new courageous_primal_proc_t( item.player, effect );
}

// Items ====================================================================

void item::rune_of_reorigination( special_effect_t& effect,
                                  const item_t& item,
                                  const special_effect_db_item_t& dbitem )
{
  struct rune_of_reorigination_callback_t : public dbc_proc_callback_t
  {
    enum
    {
      BUFF_CRIT = 0,
      BUFF_HASTE,
      BUFF_MASTERY
    };

    stat_buff_t* buff;

    rune_of_reorigination_callback_t( player_t* player, const special_effect_t& data ) :
      dbc_proc_callback_t( player, data )
    {
      buff = static_cast< stat_buff_t* >( effect.custom_buff );
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

  std::string buff_name = spell -> name_cstr();
  util::tokenize( buff_name );

  stat_buff_t* buff = stat_buff_creator_t( item.player, buff_name, spell )
                      .activated( false )
                      .add_stat( STAT_CRIT_RATING, 0 )
                      .add_stat( STAT_HASTE_RATING, 0 )
                      .add_stat( STAT_MASTERY_RATING, 0 );

  effect.custom_buff  = buff;
  effect.ppm_         = -1.0 * driver -> real_ppm();
  effect.ppm_        *= item_database::approx_scale_coefficient( 528, item.item_level() );

  new rune_of_reorigination_callback_t( item.player, effect );
}

void item::spark_of_zandalar( special_effect_t& effect,
                              const item_t& item,
                              const special_effect_db_item_t& /* dbitem */ )
{
  maintenance_check( 502 );

  const spell_data_t* buff = item.player -> find_spell( 138960 );

  std::string buff_name = buff -> name_cstr();
  util::tokenize( buff_name );

  stat_buff_t* b = stat_buff_creator_t( item.player, buff_name, buff, &item );

  effect.custom_buff = b;

  struct spark_of_zandalar_callback_t : public dbc_proc_callback_t
  {
    buff_t*      sparks;
    stat_buff_t* buff;

    spark_of_zandalar_callback_t( const item_t& i, const special_effect_t& data ) :
      dbc_proc_callback_t( i, data )
    {
      const spell_data_t* spell = listener -> find_spell( 138958 );
      sparks = buff_creator_t( listener, "zandalari_spark_driver", spell )
               .quiet( true );
    }

    void execute( action_t* /* action */, action_state_t* /* state */ )
    {
      sparks -> trigger();

      if ( sparks -> stack() == sparks -> max_stack() )
      {
        sparks -> expire();
        proc_buff -> trigger();
      }
    }
  };

  new spark_of_zandalar_callback_t( item, effect );
};

void item::unerring_vision_of_leishen( special_effect_t& effect,
                                       const item_t& item,
                                       const special_effect_db_item_t& dbitem )
{
  struct perfect_aim_buff_t : public buff_t
  {
    perfect_aim_buff_t( player_t* p ) :
      buff_t( buff_creator_t( p, "perfect_aim", p -> find_spell( 138963 ) ).activated( false ) )
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

  struct unerring_vision_of_leishen_callback_t : public dbc_proc_callback_t
  {
    unerring_vision_of_leishen_callback_t( const item_t& i, const special_effect_t& data ) :
      dbc_proc_callback_t( i.player, data )
    { }

    void initialize()
    {
      dbc_proc_callback_t::initialize();

      // Warlocks have a (hidden?) 0.6 modifier, not showing in DBCs.
      if ( listener -> type == WARLOCK )
        rppm.set_modifier( 0.6 );
    }
  };

  maintenance_check( 502 );

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );

  effect.ppm_         = -1.0 * driver -> real_ppm();
  effect.ppm_        *= item_database::approx_scale_coefficient( 528, item.item_level() );
  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.custom_buff  = new perfect_aim_buff_t( item.player );

  new unerring_vision_of_leishen_callback_t( item, effect );
}

void item::skeers_bloodsoaked_talisman( special_effect_t& effect,
                                        const item_t& item,
                                        const special_effect_db_item_t& dbitem )
{
  maintenance_check( 528 );

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();
  // Aura is hidden, thre's no linkage in spell data actual
  const spell_data_t* buff = item.player -> find_spell( 146293 );

  std::string buff_name = buff -> name_cstr();
  util::tokenize( buff_name );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = stat_buff_creator_t( item.player, buff_name, buff )
                   .add_stat( STAT_CRIT_RATING, spell -> effectN( 1 ).average( item ) )
                   .tick_behavior( BUFF_TICK_CLIP )
                   .period( spell -> effectN( 1 ).period() )
                   .duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( item.player, effect );
}

void item::black_blood_of_yshaarj( special_effect_t& effect,
                                   const item_t& item,
                                   const special_effect_db_item_t& dbitem )
{
  maintenance_check( 528 );

  const spell_data_t* driver = item.player -> find_spell( dbitem.spell_id );
  const spell_data_t* ticker = driver -> effectN( 1 ).trigger();
  const spell_data_t* buff = item.player -> find_spell( 146202 );

  std::string buff_name = buff -> name_cstr();
  util::tokenize( buff_name );

  stat_buff_t* b = stat_buff_creator_t( item.player, buff_name, buff )
                   .add_stat( STAT_INTELLECT, ticker -> effectN( 1 ).average( item ) )
                   .tick_behavior( BUFF_TICK_CLIP )
                   .period( ticker -> effectN( 1 ).period() )
                   .duration( ticker -> duration () );

  effect.custom_buff = b;

  new dbc_proc_callback_t( item.player, effect );
}

struct flurry_of_xuen_melee_t : public attack_t
{
  flurry_of_xuen_melee_t( player_t* player ) : 
    attack_t( "flurry_of_xuen", player, player -> find_spell( 147891 ) )
  {
    background = true;
    proc = false;
    aoe = 5;
    special = may_miss = may_parry = may_block = may_dodge = may_crit = true;
  }
};

struct flurry_of_xuen_ranged_t : public ranged_attack_t
{
  flurry_of_xuen_ranged_t( player_t* player ) : 
    ranged_attack_t( "flurry_of_xuen", player, player -> find_spell( 147891 ) )
  {
    // TODO: check attack_power_mod.direct = data().extra_coeff();
    background = true;
    proc = false;
    aoe = 5;
    special = may_miss = may_parry = may_block = may_dodge = may_crit = true;
  }
};

struct flurry_of_xuen_driver_t : public attack_t
{
  action_t* ac;

  flurry_of_xuen_driver_t( player_t* player, action_t* action = 0 ) :
    attack_t( "flurry_of_xuen_driver", player, player -> find_spell( 146194 ) ),
    ac( 0 )
  {
    hasted_ticks = may_crit = may_miss = may_dodge = may_parry = callbacks = false;
    proc = background = dual = true;

    if ( ! action )
    {
      if ( player -> type == HUNTER )
        ac = new flurry_of_xuen_ranged_t( player );
      else
        ac = new flurry_of_xuen_melee_t( player );
    }
    else
      ac = action;
  }

  // Don't use tick action here, so we can get class specific snapshotting, if 
  // there is a custom proc action crated. Hack and workaround and ugly.
  void tick( dot_t* dot )
  {
    if ( sim -> debug )
      sim -> out_debug.printf( "%s ticks (%d of %d)", name(), dot -> current_tick, dot -> num_ticks );

    if ( ac )
      ac -> schedule_execute();

    player -> trigger_ready();
  }
};

struct flurry_of_xuen_cb_t : public dbc_proc_callback_t
{
  flurry_of_xuen_cb_t( player_t* p, const special_effect_t& effect ) :
    dbc_proc_callback_t( p, effect )
  { }

  void trigger( action_t* action, void* call_data )
  {
    // Flurry of Xuen, and Lightning Strike cannot proc Flurry of Xuen
    if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
      return;

    dbc_proc_callback_t::trigger( action, call_data );
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

  effect.ppm_        = -1.0 * driver -> real_ppm();
  effect.ppm_       *= item_database::approx_scale_coefficient( item.parsed.data.level, item.item_level() );
  effect.rppm_scale = RPPM_HASTE;
  effect.execute_action = new flurry_of_xuen_driver_t( p, p -> create_proc_action( effect.name() ) );

  new flurry_of_xuen_cb_t( p, effect );
}

struct essence_of_yulon_t : public spell_t
{
  essence_of_yulon_t( player_t* p, const spell_data_t& driver ) :
    spell_t( "essence_of_yulon", p, p -> find_spell( 148008 ) )
  {
    background = may_crit = true;
    proc = false;
    aoe = 5;
    spell_power_mod.direct /= driver.duration().total_seconds() + 1;
  }
};

struct essence_of_yulon_driver_t : public spell_t
{
  essence_of_yulon_driver_t( player_t* player ) :
    spell_t( "essence_of_yulon", player, player -> find_spell( 146198 ) )
  {
    hasted_ticks = may_miss = may_dodge = may_parry = may_block = callbacks = may_crit = false;
    tick_zero = proc = background = dual = true;
    travel_speed = 0;

    tick_action = new essence_of_yulon_t( player, data() );
    dynamic_tick_action = true;
  }
};

struct essence_of_yulon_cb_t : public dbc_proc_callback_t
{

  essence_of_yulon_cb_t( player_t* p, const special_effect_t& effect ) :
    dbc_proc_callback_t( p, effect )
  { }

  void trigger( action_t* action, void* call_data )
  {
    if ( action -> id == 148008 ) // dot direct damage ticks can't proc itself
      return;

    dbc_proc_callback_t::trigger( action, call_data );
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

  effect.ppm_         = -1.0 * driver -> real_ppm();
  effect.ppm_        *= item_database::approx_scale_coefficient( item.parsed.data.level, item.item_level() );
  effect.rppm_scale  = RPPM_HASTE;
  effect.execute_action = new essence_of_yulon_driver_t( item.player );

  new essence_of_yulon_cb_t( item.player, effect );
}

void item::endurance_of_niuzao( special_effect_t& /* effect */,
                                const item_t& item, 
                                const special_effect_db_item_t& /* dbitem */ )
{
  maintenance_check( 600 );

  if ( item.sim -> challenge_mode )
    return;

  const spell_data_t* cd = item.player -> find_spell( 148010 );

  item.player -> legendary_tank_cloak_cd = item.player -> get_cooldown( "endurance_of_niuzao" );
  item.player -> legendary_tank_cloak_cd -> duration = cd -> duration();
}

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
      for ( size_t i = 0; i < encoded_options.length(); i++ )
        encoded_options[ i ] = std::tolower( encoded_options[ i ] );
      // Note, if the encoding parse fails (this should never ever happen), 
      // we don't parse game client data either.
      if ( ! proc::parse_special_effect_encoding( effect, item, encoded_options ) )
        return false;
    }
  }

  // Setup the driver always, though honoring any parsing performed in the 
  // first phase options.
  if ( effect.spell_id == 0 )
    effect.spell_id = spell_id;

  if ( effect.spell_id > 0 && item.player -> find_spell( effect.spell_id ) -> id() != effect.spell_id )
  {
    if ( item.sim -> debug )
      item.sim -> out_debug.printf( "Player %s unable to initialize special effect in item %s, spell identifier %u not found.", item.player -> name(), item.name(), effect.spell_id );
    effect.type = SPECIAL_EFFECT_NONE;
    return ret;
  }

  // For generic procs, make sure we have a PPM, RPPM or Proc Chance available,
  // otherwise there's no point in trying to proc anything
  if ( effect.type == SPECIAL_EFFECT_EQUIP && ! proc::usable_proc( effect ) )
    effect.type = SPECIAL_EFFECT_NONE;
  // For now only allow on-use stat buffs.
  else if ( effect.type == SPECIAL_EFFECT_USE && effect.buff_type() != SPECIAL_EFFECT_BUFF_STAT )
    effect.type = SPECIAL_EFFECT_NONE;

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
      if ( p -> sim -> debug )
        p -> sim -> out_debug.printf( "Initializing special effect %s", effect.to_string().c_str() );

      if ( effect.type == SPECIAL_EFFECT_CUSTOM )
      {
        assert( effect.spell_id > 0 );
        const special_effect_db_item_t& dbitem = find_special_effect_db_item( effect.spell_id );
        assert( dbitem.custom_cb != 0 );
        dbitem.custom_cb( effect, item, dbitem );
      }
      else if ( effect.type == SPECIAL_EFFECT_EQUIP )
        new dbc_proc_callback_t( item, effect );
    }
  }
}
