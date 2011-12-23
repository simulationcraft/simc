// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "simulationcraft.h"

/*
 * To Do:
 * - Check if curse_of_elements is counted for affliction_effects in every possible situation
 *  (optimal_raid=1, or with multiple wl's, dk's/druis), it enhances drain soul damage.
 * - Bane of Agony and haste scaling looks strange (and is crazy as hell, we'll leave it at average base_td for now)
 * - Seed of Corruption with Soulburn: Trigger Corruptions
 * - Execute felguard:felstorm by player, not the pet
 * - Verify if the current incinerate bonus calculation is correct
 * - Investigate pet mp5 - probably needs to scale with owner int
 * - Dismissing a pet drops aura -> demonic_pact. Either don't let it dropt if there is another pet alive, or recheck application when dropping it.
 */

// ==========================================================================
// Warlock
// ==========================================================================

namespace { // ANONYMOUS NAMESPACE ==========================================

namespace pet_stats { // ====================================================

struct _stat_list_t
{
  int id;
  double stats[ BASE_STAT_MAX ];
};

// Base Stats, same for all pets. Depend on level
const _stat_list_t pet_base_stats[]=
{
  // str, agi,  sta, int, spi,   hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {  453, 883,  353, 159, 225,    0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 81, {  345, 297,  333, 151, 212,    0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {  314, 226,  328, 150, 209,    0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t imp_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5026, 31607,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  5026, 17628,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t felguard_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5395, 19072,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  5395,  9109,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t felhunter_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5395, 19072,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  5395,  9109,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t succubus_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  5640, 19072,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  4530,  9109,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t infernal_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t doomguard_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t ebon_imp_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,     0,     0,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

const _stat_list_t voidwalker_base_stats[]=
{
  // str, agi,  sta, int, spi,     hp,  mana, mcrit/agi, scrit/int, d/agi, mcrit, scrit, mp5, spi_reg
  { 85, {    0,   0,    0,   0,    0,  6131, 19072,         0,         0,     0,     0,     0,   0,       0 } },
  { 80, {    0,   0,    0,   0,    0,  6131,  9109,         0,         0,     0,     0,     0,   0,       0 } },
  { 0, { 0 } }
};

struct _weapon_list_t
{
  int id;
  double min_dmg, max_dmg;
  double swing_time;
};

const _weapon_list_t imp_weapon[]=
{
  { 81, 116.7, 176.7, 2.0 },
  { 0, 0, 0, 0 }
};

const _weapon_list_t felguard_weapon[]=
{
  { 85, 926.3, 926.3, 2.0 },
  { 81, 848.7, 848.7, 2.0 },
  { 80, 824.6, 824.6, 2.0 },
  { 0, 0, 0, 0 }
};

const _weapon_list_t felhunter_weapon[]=
{
  { 85, 926.3, 926.3, 2.0 },
  { 81, 678.4, 1010.4, 2.0 },
  { 80, 824.6, 824.6, 2.0 },
  { 0, 0, 0, 0 }
};

const _weapon_list_t succubus_weapon[]=
{
  { 85, 926.3, 926.3, 2.0 },
  { 81, 848.7, 848.7, 2.0 },
  { 80, 824.6, 824.6, 2.0 },
  { 0, 0, 0, 0 }
};

const _weapon_list_t infernal_weapon[]=
{
  { 85, 1072.0, 1072.0, 2.0 }, //Rough numbers
  { 80, 924.0, 924.0, 2.0 }, //Rough numbers
  { 0, 0, 0, 0 }
};

const _weapon_list_t doomguard_weapon[]=
{
  { 0, 0, 0, 0 }
};

const _weapon_list_t ebon_imp_weapon[]=
{
  { 85, 1110.0, 1110.0, 2.0 }, //Rough numbers
  { 80, 956.0, 956.0, 2.0 }, //Rough numbers
  { 0, 0, 0, 0 }
};

const _weapon_list_t voidwalker_weapon[]=
{
  { 85, 926.3, 926.3, 2.0 },
  { 81, 848.7, 848.7, 2.0 },
  { 80, 824.6, 824.6, 2.0 },
  { 0, 0, 0, 0 }
};

} // namespace pet_stats ====================================================

struct warlock_pet_t;
struct warlock_main_pet_t;
struct warlock_guardian_pet_t;

} // ANONYMOUS namespace ====================================================

struct warlock_targetdata_t : public targetdata_t
{
  dot_t*  dots_corruption;
  dot_t*  dots_unstable_affliction;
  dot_t*  dots_bane_of_agony;
  dot_t*  dots_bane_of_doom;
  dot_t*  dots_immolate;
  dot_t*  dots_drain_life;
  dot_t*  dots_drain_soul;
  dot_t*  dots_shadowflame_dot;
  dot_t*  dots_curse_of_elements;
  dot_t*  dots_burning_embers;

  buff_t* debuffs_haunted;
  buff_t* debuffs_shadow_embrace;

  int affliction_effects()
  {
    int effects = 0;
    if ( dots_curse_of_elements -> ticking    ) effects++;
    if ( dots_bane_of_agony -> ticking        ) effects++;
    if ( dots_bane_of_doom -> ticking         ) effects++;
    if ( dots_corruption -> ticking           ) effects++;
    if ( dots_drain_life -> ticking           ) effects++;
    if ( dots_drain_soul -> ticking           ) effects++;
    if ( dots_unstable_affliction -> ticking  ) effects++;
    if ( debuffs_haunted        -> check()    ) effects++;
    if ( debuffs_shadow_embrace -> check()    ) effects++;
    return effects;
  }

  int active_dots()
  {
    int dots = 0;
    if ( dots_bane_of_agony -> ticking            ) dots++;
    if ( dots_bane_of_doom -> ticking             ) dots++;
    if ( dots_corruption -> ticking               ) dots++;
    if ( dots_drain_life -> ticking               ) dots++;
    if ( dots_drain_soul -> ticking               ) dots++;
    if ( dots_immolate -> ticking                 ) dots++;
    if ( dots_shadowflame_dot -> ticking          ) dots++;
    if ( dots_unstable_affliction -> ticking      ) dots++;
    return dots;
  }

  warlock_targetdata_t(player_t* source, player_t* target);
};

void register_warlock_targetdata(sim_t* sim)
{
  player_type t = WARLOCK;
  typedef warlock_targetdata_t type;

  REGISTER_DOT(corruption);
  REGISTER_DOT(unstable_affliction);
  REGISTER_DOT(bane_of_agony);
  REGISTER_DOT(bane_of_doom);
  REGISTER_DOT(immolate);
  REGISTER_DOT(drain_life);
  REGISTER_DOT(drain_soul);
  REGISTER_DOT(shadowflame_dot);
  REGISTER_DOT(curse_of_elements);
  REGISTER_DOT(burning_embers);

  REGISTER_DEBUFF(haunted);
  REGISTER_DEBUFF(shadow_embrace);
}


struct warlock_t : public player_t
{
  // Active Pet
  warlock_main_pet_t* active_pet;
  pet_t* pet_ebon_imp;
  pet_t* pet_fiery_imp;

  // Buffs
  buff_t* buffs_backdraft;
  buff_t* buffs_decimation;
  buff_t* buffs_demon_armor;
  buff_t* buffs_demonic_empowerment;
  buff_t* buffs_empowered_imp;
  buff_t* buffs_eradication;
  buff_t* buffs_fel_armor;
  buff_t* buffs_metamorphosis;
  buff_t* buffs_molten_core;
  buff_t* buffs_shadow_trance;
  buff_t* buffs_hand_of_guldan;
  buff_t* buffs_improved_soul_fire;
  buff_t* buffs_dark_intent_feedback;
  buff_t* buffs_soulburn;
  buff_t* buffs_demon_soul_imp;
  buff_t* buffs_demon_soul_felguard;
  buff_t* buffs_demon_soul_felhunter;
  buff_t* buffs_demon_soul_succubus;
  buff_t* buffs_demon_soul_voidwalker;
  buff_t* buffs_bane_of_havoc;
  buff_t* buffs_searing_pain_soulburn;
  buff_t* buffs_tier11_4pc_caster;
  buff_t* buffs_tier12_4pc_caster;
  buff_t* buffs_tier13_4pc_caster;

  // Cooldowns
  cooldown_t* cooldowns_metamorphosis;
  cooldown_t* cooldowns_infernal;
  cooldown_t* cooldowns_doomguard;
  cooldown_t* cooldowns_fiery_imp;

  // Talents

  // Affliction
  passive_spell_t* talent_unstable_affliction;
  talent_t* talent_doom_and_gloom; // done
  talent_t* talent_improved_life_tap; // done
  talent_t* talent_improved_corruption; // done
  talent_t* talent_jinx;
  talent_t* talent_soul_siphon; // done
  talent_t* talent_siphon_life; // done
  talent_t* talent_eradication; // done
  talent_t* talent_soul_swap;
  talent_t* talent_shadow_embrace; // done
  talent_t* talent_deaths_embrace; // done
  talent_t* talent_nightfall; // done
  talent_t* talent_soulburn_seed_of_corruption;
  talent_t* talent_everlasting_affliction; // done
  talent_t* talent_pandemic; // done
  talent_t* talent_haunt; // done

  // Demonology
  passive_spell_t* talent_summon_felguard;
  talent_t* talent_demonic_embrace; // done
  talent_t* talent_dark_arts; // done
  talent_t* talent_mana_feed; // done
  talent_t* talent_demonic_aegis;
  talent_t* talent_master_summoner;
  talent_t* talent_impending_doom;
  talent_t* talent_demonic_empowerment;
  talent_t* talent_molten_core; // done
  talent_t* talent_hand_of_guldan;
  talent_t* talent_aura_of_foreboding;
  talent_t* talent_ancient_grimoire;
  talent_t* talent_inferno;
  talent_t* talent_decimation; // done
  talent_t* talent_cremation;
  talent_t* talent_demonic_pact;
  talent_t* talent_metamorphosis; // done

  // Destruction
  passive_spell_t* talent_conflagrate;
  talent_t* talent_bane; // done
  talent_t* talent_shadow_and_flame; // done
  talent_t* talent_improved_immolate;
  talent_t* talent_improved_soul_fire;
  talent_t* talent_emberstorm; // done
  talent_t* talent_improved_searing_pain;
  talent_t* talent_backdraft; // done
  talent_t* talent_shadowburn; // done
  talent_t* talent_burning_embers;
  talent_t* talent_soul_leech; // done
  talent_t* talent_fire_and_brimstone; // done
  talent_t* talent_shadowfury; // done
  talent_t* talent_empowered_imp; // done
  talent_t* talent_bane_of_havoc;
  talent_t* talent_chaos_bolt; // done

  struct passive_spells_t
  {
    passive_spell_t* shadow_mastery;
    passive_spell_t* demonic_knowledge;
    passive_spell_t* cataclysm;
    passive_spell_t* doom_and_gloom;
    passive_spell_t* pandemic;
    passive_spell_t* nethermancy;

  };
  passive_spells_t passive_spells;

  struct mastery_spells_t
  {
    mastery_t* potent_afflictions;
    mastery_t* master_demonologist;
    mastery_t* fiery_apocalypse;
  };
  mastery_spells_t mastery_spells;

  std::string dark_intent_target_str;

  // Gains
  gain_t* gains_fel_armor;
  gain_t* gains_felhunter;
  gain_t* gains_life_tap;
  gain_t* gains_soul_leech;
  gain_t* gains_soul_leech_health;
  gain_t* gains_mana_feed;
  gain_t* gains_tier13_4pc;

  // Uptimes
  benefit_t* uptimes_backdraft[ 4 ];

  // Procs
  proc_t* procs_empowered_imp;
  proc_t* procs_impending_doom;
  proc_t* procs_shadow_trance;
  proc_t* procs_ebon_imp;
  proc_t* procs_fiery_imp;

  // Random Number Generators
  rng_t* rng_soul_leech;
  rng_t* rng_everlasting_affliction;
  rng_t* rng_pandemic;
  rng_t* rng_cremation;
  rng_t* rng_impending_doom;
  rng_t* rng_siphon_life;
  rng_t* rng_ebon_imp;
  rng_t* rng_fiery_imp;

  // Spells
  spell_t* spells_burning_embers;

  spell_data_t* tier12_4pc_caster;

  struct glyphs_t
  {
    glyph_t* metamorphosis;
    // Prime
    glyph_t* life_tap;
    glyph_t* shadow_bolt;

    // Major
    glyph_t* chaos_bolt;
    glyph_t* conflagrate;
    glyph_t* corruption;
    glyph_t* bane_of_agony;
    glyph_t* felguard;
    glyph_t* haunt;
    glyph_t* immolate;
    glyph_t* imp;
    glyph_t* incinerate;
    glyph_t* lash_of_pain;
    glyph_t* shadowburn;
    glyph_t* unstable_affliction;
  };
  glyphs_t glyphs;

  double constants_pandemic_gcd;

  int use_pre_soulburn;

  warlock_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, WARLOCK, name, r )
  {
    if ( race == RACE_NONE ) race = RACE_UNDEAD;

    tree_type[ WARLOCK_AFFLICTION  ] = TREE_AFFLICTION;
    tree_type[ WARLOCK_DEMONOLOGY  ] = TREE_DEMONOLOGY;
    tree_type[ WARLOCK_DESTRUCTION ] = TREE_DESTRUCTION;

    distance = 40;
    default_distance = 40;

    active_pet                  = 0;
    pet_ebon_imp                = 0;
    pet_fiery_imp               = 0;
    spells_burning_embers       = 0;

    cooldowns_metamorphosis                   = get_cooldown ( "metamorphosis" );
    cooldowns_infernal                        = get_cooldown ( "summon_infernal" );
    cooldowns_doomguard                       = get_cooldown ( "summon_doomguard" );
    cooldowns_fiery_imp = get_cooldown( "fiery_imp" );
    cooldowns_fiery_imp -> duration = 45.0;

    use_pre_soulburn = 1;

    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual targetdata_t* new_targetdata(player_t* source, player_t* target) {return new warlock_targetdata_t(source, target);}
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_gains();
  virtual void      init_benefits();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      init_resources( bool force );
  virtual void      reset();
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL, bool save_html=false );
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const { return RESOURCE_MANA; }
  virtual int       primary_role() const     { return ROLE_SPELL; }
  virtual double    composite_armor() const;
  virtual double    composite_spell_power( const school_type school ) const;
  virtual double    composite_spell_power_multiplier() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    composite_player_td_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;

  // Event Tracking
  virtual action_expr_t* create_expression( action_t*, const std::string& name );
};

warlock_targetdata_t::warlock_targetdata_t(player_t* source, player_t* target)
  : targetdata_t(source, target)
{
  warlock_t* p = this->source->cast_warlock();
  debuffs_haunted               = add_aura(new buff_t( this, p -> talent_haunt -> spell_id(), "haunted", p -> talent_haunt -> rank() ));
  debuffs_shadow_embrace        = add_aura(new buff_t( this, p -> talent_shadow_embrace -> effect_trigger_spell( 1 ), "shadow_embrace", p -> talent_shadow_embrace -> rank() ));
}

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Warlock Pet
// ==========================================================================

struct warlock_pet_t : public pet_t
{
  pet_type_t       pet_type;
  double    damage_modifier;
  int stats_avaiable;
  int stats2_avaiable;

  gain_t* gains_mana_feed;
  proc_t* procs_mana_feed;

  double get_attribute_base( int level, int stat_type, pet_type_t pet_type )
  {
    double r                      = 0.0;
    const pet_stats::_stat_list_t* base_list = 0;
    const pet_stats::_stat_list_t*  pet_list = 0;


    base_list = pet_stats::pet_base_stats;

    if      ( pet_type == PET_IMP        ) pet_list = pet_stats::imp_base_stats;
    else if ( pet_type == PET_FELGUARD   ) pet_list = pet_stats::felguard_base_stats;
    else if ( pet_type == PET_FELHUNTER  ) pet_list = pet_stats::felhunter_base_stats;
    else if ( pet_type == PET_SUCCUBUS   ) pet_list = pet_stats::succubus_base_stats;
    else if ( pet_type == PET_INFERNAL   ) pet_list = pet_stats::infernal_base_stats;
    else if ( pet_type == PET_DOOMGUARD  ) pet_list = pet_stats::doomguard_base_stats;
    else if ( pet_type == PET_EBON_IMP   ) pet_list = pet_stats::ebon_imp_base_stats;
    else if ( pet_type == PET_VOIDWALKER ) pet_list = pet_stats::voidwalker_base_stats;

    if ( stat_type < 0 || stat_type >= BASE_STAT_MAX )
    {
      return 0.0;
    }

    if ( base_list )
    {
      for ( int i = 0; base_list[ i ].id != 0 ; i++ )
      {
        if ( level == base_list[ i ].id )
        {
          r += base_list[ i ].stats[ stat_type ];
          stats_avaiable++;
          break;
        }
        if ( level > base_list[ i ].id )
        {
          r += base_list[ i ].stats[ stat_type ];
          break;
        }
      }
    }

    if ( pet_list )
    {
      for ( int i = 0; pet_list[ i ].id != 0 ; i++ )
      {
        if ( level == pet_list[ i ].id )
        {
          r += pet_list[ i ].stats[ stat_type ];
          stats2_avaiable++;
          break;
        }
        if ( level > pet_list[ i ].id )
        {
          r += pet_list[ i ].stats[ stat_type ];
          break;
        }
      }
    }

    return r;
  }

