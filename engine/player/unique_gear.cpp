// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "unique_gear.hpp"

#include "dbc/racial_spells.hpp"
#include "player/scaling_metric_data.hpp"
#include "player/soulbinds.hpp"
#include "sc_enums.hpp"
#include "sim/expressions.hpp"
#include "unique_gear_dragonflight.hpp"
#include "unique_gear_shadowlands.hpp"
#include "unique_gear_thewarwithin.hpp"
#include "util/util.hpp"

#include <cctype>
#include <memory>

#include "simulationcraft.hpp"

using namespace unique_gear;

#define maintenance_check( ilvl ) static_assert( (ilvl) >= 90, "unique item below min level, should be deprecated." )

namespace { // UNNAMED NAMESPACE

/**
 * Forward declarations so we can reorganize the file a bit more sanely.
 */

namespace enchants
{
  /* Legacy Enchants */
  void executioner( special_effect_t& );
  void hurricane_spell( special_effect_t& );

  /* Mists of Pandaria */
  void dancing_steel( special_effect_t& );
  void jade_spirit( special_effect_t& );
  void windsong( special_effect_t& );
  void rivers_song( special_effect_t& );
  void colossus( special_effect_t& );

  /* Warlords of Draenor */
  void hemets_heartseeker( special_effect_t& );
  void mark_of_bleeding_hollow( special_effect_t& );
  void mark_of_the_thunderlord( special_effect_t& );
  void mark_of_the_shattered_hand( special_effect_t& );
  void mark_of_the_frostwolf( special_effect_t& );
  void mark_of_blackrock( special_effect_t& );
  void mark_of_warsong( special_effect_t& );
  void megawatt_filament( special_effect_t& );
  void oglethorpes_missile_splitter( special_effect_t& );
}

namespace profession
{
  void nitro_boosts( special_effect_t& );
  void grounded_plasma_shield( special_effect_t& );
  void zen_alchemist_stone( special_effect_t& );
  void draenor_philosophers_stone( special_effect_t& );
}

namespace item
{
  /* Misc */
  void heartpierce( special_effect_t& );
  void darkmoon_card_greatness( special_effect_t& );
  void vial_of_shadows( special_effect_t& );
  void deathbringers_will( special_effect_t& );
  void cunning_of_the_cruel( special_effect_t& );
  void felmouth_frenzy( special_effect_t& );
  void matrix_restabilizer( special_effect_t& );
  void blazefury_medallion( special_effect_t& );

  /* Mists of Pandaria 5.2 */
  void rune_of_reorigination( special_effect_t& );
  void spark_of_zandalar( special_effect_t& );
  void unerring_vision_of_leishen( special_effect_t& );

  /* Mists of Pandaria 5.4 */
  void amplification( special_effect_t& );
  void black_blood_of_yshaarj( special_effect_t& );
  void cleave( special_effect_t& );
  void essence_of_yulon( special_effect_t& );
  void flurry_of_xuen( special_effect_t& );
  void prismatic_prison_of_pride( special_effect_t& );
  void purified_bindings_of_immerseus( special_effect_t& );
  void readiness( special_effect_t& );
  void skeers_bloodsoaked_talisman( special_effect_t& );
  void thoks_tail_tip( special_effect_t& );

  /* Warlords of Draenor 6.0 */
  void autorepairing_autoclave( special_effect_t& );
  void battering_talisman_trigger( special_effect_t& );
  void blackiron_micro_crucible( special_effect_t& );
  void forgemasters_insignia( special_effect_t& );
  void humming_blackiron_trigger( special_effect_t& );
  void spellbound_runic_band( special_effect_t& );
  void spellbound_solium_band( special_effect_t& );
  void legendary_ring( special_effect_t& );

  /* Warlords of Draenor 6.2 */
  void discordant_chorus( special_effect_t& );
  void empty_drinking_horn( special_effect_t& );
  void insatiable_hunger( special_effect_t& );
  void mirror_of_the_blademaster( special_effect_t& );
  void prophecy_of_fear( special_effect_t& );
  void soul_capacitor( special_effect_t& );
  void tyrants_decree( special_effect_t& );
  void unblinking_gaze_of_sethe( special_effect_t& );
  void warlords_unseeing_eye( special_effect_t& );
  void gronntooth_war_horn( special_effect_t& );
  void orb_of_voidsight( special_effect_t& );
  void infallible_tracking_charm( special_effect_t& );
  void witherbarks_branch( special_effect_t& );

  /* Timewalking Trinkets */
  void necromantic_focus( special_effect_t& );
  void sorrowsong( special_effect_t& );
}

namespace gem
{
  void capacitive_primal( special_effect_t& );
  void courageous_primal( special_effect_t& );
  void indomitable_primal( special_effect_t& );
  void sinister_primal( special_effect_t& );
}

namespace set_bonus
{
  // Generic passive stat aura adder for set bonuses
  void passive_stat_aura( special_effect_t& );
}

namespace racial
{
  void touch_of_the_grave( special_effect_t& );
  void entropic_embrace( special_effect_t& );
  void brush_it_off( special_effect_t& );
  void zandalari_loa( special_effect_t& );
  void combat_analysis( special_effect_t& );
}

namespace generic
{
  void skyfury( special_effect_t& );
  void enable_all_item_effects( special_effect_t& );
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
    stat_e compare_stat = STAT_NONE;
    stat_e my_stat = STAT_NONE;

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

std::string suffix( const item_t* item )
{
  assert( item );
  if ( item -> slot == SLOT_OFF_HAND )
    return "_oh";
  return "";
}

std::string tokenized_name( const spell_data_t* data )
{
  return util::tokenize_fn( data -> name_cstr() );
}

// Enchants ================================================================

void enchants::mark_of_bleeding_hollow( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 173322;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchants::megawatt_filament( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 156060;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchants::oglethorpes_missile_splitter( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 156055;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchants::hemets_heartseeker( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 173288;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchants::mark_of_blackrock( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_EQUIP;

  struct mob_proc_callback_t : public dbc_proc_callback_t
  {
    mob_proc_callback_t( const item_t* i, const special_effect_t& effect ) :
      dbc_proc_callback_t( i, effect )
    { }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( listener -> resources.pct( RESOURCE_HEALTH ) >= 0.6 )
        return;

      dbc_proc_callback_t::trigger( a, s );
    }
  };

  new mob_proc_callback_t( effect.item, effect );
}

void enchants::mark_of_warsong( special_effect_t& effect )
{
  // Custom callback to help the special effect initialization, we can use
  // generic initialization for the enchant, but the game client data does not
  // link driver to the procced spell, so we do it here.

  effect.type = SPECIAL_EFFECT_EQUIP;
  effect.trigger_spell_id = 159675;
  effect.reverse = true;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchants::mark_of_the_thunderlord( special_effect_t& effect )
{
  struct mott_buff_t : public stat_buff_t
  {
    unsigned extensions;
    unsigned max_extensions;

    mott_buff_t( const item_t* item, const std::string& name, unsigned max_ext ) :
      stat_buff_t( item -> player, name, item -> player -> find_spell( 159234 ) ),
      extensions( 0 ), max_extensions( max_ext )
    { }

    void extend_duration( player_t* p, timespan_t extend_duration ) override
    {
      if ( extensions < max_extensions )
      {
        stat_buff_t::extend_duration( p, extend_duration );
        extensions++;
      }
    }

    void execute( int stacks, double value, timespan_t duration ) override
    { stat_buff_t::execute( stacks, value, duration ); extensions = 0; }

    void reset() override
    { stat_buff_t::reset(); extensions = 0; }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    { stat_buff_t::expire_override( expiration_stacks, remaining_duration ); extensions = 0; }
  };

  buff_t* b = buff_t::find( effect.player, effect.name() );
  if ( ! b )
  {
    b = new mott_buff_t( effect.item, effect.name(), 3 );
  }

  // Max extensions is hardcoded, no spell data to fetch it
  effect.custom_buff = b;

  // Setup another proc callback, that uses the same driver as the proc that
  // triggers the buff, however it only procs on crits. This callback will
  // extend the duration of the buff, only if the buff is up. The extension
  // capping is handled in the buff itself.
  special_effect_t* effect2 = new special_effect_t( effect.item );
  effect2 -> name_str = effect.name() + "_crit_driver";
  effect2 -> proc_chance_ = 1;
  effect2 -> ppm_ = 0;
  effect2 -> spell_id = effect.spell_id;
  effect2 -> custom_buff = effect.custom_buff;
  effect2 -> cooldown_ = timespan_t::zero();
  effect2 -> proc_flags2_ = PF2_CRIT;

  // Inject the crit driver into the item special effects, so it is conceptually in the "right
  // place". Rather ugly, but this works, better would probably be just to make
  // special_effect_t::item pointer not const, but that is a more extensive change.
  item_t& item = const_cast<item_t&>( *effect.item );
  item.parsed.special_effects.push_back( effect2 );

  struct mott_crit_callback_t : public dbc_proc_callback_t
  {
    mott_crit_callback_t( const item_t* item, const special_effect_t& effect ) :
      dbc_proc_callback_t( item, effect )
    { }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( proc_buff -> check() )
      {
        dbc_proc_callback_t::trigger( a, s );
      }
    }

    void execute( action_t*, action_state_t* ) override
    {
      if ( proc_buff -> check() )
        proc_buff -> extend_duration( listener, timespan_t::from_seconds( 2 ) );
    }
  };

  new dbc_proc_callback_t( effect.item, effect );
  new mott_crit_callback_t( effect.item, *effect.item -> parsed.special_effects.back() );
}

void enchants::mark_of_the_frostwolf( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_EQUIP;

  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( effect.player, "mark_of_the_frostwolf" ) );
  if ( ! buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "mark_of_the_frostwolf", effect.player -> find_spell( 159676 ) );
  }
  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchants::mark_of_the_shattered_hand( special_effect_t& effect )
{
  effect.trigger_spell_id = 159238;
  effect.name_str = effect.item -> player -> find_spell( 159238 ) -> name_cstr();

  struct bleed_attack_t : public attack_t
  {
    bleed_attack_t( player_t* p, const special_effect_t& effect ) :
      attack_t( effect.name(), p, p -> find_spell( effect.trigger_spell_id ) )
    {
      hasted_ticks = false; background = true; callbacks = false; special = true;
      may_miss = may_block = may_dodge = may_parry = false; may_crit = true;
      tick_may_crit = false;
    }

    double composite_target_armor( player_t* ) const override
    { return 0.0; }
  };

  action_t* bleed = effect.player -> find_action( "shattered_bleed" );
  if ( ! bleed )
  {
    bleed = effect.item -> player -> create_proc_action( "shattered_bleed", effect );
  }

  if ( ! bleed )
  {
    bleed = new bleed_attack_t( effect.item -> player, effect );
  }

  effect.execute_action = bleed;
  effect.proc_flags_ = PF_ALL_DAMAGE; // DBC says procs off heals, but that's nothing but trouble.

  new dbc_proc_callback_t( effect.item, effect );
}

void enchants::colossus( special_effect_t& effect )
{
  const spell_data_t* spell = effect.item -> player-> find_spell( 116631 );

  auto buff = make_buff<absorb_buff_t>( effect.item -> player,
                             tokenized_name( spell ) + suffix( effect.item ),
                             spell );
  buff->set_absorb_source( effect.item -> player -> get_stats( tokenized_name( spell ) + suffix( effect.item ) ) )
      ->set_activated( false );

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchants::rivers_song( special_effect_t& effect )
{
  const spell_data_t* spell = effect.item -> player -> find_spell( 116660 );

  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( effect.item -> player, tokenized_name( spell ) ) );

  if ( ! buff )
  {
    buff = make_buff<stat_buff_t>( effect.item -> player, tokenized_name( spell ), spell );
    buff->set_activated( false );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

void enchants::dancing_steel( special_effect_t& effect )
{
  // Account for Bloody Dancing Steel and Dancing Steel buffs
  const spell_data_t* spell = effect.item -> player -> find_spell( effect.spell_id == 142531 ? 142530 : 120032 );

  double value = spell -> effectN( 1 ).average( effect.item -> player );

  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( effect.item -> player, effect.name() ) );
  if ( ! buff )
  {
    buff = make_buff<stat_buff_t>( effect.item -> player, effect.name(), spell );
    buff->add_stat( STAT_STRENGTH, value, select_attr<std::greater>() )
        ->add_stat( STAT_AGILITY,  value, select_attr<std::greater>() )
        ->set_activated( false );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
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

void enchants::jade_spirit( special_effect_t& effect )
{
  const spell_data_t* spell = effect.item -> player -> find_spell( 104993 );

  double int_value = spell -> effectN( 1 ).average( effect.item -> player );
  double spi_value = spell -> effectN( 2 ).average( effect.item -> player );

  // Set trigger spell here, so special_effect_t::name() returns a pretty name
  // for the custom buff.
  effect.trigger_spell_id = 104993;

  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( effect.item -> player, effect.name() ) );
  if ( ! buff )
  {
    buff = make_buff<stat_buff_t>( effect.item -> player, effect.name(), spell );
    buff->add_stat( STAT_INTELLECT, int_value )
        ->add_stat( STAT_SPIRIT, spi_value, jade_spirit_check_func() )
        ->set_activated( false );
  }

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

struct windsong_callback_t : public dbc_proc_callback_t
{
  stat_buff_t* haste, *crit, *mastery;

  windsong_callback_t( const item_t* i,
                       const special_effect_t& effect,
                       stat_buff_t* hb,
                       stat_buff_t* cb,
                       stat_buff_t* mb ) :
                 dbc_proc_callback_t( i, effect ),
    haste( hb ), crit( cb ), mastery( mb )
  { }

  void execute( action_t* /* a */, action_state_t* /* call_data */ ) override
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

void enchants::windsong( special_effect_t& effect )
{
  const spell_data_t* mastery = effect.item -> player -> find_spell( 104510 );
  const spell_data_t* haste = effect.item -> player -> find_spell( 104423 );
  const spell_data_t* crit = effect.item -> player -> find_spell( 104509 );

  stat_buff_t* mastery_buff = make_buff<stat_buff_t>( effect.item -> player, "windsong_mastery" + suffix( effect.item ), mastery );
  mastery_buff->set_activated( false );
  stat_buff_t* haste_buff   = make_buff<stat_buff_t>( effect.item -> player, "windsong_haste" + suffix( effect.item ), haste );
  haste_buff->set_activated( false );
  stat_buff_t* crit_buff    = make_buff<stat_buff_t>( effect.item -> player, "windsong_crit" + suffix( effect.item ), crit );
  crit_buff->set_activated( false );

  //effect.name_str = tokenized_name( mastery ) + suffix( item );

  new windsong_callback_t( effect.item, effect, haste_buff, crit_buff, mastery_buff );
}

struct hurricane_spell_proc_t : public dbc_proc_callback_t
{
  buff_t *mh_buff, *oh_buff, *s_buff;

  hurricane_spell_proc_t( const special_effect_t& effect, buff_t* mhb, buff_t* ohb, buff_t* sb ) :
    dbc_proc_callback_t( effect.item -> player, effect ),
    mh_buff( mhb ), oh_buff( ohb ), s_buff( sb )
  { }

  void execute( action_t* /* a */, action_state_t* /* call_data */ ) override
  {
    if ( mh_buff && mh_buff -> check() )
      mh_buff -> trigger();
    else if ( oh_buff && oh_buff -> check() )
      oh_buff -> trigger();
    else
      s_buff -> trigger();
  }
};

void enchants::hurricane_spell( special_effect_t& effect )
{
  int n_hurricane_enchants = 0;

  if ( effect.item -> player -> items[ SLOT_MAIN_HAND ].parsed.enchant_id == 4083 ||
       util::str_compare_ci( effect.item -> player -> items[ SLOT_MAIN_HAND ].option_enchant_str, "hurricane" ) )
    n_hurricane_enchants++;

  if ( effect.item -> player -> items[ SLOT_OFF_HAND ].parsed.enchant_id == 4083 ||
       util::str_compare_ci( effect.item -> player -> items[ SLOT_OFF_HAND ].option_enchant_str, "hurricane" ) )
    n_hurricane_enchants++;

  buff_t* mh_buff = buff_t::find( effect.item -> player, "hurricane" );
  buff_t* oh_buff = buff_t::find( effect.item -> player, "hurricane_oh" );

  // If we have 2 hurricane enchants, and we're creating the first one
  // (opposite hand weapon buff has not been created), bail out early.  Note
  // that this presumes that the spell item enchant has the procs spell ids in
  // correct order (which they are, at the moment).
  if ( n_hurricane_enchants == 2 && ( ! mh_buff || ! oh_buff ) )
    return;

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();
  stat_buff_t* spell_buff = make_buff<stat_buff_t>( effect.item -> player, "hurricane_spell", spell );
  spell_buff->set_activated( false );

  new hurricane_spell_proc_t( effect, mh_buff, oh_buff, spell_buff );

}

void enchants::executioner( special_effect_t& effect )
{
  const spell_data_t* spell = effect.item -> player -> find_spell( effect.spell_id );
  stat_buff_t* buff = static_cast<stat_buff_t*>( buff_t::find( effect.item -> player, tokenized_name( spell ) ) );

  if ( ! buff )
  {
    buff = make_buff<stat_buff_t>( effect.item -> player, tokenized_name( spell ), spell );
    buff->set_activated( false );
  }

  effect.name_str = tokenized_name( spell );
  effect.ppm_ = 1.0;

  effect.custom_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

// Profession perks =========================================================

struct engineering_effect_t : public action_t
{
  engineering_effect_t( player_t* p, const std::string& name ) :
    action_t( ACTION_USE, name, p )
  {
    background = true;
    callbacks = false;
    // All engineering on-use addons share a cooldown with potions.
    cooldown = p -> get_cooldown( "potion" );
  }

  // Use similar cooldown handling to potions, see sc_consumable.cpp dbc_potion_t
  void update_ready( timespan_t cd_duration ) override
  {
    // If the player is in combat, just make a very long CD
    if ( player -> in_combat )
      cd_duration = sim -> max_time * 3;
    else
      cd_duration = cooldown -> duration;

    action_t::update_ready( cd_duration );
  }

  result_e calculate_result( action_state_t* /* state */ ) const override
  { return RESULT_HIT; }

  void execute() override
  {
    action_t::execute();
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );
  }
};

struct nitro_boosts_action_t : public engineering_effect_t
{
  nitro_boosts_action_t( player_t* p ) :
    engineering_effect_t( p, "nitro_boosts" )
  {
    if ( !p->buffs.nitro_boosts )
      p->buffs.nitro_boosts = make_buff( p, "nitro_boosts", p->find_spell( 54861 ) );
  }

  void execute() override
  {
    engineering_effect_t::execute();

    player -> buffs.nitro_boosts-> trigger();
  }
};

struct grounded_plasma_shield_t : public engineering_effect_t
{
  absorb_buff_t* buff;

  grounded_plasma_shield_t( player_t* p ) :
    engineering_effect_t( p, "grounded_plasma_shield" )
  {
    buff = make_buff<absorb_buff_t>( p, "grounded_plasma_shield", p -> find_spell( 82626 ) );
    buff->set_cooldown( timespan_t::zero() );
  }

  void execute() override
  {
    engineering_effect_t::execute();

    buff -> trigger();
  }
};

void profession::nitro_boosts( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_USE;
  effect.execute_action = new nitro_boosts_action_t( effect.item -> player );
}

void profession::grounded_plasma_shield( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_USE;
  effect.execute_action = new grounded_plasma_shield_t( effect.item -> player );
}

void profession::zen_alchemist_stone( special_effect_t& effect )
{
  struct zen_alchemist_stone_callback : public dbc_proc_callback_t
  {
    stat_buff_t* buff_str;
    stat_buff_t* buff_agi;
    stat_buff_t* buff_int;

    zen_alchemist_stone_callback( const item_t* i, const special_effect_t& data ) :
      dbc_proc_callback_t( i -> player, data )
    {
      const spell_data_t* spell = listener -> find_spell( 105574 );

      struct common_buff_t : public stat_buff_t
      {
        common_buff_t( player_t* p, const std::string& n, stat_e stat, const spell_data_t* spell, const item_t* item ) :
          stat_buff_t ( p, "zen_alchemist_stone_" + n, spell, item )
        {
          double value = spell -> effectN( 1 ).average( *item );
          add_stat( stat, value );
          set_duration( p -> find_spell( 60229 ) -> duration() );
          set_chance( 1.0 );
          set_activated( false );
        }
      };

      buff_str = make_buff<common_buff_t>( listener, "str", STAT_STRENGTH, spell, effect.item );
      buff_agi = make_buff<common_buff_t>( listener, "agi", STAT_AGILITY, spell, effect.item );
      buff_int = make_buff<common_buff_t>( listener, "int", STAT_INTELLECT, spell, effect.item );
    }

    void execute( action_t* a, action_state_t* /* state */ ) override
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

  new zen_alchemist_stone_callback( effect.item, effect );
}

void profession::draenor_philosophers_stone( special_effect_t& effect )
{
  struct draenor_philosophers_stone_callback : public dbc_proc_callback_t
  {
    stat_buff_t* buff_str;
    stat_buff_t* buff_agi;
    stat_buff_t* buff_int;

    draenor_philosophers_stone_callback( const item_t* i, const special_effect_t& data ) :
      dbc_proc_callback_t( i -> player, data )
    {
      const spell_data_t* spell = listener -> find_spell( 157136 );

      struct common_buff_t : public stat_buff_t
      {
        common_buff_t( player_t* p, const std::string& n, stat_e stat, const spell_data_t* spell, const item_t* item ) :
          stat_buff_t ( p, "draenor_philosophers_stone_" + n, spell, item )
        {
          double value = spell -> effectN( 1 ).average( *item );
          add_stat( stat, value );
          set_duration( p -> find_spell( 60229 ) -> duration() );
          set_chance( 1.0 );
          set_activated( false );
        }
      };


      buff_str = make_buff<common_buff_t>( listener, "str", STAT_STRENGTH, spell, data.item );
      buff_agi = make_buff<common_buff_t>( listener, "agi", STAT_AGILITY, spell, data.item );
      buff_int = make_buff<common_buff_t>( listener, "int", STAT_INTELLECT, spell, data.item );
    }

    void execute( action_t* a, action_state_t* /* state */ ) override
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

  maintenance_check( 620 );

  new draenor_philosophers_stone_callback( effect.item, effect );
}

void gem::sinister_primal( special_effect_t& effect )
{
  if ( effect.item -> sim -> challenge_mode )
    return;
  if ( effect.item -> player -> level() >= 100 )
    return;

  auto p = effect.player;
  if ( !p->buffs.tempus_repit )
  {
    p->buffs.tempus_repit = make_buff( p, "tempus_repit", p->find_spell( 137590 ) )
      ->set_default_value_from_effect( 1 )
      ->add_invalidate( CACHE_SPELL_CAST_SPEED )
      ->set_activated( false );
  }

  effect.custom_buff = p->buffs.tempus_repit;

  new dbc_proc_callback_t( effect.item, effect );
}

void gem::indomitable_primal( special_effect_t& effect )
{
  if ( effect.item -> sim -> challenge_mode )
    return;
  if ( effect.item -> player -> level() >= 100 )
    return;

  auto p = effect.player;
  if ( !p->buffs.fortitude )
  {
    p->buffs.fortitude = make_buff( p, "fortitude", p->find_spell( 137593 ) )
      ->set_default_value_from_effect( 1 )
      ->set_activated( false );
  }

  effect.custom_buff = p->buffs.fortitude;

  new dbc_proc_callback_t( effect.item, effect );
}

void gem::capacitive_primal( special_effect_t& effect )
{
  if ( effect.item -> sim -> challenge_mode )
    return;
  if ( effect.item -> player -> level() >= 100 )
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
    capacitive_primal_proc_t( const item_t* i, const special_effect_t& data ) :
      dbc_proc_callback_t( i, data )
    { }

    void initialize() override
    {
      dbc_proc_callback_t::initialize();
      // Unfortunately the weapon-based RPPM modifiers have to be hardcoded,
      // as they will not show on the client tooltip data.
      if ( listener -> main_hand_weapon.group() != WEAPON_2H )
      {
        if ( listener -> specialization() == WARRIOR_FURY )
          rppm -> set_modifier( 1.152 );
        else if ( listener -> specialization() == DEATH_KNIGHT_FROST )
          rppm -> set_modifier( 1.134 );
      }
    }

    void trigger( action_t* action, action_state_t* s ) override
    {
      // Flurry of Xuen and Capacitance cannot proc Capacitance
      if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
        return;

      dbc_proc_callback_t::trigger( action, s );
    }
  };

  player_t* p = effect.item -> player;

  // Stacking Buff
  effect.custom_buff = make_buff( p, "capacitance", p -> find_spell( 137596 ) );

  // Execute Action
  action_t* ls = p -> create_proc_action( "lightning_strike", effect );
  if ( ! ls )
    ls = new lightning_strike_t( p );
  effect.execute_action = ls;

  new capacitive_primal_proc_t( effect.item, effect );
}

void gem::courageous_primal( special_effect_t& effect )
{
  if ( effect.item -> sim -> challenge_mode )
    return;

  if ( effect.item -> player -> level() >= 100 )
    return;

  auto p = effect.player;
  if ( !p->buffs.courageous_primal_diamond_lucidity )
    p->buffs.courageous_primal_diamond_lucidity = make_buff( p, "lucidity", p->find_spell( 137288 ) );

  effect.custom_buff = p->buffs.courageous_primal_diamond_lucidity;

  new dbc_proc_callback_t( effect.player, effect );
}

struct lfr_harmful_spell_t : public spell_t
{
  lfr_harmful_spell_t( const std::string& name_str, special_effect_t& effect, const spell_data_t* data ) :
    spell_t( name_str, effect.player, data )
  {
    background = may_crit = tick_may_crit = true;
    callbacks = false;
  }
};

// TODO: Ratings
[[maybe_unused]] void set_bonus::passive_stat_aura( special_effect_t& effect )
{
  const spell_data_t* spell = effect.player -> find_spell( effect.spell_id );
  stat_e stat = STAT_NONE;
  // Sanity check for stat-giving aura, either stats or aura type 465 ("bonus armor")
  if ( spell -> effectN( 1 ).subtype() != A_MOD_STAT || spell -> effectN( 1 ).subtype() == A_465 )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  if ( spell -> effectN( 1 ).subtype() == A_MOD_STAT )
  {
    if ( spell -> effectN( 1 ).misc_value1() >= 0 )
    {
      stat = static_cast< stat_e >( spell -> effectN( 1 ).misc_value1() + 1 );
    }
    else if ( spell -> effectN( 1 ).misc_value1() == -1 )
    {
      stat = STAT_ALL;
    }
  }
  else
  {
    stat = STAT_BONUS_ARMOR;
  }

  double amount = util::round( spell -> effectN( 1 ).average( effect.player, std::min( MAX_LEVEL, effect.player -> level() ) ) );

  effect.player -> passive.add_stat( stat, amount );
}

// Items ====================================================================

// Blazefury Medallion
// 243988 Driver
// 243991 Damage spell
void item::blazefury_medallion( special_effect_t& effect )
{
  struct blazefury_medallion_t : public generic_proc_t
  {
    blazefury_medallion_t( const special_effect_t& effect )
      : generic_proc_t( effect, "blazefury_medallion", effect.driver()->effectN( 1 ).trigger() )
    {
    }
  };

  struct blazefury_medallion_cb_t : public dbc_proc_callback_t
  {
    action_t* damage;
    double base_damage;

    blazefury_medallion_cb_t( const special_effect_t& e, action_t* a ) :
      dbc_proc_callback_t( e.player, e ),
      damage( a ),
      base_damage( e.driver()->effectN( 1 ).trigger()->effectN( 1 ).average( e.item ) )
    {
    }

    void execute( action_t* a, action_state_t* s ) override
    {
      if ( a->result_is_hit( s->result ) )
      {
        // Currently only uses MH weapon speed, not the speed of the triggering weapon
        // Leaving this in a CB handler just in case this changes at some point via hotfix
        // TOCHECK -- Unclear if this uses equipped weapon speed for Feral or not
        double speed_mod = ( a->player->main_hand_weapon.type == WEAPON_NONE ? 2.0 :
                             a->player->main_hand_weapon.swing_time.total_seconds() );
        damage->execute_on_target( s->target, base_damage * speed_mod );
      }
    }
  };

  // if your autoattacks happen to be aoe, this will apply to all targets hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  auto damage = create_proc_action<blazefury_medallion_t>( "blazefury_medallion", effect );
  new blazefury_medallion_cb_t( effect, damage );
}

void item::rune_of_reorigination( special_effect_t& effect )
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

    rune_of_reorigination_callback_t( const special_effect_t& data ) :
      dbc_proc_callback_t( data.item -> player, data )
    {
      buff = static_cast< stat_buff_t* >( effect.custom_buff );
    }

    void execute( action_t* action, action_state_t* /* state */ ) override
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

  const spell_data_t* spell = effect.item -> player -> find_spell( 139120 );

  auto buff_name = util::tokenize_fn( spell -> name_cstr() );

  stat_buff_t* buff = make_buff<stat_buff_t>( effect.item -> player, buff_name, spell, effect.item );
  buff->add_stat( STAT_CRIT_RATING, 0 )
      ->add_stat( STAT_HASTE_RATING, 0 )
      ->add_stat( STAT_MASTERY_RATING, 0 )
      ->set_activated( false );

  effect.custom_buff  = buff;

  new rune_of_reorigination_callback_t( effect );
}

void item::spark_of_zandalar( special_effect_t& effect )
{
  maintenance_check( 502 );

  const spell_data_t* buff = effect.item -> player -> find_spell( 138960 );

  auto buff_name = util::tokenize_fn( buff -> name_cstr() );

  stat_buff_t* b = make_buff<stat_buff_t>( effect.item -> player, buff_name, buff, effect.item );

  effect.custom_buff = b;

  struct spark_of_zandalar_callback_t : public dbc_proc_callback_t
  {
    buff_t*      sparks;
    stat_buff_t* buff;

    spark_of_zandalar_callback_t( const special_effect_t& data ) :
      dbc_proc_callback_t( *data.item, data ), buff(nullptr)
    {
      const spell_data_t* spell = listener -> find_spell( 138958 );
      sparks = make_buff( listener, "zandalari_spark_driver", spell, data.item )
               ->set_quiet( true );
    }

    void execute( action_t* /* action */, action_state_t* /* state */ ) override
    {
      sparks -> trigger();

      if ( sparks -> check() == sparks -> max_stack() )
      {
        sparks -> expire();
        proc_buff -> trigger();
      }
    }
  };

  new spark_of_zandalar_callback_t( effect );
}

void item::unerring_vision_of_leishen( special_effect_t& effect )
{
  struct perfect_aim_buff_t : public buff_t
  {
    perfect_aim_buff_t( player_t* p ) :
      buff_t( p, "perfect_aim", p -> find_spell( 138963 ) )
    {
      set_activated( false );
    }

    void execute( int stacks, double value, timespan_t duration ) override
    {
      if ( current_stack == 0 )
      {
        player -> current.spell_crit_chance  += data().effectN( 1 ).percent();
        player -> current.attack_crit_chance += data().effectN( 1 ).percent();
        player -> invalidate_cache( CACHE_CRIT_CHANCE );
      }

      buff_t::execute( stacks, value, duration );
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );

      player -> current.spell_crit_chance  -= data().effectN( 1 ).percent();
      player -> current.attack_crit_chance -= data().effectN( 1 ).percent();
      player -> invalidate_cache( CACHE_CRIT_CHANCE );
    }
  };

  struct unerring_vision_of_leishen_callback_t : public dbc_proc_callback_t
  {
    unerring_vision_of_leishen_callback_t( const special_effect_t& data ) :
      dbc_proc_callback_t( data.item -> player, data )
    { }

    void initialize() override
    {
      dbc_proc_callback_t::initialize();

      // Warlocks have a (hidden?) 0.6 modifier, not showing in DBCs.
      if ( listener -> type == WARLOCK )
        rppm -> set_modifier( rppm -> get_modifier() * 0.6 );
    }
  };

  maintenance_check( 502 );

  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.custom_buff  = new perfect_aim_buff_t( effect.item -> player );

  new unerring_vision_of_leishen_callback_t( effect );
}

void item::skeers_bloodsoaked_talisman( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();
  // Aura is hidden, thre's no linkage in spell data actual
  const spell_data_t* buff = effect.item -> player -> find_spell( 146293 );

  auto buff_name = util::tokenize_fn( buff -> name_cstr() );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = make_buff<stat_buff_t>( effect.item -> player, buff_name, buff, effect.item );
  b->add_stat( STAT_CRIT_RATING, spell -> effectN( 1 ).average( effect.item ) )
      ->set_tick_behavior( buff_tick_behavior::CLIP )
      ->set_period( spell -> effectN( 1 ).period() )
      ->set_duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::blackiron_micro_crucible( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  auto buff_name = util::tokenize_fn( spell -> name_cstr() );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = make_buff<stat_buff_t>( effect.item -> player, buff_name, spell, effect.item );
  b->add_stat( STAT_MASTERY_RATING, spell -> effectN( 1 ).average( effect.item ) )
      ->set_max_stack( 20 ) // Hardcoded for now - spell->max_stacks() returns 0
      ->set_tick_behavior( buff_tick_behavior::CLIP )
      ->set_period( spell -> effectN( 1 ).period() )
      ->set_duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::humming_blackiron_trigger( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  auto buff_name = util::tokenize_fn( spell -> name_cstr() );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = make_buff<stat_buff_t>( effect.item -> player, buff_name, spell, effect.item );
  b->add_stat( STAT_CRIT_RATING, spell -> effectN( 1 ).average( effect.item ) )
      ->set_max_stack( 20 ) // Hardcoded for now - spell->max_stacks() returns 0
      ->set_tick_behavior( buff_tick_behavior::CLIP )
      ->set_period( spell -> effectN( 1 ).period() )
      ->set_duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::darkmoon_card_greatness( special_effect_t& effect )
{
  struct darkmoon_card_greatness_callback : public dbc_proc_callback_t
  {
    stat_buff_t* buff_str;
    stat_buff_t* buff_agi;
    stat_buff_t* buff_int;
    stat_buff_t* buff_spi;

    darkmoon_card_greatness_callback( const item_t* i, const special_effect_t& data ) :
      dbc_proc_callback_t( i -> player, data )
    {
      struct common_buff_t : public stat_buff_t
      {
        common_buff_t( player_t* p, const item_t* i, const std::string& n, stat_e stat, int id ) :
          stat_buff_t ( p, "deathbringers_will_" + n, p -> find_spell( id ) )
        {
          add_stat( stat, p -> find_spell( id ) -> effectN( 1 ).average( *i ) );
        }
      };

      buff_str = make_buff<common_buff_t>( listener, i, "str", STAT_STRENGTH, 60235 );
      buff_agi = make_buff<common_buff_t>( listener, i, "agi", STAT_AGILITY, 60235 );
      buff_int = make_buff<common_buff_t>( listener, i, "int", STAT_INTELLECT, 60235 );
      buff_spi = make_buff<common_buff_t>( listener, i, "spi", STAT_SPIRIT, 60235 );
    }

    void execute( action_t* a, action_state_t* /* state */ ) override
    {
      player_t* p = a -> player;

      double str  = p -> strength();
      double agi  = p -> agility();
      double inte = p -> intellect();
      double spi  = p -> spirit();

      if ( str > agi )
      {
        if ( str > inte )
        {
          if ( str > spi )
            buff_str -> trigger();
          else
            buff_spi -> trigger();
        }
        else
        {
          if ( inte > spi )
            buff_int -> trigger();
          else
            buff_spi -> trigger();
        }
      }
      else
      {
        if ( agi > inte )
        {
          if ( agi > spi )
            buff_agi -> trigger();
          else
            buff_spi -> trigger();
        }
        else
        {
          if ( inte > spi )
            buff_int -> trigger();
          else
            buff_spi -> trigger();
        }
      }
    }
  };

  effect.proc_flags2_ = PF2_ALL_HIT;

  new darkmoon_card_greatness_callback( effect.item, effect );
}

void item::vial_of_shadows( special_effect_t& effect )
{
  // TODO: Verify this scales by item level correctly.
  struct lightning_strike_t : public attack_t
  {
    lightning_strike_t( const special_effect_t& effect ) :
      attack_t( "lightning_strike_vial", effect.player, effect.player -> find_spell( 109724 ) )
    {
      background = may_crit = true;
      callbacks = false;

      base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item );
      switch ( effect.driver() -> id() )
      {
        case 109725: attack_power_mod.direct = 0.339; break;
        case 107995: attack_power_mod.direct = 0.300; break;
        case 109722: attack_power_mod.direct = 0.266; break;
        default: assert( false ); break;
      }
    }
  };

  // Call the proc _vial so it doesn't conflict with the legendary meta gem.
  action_t* action = effect.player -> find_action( "lightning_strike_vial" );
  if ( ! action )
  {
    action = effect.player -> create_proc_action( "lightning_strike_vial", effect );
  }

  if ( ! action )
  {
    action = new lightning_strike_t( effect );
  }

  effect.execute_action = action;

  new dbc_proc_callback_t( effect.player, effect );
}

void item::cunning_of_the_cruel( special_effect_t& effect )
{
  // TODO: Verify this matches in-game.
  struct shadowbolt_volley_t : public spell_t
  {
    shadowbolt_volley_t( const special_effect_t& effect ) :
      spell_t( "shadowbolt_volley", effect.player, effect.player -> find_spell( effect.driver() -> effectN( 1 ).trigger_spell_id() ) )
    {
      background = may_crit = true;
      callbacks = false;
      aoe = -1;
      radius = effect.player -> find_spell( effect.driver() -> effectN( 1 ).trigger_spell_id() ) -> effectN( 1 ).radius();
    }
  };

  action_t* action = effect.player -> find_action( "shadowbolt_volley" );
  if ( ! action )
  {
    action = effect.player -> create_proc_action( "shadowbolt_volley", effect );
  }

  if ( ! action )
  {
    action = new shadowbolt_volley_t( effect );
  }

  effect.execute_action = action;

  new dbc_proc_callback_t( effect.player, effect );
}

void item::deathbringers_will( special_effect_t& effect )
{
  struct deathbringers_will_callback : public dbc_proc_callback_t
  {
    stat_buff_t* str;
    stat_buff_t* agi;
    stat_buff_t* ap;
    stat_buff_t* crit;
    stat_buff_t* haste;

    std::array<stat_buff_t*, 3> procs;

    deathbringers_will_callback( const item_t* i, const special_effect_t& data ) :
      dbc_proc_callback_t( i -> player, data )
    {
      struct common_buff_t : public stat_buff_t
      {
        common_buff_t( player_t* p, const item_t* i, const std::string& n, stat_e stat, int id ) :
          stat_buff_t ( p, "deathbringers_will_" + n, p -> find_spell( id ) )
        {
          add_stat( stat, p -> find_spell( id ) -> effectN( 2 ).average( *i ) );
        }
      };

      str   = make_buff<common_buff_t>( listener, i, "str",   STAT_STRENGTH,     data.spell_id == 71562 ? 71561 : 71484 );
      agi   = make_buff<common_buff_t>( listener, i, "agi",   STAT_AGILITY,      data.spell_id == 71562 ? 71556 : 71485 );
      ap    = make_buff<common_buff_t>( listener, i, "ap",    STAT_ATTACK_POWER, data.spell_id == 71562 ? 71558 : 71486 );
      crit  = make_buff<common_buff_t>( listener, i, "crit",  STAT_CRIT_RATING,  data.spell_id == 71562 ? 71559 : 71491 );
      haste = make_buff<common_buff_t>( listener, i, "haste", STAT_HASTE_RATING, data.spell_id == 71562 ? 71560 : 71492 );

      switch( i -> player -> type )
      {
        case DRUID:    procs[ 0 ] = agi; procs[ 1 ] = haste; procs[ 2 ] =  str; break;
        case HUNTER:   procs[ 0 ] = agi; procs[ 1 ] = haste; procs[ 2 ] =   ap; break;
        case ROGUE:    procs[ 0 ] = agi; procs[ 1 ] = haste; procs[ 2 ] =   ap; break;
        default:       procs[ 0 ] = str; procs[ 1 ] = haste; procs[ 2 ] = crit; break;
      }
    }

    void execute( action_t* a, action_state_t* /* state */ ) override
    {
      player_t* p = a -> player;

      procs[ static_cast<int>( p -> rng().real() * 3 ) ] -> trigger();
    }
  };

  new deathbringers_will_callback( effect.item, effect );
}

void item::battering_talisman_trigger( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();
  const spell_data_t* stacks = effect.item -> player -> find_spell( 146293 );

  auto buff_name = util::tokenize_fn( spell -> name_cstr() );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = make_buff<stat_buff_t>( effect.item -> player, buff_name, spell, effect.item );
  b->add_stat( STAT_HASTE_RATING, spell -> effectN( 1 ).average( effect.item ) )
      ->set_max_stack( stacks -> max_stacks() )
      ->set_tick_behavior( buff_tick_behavior::CLIP )
      ->set_period( spell -> effectN( 1 ).period() )
      ->set_duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::forgemasters_insignia( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  auto buff_name = util::tokenize_fn( spell -> name_cstr() );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = make_buff<stat_buff_t>( effect.item -> player, buff_name, spell, effect.item );
  b->add_stat( STAT_MASTERY_RATING, spell -> effectN( 1 ).average( effect.item ) )
      ->set_max_stack( 20 ) // Hardcoded for now - spell->max_stacks() returns 0
      ->set_tick_behavior( buff_tick_behavior::CLIP )
      ->set_period( spell -> effectN( 1 ).period() )
      ->set_duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::autorepairing_autoclave( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* spell = driver -> effectN( 1 ).trigger();

  auto buff_name = util::tokenize_fn( spell -> name_cstr() );

  // Require a damaging result, instead of any harmful spell hit
  effect.proc_flags2_ = PF2_ALL_HIT;

  stat_buff_t* b = make_buff<stat_buff_t>( effect.item -> player, buff_name, spell, effect.item );
  b->add_stat( STAT_HASTE_RATING, spell -> effectN( 1 ).average( effect.item ) )
      ->set_max_stack( 20 ) // Hardcoded for now - spell->max_stacks() returns 0
      ->set_tick_behavior( buff_tick_behavior::CLIP )
      ->set_period( spell -> effectN( 1 ).period() )
      ->set_duration( spell -> duration() );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

void item::spellbound_runic_band( special_effect_t& effect )
{
  maintenance_check( 528 );

  player_t* p = effect.item -> player;
  const spell_data_t* driver = p -> find_spell( effect.spell_id );
  buff_t* buff = nullptr;

  // Need to test which procs on off-spec rings, assume correct proc for now.
  switch( p -> convert_hybrid_stat( STAT_STR_AGI_INT ) )
  {
    case STAT_STRENGTH:
      buff = create_buff<buff_t>( p, p->find_spell( 177175 ) )
        ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
        ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );
      break;
    case STAT_AGILITY:
      buff = create_buff<buff_t>( p, p->find_spell( 177172 ) )
        ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
        ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY );
      break;
    case STAT_INTELLECT:
      buff = create_buff<buff_t>( p, p->find_spell( 177176 ) )
        ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
        ->set_pct_buff_type( STAT_PCT_BUFF_INTELLECT );
      break;
    default:
      break;
  }

  effect.ppm_ = -1.0 * driver -> real_ppm();
  effect.custom_buff = buff;
  effect.type = SPECIAL_EFFECT_EQUIP;

  new dbc_proc_callback_t( p, effect );
}

void item::spellbound_solium_band( special_effect_t& effect )
{
  maintenance_check( 528 );

  player_t* p = effect.item -> player;
  const spell_data_t* driver = p -> find_spell( effect.spell_id );
  buff_t* buff = nullptr;

  //Need to test which procs on off-spec rings, assume correct proc for now.
  switch( p -> convert_hybrid_stat( STAT_STR_AGI_INT ) )
  {
    case STAT_STRENGTH:
      buff = create_buff<buff_t>( p, p->find_spell( 177160 ) )
        ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
        ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );
      break;
    case STAT_AGILITY:
      buff = create_buff<buff_t>( p, p->find_spell( 177161 ) )
        ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
        ->set_pct_buff_type( STAT_PCT_BUFF_AGILITY );
      break;
    case STAT_INTELLECT:
      buff = create_buff<buff_t>( p, p->find_spell( 177159 ) )
        ->set_default_value_from_effect_type( A_MOD_TOTAL_STAT_PERCENTAGE )
        ->set_pct_buff_type( STAT_PCT_BUFF_STRENGTH );
      break;
    default:
      break;
  }

  effect.ppm_ = -1.0 * driver -> real_ppm();
  effect.custom_buff = buff;
  effect.type = SPECIAL_EFFECT_EQUIP;

  new dbc_proc_callback_t( p, effect );
}


void item::legendary_ring( special_effect_t& effect )
{
  maintenance_check( 528 );

  player_t* p = effect.item -> player;
  buff_t* buff = nullptr;

  struct legendary_ring_damage_t: public spell_t
  {
    double damage_coeff;
    legendary_ring_damage_t( special_effect_t& originaleffect, const spell_data_t* spell ):
      spell_t( spell -> name_cstr(), originaleffect.player, spell ),
      damage_coeff( 0 )
    {
      damage_coeff = originaleffect.player -> find_spell( originaleffect.spell_id ) -> effectN( 1 ).average( originaleffect.item ) / 10000.0;
      background = split_aoe_damage = true;
      may_crit = false;
      callbacks = false;
      trigger_gcd = timespan_t::zero();
      aoe = -1;
      radius = 20;
      range = -1;
      travel_speed = 0.0;
      item = originaleffect.item;
      if ( originaleffect.player -> level() == 110 )
        damage_coeff = 0.0;
    }

    void init() override
    {
      spell_t::init();

      snapshot_flags = STATE_MUL_DA;
      update_flags = 0;
    }

    double composite_da_multiplier( const action_state_t* ) const override
    {
      return damage_coeff;
    }
  };

    struct legendary_ring_buff_t: public buff_t
    {
      struct legendary_ring_delay_event_t : public event_t
      {
        player_t* player;
        action_t* boom;
        double value;

        legendary_ring_delay_event_t( player_t* p, action_t* b, double v ) :
          event_t( *p, timespan_t::from_seconds( 1.0 ) ), player( p ), boom( b ), value( v )
        { }

        const char* name() const override
        { return "legendary_ring_boom_delay"; }

        void execute() override
        {
          if ( ! player -> is_sleeping() )
          {
            boom -> base_dd_min = boom -> base_dd_max = value;
            boom -> execute();
          }
        }
      };

      action_t* boom;
      player_t* p;

      legendary_ring_buff_t( special_effect_t& originaleffect, const std::string& name, const spell_data_t* buff, const spell_data_t* damagespell ):
        buff_t( originaleffect.player, name, buff, originaleffect.item ),
        boom( nullptr ), p( originaleffect.player )
      {
        add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
        set_default_value( originaleffect.player -> find_spell( originaleffect.spell_id ) -> effectN( 1 ).average( originaleffect.item ) / 10000.0 );
        boom = p -> find_action( damagespell -> name_cstr() );

        if ( !boom )
        {
          boom = p -> create_proc_action( damagespell -> name_cstr(), originaleffect );
        }

        if ( !boom )
        {
          boom = new legendary_ring_damage_t( originaleffect, damagespell );
        }
        p -> buffs.legendary_aoe_ring = this;
        if ( p -> level() == 110 ) // No damage boost at level 110.
          default_value = 0;
      }

      void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
      {
        double cv = current_value;

        buff_t::expire_override( expiration_stacks, remaining_duration );

        if ( cv > 0 )
          make_event<legendary_ring_delay_event_t>( *sim, p, boom, cv );
      }
  };


  if ( effect.spell_id != 187613 )
  {
    const spell_data_t* buffspell = nullptr;
    const spell_data_t* actionspell = nullptr;
    switch ( p -> convert_hybrid_stat( STAT_STR_AGI_INT ) )
    {
    case STAT_STRENGTH:
      buffspell = p -> find_spell( 187619 );
      actionspell = p -> find_spell( 187624 );
      break;
    case STAT_AGILITY:
      buffspell = p -> find_spell( 187620 );
      actionspell = p -> find_spell( 187626 );
      break;
    case STAT_INTELLECT:
      buffspell = p -> find_spell( 187616 );
      actionspell = p -> find_spell( 187625 );
      break;
    default:
      break;
    }

    if ( buffspell && actionspell )
    {
      auto name = util::tokenize_fn( buffspell -> name_cstr() );
      buff = new legendary_ring_buff_t( effect, name, buffspell, actionspell );
    }

    // Make legendary ring do it's accounting after target has been damaged
    p -> assessor_out_damage.add( assessor::TARGET_DAMAGE + 1, [ buff ]( result_amount_type, action_state_t* state )
    {
      if ( ! buff -> up() )
      {
        return assessor::CONTINUE;
      }

      buff -> current_value += state -> result_amount;
      if ( buff -> sim -> debug )
      {
        buff -> sim -> out_debug.printf( "%s %s stores %.2f damage from %s on %s, new stored amount = %.2f",
                         buff -> player -> name(),
                         buff -> name(),
                         state -> result_amount, state -> action -> name(), state -> target -> name(),
                         buff -> current_value );
      }
      return assessor::CONTINUE;
    } );

    // Generate functions for pets
    for ( auto& pet : p -> pet_list )
    {
      if ( ! pet -> cast_pet() -> affects_wod_legendary_ring )
      {
        continue;
      }

      pet -> assessor_out_damage.add( assessor::TARGET_DAMAGE + 1, [ buff ]( result_amount_type, action_state_t* state )
      {
        if ( ! buff -> check() )
        {
          return assessor::CONTINUE;
        }

        buff -> current_value += state -> result_amount;
        if ( buff -> sim -> debug )
        {
          buff -> sim -> out_debug.printf( "%s %s stores %.2f damage from %s %s on %s, new stored amount = %.2f",
                           state -> action -> player -> name(),
                           buff -> name(),
                           state -> result_amount, buff -> player -> name(), state -> action -> name(),
                           state -> target -> name(),
                           buff -> current_value );
        }
        return assessor::CONTINUE;
      } );
    }
  }
  else // Tanks
  {
    buff = buff_t::find( p, "sanctus" );
    if ( ! buff )
    {
      const spell_data_t* driver_spell = p->find_spell( effect.spell_id );
      const spell_data_t* spell = p->find_spell( 187617 );
      buff = make_buff( p, "sanctus", spell )
        ->set_default_value( driver_spell->effectN( 1 ).average( effect.item ) / 10000.0 )
        ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY );
    }
  }

  effect.custom_buff = buff;
  effect.type = SPECIAL_EFFECT_USE;
  effect.cooldown_ = timespan_t::from_seconds( 120 );
}

void item::gronntooth_war_horn( special_effect_t& effect )
{
  stat_buff_t* buff = make_buff<stat_buff_t>( effect.player, "demonbane", effect.driver() -> effectN( 1 ).trigger(), effect.item );
  effect.custom_buff = buff;
  effect.player -> buffs.demon_damage_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

void item::infallible_tracking_charm( special_effect_t& effect )
{
  effect.custom_buff = make_buff( effect.player, "cleansing_flame", effect.driver() -> effectN( 1 ).trigger(), effect.item );
  effect.execute_action = new spell_t( "cleansing_flame", effect.player, effect.driver() -> effectN( 1 ).trigger() );

  effect.execute_action -> background = true;
  effect.execute_action -> item = effect.item;
  effect.execute_action -> base_dd_min = effect.execute_action -> base_dd_max = effect.execute_action -> data().effectN( 1 ).average( effect.item );

  effect.rppm_scale_ = RPPM_HASTE;

  effect.player -> buffs.demon_damage_buff = effect.custom_buff;

  new dbc_proc_callback_t( effect.item, effect );
}

void item::orb_of_voidsight( special_effect_t& effect )
{
  stat_buff_t* buff = make_buff<stat_buff_t>( effect.player, "voidsight", effect.driver() -> effectN( 1 ).trigger(), effect.item );
  effect.custom_buff = buff;
  effect.player -> buffs.demon_damage_buff = buff;

  new dbc_proc_callback_t( effect.item, effect );
}

void item::witherbarks_branch( special_effect_t& effect )
{
  struct aqueous_dowsing_t : public buff_t
  {
    int custom_tick;
    stat_buff_t* stat_buff;

    aqueous_dowsing_t( const actor_pair_t& p, const special_effect_t& e )
      : buff_t( p, "aqueous_dowsing", e.driver() ), custom_tick( 0 )
    {
      set_quiet( true );
      set_cooldown( 0_ms );
      set_can_cancel( false );

      stat_buff = make_buff<stat_buff_t>( e.player, "aqueous_enrichment", e.player->find_spell( 429262 ), e.item );

      set_stack_change_callback( [ this ]( buff_t*, int, int n ) {
        if ( n )
        {
          custom_tick = 0;
          make_event( player->sim, player->dragonflight_opts.witherbarks_branch_timing[ custom_tick ], [ this ]() {
            custom_tick_callback();
          } );
        }
      } );
    }

    void custom_tick_callback()
    {
      if ( !check() )
        return;

      stat_buff->trigger();

      if ( custom_tick < 2 )
        make_event( player->sim, player->dragonflight_opts.witherbarks_branch_timing[ ++custom_tick ], [ this ]() {
          custom_tick_callback();
        } );
    }
  };

  effect.custom_buff = new aqueous_dowsing_t( effect.player, effect );
  effect.stat = STAT_MASTERY_RATING;
}

void item::black_blood_of_yshaarj( special_effect_t& effect )
{
  maintenance_check( 528 );

  const spell_data_t* driver = effect.item -> player -> find_spell( effect.spell_id );
  const spell_data_t* ticker = driver -> effectN( 1 ).trigger();
  const spell_data_t* buff = effect.item -> player -> find_spell( 146202 );

  auto buff_name = util::tokenize_fn( buff -> name_cstr() );

  stat_buff_t* b = make_buff<stat_buff_t>( effect.item -> player, buff_name, buff, effect.item );
  b->add_stat( STAT_INTELLECT, ticker -> effectN( 1 ).average( effect.item ) )
      ->set_tick_behavior( buff_tick_behavior::CLIP )
      ->set_period( ticker -> effectN( 1 ).period() )
      ->set_duration( ticker -> duration () );

  effect.custom_buff = b;

  new dbc_proc_callback_t( effect.item -> player, effect );
}

struct flurry_of_xuen_melee_t : public melee_attack_t
{
  flurry_of_xuen_melee_t( player_t* player ) :
    melee_attack_t( "flurry_of_xuen", player, player -> find_spell( 147891 ) )
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

  flurry_of_xuen_driver_t( player_t* player, action_t* action = nullptr ) :
    attack_t( "flurry_of_xuen_driver", player, player -> find_spell( 146194 ) ),
    ac( nullptr )
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
  void tick( dot_t* ) override
  {
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

  void trigger( action_t* action, action_state_t* s ) override
  {
    // Flurry of Xuen, and Lightning Strike cannot proc Flurry of Xuen
    if ( action -> id == 147891 || action -> id == 146194 || action -> id == 137597 )
      return;

    dbc_proc_callback_t::trigger( action, s );
  }
};

void item::flurry_of_xuen( special_effect_t& effect )
{
  maintenance_check( 600 );

  if ( effect.item -> sim -> challenge_mode )
    return;

  if ( effect.item -> player -> level() >= 100 )
    return;

  player_t* p = effect.item -> player;
  effect.execute_action = new flurry_of_xuen_driver_t( p, p -> create_proc_action( effect.name(), effect ) );

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

  void trigger( action_t* action, action_state_t* s ) override
  {
    if ( action -> id == 148008 ) // dot direct damage ticks can't proc itself
      return;

    dbc_proc_callback_t::trigger( action, s );
  }
};

void item::essence_of_yulon( special_effect_t& effect )
{
  maintenance_check( 600 );

  if ( effect.item -> sim -> challenge_mode )
    return;

  if ( effect.item -> player -> level() >= 100 )
    return;

  player_t* p = effect.item -> player;

  effect.execute_action = new essence_of_yulon_driver_t( p );

  new essence_of_yulon_cb_t( p, effect );
}

void item::readiness( special_effect_t& effect )
{
  maintenance_check( 528 );

  struct cooldowns_t
  {
    specialization_e spec;
    std::array<const char*, 8> cooldowns;
  };

  static const cooldowns_t __cd[] =
  {
    // NOTE: Spells that trigger buffs must have the cooldown of their buffs removed if they have one, or this trinket may cause undesirable results.
    { ROGUE_ASSASSINATION, { "evasion", "vanish", "cloak_of_shadows", "vendetta", nullptr, nullptr } },
    { ROGUE_OUTLAW,        { "evasion", "adrenaline_rush", "cloak_of_shadows", "killing_spree", nullptr, nullptr } },
    { ROGUE_SUBTLETY,      { "evasion", "vanish", "cloak_of_shadows", "shadow_dance", nullptr, nullptr } },
    { SHAMAN_ENHANCEMENT,  { "earth_elemental_totem", "fire_elemental_totem", "shamanistic_rage", "ascendance", "feral_spirit", nullptr } },
    { DRUID_FERAL,         { "tigers_fury", "berserk", "barkskin", "survival_instincts", nullptr, nullptr, nullptr } },
    { DRUID_GUARDIAN,      { "might_of_ursoc", "berserk", "barkskin", "survival_instincts", nullptr, nullptr, nullptr } },
    { WARRIOR_FURY,        { "dragon_roar", "bladestorm", "shockwave", "avatar", "bloodbath", "recklessness", "storm_bolt", "heroic_leap" } },
    { WARRIOR_ARMS,        { "dragon_roar", "bladestorm", "shockwave", "avatar", "bloodbath", "recklessness", "storm_bolt", "heroic_leap" } },
    { WARRIOR_PROTECTION,  { "shield_wall", "demoralizing_shout", "last_stand", "recklessness", "heroic_leap", nullptr, nullptr } },
    { DEATH_KNIGHT_BLOOD,  { "antimagic_shell", "dancing_rune_weapon", "icebound_fortitude", "outbreak", "vampiric_blood", "bone_shield", nullptr } },
    { DEATH_KNIGHT_FROST,  { "antimagic_shell", "army_of_the_dead", "icebound_fortitude", "empower_rune_weapon", "outbreak", "pillar_of_frost", nullptr  } },
    { DEATH_KNIGHT_UNHOLY, { "antimagic_shell", "army_of_the_dead", "icebound_fortitude", "outbreak", "summon_gargoyle", nullptr } },
    { MONK_BREWMASTER,	   { "fortifying_brew", "guard", "zen_meditation", nullptr, nullptr, nullptr, nullptr } },
    { MONK_WINDWALKER,     { "energizing_brew", "fists_of_fury", "fortifying_brew", "zen_meditation", nullptr, nullptr, nullptr } },
    { PALADIN_PROTECTION,  { "ardent_defender", "avenging_wrath", "divine_protection", "divine_shield", "guardian_of_ancient_kings", nullptr } },
    { PALADIN_RETRIBUTION, { "avenging_wrath", "divine_protection", "divine_shield", "guardian_of_ancient_kings", nullptr, nullptr } },
    { HUNTER_BEAST_MASTERY,{ "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", "bestial_wrath", nullptr } },
    { HUNTER_MARKSMANSHIP, { "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", nullptr, nullptr } },
    { HUNTER_SURVIVAL,     { "black_arrow", "camouflage", "feign_death", "disengage", "stampede", "rapid_fire", nullptr } },
    { SPEC_NONE,           { nullptr } }
  };

  player_t* p = effect.item -> player;

  const spell_data_t* cdr_spell = p -> find_spell( effect.spell_id );
  const random_prop_data_t& budget = p -> dbc->random_property( effect.item -> item_level() );
  double cdr = 1.0 / ( 1.0 + budget.p_epic[ 0 ] * cdr_spell -> effectN( 1 ).m_coefficient() / 100.0 );

  if ( p -> level() > 90 )
  { // We have no clue how the trinket actually scales down with level. This will linearly decrease CDR until it hits .90 at level 100.
    double level_nerf = ( static_cast<double>( p -> level() - 90 ) / 10.0 );
    level_nerf = ( 1 - cdr ) * level_nerf;
    cdr += level_nerf;
    cdr = std::min( 0.90, cdr ); // The amount of CDR doesn't go above 90%, even at level 100.
  }

  const cooldowns_t* cd = &( __cd[ 0 ] );
  do
  {
    if ( p -> specialization() != cd -> spec )
    {
      cd++;
      continue;
    }

    for ( size_t i = 0; i < 7; i++ )
    {
      if ( cd -> cooldowns[ i ] == nullptr )
        break;

      auto action = p -> find_action( cd -> cooldowns[ i ] );
      if ( action != nullptr )
      {
        action -> base_recharge_multiplier *= cdr;
      }
    }

    break;
  } while ( cd -> spec != SPEC_NONE );
}

void item::amplification( special_effect_t& effect )
{
  maintenance_check( 528 );

  player_t* p = effect.item -> player;
  double amp_value = 0.1;  // Seems to be 0.1 regardless of level/item level now.
  if ( !p->passive_values.amplification_1 )
  {
    p->passive_values.amplification_1 = amp_value;
    p->base.crit_damage_multiplier *= 1.0 + amp_value;
    p->base.crit_healing_multiplier *= 1.0 + amp_value;
  }
  else
  {
    p->passive_values.amplification_2 = amp_value;
    p->base.crit_damage_multiplier *= 1.0 + amp_value;
    p->base.crit_healing_multiplier *= 1.0 + amp_value;
  }
}

void item::prismatic_prison_of_pride( special_effect_t& effect )
{
  // Disable Prismatic Prison of Pride stat proc (Int) for all roles but HEAL
  if ( effect.item -> player -> role != ROLE_HEAL )
    return;

  effect.type = SPECIAL_EFFECT_EQUIP;
  new dbc_proc_callback_t( effect.item, effect );
}

void item::purified_bindings_of_immerseus( special_effect_t& effect )
{
  // Disable Purified Bindings stat proc (Int) on healing roles
  if ( effect.item -> player -> role == ROLE_HEAL )
    return;

  effect.type = SPECIAL_EFFECT_EQUIP;
  new dbc_proc_callback_t( effect.item, effect );
}

void item::thoks_tail_tip( special_effect_t& effect )
{
  // Disable Thok's Tail Tip stat proc (Str) on healing roles
  if ( effect.item -> player -> role == ROLE_HEAL )
    return;

  effect.type = SPECIAL_EFFECT_EQUIP;
  new dbc_proc_callback_t( effect.item, effect );
}

template <typename T>
struct cleave_t : public T
{
  cleave_t( const item_t* item, const std::string& name, school_e s ) :
    T( name, item -> player )
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
    if ( this -> type == ACTION_ATTACK )
    {
      this -> may_dodge = true;
      this -> may_parry = true;
      this -> may_block = true;
    }
  }

  void init() override
  {
    T::init();

    this -> snapshot_flags = 0;
  }

  size_t available_targets( std::vector< player_t* >& tl ) const override
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

  double composite_target_armor( player_t* ) const override
  { return 0.0; }
};

void item::cleave( special_effect_t& effect )
{
  maintenance_check( 528 );

  struct cleave_callback_t : public dbc_proc_callback_t
  {
    cleave_t<spell_t>* cleave_spell;
    cleave_t<attack_t>* cleave_attack;

    cleave_callback_t( const special_effect_t& data ) :
      dbc_proc_callback_t( *data.item, data )
    {
      cleave_spell = new cleave_t<spell_t>( data.item, "cleave_spell", SCHOOL_NATURE );
      cleave_attack = new cleave_t<attack_t>( data.item, "cleave_attack", SCHOOL_PHYSICAL );

    }

    void execute( action_t* action, action_state_t* state ) override
    {
      action_t* a = nullptr;

      if ( action -> type == ACTION_ATTACK )
        a = cleave_attack;
      else if ( action -> type == ACTION_SPELL )
        a = cleave_spell;
      // TODO: Heal

      if ( a )
      {
        a -> base_dd_min = a -> base_dd_max = state -> result_amount;
        // Invalidate target cache if target changes
        if ( a -> target != state -> target )
          a -> target_cache.is_valid = false;
        a -> target = state -> target;
        a -> schedule_execute();
      }
    }
  };

  player_t* p = effect.item -> player;
  const random_prop_data_t& budget = p -> dbc->random_property( effect.item -> item_level() );
  const spell_data_t* cleave_driver_spell = p -> find_spell( effect.spell_id );

  // Needs a damaging result
  effect.proc_flags2_ = PF2_ALL_HIT;
  effect.proc_chance_ = budget.p_epic[ 0 ] * cleave_driver_spell -> effectN( 1 ).m_coefficient() / 10000.0;

  if ( p -> level() > 90 )
  { // We have no clue how the trinket actually scales down with level. This will linearly decrease amplification until it hits 0 at level 100.
    double level_nerf = ( static_cast<double>( p -> level() ) - 90 ) / 10.0;
     effect.proc_chance_ *= 1 - level_nerf;
     effect.proc_chance_ = std::max( 0.01, effect.proc_chance_ ); // Cap it at 1%
  }

  new cleave_callback_t( effect );
}

void item::heartpierce( special_effect_t& effect )
{
  struct invigorate_proc_t : public spell_t
  {
    gain_t* g;

    invigorate_proc_t( player_t* player, const spell_data_t* d ) :
      spell_t( "invigoration", player, d ),
      g( player -> get_gain( "invigoration" ) )
    {
      may_miss = may_crit = harmful = may_dodge = may_parry = callbacks = false;
      tick_may_crit = hasted_ticks = false;
      dual = quiet = background = true;
      target = player;
    }

    void tick( dot_t* d ) override
    {
      spell_t::tick( d );

      player -> resource_gain( player -> primary_resource(),
                               data().effectN( 1 ).resource( player -> primary_resource() ),
                               g,
                               this );
    }
  };

  unsigned spell_id = 0;
  switch ( effect.item -> player -> primary_resource() )
  {
    case RESOURCE_MANA:
      spell_id = 71881;
      break;
    case RESOURCE_ENERGY:
      spell_id = 71882;
      break;
    case RESOURCE_RAGE:
      spell_id = 71883;
      break;
    default:
      break;
  }

  if ( spell_id == 0 )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  effect.ppm_ = 1.0;
  effect.execute_action = new invigorate_proc_t( effect.item -> player, effect.item -> player -> find_spell( spell_id ) );

  new dbc_proc_callback_t( effect.item -> player, effect );
}

struct felmouth_frenzy_driver_t : public spell_t
{
  struct felmouth_frenzy_damage_t : public spell_t
  {
    player_t* p;

    felmouth_frenzy_damage_t( const special_effect_t& effect ) :
      spell_t( "felmouth_frenzy_damage", effect.player, effect.player -> find_spell( 188505 ) ),
      p( effect.player )
    {
      background = true;
      callbacks = false;

      base_dd_max = base_dd_min = 0;
      spell_power_mod.direct = attack_power_mod.direct = 0.424;
    }

    double attack_direct_power_coefficient( const action_state_t* s ) const override
    {
      if ( s -> composite_spell_power() > s -> composite_attack_power() )
        return 0;

      return spell_t::attack_direct_power_coefficient( s );
    }

    double spell_direct_power_coefficient( const action_state_t* s ) const override
    {
      if ( s -> composite_spell_power() <= s -> composite_attack_power() )
        return 0;

      return spell_t::spell_direct_power_coefficient( s );
    }
  };

  size_t bolt_avg, bolt_magnitude;
  player_t* p;
  std::vector<unsigned> n_ticks;

  felmouth_frenzy_driver_t( const special_effect_t& effect ) :
    spell_t( "felmouth_frenzy", effect.player, effect.player -> find_spell( 188512 ) ),
    bolt_avg( as<int>( effect.player -> find_spell( 188534 ) -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() ) ),
    bolt_magnitude( as<int>(bolt_avg * effect.player -> find_spell( 188534 ) -> effectN( 1 ).trigger() -> effectN( 1 ).m_delta() / 2.0) ),
    p( effect.player )
  {
    background = true;
    may_crit = callbacks = hasted_ticks = dynamic_tick_action = false;
    // Estimated from logs
    base_tick_time = timespan_t::from_millis( 250 );
    dot_behavior = DOT_EXTEND;
    travel_speed = 0;

    tick_action = effect.player -> find_action( "felmouth_frenzy_damage" );
    if ( ! tick_action )
    {
      tick_action = effect.player -> create_proc_action( "felmouth_frenzy_damage", effect );
    }

    if ( ! tick_action )
    {
      tick_action = new felmouth_frenzy_damage_t( effect );
    }

    int min_ticks = as<int>( std::floor( bolt_avg - bolt_magnitude ) );
    int max_ticks = as<int>( std::ceil( bolt_avg + bolt_magnitude ) );
    int tick_steps = max_ticks - min_ticks + 1;
    for ( int i = 0; i < tick_steps; ++i )
    {
      n_ticks.push_back( min_ticks + i );
    }
  }

  void init_finished() override
  {
    spell_t::init_finished();

    // Can't be done on init() for abilities with tick_action() as the parent init() is called
    // before action_t::consolidate_snapshot_flags().
    snapshot_flags = STATE_AP | STATE_SP | STATE_TGT_MUL_TA;
    update_flags = STATE_TGT_MUL_TA;
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    size_t ticks = n_ticks[ rng().range( n_ticks.size() ) ];
    assert( ticks >= 4 && ticks <= 6 );
    return base_tick_time * ticks;
  }

  double composite_total_spell_power() const override
  {
    // Fel Lash uses the player's highest primary school spellpower.
    double csp = 0.0;

    for ( school_e i = SCHOOL_NONE; i < SCHOOL_MAX_PRIMARY; i++ )
      csp = std::max( csp, p->composite_total_spell_power( i ) );

    return csp;
  }
};

void item::felmouth_frenzy( special_effect_t& effect )
{
  action_t* a = effect.player -> find_action( "felmouth_frenzy_driver" );
  if ( ! a )
  {
    a = effect.player -> create_proc_action( "felmouth_frenzy_driver", effect );
  }

  if ( ! a )
  {
    a = new felmouth_frenzy_driver_t( effect );
  }

  effect.execute_action = a;

  dbc_proc_callback_t* cb = new dbc_proc_callback_t( effect.player, effect );
  cb -> initialize();
}

// Matrix Restabilizer
// 96976 Driver
// 96977 Haste buff
// 96978 Critical Strike buff
// 96979 Mastery buff
void item::matrix_restabilizer( special_effect_t& effect )
{
  auto buffs    = std::make_shared<std::map<stat_e, buff_t*>>();
  (*buffs)[STAT_HASTE_RATING] = create_buff<stat_buff_t>( effect.player, "matrix_restabilized_haste_rating",
                                                              effect.player->find_spell( 96977 ), effect.item );
  (*buffs)[STAT_CRIT_RATING] = create_buff<stat_buff_t>( effect.player, "matrix_restabilized_crit_rating",
                                                              effect.player->find_spell( 96978 ), effect.item );
  (*buffs)[STAT_MASTERY_RATING] = create_buff<stat_buff_t>( effect.player, "matrix_restabilized_mastery_rating",
                                                              effect.player->find_spell( 96979 ), effect.item );

  struct matrix_restabilizer_cb_t : public dbc_proc_callback_t
  {
    std::shared_ptr<std::map<stat_e, buff_t*>> buffs;

    matrix_restabilizer_cb_t( const special_effect_t& effect, std::shared_ptr<std::map<stat_e, buff_t*>> b )
      : dbc_proc_callback_t( effect.item, effect ), buffs( std::move( b ) )
    {
    }

    void execute(action_t*, action_state_t*) override
    {
      static constexpr std::array<stat_e, 3> ratings = { STAT_MASTERY_RATING, STAT_HASTE_RATING,
                                                         STAT_CRIT_RATING };

      stat_e max_stat = util::highest_stat( effect.player, ratings );

      (*buffs)[max_stat]->trigger();
    }
  };

  new matrix_restabilizer_cb_t( effect, buffs );
}

struct empty_drinking_horn_damage_t : public melee_attack_t
{
  empty_drinking_horn_damage_t( const special_effect_t& effect ) :
    melee_attack_t( "fel_burn", effect.player, effect.player -> find_spell( 184256 ) )
  {
    background = special = tick_may_crit = true;
    may_crit = callbacks = false;
    base_td = data().effectN( 1 ).average( effect.item );
    weapon_multiplier = 0;
    item = effect.item;
    dot_behavior = dot_behavior_e::DOT_NONE;
  }
};

struct empty_drinking_horn_cb_t : public dbc_proc_callback_t
{
  action_t* burn;
  empty_drinking_horn_cb_t( const special_effect_t& effect ):
    dbc_proc_callback_t( effect.player, effect )
  {
    burn = effect.player -> find_action( "fel_burn" );
    if ( !burn )
    {
      burn = effect.item -> player -> create_proc_action( "fel_burn", effect );
    }

    if ( !burn )
    {
      burn = new empty_drinking_horn_damage_t( effect );
    }
  }

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    burn->execute_on_target( trigger_state->target );
  }
};

void item::empty_drinking_horn( special_effect_t& effect )
{
  effect.proc_flags2_ = PF2_ALL_HIT;

  new empty_drinking_horn_cb_t( effect );
}

void item::discordant_chorus( special_effect_t& effect )
{
  struct fel_cleave_t: public melee_attack_t
  {
    fel_cleave_t( const special_effect_t& effect ):
      melee_attack_t( "fel_cleave", effect.player, effect.driver() -> effectN( 1 ).trigger() )
    {
      background = special = may_crit = true;
      callbacks = false;
      base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );
      weapon_multiplier = 0;
      aoe = -1;
      item = effect.item;
    }
  };

  action_t* action = effect.player -> find_action( "fel_cleave" );

  if ( !action )
  {
    action = effect.player -> create_proc_action( "fel_cleave", effect );
  }

  if ( !action )
  {
    action = new fel_cleave_t( effect );
  }

  effect.execute_action = action;
  effect.proc_flags2_ = PF2_ALL_HIT;

  new dbc_proc_callback_t( effect.player, effect );
}

struct hammering_blows_buff_t : public stat_buff_t
{
  struct enable_event_t : public event_t
  {
    dbc_proc_callback_t* driver;

    enable_event_t( dbc_proc_callback_t* cb )
      : event_t( *cb->listener, timespan_t::zero() ), driver( cb )
    {
    }

    const char* name() const override
    { return "hammering_blows_enable_event"; }

    void execute() override
    {
      driver -> activate();
    }
  };

  dbc_proc_callback_t* stack_driver;

  hammering_blows_buff_t( const special_effect_t& source_effect ) :
    stat_buff_t( source_effect.player, "hammering_blows",
                 source_effect.trigger(), source_effect.item ),
    stack_driver( nullptr )
  {
    set_refresh_behavior( buff_refresh_behavior::DISABLED );
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    bool state_change = current_stack == 0;
    stat_buff_t::execute( stacks, value, duration );

    if ( state_change )
    {
      make_event<enable_event_t>( *sim, stack_driver );
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    stat_buff_t::expire_override( expiration_stacks, remaining_duration );

    stack_driver -> deactivate();
  }

  void reset() override
  {
    stat_buff_t::reset();

    stack_driver -> deactivate();
  }
};

struct hammering_blows_driver_cb_t : public dbc_proc_callback_t
{
  hammering_blows_driver_cb_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect )
  { }

  void execute( action_t* /* a */, action_state_t* /* state */ ) override
  {
    int stack = proc_buff -> check();

    if ( stack == 0 )
    {
      proc_buff -> trigger();
    }
    // Kludge refresh, since the buff is refresh-disabled and we have no way of bumping stacks
    // without refreshing the whole buff.
    else
    {
      proc_buff -> refresh_behavior = buff_refresh_behavior::DURATION;
      proc_buff -> refresh( 0, buff_t::DEFAULT_VALUE(), proc_buff -> data().duration() );
      proc_buff -> refresh_behavior = buff_refresh_behavior::DISABLED;
    }
  }
};

// Secondary initialization for the stack-gain driver for insatiable hunger
void insatiable_hunger_2( special_effect_t& effect )
{
  effect.proc_chance_ = 1.0;
  effect.custom_buff = buff_t::find( effect.player, "hammering_blows" );

  hammering_blows_buff_t* b = static_cast<hammering_blows_buff_t*>( effect.custom_buff );

  b -> stack_driver = new dbc_proc_callback_t( effect.player, effect );
}

void item::insatiable_hunger( special_effect_t& effect )
{
  // Setup a secondary driver when the buff is up to generate stacks to it
  special_effect_t* effect_driver = new special_effect_t( effect.player );
  effect_driver -> type = SPECIAL_EFFECT_EQUIP;
  effect_driver -> name_str = "hammering_blows_driver";
  effect_driver -> spell_id = effect.trigger() -> id();
  effect_driver -> custom_init = insatiable_hunger_2;

  // And make it a player-special effect for now
  effect.player -> special_effects.push_back( effect_driver );

  // Instatiate the actual buff
  effect.custom_buff = new hammering_blows_buff_t( effect );

  new hammering_blows_driver_cb_t( effect );
}

// Int DPS 4 trinket aoe effect, emits from the Mark of Doomed target
struct doom_nova_t : public spell_t
{
  doom_nova_t( const special_effect_t& effect ) :
    spell_t( "doom_nova", effect.player, effect.player -> find_spell( 184075 ) )
  {
    background = may_crit = true;
    callbacks = false;

    base_dd_min = base_dd_max = data().effectN( 1 ).average( effect.item );
    item = effect.item;

    aoe = -1;
  }
};

// Callback driver that executes the Doom Nova on the enemy when the Mark of Doom
// debuff is up.
struct mark_of_doom_damage_driver_t : public dbc_proc_callback_t
{
  action_t* damage;
  player_t* target;

  mark_of_doom_damage_driver_t( const special_effect_t& effect, action_t* d, player_t* t ) :
    dbc_proc_callback_t( effect.player, effect ), damage( d ), target( t )
  { }

  void trigger( action_t* a, action_state_t* s ) override
  {
    if ( s -> target != target )
    {
      return;
    }

    dbc_proc_callback_t::trigger( a, s );
  }

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    damage -> target = trigger_state -> target;
    damage -> execute();
  }
};

// Specialized Mark of Doom debuff, enables/disables mark_of_doom_driver_t (above) when the debuff
// goes up/down.
struct mark_of_doom_t : public buff_t
{
  mark_of_doom_damage_driver_t* driver_cb;
  action_t* damage_spell;
  special_effect_t* effect;

  mark_of_doom_t( const actor_pair_t& p, const special_effect_t* e, const spell_data_t* s, action_t* a )
    : buff_t( p, "mark_of_doom", s, e ? e->item : nullptr ), damage_spell( a )
  {
    set_activated( false );

    // Special effect to drive the AOE damage callback
    effect = new special_effect_t( p.source );
    effect -> name_str = "mark_of_doom_damage_driver";
    effect -> proc_chance_ = 1.0;
    effect -> proc_flags_ = PF_MAGIC_SPELL | PF_NONE_SPELL;
    effect -> proc_flags2_ = PF2_ALL_HIT;
    p.source -> special_effects.push_back( effect );

    // And create, initialized and deactivate the callback
    driver_cb = new mark_of_doom_damage_driver_t( *effect, damage_spell, p.target );
    driver_cb -> initialize();
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    driver_cb -> deactivate();
  }

  void execute( int stacks, double value, timespan_t duration ) override
  {
    buff_t::execute( stacks, value, duration );

    driver_cb -> activate();
  }

  void reset() override
  {
    buff_t::reset();

    driver_cb -> deactivate();
  }
};

// Prophecy of Fear base driver, handles the proccing (triggering) of Mark of Doom on targets
struct prophecy_of_fear_driver_t : public dbc_proc_callback_t
{
  action_t* damage;
  const special_effect_t* eff;

  prophecy_of_fear_driver_t( const special_effect_t& effect ) :
    dbc_proc_callback_t( effect.player, effect ), eff( &effect )
  {
    damage = new doom_nova_t( effect );
  }

  buff_t* create_debuff( player_t* t ) override
  {
    return new mark_of_doom_t( { t, listener }, eff, eff->trigger(), damage );
  }

  void execute( action_t* /* a */, action_state_t* trigger_state ) override
  {
    get_debuff( trigger_state->target )->trigger();
  }
};

void item::prophecy_of_fear( special_effect_t& effect )
{
  effect.proc_flags_ = effect.driver() -> proc_flags() | PF_NONE_SPELL;
  effect.proc_flags2_ = PF2_ALL_HIT;

  new prophecy_of_fear_driver_t( effect );
}

struct darklight_ray_t : public spell_t
{
  darklight_ray_t( const special_effect_t& effect ) :
    spell_t( "darklight_ray", effect.player, effect.player -> find_spell( 183950 ) )
  {
    background = may_crit = true;
    callbacks = false;
    item = effect.item;

    base_dd_min = base_dd_max = effect.driver() -> effectN( 1 ).average( effect.item );

    aoe = -1;
  }
};

void item::unblinking_gaze_of_sethe( special_effect_t& effect )
{
  action_t* action = effect.player -> find_action( "darklight_ray" );
  if ( ! action )
  {
    action = effect.player -> create_proc_action( "darklight_ray", effect );
  }

  if ( ! action )
  {
    action = new darklight_ray_t( effect );
  }

  effect.execute_action = action;
  effect.proc_flags2_= PF2_ALL_HIT;

  new dbc_proc_callback_t( effect.player, effect );
}

struct soul_capacitor_explosion_t : public spell_t
{
  double explosion_multiplier;

  soul_capacitor_explosion_t( player_t* player, special_effect_t& effect ) :
    spell_t( "spirit_eruption", player, player -> find_spell( 184559 ) ),
    explosion_multiplier( 0 )
  {
    background = split_aoe_damage = true;
    callbacks = false;
    aoe = -1;
    item = effect.item;

    explosion_multiplier = 1.0 + player -> find_spell( effect.spell_id ) -> effectN( 1 ).average( effect.item ) / 10000.0;
  }

  void init() override
  {
    spell_t::init();

    snapshot_flags = STATE_MUL_DA;
    update_flags = 0;
  }

  double composite_da_multiplier( const action_state_t* ) const override
  { return explosion_multiplier; }
};


struct soul_capacitor_buff_t;

struct spirit_shift_explode_callback_t
{
  soul_capacitor_buff_t* buff;

  spirit_shift_explode_callback_t( soul_capacitor_buff_t* b );
  void operator()(player_t*);
};


struct soul_capacitor_buff_t : public buff_t
{
  // Explosion here
  spell_t* explosion;

  soul_capacitor_buff_t( player_t* player, special_effect_t& effect ) :
    buff_t( player, "spirit_shift", player -> find_spell( 184293 ), effect.item ),
    explosion( new soul_capacitor_explosion_t( player, effect ) )
  { }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );

    if ( current_value > 0 && ! player -> is_sleeping() )
    {
      explosion -> base_dd_min = explosion -> base_dd_max = current_value;
      explosion -> execute();
    }
  }
};

spirit_shift_explode_callback_t::spirit_shift_explode_callback_t( soul_capacitor_buff_t* b ) :
  buff( b )
{ }

void spirit_shift_explode_callback_t::operator()(player_t* player)
{
  if ( player != player -> sim -> target )
  {
    return;
  }

  buff -> expire();
}

void item::soul_capacitor( special_effect_t& effect )
{
  auto b = new soul_capacitor_buff_t( effect.player, effect );

  effect.custom_buff = b;
  // Perform Spirit Shift accounting just before the target actor is damaged. Spirit Shift will stop
  // the assessing of the state object.
  effect.player -> assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, [ b ]( result_amount_type, action_state_t* state ) {
    player_t* p = state -> action -> player;
    if ( ! b -> up() || ! state -> target -> is_enemy() )
    {
      return assessor::CONTINUE;
    }

    if ( p -> sim -> debug )
    {
      p -> sim -> out_debug.printf( "%s spirit_shift accumulates %.0f damage.",
        p -> name(), state -> result_amount );
    }

    b -> current_value += state -> result_amount;
    // All damage is absorbed, so make the result be zero
    state -> result_amount = 0;

    return assessor::STOP;
  } );

  effect.activation_cb = [ b ]() {
    b->sim->target_non_sleeping_list.register_callback( spirit_shift_explode_callback_t( b ) );
  };

  new dbc_proc_callback_t( effect.player, effect );
}


const char* BLADEMASTER_PET_NAME = "mirror_image_(trinket)";

struct felstorm_tick_t : public melee_attack_t
{
  felstorm_tick_t( pet_t* p ) :
    melee_attack_t( "felstorm_tick", p, p -> find_spell( 184280 ) )
  {
    school = SCHOOL_PHYSICAL;

    background = special = may_crit = true;
    callbacks = false;
    aoe = -1;
    range = data().effectN( 1 ).radius();

    weapon = &( p -> main_hand_weapon );
  }

  void init_finished() override
  {
    // Find first blademaster pet, it'll be the first trinket-created pet
    pet_t* main_pet = player -> cast_pet() -> owner -> find_pet( BLADEMASTER_PET_NAME );
    if ( player != main_pet )
    {
      stats = main_pet -> find_action( "felstorm_tick" ) -> stats;
    }

    melee_attack_t::init_finished();
  }
};

struct felstorm_t : public melee_attack_t
{
  felstorm_t( pet_t* p, const util::string_view opts ) :
    melee_attack_t( "felstorm", p, p -> find_spell( 184279 ) )
  {
    parse_options( opts );

    callbacks = may_miss = may_block = may_parry = false;
    dynamic_tick_action = hasted_ticks = true;
    trigger_gcd = timespan_t::from_seconds( 1.0 );

    tick_action = new felstorm_tick_t( p );
  }

  // Make dot long enough to last for the duration of the summon
  timespan_t composite_dot_duration( const action_state_t* ) const override
  { return sim -> expected_iteration_time; }

  void init_finished() override
  {
    pet_t* main_pet = player -> cast_pet() -> owner -> find_pet( BLADEMASTER_PET_NAME );
    if ( player != main_pet )
    {
      stats = main_pet -> find_action( "felstorm" ) -> stats;
    }

    melee_attack_t::init_finished();
  }
};

struct blademaster_pet_t : public pet_t
{
  action_t* felstorm;

  blademaster_pet_t( player_t* owner ) :
    pet_t( owner -> sim, owner, BLADEMASTER_PET_NAME, true, true ),
    felstorm( nullptr )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    // Verified 5/11/15, TODO: Check if this is still the same on live
    owner_coeff.ap_from_ap = 1.0;

    // Magical constants for base damage
    double damage_range = 0.4;
    double base_dps = owner -> dbc->spell_scaling( PLAYER_SPECIAL_SCALE, owner -> level() ) * 4.725;
    double min_dps = base_dps * ( 1 - damage_range / 2.0 );
    double max_dps = base_dps * ( 1 + damage_range / 2.0 );
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg =  min_dps * main_hand_weapon.swing_time.total_seconds();
    main_hand_weapon.max_dmg =  max_dps * main_hand_weapon.swing_time.total_seconds();
  }

  timespan_t available() const override
  { return timespan_t::from_seconds( 20.0 ); }

  void init_action_list() override
  {
    action_list_str = "felstorm,if=!ticking";

    pet_t::init_action_list();
  }

  void dismiss( bool expired = false ) override
  {
    pet_t::dismiss( expired );

    if ( dot_t* d = felstorm -> find_dot( felstorm -> target ) )
    {
      d -> cancel();
    }
  }

  action_t* create_action( util::string_view name,
                           util::string_view options_str ) override
  {
    if ( name == "felstorm" )
    {
      felstorm = new felstorm_t( this, options_str );
      return felstorm;
    }

    return pet_t::create_action( name, options_str );
  }
};

struct burning_mirror_t : public spell_t
{
  size_t n_mirrors;
  std::vector<pet_t*> pets;
  const spell_data_t* summon_spell;