  double get_weapon( int level, pet_type_t pet_type, int n )
  {
    double r = 0.0;
    const pet_stats::_weapon_list_t*  weapon_list = 0;

    if      ( pet_type == PET_IMP          ) weapon_list = pet_stats::imp_weapon;
    else if ( pet_type == PET_FELGUARD     ) weapon_list = pet_stats::felguard_weapon;
    else if ( pet_type == PET_FELHUNTER    ) weapon_list = pet_stats::felhunter_weapon;
    else if ( pet_type == PET_SUCCUBUS     ) weapon_list = pet_stats::succubus_weapon;
    else if ( pet_type == PET_INFERNAL     ) weapon_list = pet_stats::infernal_weapon;
    else if ( pet_type == PET_DOOMGUARD    ) weapon_list = pet_stats::doomguard_weapon;
    else if ( pet_type == PET_EBON_IMP     ) weapon_list = pet_stats::ebon_imp_weapon;
    else if ( pet_type == PET_VOIDWALKER   ) weapon_list = pet_stats::voidwalker_weapon;

    if ( weapon_list )
    {
      if ( n == 3 )
      {
        for ( int i = 0; weapon_list[ i ].id != 0 ; i++ )
        {
          if ( level == weapon_list[ i ].id )
          {
            r += weapon_list[ i ].swing_time;
            break;
          }
          if ( level > weapon_list[ i ].id )
          {
            r += weapon_list[ i ].swing_time;
            break;
          }
        }
      }
      else if ( n == 1 )
      {
        for ( int i = 0; weapon_list[ i ].id != 0 ; i++ )
        {
          if ( level == weapon_list[ i ].id )
          {
            r += weapon_list[ i ].min_dmg;
            break;
          }
          if ( level > weapon_list[ i ].id )
          {
            r += weapon_list[ i ].min_dmg;
            break;
          }
        }
      }
      else if ( n == 2 )
      {
        for ( int i = 0; weapon_list[ i ].id != 0 ; i++ )
        {
          if ( level == weapon_list[ i ].id )
          {
            r += weapon_list[ i ].max_dmg;
            break;
          }
          if ( level > weapon_list[ i ].id )
          {
            r += weapon_list[ i ].max_dmg;
            break;
          }
        }
      }
    }
    if ( n == 3 && r == 0 )r = 1.0; // set swing-time to 1.00 if there is no weapon

    return r;
  }

  warlock_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, pet_type_t pt, bool guardian = false ) :
    pet_t( sim, owner, pet_name, guardian ), pet_type( pt ), damage_modifier( 1.0 )
  {
    gains_mana_feed = get_gain( "mana_feed" );
    procs_mana_feed = get_proc( "mana_feed" );
    stats_avaiable = 0;
    stats2_avaiable = 0;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = get_weapon( level, pet_type, 1 );
    main_hand_weapon.max_dmg    = get_weapon( level, pet_type, 2 );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = get_weapon( level, pet_type, 3 );
    if ( main_hand_weapon.swing_time == 0 )
    {
      sim -> errorf( "Pet %s has swingtime == 0.\n", name() );
      assert( 0 );
    }
  }

  virtual bool ooc_buffs() { return true; }

  virtual void init_base()
  {
    pet_t::init_base();

    attribute_base[ ATTR_STRENGTH  ]  = get_attribute_base( level, BASE_STAT_STRENGTH, pet_type );
    attribute_base[ ATTR_AGILITY   ]  = get_attribute_base( level, BASE_STAT_AGILITY, pet_type );
    attribute_base[ ATTR_STAMINA   ]  = get_attribute_base( level, BASE_STAT_STAMINA, pet_type );
    attribute_base[ ATTR_INTELLECT ]  = get_attribute_base( level, BASE_STAT_INTELLECT, pet_type );
    attribute_base[ ATTR_SPIRIT    ]  = get_attribute_base( level, BASE_STAT_SPIRIT, pet_type );
    resource_base[ RESOURCE_HEALTH ]  = get_attribute_base( level, BASE_STAT_HEALTH, pet_type );
    resource_base[ RESOURCE_MANA   ]  = get_attribute_base( level, BASE_STAT_MANA, pet_type );
    initial_attack_crit_per_agility   = get_attribute_base( level, BASE_STAT_MELEE_CRIT_PER_AGI, pet_type );
    initial_spell_crit_per_intellect  = get_attribute_base( level, BASE_STAT_SPELL_CRIT_PER_INT, pet_type );
    initial_dodge_per_agility         = get_attribute_base( level, BASE_STAT_DODGE_PER_AGI, pet_type );
    base_spell_crit                   = get_attribute_base( level, BASE_STAT_SPELL_CRIT, pet_type );
    base_attack_crit                  = get_attribute_base( level, BASE_STAT_MELEE_CRIT, pet_type );
    base_mp5                          = get_attribute_base( level, BASE_STAT_MP5, pet_type );

    if ( stats_avaiable != 13 )
      sim -> errorf( "Pet %s has no general base stats avaiable on level=%.i.\n", name(), level );
    if ( stats2_avaiable != 13 )
      sim -> errorf( "Pet %s has no base stats avaiable on level=%.i.\n", name(), level );

    initial_attack_power_per_strength = 2.0; // tested in-game as of 2010/12/20
    base_attack_power = -20; // technically, the first 20 str give 0 ap. - tested
    stamina_per_owner = 0.6496; // level invariant, tested
    intellect_per_owner = 0; // removed in cata, tested

    base_attack_crit                  += 0.0328; // seems to be level invariant, untested
    base_spell_crit                   += 0.0328; // seems to be level invariant, untested
    initial_attack_crit_per_agility   += 0.01 / 52.0; // untested
    initial_spell_crit_per_intellect  += owner -> initial_spell_crit_per_intellect; // untested
    health_per_stamina = 10.0; // untested!
    mana_per_intellect = 0; // tested - does not scale with pet int, but with owner int, at level/80 * 7.5 mana per point of owner int that exceeds owner base int
    mp5_per_intellect  = 2.0 / 3.0; // untested!
  }

  virtual void init_resources( bool force )
  {
    bool mana_force = ( force || resource_initial[ RESOURCE_MANA ] == 0 );
    player_t::init_resources( force );
    if ( mana_force ) resource_initial[ RESOURCE_MANA ] += ( owner -> intellect() - owner -> attribute_base[ ATTR_INTELLECT ] ) * ( level / 80.0 ) * 7.5;
    resource_current[ RESOURCE_MANA ] = resource_max[ RESOURCE_MANA ] = resource_initial[ RESOURCE_MANA ];
  }

  virtual void schedule_ready( double delta_time=0,
                               bool   waiting=false )
  {
    if ( main_hand_attack && ! main_hand_attack -> execute_event )
    {
      main_hand_attack -> schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  virtual void summon( double duration=0 )
  {
    warlock_t*  o = owner -> cast_warlock();
    pet_t::summon( duration );
    if ( o -> talent_demonic_pact -> rank() )
      sim -> auras.demonic_pact -> trigger();
  }

  virtual void dismiss()
  {
    pet_t::dismiss();
    /* Commenting this out for now - we never dismiss the real pet during combat
    anyway, and we don't want to accidentally turn off DP when guardians are dismissed
    warlock_t*  o = owner -> cast_warlock();
    if ( o -> talent_demonic_pact -> rank() )
    sim -> auras.demonic_pact -> expire();
      */
  }

  virtual double composite_spell_haste() const
  {
    double h = player_t::composite_spell_haste();
    h *= owner -> spell_haste;

    // According to Issue 881, DI on the owner doesn't increase pet haste, only when cast directly on the pet. 28/09/11
    if ( owner -> buffs.dark_intent -> check() )
      h *= 1.03;

    return h;
  }

  virtual double composite_attack_haste() const
  {
    double h = player_t::composite_attack_haste();
    h *= owner -> spell_haste;
    return h;
  }

  virtual double composite_spell_power( const school_type school ) const
  {
    double sp = pet_t::composite_spell_power( school );
    sp += owner -> composite_spell_power( school ) * ( level / 80.0 ) * 0.5 * owner -> composite_spell_power_multiplier();
    return sp;
  }

  virtual double composite_attack_power() const
  {
    double ap = pet_t::composite_attack_power();
    ap += owner -> composite_spell_power( SCHOOL_MAX ) * ( level / 80.0 ) * owner -> composite_spell_power_multiplier();
    return ap;
  }

  virtual double composite_attack_crit() const
  {
   double ac = owner -> composite_spell_crit(); // Seems to just use our crit directly, based on very rough numbers, needs more testing.

    // According to Issue 881, FM on the owner doesn't increase pet crit, only when cast directly on the pet. 28/09/11
    if ( owner -> buffs.focus_magic -> check() )
      ac -= 0.03;

    return ac;
  }

  virtual double composite_spell_crit() const
  {
    double sc = owner -> composite_spell_crit(); // Seems to just use our crit directly, based on very rough numbers, needs more testing.

    // According to Issue 881, FM on the owner doesn't increase pet crit, only when cast directly on the pet. 28/09/11
    if ( owner -> buffs.focus_magic -> check() )
      sc -= 0.03;

    return sc;
  }
};

// ==========================================================================
// Warlock Main Pet
// ==========================================================================

struct warlock_main_pet_t : public warlock_pet_t
{
  warlock_main_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, pet_type_t pt ) :
    warlock_pet_t( sim, owner, pet_name, pt )
  {}

  virtual void summon( double duration=0 )
  {
    warlock_t* o = owner -> cast_warlock();
    o -> active_pet = this;
    warlock_pet_t::summon( duration );
  }

  virtual void dismiss()
  {
    warlock_t* o = owner -> cast_warlock();
    warlock_pet_t::dismiss();
    o -> active_pet = 0;
  }

  virtual double composite_attack_expertise() const
  {
    return owner -> spell_hit * 26.0 / 17.0;
  }

  virtual int primary_resource() const { return RESOURCE_MANA; }

  virtual double composite_player_multiplier( const school_type school, action_t* a ) const
  {
    double m = warlock_pet_t::composite_player_multiplier( school, a );

    warlock_t* o = owner -> cast_warlock();

    double mastery_value = o -> mastery_spells.master_demonologist -> effect_base_value( 3 );

    m *= 1.0 + ( o -> mastery_spells.master_demonologist -> ok() * o -> composite_mastery() * mastery_value / 10000.0 );

    return m;
  }

  virtual double composite_mp5() const
  {
    double h = warlock_pet_t::composite_mp5();
    h += mp5_per_intellect * owner -> intellect();
    return h;
  }
};

// ==========================================================================
// Warlock Guardian Pet
// ==========================================================================

struct warlock_guardian_pet_t : public warlock_pet_t
{
  double snapshot_crit, snapshot_haste, snapshot_sp, snapshot_mastery;

  warlock_guardian_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, pet_type_t pt ) :
    warlock_pet_t( sim, owner, pet_name, pt, true ),
    snapshot_crit( 0 ), snapshot_haste( 0 ), snapshot_sp( 0 ), snapshot_mastery( 0 )
  {}

  virtual void summon( double duration=0 )
  {
    reset();
    warlock_pet_t::summon( duration );
    // Guardians use snapshots
    snapshot_crit = owner -> composite_spell_crit();
    snapshot_haste = owner -> composite_spell_haste();
    snapshot_sp = floor( owner -> composite_spell_power( SCHOOL_MAX ) * owner -> composite_spell_power_multiplier() );
    snapshot_mastery = owner -> composite_mastery();
  }

  virtual double composite_attack_crit() const
  {
    return snapshot_crit;
  }

  virtual double composite_attack_expertise() const
  {
    return 0;
  }

  virtual double composite_attack_haste() const
  {
    return snapshot_haste * player_t::composite_attack_haste();
  }

  virtual double composite_attack_hit() const
  {
    return 0;
  }

  virtual double composite_attack_power() const
  {
    double ap = pet_t::composite_attack_power();
    ap += snapshot_sp * ( level / 80.0 ) * owner -> composite_spell_power_multiplier();
    return ap;
  }

  virtual double composite_spell_crit() const
  {
    return snapshot_crit;
  }

  virtual double composite_spell_haste() const
  {
    // FIXME: Needs testing, but Doomguard seems to not scale with our haste after 4.1 or so
    return 1.0;
  }

  virtual double composite_spell_power( const school_type school ) const
  {
    double sp = pet_t::composite_spell_power( school );
    sp += snapshot_sp * ( level / 80.0 ) * 0.5;
    return sp;
  }

  virtual double composite_spell_power_multiplier() const
  {
    double m = warlock_pet_t::composite_spell_power_multiplier();
    warlock_t* o = owner -> cast_warlock();

    // Guardians normally don't gain demonic pact, but when they provide it they also provide it to themselves
    if ( o -> talent_demonic_pact -> rank() ) m *= 1.10;

    return m;
  }

};

// ==========================================================================
// Warlock Heal
// ==========================================================================

struct warlock_heal_t : public heal_t
{
  warlock_heal_t( const char* n, warlock_t* p, const uint32_t id ) :
    heal_t( n, p, id )
  { }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();

    heal_t::player_buff();

    player_multiplier *= 1.0 + p -> buffs_demon_armor -> value();
  }
};

// ==========================================================================
// Warlock Spell
// ==========================================================================

struct warlock_spell_t : public spell_t
{
private:
  void _init_warlock_spell_t()
  {
    may_crit      = true;
    tick_may_crit = true;
    dot_behavior  = DOT_REFRESH;
    weapon_multiplier = 0.0;
    crit_multiplier *= 1.33;
  }

public:
  warlock_spell_t( const char* n, warlock_t* p, const school_type s, int t ) :
    spell_t( n, p, RESOURCE_MANA, s, t )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const active_spell_t& s, int t = TREE_NONE ) :
    spell_t( s, t )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const char* n, warlock_t* p, const char* sname, int t = TREE_NONE ) :
    spell_t( n, sname, p, t )
  {
    _init_warlock_spell_t();
  }

  warlock_spell_t( const char* n, warlock_t* p, const uint32_t id, int t = TREE_NONE ) :
    spell_t( n, id, p, t )
  {
    _init_warlock_spell_t();
  }

  // warlock_spell_t::haste =================================================

  virtual double haste() const
  {
    warlock_t* p = player -> cast_warlock();
    double h = spell_t::haste();

    if ( p -> buffs_eradication -> up() )
    {
      h *= 1.0 / ( 1.0 + p -> buffs_eradication -> effect1().percent() );
    }

    if ( p -> buffs_demon_soul_felguard -> up() )
    {
      h *= 1.0 / ( 1.0 + p -> buffs_demon_soul_felguard -> effect1().percent() );
    }

    return h;
  }

  // warlock_spell_t::player_buff ===========================================

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();

    spell_t::player_buff();

    if ( base_execute_time > 0 && s_tree == WARLOCK_DESTRUCTION && p -> buffs_demon_soul_imp -> up() )
    {
      player_crit += p -> buffs_demon_soul_imp -> effect1().percent();
    }

    if ( school == SCHOOL_SHADOW || school == SCHOOL_SHADOWFLAME )
      player_multiplier *= 1.0 + trigger_deaths_embrace( this );
  }

  // warlock_spell_t::target_debuff =========================================

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    warlock_t* p = player -> cast_warlock();

    spell_t::target_debuff( t, dmg_type );

    if ( p -> buffs_bane_of_havoc -> up() )
      target_multiplier *= 1.0 + p -> buffs_bane_of_havoc -> effect1().percent();
  }

  // warlock_spell_t::tick ==================================================

  virtual void tick( dot_t* d )
  {
    spell_t::tick( d );

    if ( tick_dmg > 0 )
    {
      trigger_fiery_imp( this );
    }
  }

  // warlock_spell_t::total_td_multiplier ===================================

  virtual double total_td_multiplier() const
  {
    double shadow_td_multiplier = 1.0;
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    if ( school == SCHOOL_SHADOW || school == SCHOOL_SHADOWFLAME )
    {
      //FIXME: These should be modeled as debuffs, and we need to check if they affect trinkets (if there is ever a shadow td trinket)
      if ( td -> debuffs_shadow_embrace -> up() )
      {
        shadow_td_multiplier *= 1.0 + td -> debuffs_shadow_embrace -> check() * td -> debuffs_shadow_embrace -> effect1().percent();
      }
      if ( td -> debuffs_haunted -> up() )
      {
        shadow_td_multiplier *= 1.0 + td -> debuffs_haunted -> effect3().percent() + ( p -> glyphs.haunt -> effect1().percent() );
      }
    }

    return spell_t::total_td_multiplier() * shadow_td_multiplier;
  }

  // trigger_impending_doom =================================================

  static void trigger_impending_doom ( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();

    if ( p -> rng_impending_doom -> roll ( p -> talent_impending_doom -> proc_chance() ) )
    {
      p -> procs_impending_doom -> occur();
      if ( p -> cooldowns_metamorphosis -> remains() > p -> talent_impending_doom -> effect2().base_value() )
        p -> cooldowns_metamorphosis -> ready -= p -> talent_impending_doom -> effect2().base_value();
      else
        p -> cooldowns_metamorphosis -> reset();
    }
  }

  // trigger_soul_leech =====================================================

  static void trigger_soul_leech( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();

    if ( p -> talent_soul_leech -> rank() )
    {
      p -> resource_gain( RESOURCE_HEALTH, p -> resource_max[ RESOURCE_HEALTH ] * p -> talent_soul_leech -> effect1().percent(), p -> gains_soul_leech_health );
      p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * p -> talent_soul_leech -> effect1().percent(), p -> gains_soul_leech );

      p -> trigger_replenishment();
    }
  }

  // trigger_decimation =====================================================

  static void trigger_decimation( warlock_spell_t* s, int result )
  {
    warlock_t* p = s -> player -> cast_warlock();
    if ( ( result != RESULT_HIT ) && ( result != RESULT_CRIT ) ) return;
    if ( s -> target -> health_percentage() > p -> talent_decimation -> effect2().base_value() ) return;
    p -> buffs_decimation -> trigger();
  }

  // trigger_deaths_embrace =================================================

  static double trigger_deaths_embrace( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();

    if ( ! p -> talent_deaths_embrace -> rank() ) return 0;


    // The target health percentage is ONLY contained in the Rank-1 version of the talent.
    if ( s -> target -> health_percentage() <= p -> talent_deaths_embrace -> spell( 1 ).effect3().base_value() )
    {
      return p -> talent_deaths_embrace -> effect2().percent();
    }

    return 0;
  }

  // trigger_everlasting_affliction =========================================

  static void trigger_everlasting_affliction( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();
    warlock_targetdata_t* td = s -> targetdata() -> cast_warlock();

    if ( ! p -> talent_everlasting_affliction -> rank() ) return;

    if ( ! td -> dots_corruption -> ticking ) return;

    if ( p -> rng_everlasting_affliction -> roll( p -> talent_everlasting_affliction -> proc_chance() ) )
    {
      td -> dots_corruption -> refresh_duration();
    }
  }

  // trigger_fiery_imp ======================================================

  static void trigger_fiery_imp( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();

    if ( p -> set_bonus.tier12_2pc_caster() && ( p -> cooldowns_fiery_imp -> remains() == 0 ) )
    {
      if ( p -> rng_fiery_imp -> roll( p -> sets -> set( SET_T12_2PC_CASTER ) -> proc_chance() ) )
      {
        p -> procs_fiery_imp -> occur();
        p -> pet_fiery_imp -> dismiss();
        p -> pet_fiery_imp -> summon( p -> dbc.spell( 99221 ) -> duration() - 0.01 );
        p -> cooldowns_fiery_imp -> start();
      }
    }
  }

  // trigger_tier12_4pc_caster ==============================================

  static void trigger_tier12_4pc_caster( spell_t* s )
  {
    warlock_t* p = s -> player -> cast_warlock();

    if ( ! p -> set_bonus.tier12_4pc_caster() ) return;

    p -> buffs_tier12_4pc_caster -> trigger( 1, p -> tier12_4pc_caster -> effect1().percent() );
  }
};