  burning_mirror_t( const special_effect_t& effect ) :
    spell_t( "burning_mirror", effect.player, effect.driver() ),
    n_mirrors( static_cast<size_t>( effect.driver() -> effectN( 1 ).base_value() ) ),
    summon_spell( effect.player -> find_spell( 184271 ) )
  {
    background = true;
    may_miss = may_crit = callbacks = harmful = false;

    if ( effect.player -> specialization() == HUNTER_BEAST_MASTERY )
    {
      n_mirrors /= 2;
    }

    bool use_custom = true;
    for ( size_t i = 0; i < n_mirrors; ++i )
    {
      pet_t* blade_master = nullptr;
      if ( use_custom )
        blade_master = effect.player -> create_pet( BLADEMASTER_PET_NAME );
      if ( blade_master == nullptr )
      {
        use_custom = false;
        blade_master = new blademaster_pet_t( effect.player );
      }
      pets.push_back( blade_master );

      // Spawn every other image in front of the target
      if ( i % 2 )
        pets[ i ] -> base.position = POSITION_FRONT;
    }
  }

  void execute() override
  {
    spell_t::execute();

    for ( size_t i = 0; i < n_mirrors; ++i )
    {
      pets[ i ] -> summon( summon_spell -> duration() );
    }
  }
};

void item::mirror_of_the_blademaster( special_effect_t& effect )
{
  // Disabled because pet data is severely out of date.
  effect.type = SPECIAL_EFFECT_NONE;
  return;

  action_t* action = effect.player -> find_action( "burning_mirror" );
  if ( ! action )
  {
    action = effect.player -> create_proc_action( "burning_mirror", effect );
  }

  if ( ! action )
  {
    action = new burning_mirror_t( effect );
  }

  effect.execute_action = action;
  // Changing the special effect type to _USE (from _CUSTOM) is necessary so use_item_t action can
  // properly detect the special effect.
  effect.type = SPECIAL_EFFECT_USE;
}

void item::tyrants_decree( special_effect_t& effect )
{
  struct tyrants_decree_callback_t : public dbc_proc_callback_t
  {
    double cancel_threshold;
    player_t* p;
    buff_t* tyrant;

    tyrants_decree_callback_t( player_t* player, const special_effect_t& effect, buff_t* b ) :
      dbc_proc_callback_t( player, effect ), p( player ), tyrant( b )
    {
      cancel_threshold = effect.driver() -> effectN( 2 ).percent();
    }

    void trigger( action_t* , action_state_t* ) override
    {
      if ( p -> resources.pct( RESOURCE_HEALTH ) < cancel_threshold )
        tyrant->expire();
    }
  };

  auto trigger =
      make_buff<stat_buff_t>( effect.player, "tyrants_immortality", effect.player->find_spell( 184770 ) )
          ->add_stat( STAT_STAMINA, effect.player->find_spell( 184770 )->effectN( 1 ).average( effect.item ) )
          ->set_duration( timespan_t::zero() );  // indefinite, this will never expire naturally so might as well save
                                                 // some CPU cycles

  auto driver = make_buff( effect.player, "tyrants_decree_driver", effect.driver() )
    ->set_period( effect.driver()->effectN( 1 ).period() )
    ->set_tick_behavior( buff_tick_behavior::REFRESH )
    ->set_quiet( true )
    ->set_tick_callback( [ trigger ]( buff_t* b, int, timespan_t ) {
      if ( b->player->resources.pct( RESOURCE_HEALTH ) > b->data().effectN( 2 ).percent() )
        trigger->trigger();
    } );

  // Assume actor has stacked the buff to max stack precombat.
  effect.player->register_on_arise_callback( effect.player, [ driver, trigger ]() {
    driver->trigger();
    trigger->trigger( trigger->max_stack() );
  } );

  // Create a callback that triggers on damage taken to check if the buff should be expired.
  effect.proc_flags_= PF_DAMAGE_TAKEN;
  effect.proc_chance_ = 1.0;

  new tyrants_decree_callback_t( effect.player, effect, trigger );
}

void item::warlords_unseeing_eye( special_effect_t& effect )
{
  // Store the magic mitigation number in a player-scope variable.
  auto magic = effect.driver()->effectN( 2 ).average( effect.item ) / 10000.0;
  // Register our handler function so it can be managed by player_t::account_absorb_buffs()
  effect.player->instant_absorb_list.insert( std::make_pair<unsigned, instant_absorb_t>(
      effect.driver()->id(),
      instant_absorb_t( effect.player, effect.driver(), "warlords_unseeing_eye", [ magic ]( const action_state_t* s ) {
        /* Absorb is based on what the player's HP would be after taking the damage,
          accounting for absorbs that occur prior but ignoring the rest.

          Max current health to 0 to keep the trinket's value somewhat sane when the
          actor goes negative.

          TOCHECK: If the actor would die from the hit, does the size of the absorb
          scale with the amount of overkill? */
        player_t* p = s->target;

        double absorb_amount = s->result_amount * magic;

        absorb_amount *= 1 - ( std::max( p->resources.current[ RESOURCE_HEALTH ], 0.0 ) - s->result_amount ) /
                                 p->resources.max[ RESOURCE_HEALTH ];

        return absorb_amount;
      } ) ) );

  // Push the effect into the priority list.
  effect.player -> absorb_priority.push_back( 184762 );
}

// Normally this would be parsed automatically, however since we already use soul_fragment for
// other buffs we need to specifically define this so they don't trigger each other
void item::necromantic_focus( special_effect_t& effect )
{
  auto buff = buff_t::find( effect.player, "soul_fragment_trinket" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "soul_fragment_trinket", effect.trigger() )
               ->add_stat( STAT_MASTERY_RATING, effect.trigger()->effectN( 1 ).average( effect.item ) );
  }

  effect.proc_flags_  = PF_PERIODIC;
  effect.proc_flags2_ = PF2_ALL_HIT | PF2_PERIODIC_DAMAGE;
  effect.custom_buff  = buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void item::sorrowsong( special_effect_t& effect )
{
  new dbc_proc_callback_t( effect.player, effect );

  effect.player->callbacks.register_callback_trigger_function(
      effect.driver()->id(), dbc_proc_callback_t::trigger_fn_type::CONDITION,
      [ effect ]( const dbc_proc_callback_t*, action_t*, action_state_t* s ) {
        return s->target->health_percentage() <= effect.driver()->effectN( 1 ).base_value();
      } );
}

struct touch_of_the_grave_t : public spell_t
{
  touch_of_the_grave_t( player_t* p, const spell_data_t* spell ) :
    spell_t( "touch_of_the_grave", p, spell )
  {
    background = may_crit = true;
    base_dd_min = base_dd_max = 0;
    ap_type = attack_power_type::NO_WEAPON;
    // these are sadly hardcoded in the tooltip
    attack_power_mod.direct = 1.25 * .25;
    spell_power_mod.direct = 1.0 * .25;
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.direct * s -> composite_attack_power();
    const double sp = spell_power_mod.direct * s -> composite_spell_power();

    if ( ap <= sp )
      return 0;
    return spell_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.direct * s -> composite_attack_power();
    const double sp = spell_power_mod.direct * s -> composite_spell_power();

    if ( ap > sp )
      return 0;
    return spell_t::spell_direct_power_coefficient( s );
  }
};

void racial::touch_of_the_grave( special_effect_t& effect )
{
  effect.execute_action = new touch_of_the_grave_t( effect.player, effect.trigger() );

  new dbc_proc_callback_t( effect.player, effect );
}

struct entropic_embrace_damage_t : public spell_t
{
  entropic_embrace_damage_t( const special_effect_t& effect ) :
    spell_t( "entropic_embrace", effect.player, effect.player -> find_spell( 259756 ) )
  {
    background = true;
    may_miss = callbacks = false;
  }