// ==========================================================================
// Warlock Pet Melee
// ==========================================================================

struct warlock_pet_melee_t : public attack_t
{
  warlock_pet_melee_t( warlock_pet_t* p, const char* name ) :
    attack_t( name, p, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
  {
    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    may_crit    = true;
    background  = true;
    repeating   = true;

    base_multiplier *= p -> damage_modifier;
  }
};

// ==========================================================================
// Warlock Pet Attack
// ==========================================================================

struct warlock_pet_attack_t : public attack_t
{
  warlock_pet_attack_t( const char* n, warlock_pet_t* p, int r=RESOURCE_MANA, const school_type s=SCHOOL_PHYSICAL ) :
    attack_t( n, p, r, s, TREE_NONE, true )
  {
    weapon = &( p -> main_hand_weapon );
    may_crit   = true;
    special = true;
  }

  warlock_pet_attack_t( const char* n, player_t* player, const char* sname, int t = TREE_NONE ) :
    attack_t( n, sname, player, t, true )
  {
    may_crit   = true;
    special = true;
  }

  warlock_pet_attack_t( const char* n, const uint32_t id, player_t* player, int t = TREE_NONE ) :
    attack_t( n, id, player, t, true )
  {
    may_crit   = true;
    special = true;
  }

  virtual void player_buff()
  {
    warlock_pet_t* p = ( warlock_pet_t* ) player -> cast_pet();
    warlock_t* o = p -> owner -> cast_warlock();
    attack_t::player_buff();

    if ( o -> buffs_hand_of_guldan -> up() )
    {
      player_crit += 0.10;
    }
  }
};

// trigger_mana_feed ========================================================

void trigger_mana_feed( action_t* s, double impact_result )
{
  warlock_pet_t* a = ( warlock_pet_t* ) s -> player -> cast_pet();
  warlock_t* p = a -> owner -> cast_warlock();

  if ( p -> talent_mana_feed -> rank() )
  {
    if ( impact_result == RESULT_CRIT )
    {
      double mana = p -> resource_max[ RESOURCE_MANA ] * p -> talent_mana_feed -> effect3().percent();
      if ( p -> active_pet -> pet_type == PET_FELGUARD || p -> active_pet -> pet_type == PET_FELHUNTER ) mana *= 4;
      p -> resource_gain( RESOURCE_MANA, mana, p -> gains_mana_feed );
      a -> procs_mana_feed -> occur();
    }
  }
}

// ==========================================================================
// Warlock Pet Spell
// ==========================================================================

struct warlock_pet_spell_t : public spell_t
{

  warlock_pet_spell_t( const char* n, warlock_pet_t* p, int r=RESOURCE_MANA, const school_type s=SCHOOL_SHADOW ) :
    spell_t( n, p, r, s )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  warlock_pet_spell_t( const active_spell_t& s, int t = TREE_NONE ) :
    spell_t( s, t )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  warlock_pet_spell_t( const char* n, warlock_pet_t* p, const char* sname, int t = TREE_NONE ) :
    spell_t( n, sname, p, t )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  warlock_pet_spell_t( const char* n, const uint32_t id, warlock_pet_t* p, int t = TREE_NONE ) :
    spell_t( n, id, p, t )
  {
    may_crit = true;
    crit_multiplier *= 1.33;
  }

  virtual void player_buff()
  {
    warlock_pet_t* p = ( warlock_pet_t* ) player -> cast_pet();
    warlock_t* o = p -> owner -> cast_warlock();
    spell_t::player_buff();

    if ( o -> buffs_hand_of_guldan -> up() )
    {
      player_crit += 0.10;
    }
  }
};

// ==========================================================================
// Pet Imp
// ==========================================================================

struct imp_pet_t : public warlock_main_pet_t
{
  struct firebolt_t : public warlock_pet_spell_t
  {
    firebolt_t( imp_pet_t* p ):
      warlock_pet_spell_t( "firebolt", p, "Firebolt" )
    {
      warlock_t*  o = p -> owner -> cast_warlock();

      direct_power_mod = 0.618; // tested in-game as of 2011/05/10
      base_execute_time += o -> talent_dark_arts -> effect1().seconds();
      if ( o -> bugs ) min_gcd = 1.5;
    }

    virtual void player_buff()
    {
      warlock_t* o = player -> cast_pet() -> owner -> cast_warlock();
      warlock_pet_spell_t::player_buff();

      if ( o -> race == RACE_ORC )
      {
        // Glyph is additive with orc racial
        player_multiplier /= 1.05;
        player_multiplier *= 1.05 + o -> glyphs.imp -> base_value();
      }
      else
      {
        player_multiplier *= 1.0 + o -> glyphs.imp -> base_value();
      }
    }

    virtual void impact( player_t* t, int impact_result, double travel_dmg );
  };

  imp_pet_t( sim_t* sim, player_t* owner ) :
    warlock_main_pet_t( sim, owner, "imp", PET_IMP )
  {
    action_list_str += "/snapshot_stats";
    action_list_str += "/firebolt";
  }

  virtual void init_base()
  {
    warlock_main_pet_t::init_base();

    // untested !!
    mana_per_intellect = 14.28;
    mp5_per_intellect  = 5.0 / 6.0;
    // untested !!
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "firebolt" ) return new firebolt_t( this );

    return warlock_main_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Pet Felguard
// ==========================================================================

struct felguard_pet_t : public warlock_main_pet_t
{
  struct legion_strike_t : public warlock_pet_attack_t
  {
    legion_strike_t( felguard_pet_t* p ) :
      warlock_pet_attack_t( "legion_strike", p, "Legion Strike" )
    {
      warlock_t*      o = p -> owner -> cast_warlock();
      aoe               = -1;
      direct_power_mod  = 0.264;
      weapon   = &( p -> main_hand_weapon );
      base_multiplier *= 1.0 + o -> talent_dark_arts -> effect2().percent();
    }

    virtual void execute()
    {
      warlock_pet_attack_t::execute();

      trigger_mana_feed ( this, result );
    }

    virtual void player_buff()
    {
      warlock_t* o = player -> cast_pet() -> owner -> cast_warlock();
      warlock_pet_attack_t::player_buff();

      if ( o -> race == RACE_ORC )
      {
        // Glyph is additive with orc racial
        player_multiplier /= 1.05;
        player_multiplier *= 1.05 + o -> glyphs.felguard -> base_value();
      }
      else
      {
        player_multiplier *= 1.0 + o -> glyphs.felguard -> base_value();
      }
    }
  };

  struct felstorm_tick_t : public warlock_pet_attack_t
  {
    felstorm_tick_t( felguard_pet_t* p ) :
      warlock_pet_attack_t( "felstorm_tick", 89753, p )
    {
      direct_power_mod = 0.231; // hardcoded from the tooltip
      dual        = true;
      background  = true;
      aoe         = -1;
      direct_tick = true;
      stats       = p -> get_stats( "felstorm", this );
      resource    = RESOURCE_MANA;
    }
  };

  struct felstorm_t : public warlock_pet_attack_t
  {
    felstorm_tick_t* felstorm_tick;

    felstorm_t( felguard_pet_t* p ) :
      warlock_pet_attack_t( "felstorm", 89751, p ), felstorm_tick( 0 )
    {
      aoe       = -1;
      harmful   = false;
      tick_zero = true;

      felstorm_tick = new felstorm_tick_t( p );
      felstorm_tick -> weapon = &( p -> main_hand_weapon );
    }

    virtual void tick( dot_t* d )
    {
      felstorm_tick -> execute();

      stats -> add_tick( d -> time_to_tick );
    }
  };

  struct melee_t : public warlock_pet_melee_t
  {
    melee_t( felguard_pet_t* p ) :
      warlock_pet_melee_t( p, "melee" )
    { }
  };

  felguard_pet_t( sim_t* sim, player_t* owner ) :
    warlock_main_pet_t( sim, owner, "felguard", PET_FELGUARD )
  {
    damage_modifier = 1.0;

    action_list_str += "/snapshot_stats";
    action_list_str += "/felstorm";
    action_list_str += "/legion_strike";
    action_list_str += "/wait_until_ready";
  }

  virtual void init_base()
  {
    warlock_main_pet_t::init_base();

    main_hand_attack = new melee_t( this );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "legion_strike"   ) return new legion_strike_t( this );
    if ( name == "felstorm"        ) return new      felstorm_t( this );

    return warlock_main_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Pet Felhunter
// ==========================================================================

struct felhunter_pet_t : public warlock_main_pet_t
{
  // TODO: Need to add fel intelligence on the warlock while felhunter is out
  struct shadow_bite_t : public warlock_pet_spell_t
  {
    shadow_bite_t( felhunter_pet_t* p ) :
      warlock_pet_spell_t( "shadow_bite", p, "Shadow Bite" )
    {
      warlock_t*       o = p -> owner -> cast_warlock();

      base_multiplier *= 1.0 + o -> talent_dark_arts -> effect3().percent();
      direct_power_mod = 0.614; // tested in-game as of 2010/12/20
      base_dd_min *= 2.5; // only tested at level 85, applying base damage adjustment as a percentage
      base_dd_max *= 2.5; // modifier in hopes of getting it "somewhat right" for other levels as well

      // FIXME - Naive assumption, needs testing on the 4.1 PTR!
      base_dd_min *= 2;
      base_dd_max *= 2;
      direct_power_mod *= 2;
    }

    virtual void player_buff()
    {
      warlock_pet_spell_t::player_buff();
      warlock_t*  o = player -> cast_pet() -> owner -> cast_warlock();
      warlock_targetdata_t* td = targetdata_t::get(o, target) -> cast_warlock();

      player_multiplier *= 1.0 + td -> active_dots() * effect3().percent();
    }

    virtual void impact( player_t* t, int impact_result, double travel_dmg )
    {
      warlock_pet_spell_t::impact( t, impact_result, travel_dmg );
      if ( result_is_hit( impact_result ) )
        trigger_mana_feed ( this, impact_result );
    }
  };

  felhunter_pet_t( sim_t* sim, player_t* owner ) :
    warlock_main_pet_t( sim, owner, "felhunter", PET_FELHUNTER )
  {
    damage_modifier = 0.8;

    action_list_str += "/snapshot_stats";
    action_list_str += "/shadow_bite";
    action_list_str += "/wait_for_shadow_bite";
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    // untested in cataclyms!!
    health_per_stamina = 9.5;
    mana_per_intellect = 11.55;
    mp5_per_intellect  = 8.0 / 324.0;
    base_mp5 = 11.22;
    // untested in cataclyms!!

    main_hand_attack = new warlock_pet_melee_t( this, "felhunter_melee" );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "shadow_bite" ) return new shadow_bite_t( this );
    if ( name == "wait_for_shadow_bite" ) return new wait_for_cooldown_t( this, "shadow_bite" );

    return warlock_main_pet_t::create_action( name, options_str );
  }

  virtual void summon( double duration=0 )
  {
    sim -> auras.fel_intelligence -> trigger();

    warlock_main_pet_t::summon( duration );
  }

  virtual void dismiss()
  {
    warlock_main_pet_t::dismiss();

    sim -> auras.fel_intelligence -> expire();
  }
};

// ==========================================================================
// Pet Succubus
// ==========================================================================

struct succubus_pet_t : public warlock_main_pet_t
{
  struct lash_of_pain_t : public warlock_pet_spell_t
  {
    lash_of_pain_t( succubus_pet_t* p ) :
      warlock_pet_spell_t( "lash_of_pain", p, "Lash of Pain" )
    {
      warlock_t*  o     = p -> owner -> cast_warlock();

      direct_power_mod  = 0.642; // tested in-game as of 2010/12/20

      if ( o -> level == 85 )
      {
        // only tested at level 85
        base_dd_min = 283;
        base_dd_max = 314;
      }

      if ( o -> bugs ) min_gcd = 1.5;
    }

    virtual void impact( player_t* t, int impact_result, double travel_dmg )
    {
      warlock_pet_spell_t::impact( t, impact_result, travel_dmg );

      if ( result_is_hit( impact_result ) )
        trigger_mana_feed ( this, impact_result );
    }

    virtual void player_buff()
    {
      warlock_t* o = player -> cast_pet() -> owner -> cast_warlock();
      warlock_pet_spell_t::player_buff();

      if ( o -> race == RACE_ORC )
      {
        // Glyph is additive with orc racial
        player_multiplier /= 1.05;
        player_multiplier *= 1.05 + o -> glyphs.lash_of_pain -> base_value();
      }
      else
      {
        player_multiplier *= 1.0 + o -> glyphs.lash_of_pain -> base_value();
      }
    }
  };

  succubus_pet_t( sim_t* sim, player_t* owner ) :
    warlock_main_pet_t( sim, owner, "succubus", PET_SUCCUBUS )
  {
    damage_modifier = 1.025;

    action_list_str += "/snapshot_stats";
    action_list_str += "/lash_of_pain";
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "lash_of_pain" ) return new lash_of_pain_t( this );

    return warlock_main_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Pet Voidwalker
// ==========================================================================

struct voidwalker_pet_t : public warlock_main_pet_t
{
  struct torment_t : public warlock_pet_spell_t
  {
    torment_t( voidwalker_pet_t* p ) :
      warlock_pet_spell_t( "torment", p, "Torment" )
    {
      direct_power_mod = 0.512;
    }

    virtual void impact( player_t* t, int impact_result, double travel_dmg )
    {
      warlock_pet_spell_t::impact( t, impact_result, travel_dmg );

      if ( result_is_hit( impact_result ) )
        trigger_mana_feed ( this, impact_result ); // untested
    }
  };

  voidwalker_pet_t( sim_t* sim, player_t* owner ) :
    warlock_main_pet_t( sim, owner, "voidwalker", PET_VOIDWALKER )
  {
    damage_modifier = 0.86;

    action_list_str += "/snapshot_stats";
    action_list_str += "/torment";
  }

  virtual void init_base()
  {
    warlock_main_pet_t::init_base();

    main_hand_attack = new warlock_pet_melee_t( this, "voidwalker_melee" );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "torment" ) return new torment_t( this );

    return warlock_main_pet_t::create_action( name, options_str );
  }
};
// ==========================================================================
// Pet Infernal
// ==========================================================================

struct infernal_pet_t : public warlock_guardian_pet_t
{
  // Immolation Damage Spell ================================================

  struct immolation_damage_t : public warlock_pet_spell_t
  {
    immolation_damage_t( infernal_pet_t* p ) :
      warlock_pet_spell_t( "immolation_dmg", 20153, p )
    {
      dual        = true;
      background  = true;
      aoe         = -1;
      direct_tick = true;
      may_crit    = false;
      stats = p -> get_stats( "infernal_immolation", this );
      direct_power_mod  = 0.4;
    }
  };

  struct infernal_immolation_t : public warlock_pet_spell_t
  {
    immolation_damage_t* immolation_damage;

    infernal_immolation_t( infernal_pet_t* p, const std::string& options_str ) :
      warlock_pet_spell_t( "infernal_immolation", 19483, p ), immolation_damage( 0 )
    {
      parse_options( NULL, options_str );

      callbacks    = false;
      num_ticks    = 1;
      hasted_ticks = false;
      harmful = false;
      trigger_gcd=1.5;

      immolation_damage = new immolation_damage_t( p );
    }

    virtual void tick( dot_t* d )
    {
      d -> current_tick = 0; // ticks indefinitely

      immolation_damage -> execute();
    }
  };

  infernal_pet_t( sim_t* sim, player_t* owner ) :
    warlock_guardian_pet_t( sim, owner, "infernal", PET_INFERNAL )
  {
    action_list_str += "/snapshot_stats";
    if ( level >= 50 ) action_list_str += "/immolation,if=!ticking";
  }

  virtual void init_base()
  {
    warlock_guardian_pet_t::init_base();

    main_hand_attack = new warlock_pet_melee_t( this, "Infernal Melee" );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "immolation" ) return new infernal_immolation_t( this, options_str );