  void init() override
  {
    spell_t::init();

    snapshot_flags = update_flags = STATE_TGT_MUL_DA | STATE_TGT_MUL_TA;
  }
};

struct entropic_embrace_damage_cb_t : public dbc_proc_callback_t
{
  double coeff;
  entropic_embrace_damage_cb_t( const special_effect_t* effect, double c )
    : dbc_proc_callback_t( effect->player, *effect ), coeff( c )
  {}

  void trigger( action_t* a, action_state_t* state ) override
  {
    if ( state->result_amount <= 0 )
    {
      return;
    }

    dbc_proc_callback_t::trigger( a, state );
  }

  void execute( action_t* /* a */, action_state_t* state ) override
  {
    proc_action->base_dd_min = state->result_amount * coeff;
    proc_action->base_dd_max = proc_action->base_dd_min;

    proc_action->set_target( state->target );
    proc_action->execute();
  }
};

void racial::entropic_embrace( special_effect_t& effect )
{
  special_effect_t* effect_driver = new special_effect_t( effect.player );
  effect_driver->source = SPECIAL_EFFECT_SOURCE_RACE;
  effect_driver->type = SPECIAL_EFFECT_EQUIP;
  // TODO: healing proc NYI
  effect_driver->proc_flags_ = effect.trigger()->proc_flags() & ~( PF_NONE_HEAL | PF_MAGIC_HEAL | PF_HELPFUL_PERIODIC );
  effect_driver->proc_flags2_ = PF2_ALL_HIT;
  effect_driver->name_str = "entropic_embrace_damage_driver";
  effect_driver->spell_id = effect.trigger()->id();
  effect_driver->execute_action = create_proc_action<entropic_embrace_damage_t>( "entropic_embrace", effect );
  effect.player->special_effects.push_back( effect_driver );

  auto proc = new entropic_embrace_damage_cb_t( effect_driver, effect.trigger()->effectN( 1 ).percent() );
  proc->deactivate();

  buff_t* base_buff = buff_t::find( effect.player, "entropic_embrace" );
  if ( base_buff == nullptr )
  {
    base_buff = make_buff( effect.player, "entropic_embrace", effect.trigger() )
      ->set_stack_change_callback( [ proc ]( buff_t*, int, int new_ ) {
        if ( new_ > 0 )
          proc->activate();
        else
          proc->deactivate();
      } );
  }

  effect.custom_buff = base_buff;

  new dbc_proc_callback_t( effect.player, effect );
}

void racial::brush_it_off( special_effect_t& effect )
{
  struct brush_it_off_cb_t : public dbc_proc_callback_t
  {
    action_t* regen;
    double heal_pct;

    brush_it_off_cb_t( const special_effect_t& e )
      : dbc_proc_callback_t( e.player, e ), heal_pct( e.driver()->effectN( 2 ).percent() )
    {
      regen = new residual_action::residual_periodic_action_t<proc_heal_t>( "brush_it_off", e.player,
                                                                            e.player->find_spell( 291843 ) );
    }

    void execute( action_t*, action_state_t* s ) override
    {
      residual_action::trigger( regen, listener, s->result_amount * heal_pct );
    }
  };

  new brush_it_off_cb_t( effect );
}

struct embrace_of_bwonsamdi_t : public spell_t
{
  embrace_of_bwonsamdi_t(player_t* p, const spell_data_t* sd) :
    spell_t("embrace_of_bwonsamdi", p, sd)
  {
    background = true;
    ap_type = attack_power_type::NO_WEAPON; //TOCHECK: Is this true? Based off of Touch of the Grave right now.
    base_dd_min = base_dd_max = 0;
    //Hardcoded tooltip values
    attack_power_mod.direct = 0.22;
    spell_power_mod.direct = 0.22;
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.direct * s -> composite_attack_power();
    const double sp = spell_power_mod.direct * s -> composite_spell_power();

    if ( ap <= sp )
      return 0;
    return spell_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.direct * s -> composite_attack_power();
    const double sp = spell_power_mod.direct * s -> composite_spell_power();

    if ( ap > sp )
      return 0;
    return spell_t::spell_direct_power_coefficient( s );
  }
};

struct embrace_of_kimbul_t : public spell_t
{
  embrace_of_kimbul_t(player_t* p, const spell_data_t* sd) :
    spell_t("embrace_of_kimbul", p, sd)
  {
    tick_may_crit = false; //TOCHECK: Longer test needed with max level character for these values
    background = true;
    hasted_ticks = false;
    dot_max_stack = sd->max_stacks();
    dot_behavior = DOT_REFRESH_DURATION;
    ap_type = attack_power_type::NO_WEAPON;
    attack_power_mod.tick = 0.075; //Hardcoded in tooltip
    spell_power_mod.tick = 0.075;
  }