    return warlock_guardian_pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Pet Doomguard
// ==========================================================================

struct doomguard_pet_t : public warlock_guardian_pet_t
{
  struct doom_bolt_t : public warlock_pet_spell_t
  {
    doom_bolt_t( doomguard_pet_t* p ) :
      warlock_pet_spell_t( "doombolt", p, "Doom Bolt" )
    {
      //FIXME: Needs testing, but WoL seems to suggest it has been changed from 2.5 to 3.0 sometime after 4.1.
      base_execute_time = 3.0;

      //Rough numbers based on report in EJ thread 2011/07/04
      direct_power_mod  = 1.36;
      base_dd_min *= 1.25;
      base_dd_max *= 1.25;
    }
  };

  doomguard_pet_t( sim_t* sim, player_t* owner ) :
    warlock_guardian_pet_t( sim, owner, "doomguard", PET_DOOMGUARD )
  { }

  virtual void init_base()
  {
    warlock_guardian_pet_t::init_base();

    action_list_str += "/snapshot_stats";
    action_list_str += "/doom_bolt";
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "doom_bolt" ) return new doom_bolt_t( this );

    return warlock_guardian_pet_t::create_action( name, options_str );
  }


  virtual double composite_player_multiplier( const school_type school, action_t* a ) const
  {
    //FIXME: This is all untested, but seems to match what people are reporting in forums

    double m = player_t::composite_player_multiplier( school, a );

    warlock_t* o = owner -> cast_warlock();

    if ( o -> race == RACE_ORC )
    {
      m  *= 1.05;
    }

    double mastery_value = o -> mastery_spells.master_demonologist -> effect_base_value( 3 );
    
    double mastery_gain = ( o -> mastery_spells.master_demonologist -> ok() * snapshot_mastery * mastery_value / 10000.0 );
       
    m *= 1.0 + mastery_gain;

    return m;
  }

};

// ==========================================================================
// Pet Ebon Imp
// ==========================================================================

struct ebon_imp_pet_t : public warlock_guardian_pet_t
{
  ebon_imp_pet_t( sim_t* sim, player_t* owner ) :
    warlock_guardian_pet_t( sim, owner, "ebon_imp", PET_EBON_IMP )
  {
    action_list_str += "/snapshot_stats";
  }

  virtual double    available() const { return sim -> max_time; }

  virtual double composite_attack_power() const
  {
    return 0;
  }

  virtual void init_base()
  {
    warlock_guardian_pet_t::init_base();

    main_hand_attack = new warlock_pet_melee_t( this, "ebon_imp_melee" );
  }
};

// ==========================================================================
// Pet Fiery Imp Tier 12 set bonus
// ==========================================================================

struct fiery_imp_pet_t : public pet_t
{
  double snapshot_crit;

  fiery_imp_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "fiery_imp", true /*guardian*/ ),
    snapshot_crit( 0 )
  {
    action_list_str += "/snapshot_stats";
    action_list_str += "/flame_blast";
  }

  virtual void summon( double duration=0 )
  {
    pet_t::summon( duration );
    // Guardians use snapshots
    snapshot_crit = owner -> composite_spell_crit();
    if ( owner -> bugs )
    {
      snapshot_crit = 0.00; // Rough guess
    }
  }

  virtual double composite_spell_crit() const
  {
    return snapshot_crit;
  }

  virtual double composite_spell_haste() const
  {
    return 1.0;
  }

  struct flame_blast_t : public spell_t
  {
    flame_blast_t( fiery_imp_pet_t* p ):
      spell_t( "flame_blast", 99226, p )
    {
      may_crit          = true;
      trigger_gcd = 1.5;
      if ( p -> owner -> bugs )
      {
        ability_lag = 0.74;
        ability_lag_stddev = 0.62 / 2.0;
      }
    }
  };

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "flame_blast" ) return new flame_blast_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// Curse of Elements Debuff =================================================

struct coe_debuff_t : public debuff_t
{
  coe_debuff_t( player_t* t ) : debuff_t( t, "curse_of_elements", 1, 300.0 )
  {}

  virtual void expire()
  {
    if ( player )
    {
      player = 0;
    }
    debuff_t::expire();
  }
};

// Curse of Elements Spell ==================================================

struct curse_of_elements_t : public warlock_spell_t
{
  curse_of_elements_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "curse_of_the_elements", p, "Curse of the Elements" )
  {
    parse_options( NULL, options_str );

    trigger_gcd -= p -> constants_pandemic_gcd * p -> talent_pandemic -> rank();
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      warlock_t* p = player -> cast_warlock();
      target -> debuffs.curse_of_elements -> expire();
      target -> debuffs.curse_of_elements -> trigger( 1, effect2().base_value() );
      target -> debuffs.curse_of_elements -> source = p;
    }
  }

  virtual bool ready()
  {
    if ( target -> debuffs.curse_of_elements -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Bane of Agony Spell ======================================================

struct bane_of_agony_t : public warlock_spell_t
{
  bane_of_agony_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "bane_of_agony", p, "Bane of Agony" )
  {
    /*
     * BOA DATA:
     * No Glyph
     * 16 Ticks:
     * middle low low low low low    middle middle middle middle middle high high high high high
     * 15 ticks:
     * low    low low low low middle middle middle middle middle high   high high high high
     * 14 ticks:
     * low    low low low low middle middle middle middle high strange strange strange
     * 828 828 403 403 403 486 998 485 485 1167  691 1421 691 1316
     * 374 373 373 373 767 449 449 450 924  526  640  640 640 1316
     * 378 777 777 777 378 461 460 460 460  543 1370  666 666  666
     * 378 378 777 378 378 460 460 946 460  542 1369  667 666 1369
     *
     *
     * 13 ticks:
     * 2487 sp, lvl 80 -> middle = 130.14 + 0.1000000015 * 2487 = 378.84
     * -> difference = base_td / 2, only 1 middle tick at the beginning
     * 378 645 313 313 313 378 379 778 378 443 444 444 443
     * 12 ticks:
     * low low low low    middle middle middle middle   high high high high
     * and it looks like low-tick = middle-tick - base_td / 2
     * and              high-tick = middle-tick + base_td / 2
     * at 12 ticks, everything is very consistent with this logic
     */

    parse_options( NULL, options_str );

    may_crit   = false;

    base_crit += p -> talent_doom_and_gloom -> effect1().percent();
    trigger_gcd -= p -> constants_pandemic_gcd * p -> talent_pandemic -> rank();

    int extra_ticks = ( int ) ( p -> glyphs.bane_of_agony -> base_value() / 1000.0 / base_tick_time );

    if ( extra_ticks > 0 )
    {
      // after patch 3.0.8, the added ticks are double the base damage

      double total_damage = base_td_init * ( num_ticks + extra_ticks * 2 );

      num_ticks += extra_ticks;

      base_td_init = total_damage / num_ticks;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      if ( td -> dots_bane_of_doom -> ticking )
      {
        td -> dots_bane_of_doom -> cancel();
      }
      else if ( p -> buffs_bane_of_havoc -> up() )
      {
        p -> buffs_bane_of_havoc -> expire();
      }
    }
  }
};

// Bane of Doom Spell =======================================================

struct bane_of_doom_t : public warlock_spell_t
{
  bane_of_doom_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "bane_of_doom", p, "Bane of Doom" )
  {
    parse_options( NULL, options_str );

    hasted_ticks = false;
    may_crit     = false;

    trigger_gcd -= p -> constants_pandemic_gcd * p -> talent_pandemic -> rank();
    base_crit += p -> talent_doom_and_gloom -> effect1().percent();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      if ( td -> dots_bane_of_agony -> ticking )
      {
        td -> dots_bane_of_agony -> cancel();
      }
      else if ( p -> buffs_bane_of_havoc -> up() )
      {
        p -> buffs_bane_of_havoc -> expire();
      }
    }
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );
    warlock_t* p = player -> cast_warlock();

    double x = effect2().percent() + p -> talent_impending_doom -> effect1().percent();
    if ( p -> rng_ebon_imp -> roll ( x ) )
    {
      p -> procs_ebon_imp -> occur();
      p -> pet_ebon_imp -> dismiss();
      p -> pet_ebon_imp -> summon( 14.99 );
    }
  }
};

// Bane of Havoc Spell ======================================================

struct bane_of_havoc_t : public warlock_spell_t
{
  bane_of_havoc_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "bane_of_havoc", p, "Bane of Havoc" )
  {
    parse_options( NULL, options_str );

    trigger_gcd -= p -> constants_pandemic_gcd * p -> talent_pandemic -> rank();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      p -> buffs_bane_of_havoc -> trigger();

      if ( td -> dots_bane_of_agony -> ticking )
      {
        td -> dots_bane_of_agony -> cancel();
      }
      else if ( td -> dots_bane_of_doom -> ticking )
      {
        td -> dots_bane_of_doom -> cancel();
      }
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_bane_of_havoc -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Shadow Bolt Spell ========================================================

struct shadow_bolt_t : public warlock_spell_t
{
  int isb;
  bool used_shadow_trance;

  shadow_bolt_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "shadow_bolt", p, "Shadow Bolt" ),
    isb( 0 ), used_shadow_trance( 0 )
  {
    option_t options[] =
    {
      { "isb",   OPT_BOOL, &isb },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_execute_time += p -> talent_bane -> effect1().seconds();
    base_cost  *= 1.0 + p -> glyphs.shadow_bolt -> base_value();
    base_multiplier *= 1.0 + ( p -> talent_shadow_and_flame -> effect2().percent() );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new shadow_bolt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double execute_time() const
  {
    double h = warlock_spell_t::execute_time();
    warlock_t* p = player -> cast_warlock();
    if ( p -> buffs_shadow_trance -> up() ) h = 0;
    if ( p -> buffs_backdraft -> up() )
    {
      h *= 1.0 + p -> buffs_backdraft -> effect1().percent();
    }
    return h;
  }

  virtual void schedule_execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::schedule_execute();

    if ( p -> buffs_shadow_trance -> check() )
    {
      p -> buffs_shadow_trance -> expire();
      used_shadow_trance = true;
    }
    else
    {
      used_shadow_trance = false;
    }
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    for ( int i=0; i < 4; i++ )
    {
      p -> uptimes_backdraft[ i ] -> update( i == p -> buffs_backdraft -> check() );
    }

    if ( ! used_shadow_trance && p -> buffs_backdraft -> check() )
    {
      p -> buffs_backdraft -> decrement();
    }

    trigger_tier12_4pc_caster( this );
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    if ( p -> buffs_demon_soul_succubus -> up() )
    {
      player_multiplier *= 1.0 + p -> buffs_demon_soul_succubus -> effect1().percent();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();

      trigger_decimation( this, impact_result );
      trigger_impending_doom( this );
      td -> debuffs_shadow_embrace -> trigger();
      t -> debuffs.shadow_and_flame -> trigger( 1, 1.0, p -> talent_shadow_and_flame -> proc_chance() );
    }
  }

  virtual bool ready()
  {
    if ( isb )
      if ( ! target -> debuffs.shadow_and_flame -> check() )
        return false;

    return warlock_spell_t::ready();
  }
};

// Burning Embers Spell =====================================================

// Trigger Burning Embers ===================================================

void trigger_burning_embers ( spell_t* s, double dmg )
{
  warlock_t* p;
  if ( s -> player -> type == WARLOCK )
  {
    p = s -> player -> cast_warlock();
  }
  else
  {
    pet_t* a = ( pet_t* ) s -> player -> cast_pet();
    p = a -> owner -> cast_warlock();
  }

  if ( p -> talent_burning_embers -> rank() )
  {
    struct burning_embers_t : public warlock_spell_t
    {
      burning_embers_t( warlock_t* p ) :
        warlock_spell_t( "burning_embers", p, 85421 )
      {
        background = true;
        tick_may_crit = false;
        hasted_ticks = false;
        may_trigger_dtr = false;
        init();
      }

      virtual double calculate_tick_damage() { return base_td; }
    };

    if ( ! p -> spells_burning_embers ) p -> spells_burning_embers = new burning_embers_t( p );

    if ( ! p -> spells_burning_embers -> dot() -> ticking ) p -> spells_burning_embers -> base_td = 0;

    int num_ticks = p -> spells_burning_embers -> num_ticks;

    double spmod = 0.7;

    //FIXME: The 1.2 modifier to the adder was experimentally observed on live realms 2011/10/14
    double cap = ( spmod * p -> talent_burning_embers -> rank() * p -> composite_spell_power( SCHOOL_FIRE ) * p -> composite_spell_power_multiplier() + p -> talent_burning_embers -> effect_min( 2 ) * 1.2 ) / num_ticks;

    p -> spells_burning_embers -> base_td += ( dmg * p -> talent_burning_embers -> effect1().percent() ) / num_ticks;

    if ( p -> spells_burning_embers -> base_td > cap ) p -> spells_burning_embers -> base_td = cap;

    p -> spells_burning_embers -> execute();
  }
}

// Chaos Bolt Spell =========================================================

struct chaos_bolt_t : public warlock_spell_t
{
  chaos_bolt_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "chaos_bolt", p, "Chaos Bolt" )
  {
    parse_options( NULL, options_str );

    may_resist = false;
    may_miss = false;

    base_execute_time += p -> talent_bane -> effect1().seconds();
    base_execute_time *= 1 + p -> sets -> set ( SET_T11_2PC_CASTER ) -> effect_base_value( 1 ) * 0.01;
    cooldown -> duration += ( p -> glyphs.chaos_bolt -> base_value() / 1000.0 );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new chaos_bolt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double execute_time() const
  {
    double h = warlock_spell_t::execute_time();
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_backdraft -> up() )
      h *= 1.0 + p -> buffs_backdraft -> effect1().percent();

    return h;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    for ( int i=0; i < 4; i++ )
    {
      p -> uptimes_backdraft[ i ] -> update( i == p -> buffs_backdraft -> check() );
    }

    if ( p -> buffs_backdraft -> check() )
      p -> buffs_backdraft -> decrement();
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();
    warlock_spell_t::player_buff();

    if ( td -> dots_immolate -> ticking )
      player_multiplier *= 1 + p -> talent_fire_and_brimstone -> effect1().percent();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      trigger_soul_leech( this );
  }
};

// Death Coil Spell =========================================================

struct death_coil_t : public warlock_spell_t
{
  death_coil_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "death_coil", p, "Death Coil" )
  {
    parse_options( NULL, options_str );

    binary = true;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      player -> resource_gain( RESOURCE_HEALTH, direct_dmg );
  }
};

// Shadow Burn Spell ========================================================

struct shadowburn_t : public warlock_spell_t
{
  cooldown_t* cd_glyph_of_shadowburn;

  shadowburn_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "shadowburn", p, "Shadowburn" ),
    cd_glyph_of_shadowburn( 0 )
  {
    check_talent( p -> talent_shadowburn -> rank() );

    parse_options( NULL, options_str );


    if ( p -> glyphs.shadowburn -> ok() )
    {
      cd_glyph_of_shadowburn             = p -> get_cooldown ( "glyph_of_shadowburn" );
      cd_glyph_of_shadowburn -> duration = p -> dbc.spell( 91001 ) -> duration();
    }

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new shadowburn_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      trigger_soul_leech( this );
  }

  virtual void update_ready()
  {
    warlock_spell_t::update_ready();
    warlock_t* p = player -> cast_warlock();

    if ( p -> glyphs.shadowburn -> ok() )
    {
      if ( cd_glyph_of_shadowburn -> remains() == 0 && target -> health_percentage() < p -> glyphs.shadowburn -> effect1().base_value() )
      {
        cooldown -> reset();
        cd_glyph_of_shadowburn -> start();
      }
    }
  }

  virtual bool ready()
  {
    if ( target -> health_percentage() >= 20 )
      return false;

    return warlock_spell_t::ready();
  }
};

// Shadowfury Spell =========================================================

struct shadowfury_t : public warlock_spell_t
{
  double cast_gcd;

  shadowfury_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "shadowfury", p, "Shadowfury" ),
    cast_gcd( -1 )
  {
    check_talent( p -> talent_shadowfury -> rank() );

    option_t options[] =
    {
      { "cast_gcd",    OPT_FLT,  &cast_gcd    },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    // estimate - measured at ~0.6sec, but lag in there too, plus you need to mouse-click
    trigger_gcd = ( cast_gcd >= 0 ) ? cast_gcd : 0.5;
  }
};

// Corruption Spell =========================================================

struct corruption_t : public warlock_spell_t
{
  corruption_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "corruption", p, "Corruption" )
  {
    parse_options( NULL, options_str );

    may_crit   = false;
    base_crit += p -> talent_everlasting_affliction -> effect2().percent();
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    player_td_multiplier += p -> talent_improved_corruption -> effect1().percent();
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick( d );

    p -> buffs_eradication -> trigger();

    if ( p -> buffs_shadow_trance -> trigger() )
      p -> procs_shadow_trance -> occur();

    if ( p -> talent_siphon_life -> rank() )
    {
      if ( p -> rng_siphon_life -> roll ( p -> talent_siphon_life -> proc_chance() ) )
      {
        p -> resource_gain( RESOURCE_HEALTH, p -> resource_max[ RESOURCE_HEALTH ] * 0.02 );
      }
    }
  }
};

// Drain Life Spell =========================================================