  double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.tick * s -> composite_attack_power();
    const double sp = spell_power_mod.tick * s -> composite_spell_power();

    if ( ap <= sp )
      return 0;
    return spell_t::attack_tick_power_coefficient( s );
  }

  double spell_tick_power_coefficient( const action_state_t* s ) const override
  {
    const double ap = attack_power_mod.tick * s -> composite_attack_power();
    const double sp = spell_power_mod.tick * s -> composite_spell_power();

    if ( ap > sp )
      return 0;
    return spell_t::spell_tick_power_coefficient( s );
  }
};

void racial::zandalari_loa( special_effect_t& effect )
{
  if ( create_fallback_buffs( effect, { "embrace_of_paku" } ) )
    return;

  if ( effect.player->zandalari_loa == player_t::AKUNDA )
  {
    // Akunda - Healing Proc (not implemented)
  }
  else if ( effect.player->zandalari_loa == player_t::GONK )
  {
    effect.player->passive_modifier += effect.player->find_spell( 292362 )->effectN( 1 ).percent();
  }
  else if ( effect.player->zandalari_loa == player_t::BWONSAMDI )
  {
    // Bwonsamdi - 100% of damage done is returned as healing (healing not implemented)
    special_effect_t* driver = new special_effect_t( effect.player );
    driver->source = SPECIAL_EFFECT_SOURCE_RACE;
    unique_gear::initialize_special_effect( *driver, 292360 );
    driver->execute_action = new embrace_of_bwonsamdi_t( effect.player, effect.player->find_spell( 292380 ) );

    effect.player->special_effects.push_back( driver );

    new dbc_proc_callback_t( effect.player, *driver );
  }
  else if ( effect.player->zandalari_loa == player_t::KIMBUL )
  {
    // Kimbul - Chance to apply bleed dot, max stack of 3
    special_effect_t* driver = new special_effect_t( effect.player );
    driver->source = SPECIAL_EFFECT_SOURCE_RACE;
    unique_gear::initialize_special_effect( *driver, 292363 );
    driver->execute_action = new embrace_of_kimbul_t( effect.player, effect.player->find_spell( 292473 ) );

    effect.player->special_effects.push_back( driver );

    new dbc_proc_callback_t( effect.player, *driver );
  }
  else if ( effect.player->zandalari_loa == player_t::KRAGWA )
  {
    // Kragwa - Grants health and armor (not implemented)
  }
  else if ( effect.player->zandalari_loa == player_t::PAKU )
  {
    special_effect_t* driver = new special_effect_t( effect.player );
    driver->source = SPECIAL_EFFECT_SOURCE_RACE;
    unique_gear::initialize_special_effect( *driver, 292361 );  // Permanent buff spell id, contains proc data

    // Paku - Grants crit chance
    buff_t* paku = buff_t::find( effect.player, "embrace_of_paku" );
    if ( paku == nullptr )
    {
      // Buff spell data contains duration and amount
      paku = make_buff( effect.player, "embrace_of_paku", effect.player->find_spell( 292463 ) )
                 ->set_default_value_from_effect_type( A_MOD_ALL_CRIT_CHANCE )
                 ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );
    }

    driver->custom_buff = paku;

    effect.player->special_effects.push_back( driver );

    new dbc_proc_callback_t( driver->player, *driver );
  }
}

void racial::combat_analysis( special_effect_t& effect )
{
  const spell_data_t* buff_spell = effect.player->find_spell( 312923 );
  effect.stat                    = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );

  buff_t* buff = buff_t::find( effect.player, "combat_analysis" );
  if ( !buff )
  {
    buff = make_buff<stat_buff_t>( effect.player, "combat_analysis", buff_spell )
               ->add_stat( effect.stat, buff_spell->effectN( 1 ).average( effect.player ) );
    buff->set_max_stack( as<int>( buff_spell->effectN( 3 ).base_value() ) );
  }

  effect.player->register_combat_begin( [buff, buff_spell]( player_t* ) {
    make_repeating_event( *buff->sim, buff_spell->effectN( 1 ).period(), [buff]() { buff->trigger(); } );
  } );
}

void generic::skyfury( special_effect_t& effect )
{
  struct skyfury_cb_t : public dbc_proc_callback_t
  {
    proc_t* proc_mh, *proc_oh;

    skyfury_cb_t( const special_effect_t& effect ) :
      dbc_proc_callback_t( effect.player, effect ), proc_mh( nullptr ), proc_oh( nullptr )
    {
      if ( effect.player->items[ SLOT_MAIN_HAND ].active() &&
           effect.player->items[ SLOT_MAIN_HAND ].dbc_inventory_type() != INVTYPE_RANGED )
      {
        proc_mh = effect.player->get_proc( "Skyfury (Main Hand)" );
      }

      if ( effect.player->items[ SLOT_OFF_HAND ].active() &&
           effect.player->items[ SLOT_OFF_HAND ].dbc_inventory_type() != INVTYPE_RANGED )
      {
        proc_oh = effect.player->get_proc( "Skyfury (Off Hand)" );
      }
    }

    void trigger( action_t* a, action_state_t* s ) override
    {
      if ( !s->action->weapon )
      {
        return;
      }

      auto action = s->action->weapon->slot == SLOT_MAIN_HAND
        ? s->action->player->main_hand_attack
        : s->action->player->off_hand_attack;

      // If for some reason there's no auto attack action that would initialize the attack, bail out
      if ( !action )
      {
        return;
      }

      dbc_proc_callback_t::trigger( a, s );
    }

    void execute( action_t*, action_state_t* state ) override
    {
      auto atk = state->action->weapon->slot == SLOT_MAIN_HAND
        ? state->action->player->main_hand_attack
        : state->action->player->off_hand_attack;
      auto proc = state->action->weapon->slot == SLOT_MAIN_HAND
        ? proc_mh
        : proc_oh;

      auto old_target = atk->target;

      listener->sim->print_log( "{} skyfury repeats {}", *listener, *atk );
      if ( proc )
      {
        proc->occur();
      }

      auto old_may_miss = atk->may_miss;

      atk->may_miss = false;
      atk->repeating = false;
      atk->set_target( state->target );
      atk->execute();

      atk->repeating = true;
      atk->may_miss = old_may_miss;
      atk->set_target( old_target );
    }
  };

  if ( effect.player->is_enemy() || effect.player->type == HEALING_ENEMY )
  {
    return;
  }

  new skyfury_cb_t( effect );
}

void generic::enable_all_item_effects( special_effect_t& effect )
{
  if ( !effect.player->sim->enable_all_item_effects )
  {
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  struct enable_all_item_effects_t : public action_t
  {
    std::vector<special_effect_t*> action_effects;
    std::vector<special_effect_t*> buff_effects;

    enable_all_item_effects_t( const special_effect_t& e )
      : action_t( action_e::ACTION_USE, "enable_all_item_effects", e.player )
    {
      callbacks = false;
      cooldown->duration = 20_s;
    }

    void init() override
    {
      action_t::init();

      for ( auto id : thewarwithin::__tww_special_effect_ids )
      {
        if ( auto eff = find_special_effect( player, id ) )
        {
          if ( eff->custom_buff )
            buff_effects.push_back( eff );

          if ( eff->execute_action )
            action_effects.push_back( eff );
        }
      }
    }

    result_e calculate_result( action_state_t* ) const override
    {
      return result_e::RESULT_NONE;
    }

    void execute() override
    {
      action_t::execute();

      for ( auto eff : buff_effects )
        eff->custom_buff->trigger();

      for ( auto eff : action_effects )
        if ( eff->execute_action->action_ready() )
          eff->execute_action->execute();
    }
  };

  effect.execute_action = new enable_all_item_effects_t( effect );
}

bool stat_fits_criteria( stat_e stat, stat_e criteria )
{
  if ( !stat )
    return false;

  if ( criteria == STAT_ALL )
    return true;

  if ( criteria == STAT_ANY_DPS )
    return stat != STAT_LEECH_RATING && stat != STAT_SPEED_RATING && stat != STAT_AVOIDANCE_RATING;

  return stat == criteria;
}

// Figure out if a given generic buff (associated with a trinket/item) is a
// stat buff of the correct type
bool buff_has_stat( const buff_t* buff, stat_e stat )
{
  if ( ! buff )
    return false;

  // Not a stat buff
  const stat_buff_t* stat_buff = dynamic_cast< const stat_buff_t* >( buff );
  if ( ! stat_buff )
    return false;

  // At this point, if "any" was specificed, we're satisfied
  if ( stat == STAT_ALL )
    return true;

  // TODO: Probably needs more cases or potentially a more elegant solution here
  //       Just filter tertiaries for now since they are present on some DPS trinket use effects
  if ( stat == STAT_ANY_DPS )
  {
    return range::any_of( stat_buff->stats, []( auto elem ) {
      return elem.stat != STAT_LEECH_RATING && elem.stat != STAT_SPEED_RATING && elem.stat != STAT_AVOIDANCE_RATING;
    } );
  }

  for ( auto & elem : stat_buff->stats )
  {
    if ( elem.stat == stat )
      return true;
  }

  return false;
}

bool action_has_damage( const action_t* action )
{
  if ( !action )
    return false;

  // check direct damage
  if ( action->has_direct_damage_effect( action->data() ) ||
       action->base_dd_min > 0 ||
       action->spell_power_mod.direct > 0 ||
       action->attack_power_mod.direct > 0 ||
       action->weapon_multiplier > 0 )
  {
    return true;
  }

  // check periodic damage
  if ( action->has_periodic_damage_effect( action->data() ) ||
       ( action->dot_duration > 0_ms &&
         ( action->base_td > 0 ||
           action->spell_power_mod.tick > 0 ||
           action->attack_power_mod.tick > 0 ||
           action->rolling_periodic ) ) )
  {
    return true;
  }

  return false;
}
} // UNNAMED NAMESPACE

item_targetdata_initializer_t::item_targetdata_initializer_t( unsigned iid, util::span<const slot_e> s )
  : targetdata_initializer_t(), item_id( iid ), spell_id( 0 ), slots_( s.begin(), s.end() )
{
  active_fn = []( const special_effect_t* e ) { return e != nullptr; };
  debuff_fn = []( player_t*, const special_effect_t* e ) { return e->trigger(); };
}

item_targetdata_initializer_t::item_targetdata_initializer_t( unsigned sid, unsigned did )
  : targetdata_initializer_t(), item_id( 0 ), spell_id( sid )
{
  active_fn = []( const special_effect_t* e ) { return e != nullptr; };

  if ( did )
    debuff_fn = [ did ]( player_t* p, const special_effect_t* ) { return p->find_spell( did ); };
  else
    debuff_fn = []( player_t*, const special_effect_t* e ) { return e->trigger(); };
}

const special_effect_t* item_targetdata_initializer_t::find( player_t* p ) const
{
  if ( spell_id )
  {
    return unique_gear::find_special_effect( p, spell_id );
  }
  else
  {
    for ( slot_e slot : slots_ )
    {
      if ( p->items[ slot ].parsed.data.id == item_id )
      {
        return p->items[ slot ].parsed.special_effects[ 0 ];
      }
    }
  }

  return nullptr;
}

bool item_targetdata_initializer_t::init( player_t* p ) const
{
  // No need to check on pets/enemies
  if ( p->is_pet() || p->is_enemy() || p->type == HEALING_ENEMY )
    return false;

  return targetdata_initializer_t::init( p );
}

const special_effect_t* item_targetdata_initializer_t::effect( actor_target_data_t* td ) const
{
  return data[ td->source ];
}

/**
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
void unique_gear::initialize_special_effect( special_effect_t& effect,
                                             unsigned          spell_id )
{
  player_t* p = effect.player;

  // Perform max level checking on the driver before anything
  const spell_data_t* spell = p -> find_spell( spell_id );
  if ( spell -> req_max_level() > 0 && as<unsigned>( p -> level() ) > spell -> req_max_level() )
  {
    if ( p -> sim -> debug )
    {
      p -> sim -> out_debug.printf( "%s disabled effect %s, player level %d higher than maximum effect level %u",
        p -> name(), spell -> name_cstr(), p -> level(), spell -> req_max_level() );
    }
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  // Try to find the special effect from the custom effect database
  for ( const auto dbitem: find_special_effect_db_item( spell_id ) )
  {
    // Parse auxilary effect options before doing spell data based parsing
    if ( ! dbitem -> encoded_options.empty() )
    {
      std::string encoded_options = dbitem -> encoded_options;
      util::tolower( encoded_options );
      // Note, if the encoding parse fails (this should never ever happen),
      // we don't parse game client data either.
      special_effect::parse_special_effect_encoding( effect, encoded_options );
    }
    else if ( dbitem -> cb_obj )
    {
      // Check that the custom special effect initializer is valid. Invalid special effect
      // validators could be for example spec-specific initializers (see scoped_action_callback_t
      // and child classes derived off of it).
      if ( ! dbitem -> cb_obj -> valid( effect ) )
      {
        continue;
      }

      // Custom special effect initialization is deferred, and no parsing from spell data is done
      // automatically.
      if ( dbitem -> cb_obj )
      {
        effect.custom_init_object.push_back( dbitem -> cb_obj );
      }
    }
  }

  // Setup the driver always, though honoring any parsing performed in the first phase options.
  if ( effect.spell_id == 0 )
    effect.spell_id = spell_id;

  // Custom init found a valid initializer callback, this special effect will be initialized with it
  // later on
  if ( !effect.custom_init_object.empty() )
  {
    return;
  }

  // If the item is legendary, and it has an item effect, mandate that the item effect is actually
  // created through custom means. This is to prevent the automatic inference below to create
  // completely nonsensical special effects, when there is not enough client data to fully implement
  // the effect properly.
  //
  // This is mostly relevant for "simple looking" legendary effects such as Recurrent Ritual that
  // gets automatically inferred to affect all (warlock) spells globally.
  if ( effect.custom_init_object.empty() && effect.item &&
       effect.source == SPECIAL_EFFECT_SOURCE_ITEM &&
       effect.item->parsed.data.quality == ITEM_QUALITY_LEGENDARY )
  {
    if ( p -> sim -> debug )
    {
      p -> sim -> out_debug.printf( "Player %s no custom special effect initializer for item %s, "
                                    "spell %s (id=%u), disabling effect",
        p -> name(), effect.item -> name(), p -> find_spell( spell_id ) -> name_cstr(), spell_id );
    }
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  // No custom effect found, so ensure that we have spell data for the driver
  if ( p -> find_spell( effect.spell_id ) -> id() != effect.spell_id )
  {
    if ( p -> sim -> debug )
      p -> sim -> out_debug.printf( "Player %s unable to initialize special effect in item %s, spell identifier %u not found.",
          p -> name(), effect.item ? effect.item -> name() : "unknown", effect.spell_id );
    effect.type = SPECIAL_EFFECT_NONE;
    return;
  }

  // For generic procs, make sure we have a PPM, RPPM or Proc Chance available,
  // otherwise there's no point in trying to proc anything
  if ( effect.type == SPECIAL_EFFECT_EQUIP )
  {
    if (!special_effect::usable_proc( effect ))
    {
      effect.type = SPECIAL_EFFECT_NONE;
    }
  }

  // For generic use stuff, we need to have a proper buff or action that we can generate
  else if ( effect.type == SPECIAL_EFFECT_USE &&
            effect.buff_type() == SPECIAL_EFFECT_BUFF_NONE &&
            effect.action_type() == SPECIAL_EFFECT_ACTION_NONE )
  {
    effect.type = SPECIAL_EFFECT_NONE;
  }
}

// Second phase initialization, creates the proc callback object for generic on-equip special
// effects, or calls the custom initialization function given in the first phase initialization.
void unique_gear::initialize_special_effect_2( special_effect_t* effect )
{
  if ( effect -> custom_init || !effect -> custom_init_object.empty() )
  {
    if ( effect -> custom_init )
    {
      effect -> custom_init( *effect );
    }
    else
    {
      range::for_each( effect -> custom_init_object, [ effect ]( scoped_callback_t* cb ) {
        cb -> initialize( *effect );
      } );
    }

    // Allow class modules to adjust the special_effect_t object generated by the core
    // special effect registry, if they want.
    effect->player->init_special_effect( *effect );
  }
  else if ( effect -> type == SPECIAL_EFFECT_EQUIP )
  {
    // Ensure we are not accidentally initializing a generic special effect multiple times
    bool exists = effect -> player -> callbacks.has_callback( [ effect ]( const action_callback_t* cb ) {
      auto dbc_cb = dynamic_cast<const dbc_proc_callback_t*>( cb );
      if ( dbc_cb == nullptr )
      {
        return false;
      }

      // Special effects are unique, and have an 1:1 relationship with (dbc proc) callbacks. Pointer
      // comparison here is enough to ensure that no special_effect_t object gets more than one
      // dbc_proc_callback_t object.
      return &( dbc_cb -> effect ) == effect;
    } );

    if ( exists )
    {
      return;
    }

    // Allow class modules to adjust the special_effect_t object generated by the core
    // special effect registry, if they want.
    effect->player->init_special_effect( *effect );

    if ( effect -> item )
    {
      new dbc_proc_callback_t( effect -> item, *effect );
    }
    else
    {
      new dbc_proc_callback_t( effect -> player, *effect );
    }
  }
}

void unique_gear::initialize_racial_effects( player_t* player )
{
  if ( player->race == RACE_NONE )
  {
    return;
  }

  if ( !util::race_id( player->race ) )
  {
    return;
  }

  // Iterate over all race spells for the player
  for ( const auto* entry : player->dbc->racial_spell( player->type, player->race ) )
  {
    auto spell = dbc::find_spell( player, entry->spell_id );
    if ( !spell || !spell->ok() )
    {
      continue;
    }

    special_effect_t effect( player );
    effect.source = SPECIAL_EFFECT_SOURCE_RACE;
    unique_gear::initialize_special_effect( effect, spell->id() );
    if ( !effect.is_custom() )
    {
      continue;
    }

    player->sim->print_debug( "Player {} initialized racial spell {} (id={}, class_mask={:#08x})",
      player->name(), spell->name_cstr(), spell->id(), entry->mask_class );

    player->special_effects.push_back( new special_effect_t( effect ) );
  }
}

// ==========================================================================
// unique_gear::init
// ==========================================================================

void unique_gear::init( player_t* p )
{
  if ( p->is_pet() || p->is_enemy() )
    return;

  for ( size_t i = 0; i < p->items.size(); i++ )
  {
    item_t& item = p->items[ i ];

    for ( size_t j = 0; j < item.parsed.special_effects.size(); j++ )
    {
      special_effect_t* effect = item.parsed.special_effects[ j ];

      p->sim->print_debug( "Initializing item-based special effect {}", *effect );

      initialize_special_effect_2( effect );
    }
  }

  // Generic special effects, bound to no specific item
  for ( size_t i = 0; i < p->special_effects.size(); i++ )
  {
    special_effect_t* effect = p->special_effects[ i ];

    p->sim->print_debug( "Initializing generic special effect {}", *effect );

    initialize_special_effect_2( effect );
  }
}

// Base class for item effect expressions, finds all the special effects in the
// listed slots
struct item_effect_base_expr_t : public expr_t
{
  std::vector<const special_effect_t*> effects;

  item_effect_base_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    expr_t( full_expression )
  {
    const special_effect_t* e = nullptr;

    for (auto slot : slots)
    {
      e = player.items[ slot ].special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_EQUIP );
      if ( e && e -> source != SPECIAL_EFFECT_SOURCE_NONE )
        effects.push_back( e );

      e = player.items[ slot ].special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE );
      if ( e && e -> source != SPECIAL_EFFECT_SOURCE_NONE )
        effects.push_back( e );
    }
  }
};

// Base class for expression-based item expressions (such as buff, or cooldown
// expressions). Implements the behavior of expression evaluation.
struct item_effect_expr_t : public item_effect_base_expr_t
{
  std::vector<std::unique_ptr<expr_t>> exprs;

  item_effect_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    item_effect_base_expr_t( player, slots, full_expression )
  { }

  // Evaluates automatically to the maximum value out of all expressions, may
  // not be wanted in all situations. Best case? We should allow internal
  // operators here somehow
  double evaluate() override
  {
    double result = 0;
    for (auto&& expr : exprs)
    {
      double r = expr -> eval();
      if ( r > result )
        result = r;
    }

    return result;
  }

  bool is_constant() override
  {
    return exprs.empty();
  }
};

// Buff based item expressions, creates buff expressions for the items from
// user input
struct item_buff_expr_t : public item_effect_expr_t
{
  item_buff_expr_t( player_t& player, const std::vector<slot_e>& slots, stat_e s, bool stacking, util::string_view expr_str ) :
    item_effect_expr_t( player, slots, expr_str )
  {
    for (auto e : effects)
    {
      buff_t* b = buff_t::find( &player, e -> name() );
      if ( buff_has_stat( b, s ) && ( ! stacking || ( stacking && b -> max_stack() > 1 ) ) )
      {
        if ( auto expr_obj = buff_t::create_expression( b -> name(), expr_str, *b ) )
          exprs.push_back( std::move(expr_obj) );
      }
    }
  }
};

struct item_buff_exists_expr_t : public item_effect_expr_t
{
  double v;

  item_buff_exists_expr_t( player_t& player, const std::vector<slot_e>& slots, stat_e s, util::string_view full_expression ) :
    item_effect_expr_t( player, slots, full_expression ), v( 0 )
  {
    for (auto e : effects)
    {
      buff_t* b = buff_t::find( &player, e -> name() );
      if ( buff_has_stat( b, s ) )
      {
        v = 1;
        break;
      }
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  { return v; }
};

// Cooldown based item expressions, creates cooldown expressions for the items
// from user input
struct item_cooldown_expr_t : public item_effect_expr_t
{
  item_cooldown_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view expr, util::string_view full_expression ) :
    item_effect_expr_t( player, slots, full_expression )
  {
    for (auto e : effects)
    {
      if ( e -> cooldown() != timespan_t::zero() )
      {
        cooldown_t* cd = player.get_cooldown( e -> cooldown_name() );
        if ( auto expr_obj = cd -> create_expression( expr ) )
          exprs.push_back( std::move(expr_obj) );
      }
    }
  }
};

struct item_cast_time_expr_t : public item_effect_expr_t
{
  double v = 0;

  item_cast_time_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    item_effect_expr_t( player, slots, full_expression ), v( 0 )
  {
    for (auto e : effects)
    {
      if ( e -> execute_action )
      {
        if ( e->execute_action->channeled )
        {
          v = e->execute_action->dot_duration.total_seconds();
        }
        else
        {
          v = e->execute_action->execute_time().total_seconds();
        }
        break;
      }
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  { return v; }
};

struct item_ready_expr_t : public item_effect_base_expr_t
{
  item_ready_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    item_effect_base_expr_t( player, slots, full_expression )
  {
  }

  double evaluate() override
  {
    for ( auto e : effects )
    {
      if ( e -> cooldown_group_duration() != timespan_t::zero() )
      {
        cooldown_t* cd = e->player->get_cooldown( e -> cooldown_group_name() );
        if ( !cd -> up() )
          return 0;
      }
      if ( e -> cooldown() != timespan_t::zero() )
      {
        cooldown_t* cd = e->player->get_cooldown( e -> cooldown_name() );
        if ( !cd -> up() )
          return 0;
      }
    }

    return 1;
  }

  bool is_constant() override
  {
    return effects.empty();
  }
};

struct item_is_expr_t : public expr_t
{
  double is = 0;

  item_is_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view item_name )
    : expr_t( "item_is_expr" )
  {
    for ( auto slot : slots )
    {
      if ( player.items[ slot ].name() == item_name )
        is = 1;
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  {
    return is;
  }
};

struct item_lvl_expr_t : public item_effect_expr_t
{
  double ilvl;
  item_lvl_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression )
    : item_effect_expr_t( player, slots, full_expression )
  {
    for (auto slot : slots)
    {
      ilvl = player.items[ slot ].item_level();
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  {
    return ilvl;
  }
};

struct item_cooldown_exists_expr_t : public item_effect_expr_t
{
  double v;

  item_cooldown_exists_expr_t( player_t& player, const std::vector<slot_e>& slots, util::string_view full_expression ) :
    item_effect_expr_t( player, slots, full_expression ), v( 0 )
  {
    for (auto e : effects)
    {
      if ( e -> cooldown() != timespan_t::zero() && e -> rppm() == 0 ) // Technically, rppm doesn't have a cooldown.
      {
        v = 1;
        break;
      }
    }
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  { return v; }
};

struct item_has_use_expr_t : public item_effect_expr_t
{
  double v;
  bool has_use;
  bool has_buff;
  bool has_damage;

  item_has_use_expr_t( player_t& player, const std::vector<slot_e>& slots, std::string_view full_expression,
                       bool check_buff, bool check_damage )
    : item_effect_expr_t( player, slots, full_expression ),
      v( 0 ),
      has_use( false ),
      has_buff( false ),
      has_damage( false )
  {
    for ( auto e : effects )
    {
      if ( e->cooldown() != 0_ms && e->rppm() == 0 )  // Technically, rppm doesn't have a cooldown.
      {
        has_use = true;
        break;
      }
    }

    if ( check_buff )
    {
      for ( auto e : effects )
      {
        // Check if there is a stat set on the special effect
        if ( stat_fits_criteria( e->stat, STAT_ANY_DPS ) )
        {
          has_buff = true;
          break;
        }

        // Check if the special effect has a suitable buff effect
        for ( size_t i = 1, end = e->trigger()->effect_count(); i <= end; i++ )
        {
          if ( has_buff )
            break;

          const spelleffect_data_t& effect = e->trigger()->effectN( i );
          if ( effect.id() == 0 )
            continue;

          if ( stat_fits_criteria( e->stat_buff_type( effect ), STAT_ANY_DPS ) )
          {
            has_buff = true;
            break;
          }

          // Check if an effect triggers something with a suitable buff effect
          if ( effect.trigger() )
          {
            for ( size_t i = 1, end = effect.trigger()->effect_count(); i <= end; i++ )
            {
              const spelleffect_data_t& trigger_effect = effect.trigger()->effectN( i );
              if ( trigger_effect.id() == 0 )
                continue;

              if ( stat_fits_criteria( e->stat_buff_type( trigger_effect ), STAT_ANY_DPS ) )
              {
                has_buff = true;
                break;
              }
            }
          }
        }

        // Check if the special effect created a suitable buff
        buff_t* b = buff_t::find( &player, e->name() );
        if ( buff_has_stat( b, STAT_ANY_DPS ) )
        {
          has_buff = true;
          break;
        }
      }
    }

    if ( check_damage )
    {
      for ( auto e : effects )
      {
        // check if action name exists
        action_t* a = player.find_action( e->name() );
        if ( action_has_damage( a ) )
        {
          has_damage = true;
          break;
        }

        // check any custom execute_action
        if ( action_has_damage( e->execute_action ) )
        {
          has_damage = true;
          break;
        }

        // check any auto-parsed actions
        if ( e->is_offensive_spell_action() || e->is_attack_action() )
        {
          has_damage = true;
          break;
        }
      }
    }

    if ( has_use && ( has_buff || !check_buff ) && ( has_damage || !check_damage ) )
      v = 1;
  }

  bool is_constant() override
  {
    return true;
  }

  double evaluate() override
  {
    return v;
  }
};

/**
 * Create "trinket" expressions, or anything relating to special effects.
 *
 * Note that this method returns zero (nullptr) when it cannot create an
 * expression.  The callee (player_t::create_expression) will handle unknown
 * expression processing.
 *
 * Trinket expressions are of the form:
 * trinket[.1|2|name].(has_|)(stacking_|)proc.<stat>.<buff_expr> OR
 * trinket[.1|2|name].(has_|)cooldown.<cooldown_expr>
 */
std::unique_ptr<expr_t> unique_gear::create_expression( player_t& player, util::string_view name_str )
{
  enum proc_expr_e
  {
    PROC_EXISTS,
    PROC_ENABLED,
    PROC_READY
  };

  enum proc_type_e
  {
    PROC_STAT,
    PROC_STACKING_STAT,
    PROC_COOLDOWN,
  };

  unsigned int ptype_idx = 1;
  unsigned int stat_idx = 2;
  unsigned int expr_idx = 3;
  enum proc_expr_e pexprtype = PROC_ENABLED;
  enum proc_type_e ptype = PROC_STAT;
  stat_e stat = STAT_NONE;
  std::vector<slot_e> slots;
  bool legendary_ring = false;

  auto splits = util::string_split<util::string_view>( name_str, "." );

  // Shards of Domination
  if ( splits[ 0 ] == "rune_word" )
  {
    return shadowlands::items::shards_of_domination::create_expression( player, name_str );
  }

  // Hyperthread Wristwraps
  if ( splits[ 0 ] == "hyperthread_wristwraps" )
  {
    if ( auto a = player.find_action( "hyperthread_wristwraps" ) )
    {
      return a->create_expression( name_str );
    }
  }

  if ( splits.size() < 2 )
  {
    return nullptr;
  }

  if ( util::is_number( splits[ 1 ] ) )
  {
    if ( splits[ 1 ] == "1" )
    {
      slots.push_back( SLOT_TRINKET_1 );
    }
    else if ( splits[ 1 ] == "2" )
    {
      slots.push_back( SLOT_TRINKET_2 );
    }
    else
      return nullptr;
    ptype_idx++;

    stat_idx++;
    expr_idx++;
  }
  // Try to find trinket.<trinketname>
  else if ( !splits[ 1 ].empty() )
  {
    auto item = player.find_item_by_name( splits[ 1 ] );
    if ( item && ( item->slot == SLOT_TRINKET_1 || item->slot == SLOT_TRINKET_2 ) )
    {
      slots.push_back( item->slot );
    }
    // If the item is not found, return an always false expression instead of erroring out
    else
    {
      return expr_t::create_constant( "trinket-named-expr", 0 );
    }

    ptype_idx++;

    stat_idx++;
    expr_idx++;
  }
  // No positional parameter given so check both trinkets
  else
  {
    slots.push_back( SLOT_TRINKET_1 );
    slots.push_back( SLOT_TRINKET_2 );
  }

  if ( splits.size() <= ptype_idx )
  {
    throw std::invalid_argument(
      fmt::format( "Cannot create unique gear expression: too few parts '{}' < '{}'.", splits.size(), ptype_idx + 1 ) );
  }

  if ( util::str_compare_ci( splits[ ptype_idx ], "is" ) )
  {
    return std::make_unique<item_is_expr_t>( player, slots, splits[ expr_idx - 1 ] );
  }

  if ( util::str_compare_ci( splits[ ptype_idx ], "ilvl" ) )
  {
    return std::make_unique<item_lvl_expr_t>( player, slots, name_str );
  }

  if ( util::str_prefix_ci( splits[ ptype_idx ], "has_use" ) )
  {
    bool check_buff = false;
    bool check_damage = false;

    if ( util::str_in_str_ci( splits[ ptype_idx ], "buff" ) )
      check_buff = true;
    else if ( util::str_in_str_ci( splits[ ptype_idx ], "damage" ) )
      check_damage = true;

    return std::make_unique<item_has_use_expr_t>( player, slots, name_str, check_buff, check_damage );
  }

  if ( util::str_compare_ci( splits[ ptype_idx ], "cast_time" ) )
  {
    return std::make_unique<item_cast_time_expr_t>( player, slots, name_str );
  }

  if ( util::str_prefix_ci( splits[ ptype_idx ], "has_" ) )
    pexprtype = PROC_EXISTS;
  else if ( util::str_prefix_ci( splits[ ptype_idx ], "ready_" ) )
    pexprtype = PROC_READY;

  if ( util::str_in_str_ci( splits[ ptype_idx ], "cooldown" ) )
  {
    ptype = PROC_COOLDOWN;
    // Cooldowns dont have stat type for now
    expr_idx--;
  }

  if ( util::str_in_str_ci( splits[ ptype_idx ], "stacking_" ) )
    ptype = PROC_STACKING_STAT;

  if ( ptype != PROC_COOLDOWN && !legendary_ring )
  {
    if ( splits.size() <= stat_idx )
    {
      throw std::invalid_argument(
        fmt::format( "Cannot create unique gear expression: too few parts to parse stat: '{}' < '{}'.", splits.size(),
                     stat_idx + 1 ) );
    }
    // Use "all stat" to indicate "any" ..
    if ( util::str_compare_ci( splits[ stat_idx ], "any" ) )
      stat = STAT_ALL;
    else if ( util::str_compare_ci( splits[ stat_idx ], "any_dps" ) )
      stat = STAT_ANY_DPS;
    else
    {
      stat = util::parse_stat_type( splits[ stat_idx ] );
      if ( stat == STAT_NONE )
      {
        throw std::invalid_argument( fmt::format( "Cannot parse stat '{}'.", splits[ stat_idx ] ) );
      }
    }
  }

  if ( pexprtype == PROC_ENABLED && ptype != PROC_COOLDOWN && splits.size() >= 4 )
  {
    if ( splits.size() <= expr_idx )
    {
      throw std::invalid_argument(
        fmt::format( "Cannot create unique gear expression: too few parts to parse buff expression: '{}' < '{}'.",
                     splits.size(), expr_idx + 1 ) );
    }
    return std::make_unique<item_buff_expr_t>( player, slots, stat, ptype == PROC_STACKING_STAT, splits[ expr_idx ] );
  }
  else if ( pexprtype == PROC_ENABLED && ptype == PROC_COOLDOWN && splits.size() >= 3 )
  {
    return std::make_unique<item_cooldown_expr_t>( player, slots, splits[ expr_idx ], name_str );
  }
  else if ( pexprtype == PROC_EXISTS )
  {
    if ( ptype != PROC_COOLDOWN )
    {
      return std::make_unique<item_buff_exists_expr_t>( player, slots, stat, name_str );
    }
    else
    {
      return std::make_unique<item_cooldown_exists_expr_t>( player, slots, name_str );
    }
  }
  else if ( pexprtype == PROC_READY )
  {
    return std::make_unique<item_ready_expr_t>( player, slots, name_str );
  }

  throw std::invalid_argument( fmt::format( "Unsupported unique gear expression '{}'.", splits.back() ) );
}

namespace unique_gear
{
  void proc_resource_t::__initialize()
  {
    may_miss = may_dodge = may_parry = may_block = harmful = false;
    target = player;

    for (size_t i = 1; i <= data().effect_count(); i++)
    {
      const spelleffect_data_t& effect = data().effectN(i);
      if (effect.type() == E_ENERGIZE)
      {
        gain_da = effect.average(item);
        gain_resource = effect.resource_gain_type();
      }
      else if (effect.type() == E_APPLY_AURA && effect.subtype() == A_PERIODIC_ENERGIZE)
      {
        gain_ta = effect.average(item);
        gain_resource = effect.resource_gain_type();
      }
    }

    gain = player->get_gain(name());
  }

std::vector<special_effect_db_item_t> __special_effect_db, __fallback_effect_db;

bool class_scoped_callback_t::valid(const special_effect_t& effect) const
{
  assert(effect.player);

  if (!class_.empty() && range::find(class_, effect.player->type) == class_.end())
  {
    return false;
  }

  if (!spec_.empty() && range::find(spec_, effect.player->specialization()) == spec_.end())
  {
    return false;
  }

  return true;
}

void proc_attack_t::override_data(const special_effect_t& e)
{
  super::override_data(e);

  if ((e.override_result_es_mask & RESULT_DODGE_MASK))
  {
    this->may_dodge = e.result_es_mask & RESULT_DODGE_MASK;
  }

  if ((e.override_result_es_mask & RESULT_PARRY_MASK))
  {
    this->may_parry = e.result_es_mask & RESULT_PARRY_MASK;
  }
}

} // unique_gear