struct drain_life_heal_t : public warlock_heal_t
{
  drain_life_heal_t( warlock_t* p ) :
    warlock_heal_t( "drain_life_heal", p, 89653 )
  {
    background = true;
    may_miss = false;
    base_dd_min = base_dd_max = 0; // Is parsed as 2
    init();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    double heal_pct = effect1().percent();

    if ( ( p -> resource_current[ RESOURCE_HEALTH ] / p -> resource_max[ RESOURCE_HEALTH ] ) <= 0.25 )
      heal_pct += p -> talent_deaths_embrace -> effect1().percent();

    base_dd_min = base_dd_max = p -> resource_max[ RESOURCE_HEALTH ] * heal_pct;
    warlock_heal_t::execute();
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();
    warlock_heal_t::player_buff();

    double min_multiplier[] = { 0, 0.03, 0.06 };
    double max_multiplier [3] = {0};
    if ( p -> bugs )
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.12;
      max_multiplier[2] =  0.24;
    }
    else
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.09;
      max_multiplier[2] =  0.18;
    }

    assert( p -> talent_soul_siphon -> rank() <= 2 );

    double min = min_multiplier[ p -> talent_soul_siphon -> rank() ];
    double max = max_multiplier[ p -> talent_soul_siphon -> rank() ];

    double multiplier = td -> affliction_effects() * min;

    if ( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;
  }
};

struct drain_life_t : public warlock_spell_t
{
  drain_life_heal_t* heal;

  drain_life_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "drain_life", p, "Drain Life" ), heal( 0 )
  {
    parse_options( NULL, options_str );

    channeled    = true;
    binary       = true;
    hasted_ticks = false;
    may_crit     = false;

    heal = new drain_life_heal_t( p );
  }

  virtual void last_tick( dot_t* d)
  {
    warlock_spell_t::last_tick( d );
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_soulburn -> check() )
      p -> buffs_soulburn -> expire();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      trigger_everlasting_affliction( this );
  }

  virtual double tick_time() const
  {
    warlock_t* p = player -> cast_warlock();
    double t = warlock_spell_t::tick_time();

    if ( p -> buffs_soulburn -> up() )
      t *= 1.0 - 0.5;

    return t;
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();
    warlock_spell_t::player_buff();

    double min_multiplier[] = { 0, 0.03, 0.06 };
    double max_multiplier [3] = {0};
    if ( p -> bugs )
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.12;
      max_multiplier[2] =  0.24;
    }
    else
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.09;
      max_multiplier[2] =  0.18;
    }

    assert( p -> talent_soul_siphon -> rank() <= 2 );

    double min = min_multiplier[ p -> talent_soul_siphon -> rank() ];
    double max = max_multiplier[ p -> talent_soul_siphon -> rank() ];

    double multiplier = td -> affliction_effects() * min;

    if ( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick( d );

    if ( p -> buffs_shadow_trance -> trigger( 1, 1.0, p -> talent_nightfall -> proc_chance() ) )
      p -> procs_shadow_trance -> occur();

    heal -> execute();
  }
};

// Drain Soul Spell =========================================================

struct drain_soul_t : public warlock_spell_t
{
  drain_soul_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "drain_soul", p, "Drain Soul" )
  {
    parse_options( NULL, options_str );

    channeled    = true;
    hasted_ticks = true; // informative
    may_crit     = false;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    trigger_tier12_4pc_caster( this );
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( result_is_hit() )
    {
      warlock_t* p = player -> cast_warlock();

      trigger_everlasting_affliction( this );

      if ( p -> talent_pandemic -> rank() )
      {
        if ( ( target -> health_percentage() < effect3().base_value() ) && ( p -> rng_pandemic -> roll( p -> talent_pandemic -> rank() * 0.5 ) ) )
        {
          warlock_targetdata_t* td = targetdata() -> cast_warlock();
          td -> dots_unstable_affliction -> refresh_duration();
        }
      }
    }
  }

  virtual void player_buff()
  {
    warlock_spell_t::player_buff();
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    double min_multiplier[] = { 0, 0.03, 0.06 };
    double max_multiplier [3] = {0};
    if ( p -> bugs )
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.12;
      max_multiplier[2] =  0.24;
    }
    else
    {
      max_multiplier[0] =  0.00;
      max_multiplier[1] =  0.09;
      max_multiplier[2] =  0.18;
    }

    assert( p -> talent_soul_siphon -> rank() <= 2 );

    double min = min_multiplier[ p -> talent_soul_siphon -> rank() ];
    double max = max_multiplier[ p -> talent_soul_siphon -> rank() ];

    double multiplier = td -> affliction_effects() * min;

    if ( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;

    // FIXME! Hack! Deaths Embrace is additive with Drain Soul "execute".
    // Perhaps it is time to add notion of "execute" into action_t class.

    double de_bonus = trigger_deaths_embrace( this );
    if ( de_bonus ) player_multiplier /= 1.0 + de_bonus;


    if ( target -> health_percentage() < effect3().base_value() )
    {
      player_multiplier *= 2.0 + de_bonus;
    }
  }
};

// Unstable Affliction Spell ================================================

struct unstable_affliction_t : public warlock_spell_t
{
  unstable_affliction_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "unstable_affliction", p, "Unstable Affliction" )
  {
    parse_options( NULL, options_str );

    check_talent( ok() );

    may_crit   = false;
    base_crit += p -> talent_everlasting_affliction -> effect2().percent();
    base_execute_time += p -> glyphs.unstable_affliction -> base_value() / 1000.0;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( result_is_hit() )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      if ( td -> dots_immolate -> ticking )
        td -> dots_immolate -> cancel();
    }
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick( d );

    if ( tick_dmg > 0 )
      p -> buffs_tier11_4pc_caster -> trigger( 2 );
  }

};

// Haunt Spell ==============================================================

struct haunt_t : public warlock_spell_t
{
  haunt_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "Haunt", p, "Haunt" )
  {
    check_talent( p -> talent_haunt -> rank() );

    parse_options( NULL, options_str );

    base_execute_time *= 1 + p -> sets -> set ( SET_T11_2PC_CASTER ) -> effect_base_value( 1 ) * 0.01;
    direct_power_mod = 0.5577;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new haunt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_t* p = player -> cast_warlock();
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      td -> debuffs_haunted -> trigger();
      td -> debuffs_shadow_embrace -> trigger();
      trigger_everlasting_affliction( this );
    }
  }
};

// Immolate Spell ===========================================================

struct immolate_t : public warlock_spell_t
{
  immolate_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "immolate", p, "Immolate", WARLOCK )
  {
    parse_options( NULL, options_str );

    base_execute_time += p -> talent_bane -> effect1().seconds();

    base_dd_multiplier *= 1.0 + ( p -> talent_improved_immolate -> effect1().percent() );

    if ( p -> talent_inferno -> rank() ) num_ticks += 2;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new immolate_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    player_td_multiplier += ( p -> glyphs.immolate -> base_value() + p -> talent_improved_immolate -> effect1().percent() );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      warlock_t* p = player -> cast_warlock();
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      if ( td -> dots_unstable_affliction -> ticking )
      {
        td -> dots_unstable_affliction -> cancel();
      }
    }
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );
    warlock_t* p = player -> cast_warlock();

    p -> buffs_molten_core -> trigger( 3 );
    if ( tick_dmg > 0 )
    {
      p -> buffs_tier11_4pc_caster -> trigger( 2 );
    }
  }

};

// Shadowflame DOT Spell ====================================================

struct shadowflame_dot_t : public warlock_spell_t
{
  shadowflame_dot_t( warlock_t* p ) :
    warlock_spell_t( "shadowflame_dot", p, 47960 )
  {
    proc       = true;
    background = true;
  }
};

// Shadowflame Spell ========================================================

struct shadowflame_t : public warlock_spell_t
{
  shadowflame_dot_t* sf_dot;

  shadowflame_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "shadowflame", p, "Shadowflame" ), sf_dot( 0 )
  {
    parse_options( NULL, options_str );

    sf_dot = new shadowflame_dot_t( p );

    add_child( sf_dot );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new shadowflame_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    sf_dot -> execute();
  }
};

// Conflagrate Spell ========================================================

struct conflagrate_t : public warlock_spell_t
{
  conflagrate_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "conflagrate", p, "Conflagrate" )
  {
    check_talent( p -> talent_conflagrate -> ok() );

    parse_options( NULL, options_str );

    base_crit += p -> talent_fire_and_brimstone -> effect2().percent();
    cooldown -> duration += ( p -> glyphs.conflagrate -> base_value() / 1000.0 );
    base_dd_multiplier *= 1.0 + ( p -> glyphs.immolate -> base_value() ) + ( p -> talent_improved_immolate -> effect1().percent() );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new conflagrate_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    action_t* a = td -> dots_immolate -> action;

    double periodic_dmg = a -> base_td + total_power() * a -> tick_power_mod;

    int periodic_ticks = td -> dots_immolate -> action -> hasted_num_ticks();

    base_dd_min = base_dd_max = periodic_dmg * periodic_ticks * effect2().percent() ;

    warlock_spell_t::execute();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
      p -> buffs_backdraft -> trigger( 3 );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    if ( ! ( td -> dots_immolate -> ticking ) )
      return false;

    return warlock_spell_t::ready();
  }
};

// Incinerate Spell =========================================================

struct incinerate_t : public warlock_spell_t
{
  spell_t*  incinerate_burst_immolate;

  incinerate_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "incinerate", p, "Incinerate" ),
    incinerate_burst_immolate( 0 )
  {
    parse_options( NULL, options_str );

    base_multiplier   *= 1.0 + ( p -> talent_shadow_and_flame -> effect2().percent() );
    base_execute_time += p -> talent_emberstorm -> effect3().seconds();
    base_multiplier   *= 1.0 + ( p -> glyphs.incinerate -> base_value() );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new incinerate_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    if ( td -> dots_immolate -> ticking ) {
      base_dd_adder = ( sim -> range( base_dd_min, base_dd_max ) + direct_power_mod * total_power() ) / 6;
    }

    warlock_spell_t::execute();

    for ( int i=0; i < 4; i++ )
    {
      p -> uptimes_backdraft[ i ] -> update( i == p -> buffs_backdraft -> check() );
    }

    if ( p -> buffs_backdraft -> check() )
    {
      p -> buffs_backdraft -> decrement();
    }

    trigger_tier12_4pc_caster( this );
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );
    warlock_t* p = player -> cast_warlock();

    if ( result_is_hit( impact_result ) )
    {
      trigger_decimation( this, impact_result );
      trigger_impending_doom( this );
      t -> debuffs.shadow_and_flame -> trigger( 1, 1.0, p -> talent_shadow_and_flame -> proc_chance() );
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    if ( p -> buffs_molten_core -> check() )
    {
      player_multiplier *= 1 + p -> buffs_molten_core -> effect1().percent();
      p -> buffs_molten_core -> decrement();
    }
    warlock_targetdata_t* td = targetdata() -> cast_warlock();

    if ( td -> dots_immolate -> ticking )
    {
      player_multiplier *= 1 + p -> talent_fire_and_brimstone -> effect1().percent();
    }
  }

  virtual double execute_time() const
  {
    warlock_t* p = player -> cast_warlock();
    double h = warlock_spell_t::execute_time();

    if ( p -> buffs_molten_core -> up() )
    {
      h *= 1.0 + p -> buffs_molten_core -> effect3().percent();
    }
    if ( p -> buffs_backdraft -> up() )
    {
      h *= 1.0 + p -> buffs_backdraft -> effect1().percent();
    }

    return h;
  }
};

// Searing Pain Spell =======================================================

struct searing_pain_t : public warlock_spell_t
{
  searing_pain_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "searing_pain", p, "Searing Pain" )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new searing_pain_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    if ( target -> health_percentage() <= 25 && p -> talent_improved_searing_pain -> rank() )
    {
      player_crit += p -> talent_improved_searing_pain -> effect1().percent();
    }

    if ( p -> buffs_soulburn -> up() )
      player_crit +=  p -> buffs_soulburn -> effect3().percent();

    if ( p -> buffs_searing_pain_soulburn -> up() )
      player_crit += p -> buffs_searing_pain_soulburn -> effect1().percent();
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_soulburn -> check() )
    {
      p -> buffs_soulburn -> expire();
      p -> buffs_searing_pain_soulburn -> trigger();
    }
  }
};

// Soul Fire Spell ==========================================================

struct soul_fire_t : public warlock_spell_t
{
  soul_fire_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "soul_fire", p, "Soul Fire" )
  {
    parse_options( NULL, options_str );

    base_execute_time += p -> talent_emberstorm -> effect1().seconds();

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new soul_fire_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_empowered_imp -> check() )
      p -> buffs_empowered_imp -> expire();
    else if ( p -> buffs_soulburn -> check() )
    {
      p -> buffs_soulburn -> expire();
      if ( p -> set_bonus.tier13_4pc_caster() )
      {
        p -> resource_gain( RESOURCE_SOUL_SHARDS, 1, p -> gains_tier13_4pc );
      }
    }

    trigger_tier12_4pc_caster( this );
  }

  virtual double execute_time() const
  {
    warlock_t* p = player -> cast_warlock();
    double t = warlock_spell_t::execute_time();

    if ( p -> buffs_decimation -> up() )
    {
      t *= 1.0 - p -> talent_decimation -> effect1().percent();
    }
    if ( p -> buffs_empowered_imp -> up() )
    {
      t = 0;
    }
    if ( p -> buffs_soulburn -> up() )
    {
      t = 0;
    }

    return t;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_t* p = player -> cast_warlock();

      trigger_decimation( this, impact_result );

      trigger_impending_doom( this );

      trigger_soul_leech( this );

      trigger_burning_embers( this, travel_dmg );

      p -> buffs_improved_soul_fire -> trigger();
    }
  }
};

// Life Tap Spell ===========================================================

struct life_tap_t : public warlock_spell_t
{
  double trigger;
  double max_mana_pct;

  life_tap_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "life_tap", p, "Life Tap" ),
    trigger( 0 ), max_mana_pct( 0 )
  {
    option_t options[] =
    {
      { "mana_percentage<", OPT_FLT,  &max_mana_pct     },
      { "trigger",          OPT_FLT,  &trigger          },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;

    if ( p -> glyphs.life_tap -> ok() )
      trigger_gcd += p -> glyphs.life_tap -> effect1().seconds();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    double life = p -> resource_max[ RESOURCE_HEALTH ] * effect3().percent();
    double mana = life * effect2().percent() * ( 1.0 + p -> talent_improved_life_tap -> base_value() / 100.0 );
    p -> resource_loss( RESOURCE_HEALTH, life );
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains_life_tap );
    if ( p -> talent_mana_feed -> rank() && p -> active_pet )
    {
      p -> active_pet -> resource_gain( RESOURCE_MANA, mana * p -> talent_mana_feed -> effect1().percent(), p -> active_pet -> gains_mana_feed );
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if (  max_mana_pct > 0 )
      if ( ( 100.0 * p -> resource_current[ RESOURCE_MANA ] / p -> resource_max[ RESOURCE_MANA ] ) > max_mana_pct )
        return false;

    if ( trigger > 0 )
      if ( p -> resource_current[ RESOURCE_MANA ] > trigger )
        return false;

    return warlock_spell_t::ready();
  }
};

// Demon Armor ==============================================================

struct demon_armor_t : public warlock_spell_t
{
  demon_armor_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "demon_armor", p, "Demon Armor" )
  {
    parse_options( NULL, options_str );
   
    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    warlock_spell_t::execute();

    p -> buffs_demon_armor -> trigger( 1, effect2().percent() + p -> talent_demonic_aegis -> effect1().percent() );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_demon_armor -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Fel Armor Spell ==========================================================

struct fel_armor_t : public warlock_spell_t
{
  double bonus_spell_power;

  fel_armor_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "fel_armor", p, "Fel Armor" ), bonus_spell_power( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;

    bonus_spell_power = p -> buffs_fel_armor -> effect_min( 1 );

    // Model the passive health tick.....
    // FIXME: For PTR need to add new healing mechanic
#if 0
    if ( ! p -> dbc.ptr )
    {
      base_tick_time = effect_period( 2 );
      num_ticks = 1;
    }
#endif
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    p -> buffs_fel_armor -> trigger( 1, bonus_spell_power );

    warlock_spell_t::execute();
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();

    d -> current_tick = 0; // ticks indefinitely

    p -> resource_gain( RESOURCE_HEALTH,
                        p -> resource_max[ RESOURCE_HEALTH ] * effect2().percent() * ( 1.0 + p -> talent_demonic_aegis -> effect1().percent() ),
                        p -> gains_fel_armor, this );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_fel_armor -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Summon Pet Spell =========================================================

struct summon_pet_t : public warlock_spell_t
{
  double summoning_duration;
  pet_t* pet;

private:
  void _init_summon_pet_t( const std::string& options_str, const char* pet_name )
  {
    parse_options( NULL, options_str );

    warlock_t* p = player -> cast_warlock();
    harmful = false;
    base_execute_time += p -> talent_master_summoner -> effect1().seconds();
    base_cost         *= 1.0 + p -> talent_master_summoner -> effect2().percent();

    pet = p -> find_pet( pet_name );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", p -> name(), pet_name );
      sim -> cancel();
    }
  }

public:
  summon_pet_t( const char* n, warlock_t* p, const char* sname, const std::string& options_str="" ) :
    warlock_spell_t( n, p, sname ), summoning_duration ( 0 ), pet( 0 )
  {
    _init_summon_pet_t( options_str, n );
  }

  summon_pet_t( const char* n, warlock_t* p, int id, const std::string& options_str="" ) :
    warlock_spell_t( n, p, id ), summoning_duration ( 0 ), pet( 0 )
  {
    _init_summon_pet_t( options_str, n );
  }

  virtual void execute()
  {
    pet -> summon( summoning_duration );

    warlock_spell_t::execute();
  }
};

// Summon Main Pet Spell ====================================================

struct summon_main_pet_t : public summon_pet_t
{

  summon_main_pet_t( const char* n, warlock_t* p, const char* sname, const std::string& options_str ) :
    summon_pet_t( n, p, sname, options_str )
  { }

  virtual void schedule_execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::schedule_execute();

    if ( p -> active_pet )
      p -> active_pet -> dismiss();
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> active_pet == pet )
      return false;

    return summon_pet_t::ready();
  }

  virtual double execute_time() const
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_soulburn -> up() )
      return 0.0;

    return warlock_spell_t::execute_time();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    summon_pet_t::execute();
    if ( p -> buffs_soulburn -> check() )
      p -> buffs_soulburn -> expire();
  }
};

struct summon_felhunter_t : public summon_main_pet_t
{
  summon_felhunter_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "felhunter", p, "Summon Felhunter", options_str )
  { }
};

struct summon_felguard_t : public summon_main_pet_t
{
  summon_felguard_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "felguard", p, "Summon Felguard", options_str )
  {
    check_talent( p -> talent_summon_felguard -> ok() );
  }
};

struct summon_succubus_t : public summon_main_pet_t
{
  summon_succubus_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "succubus", p, "Summon Succubus", options_str )
  { }
};

struct summon_imp_t : public summon_main_pet_t
{
  summon_imp_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "imp", p, "Summon Imp", options_str )
  { }
};

struct summon_voidwalker_t : public summon_main_pet_t
{
  summon_voidwalker_t( warlock_t* p, const std::string& options_str ) :
    summon_main_pet_t( "voidwalker", p, "Summon Voidwalker", options_str )
  { }
};

// Infernal Awakening =======================================================

struct infernal_awakening_t : public warlock_spell_t
{
  infernal_awakening_t( warlock_t* p ) :
    warlock_spell_t( "Infernal_Awakening", p, 22703 )
  {
    aoe        = -1;
    background = true;
    proc       = true;
    trigger_gcd= 0;
  }
};

// Summon Infernal Spell ====================================================

struct summon_infernal_t : public summon_pet_t
{
  infernal_awakening_t* infernal_awakening;

  summon_infernal_t( warlock_t* p, const std::string& options_str  ) :
    summon_pet_t( "infernal", p, "Summon Infernal", options_str ),
    infernal_awakening( 0 )
  {
    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 3 ) / 1000.0 : 0.0;

    summoning_duration = duration() + p -> talent_ancient_grimoire -> effect1().seconds();
    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ? 
      ( p -> talent_summon_felguard -> ok() ? 
        p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 1 ) : 
        p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 2 ) 
      ) : 0.0;
    infernal_awakening = new infernal_awakening_t( p );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    if ( infernal_awakening )
      infernal_awakening -> execute();

    p -> cooldowns_doomguard -> start();

    summon_pet_t::execute();
  }
};

// Summon Doomguard2 Spell ==================================================

struct summon_doomguard2_t : public summon_pet_t
{
  summon_doomguard2_t( warlock_t* p ) :
    summon_pet_t( "doomguard", p, 60478 )
  {
    harmful = false;
    background = true;
    summoning_duration = duration() + p -> talent_ancient_grimoire -> effect1().seconds();
    summoning_duration += ( p -> set_bonus.tier13_2pc_caster() ) ? 
      ( p -> talent_summon_felguard -> ok() ? 
        p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 1 ) : 
        p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 2 ) 
      ) : 0.0;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    p -> cooldowns_infernal -> start();

    summon_pet_t::execute();
  }
};

// Summon Doomguard Spell ===================================================

struct summon_doomguard_t : public warlock_spell_t
{
  summon_doomguard2_t* summon_doomguard2;

  summon_doomguard_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "summon_doomguard", p, "Summon Doomguard" ),
    summon_doomguard2( 0 )
  {
    parse_options( NULL, options_str );

    cooldown -> duration += ( p -> set_bonus.tier13_2pc_caster() ) ? p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 3 ) / 1000.0 : 0.0;

    harmful = false;
    summon_doomguard2 = new summon_doomguard2_t( p );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    consume_resource();
    update_ready();

    p -> cooldowns_infernal -> start();

    summon_doomguard2 -> execute();
  }
};

// Immolation Damage Spell ==================================================

struct immolation_damage_t : public warlock_spell_t
{
  immolation_damage_t( warlock_t* p ) :
    warlock_spell_t( "immolation_dmg", p, 50590 )
  {
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;
    may_crit    = false;

    stats = p -> get_stats( "immolation_aura", this );
  }
};

// Immolation Aura Spell ====================================================

struct immolation_aura_t : public warlock_spell_t
{
  immolation_damage_t* immolation_damage;

  immolation_aura_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "immolation_aura", p, "Immolation Aura" ),
    immolation_damage( 0 )
  {
    parse_options( NULL, options_str );

    harmful = true;
    tick_may_crit = false;
    immolation_damage = new immolation_damage_t( p );
  }

  virtual void tick( dot_t* d )
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_metamorphosis -> check() )
    {
      immolation_damage -> execute();
    }
    else
    {
      // Cancel the aura
      d -> current_tick = dot() -> num_ticks;
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! p -> buffs_metamorphosis -> check() )
      return false;

    return warlock_spell_t::ready();
  }
};

// Metamorphosis Spell ======================================================

struct metamorphosis_t : public warlock_spell_t
{
  metamorphosis_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "metamorphosis", p, "Metamorphosis" )
  {
    check_talent( p -> talent_metamorphosis -> rank() );

    parse_options( NULL, options_str );

    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    p -> buffs_metamorphosis -> trigger( 1, p -> composite_mastery() );
  }
};

// Demonic Empowerment Spell ================================================

struct demonic_empowerment_t : public warlock_spell_t
{
  demonic_empowerment_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "demonic_empowerment", p, "Demonic Empowerment" )
  {
    check_talent( p -> talent_demonic_empowerment -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    if ( p -> active_pet -> pet_type == PET_FELGUARD )
      p -> active_pet -> buffs.stunned -> expire();
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! p -> active_pet )
      return false;

    return warlock_spell_t::ready();
  }
};

// Hand of Gul'dan Spell ====================================================

struct hand_of_guldan_t : public warlock_spell_t
{
  hand_of_guldan_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "hand_of_guldan", p, "Hand of Gul'dan" )
  {
    check_talent( p -> talent_hand_of_guldan -> rank() );

    parse_options( NULL, options_str );

    base_execute_time *= 1 + p -> sets -> set ( SET_T11_2PC_CASTER ) -> effect_base_value( 1 ) * 0.01;

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new hand_of_guldan_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      p -> buffs_hand_of_guldan -> trigger();
      trigger_impending_doom( this );

      if ( td -> dots_immolate -> ticking && p -> talent_cremation -> rank() )
      {
        if ( p -> rng_cremation -> roll( p -> talent_cremation -> proc_chance() ) )
        {
          td -> dots_immolate -> refresh_duration();
        }
      }
    }
  }

  virtual double travel_time()
  {
    return 0.2;
  }
};

// Fel Flame Spell ==========================================================

struct fel_flame_t : public warlock_spell_t
{
  fel_flame_t( warlock_t* p, const std::string& options_str, bool dtr=false ) :
    warlock_spell_t( "fel_flame", p, "Fel Flame" )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && p -> has_dtr )
    {
      dtr_action = new fel_flame_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    p -> buffs_tier11_4pc_caster -> decrement();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_spell_t::impact( t, impact_result, travel_dmg );
    warlock_t* p = player -> cast_warlock();

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      td -> dots_immolate            -> extend_duration( 2, true );
      td -> dots_unstable_affliction -> extend_duration( 2, true );
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();

    if ( p -> buffs_tier11_4pc_caster -> up() )
    {
      player_multiplier *= 4;
    }
  }
};

// Dark Intent Spell ========================================================

struct dark_intent_t : public warlock_spell_t
{
  player_t* dark_intent_target;

  dark_intent_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "dark_intent", p, "Dark Intent" ),
    dark_intent_target( 0 )
  {
    std::string target_str = p -> dark_intent_target_str;
    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;

    if ( target_str.empty() )
    {
      dark_intent_target = p;
    }
    else
    {
      dark_intent_target = sim -> find_player( target_str );
      if ( ! dark_intent_target )
      {
        sim -> errorf( "%s Dark Intent Target: Can't find player %s, setting Dark Intent Target to %s.\n", player -> name(), target_str.c_str(), player -> name() );
        dark_intent_target = p;
      }
    }

    assert ( dark_intent_target != 0 );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    if ( dark_intent_target == p )
    {
      if ( sim -> log ) log_t::output( sim, "%s grants SomebodySomewhere Dark Intent", p -> name() );
      p -> buffs.dark_intent_feedback -> override( 3 );
      if ( p -> buffs.dark_intent -> check() ) p -> buffs.dark_intent -> expire();
      p -> buffs.dark_intent -> override( 1, 0.03 );
    }
    else
    {
      warlock_t* p = player -> cast_warlock();
      if ( sim -> log ) log_t::output( sim, "%s grants %s Dark Intent", p -> name(), dark_intent_target -> name() );
      dark_intent_target -> buffs.dark_intent -> trigger( 1, 0.01 );
      dark_intent_target -> dark_intent_cb -> active = true;

      p -> buffs.dark_intent -> trigger( 1, 0.03 );
      p -> dark_intent_cb -> active = true;
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( dark_intent_target == p )
    {
      if ( p -> buffs.dark_intent_feedback -> check() )
        return false;
    }
    else
    {
      if ( dark_intent_target -> buffs.dark_intent -> check() )
        return false;
    }

    return warlock_spell_t::ready();
  }
};

// Soulburn Spell ===========================================================

struct soulburn_t : public warlock_spell_t
{
  soulburn_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "soulburn", p, "Soulburn" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> use_pre_soulburn || p -> in_combat )
    {
      p -> buffs_soulburn -> trigger();
      p -> buffs_tier13_4pc_caster -> trigger();
      // If this was a pre-combat soulburn, ensure we model the 3 seconds needed to regenerate the soul shard
      if ( ! p -> in_combat )
      {
        p -> buffs_soulburn -> extend_duration( p, -3 );
        if ( p -> buffs_tier13_4pc_caster -> check() ) p -> buffs_tier13_4pc_caster -> extend_duration( p, -3 );
      }
    }

    warlock_spell_t::execute();
  }
};

// Demon Soul Spell =========================================================

struct demon_soul_t : public warlock_spell_t
{
  demon_soul_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "demon_soul", p, "Demon Soul" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    assert ( p -> active_pet );

    if ( p -> active_pet -> pet_type == PET_IMP )
    {
      p -> buffs_demon_soul_imp -> trigger();
    }
    if ( p -> active_pet -> pet_type == PET_FELGUARD )
    {
      p -> buffs_demon_soul_felguard -> trigger();
    }
    if ( p -> active_pet -> pet_type == PET_FELHUNTER )
    {
      p -> buffs_demon_soul_felhunter -> trigger();
    }
    if ( p -> active_pet -> pet_type == PET_SUCCUBUS )
    {
      p -> buffs_demon_soul_succubus -> trigger();
    }
    if ( p -> active_pet -> pet_type == PET_VOIDWALKER )
    {
      p -> buffs_demon_soul_voidwalker -> trigger();
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! p ->  active_pet )
      return false;

    return warlock_spell_t::ready();
  }
};

// Hellfire Effect Spell ====================================================

struct hellfire_tick_t : public warlock_spell_t
{
  hellfire_tick_t( warlock_t* p ) :
    warlock_spell_t( "hellfire_tick", p, 5857 )
  {
    dual        = true;
    background  = true;
    aoe         = -1;
    direct_tick = true;

    base_multiplier *= 1.0 + p -> talent_cremation -> effect1().percent();
    stats = p -> get_stats( "hellfire", this );
  }
};

// Hellfire Spell ===========================================================

struct hellfire_t : public warlock_spell_t
{
  hellfire_tick_t* hellfire_tick;

  hellfire_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "hellfire", p, 1949 ), hellfire_tick( 0 )
  {
    parse_options( NULL, options_str );

    // Hellfire has it's own damage effect, which is actually the damage to the player himself, so harmful is set to false.
    harmful = false;

    channeled    = true;
    hasted_ticks = false;

    hellfire_tick = new hellfire_tick_t( p );

    if ( p -> talent_inferno -> rank() )
    {
      range += p -> talent_inferno -> effect1().base_value();
    }
  }

  virtual bool usable_moving()
  {
    warlock_t* p = player -> cast_warlock();

    return p -> talent_inferno -> rank() > 0;
  }

  virtual void tick( dot_t* /* d */ )
  {
    hellfire_tick -> execute();
  }
};
// Seed of Corruption AOE Spell =============================================

struct seed_of_corruption_aoe_t : public warlock_spell_t
{
  seed_of_corruption_aoe_t( warlock_t* p ) :
    warlock_spell_t( "seed_of_corruption_aoe", p, 27285 )
  {
    proc       = true;
    background = true;
    aoe        = -1;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();

    if ( p -> buffs_soulburn -> check() && p -> talent_soulburn_seed_of_corruption -> rank() )
    {
      // Trigger Multiple Corruptions
      p -> buffs_soulburn -> expire();
    }
  }
};

// Seed of Corruption Spell =================================================

struct seed_of_corruption_t : public warlock_spell_t
{
  seed_of_corruption_aoe_t* seed_of_corruption_aoe;
  double dot_damage_done;

  seed_of_corruption_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "seed_of_corruption", p, "Seed of Corruption" ),
    seed_of_corruption_aoe( 0 ), dot_damage_done( 0 )
  {
    parse_options( NULL, options_str );

    seed_of_corruption_aoe = new seed_of_corruption_aoe_t( p );
    add_child( seed_of_corruption_aoe );

    base_crit += p -> talent_everlasting_affliction -> effect2().percent();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_targetdata_t* td = targetdata() -> cast_warlock();
    warlock_spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      warlock_targetdata_t* td = targetdata() -> cast_warlock();
      dot_damage_done = t -> iteration_dmg_taken;
      if ( td -> dots_corruption -> ticking )
      {
        td -> dots_corruption -> cancel();
      }
    }
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    if ( target -> iteration_dmg_taken - dot_damage_done > effect2().base_value() )
    {
      dot_damage_done=0.0;
      seed_of_corruption_aoe -> execute();
      spell_t::cancel();
    }
  }
};

// Rain of Fire Tick Spell ==================================================

struct rain_of_fire_tick_t : public warlock_spell_t
{
  rain_of_fire_tick_t( warlock_t* p ) :
    warlock_spell_t( "rain_of_fire_tick", p, 42223 )
  {
    background  = true;
    aoe         = -1;
    direct_tick = true;

    stats = p -> get_stats( "rain_of_fire", this );
  }
};

// Rain of Fire Spell =======================================================

struct rain_of_fire_t : public warlock_spell_t
{
  rain_of_fire_tick_t* rain_of_fire_tick;

  rain_of_fire_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "rain_of_fire", p, "Rain of Fire" ),
    rain_of_fire_tick( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;
    channeled = true;

    rain_of_fire_tick = new rain_of_fire_tick_t( p );

    add_child( rain_of_fire_tick );
  }

  virtual void tick( dot_t* d )
  {
    warlock_spell_t::tick( d );

    rain_of_fire_tick -> execute();
  }
};

// imp_pet_t::fire_bolt_t::execute ==========================================

void imp_pet_t::firebolt_t::impact( player_t* t, int impact_result, double travel_dmg )
{
  warlock_pet_spell_t::impact( t, impact_result, travel_dmg );
  warlock_t* o = player -> cast_pet() -> owner -> cast_warlock();

  if ( result_is_hit( impact_result ) )
  {
    if ( o -> buffs_empowered_imp -> trigger() ) o -> procs_empowered_imp -> occur();

    trigger_burning_embers ( this, travel_dmg );

    trigger_mana_feed ( this, impact_result );
  }
}

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Warlock Character Definition
// ==========================================================================

// warlock_t::composite_armor ===============================================

double warlock_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buffs_demon_armor -> up() )
    a += buffs_demon_armor ->effect1().base_value();

  return a;
}

// warlock_t::composite_spell_power =========================================

double warlock_t::composite_spell_power( const school_type school ) const
{
  double sp = player_t::composite_spell_power( school );

  sp += buffs_fel_armor -> value();

  return sp;
}

// warlock_t::composite_spell_power_multiplier ==============================

double warlock_t::composite_spell_power_multiplier() const
{
  double m = player_t::composite_spell_power_multiplier();

  if ( buffs_tier13_4pc_caster -> up() )
  {
    m *= 1.0 + dbc.spell( sets -> set ( SET_T13_4PC_CASTER ) -> effect_trigger_spell( 1 ) ) -> effect1().percent();
  }

  return m;
}

// warlock_t::composite_player_multiplier ===================================

double warlock_t::composite_player_multiplier( const school_type school, action_t* a ) const
{
  double player_multiplier = player_t::composite_player_multiplier( school, a );

  double mastery_value = mastery_spells.master_demonologist -> effect_base_value( 3 );

  if ( buffs_metamorphosis -> up() )
  {
    player_multiplier *= 1.0 + buffs_metamorphosis -> effect3().percent()
                         + ( buffs_metamorphosis -> value() * mastery_value / 10000.0 );
  }

  player_multiplier *= 1.0 + ( talent_demonic_pact -> effect3().percent() );

  if ( ( school == SCHOOL_FIRE || school == SCHOOL_SHADOW ) && buffs_demon_soul_felguard -> up() )
  {
    player_multiplier *= 1.0 + buffs_demon_soul_felguard -> effect2().percent();
  }

  double fire_multiplier   = 1.0;
  double shadow_multiplier = 1.0;

  // Fire
  fire_multiplier *= 1.0 + ( passive_spells.cataclysm -> effect_base_value( 1 ) * 0.01 );
  if ( mastery_spells.fiery_apocalypse -> ok() )
    fire_multiplier *= 1.0 + ( composite_mastery() * mastery_spells.fiery_apocalypse -> effect_base_value( 2 ) / 10000.0 );
  fire_multiplier *= 1.0 + passive_spells.demonic_knowledge -> effect_base_value( 1 ) * 0.01 ;

  // Shadow
  shadow_multiplier *= 1.0 + ( passive_spells.demonic_knowledge -> effect_base_value( 1 ) * 0.01 );
  shadow_multiplier *= 1.0 + ( passive_spells.shadow_mastery -> effect_base_value( 1 ) * 0.01 );

  if ( buffs_improved_soul_fire -> up() )
  {
    fire_multiplier *= 1.0 + talent_improved_soul_fire -> rank() * 0.04;
    shadow_multiplier *= 1.0 + talent_improved_soul_fire -> rank() * 0.04;
  }

  if ( buffs_tier12_4pc_caster -> up() )
  {
    double v = buffs_tier12_4pc_caster -> value();
    fire_multiplier *= 1.0 + v;
    shadow_multiplier *= 1.0 + v;
  }

  if ( school == SCHOOL_FIRE )
    player_multiplier *= fire_multiplier;
  else if ( school == SCHOOL_SHADOW )
    player_multiplier *= shadow_multiplier;
  else if ( school == SCHOOL_SHADOWFLAME )
  {
    if ( fire_multiplier > shadow_multiplier )
      player_multiplier *= fire_multiplier;
    else
      player_multiplier *= shadow_multiplier;
  }

  return player_multiplier;
}