static unique_gear::special_effect_set_t do_find_special_effect_db_item(
    const std::vector<special_effect_db_item_t>& db, unsigned spell_id )
{
  special_effect_set_t entries;

  auto it = range::lower_bound( db, spell_id, {}, &special_effect_db_item_t::spell_id );

  if ( it == db.end() || it -> spell_id != spell_id )
  {
    return { };
  }

  while ( it != db.end() && it -> spell_id == spell_id )
  {
    // If there's an encoded option string, just return it straight up
    if ( ! it -> encoded_options.empty() )
    {
      return { &( *it ) };
    }

    assert( it -> cb_obj );

    // Push all callback-based initializers of the same priority into the vector
    if ( entries.empty() || it -> cb_obj -> priority == entries.front() -> cb_obj -> priority )
    {
      entries.push_back( &( *it ) );
    }
    else
    {
      break;
    }

    it++;
  }

  return entries;
}

static special_effect_set_t find_fallback_effect_db_item( unsigned spell_id )
{ return do_find_special_effect_db_item( __fallback_effect_db, spell_id ); }

special_effect_set_t unique_gear::find_special_effect_db_item( unsigned spell_id )
{ return do_find_special_effect_db_item( __special_effect_db, spell_id ); }

void unique_gear::add_effect( const special_effect_db_item_t& dbitem )
{
  __special_effect_db.push_back( dbitem );
  if ( dbitem.fallback )
    __fallback_effect_db.push_back( dbitem );
}

void unique_gear::register_special_effect( unsigned spell_id, custom_cb_t init_callback, bool fallback )
{
  special_effect_db_item_t dbitem;
  dbitem.spell_id = spell_id;
  dbitem.cb_obj = new wrapper_callback_t( std::move(init_callback) );
  dbitem.fallback = fallback;

  add_effect( dbitem );
}

void unique_gear::register_special_effect( std::initializer_list<unsigned> spell_ids, custom_cb_t init_callback,
                                           bool fallback )
{
  for ( auto id : spell_ids )
    register_special_effect( id, init_callback, fallback );
}

void unique_gear::register_special_effect( unsigned spell_id, const char* encoded_str )
{
  special_effect_db_item_t dbitem;
  dbitem.spell_id = spell_id;
  dbitem.encoded_options = encoded_str;

  __special_effect_db.push_back( dbitem );
}

bool unique_gear::create_fallback_buffs( const special_effect_t& effect, const std::vector<util::string_view>& names )
{
  if ( effect.source != SPECIAL_EFFECT_SOURCE_FALLBACK )
    return false;

  for ( auto name : names )
    buff_t::make_fallback( effect.player, name, effect.player );

  return true;
}

void unique_gear::init_feast( special_effect_t& effect, std::initializer_list<std::pair<stat_e, int>> stat_map )
{
  effect.stat = effect.player->convert_hybrid_stat( STAT_STR_AGI_INT );

  for ( auto&& stat : stat_map )
  {
    if ( stat.first == effect.stat )
    {
      effect.trigger_spell_id = stat.second;
      break;
    }
  }
  effect.stat_amount = effect.player->find_spell( effect.trigger_spell_id )->effectN( 1 ).average( effect.player );
}

void unique_gear::DISABLED_EFFECT( special_effect_t& effect )
{
  effect.type = SPECIAL_EFFECT_NONE;
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
void unique_gear::register_special_effects()
{
  // Register legion special effects
  register_special_effects_legion();

  // Register azerite special effects
  azerite::register_azerite_powers();

  register_special_effects_bfa();

  shadowlands::register_special_effects();
  covenant::soulbinds::register_special_effects();

  dragonflight::register_special_effects();

  thewarwithin::register_special_effects();

  /* Legacy Effects, pre-5.0 */
  register_special_effect( 45481,  "ProcOn/hit_45479Trigger"            ); /* Shattered Sun Pendant of Acumen */
  register_special_effect( 45482,  "ProcOn/hit_45480Trigger"            ); /* Shattered Sun Pendant of Might */
  register_special_effect( 45483,  "ProcOn/hit_45431Trigger"            ); /* Shattered Sun Pendant of Resolve */
  register_special_effect( 45484,  "ProcOn/hit_45478Trigger"            ); /* Shattered Sun Pendant of Restoration */
  register_special_effect( 57345,  item::darkmoon_card_greatness        );
  register_special_effect( 71519,  item::deathbringers_will             );
  register_special_effect( 71562,  item::deathbringers_will             );
  register_special_effect( 71892,  item::heartpierce                    );
  register_special_effect( 71880,  item::heartpierce                    );
  register_special_effect( 72413,  "10%"                                ); /* ICC Melee Ring */
  register_special_effect( 96976,  item::matrix_restabilizer            ); /* Matrix Restabilizer */
  register_special_effect( 107824, "1Tick_108016Trigger_20Dur"          ); /* Kiril, Fury of Beasts */
  register_special_effect( 109862, "1Tick_109860Trigger_20Dur"          ); /* Kiril, Fury of Beasts */
  register_special_effect( 109865, "1Tick_109863Trigger_20Dur"          ); /* Kiril, Fury of Beasts */
  register_special_effect( 107995, item::vial_of_shadows                );
  register_special_effect( 109725, item::vial_of_shadows                );
  register_special_effect( 109722, item::vial_of_shadows                );
  register_special_effect( 108006, item::cunning_of_the_cruel           );
  register_special_effect( 109799, item::cunning_of_the_cruel           );
  register_special_effect( 109801, item::cunning_of_the_cruel           );
  register_special_effect( 243988, item::blazefury_medallion            ); /* Kazzak Neck */

  /* Misc effects */
  register_special_effect( 188534, item::felmouth_frenzy                );

  /* Warlords of Draenor 6.2 */
  register_special_effect( 184270, item::mirror_of_the_blademaster      );
  register_special_effect( 184291, item::soul_capacitor                 );
  register_special_effect( 183942, item::insatiable_hunger              );
  register_special_effect( 184066, item::prophecy_of_fear               );
  register_special_effect( 183951, item::unblinking_gaze_of_sethe       );
  register_special_effect( 184249, item::discordant_chorus              );
  register_special_effect( 184257, item::empty_drinking_horn            );
  register_special_effect( 184767, item::tyrants_decree                 );
  register_special_effect( 184762, item::warlords_unseeing_eye          );
  register_special_effect( 187614, item::legendary_ring                 );
  register_special_effect( 187611, item::legendary_ring                 );
  register_special_effect( 187615, item::legendary_ring                 );
  register_special_effect( 187613, item::legendary_ring                 );
  register_special_effect( 201404, item::gronntooth_war_horn            );
  register_special_effect( 201407, item::infallible_tracking_charm      );
  register_special_effect( 201409, item::orb_of_voidsight               );
  register_special_effect( 429257, item::witherbarks_branch             );

  /* Warlords of Draenor 6.0 */
  register_special_effect( 177085, item::blackiron_micro_crucible       );
  register_special_effect( 177071, item::humming_blackiron_trigger      );
  register_special_effect( 177104, item::battering_talisman_trigger     );
  register_special_effect( 177098, item::forgemasters_insignia          );
  register_special_effect( 177090, item::autorepairing_autoclave        );
  register_special_effect( 177171, item::spellbound_runic_band          );
  register_special_effect( 177163, item::spellbound_solium_band         );

  /* Mists of Pandaria: 5.4 */
  register_special_effect( 146195, item::flurry_of_xuen                 );
  register_special_effect( 146197, item::essence_of_yulon               );

  register_special_effect( 146219, "ProcOn/Hit"                         ); /* Yu'lon's Bite */
  register_special_effect( 146251, "ProcOn/Hit"                         ); /* Thok's Tail Tip (Str proc) */

  register_special_effect( 145955, item::readiness                      );
  register_special_effect( 146019, item::readiness                      );
  register_special_effect( 146025, item::readiness                      );
  register_special_effect( 146051, item::amplification                  );
  register_special_effect( 146136, item::cleave                         );

  register_special_effect( 146183, item::black_blood_of_yshaarj         );
  register_special_effect( 146286, item::skeers_bloodsoaked_talisman    );
  register_special_effect( 146315, item::prismatic_prison_of_pride      );
  register_special_effect( 146047, item::purified_bindings_of_immerseus );
  register_special_effect( 146251, item::thoks_tail_tip                 );

  /* Mists of Pandaria: 5.2 */
  register_special_effect( 139116, item::rune_of_reorigination          );
  register_special_effect( 138957, item::spark_of_zandalar              );
  register_special_effect( 138964, item::unerring_vision_of_leishen     );

  register_special_effect( 138728, "Reverse"                            ); /* Steadfast Talisman of the Shado-Pan Assault */
  register_special_effect( 138701, "ProcOn/Hit"                         ); /* Brutal Talisman of the Shado-Pan Assault */
  register_special_effect( 138700, "ProcOn/Hit"                         ); /* Vicious Talisman of the Shado-Pan Assault */
  register_special_effect( 139171, "ProcOn/Crit_RPPMAttackCrit"         ); /* Gaze of the Twins */
  register_special_effect( 138757, "1Tick_138737Trigger"                ); /* Renataki's Soul Charm */
  register_special_effect( 138790, "ProcOn/Hit_1Tick_138788Trigger"     ); /* Wushoolay's Final Choice */
  register_special_effect( 138758, "1Tick_138760Trigger"                ); /* Fabled Feather of Ji-Kun */
  register_special_effect( 139134, "ProcOn/Crit_RPPMSpellCrit"          ); /* Cha-Ye's Essence of Brilliance */

  register_special_effect( 138865, "ProcOn/Dodge"                       ); /* Delicate Vial of the Sanguinaire */

  /* Mists of Pandaria: 5.0 */
  register_special_effect( 126650, "ProcOn/Hit"                         ); /* Terror in the Mists */
  register_special_effect( 126658, "ProcOn/Hit"                         ); /* Darkmist Vortex */

  /* Mists of Pandaria: Dungeon */
  register_special_effect( 126473, "ProcOn/Hit"                         ); /* Vision of the Predator */
  register_special_effect( 126516, "ProcOn/Hit"                         ); /* Carbonic Carbuncle */
  register_special_effect( 126482, "ProcOn/Hit"                         ); /* Windswept Pages */
  register_special_effect( 126490, "ProcOn/Crit"                        ); /* Searing Words */

  /* Mists of Pandaria: Player versus Player */
  register_special_effect( 126706, "ProcOn/Hit"                         ); /* Gladiator's Insignia of Dominance */

  /* Mists of Pandaria: Darkmoon Faire */
  register_special_effect( 128990, "ProcOn/Hit"                         ); /* Relic of Yu'lon */
  register_special_effect( 128445, "ProcOn/Crit"                        ); /* Relic of Xuen (agi) */

  /* Timewalking */
  register_special_effect( 96963, item::necromantic_focus               ); // Firelands Timewalking Trinket
  register_special_effect( 91003, item::sorrowsong                      ); // Lost City of Tol'vir Timewalking Trinket

  /**
   * Enchants
   */

  /* The Burning Crusade */
  register_special_effect(  28093, "1PPM"                               ); /* Mongoose */

  /* Wrath of the Lich King */
  register_special_effect(  59620, "1PPM"                               ); /* Berserking */
  register_special_effect(  42976, enchants::executioner                );

  /* Cataclysm */
  register_special_effect(  94747, enchants::hurricane_spell            );
  register_special_effect(  74221, "1PPM"                               ); /* Hurricane Weapon */
  register_special_effect(  74245, "1PPM"                               ); /* Landslide */

  /* Mists of Pandaria */
  register_special_effect( 118333, enchants::dancing_steel              );
  register_special_effect( 142531, enchants::dancing_steel              ); /* Bloody Dancing Steel */
  register_special_effect( 120033, enchants::jade_spirit                );
  register_special_effect( 141178, enchants::jade_spirit                );
  register_special_effect( 104561, enchants::windsong                   );
  register_special_effect( 104428, "rppmhaste"                          ); /* Elemental Force */
  register_special_effect( 104441, enchants::rivers_song                );
  register_special_effect( 118314, enchants::colossus                   );

  /* Warlords of Draenor */
  register_special_effect( 159239, enchants::mark_of_the_shattered_hand );
  register_special_effect( 159243, enchants::mark_of_the_thunderlord    );
  register_special_effect( 159682, enchants::mark_of_warsong            );
  register_special_effect( 159683, enchants::mark_of_the_frostwolf      );
  register_special_effect( 159685, enchants::mark_of_blackrock          );
  register_special_effect( 156059, enchants::megawatt_filament          );
  register_special_effect( 156052, enchants::oglethorpes_missile_splitter );
  register_special_effect( 173286, enchants::hemets_heartseeker         );
  register_special_effect( 173321, enchants::mark_of_bleeding_hollow    );

  /* Engineering enchants */
  register_special_effect( 177708, "1PPM_109092Trigger"                 ); /* Mirror Scope */
  register_special_effect( 177707, "1PPM_109085Trigger"                 ); /* Lord Blastingtons Scope of Doom */
  register_special_effect(  95713, "1PPM_95712Trigger"                  ); /* Gnomish XRay */
  register_special_effect(  99622, "1PPM_99621Trigger"                  ); /* Flintlocks Woodchucker */

  /* Profession perks */
  register_special_effect( 105574, profession::zen_alchemist_stone      ); /* Zen Alchemist Stone (stat proc) */
  register_special_effect( 157136, profession::draenor_philosophers_stone ); /* Draenor Philosopher's Stone (stat proc) */
  register_special_effect(  55004, profession::nitro_boosts             );
  register_special_effect(  82626, profession::grounded_plasma_shield   );

  /**
   * Gems
   */

  // TODO: check why 20% PPM and not 100% from spell data?
  register_special_effect(  39958, "0.2PPM"                             ); /* Thundering Skyfire */
  register_special_effect(  55380, "0.2PPM"                             ); /* Thundering Skyflare */
  register_special_effect( 137592, gem::sinister_primal                 ); /* Caster Legendary Gem */
  register_special_effect( 137594, gem::indomitable_primal              ); /* Tank Legendary Gem */
  register_special_effect( 137595, gem::capacitive_primal               ); /* Melee Legendary Gem */
  register_special_effect( 137248, gem::courageous_primal               ); /* Healer Legendary Gem */

  /* Generic special effects begin here */

  /* Racial special effects */
  register_special_effect( 5227,   racial::touch_of_the_grave );
  register_special_effect( 255669, racial::entropic_embrace );
  register_special_effect( 291628, racial::brush_it_off );
  register_special_effect( 292751, racial::zandalari_loa, true );
  register_special_effect( 312923, racial::combat_analysis );

  /* Generic "global scope" special effects */
  register_special_effect( 462854, generic::skyfury );
  register_special_effect( 63604, generic::enable_all_item_effects );
}

void unique_gear::unregister_special_effects()
{
  for ( auto& dbitem: __special_effect_db )
    delete dbitem.cb_obj;
}

action_t* unique_gear::create_action( player_t* player, util::string_view name, util::string_view options )
{
  if ( auto action = shadowlands::create_action( player, name, options ) )
    return action;
  else if ( auto action = thewarwithin::create_action( player, name, options ) )
    return action;

  return nullptr;
}

void unique_gear::register_hotfixes()
{
  register_hotfixes_legion();
  register_hotfixes_bfa();
  shadowlands::register_hotfixes();
  dragonflight::register_hotfixes();
  thewarwithin::register_hotfixes();
}

void unique_gear::register_target_data_initializers( sim_t* sim )
{
  register_target_data_initializers_legion( sim );
  register_target_data_initializers_bfa( sim );
  azerite::register_azerite_target_data_initializers( sim );

  shadowlands::register_target_data_initializers( *sim );
  covenant::soulbinds::register_target_data_initializers( sim );

  dragonflight::register_target_data_initializers( *sim );

  thewarwithin::register_target_data_initializers( *sim );
}

special_effect_t* unique_gear::find_special_effect( player_t* actor, unsigned spell_id, special_effect_e type )
{
  auto it = range::find_if( actor -> special_effects, [ spell_id, type ]( const special_effect_t* e ) {
    return e -> driver() -> id() == spell_id && ( type == SPECIAL_EFFECT_NONE || type == e -> type );
  });

  if ( it != actor -> special_effects.end() )
  {
    return *it;
  }

  for ( const auto& item: actor -> items )
  {
    auto it = range::find_if( item.parsed.special_effects, [ spell_id, type ]( const special_effect_t* e ) {
      return e -> driver() -> id() == spell_id && ( type == SPECIAL_EFFECT_NONE || type == e -> type );
    });

    if ( it != item.parsed.special_effects.end() )
    {
      return *it;
    }
  }

  return nullptr;
}

// Some special effects may use fallback initializers, where the fallback initializer is called if
// the special effect is not found on the actor. Typical cases include anything relating to
// class-specific special effects, where buffs for example should be unconditionally created for the
// actor. This method is called after the normal special effect initialization process finishes on
// the actor.
void unique_gear::initialize_special_effect_fallbacks( player_t* actor )
{
  special_effect_t fallback_effect( actor );

  // Generate an unique list of fallback spell ids
  std::vector<unsigned> fallback_ids;
  range::for_each( __fallback_effect_db, [ &fallback_ids ]( const special_effect_db_item_t& elem ) {
    if ( range::find( fallback_ids, elem.spell_id ) == fallback_ids.end() )
    {
      fallback_ids.push_back( elem.spell_id );
    }
  });

  // Check all fallback ids
  for ( auto fallback_id: fallback_ids )
  {
    // Actor already has a special effect with the fallback id, so don't do anything
    if ( find_special_effect( actor, fallback_id ) )
    {
      continue;
    }

    fallback_effect.reset();
    fallback_effect.spell_id = fallback_id;
    // TODO: Is this really needed?
    fallback_effect.source = SPECIAL_EFFECT_SOURCE_FALLBACK;
    fallback_effect.type = SPECIAL_EFFECT_FALLBACK;

    // Get all registered fallback effects for the spell (fallback) id
    auto dbitems = find_fallback_effect_db_item( fallback_id );
    // .. nothing found, continue
    if ( dbitems.empty() )
    {
      continue;
    }

    // For all registered fallback effects
    for ( const auto& dbitem: dbitems )
    {
      // Ensure that the fallback effect is actually valid for the special effect (actor)
      if ( ! dbitem -> cb_obj -> valid( fallback_effect ) )
      {
        continue;
      }

      fallback_effect.custom_init_object.push_back( dbitem -> cb_obj );
    }

    if ( !fallback_effect.custom_init_object.empty() )
    {
      actor -> special_effects.push_back( new special_effect_t( fallback_effect ) );
    }
  }
}

namespace
{
bool cmp_special_effect( const special_effect_db_item_t& a, const special_effect_db_item_t& b )
{
  if ( &a == &b )
    return false;

  if ( a.spell_id == b.spell_id )
  {
    if ( ! a.encoded_options.empty() && b.encoded_options.empty() )
    {
      return true;
    }
    else if ( a.encoded_options.empty() && ! b.encoded_options.empty() )
    {
      return false;
    }
    else if ( ! a.encoded_options.empty() && ! b.encoded_options.empty() )
    {
      return a.encoded_options < b.encoded_options;
    }

    assert( a.cb_obj && b.cb_obj );
    // Note, descending priority order
    return a.cb_obj -> priority > b.cb_obj -> priority;
  }

  return a.spell_id < b.spell_id;
}

} // unnamed namespace ends

void unique_gear::sort_special_effects()
{
  range::sort( __special_effect_db, cmp_special_effect );
  range::sort( __fallback_effect_db, cmp_special_effect );
}