// warlock_t::composite_player_td_multiplier ================================

double warlock_t::composite_player_td_multiplier( const school_type school, action_t* a ) const
{
  double player_multiplier = player_t::composite_player_td_multiplier( school, a );

  if ( school == SCHOOL_SHADOW || school == SCHOOL_SHADOWFLAME )
  {
    // Shadow TD
    if ( mastery_spells.potent_afflictions -> ok() )
    {
      player_multiplier += floor ( ( composite_mastery() * mastery_spells.potent_afflictions -> effect_base_value( 2 ) / 10000.0 ) * 1000 ) / 1000;
    }
    if ( buffs_demon_soul_felhunter -> up() )
    {
      player_multiplier += buffs_demon_soul_felhunter -> effect1().percent();
    }
  }

  return player_multiplier;
}

// warlock_t::matching_gear_multiplier ======================================

double warlock_t::matching_gear_multiplier( const attribute_type attr ) const
{
  if ( ( attr == ATTR_INTELLECT ) && passive_spells.nethermancy -> ok() )
    return ( passive_spells.nethermancy -> effect_base_value( 1 ) * 0.01 );

  return 0.0;
}

// warlock_t::create_action =================================================

action_t* warlock_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "chaos_bolt"          ) return new          chaos_bolt_t( this, options_str );
  if ( name == "conflagrate"         ) return new         conflagrate_t( this, options_str );
  if ( name == "corruption"          ) return new          corruption_t( this, options_str );
  if ( name == "bane_of_agony"       ) return new       bane_of_agony_t( this, options_str );
  if ( name == "bane_of_doom"        ) return new        bane_of_doom_t( this, options_str );
  if ( name == "curse_of_elements"   ) return new   curse_of_elements_t( this, options_str );
  if ( name == "death_coil"          ) return new          death_coil_t( this, options_str );
  if ( name == "demon_armor"         ) return new         demon_armor_t( this, options_str );
  if ( name == "demonic_empowerment" ) return new demonic_empowerment_t( this, options_str );
  if ( name == "drain_life"          ) return new          drain_life_t( this, options_str );
  if ( name == "drain_soul"          ) return new          drain_soul_t( this, options_str );
  if ( name == "fel_armor"           ) return new           fel_armor_t( this, options_str );
  if ( name == "haunt"               ) return new               haunt_t( this, options_str );
  if ( name == "immolate"            ) return new            immolate_t( this, options_str );
  if ( name == "immolation_aura"     ) return new     immolation_aura_t( this, options_str );
  if ( name == "shadowflame"         ) return new         shadowflame_t( this, options_str );
  if ( name == "incinerate"          ) return new          incinerate_t( this, options_str );
  if ( name == "life_tap"            ) return new            life_tap_t( this, options_str );
  if ( name == "metamorphosis"       ) return new       metamorphosis_t( this, options_str );
  if ( name == "shadow_bolt"         ) return new         shadow_bolt_t( this, options_str );
  if ( name == "shadowburn"          ) return new          shadowburn_t( this, options_str );
  if ( name == "shadowfury"          ) return new          shadowfury_t( this, options_str );
  if ( name == "searing_pain"        ) return new        searing_pain_t( this, options_str );
  if ( name == "soul_fire"           ) return new           soul_fire_t( this, options_str );
  if ( name == "summon_felhunter"    ) return new    summon_felhunter_t( this, options_str );
  if ( name == "summon_felguard"     ) return new     summon_felguard_t( this, options_str );
  if ( name == "summon_succubus"     ) return new     summon_succubus_t( this, options_str );
  if ( name == "summon_voidwalker"   ) return new   summon_voidwalker_t( this, options_str );
  if ( name == "summon_imp"          ) return new          summon_imp_t( this, options_str );
  if ( name == "summon_infernal"     ) return new     summon_infernal_t( this, options_str );
  if ( name == "summon_doomguard"    ) return new    summon_doomguard_t( this, options_str );
  if ( name == "unstable_affliction" ) return new unstable_affliction_t( this, options_str );
  if ( name == "hand_of_guldan"      ) return new      hand_of_guldan_t( this, options_str );
  if ( name == "fel_flame"           ) return new           fel_flame_t( this, options_str );
  if ( name == "dark_intent"         ) return new         dark_intent_t( this, options_str );
  if ( name == "soulburn"            ) return new            soulburn_t( this, options_str );
  if ( name == "demon_soul"          ) return new          demon_soul_t( this, options_str );
  if ( name == "bane_of_havoc"       ) return new       bane_of_havoc_t( this, options_str );
  if ( name == "hellfire"            ) return new            hellfire_t( this, options_str );
  if ( name == "seed_of_corruption"  ) return new  seed_of_corruption_t( this, options_str );
  if ( name == "rain_of_fire"        ) return new        rain_of_fire_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// warlock_t::create_pet ====================================================

pet_t* warlock_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "felguard"     ) return new    felguard_pet_t( sim, this );
  if ( pet_name == "felhunter"    ) return new   felhunter_pet_t( sim, this );
  if ( pet_name == "imp"          ) return new         imp_pet_t( sim, this );
  if ( pet_name == "succubus"     ) return new    succubus_pet_t( sim, this );
  if ( pet_name == "voidwalker"   ) return new  voidwalker_pet_t( sim, this );
  if ( pet_name == "infernal"     ) return new    infernal_pet_t( sim, this );
  if ( pet_name == "doomguard"    ) return new   doomguard_pet_t( sim, this );
  if ( pet_name == "ebon_imp"     ) return new    ebon_imp_pet_t( sim, this );
  if ( pet_name == "fiery_imp"    ) return new   fiery_imp_pet_t( sim, this );

  return 0;
}

// warlock_t::create_pets ===================================================

void warlock_t::create_pets()
{
  create_pet( "felguard"  );
  create_pet( "felhunter" );
  create_pet( "imp"       );
  create_pet( "succubus"  );
  create_pet( "voidwalker" );
  create_pet( "infernal"  );
  create_pet( "doomguard" );
  pet_ebon_imp = create_pet( "ebon_imp"  );
  pet_fiery_imp = create_pet( "fiery_imp" );
}

// warlock_t::init_talents ==================================================

void warlock_t::init_talents()
{
  // Affliction
  talent_unstable_affliction          = new passive_spell_t ( this, "unstable_affliction", "Unstable Affliction" );
  talent_doom_and_gloom               = find_talent( "Doom and Gloom" );
  talent_improved_life_tap            = find_talent( "Improved Life Tap" );
  talent_improved_corruption          = find_talent( "Improved Corruption" );
  talent_jinx                         = find_talent( "Jinx" );
  talent_soul_siphon                  = find_talent( "Soul Siphon" );
  talent_siphon_life                  = find_talent( "Siphon Life" );
  talent_eradication                  = find_talent( "Eradication" );
  talent_soul_swap                    = find_talent( "Soul Swap" );
  talent_shadow_embrace               = find_talent( "Shadow Embrace" );
  talent_deaths_embrace               = find_talent( "Death's Embrace" );
  talent_nightfall                    = find_talent( "Nightfall" );
  talent_soulburn_seed_of_corruption  = find_talent( "Soulburn: Seed of Corruption" );
  talent_everlasting_affliction       = find_talent( "Everlasting Affliction" );
  talent_pandemic                     = find_talent( "Pandemic" );
  talent_haunt                        = find_talent( "Haunt" );

  // Demonology
  talent_summon_felguard      = new passive_spell_t ( this, "summon_felguard", "Summon Felguard" );
  talent_demonic_embrace      = find_talent( "Demonic Embrace" );
  talent_dark_arts            = find_talent( "Dark Arts" );
  talent_mana_feed            = find_talent( "Mana Feed" );
  talent_demonic_aegis        = find_talent( "Demonic Aegis" );
  talent_master_summoner      = find_talent( "Master Summoner" );
  talent_impending_doom       = find_talent( "Impending Doom" );
  talent_demonic_empowerment  = find_talent( "Demonic Empowerment" );
  talent_molten_core          = find_talent( "Molten Core" );
  talent_hand_of_guldan       = find_talent( "Hand of Gul'dan" );
  talent_aura_of_foreboding   = find_talent( "Aura of Foreboding" );
  talent_ancient_grimoire     = find_talent( "Ancient Grimoire" );
  talent_inferno              = find_talent( "Inferno" );
  talent_decimation           = find_talent( "Decimation" );
  talent_cremation            = find_talent( "Cremation" );
  talent_demonic_pact         = find_talent( "Demonic Pact" );
  talent_metamorphosis        = find_talent( "Metamorphosis" );

  // Destruction
  talent_conflagrate            = new passive_spell_t ( this, "conflagrate", "Conflagrate" );
  talent_bane                   = find_talent( "Bane" );
  talent_shadow_and_flame       = find_talent( "Shadow and Flame" );
  talent_improved_immolate      = find_talent( "Improved Immolate" );
  talent_improved_soul_fire     = find_talent( "Improved Soul Fire" );
  talent_emberstorm             = find_talent( "Emberstorm" );
  talent_improved_searing_pain  = find_talent( "Improved Searing Pain" );
  talent_backdraft              = find_talent( "Backdraft" );
  talent_shadowburn             = find_talent( "Shadowburn" );
  talent_burning_embers         = find_talent( "Burning Embers" );
  talent_soul_leech             = find_talent( "Soul Leech" );
  talent_fire_and_brimstone     = find_talent( "Fire and Brimstone" );
  talent_shadowfury             = find_talent( "Shadowfury" );
  talent_empowered_imp          = find_talent( "Empowered Imp" );
  talent_bane_of_havoc          = find_talent( "Bane of Havoc" );
  talent_chaos_bolt             = find_talent( "Chaos Bolt" );

  player_t::init_talents();
}

// warlock_t::init_spells ===================================================

void warlock_t::init_spells()
{
  player_t::init_spells();

  // New set bonus system
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    {  89934,  89935,     0,     0,     0,     0,     0,     0 }, // Tier11
    {  99220,  99229,     0,     0,     0,     0,     0,     0 }, // Tier12
    { 105888, 105787,     0,     0,     0,     0,     0,     0 }, // Tier13
    {      0,      0,     0,     0,     0,     0,     0,     0 },
  };
  sets                        = new set_bonus_array_t( this, set_bonuses );

  // passive_spells =========================================================

  // Core
  passive_spells.shadow_mastery       = new passive_spell_t( this, "shadow_mastery", "Shadow Mastery" );
  passive_spells.demonic_knowledge    = new passive_spell_t( this, "demonic_knowledge", "Demonic Knowledge" );
  passive_spells.cataclysm            = new passive_spell_t( this, "cataclysm", "Cataclysm" );
  passive_spells.nethermancy          = new passive_spell_t( this, "nethermancy", 86091 );
  //Affliction
  passive_spells.doom_and_gloom       = new passive_spell_t( this, "doom_and_gloom", "Doom and Gloom", talent_doom_and_gloom );
  passive_spells.pandemic             = new passive_spell_t( this, "pandemic", "Pandemic", talent_pandemic );

  // Mastery
  mastery_spells.fiery_apocalypse     = new mastery_t( this, "fiery_apocalypse", "Fiery Apocalypse", TREE_DESTRUCTION );
  mastery_spells.potent_afflictions   = new mastery_t( this, "potent_afflictions", "Potent Afflictions", TREE_AFFLICTION );
  mastery_spells.master_demonologist  = new mastery_t( this, "master_demonologist", "Master Demonologist", TREE_DEMONOLOGY );

  // Constants
  constants_pandemic_gcd              = 0.25;

  // Prime
  glyphs.metamorphosis        = find_glyph( "Glyph of Metamorphosis" );
  glyphs.conflagrate          = find_glyph( "Glyph of Conflagrate" );
  glyphs.chaos_bolt           = find_glyph( "Glyph of Chaos Bolt" );
  glyphs.corruption           = find_glyph( "Glyph of Corruption" );
  glyphs.bane_of_agony        = find_glyph( "Glyph of Bane of Agony" );
  glyphs.felguard             = find_glyph( "Glyph of Felguard" );
  glyphs.haunt                = find_glyph( "Glyph of Haunt" );
  glyphs.immolate             = find_glyph( "Glyph of Immolate" );
  glyphs.imp                  = find_glyph( "Glyph of Imp" );
  glyphs.lash_of_pain         = find_glyph( "Glyph of Lash of Pain" );
  glyphs.incinerate           = find_glyph( "Glyph of Incinerate" );
  glyphs.shadowburn           = find_glyph( "Glyph of Shadowburn" );
  glyphs.unstable_affliction  = find_glyph( "Glyph of Unstable Affliction" );

  // Major
  glyphs.life_tap             = find_glyph( "Glyph of Life Tap" );
  glyphs.shadow_bolt          = find_glyph( "Glyph of Shadow Bolt" );

  tier12_4pc_caster           = spell_data_t::find( 99232, "Apocalypse", dbc.ptr );
}

// warlock_t::init_base =====================================================

void warlock_t::init_base()
{
  player_t::init_base();

  attribute_multiplier_initial[ ATTR_STAMINA ] *= 1.0 + talent_demonic_embrace -> effect1().percent();

  base_attack_power = -10;
  initial_attack_power_per_strength = 2.0;
  initial_spell_power_per_intellect = 1.0;

  mana_per_intellect = 15;

  resource_base[ RESOURCE_SOUL_SHARDS ] = 3;

  diminished_kfactor    = 0.009830;
  diminished_dodge_capi = 0.006650;
  diminished_parry_capi = 0.006650;
}

// warlock_t::init_scaling ==================================================

void warlock_t::init_scaling()
{
  player_t::init_scaling();
  scales_with[ STAT_SPIRIT ] = 0;
  scales_with[ STAT_STAMINA ] = 0;
}

// warlock_t::init_buffs ====================================================

void warlock_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )

  buffs_backdraft             = new buff_t( this, talent_backdraft -> effect_trigger_spell( 1 ), "backdraft" );
  buffs_decimation            = new buff_t( this, talent_decimation -> effect_trigger_spell( 1 ), "decimation", talent_decimation -> ok() );
  buffs_demon_armor           = new buff_t( this,   687, "demon_armor" );
  buffs_demonic_empowerment   = new buff_t( this, "demonic_empowerment",   1 );
  buffs_empowered_imp         = new buff_t( this, 47283, "empowered_imp", talent_empowered_imp -> effect1().percent() );
  buffs_eradication           = new buff_t( this, talent_eradication -> effect_trigger_spell( 1 ), "eradication", talent_eradication -> proc_chance() );
  buffs_metamorphosis         = new buff_t( this, 47241, "metamorphosis", talent_metamorphosis -> rank() );
  buffs_metamorphosis -> buff_duration += glyphs.metamorphosis -> base_value() / 1000.0;
  buffs_metamorphosis -> cooldown -> duration = 0;
  buffs_molten_core           = new buff_t( this, talent_molten_core -> effect_trigger_spell( 1 ), "molten_core", talent_molten_core -> rank() * 0.02 );
  buffs_shadow_trance         = new buff_t( this, 17941, "shadow_trance", talent_nightfall -> proc_chance() +  glyphs.corruption -> base_value() / 100.0 );

  buffs_hand_of_guldan        = new buff_t( this, "hand_of_guldan",        1, 15.0, 0.0, talent_hand_of_guldan -> rank() );
  buffs_improved_soul_fire    = new buff_t( this, 85383, "improved_soul_fire", ( talent_improved_soul_fire -> rank() > 0 ) );
  buffs_soulburn              = new buff_t( this, 74434, "soulburn" );
  buffs_demon_soul_imp        = new buff_t( this, 79459, "demon_soul_imp" );
  buffs_demon_soul_imp        -> activated = false;
  buffs_demon_soul_felguard   = new buff_t( this, 79462, "demon_soul_felguard" );
  buffs_demon_soul_felguard   -> activated = false;
  buffs_demon_soul_felhunter  = new buff_t( this, 79460, "demon_soul_felhunter" );
  buffs_demon_soul_felhunter  -> activated = false;
  buffs_demon_soul_succubus   = new buff_t( this, 79463, "demon_soul_succubus" );
  buffs_demon_soul_succubus   -> activated = false;
  buffs_demon_soul_voidwalker = new buff_t( this, 79464, "demon_soul_voidwalker" );
  buffs_demon_soul_voidwalker -> activated = false;
  buffs_bane_of_havoc         = new buff_t( this, 80240, "bane_of_havoc" );
  buffs_searing_pain_soulburn = new buff_t( this, 79440, "searing_pain_soulburn" );
  buffs_fel_armor             = new buff_t( this, "fel_armor", "Fel Armor" );
  buffs_tier11_4pc_caster     = new buff_t( this, sets -> set ( SET_T11_4PC_CASTER ) -> effect_trigger_spell( 1 ), "tier11_4pc_caster", sets -> set ( SET_T11_4PC_CASTER ) -> proc_chance() );
  buffs_tier12_4pc_caster     = new buff_t( this, sets -> set ( SET_T12_4PC_CASTER ) -> effect_trigger_spell( 1 ), "tier12_4pc_caster", sets -> set ( SET_T12_4PC_CASTER ) -> proc_chance() );
  buffs_tier13_4pc_caster     = new buff_t( this, sets -> set ( SET_T13_4PC_CASTER ) -> effect_trigger_spell( 1 ), "tier13_4pc_caster", sets -> set ( SET_T13_4PC_CASTER ) -> proc_chance() );
}

// warlock_t::init_values ======================================================

void warlock_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 90;
}

// warlock_t::init_gains ====================================================

void warlock_t::init_gains()
{
  player_t::init_gains();

  gains_fel_armor         = get_gain( "fel_armor"         );
  gains_felhunter         = get_gain( "felhunter"         );
  gains_life_tap          = get_gain( "life_tap"          );
  gains_mana_feed         = get_gain( "mana_feed"         );
  gains_soul_leech        = get_gain( "soul_leech"        );
  gains_soul_leech_health = get_gain( "soul_leech_health" );
  gains_tier13_4pc        = get_gain( "tier13_4pc"        );
}

// warlock_t::init_uptimes ==================================================

void warlock_t::init_benefits()
{
  player_t::init_benefits();

  uptimes_backdraft[ 0 ]  = get_benefit( "backdraft_0" );
  uptimes_backdraft[ 1 ]  = get_benefit( "backdraft_1" );
  uptimes_backdraft[ 2 ]  = get_benefit( "backdraft_2" );
  uptimes_backdraft[ 3 ]  = get_benefit( "backdraft_3" );
}

// warlock_t::init_procs ====================================================

void warlock_t::init_procs()
{
  player_t::init_procs();

  procs_ebon_imp         = get_proc( "ebon_imp"       );
  procs_empowered_imp    = get_proc( "empowered_imp"  );
  procs_fiery_imp        = get_proc( "fiery_imp"      );
  procs_impending_doom   = get_proc( "impending_doom" );
  procs_shadow_trance    = get_proc( "shadow_trance"  );
}

// warlock_t::init_rng ======================================================

void warlock_t::init_rng()
{
  player_t::init_rng();

  rng_cremation               = get_rng( "cremation"              );
  rng_ebon_imp                = get_rng( "ebon_imp_proc"          );
  rng_everlasting_affliction  = get_rng( "everlasting_affliction" );
  rng_fiery_imp               = get_rng( "fiery_imp_proc"         );
  rng_impending_doom          = get_rng( "impending_doom"         );
  rng_pandemic                = get_rng( "pandemic"               );
  rng_siphon_life             = get_rng( "siphon_life"            );
  rng_soul_leech              = get_rng( "soul_leech"             );
}

// warlock_t::init_actions ==================================================

void warlock_t::init_actions()
{
  if ( action_list_str.empty() )
  {

    // Trinket check
    bool has_mwc = false;
    bool has_wou = false;
    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ i ];
      if ( strstr( item.name(), "moonwell_chalice" ) )
      {
        has_mwc = true;
      }
      if ( strstr( item.name(), "will_of_unbinding" ) )
      {
        has_wou = true;
      }
    }

    // Flask
    if ( level >= 80 )
      action_list_str = "flask,type=draconic_mind";
    else if ( level >= 75 )
      action_list_str = "flask,type=frost_wyrm";

    // Food
    if ( level >= 80 ) action_list_str += "/food,type=seafood_magnifique_feast";
    else if ( level >= 70 ) action_list_str += "/food,type=fish_feast";

    // Armor
    if ( level >= 62 ) action_list_str += "/fel_armor";

    // Choose Pet
    if ( primary_tree() == TREE_DESTRUCTION )
    {
      action_list_str += "/summon_imp";
    }
    else if ( primary_tree() == TREE_DEMONOLOGY )
    {
      if ( has_mwc )
        action_list_str += "/summon_felguard,if=cooldown.demon_soul.remains<5&cooldown.metamorphosis.remains<5&!pet.felguard.active";
      else
        action_list_str += "/summon_felguard,if=!in_combat";
    }
    else if ( primary_tree() == TREE_AFFLICTION )
    {
      if ( glyphs.lash_of_pain -> ok() )
        action_list_str += "/summon_succubus";
      else if ( glyphs.imp -> ok() )
        action_list_str += "/summon_imp";
      else
        action_list_str += "/summon_felhunter";
    }
    else
      action_list_str += "/summon_imp";

    // Dark Intent
    if ( level >= 83 )
      action_list_str += "/dark_intent";

    // Pre soulburn
    if ( use_pre_soulburn && !set_bonus.tier13_4pc_caster() )
      action_list_str += "/soulburn,if=!in_combat";

    // Snapshot Stats
    action_list_str += "/snapshot_stats";

    // Usable Item
    int num_items = ( int ) items.size();
    for ( int i = num_items - 1; i >= 0; i-- )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
        if ( primary_tree() == TREE_DEMONOLOGY )
          action_list_str += ",if=cooldown.metamorphosis.remains=0|cooldown.metamorphosis.remains>cooldown";
      }
    }

    action_list_str += init_use_profession_actions();
    action_list_str += init_use_racial_actions();

    // Choose Potion
    if ( level >= 80 )
    {
      if ( primary_tree() == TREE_DEMONOLOGY )
        action_list_str += "/volcanic_potion,if=buff.metamorphosis.up|!in_combat";
      else
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react|!in_combat|target.health_pct<=20";
    }
    else if ( level >= 70 )
    {
      if ( primary_tree() == TREE_AFFLICTION )
        action_list_str += "/speed_potion,if=buff.bloodlust.react|!in_combat";
      else
        action_list_str += "/wild_magic_potion,if=buff.bloodlust.react|!in_combat";
    }

    switch ( primary_tree() )
    {

    case TREE_AFFLICTION:
      if ( level >= 85 && ! glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soulburn";
      action_list_str += "/corruption,if=(!ticking|remains<tick_time)&miss_react";
      action_list_str += "/unstable_affliction,if=(!ticking|remains<(cast_time+tick_time))&target.time_to_die>=5&miss_react";
      if ( level >= 12 ) action_list_str += "/bane_of_doom,if=target.time_to_die>15&!ticking&miss_react";
      if ( talent_haunt -> rank() ) action_list_str += "/haunt";
      if ( level >= 81 && set_bonus.tier11_4pc_caster() ) action_list_str += "/fel_flame,if=buff.tier11_4pc_caster.react&dot.unstable_affliction.remains<8";
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      if ( talent_soul_siphon -> rank() )
      {
        action_list_str += "/drain_soul,interrupt=1,if=target.health_pct<=25";
        // If the profile has the Will of Unbinding, we need to make sure the stacks don't drop during execute phase
        if ( has_wou )
        {
          // Attempt to account for non-default channel_lag settings
          char delay = (char) ( sim -> channel_lag * 20 + 48 );
          if ( delay > 57 ) delay = 57;
          action_list_str += ",interrupt_if=buff.will_of_unbinding.up&cooldown.haunt.remains<tick_time&buff.will_of_unbinding.remains<action.haunt.cast_time+tick_time+0.";
          action_list_str += delay;
        }
      }
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soul_fire,if=buff.soulburn.up";
      if ( talent_bane -> rank() == 3 )
      {
        action_list_str += "/life_tap,mana_percentage<=35";
        if ( ! set_bonus.tier13_4pc_caster() )
        {
          if ( glyphs.lash_of_pain -> ok() )
          {
            action_list_str += "/soulburn,if=buff.demon_soul_succubus.down";
          }
          else
          {
            action_list_str += "/soulburn,if=buff.demon_soul_felhunter.down";
          }
          action_list_str += "/soul_fire,if=buff.soulburn.up";
        }
        if ( level >= 85 && glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul";
        action_list_str += "/shadow_bolt";
      }
      else
      {
        if ( level >= 85 && glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul,if=buff.shadow_trance.react";
        action_list_str += "/shadow_bolt,if=buff.shadow_trance.react";
        action_list_str += "/life_tap,mana_percentage<=5";
        action_list_str += "/soulburn";
        action_list_str += "/drain_life";
      }

      break;

    case TREE_DESTRUCTION:
      if ( level >= 85 && ! glyphs.lash_of_pain -> ok() ) action_list_str += "/demon_soul";
      if ( set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/soulburn";
        action_list_str += "/soul_fire,if=buff.soulburn.up&!in_combat";
      }
      else
      {
        action_list_str += "/soulburn,if=buff.bloodlust.down";
        if ( talent_improved_soul_fire -> ok() && level >= 54 )
        {
          action_list_str += "/soul_fire,if=buff.soulburn.up&!in_combat";
        }
      }
      if ( level >= 81 && set_bonus.tier11_4pc_caster() ) action_list_str += "/fel_flame,if=buff.tier11_4pc_caster.react&dot.immolate.remains<8";
      action_list_str += "/immolate,if=(remains<cast_time+gcd|!ticking)&target.time_to_die>=4&miss_react";
      if ( talent_conflagrate -> ok() ) action_list_str += "/conflagrate";
      action_list_str += "/immolate,if=buff.bloodlust.react&buff.bloodlust.remains>32&cooldown.conflagrate.remains<=3&remains<12";
      if ( level >= 20 ) action_list_str += "/bane_of_doom,if=!ticking&target.time_to_die>=15&miss_react";
      action_list_str += "/corruption,if=(!ticking|dot.corruption.remains<tick_time)&miss_react";
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( talent_chaos_bolt -> ok() ) action_list_str += "/chaos_bolt,if=cast_time>0.9";
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      if ( set_bonus.tier13_4pc_caster() ) action_list_str += "/soul_fire,if=buff.soulburn.up";
      if ( talent_improved_soul_fire -> ok() && level >= 54 )
      {
        action_list_str += "/soul_fire,if=((buff.empowered_imp.react&buff.empowered_imp.remains<(buff.improved_soul_fire.remains+action.soul_fire.travel_time))|buff.improved_soul_fire.remains<(cast_time+travel_time+action.incinerate.cast_time+gcd))&!in_flight";
      }
      if ( talent_shadowburn -> ok() ) action_list_str += "/shadowburn";
      if ( level >= 64 ) action_list_str += "/incinerate"; else action_list_str += "/shadow_bolt";

      break;

    case TREE_DEMONOLOGY:
      if ( talent_metamorphosis -> ok() )
      {
        action_list_str += "/metamorphosis";
        if ( has_mwc ) action_list_str += ",if=buff.moonwell_chalice.up&pet.felguard.active";
      }
      if ( level >= 85 ) 
      {
        action_list_str += "/demon_soul";
        if ( has_mwc ) action_list_str += ",if=buff.metamorphosis.up";
      }
      if ( level >= 50 ) action_list_str += "/summon_doomguard,if=time>10";
      action_list_str += "/felguard:felstorm";
      action_list_str += "/soulburn,if=pet.felguard.active&!pet.felguard.dot.felstorm.ticking";
      action_list_str += "/summon_felhunter,if=!pet.felguard.dot.felstorm.ticking&pet.felguard.active";
      if ( set_bonus.tier13_4pc_caster() )
      {
        action_list_str += "/soulburn,if=pet.felhunter.active";
        if ( has_mwc ) action_list_str += "&cooldown.metamorphosis.remains>60";
        action_list_str += "/soul_fire,if=pet.felhunter.active&buff.soulburn.up";
        if ( has_mwc ) action_list_str += "&cooldown.metamorphosis.remains>60";
      }
      action_list_str += "/immolate,if=!ticking&target.time_to_die>=4&miss_react";
      if ( level >= 20 )
      {
        action_list_str += "/bane_of_doom,if=(!ticking";
        if ( talent_metamorphosis -> ok() )
          action_list_str += "|(buff.metamorphosis.up&remains<45)";
        action_list_str += ")&target.time_to_die>=15&miss_react";
      }
      action_list_str += "/corruption,if=(remains<tick_time|!ticking)&target.time_to_die>=6&miss_react";
      if ( level >= 81 && set_bonus.tier11_4pc_caster() ) action_list_str += "/fel_flame,if=buff.tier11_4pc_caster.react";
      if ( level >= 75 ) action_list_str += "/shadowflame";
      if ( talent_hand_of_guldan -> ok() ) action_list_str += "/hand_of_guldan";
      if ( level >= 60 ) action_list_str += "/immolation_aura,if=buff.metamorphosis.remains>10";
      if ( glyphs.corruption -> ok() ) action_list_str += "/shadow_bolt,if=buff.shadow_trance.react";
      if ( level >= 64 ) action_list_str += "/incinerate,if=buff.molten_core.react";
      if ( level >= 54 ) action_list_str += "/soul_fire,if=buff.decimation.up";
      action_list_str += "/life_tap,if=mana_pct<=30&buff.bloodlust.down&buff.metamorphosis.down&buff.demon_soul_felguard.down";
      action_list_str += ( talent_bane -> ok() ) ? "/shadow_bolt" : "/incinerate";

      break;

    default:
      action_list_str += "/bane_of_doom,if=(remains<3|!ticking)&target.time_to_die>=15&miss_react";
      action_list_str += "/corruption,if=(!ticking|remains<tick_time)&miss_react";
      action_list_str += "/immolate,if=(!ticking|remains<(cast_time+tick_time))&miss_react";
      if ( level >= 50 ) action_list_str += "/summon_infernal";
      if ( level >= 64 ) action_list_str += "/incinerate"; else action_list_str += "/shadow_bolt";
      if ( sim->debug ) log_t::output( sim, "Using generic action string for %s.", name() );
      break;
    }

    // Movement
    action_list_str += "/life_tap,moving=1,if=mana_pct<80&mana_pct<target.health_pct";
    action_list_str += "/fel_flame,moving=1";

    action_list_str += "/life_tap,if=mana_pct_nonproc<100"; // to use when no mana or nothing else is possible

    action_list_default = 1;
  }

  player_t::init_actions();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );
  if ( active_pet ) active_pet -> init_resources( force );
}

// warlock_t::reset =========================================================

void warlock_t::reset()
{
  player_t::reset();

  // Active
  active_pet = 0;
}

// warlock_t::create_expression =============================================

action_expr_t* warlock_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "shards" )
  {
    struct shards_expr_t : public action_expr_t
    {
      shards_expr_t( action_t* a ) : action_expr_t( a, "shards", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> cast_warlock() -> resource_current[ RESOURCE_SOUL_SHARDS ]; return TOK_NUM; }
    };
    return new shards_expr_t( a );
  }
  return player_t::create_expression( a, name_str );
}

// warlock_t::create_options ================================================

void warlock_t::create_options()
{
  player_t::create_options();

  option_t warlock_options[] =
  {
    { "use_pre_soulburn",    OPT_BOOL,   &( use_pre_soulburn       ) },
    { "dark_intent_target",  OPT_STRING, &( dark_intent_target_str ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, warlock_options );
}

// warlock_t::create_profile ================================================

bool warlock_t::create_profile( std::string& profile_str, int save_type, bool save_html )
{
  player_t::create_profile( profile_str, save_type, save_html );

  if ( save_type == SAVE_ALL )
  {
    if ( use_pre_soulburn ) profile_str += "use_pre_soulburn=1\n";
    if ( ! dark_intent_target_str.empty() ) profile_str += "dark_intent_target=" + dark_intent_target_str + "\n";
  }

  return true;
}

// warlock_t::copy_from =====================================================

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );
  warlock_t* p = source -> cast_warlock();
  dark_intent_target_str = p -> dark_intent_target_str;
  use_pre_soulburn       = p -> use_pre_soulburn;
}

// warlock_t::decode_set ====================================================

int warlock_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "shadowflame"             ) ) return SET_T11_CASTER;
  if ( strstr( s, "balespiders"             ) ) return SET_T12_CASTER;
  if ( strstr( s, "_of_the_faceless_shroud" ) ) return SET_T13_CASTER;

  if ( strstr( s, "_gladiators_felweave_"   ) ) return SET_PVP_CASTER;

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_warlock =================================================

player_t* player_t::create_warlock( sim_t* sim, const std::string& name, race_type r )
{
  return new warlock_t( sim, name, r );
}

// player_t::warlock_init ===================================================

void player_t::warlock_init( sim_t* sim )
{
  sim -> auras.demonic_pact         = new aura_t( sim, "demonic_pact", 1 );
  sim -> auras.fel_intelligence     = new aura_t( sim, "fel_intelligence", 1 );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> debuffs.shadow_and_flame     = new     debuff_t( p, 17800, "shadow_and_flame" );
    p -> debuffs.curse_of_elements    = new coe_debuff_t( p );
  }
}

// player_t::warlock_combat_begin ===========================================

void player_t::warlock_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.demonic_pact     ) sim -> auras.demonic_pact     -> override();
  if ( sim -> overrides.fel_intelligence ) sim -> auras.fel_intelligence -> override();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( sim -> overrides.dark_intent && ! p -> is_pet() )
    {
      p -> buffs.dark_intent          -> override( 1, p -> type == WARLOCK ? 0.03 : 0.01 );
      p -> buffs.dark_intent_feedback -> override( 3 );
      p -> dark_intent_cb -> active = true;
    }
  }

  for ( player_t* t = sim -> target_list; t; t = t -> next )
  {
    if ( sim -> overrides.curse_of_elements ) t -> debuffs.curse_of_elements -> override( 1, 8 );
    if ( sim -> overrides.shadow_and_flame  ) t -> debuffs.shadow_and_flame  -> override();
  }
}

