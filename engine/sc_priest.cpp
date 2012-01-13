// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace {
struct remove_dots_event_t;
}

struct priest_targetdata_t : public targetdata_t
{
  dot_t*            dots_shadow_word_pain;
  dot_t*            dots_vampiric_touch;
  dot_t*            dots_devouring_plague;
  dot_t*            dots_holy_fire;
  dot_t*            dots_renew;

  remove_dots_event_t* remove_dots_event;

  priest_targetdata_t( player_t* source, player_t* target )
    : targetdata_t( source, target ), remove_dots_event( NULL )
  {
  }
};

void register_priest_targetdata( sim_t* sim )
{
  player_type t = PRIEST;
  typedef priest_targetdata_t type;

  REGISTER_DOT( devouring_plague );
  REGISTER_DOT( holy_fire );
  REGISTER_DOT( renew );
  REGISTER_DOT( shadow_word_pain );
  REGISTER_DOT( vampiric_touch );
}

// ==========================================================================
// Priest
// ==========================================================================

struct priest_t : public player_t
{
  // Buffs

  // Discipline
  buff_t* buffs_borrowed_time;
  buff_t* buffs_dark_evangelism;
  buff_t* buffs_holy_evangelism;
  buff_t* buffs_dark_archangel;
  buff_t* buffs_holy_archangel;
  buff_t* buffs_inner_fire;
  buff_t* buffs_inner_focus;
  buff_t* buffs_inner_will;

  // Holy
  buff_t* buffs_chakra_pre;
  buff_t* buffs_chakra_chastise;
  buff_t* buffs_chakra_sanctuary;
  buff_t* buffs_chakra_serenity;
  buff_t* buffs_serendipity;
  buff_t* buffs_serenity;
  buff_t* buffs_surge_of_light;

  // Shadow
  buff_t* buffs_empowered_shadow;
  buff_t* buffs_glyph_of_shadow_word_death;
  buff_t* buffs_mind_melt;
  buff_t* buffs_mind_spike;
  buff_t* buffs_shadow_form;
  buff_t* buffs_shadow_orb;
  buff_t* buffs_shadowfiend;
  buff_t* buffs_glyph_of_spirit_tap;
  buff_t* buffs_vampiric_embrace;

  // Set Bonus
  buff_t* buffs_indulgence_of_the_penitent;
  buff_t* buffs_divine_fire;
  buff_t* buffs_cauterizing_flame;
  buff_t* buffs_tier13_2pc_heal;

  // Talents
  struct talents_list_t
  {
    // Discipline
    talent_t* improved_power_word_shield;
    talent_t* twin_disciplines;
    talent_t* mental_agility;
    talent_t* evangelism;
    talent_t* archangel;
    talent_t* inner_sanctum;
    talent_t* soul_warding;
    talent_t* renewed_hope;
    talent_t* power_infusion;
    talent_t* atonement;
    talent_t* inner_focus;
    talent_t* rapture;
    talent_t* borrowed_time;
    talent_t* strength_of_soul;
    talent_t* divine_aegis;
    talent_t* pain_suppression;
    talent_t* train_of_thought;
    talent_t* grace;
    talent_t* power_word_barrier;

    // Holy
    talent_t* improved_renew;
    talent_t* empowered_healing;
    talent_t* divine_fury;
    talent_t* surge_of_light;
    talent_t* inspiration;
    talent_t* divine_touch;
    talent_t* holy_concentration;
    talent_t* lightwell;
    talent_t* tome_of_light;
    talent_t* rapid_renewal;
    talent_t* serendipity;
    talent_t* body_and_soul;
    talent_t* chakra;
    talent_t* revelations;
    talent_t* test_of_faith;
    talent_t* state_of_mind;
    talent_t* heavenly_voice;
    talent_t* circle_of_healing;
    talent_t* guardian_spirit;

    // Shadow
    talent_t* darkness;
    talent_t* improved_devouring_plague;
    talent_t* improved_mind_blast;
    talent_t* mind_melt;
    talent_t* dispersion;
    talent_t* improved_shadow_word_pain;
    talent_t* pain_and_suffering;
    talent_t* masochism;
    talent_t* shadow_form;
    talent_t* twisted_faith;
    talent_t* veiled_shadows;
    talent_t* harnessed_shadows;
    talent_t* shadowy_apparition;
    talent_t* vampiric_embrace;
    talent_t* vampiric_touch;
    talent_t* sin_and_punishment;
    talent_t* phantasm;
    talent_t* paralysis;
  };
  talents_list_t talents;

  // Passive Spells
  struct passive_spells_t
  {
    // Discipline
    passive_spell_t* enlightenment;
    passive_spell_t* meditation_disc;

    // Holy
    passive_spell_t* spiritual_healing;
    passive_spell_t* meditation_holy;

    // Shadow
    passive_spell_t* shadow_power;
    passive_spell_t* shadow_orbs;
    passive_spell_t* shadowy_apparition_num;
  };
  passive_spells_t passive_spells;

  // Mastery Spells
  struct mastery_spells_t
  {
    mastery_t* shield_discipline;
    mastery_t* echo_of_light;
    mastery_t* shadow_orb_power;
  };
  mastery_spells_t mastery_spells;

  // Active Spells
  struct active_spells_t
  {
    active_spell_t* mind_spike;
    active_spell_t* shadow_fiend;
    active_spell_t* holy_archangel;
    active_spell_t* holy_archangel2;
    active_spell_t* dark_archangel;
  };
  active_spells_t   active_spells;

  spell_data_t*     dark_flames;

  // Cooldowns
  cooldown_t*       cooldowns_mind_blast;
  cooldown_t*       cooldowns_shadow_fiend;
  cooldown_t*       cooldowns_chakra;
  cooldown_t*       cooldowns_rapture;
  cooldown_t*       cooldowns_inner_focus;
  cooldown_t*       cooldowns_penance;

  // Gains
  gain_t* gains_dispersion;
  gain_t* gains_shadow_fiend;
  gain_t* gains_archangel;
  gain_t* gains_masochism;
  gain_t* gains_rapture;
  gain_t* gains_hymn_of_hope;
  gain_t* gains_divine_fire;

  // Uptimes
  benefit_t* uptimes_mind_spike[ 4 ];
  benefit_t* uptimes_dark_flames;
  benefit_t* uptimes_shadow_orb[ 4 ];
  benefit_t* uptimes_test_of_faith;

  // Procs
  proc_t* procs_shadowy_apparation;
  proc_t* procs_surge_of_light;
  proc_t* procs_train_of_thought_gh;
  proc_t* procs_train_of_thought_smite;

  // Special
  std::queue<spell_t*> shadowy_apparition_free_list;
  std::list<spell_t*>  shadowy_apparition_active_list;
  heal_t* heals_echo_of_light;
  bool echo_of_light_merged;

  // Random Number Generators
  rng_t* rng_pain_and_suffering;
  rng_t* rng_cauterizing_flame;
  rng_t* rng_train_of_thought;
  rng_t* rng_tier13_4pc_heal;

  // Pets
  pet_t* pet_shadow_fiend;
  pet_t* pet_cauterizing_flame;
  pet_t* pet_lightwell;

  // Options
  std::string atonement_target_str;

  // Mana Resource Tracker
  struct mana_resource_t
  {
    double mana_gain;
    double mana_loss;
    void reset() { memset( ( void* ) this, 0x00, sizeof( mana_resource_t ) ); }

    mana_resource_t() { reset(); }
  };

  mana_resource_t mana_resource;
  double max_mana_cost;
  std::vector<player_t *> party_list;

  // Glyphs
  struct glyphs_t
  {
    glyph_t* circle_of_healing;
    glyph_t* dispersion;
    glyph_t* divine_accuracy;
    glyph_t* guardian_spirit;
    glyph_t* holy_nova;
    glyph_t* inner_fire;
    glyph_t* lightwell;
    glyph_t* mind_flay;
    glyph_t* penance;
    glyph_t* power_word_shield;
    glyph_t* prayer_of_healing;
    glyph_t* prayer_of_mending;
    glyph_t* renew;
    glyph_t* shadow_word_death;
    glyph_t* shadow_word_pain;
    glyph_t* smite;
    glyph_t* spirit_tap;
  };
  glyphs_t glyphs;

  // Constants
  struct constants_t
  {
    double meditation_value;

    // Discipline
    double twin_disciplines_value;
    double dark_archangel_damage_value;
    double dark_archangel_mana_value;
    double holy_archangel_value;
    double archangel_mana_value;
    double inner_will_value;
    double borrowed_time_value;

    // Holy
    double holy_concentration_value;

    // Shadow
    double shadow_power_damage_value;
    double shadow_power_crit_value;
    double shadow_orb_proc_value;
    double shadow_orb_damage_value;
    double shadow_orb_mastery_value;

    double darkness_value;
    double improved_shadow_word_pain_value;
    double twisted_faith_static_value;
    double twisted_faith_dynamic_value;
    double shadow_form_value;
    double harnessed_shadows_value;
    double pain_and_suffering_value;

    double devouring_plague_health_mod;

    uint32_t max_shadowy_apparitions;

    constants_t() { memset( ( void * ) this, 0x0, sizeof( constants_t ) ); }
  };
  constants_t constants;

  // Power Mods
  struct power_mod_t
  {
    double devouring_plague;
    double mind_blast;
    double mind_flay;
    double mind_spike;
    double shadow_word_death;
    double shadow_word_pain;
    double vampiric_touch;
    double holy_fire;
    double smite;

    power_mod_t() { memset( ( void * ) this, 0x0, sizeof( power_mod_t ) ); }
  };
  power_mod_t power_mod;

  bool   was_sub_25;

  priest_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, PRIEST, name, r )
  {
    if ( race == RACE_NONE ) race = RACE_NIGHT_ELF;

    tree_type[ PRIEST_DISCIPLINE ] = TREE_DISCIPLINE;
    tree_type[ PRIEST_HOLY       ] = TREE_HOLY;
    tree_type[ PRIEST_SHADOW     ] = TREE_SHADOW;

    was_sub_25                           = false;
    echo_of_light_merged                 = false;

    heals_echo_of_light                  = NULL;

    distance                             = 40.0;
    default_distance                     = 40.0;

    max_mana_cost                        = 0.0;

    cooldowns_mind_blast                 = get_cooldown( "mind_blast" );
    cooldowns_shadow_fiend               = get_cooldown( "shadow_fiend" );
    cooldowns_chakra                     = get_cooldown( "chakra"   );
    cooldowns_rapture                    = get_cooldown( "rapture" );
    cooldowns_rapture -> duration        = dbc.spell( 63853 ) -> duration();
    cooldowns_inner_focus                = get_cooldown( "inner_focus" );
    cooldowns_penance                    = get_cooldown( "penance" );

    pet_shadow_fiend                     = NULL;
    pet_cauterizing_flame                = NULL;
    pet_lightwell                        = NULL;

    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target ) {return new priest_targetdata_t( source, target );}
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_actions();
  virtual void      init_procs();
  virtual void      init_scaling();
  virtual void      reset();
  virtual void      demise();
  virtual void      init_party();
  virtual void      create_options();
  virtual bool      create_profile( std::string& profile_str, int save_type=SAVE_ALL, bool save_html=false );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const { return RESOURCE_MANA; }
  virtual int       primary_role() const;
  virtual double    composite_armor() const;
  virtual double    composite_spell_power( const school_type school ) const;
  virtual double    composite_spell_hit() const;
  virtual double    composite_spell_haste() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    composite_player_td_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    composite_player_heal_multiplier( const school_type school ) const;
  virtual double    composite_movement_speed() const;

  virtual double    matching_gear_multiplier( const attribute_type attr ) const;

  virtual double    spirit() const;

  virtual double    resource_gain( int resource, double amount, gain_t* source=0, action_t* action=0 );
  virtual double    resource_loss( int resource, double amount, action_t* action=0 );

  virtual double    target_mitigation( double amount, const school_type school, int type, int result, action_t* a=0 );

  virtual double    empowered_shadows_amount() const;
  virtual double    shadow_orb_amount() const;

  void fixup_atonement_stats( const char* trigger_spell_name, const char* atonement_spell_name );
  virtual void pre_analyze_hook();

  void trigger_cauterizing_flame();
};


namespace   // ANONYMOUS NAMESPACE ==========================================
{

struct remove_dots_event_t : public event_t
{
private:
  priest_targetdata_t* td;

  static void cancel_dot( dot_t* dot )
  {
    if ( dot -> ticking )
    {
      dot -> cancel();
      dot -> reset();
    }
  }

public:
  remove_dots_event_t( sim_t* sim, priest_t* pr, priest_targetdata_t* td ) : event_t( sim, pr, "mind_spike_remove_dots" ), td( td )
  { sim -> add_event( this, sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev ) ); }

  virtual void execute()
  {
    td -> remove_dots_event = 0;
    cancel_dot( td -> dots_shadow_word_pain );
    cancel_dot( td -> dots_vampiric_touch );
    cancel_dot( td -> dots_devouring_plague );
  }
};

// ==========================================================================
// Priest Absorb
// ==========================================================================

struct priest_absorb_t : public absorb_t
{
  cooldown_t* min_interval;

private:
  void _init_priest_absorb_t()
  {
    may_crit          = false;
    tick_may_crit     = false;
    may_miss          = false;
    may_resist        = false;
    min_interval      = player -> get_cooldown( "min_interval_" + name_str );
  }

public:
  priest_absorb_t( const char* n, player_t* player, const char* sname, int t = TREE_NONE ) :
    absorb_t( n, player, sname, t )
  {
    _init_priest_absorb_t();
  }

  priest_absorb_t( const char* n, player_t* player, const uint32_t id, int t = TREE_NONE ) :
    absorb_t( n, player, id, t )
  {
    _init_priest_absorb_t();
  }

  virtual void player_buff()
  {
    absorb_t::player_buff();

    priest_t* p = player -> cast_priest();

    player_multiplier *= 1.0 + ( p -> composite_mastery() * p -> mastery_spells.shield_discipline -> ok() * 2.5 / 100.0 );
  }

  virtual double cost() const
  {
    priest_t* p = player -> cast_priest();

    double c = absorb_t::cost();

    if ( base_execute_time <= timespan_t::zero )
    {
      c *= 1.0 - p -> buffs_inner_will -> check() * p -> buffs_inner_will -> effect1().percent();
      c  = floor( c );
    }

    // Needs testing
    if ( p -> buffs_tier13_2pc_heal -> check() )
    {
      c *= 1.0 + p -> buffs_tier13_2pc_heal -> effect1().percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    absorb_t::consume_resource();

    priest_t* p = player -> cast_priest();

    if ( base_execute_time <= timespan_t::zero )
      p -> buffs_inner_will -> up();
    // Needs testing
    p -> buffs_tier13_2pc_heal -> up();
  }

  virtual void execute()
  {
    absorb_t::execute();

    priest_t* p = player -> cast_priest();

    if ( execute_time() > timespan_t::zero )
      p -> buffs_borrowed_time -> expire();

    if ( ! ( background || proc ) )
      p -> trigger_cauterizing_flame();
  }

  bool ready()
  {
    if ( min_interval -> remains() > timespan_t::zero )
      return false;

    return absorb_t::ready();
  }

  void parse_options( option_t*          options,
                      const std::string& options_str )
  {
    const option_t base_options[] =
    {
      { "min_interval", OPT_TIMESPAN,     &( min_interval -> duration ) },
      { NULL,           OPT_UNKNOWN, NULL      }
    };

    std::vector<option_t> merged_options;
    absorb_t::parse_options( option_t::merge( merged_options, options, base_options ), options_str );
  }

  void update_ready()
  {
    absorb_t::update_ready();

    if ( min_interval -> duration > timespan_t::zero && ! dual )
    {
      min_interval -> start( timespan_t::min, timespan_t::zero );

      if ( sim -> debug ) log_t::output( sim, "%s starts min_interval for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready.total_seconds() );
    }
  }
};

// ==========================================================================
// Priest Heal
// ==========================================================================

struct priest_heal_t : public heal_t
{
  struct divine_aegis_t : public priest_absorb_t
  {
    double shield_multiple;

    divine_aegis_t( const char* n, priest_t* p ) :
      priest_absorb_t( n, p, 47753 ), shield_multiple( 0 )
    {
      check_talent( p -> talents.divine_aegis -> rank() );

      proc             = true;
      background       = true;
      direct_power_mod = 0;

      shield_multiple  = p -> talents.divine_aegis -> effect1().percent();
    }

    virtual void impact( player_t* t, int impact_result, double travel_dmg )
    {
      // Get DA buff:
      buff_t* buff_da = 0;
      for ( unsigned int i = 0; i < t -> buffs.divine_aegis.size(); i++ )
      {
        if ( t -> buffs.divine_aegis[i ] -> initial_source == player )
        {
          buff_da = t -> buffs.divine_aegis[i ];
          break;
        }
      }
      assert( buff_da );

      double old_amount = buff_da -> current_value;
      double new_amount = std::min( t -> resource_current[ RESOURCE_HEALTH ] * 0.4 - old_amount, travel_dmg );
      buff_da -> trigger( 1, old_amount + new_amount );
      stats -> add_result( sim -> report_overheal ? new_amount : travel_dmg, travel_dmg, STATS_ABSORB, impact_result );
    }
  };

  bool can_trigger_DA;
  divine_aegis_t* da;
  cooldown_t* min_interval;

  void trigger_echo_of_light( heal_t* a, player_t* /* t */ )
  {
    priest_t* p = a -> player -> cast_priest();

    if ( ! p -> mastery_spells.echo_of_light -> ok() ) return;

    struct echo_of_light_t : public priest_heal_t
    {
      echo_of_light_t( player_t* p ) :
        priest_heal_t( "echo_of_light", p, 77489 )
      {
        base_tick_time = timespan_t::from_seconds( 1.0 );
        num_ticks      = 6;

        background     = true;
        tick_may_crit  = false;
        hasted_ticks   = false;
        may_crit       = false;

        init();
      }
    };

    if ( ! p -> heals_echo_of_light ) p -> heals_echo_of_light = new echo_of_light_t( p );

    if ( p -> heals_echo_of_light -> dot() -> ticking )
    {
      if ( p -> echo_of_light_merged )
      {
        // The old tick_dmg is multiplied by the remaining ticks, added to the new complete heal, and then again divided by num_ticks
        p -> heals_echo_of_light -> base_td = ( p -> heals_echo_of_light -> base_td *
                                                p -> heals_echo_of_light -> dot() -> ticks() +
                                                a -> direct_dmg * p -> composite_mastery() *
                                                p -> mastery_spells.echo_of_light -> effect_base_value( 2 ) / 10000.0 ) /
                                              p -> heals_echo_of_light -> num_ticks;
        p -> heals_echo_of_light -> dot() -> refresh_duration();
      }
      else
      {
        // The old tick_dmg is multiplied by the remaining ticks minus one!, added to the new complete heal, and then again divided by num_ticks
        p -> heals_echo_of_light -> base_td = ( p -> heals_echo_of_light -> base_td *
                                                ( p -> heals_echo_of_light -> dot() -> ticks() - 1 ) +
                                                a -> direct_dmg * p -> composite_mastery() *
                                                p -> mastery_spells.echo_of_light -> effect_base_value( 2 ) / 10000.0 ) /
                                              p -> heals_echo_of_light -> num_ticks;
        p -> heals_echo_of_light -> dot() -> refresh_duration();
        p -> echo_of_light_merged = true;
      }
    }
    else
    {
      p -> heals_echo_of_light -> base_td = a -> direct_dmg * p -> composite_mastery() *
                                            p -> mastery_spells.echo_of_light -> effect_base_value( 2 ) / 10000.0 /
                                            p -> heals_echo_of_light -> num_ticks;
      p -> heals_echo_of_light -> execute();
      p -> echo_of_light_merged = false;
    }
  }

  virtual void init()
  {
    heal_t::init();

    priest_t* p = player -> cast_priest();
    if ( can_trigger_DA && p -> talents.divine_aegis -> ok() )
    {
      da = new divine_aegis_t( ( name_str + "_divine_aegis" ).c_str(), p );
      add_child( da );
      da -> target = target;
    }
  }

  priest_heal_t( const char* n, player_t* player, const char* sname, int t = TREE_NONE ) :
    heal_t( n, player, sname, t ), can_trigger_DA( true ), da()
  {
    min_interval = player -> get_cooldown( "min_interval_" + name_str );
  }

  priest_heal_t( const char* n, player_t* player, const uint32_t id, int t = TREE_NONE ) :
    heal_t( n, player, id, t ), can_trigger_DA( true ), da()
  {
    min_interval = player -> get_cooldown( "min_interval_" + name_str );
  }

  virtual void player_buff();

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    heal_t::target_debuff( t, dmg_type );

    priest_t* p = player -> cast_priest();

    // Grace
    if ( p -> talents.grace -> ok() )
      target_multiplier *= 1.0 + t -> buffs.grace -> check() * t -> buffs.grace -> value();

    // Test of Faith
    if ( p -> talents.test_of_faith -> rank() )
    {
      if (  t -> resource_current[ RESOURCE_HEALTH ] * 2 <= t -> resource_max[ RESOURCE_HEALTH ] )
      {
        target_multiplier *= 1.0 + p -> talents.test_of_faith -> effect2().percent();
        p -> uptimes_test_of_faith -> update( true );
      }
      else
        p -> uptimes_test_of_faith -> update( false );
    }
  }

  virtual double cost() const
  {
    priest_t* p = player -> cast_priest();

    double c = heal_t::cost();

    if ( base_execute_time <= timespan_t::zero )
    {
      c *= 1.0 - p -> buffs_inner_will -> check() * p -> buffs_inner_will -> effect1().percent();
      c  = floor( c );
    }

    // Needs testing
    if ( p -> buffs_tier13_2pc_heal -> check() )
    {
      c *= 1.0 + p -> buffs_tier13_2pc_heal -> effect1().percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    heal_t::consume_resource();

    priest_t* p = player -> cast_priest();

    if ( base_execute_time <= timespan_t::zero )
      p -> buffs_inner_will -> up();
    // Needs testing
    p -> buffs_tier13_2pc_heal -> up();
  }

  virtual void execute()
  {
    heal_t::execute();

    priest_t* p = player -> cast_priest();

    if ( ! ( background || proc ) )
      p -> trigger_cauterizing_flame();

    if ( execute_time() > timespan_t::zero )
      p -> buffs_borrowed_time -> expire();
  }

  void trigger_divine_aegis( player_t* t, double amount )
  {
    if ( da )
    {
      da -> base_dd_min = da -> base_dd_max = amount * da -> shield_multiple;
      da -> heal_target.clear();
      da -> heal_target.push_back( t );
      da -> execute();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    heal_t::impact( t, impact_result, travel_dmg );
    priest_t* p = player -> cast_priest();
    priest_targetdata_t* td = targetdata() -> cast_priest();

    if ( travel_dmg > 0 )
    {
      // Divine Aegis
      if ( impact_result == RESULT_CRIT )
      {
        trigger_divine_aegis( t, travel_dmg );
      }

      trigger_echo_of_light( this, t );

      if ( p -> buffs_chakra_serenity -> up() && td -> dots_renew -> ticking )
        td -> dots_renew -> refresh_duration();
    }
  }

  void tick( dot_t* d )
  {
    heal_t::tick( d );

    // Divine Aegis
    if ( result == RESULT_CRIT )
    {
      trigger_divine_aegis( target, tick_dmg );
    }
  }

  void trigger_inspiration( int insp_result, player_t* t )
  {
    priest_t* p = player -> cast_priest();

    if ( p -> talents.inspiration -> rank() && insp_result == RESULT_CRIT )
      t -> buffs.inspiration -> trigger( 1, p -> talents.inspiration -> rank() * 5 );
  }

  void trigger_grace( player_t* t )
  {
    priest_t* p = player -> cast_priest();

    if ( p -> talents.grace -> rank() )
      t -> buffs.grace -> trigger( 1, p -> dbc.effect( p -> dbc.spell( p -> talents.grace -> effect_trigger_spell( 1 ) ) -> effect1().id() ) -> base_value() / 100.0 );
  }

  void trigger_strength_of_soul( player_t* t )
  {
    priest_t* p = player -> cast_priest();

    if ( p -> talents.strength_of_soul -> rank() && t -> buffs.weakened_soul -> up() )
      t -> buffs.weakened_soul -> extend_duration( p, timespan_t::from_seconds( -1 * p -> talents.strength_of_soul -> effect1().base_value() ) );
  }

  void update_ready()
  {
    heal_t::update_ready();

    if ( min_interval -> duration > timespan_t::zero && ! dual )
    {
      min_interval -> start( timespan_t::min, timespan_t::zero );

      if ( sim -> debug ) log_t::output( sim, "%s starts min_interval for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready.total_seconds() );
    }
  }

  bool ready()
  {
    if ( ! heal_t::ready() )
      return false;

    return ( min_interval -> remains() <= timespan_t::zero );
  }

  void parse_options( option_t*          options,
                      const std::string& options_str )
  {
    const option_t base_options[] =
    {
      { "min_interval", OPT_TIMESPAN,     &( min_interval -> duration ) },
      { NULL,           OPT_UNKNOWN, NULL      }
    };

    std::vector<option_t> merged_options;
    heal_t::parse_options( option_t::merge( merged_options, options, base_options ), options_str );
  }
};

// Atonement heal ===========================================================

struct atonement_heal_t : public priest_heal_t
{
  atonement_heal_t( const char* n, priest_t* p ) :
    priest_heal_t( n, p, 81751 )
  {
    proc           = true;
    background     = true;
    round_base_dmg = false;


    // HACK: Setting may_crit = true will force crits.
    may_crit = false;
    base_crit = 1.0;

    if ( ! p -> atonement_target_str.empty() )
      target = sim -> find_player( p -> atonement_target_str.c_str() );
  }

  void trigger( double atonement_dmg, int dmg_type, int result )
  {
    priest_t* p = player -> cast_priest();

    atonement_dmg *= p -> talents.atonement -> effect1().percent();
    double cap = p -> resource_max[ RESOURCE_HEALTH ] * 0.3;

    if ( result == RESULT_CRIT )
    {
      // FIXME: Crits in 4.1 capped at 150% of the non-crit cap. This may
      //        be changed to 200% in 4.2 along with the general heal
      //        crit multiplier change.
      cap *= 1.5;
    }

    if ( atonement_dmg > cap )
      atonement_dmg = cap;

    if ( dmg_type == DMG_OVER_TIME )
    {
      // num_ticks = 1;
      base_td = atonement_dmg;
      tick_may_crit = ( result == RESULT_CRIT );
      tick( dot() );
    }
    else
    {
      assert( dmg_type == DMG_DIRECT );
      // num_ticks = 0;
      base_dd_min = base_dd_max = atonement_dmg;
      may_crit = ( result == RESULT_CRIT );

      execute();
    }
  }

  virtual double total_crit_bonus() const
  {
    return 0;
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();
    priest_t* p = player -> cast_priest();

    // Atonement does not scale with Twin Disciplines as of WoW 4.2
    player_multiplier /= 1.0 + p -> constants.twin_disciplines_value;
  }

  virtual void execute()
  {
    heal_target.clear();
    heal_target.push_back( find_lowest_player() );

    priest_heal_t::execute();
  }

  virtual void tick( dot_t* d )
  {
    heal_target.clear();
    heal_target.push_back( find_lowest_player() );

    priest_heal_t::tick( d );
  }
};

// ==========================================================================
// Priest Spell
// ==========================================================================

struct priest_spell_t : public spell_t
{
  atonement_heal_t* atonement;
  bool can_trigger_atonement;

private:
  void _init_priest_spell_t()
  {
    priest_t* p = player -> cast_priest();

    may_crit          = true;
    tick_may_crit     = true;

    dot_behavior      = DOT_REFRESH;
    weapon_multiplier = 0.0;

    crit_bonus_multiplier *= 1.0 + p -> constants.shadow_power_crit_value;

    atonement         = 0;
    can_trigger_atonement = false;
  }

public:
  priest_spell_t( const char* n, player_t* player, const school_type s, int t ) :
    spell_t( n, player, RESOURCE_MANA, s, t ), atonement( 0 ), can_trigger_atonement( 0 )
  {
    _init_priest_spell_t();
  }

  priest_spell_t( const active_spell_t& s ) :
    spell_t( s ), atonement( 0 ), can_trigger_atonement( 0 )
  {
    _init_priest_spell_t();
  }

  priest_spell_t( const char* n, player_t* player, const char* sname, int t = TREE_NONE ) :
    spell_t( n, sname, player, t ), atonement( 0 ), can_trigger_atonement( 0 )
  {
    _init_priest_spell_t();
  }

  priest_spell_t( const char* n, player_t* player, const uint32_t id, int t = TREE_NONE ) :
    spell_t( n, id, player, t ), atonement( 0 ), can_trigger_atonement( 0 )
  {
    _init_priest_spell_t();
  }

  virtual void init()
  {
    spell_t::init();

    priest_t* p = player -> cast_priest();

    if ( can_trigger_atonement && p -> talents.atonement -> rank() )
    {
      std::string n = "atonement_" + name_str;
      atonement = new atonement_heal_t( n.c_str(), p );
    }
  }

  virtual double cost() const
  {
    priest_t* p = player -> cast_priest();

    double c = spell_t::cost();

    if ( base_execute_time <= timespan_t::zero )
    {
      c *= 1.0 - p -> buffs_inner_will -> check() * p -> buffs_inner_will -> effect1().percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    spell_t::consume_resource();

    priest_t* p = player -> cast_priest();

    if ( base_execute_time <= timespan_t::zero )
      p -> buffs_inner_will -> up();
  }

  virtual void execute();

  virtual void assess_damage( player_t* t,
                              double amount,
                              int    dmg_type,
                              int    impact_result )
  {
    priest_t* p = player -> cast_priest();

    spell_t::assess_damage( t, amount, dmg_type, impact_result );

    if ( p -> buffs_vampiric_embrace -> up() && result_is_hit( impact_result ) )
    {
      double a = amount * ( 1.0 + p -> constants.twin_disciplines_value );
      p -> resource_gain( RESOURCE_HEALTH, a * 0.06, p -> gains.vampiric_embrace );

      pet_t* r = p -> pet_list;

      while ( r )
      {
        r -> resource_gain( RESOURCE_HEALTH, a * 0.03, r -> gains.vampiric_embrace );
        r = r -> next_pet;
      }

      int num_players = ( int ) p -> party_list.size();

      for ( int i=0; i < num_players; i++ )
      {
        player_t* q = p -> party_list[ i ];

        q -> resource_gain( RESOURCE_HEALTH, a * 0.03, q -> gains.vampiric_embrace );

        r = q -> pet_list;

        while ( r )
        {
          r -> resource_gain( RESOURCE_HEALTH, a * 0.03, r -> gains.vampiric_embrace );
          r = r -> next_pet;
        }
      }
    }

    if ( atonement )
      atonement -> trigger( amount, dmg_type, impact_result );
  }

  static void trigger_shadowy_apparition( player_t* player );
  static void add_more_shadowy_apparitions( player_t* player );
};

// ==========================================================================
// Pet Shadow Fiend
// ==========================================================================

struct shadow_fiend_pet_t : public pet_t
{
  struct tier12_flame_attack_t : public spell_t
  {
    double dmg_mult;

    tier12_flame_attack_t( shadow_fiend_pet_t* player ) :
      spell_t( "Shadowflame", player, SCHOOL_FIRE ), dmg_mult( 0.2 )
    {
      priest_t* o = player -> owner -> cast_priest();

      background       = true;
      may_miss         = false;
      proc             = true;
      may_crit         = false;
      direct_power_mod = 0.0;
      trigger_gcd      = timespan_t::zero;
      school           = SCHOOL_FIRE;

      if ( const spell_data_t* shadowflame = spell_data_t::find( 99156, "Shadowflame", o -> dbc.ptr ) )
      {
        dmg_mult = shadowflame->effect1().percent();
      }
    }

    virtual double calculate_direct_damage( int )
    {
      double dmg = base_dd_min * dmg_mult;

      if ( ! binary )
      {
        dmg *= 1.0 - resistance();
      }

      return dmg;
    }
  };

  struct shadowcrawl_t : public spell_t
  {
    shadowcrawl_t( shadow_fiend_pet_t* player ) :
      spell_t( *( player -> shadowcrawl ) )
    {
      may_miss = false;
    }

    virtual void execute()
    {
      spell_t::execute();

      shadow_fiend_pet_t* p = static_cast<shadow_fiend_pet_t*>( player );

      p -> buffs_shadowcrawl -> start( 1, p -> shadowcrawl -> effect_base_value( 2 ) / 100.0 );
    }
  };

  struct melee_t : public attack_t
  {
    tier12_flame_attack_t* tier12_flame_attack_spell;

    melee_t( shadow_fiend_pet_t* player ) :
      attack_t( "melee", player, RESOURCE_NONE, SCHOOL_SHADOW ), tier12_flame_attack_spell( 0 )
    {
      priest_t* o = player -> owner -> cast_priest();
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      weapon_multiplier = 0;
      direct_power_mod = 0.0064 * o -> level;
      if ( harmful ) base_spell_power_multiplier = 1.0;
      base_attack_power_multiplier = 0.0;
      base_dd_min = util_t::ability_rank( player -> level,  221.0,85,  197.0,82,  175.0,80,  1.0,0 );
      base_dd_max = util_t::ability_rank( player -> level,  271.0,85,  245.0,82,  222.0,80,  2.0,0 );
      background = true;
      repeating  = true;
      may_dodge  = true;
      may_miss   = false;
      may_parry  = false; // Technically it can be parried on the first swing or if the rear isn't reachable
      may_crit   = true;
      may_block  = false; // Technically it can be blocked on the first swing or if the rear isn't reachable

      if ( o -> set_bonus.tier12_2pc_caster() )
      {
        tier12_flame_attack_spell = new tier12_flame_attack_t( player );

        add_child( tier12_flame_attack_spell );
      }
    }

    void assess_damage( player_t* t, double amount, int dmg_type, int impact_result )
    {
      attack_t::assess_damage( t, amount, dmg_type, impact_result );

      shadow_fiend_pet_t* p = ( shadow_fiend_pet_t* ) player -> cast_pet();
      priest_t* o = p -> owner -> cast_priest();

      if ( result_is_hit( impact_result ) )
      {
        if ( o -> set_bonus.tier13_4pc_caster() )
          o -> buffs_shadow_orb -> trigger( 3, 1, o -> constants.shadow_orb_proc_value + o -> constants.harnessed_shadows_value );

        o -> resource_gain( RESOURCE_MANA, o -> resource_max[ RESOURCE_MANA ] *
                            p -> mana_leech -> effect_base_value( 1 ) / 100.0,
                            o -> gains_shadow_fiend );
      }
    }

    void player_buff()
    {
      attack_t::player_buff();

      shadow_fiend_pet_t* p = ( shadow_fiend_pet_t* ) player -> cast_pet();

      if ( p -> bad_swing )
        p -> bad_swing = false;

      player_multiplier *= 1.0 + p -> buffs_shadowcrawl -> value();
    }

    void execute()
    {
      attack_t::execute();

      if ( tier12_flame_attack_spell )
      {
        tier12_flame_attack_spell -> base_dd_min = direct_dmg;
        tier12_flame_attack_spell -> execute();
      }
    }

    virtual void impact( player_t* t, int result, double dmg )
    {
      shadow_fiend_pet_t* p = ( shadow_fiend_pet_t* ) player -> cast_pet();
      priest_t* o = p -> owner -> cast_priest();

      attack_t::impact( t, result, dmg );

      // Needs testing
      if ( o -> set_bonus.tier13_4pc_caster() )
      {
        o -> buffs_shadow_orb -> trigger( 3, 1, 1.0 );
      }
    }
  };

  double bad_spell_power;
  buff_t* buffs_shadowcrawl;
  active_spell_t* shadowcrawl;
  passive_spell_t* mana_leech;
  bool bad_swing;
  bool extra_tick;

  shadow_fiend_pet_t( sim_t* sim, priest_t* owner ) :
    pet_t( sim, owner, "shadow_fiend" ),
    bad_spell_power( util_t::ability_rank( owner -> level,  370.0,85,  358.0,82,  352.0,80,  0.0,0 ) ),
    buffs_shadowcrawl( 0 ), shadowcrawl( 0 ), mana_leech( 0 ),
    bad_swing( false ), extra_tick( false )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
    main_hand_weapon.school     = SCHOOL_SHADOW;

    stamina_per_owner           = 0.30;
    intellect_per_owner         = 0.50;

    action_list_str             = "/snapshot_stats/shadowcrawl/wait_for_shadowcrawl";
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "shadowcrawl" ) return new shadowcrawl_t( this );
    if ( name == "wait_for_shadowcrawl" ) return new wait_for_cooldown_t( this, "shadowcrawl" );

    return pet_t::create_action( name, options_str );
  }

  virtual void init_spells()
  {
    player_t::init_spells();

    shadowcrawl                 = new active_spell_t ( this, "shadowcrawl", "Shadowcrawl" );
    mana_leech                  = new passive_spell_t( this, "mana_leech", 34650 );
  }

  virtual void init_base()
  {
    pet_t::init_base();

    attribute_base[ ATTR_STRENGTH  ]  = 0; // Unknown
    attribute_base[ ATTR_AGILITY   ]  = 0; // Unknown
    attribute_base[ ATTR_STAMINA   ]  = 0; // Unknown
    attribute_base[ ATTR_INTELLECT ]  = 0; // Unknown
    resource_base[ RESOURCE_HEALTH ]  = util_t::ability_rank( owner -> level,  18480.0,85,  7475.0,82,  6747.0,80,  100.0,0 );
    resource_base[ RESOURCE_MANA   ]  = util_t::ability_rank( owner -> level,  16828.0,85,  9824.0,82,  7679.0,80,  100.0,0 );
    base_attack_power                 = 0;  // Unknown
    base_attack_crit                  = 0.07; // Needs more testing
    initial_attack_power_per_strength = 0; // Unknown

    main_hand_attack = new melee_t( this );
  }

  virtual void init_buffs()
  {
    pet_t::init_buffs();

    buffs_shadowcrawl = new buff_t( this, "shadowcrawl", 1, shadowcrawl -> duration() );
  }

  virtual double composite_spell_power( const school_type school ) const
  {
    priest_t* p = owner -> cast_priest();

    double sp;

    if ( bad_swing )
      sp = bad_spell_power;
    else
      sp = p -> composite_spell_power( school ) * p -> composite_spell_power_multiplier();

    return sp;
  }

  virtual double composite_attack_hit() const
  {
    return owner -> composite_spell_hit();
  }

  virtual double composite_attack_expertise() const
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }

  virtual double composite_attack_crit() const
  {
    double c = pet_t::composite_attack_crit();

    c += owner -> composite_spell_crit(); // Needs confirming that it benefits from ALL owner crit.

    return c;
  }

  virtual void summon( timespan_t duration )
  {
    priest_t* p = owner -> cast_priest();

    // Simulate "Bad" swings
    if ( owner -> bugs && owner -> sim -> roll( 0.3 ) )
    {
      bad_swing = true;
    }
    // Simulate extra tick
    if ( !bugs || !owner -> sim -> roll( 0.5 ) )
    {
      duration -= timespan_t::from_seconds( 0.1 );
    }

    dismiss();

    pet_t::summon( duration );

    p -> buffs_shadowfiend -> start();
  }

  virtual void dismiss()
  {
    priest_t* p = owner -> cast_priest();

    pet_t::dismiss();

    p -> buffs_shadowfiend -> expire();
  }

  virtual void schedule_ready( timespan_t delta_time=timespan_t::zero,
                               bool   waiting=false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! main_hand_attack -> execute_event ) main_hand_attack -> execute();
  }
};

// ==========================================================================
// Pet Lightwell
// ==========================================================================

struct lightwell_pet_t : public pet_t
{
  struct lightwell_renew_t : public heal_t
  {
    lightwell_renew_t( lightwell_pet_t* player ) :
      heal_t( "lightwell_renew", player, 7001 )
    {
      priest_t* p = player -> owner -> cast_priest();

      may_crit = false;
      tick_may_crit = true;

      tick_power_mod = 0.308;
      base_multiplier *= ( 1.0 + p -> constants.twin_disciplines_value ) * 1.15;
    }

    lightwell_pet_t* cast()
    {
      return static_cast<lightwell_pet_t*>( player );
    }

    virtual void execute()
    {
      cast() -> charges--;

      assert( heal_target.size() == 1 );
      heal_target[ 0 ] = find_lowest_player();

      heal_t::execute();
    }

    virtual void last_tick( dot_t* d )
    {
      heal_t::last_tick( d );

      if ( cast() -> charges <= 0 )
        cast() -> dismiss();
    }

    virtual bool ready()
    {
      if ( cast() -> charges <= 0 )
        return false;
      return heal_t::ready();
    }
  };

  int charges;

  lightwell_pet_t( sim_t* sim, priest_t* p ) :
    pet_t( sim, p, "lightwell", PET_NONE, true ),
    charges( 0 )
  {
    role = ROLE_HEAL;
    action_list_str = "/snapshot_stats/lightwell_renew/wait,sec=cooldown.lightwell_renew.remains";
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "lightwell_renew" ) return new lightwell_renew_t( this );

    return pet_t::create_action( name, options_str );
  }

  virtual void summon( timespan_t duration )
  {
    priest_t* p = owner -> cast_priest();

    spell_haste = p -> spell_haste;
    spell_power[ SCHOOL_HOLY ] = p -> composite_spell_power( SCHOOL_HOLY ) * p -> composite_spell_power_multiplier();

    charges = 10 + p -> glyphs.lightwell -> effect1().base_value();

    pet_t::summon( duration );
  }
};

// ==========================================================================
// Pet Cauterizing Flame
// ==========================================================================

struct cauterizing_flame_pet_t : public pet_t
{
  struct cauterizing_flame_heal_t : public heal_t
  {
    cauterizing_flame_heal_t( player_t* player ) :
      heal_t( "cauterizing_flame_heal", player, 99152 )
    {
      cooldown -> duration = timespan_t::from_seconds( 1 );
    }

    void execute()
    {
      // Choose Heal Target
      heal_target.clear();
      heal_target.push_back( find_lowest_player() );
      heal_t::execute();
    }
  };

  cauterizing_flame_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "cauterizing_flame", PET_NONE, true )
  {
    role = ROLE_HEAL;
    action_list_str = "/snapshot_stats/cauterizing_flame_heal/wait_for_cauterizing_flame_heal";
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "cauterizing_flame_heal" ) return new cauterizing_flame_heal_t( this );
    if ( name == "wait_for_cauterizing_flame_heal" ) return new wait_for_cooldown_t( this, "cauterizing_flame_heal" );

    return pet_t::create_action( name, options_str );
  }

  virtual void summon( timespan_t duration )
  {
    priest_t* p = owner -> cast_priest();
    pet_t::summon( duration );
    p -> buffs_cauterizing_flame -> start();
  }

  virtual void dismiss()
  {
    priest_t* p = owner -> cast_priest();
    pet_t::dismiss();
    p -> buffs_cauterizing_flame -> expire();
  }
};

// ==========================================================================
// Priest Spell Increments
// ==========================================================================

// Shadowy Apparition Spell =================================================

struct shadowy_apparition_t : public priest_spell_t
{
  shadowy_apparition_t( player_t* player ) :
    priest_spell_t( "shadowy_apparition", player, 87532 )
  {
    background        = true;
    proc              = true;
    callbacks         = false;

    trigger_gcd       = timespan_t::zero;
    travel_speed      = 3.5;

    base_crit += 0.06; // estimated.

    if ( player -> bugs )
    {
      base_dd_min *= 1.0625;
      base_dd_max *= 1.0625;
    }

    init();
  }

  virtual void impact( player_t* t, int result, double dmg )
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::impact( t, result, dmg );


    // Needs testing
    if ( p -> set_bonus.tier13_4pc_caster() )
    {
      p -> buffs_shadow_orb -> trigger( 3, 1, 1.0 );
    }

    // Cleanup. Re-add to free list.
    p -> shadowy_apparition_active_list.remove( this );
    p -> shadowy_apparition_free_list.push( this );
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::player_buff();

    if ( player -> bugs )
    {
      player_multiplier /= 1.0 + p -> constants.twisted_faith_static_value;
    }

    player_multiplier *= 1.0 + p -> sets -> set( SET_T11_4PC_CASTER ) -> mod_additive( P_GENERIC );
  }
};

void priest_spell_t::trigger_shadowy_apparition( player_t* player )
{
  priest_t* p = player -> cast_priest();

  spell_t* s = NULL;

  if ( p -> talents.shadowy_apparition -> rank() )
  {
    double h = p -> talents.shadowy_apparition -> rank() * ( p -> is_moving() ? 0.2 : 0.04 );
    if ( p -> sim -> roll( h ) && ( !p -> shadowy_apparition_free_list.empty() ) )
    {
      s = p -> shadowy_apparition_free_list.front();

      p -> shadowy_apparition_free_list.pop();

      p -> shadowy_apparition_active_list.push_back( s );

      p -> procs_shadowy_apparation -> occur();

      s -> execute();
    }
  }
}

void priest_spell_t::add_more_shadowy_apparitions( player_t* player )
{
  priest_t* p = player -> cast_priest();

  spell_t* s = NULL;

  if ( ! p -> shadowy_apparition_free_list.size() )
  {
    for ( uint32_t i = 0; i < p -> constants.max_shadowy_apparitions; i++ )
    {
      s = new shadowy_apparition_t( p );
      p -> shadowy_apparition_free_list.push( s );
    }
  }
}

// ==========================================================================
// Priest Abilities
// ==========================================================================


// ==========================================================================
// Priest Non-Harmful Spells
// ==========================================================================


// Archangel Spell ==========================================================

struct archangel_t : public priest_spell_t
{
  archangel_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "archangel", p, "Archangel" )
  {
    check_talent( p -> talents.archangel -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    timespan_t delta = p -> buffs_holy_evangelism -> last_trigger - p -> buffs_dark_evangelism -> last_trigger;

    if ( p -> buffs_holy_evangelism -> up() && delta > timespan_t::zero )
    {
      cooldown -> duration = timespan_t::from_seconds( effect2().base_value() );
      p -> buffs_holy_archangel -> trigger( 1, p -> constants.holy_archangel_value * p -> buffs_holy_evangelism -> stack() );
      p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * p -> constants.archangel_mana_value * p -> buffs_holy_evangelism -> stack(), p -> gains_archangel );
      p -> buffs_holy_evangelism -> expire();
    }
    else if ( p -> buffs_dark_evangelism -> up() && delta < timespan_t::zero )
    {
      cooldown -> duration = timespan_t::from_seconds( effect3().base_value() );
      p -> buffs_dark_archangel -> trigger( 1, p -> buffs_dark_evangelism -> stack() );
      p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * p -> constants.dark_archangel_mana_value * p -> buffs_dark_evangelism -> stack(), p -> gains_archangel );
      p -> buffs_dark_evangelism -> expire();
    }

    priest_spell_t::execute();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( ! ( p -> buffs_holy_evangelism -> check() || p -> buffs_dark_evangelism -> check() ) )
      return false;

    return priest_spell_t::ready();
  }
};

// Chakra_Pre Spell =========================================================

struct chakra_t : public priest_spell_t
{
  chakra_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "chakra", p, "Chakra" )
  {
    check_talent( p -> talents.chakra -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    // FIXME: Doesn't the cooldown work like Inner Focus?
    cooldown -> duration = timespan_t::from_seconds( sim -> wheel_seconds );

    priest_spell_t::execute();

    priest_t* p = player -> cast_priest();

    p -> buffs_chakra_pre -> start();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( p -> buffs_chakra_pre -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Dispersion Spell =========================================================

struct dispersion_t : public priest_spell_t
{
  int low_mana;

  dispersion_t( player_t* player, const std::string& options_str ) :
    priest_spell_t( "dispersion", player, "Dispersion" ), low_mana( 0 )
  {
    option_t options[] =
    {
      { "low_mana",  OPT_BOOL,    &low_mana },
      { NULL,        OPT_UNKNOWN, NULL      }
    };
    parse_options( options, options_str );

    priest_t* p = player -> cast_priest();

    check_talent( p -> talents.dispersion -> rank() );

    base_tick_time    = timespan_t::from_seconds( 1.0 );
    num_ticks         = 6;

    channeled         = true;
    harmful           = false;
    tick_may_crit     = false;

    cooldown -> duration += p -> glyphs.dispersion -> effect1().time_value();
  }

  virtual void tick( dot_t* d )
  {
    priest_t* p = player -> cast_priest();

    double regen_amount = p -> dbc.spell( 49766 ) -> effect1().percent() * p -> resource_max[ RESOURCE_MANA ];

    p -> resource_gain( RESOURCE_MANA, regen_amount, p -> gains_dispersion );

    priest_spell_t::tick( d );
  }

  virtual bool ready()
  {
    if ( ! priest_spell_t::ready() )
      return false;

    if ( ! low_mana )
      return true;

    priest_t* p = player -> cast_priest();

    if ( ! p -> pet_shadow_fiend -> sleeping ) return false;

    double sf_cooldown_remains  = p -> cooldowns_shadow_fiend -> remains().total_seconds();
    double sf_cooldown_duration = p -> cooldowns_shadow_fiend -> duration.total_seconds();

    if ( sf_cooldown_remains <= 0 ) return false;

    double     max_mana = p -> resource_max    [ RESOURCE_MANA ];
    double current_mana = p -> resource_current[ RESOURCE_MANA ];

    double consumption_rate = ( p -> mana_resource.mana_loss - p -> mana_resource.mana_gain ) / sim -> current_time.total_seconds();
    double time_to_die = p -> target -> time_to_die().total_seconds();

    if ( consumption_rate <= 0.00001 ) return false;

    double oom_time = current_mana / consumption_rate;

    if ( oom_time >= time_to_die ) return false;

    double sf_regen = 0.30 * max_mana;

    consumption_rate -= sf_regen / sf_cooldown_duration;

    if ( consumption_rate <= 0.00001 ) return false;

    double future_mana = current_mana + sf_regen - consumption_rate * sf_cooldown_remains;

    if ( future_mana > max_mana ) future_mana = max_mana;

    oom_time = future_mana / consumption_rate;

    if ( oom_time >= time_to_die ) return false;

    return ( max_mana - current_mana ) > p -> max_mana_cost;
  }
};

// Fortitude Spell ==========================================================

struct fortitude_t : public priest_spell_t
{
  double bonus;

  fortitude_t( player_t* player, const std::string& options_str ) :
    priest_spell_t( "fortitude", player, "Power Word: Fortitude" ), bonus( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;

    background = ( sim -> overrides.fortitude != 0 );

    bonus = floor( player -> dbc.effect_average( player -> dbc.spell( 79104 ) -> effect1().id() , player -> level ) );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> ooc_buffs() )
      {
        double before = p -> attribute[ ATTR_STAMINA ];
        p -> buffs.fortitude -> trigger( 1, bonus );
        double  after = p -> attribute[ ATTR_STAMINA ];
        p -> stat_gain( STAT_HEALTH, ( after - before ) * p -> health_per_stamina );
      }
    }
  }

  virtual bool ready()
  {
    if ( player -> buffs.fortitude -> current_value >= bonus ||
         sim -> auras.qiraji_fortitude -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Hymn of Hope Spell =======================================================

struct hymn_of_hope_tick_t : public priest_spell_t
{
  hymn_of_hope_tick_t( player_t* player ) :
    priest_spell_t( "hymn_of_hope_tick", player, 64904 )
  {
    dual        = true;
    background  = true;
    may_crit    = true;
    direct_tick = true;

    harmful     = false;

    stats = player -> get_stats( "hymn_of_hope", this );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    priest_t* p = player -> cast_priest();

    p -> resource_gain( RESOURCE_MANA, effect1().percent() * p -> resource_max[ RESOURCE_MANA ], p -> gains_hymn_of_hope );

    // Hymn of Hope only adds +x% of the current_max mana, it doesn't change if afterwards max_mana changes.
    p -> buffs.hymn_of_hope -> trigger();
  }
};

struct hymn_of_hope_t : public priest_spell_t
{
  hymn_of_hope_tick_t* hymn_of_hope_tick;

  hymn_of_hope_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "hymn_of_hope", p, "Hymn of Hope" ),
    hymn_of_hope_tick( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;

    channeled = true;

    hymn_of_hope_tick = new hymn_of_hope_tick_t( p );
  }

  virtual void tick( dot_t* d )
  {
    hymn_of_hope_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }
};

// Inner Focus Spell ========================================================

struct inner_focus_t : public priest_spell_t
{
  inner_focus_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "inner_focus", p, "Inner Focus" )
  {
    check_talent( p -> talents.inner_focus -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    cooldown -> duration = timespan_t::from_seconds( sim -> wheel_seconds );

    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();

    p -> buffs_inner_focus -> trigger();
  }
};

// Inner Fire Spell =========================================================

struct inner_fire_t : public priest_spell_t
{
  inner_fire_t( player_t* player, const std::string& options_str ) :
    priest_spell_t( "inner_fire", player, "Inner Fire" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();

    p -> buffs_inner_will -> expire ();
    p -> buffs_inner_fire -> trigger();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( p -> buffs_inner_fire -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Inner Will Spell =========================================================

struct inner_will_t : public priest_spell_t
{
  inner_will_t( player_t* player, const std::string& options_str ) :
    priest_spell_t( "inner_will", player, "Inner Will" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();

    p -> buffs_inner_fire -> expire();

    p -> buffs_inner_will -> trigger();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( p -> buffs_inner_will -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Pain Supression ==========================================================

struct pain_suppression_t : public priest_spell_t
{
  pain_suppression_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "pain_suppression", p, "Pain Suppression" )
  {
    check_talent( p -> talents.pain_suppression -> ok() );

    parse_options( NULL, options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target -> is_enemy() || target -> is_add() )
    {
      target = p;
    }

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    target -> buffs.pain_supression -> trigger();
  }
};

// Power Infusion Spell =====================================================

struct power_infusion_t : public priest_spell_t
{
  power_infusion_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "power_infusion", p, "Power Infusion" )
  {
    check_talent( p -> talents.power_infusion -> rank() );

    parse_options( NULL, options_str );

    // If we don't specify a target, it's defaulted to the mob, so default to the player instead
    if ( target -> is_enemy() || target -> is_add() )
    {
      target = p;
    }

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    priest_t* p = player -> cast_priest();

    target -> buffs.power_infusion -> trigger();

    // Needs testing
    p -> buffs_tier13_2pc_heal -> trigger();
  }

  virtual bool ready()
  {
    if ( target -> buffs.bloodlust -> check() )
      return false;

    if ( target -> buffs.power_infusion -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Shadow Form Spell ========================================================

struct shadow_form_t : public priest_spell_t
{
  shadow_form_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "shadow_form", p, "Shadowform" )
  {
    parse_options( NULL, options_str );

    harmful = false;

    base_cost *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost  = floor( base_cost );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();

    p -> buffs_shadow_form -> trigger();

    sim -> auras.mind_quickening -> trigger();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if (  p -> buffs_shadow_form -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Shadow Fiend Spell =======================================================

struct shadow_fiend_spell_t : public priest_spell_t
{
  shadow_fiend_spell_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "shadow_fiend", p, "Shadowfiend" )
  {
    parse_options( NULL, options_str );

    cooldown = p -> cooldowns_shadow_fiend;
    cooldown -> duration = p -> active_spells.shadow_fiend -> cooldown() +
                           p -> talents.veiled_shadows -> effect2().time_value() +
                           ( p -> set_bonus.tier12_2pc_caster() ? timespan_t::from_seconds( -75.0 ) : timespan_t::zero );

    harmful = false;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();

    p -> pet_shadow_fiend -> summon( duration() );
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( p -> buffs_shadowfiend -> check() )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};


// ==========================================================================
// Priest Damage Spells
// ==========================================================================

// priest_spell_t::execute - artifical separator ============================

void priest_spell_t::execute()
{
  spell_t::execute();

  priest_t* p = player -> cast_priest();

  if ( execute_time() > timespan_t::zero )
    p -> buffs_borrowed_time -> expire();
}

// Devouring Plague Spell ===================================================

struct devouring_plague_burst_t : public priest_spell_t
{
  devouring_plague_burst_t( player_t* player ) :
    priest_spell_t( "devouring_plague_burst", player, SCHOOL_SHADOW, TREE_SHADOW )
  {
    proc       = true;
    background = true;
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();

    priest_t* p = player -> cast_priest();

    double m = 1.0;

    m += p -> buffs_dark_evangelism -> stack () * p -> buffs_dark_evangelism -> effect1().percent();
    m += p -> buffs_empowered_shadow -> value();

    if ( ! p -> bugs )
    {
      m += 0.01 * p -> buffs.dark_intent_feedback -> stack();
    }

    player_multiplier *= m;
  }
};

struct devouring_plague_t : public priest_spell_t
{
  devouring_plague_burst_t* burst_spell;

  devouring_plague_t( player_t* player, const std::string& options_str ) :
    priest_spell_t( "devouring_plague", player, "Devouring Plague" ), burst_spell( 0 )
  {
    priest_t* p = player -> cast_priest();
    parse_options( NULL, options_str );

    may_crit   = false;

    base_cost        *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost         = floor( base_cost );

    if ( p -> talents.improved_devouring_plague -> rank() )
    {
      burst_spell = new devouring_plague_burst_t( p );

      add_child( burst_spell );
    }
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();

    // Improved Devouring Plague
    if ( burst_spell )
    {
      double dmg = ceil( base_td ) + total_power() * tick_power_mod;

      dmg *= p -> talents.improved_devouring_plague -> base_value( E_APPLY_AURA, A_DUMMY, P_DAMAGE_TAKEN ) / 100.0;

      double n = hasted_num_ticks();

      if ( p -> bugs )
      {
        // Currently it's rounding up but only using haste rating haste.
        n = ( int ) ceil( num_ticks / p -> spell_haste );
      }

      burst_spell -> base_dd_min    = dmg * n;
      burst_spell -> base_dd_max    = dmg * n;
      burst_spell -> round_base_dmg = false;
      burst_spell -> execute();
    }
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    priest_t* p = player -> cast_priest();

    player -> resource_gain( RESOURCE_HEALTH, tick_dmg * p -> constants.devouring_plague_health_mod );
  }
};

// Mind Blast Spell =========================================================

struct mind_blast_t : public priest_spell_t
{
  stats_t* orb_stats[ 4 ];

  mind_blast_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "mind_blast", player, "Mind Blast" )
  {
    parse_options( NULL, options_str );

    priest_t* p = player -> cast_priest();

    cooldown -> duration += p -> talents.improved_mind_blast -> effect1().time_value();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new mind_blast_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void init()
  {
    priest_t* p = player -> cast_priest();

    for ( int i=0; i < 4; i++ )
    {
      std::string str = name_str + "_";
      orb_stats[ i ] = p -> get_stats( str + ( char ) ( i + ( int ) '0' ) + ( is_dtr_action ? "_DTR" : "" ), this );
    }
    priest_spell_t::init();
  }

  virtual void execute()
  {
    timespan_t saved_cooldown = cooldown -> duration;

    priest_t* p = player -> cast_priest();
    priest_targetdata_t* td = targetdata() -> cast_priest();

    stats = orb_stats[ p -> buffs_shadow_orb -> stack() ];

    priest_spell_t::execute();

    cooldown -> duration = saved_cooldown;

    for ( int i=0; i < 4; i++ )
    {
      p -> uptimes_mind_spike[ i ] -> update( i == p -> buffs_mind_spike -> stack() );
    }
    for ( int i=0; i < 4; i++ )
    {
      p -> uptimes_shadow_orb[ i ] -> update( i == p -> buffs_shadow_orb -> stack() );
    }

    p -> buffs_mind_melt -> expire();
    p -> buffs_mind_spike -> expire();

    if ( p -> buffs_shadow_orb -> check() )
    {
      p -> buffs_shadow_orb -> expire();

      p -> buffs_empowered_shadow -> trigger( 1, p -> empowered_shadows_amount() );
    }

    if ( result_is_hit() )
    {
      if ( td -> dots_vampiric_touch -> ticking )
        p -> trigger_replenishment();
    }
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();
    priest_targetdata_t* td = targetdata() -> cast_priest();

    priest_spell_t::player_buff();

    double m = 1.0;

    if ( p -> set_bonus.tier12_4pc_caster() )
    {
      if ( td -> dots_shadow_word_pain -> ticking && td -> dots_vampiric_touch -> ticking && td -> dots_devouring_plague -> ticking )
      {
        m += p -> dark_flames -> effect1().percent();
        p -> uptimes_dark_flames -> update( 1 );
      }
      else
      {
        p -> uptimes_dark_flames -> update( 0 );
      }
    }

    m += p -> buffs_dark_archangel -> value() * p -> constants.dark_archangel_damage_value;

    player_multiplier *= m;

    player_multiplier *= 1.0 + ( p -> buffs_shadow_orb -> stack() * p -> shadow_orb_amount() );
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    priest_spell_t::target_debuff( t, dmg_type );

    priest_t* p = player -> cast_priest();

    target_crit       += p -> buffs_mind_spike -> value() * p -> buffs_mind_spike -> check();
  }

  virtual timespan_t execute_time() const
  {
    timespan_t a = priest_spell_t::execute_time();

    priest_t* p = player -> cast_priest();

    a *= 1 + ( p -> buffs_mind_melt -> check() * p -> talents.mind_melt -> effect2().percent() );

    return a;
  }
};

// Mind Flay Spell ==========================================================

struct mind_flay_t : public priest_spell_t
{
  timespan_t mb_wait;
  int    swp_refresh;
  int    cut_for_mb;

  mind_flay_t( priest_t* p, const std::string& options_str,
               const char* name = "mind_flay" ) :
    priest_spell_t( name, p, "Mind Flay" ), mb_wait( timespan_t::zero ), swp_refresh( 0 ), cut_for_mb( 0 )
  {
    check_spec( TREE_SHADOW );

    option_t options[] =
    {
      { "cut_for_mb",  OPT_BOOL, &cut_for_mb  },
      { "mb_wait",     OPT_TIMESPAN,  &mb_wait     },
      { "swp_refresh", OPT_BOOL, &swp_refresh },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit     = false;
    channeled    = true;
    hasted_ticks = false;
    base_crit   += p -> sets -> set( SET_T11_2PC_CASTER ) -> mod_additive( P_CRIT );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    if ( cut_for_mb )
      if ( p -> cooldowns_mind_blast -> remains() <= ( 2 * base_tick_time * haste() ) )
        num_ticks = 2;

    priest_spell_t::execute();

    if ( result_is_hit() )
    {
      // Evangelism procs off both the initial cast and each tick.
      p -> buffs_dark_evangelism -> trigger();
    }
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::player_buff();

    player_td_multiplier += p -> buffs_dark_archangel -> value() * p -> constants.dark_archangel_damage_value;

    player_td_multiplier += p -> glyphs.mind_flay -> effect1().percent();
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    priest_t* p = player -> cast_priest();
    priest_targetdata_t* td = targetdata() -> cast_priest();

    if ( result_is_hit() )
    {
      p -> buffs_dark_evangelism -> trigger();
      p -> buffs_shadow_orb  -> trigger( 1, 1, p -> constants.shadow_orb_proc_value + p -> constants.harnessed_shadows_value );

      if ( td -> dots_shadow_word_pain -> ticking )
      {
        if ( p -> rng_pain_and_suffering -> roll( p -> constants.pain_and_suffering_value ) )
        {
          td -> dots_shadow_word_pain -> refresh_duration();
        }
      }
      if ( result == RESULT_CRIT )
      {
        p -> cooldowns_shadow_fiend -> ready -= timespan_t::from_seconds( 1.0 ) * p -> talents.sin_and_punishment -> effect2().base_value();
      }
    }
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    // Optional check to only cast Mind Flay if there's 2 or less ticks left on SW:P
    // This allows a action+=/mind_flay,swp_refresh to be added as a higher priority to ensure that SW:P doesn't fall off
    // Won't be necessary as part of a standard rotation, but if there's movement or other delays in casting it would have
    // it's uses.
    if ( swp_refresh && ( p -> talents.pain_and_suffering -> rank() > 0 ) )
    {
      priest_targetdata_t* td = targetdata() -> cast_priest();
      if ( ! td ->dots_shadow_word_pain -> ticking )
        return false;

      if ( ( td -> dots_shadow_word_pain -> num_ticks - td -> dots_shadow_word_pain -> current_tick ) > 2 )
        return false;
    }

    // If this option is set (with a value in seconds), don't cast Mind Flay if Mind Blast
    // is about to come off it's cooldown.
    if ( mb_wait != timespan_t::zero )
    {
      if ( p -> cooldowns_mind_blast -> remains() < mb_wait )
        return false;
    }

    return priest_spell_t::ready();
  }
};

// Mind Spike Spell =========================================================

struct mind_spike_t : public priest_spell_t
{
  mind_spike_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "mind_spike", player, "Mind Spike" )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new mind_spike_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void reset()
  {
    priest_targetdata_t* td = targetdata() -> cast_priest();

    priest_spell_t::reset();

    td -> remove_dots_event = 0;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_chakra_pre -> up() )
    {
      p -> buffs_chakra_chastise -> trigger();

      p -> buffs_chakra_pre -> expire();

      p -> cooldowns_chakra -> reset();
      p -> cooldowns_chakra -> duration  = p -> buffs_chakra_pre -> spell_id_t::cooldown();
      p -> cooldowns_chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double dmg )
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::impact( t, impact_result, dmg );

    if ( result_is_hit( impact_result ) )
    {
      priest_targetdata_t* td = targetdata() -> cast_priest();
      for ( int i=0; i < 4; i++ )
      {
        p -> uptimes_shadow_orb[ i ] -> update( i == p -> buffs_shadow_orb -> stack() );
      }

      p -> buffs_mind_melt -> trigger( 1, 1.0 );

      if ( p -> buffs_shadow_orb -> check() )
      {
        p -> buffs_shadow_orb -> expire();
        p -> buffs_empowered_shadow -> trigger( 1, p -> empowered_shadows_amount() );
      }
      p -> buffs_mind_spike -> trigger( 1, effect2().percent() );

      if ( ! td -> remove_dots_event )
      {
        td -> remove_dots_event = new ( sim ) remove_dots_event_t( sim, p, td );
      }
    }
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::player_buff();

    player_multiplier *= 1.0 + p -> buffs_dark_archangel -> value() * p -> constants.dark_archangel_damage_value;

    player_multiplier *= 1.0 + ( p -> buffs_shadow_orb -> stack() * p -> shadow_orb_amount() );
  }
};

// Mind Sear Spell ==========================================================

struct mind_sear_tick_t : public priest_spell_t
{
  mind_sear_tick_t( player_t* player ) :
    priest_spell_t( "mind_sear_tick", player, 49821 )
  {
    background  = true;
    dual        = true;
    direct_tick = true;

    stats = player -> get_stats( "mind_sear", this );
  }
};

struct mind_sear_t : public priest_spell_t

{
  mind_sear_tick_t* mind_sear_tick;

  mind_sear_t( player_t* player, const std::string& options_str ) :
    priest_spell_t( "mind_sear", player, "Mind Sear" ),
    mind_sear_tick( 0 )
  {
    parse_options( NULL, options_str );

    channeled = true;
    may_crit  = false;

    mind_sear_tick = new mind_sear_tick_t( player );
  }

  virtual void tick( dot_t* d )
  {
    if ( mind_sear_tick )
      mind_sear_tick -> execute();

    stats -> add_tick( d -> time_to_tick );
  }
};

// Shadow Word Death Spell ==================================================

struct shadow_word_death_t : public priest_spell_t
{
  timespan_t mb_min_wait;
  timespan_t mb_max_wait;

  shadow_word_death_t( priest_t* p, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "shadow_word_death", p, "Shadow Word: Death" ), mb_min_wait( timespan_t::zero ), mb_max_wait( timespan_t::zero )
  {
    option_t options[] =
    {
      { "mb_min_wait", OPT_TIMESPAN,  &mb_min_wait },
      { "mb_max_wait", OPT_TIMESPAN,  &mb_max_wait },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost  = floor( base_cost );

    base_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 1 ) / 100.0;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new shadow_word_death_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    p -> was_sub_25 = ! is_dtr_action && p -> glyphs.shadow_word_death -> ok() && ( ! p -> buffs_glyph_of_shadow_word_death -> check() ) &&
                      ( target -> health_percentage() <= 25 );

    priest_spell_t::execute();

    if ( result_is_hit() && p -> was_sub_25 && ( target -> health_percentage() > 0 ) )
    {
      cooldown -> reset();
      p -> buffs_glyph_of_shadow_word_death -> trigger();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::impact( t, impact_result, travel_dmg );

    double health_loss = travel_dmg * ( 1.0 - p -> talents.pain_and_suffering -> rank() * 0.20 );

    // Needs testing
    if ( p -> set_bonus.tier13_2pc_caster() )
    {
      health_loss *= 1.0 - p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 2 ) / 100.0;
    }

    p -> assess_damage( health_loss, school, DMG_DIRECT, RESULT_HIT, this );

    if ( ( ( health_loss > 0.0 ) || ( p -> set_bonus.tier13_2pc_caster() )  ) && p -> talents.masochism -> rank() )
    {
      p -> resource_gain( RESOURCE_MANA, p -> talents.masochism -> effect1().percent() * p -> resource_max[ RESOURCE_MANA ], p -> gains_masochism );
    }
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::player_buff();

    if ( p -> talents.mind_melt -> rank() && ( target -> health_percentage() <= p -> talents.mind_melt-> spell( 1 ).effect3().base_value() ) )
      player_multiplier *= 1.0 + p -> talents.mind_melt -> effect1().percent();

    // Hardcoded into the tooltip
    if ( target -> health_percentage() <= 25 )
      player_multiplier *= 3.0;

    player_multiplier *= 1.0 + p -> buffs_dark_archangel -> value() * p -> constants.dark_archangel_damage_value;
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( mb_min_wait != timespan_t::zero )
      if ( p -> cooldowns_mind_blast -> remains() < mb_min_wait )
        return false;

    if ( mb_max_wait != timespan_t::zero )
      if ( p -> cooldowns_mind_blast -> remains() > mb_max_wait )
        return false;

    return priest_spell_t::ready();
  }
};

// Shadow Word Pain Spell ===================================================

struct shadow_word_pain_t : public priest_spell_t
{
  shadow_word_pain_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "shadow_word_pain", p, "Shadow Word: Pain" )
  {
    parse_options( NULL, options_str );

    may_crit   = false;

    base_cost *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost  = floor( base_cost );

    stats -> children.push_back( player -> get_stats( "shadowy_apparition", this ) );
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::player_buff();

    player_td_multiplier += p -> constants.improved_shadow_word_pain_value;

    player_td_multiplier += p -> glyphs.shadow_word_pain -> effect1().percent();
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    priest_t* p = player -> cast_priest();

    trigger_shadowy_apparition( p );

    if ( result_is_hit() )
    {
      p -> buffs_shadow_orb  -> trigger( 1, 1, p -> constants.shadow_orb_proc_value + p -> constants.harnessed_shadows_value );
    }
  }
};

// Vampiric Embrace Spell ===================================================

struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "vampiric_embrace", p, "Vampiric Embrace" )
  {
    check_talent( p -> talents.vampiric_embrace -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();

    p -> buffs_vampiric_embrace -> trigger();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( p -> buffs_vampiric_embrace -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Vampiric Touch Spell =====================================================

struct vampiric_touch_t : public priest_spell_t
{
  vampiric_touch_t( player_t* player, const std::string& options_str ) :
    priest_spell_t( "vampiric_touch", player, "Vampiric Touch" )
  {
    parse_options( NULL, options_str );
    may_crit   = false;
  }
};

// Holy Fire Spell ==========================================================

struct holy_fire_t : public priest_spell_t
{
  holy_fire_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "holy_fire", player, "Holy Fire" )
  {
    parse_options( NULL, options_str );

    priest_t* p = player -> cast_priest();

    base_execute_time += p -> talents.divine_fury -> effect1().time_value();

    base_hit += p -> glyphs.divine_accuracy -> effect1().percent();

    can_trigger_atonement = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new holy_fire_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    priest_t* p = player -> cast_priest();

    p -> buffs_holy_evangelism  -> trigger();
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();

    priest_t* p = player -> cast_priest();

    player_multiplier *= 1.0 + ( p -> buffs_holy_evangelism -> stack() * p -> buffs_holy_evangelism -> effect1().percent() );
  }

  virtual double cost() const
  {
    double c = priest_spell_t::cost();

    priest_t* p = player -> cast_priest();

    c *= 1.0 + ( p -> buffs_holy_evangelism -> check() * p -> buffs_holy_evangelism -> effect2().percent() );

    return c;
  }
};

// Penance Spell ============================================================

struct penance_t : public priest_spell_t
{
  struct penance_tick_t : public priest_spell_t
  {
    penance_tick_t( player_t* player ) :
      priest_spell_t( "penance_tick", player, 47666 )
    {
      background  = true;
      dual        = true;
      direct_tick = true;

      stats = player -> get_stats( "penance", this );
    }

    virtual void player_buff()
    {
      priest_spell_t::player_buff();

      priest_t* p = player -> cast_priest();

      player_multiplier *= 1.0 + ( p -> buffs_holy_evangelism -> stack() * p -> buffs_holy_evangelism -> effect1().percent() );
    }
  };

  penance_tick_t* tick_spell;

  penance_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "penance", p, "Penance" ),
    tick_spell( 0 )
  {
    check_spec( TREE_DISCIPLINE );

    parse_options( NULL, options_str );

    may_crit       = false;
    may_miss       = false;
    channeled      = true;
    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

    cooldown -> duration = spell_id_t::cooldown() + p -> glyphs.penance -> effect1().time_value();

    tick_spell = new penance_tick_t( p );
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual double cost() const
  {
    double c = priest_spell_t::cost();

    priest_t* p = player -> cast_priest();

    c *= 1.0 + ( p -> buffs_holy_evangelism -> check() * p -> buffs_holy_evangelism -> effect2().percent() );

    return c;
  }
};

// Smite Spell ==============================================================

struct smite_t : public priest_spell_t
{
  smite_t( priest_t* p, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "smite", p, "Smite" )
  {
    parse_options( NULL, options_str );

    base_execute_time += p -> talents.divine_fury -> effect1().time_value();
    base_hit += p -> glyphs.divine_accuracy -> effect1().percent();

    can_trigger_atonement = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new smite_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();

    if ( p -> buffs_surge_of_light -> trigger() )
      p -> procs_surge_of_light -> occur();

    p -> buffs_holy_evangelism -> trigger();

    // Chakra
    if ( p -> buffs_chakra_pre -> up() )
    {
      p -> buffs_chakra_chastise -> trigger();

      p -> buffs_chakra_pre -> expire();

      p -> cooldowns_chakra -> reset();
      p -> cooldowns_chakra -> duration = p -> buffs_chakra_pre -> spell_id_t::cooldown();
      p -> cooldowns_chakra -> start();
    }

    // Train of Thought
    if ( p -> talents.train_of_thought -> rank() &&
         p -> rng_train_of_thought -> roll( util_t::talent_rank( p -> talents.train_of_thought -> rank(), 2, 0.5, 1.0 ) ) )
    {
      if ( p -> cooldowns_penance -> remains() > p -> talents.train_of_thought -> spell( 1 ).effect2().time_value() )
        p -> cooldowns_penance -> ready -= p -> talents.train_of_thought -> spell( 1 ).effect2().time_value();
      else
        p -> cooldowns_penance -> reset();

      p -> procs_train_of_thought_smite -> occur();
    }
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();

    priest_t* p = player -> cast_priest();
    priest_targetdata_t* td = targetdata() -> cast_priest();

    player_multiplier *= 1.0 + ( p -> buffs_holy_evangelism -> stack() * p -> buffs_holy_evangelism -> effect1().percent() );

    if ( td -> dots_holy_fire -> ticking && p -> glyphs.smite -> ok() )
      player_multiplier *= 1.0 + p -> glyphs.smite -> effect1().percent();
  }

  virtual double cost() const
  {
    double c = priest_spell_t::cost();

    priest_t* p = player -> cast_priest();

    c *= 1.0 + ( p -> buffs_holy_evangelism -> check() * p -> buffs_holy_evangelism -> effect2().percent() );

    return c;
  }
};


// ==========================================================================
// Priest Heal & Absorb Spells
// ==========================================================================

// priest_heal_t::player_buff - artifical separator =========================

void priest_heal_t::player_buff()
{
  heal_t::player_buff();

  priest_t* p = player -> cast_priest();

  player_multiplier *= 1.0 + p -> passive_spells.spiritual_healing -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) ;

  if ( p -> buffs_chakra_serenity -> up() )
    player_crit += p -> buffs_chakra_serenity -> effect1().percent();
  if ( p -> buffs_serenity -> up() )
    player_crit += p -> buffs_serenity -> effect2().percent();

  player_multiplier *= 1.0 + p -> buffs_holy_archangel -> value();
}


// Binding Heal Spell =======================================================

struct binding_heal_t : public priest_heal_t
{
  binding_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "binding_heal", p, "Binding Heal" )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p -> talents.empowered_healing -> base_value( E_APPLY_AURA , A_ADD_PCT_MODIFIER );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    // Adjust heal_target
    heal_target.resize( 1 );
    if ( heal_target[0] != p )
      heal_target.push_back( p );
    else
    {
      for ( player_t* q = sim -> player_list; q; q = q -> next )
      {
        if ( q != p && ! q -> is_pet() && p -> get_player_distance( q ) < ( range * range ) )
        {
          heal_target.push_back( q );
          break;
        }
      }
    }

    priest_heal_t::execute();

    p -> buffs_serendipity -> trigger();

    if ( p -> buffs_surge_of_light -> trigger() )
      p -> procs_surge_of_light -> occur();

    // Inner Focus
    if ( p -> buffs_inner_focus -> up() )
    {
      // Inner Focus cooldown starts when consumed.
      p -> cooldowns_inner_focus -> reset();
      p -> cooldowns_inner_focus -> duration = p -> buffs_inner_focus -> spell_id_t::cooldown();
      p -> cooldowns_inner_focus -> start();
      p -> buffs_inner_focus -> expire();
    }

    // Chakra
    if ( p -> buffs_chakra_pre -> up() )
    {
      p -> buffs_chakra_serenity -> trigger();

      p -> buffs_chakra_pre -> expire();

      p -> cooldowns_chakra -> reset();
      p -> cooldowns_chakra -> duration = p -> buffs_chakra_pre -> spell_id_t::cooldown();
      p -> cooldowns_chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_inspiration( impact_result, t );
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_inner_focus -> check() )
      player_crit += p -> buffs_inner_focus -> effect2().percent();
  }

  virtual double cost() const
  {
    priest_t* p = player -> cast_priest();

    double c = priest_heal_t::cost();

    if ( p -> buffs_inner_focus -> check() )
      c = 0;

    return c;
  }
};

// Circle of Healing ========================================================

struct circle_of_healing_t : public priest_heal_t
{
  circle_of_healing_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "circle_of_healing", p, "Circle of Healing" )
  {
    check_talent( p -> talents.circle_of_healing -> rank() );

    parse_options( NULL, options_str );

    base_cost *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost  = floor( base_cost );

    base_cost *= 1.0 + p -> glyphs.circle_of_healing -> effect2().percent();
    base_cost  = floor( base_cost );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    cooldown -> duration = spell_id_t::cooldown();

    if ( p -> buffs_chakra_sanctuary -> up() )
      cooldown -> duration +=  p -> buffs_chakra_sanctuary -> effect2().time_value();

    // Choose Heal Target
    heal_target.clear();
    heal_target.push_back( find_lowest_player() );
    for ( player_t* q = sim -> player_list; q; q = q -> next )
    {
      if ( !q -> is_pet() && q != heal_target[0] && q -> get_player_distance( target ) < ( range * range ) )
      {
        heal_target.push_back( q );
        if ( heal_target.size() >= ( unsigned ) ( p -> glyphs.circle_of_healing -> ok() ? 6 : 5 ) ) break;
      }
    }

    priest_heal_t::execute();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_inspiration( impact_result, t );
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p -> buffs_chakra_sanctuary -> effect1().percent();
  }
};

// Divine Hymn Spell ========================================================

struct divine_hymn_tick_t : public priest_heal_t
{
  size_t target_count;

  divine_hymn_tick_t( player_t* player ) :
    priest_heal_t( "divine_hymn_tick", player, 64844 ),
    target_count( 0 )
  {
    priest_t* p = player -> cast_priest();

    background  = true;

    base_multiplier *= 1.0 + p -> talents.heavenly_voice -> effect1().percent();
  }

  virtual void execute()
  {
    // Choose Heal Targets
    heal_target.clear();
    heal_target.push_back( find_lowest_player() ); // Find at least the lowest player
    for ( player_t* q = sim -> player_list; q; q = q -> next )
    {
      if ( !q -> is_pet() && q != heal_target[0] )
      {
        heal_target.push_back( q );
        if ( heal_target.size() >= target_count ) break;
      }
    }

    priest_heal_t::execute();
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p -> buffs_chakra_sanctuary -> effect1().percent();
  }
};

struct divine_hymn_t : public priest_heal_t
{
  divine_hymn_tick_t* divine_hymn_tick;

  divine_hymn_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "divine_hymn", p, "Divine Hymn" ),
    divine_hymn_tick( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;
    channeled = true;

    cooldown -> duration += p -> talents.heavenly_voice -> effect2().time_value();

    divine_hymn_tick = new divine_hymn_tick_t( p );
    divine_hymn_tick -> target_count = effect2().base_value();
    add_child( divine_hymn_tick );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_heal_t::execute();

    // Needs testing
    p -> buffs_tier13_2pc_heal -> trigger();
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
    divine_hymn_tick -> execute();
    stats -> add_tick( d -> time_to_tick );
  }
};

// Flash Heal Spell =========================================================

struct flash_heal_t : public priest_heal_t
{
  flash_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "flash_heal", p, "Flash Heal" )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p -> talents.empowered_healing -> base_value( E_APPLY_AURA , A_ADD_PCT_MODIFIER );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_heal_t::execute();

    // TEST: Assuming a SoL Flash Heal can't proc SoL
    if ( p -> buffs_surge_of_light -> up() )
      p -> buffs_surge_of_light -> expire();
    else if ( p -> buffs_surge_of_light -> trigger() )
      p -> procs_surge_of_light -> occur();

    p -> buffs_divine_fire -> trigger();

    p -> buffs_serendipity -> trigger();

    // Inner Focus
    if ( p -> buffs_inner_focus -> up() )
    {
      // Inner Focus cooldown starts when consumed.
      p -> cooldowns_inner_focus -> reset();
      p -> cooldowns_inner_focus -> duration = p -> buffs_inner_focus -> spell_id_t::cooldown();
      p -> cooldowns_inner_focus -> start();
      p -> buffs_inner_focus -> expire();
    }

    // Chakra
    if ( p -> buffs_chakra_pre -> up() )
    {
      p -> buffs_chakra_serenity -> trigger();

      p -> buffs_chakra_pre -> expire();

      p -> cooldowns_chakra -> reset();
      p -> cooldowns_chakra -> duration = p -> buffs_chakra_pre -> spell_id_t::cooldown();
      p -> cooldowns_chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_inspiration( impact_result, t );

    trigger_grace( t );
    trigger_strength_of_soul( t );
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_inner_focus -> up() )
      player_crit += p -> buffs_inner_focus -> effect2().percent();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    priest_heal_t::target_debuff( t, dmg_type );

    priest_t* p = player -> cast_priest();

    if ( t -> buffs.grace -> up() || t -> buffs.weakened_soul -> up() )
      player_crit += p -> talents.renewed_hope -> effect1().percent();
  }

  virtual double cost() const
  {
    double c = priest_heal_t::cost();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_inner_focus -> check() )
      c = 0;

    if ( p -> buffs_surge_of_light -> check() )
      c = 0;

    return c;
  }

  virtual timespan_t execute_time() const
  {
    priest_t* p = player -> cast_priest();

    if ( p -> buffs_surge_of_light -> check() )
      return timespan_t::zero;

    return priest_heal_t::execute_time();
  }
};

// Guardian Spirit ==========================================================

struct guardian_spirit_t : public priest_heal_t
{
  guardian_spirit_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "guardian_spirit", p, "Guardian Spirit" )
  {
    check_talent( p -> talents.guardian_spirit -> ok() );

    parse_options( NULL, options_str );

    // The absorb listed isn't a real absorb
    base_dd_min = base_dd_max = 0;

    harmful = false;

    cooldown -> duration += p -> glyphs.guardian_spirit -> effect1().time_value();
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    target -> buffs.guardian_spirit -> trigger();
    target -> buffs.guardian_spirit -> source = player;
  }
};

// Greater Heal Spell =======================================================

struct greater_heal_t : public priest_heal_t
{
  greater_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "greater_heal", p, "Greater Heal" )
  {
    parse_options( NULL, options_str );

    base_execute_time += p -> talents.divine_fury -> effect1().time_value();

    base_multiplier *= 1.0 + p -> talents.empowered_healing -> base_value( E_APPLY_AURA , A_ADD_PCT_MODIFIER );
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    priest_t* p = player -> cast_priest();

    p -> buffs_serendipity -> expire();

    p -> buffs_divine_fire -> trigger();

    if ( p -> buffs_surge_of_light -> trigger() )
      p -> procs_surge_of_light -> occur();

    // Train of Thought
    // NOTE: Process Train of Thought _before_ Inner Focus: the GH that consumes Inner Focus does not
    //       reduce the cooldown, since Inner Focus doesn't go on cooldown until after it is consumed.
    if ( p -> talents.train_of_thought -> rank() &&
         p -> rng_train_of_thought -> roll( util_t::talent_rank( p -> talents.train_of_thought -> rank(), 2, 0.5, 1.0 ) ) )
    {
      if ( p -> cooldowns_inner_focus -> remains() > timespan_t::from_seconds( p -> talents.train_of_thought -> effect1().base_value() ) )
        p -> cooldowns_inner_focus -> ready -= timespan_t::from_seconds( p -> talents.train_of_thought -> effect1().base_value() );
      else
        p -> cooldowns_inner_focus -> reset();

      p -> procs_train_of_thought_gh -> occur();
    }

    // Inner Focus
    if ( p -> buffs_inner_focus -> up() )
    {
      // Inner Focus cooldown starts when consumed.
      p -> cooldowns_inner_focus -> reset();
      p -> cooldowns_inner_focus -> duration = p -> buffs_inner_focus -> spell_id_t::cooldown();
      p -> cooldowns_inner_focus -> start();
      p -> buffs_inner_focus -> expire();
    }

    // Chakra
    if ( p -> buffs_chakra_pre -> up() )
    {
      p -> buffs_chakra_serenity -> trigger();

      p -> buffs_chakra_pre -> expire();

      p -> cooldowns_chakra -> reset();
      p -> cooldowns_chakra -> duration = p -> buffs_chakra_pre -> spell_id_t::cooldown();
      p -> cooldowns_chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_inspiration( impact_result, t );

    trigger_grace( t );
    trigger_strength_of_soul( t );
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_inner_focus -> up() )
      player_crit += p -> buffs_inner_focus -> effect2().percent();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    priest_heal_t::target_debuff( t, dmg_type );

    priest_t* p = player -> cast_priest();

    if ( t -> buffs.grace -> up() || t -> buffs.weakened_soul -> up() )
      player_crit += p -> talents.renewed_hope -> effect1().percent();
  }

  virtual timespan_t execute_time() const
  {
    priest_t* p = player -> cast_priest();

    timespan_t c = priest_heal_t::execute_time();

    if ( p -> buffs_serendipity -> check() )
      c *= 1.0 + p -> buffs_serendipity -> effect1().percent();

    return c;
  }

  virtual double cost() const
  {
    priest_t* p = player -> cast_priest();

    double c = priest_heal_t::cost();

    if ( p -> buffs_serendipity -> check() )
      c *= 1.0 + p -> buffs_serendipity -> effect2().percent();

    if ( p -> buffs_inner_focus -> check() )
      c = 0;

    return c;
  }
};

// Heal Spell ===============================================================

struct _heal_t : public priest_heal_t
{
  _heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "heal", p, "Heal" )
  {
    parse_options( NULL, options_str );

    base_execute_time += p -> talents.divine_fury -> effect1().time_value();
    base_multiplier *= 1.0 + p -> talents.empowered_healing -> base_value( E_APPLY_AURA , A_ADD_PCT_MODIFIER );

    // FIXME: Check Spell when it's imported into the dbc
    base_crit      += p -> set_bonus.tier11_2pc_heal() * 0.05;
  }

  virtual void execute()
  {
    priest_heal_t::execute();
    priest_t* p = player -> cast_priest();

    if ( p -> buffs_surge_of_light -> trigger() )
      p -> procs_surge_of_light -> occur();

    p -> buffs_divine_fire -> trigger();

    // Trigger Chakra
    if ( p -> buffs_chakra_pre -> up() )
    {
      p -> buffs_chakra_serenity -> trigger();

      p -> buffs_chakra_pre -> expire();

      p -> cooldowns_chakra -> reset();
      p -> cooldowns_chakra -> duration = p -> buffs_chakra_pre -> spell_id_t::cooldown();
      p -> cooldowns_chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_inspiration( impact_result, t );

    trigger_grace( t );
    trigger_strength_of_soul( t );
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    priest_heal_t::target_debuff( t, dmg_type );

    priest_t* p = player -> cast_priest();

    if ( t -> buffs.grace -> up() || t -> buffs.weakened_soul -> up() )
      player_crit += p -> talents.renewed_hope -> effect1().percent();
  }
};

// Holy Word Sanctuary ======================================================

struct holy_word_sanctuary_t : public priest_heal_t
{
  struct holy_word_sanctuary_tick_t : public priest_heal_t
  {
    holy_word_sanctuary_tick_t( player_t* player ) :
      priest_heal_t( "holy_word_sanctuary_tick", player, 88686 )
    {
      dual        = true;
      background  = true;
      direct_tick = true;

      stats = player -> get_stats( "holy_word_sanctuary", this );
    }
    virtual void execute()
    {
      // Choose Heal Targets
      heal_target.clear();
      for ( player_t* q = sim -> player_list; q; q = q -> next )
      {
        if ( !q -> is_pet() && q -> get_player_distance( target ) < ( range * range ) )
        {
          heal_target.push_back( q );
        }
      }

      priest_heal_t::execute();
    }

    virtual void player_buff()
    {
      priest_heal_t::player_buff();

      priest_t* p = player -> cast_priest();

      if ( p -> buffs_chakra_sanctuary -> up() )
        player_multiplier *= 1.0 + p -> buffs_chakra_sanctuary -> effect1().percent();
    }
  };

  holy_word_sanctuary_tick_t* tick_spell;

  holy_word_sanctuary_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "holy_word_sanctuary", p, 88685 ),
    tick_spell( 0 )
  {
    check_talent( p -> talents.revelations -> rank() );

    parse_options( NULL, options_str );

    hasted_ticks = false;
    may_crit     = false;

    base_tick_time = timespan_t::from_seconds( 2.0 );
    num_ticks = 9;

    tick_spell = new holy_word_sanctuary_tick_t( p );

    cooldown -> duration *= 1.0 + p -> talents.tome_of_light -> effect1().percent();

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;

    // HW: Sanctuary is treated as a instant cast spell, both affected by Inner Will and Mental Agility
    // Implemented 06/12/2011 ( Patch 4.3 ),
    // see Issue1023 and http://elitistjerks.com/f77/t110245-cataclysm_holy_priest_compendium/p25/#post2054467
    base_cost        *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost         = floor( base_cost );
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( ! p -> buffs_chakra_sanctuary -> check() )
      return false;

    return priest_heal_t::ready();
  }

  // HW: Sanctuary is treated as a instant cast spell, both affected by Inner Will and Mental Agility

  virtual double cost() const
  {
    priest_t* p = player -> cast_priest();

    double c = priest_heal_t::cost();

    c *= 1.0 - p -> buffs_inner_will -> check() * p -> buffs_inner_will -> effect1().percent();
    c  = floor( c );

    return c;
  }

  virtual void consume_resource()
  {
    priest_heal_t::consume_resource();

    priest_t* p = player -> cast_priest();

    p -> buffs_inner_will -> up();
  }
};

// Holy Word Chastise =======================================================

struct holy_word_chastise_t : public priest_spell_t
{
  holy_word_chastise_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "holy_word_chastise", p, "Holy Word: Chastise" )
  {
    parse_options( NULL, options_str );

    base_cost *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost  = floor( base_cost );

    cooldown -> duration *= 1.0 + p -> talents.tome_of_light -> effect1().percent();

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( p -> talents.revelations -> rank() )
    {
      if ( p -> buffs_chakra_sanctuary -> check() )
        return false;

      if ( p -> buffs_chakra_serenity -> check() )
        return false;
    }

    return priest_spell_t::ready();
  }
};

// Holy Word Serenity =======================================================

struct holy_word_serenity_t : public priest_heal_t
{
  holy_word_serenity_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "holy_word_serenity", p, 88684 )
  {
    check_talent( p -> talents.revelations -> rank() );

    parse_options( NULL, options_str );

    base_cost *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost  = floor( base_cost );

    cooldown -> duration *= 1.0 + p -> talents.tome_of_light -> effect1().percent();

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    priest_t* p = player -> cast_priest();

    p -> buffs_serenity -> trigger();
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( ! p -> buffs_chakra_serenity -> check() )
      return false;

    return priest_heal_t::ready();
  }
};

// Holy Word ================================================================

struct holy_word_t : public priest_spell_t
{
  holy_word_sanctuary_t* hw_sanctuary;
  holy_word_chastise_t*  hw_chastise;
  holy_word_serenity_t*  hw_serenity;

  holy_word_t( player_t* player, const std::string& options_str ) :
    priest_spell_t( "holy_word", player, SCHOOL_HOLY, TREE_HOLY ),
    hw_sanctuary( 0 ), hw_chastise( 0 ), hw_serenity( 0 )
  {
    priest_t* p = player -> cast_priest();

    hw_sanctuary = new holy_word_sanctuary_t( p, options_str );
    hw_chastise  = new holy_word_chastise_t ( p, options_str );
    hw_serenity  = new holy_word_serenity_t ( p, options_str );
  }

  virtual void schedule_execute()
  {
    priest_t* p = player -> cast_priest();

    if ( p -> talents.revelations -> rank() && p -> buffs_chakra_serenity -> up() )
    {
      player -> last_foreground_action = hw_serenity;
      hw_serenity -> schedule_execute();
    }
    else if ( p -> talents.revelations -> rank() && p -> buffs_chakra_sanctuary -> up() )
    {
      player -> last_foreground_action = hw_sanctuary;
      hw_sanctuary -> schedule_execute();
    }
    else
    {
      player -> last_foreground_action = hw_chastise;
      hw_chastise -> schedule_execute();
    }
  }

  virtual void execute()
  {
    assert( 0 );
  }

  virtual bool ready()
  {
    priest_t* p = player -> cast_priest();

    if ( p -> talents.revelations -> rank() && p -> buffs_chakra_serenity -> check() )
      return hw_serenity -> ready();

    else if ( p -> talents.revelations -> rank() && p -> buffs_chakra_sanctuary -> check() )
      return hw_sanctuary -> ready();

    else
      return hw_chastise -> ready();
  }
};

// Lightwell Spell ==========================================================

struct lightwell_t : public priest_spell_t
{
  timespan_t consume_interval;

  lightwell_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "lightwell", p, "Lightwell" ), consume_interval( timespan_t::from_seconds( 10 ) )
  {
    option_t options[] =
    {
      { "consume_interval", OPT_TIMESPAN,     &consume_interval },
      { NULL,               OPT_UNKNOWN, NULL              }
    };
    parse_options( options, options_str );

    harmful = false;

    assert( consume_interval > timespan_t::zero && consume_interval < cooldown -> duration );
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    priest_spell_t::execute();

    p -> pet_lightwell -> get_cooldown( "lightwell_renew" ) -> duration = consume_interval;
    p -> pet_lightwell -> summon( duration() );
  }
};

// Penance Heal Spell =======================================================

struct penance_heal_tick_t : public priest_heal_t
{
  penance_heal_tick_t( player_t* player ) :
    priest_heal_t( "penance_heal_tick", player, 47750 )
  {
    background  = true;
    may_crit    = true;
    dual        = true;
    direct_tick = true;

    stats = player -> get_stats( "penance_heal", this );
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    priest_t* p = player -> cast_priest();

    p -> buffs_indulgence_of_the_penitent -> trigger();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_inspiration( impact_result, t );
    trigger_grace( t );
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    priest_heal_t::target_debuff( t, dmg_type );

    priest_t* p = player -> cast_priest();

    if ( t -> buffs.grace -> up() || t -> buffs.weakened_soul -> up() )
      player_crit += p -> talents.renewed_hope -> effect1().percent();
  }
};

struct penance_heal_t : public priest_heal_t
{
  penance_heal_tick_t* penance_tick;

  penance_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "penance_heal", p, "Penance" ), penance_tick( 0 )
  {
    check_spec( TREE_DISCIPLINE );

    parse_options( NULL, options_str );

    may_crit       = false;
    channeled      = true;
    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

    cooldown = p -> cooldowns_penance;
    cooldown -> duration = spell_id_t::cooldown() + p -> glyphs.penance -> effect1().time_value();

    penance_tick = new penance_heal_tick_t( p );
    penance_tick -> target = target;
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
    penance_tick -> heal_target.clear();
    penance_tick -> heal_target.push_back( target );
    penance_tick -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual double cost() const
  {
    double c = priest_heal_t::cost();

    priest_t* p = player -> cast_priest();

    c *= 1.0 + ( p -> buffs_holy_evangelism -> check() * p -> buffs_holy_evangelism -> effect2().percent() );

    return c;
  }
};

// Power Word: Shield Spell =================================================

struct glyph_power_word_shield_t : public priest_heal_t
{
  glyph_power_word_shield_t( player_t* player ) :
    priest_heal_t( "power_word_shield_glyph", player, 55672 )
  {
    school          = SCHOOL_HOLY;
    stats -> school = school;

    background = true;
    proc       = true;
  }
};

struct power_word_shield_t : public priest_absorb_t
{
  glyph_power_word_shield_t* glyph_pws;
  int ignore_debuff;

  power_word_shield_t( priest_t* p, const std::string& options_str ) :
    priest_absorb_t( "power_word_shield", p, "Power Word: Shield" ), glyph_pws( 0 ), ignore_debuff( 0 )
  {
    option_t options[] =
    {
      { "ignore_debuff", OPT_BOOL,    &ignore_debuff },
      { NULL,            OPT_UNKNOWN, NULL           }
    };
    parse_options( options, options_str );

    // Soul Warding does not lower the CD to 1; instead it takes us
    // down to the GCD.  If the player is 2/2 in Soul Warding, set our
    // cooldown to 0 instead of to 1.0.
    cooldown -> duration += p -> talents.soul_warding -> effect1().time_value();
    if ( p -> talents.soul_warding -> rank() == 2 )
    {
      cooldown -> duration = timespan_t::zero;
    }

    base_cost *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost  = floor( base_cost );

    base_multiplier *= 1.0 + p -> talents.improved_power_word_shield -> effect1().percent();

    direct_power_mod = 0.87; // hardcoded into tooltip

    if ( p -> glyphs.power_word_shield -> ok() )
    {
      glyph_pws = new glyph_power_word_shield_t( p );
      glyph_pws -> target = target;
      add_child( glyph_pws );
    }
  }

  virtual void player_buff()
  {
    priest_t* p = player -> cast_priest();

    priest_absorb_t::player_buff();

    // Needs testing
    if ( p -> set_bonus.tier13_4pc_heal() )
      if ( p -> rng_tier13_4pc_heal -> roll( 0.1 ) )
        player_multiplier *= 2.0;
  }

  virtual void execute()
  {
    priest_absorb_t::execute();

    priest_t* p = player -> cast_priest();

    p -> buffs_borrowed_time -> trigger();

    // Rapture
    if ( p -> cooldowns_rapture -> remains() == timespan_t::zero && p -> talents.rapture -> rank() )
    {
      p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * p -> talents.rapture -> effect1().percent(), p -> gains_rapture );
      p -> cooldowns_rapture -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_t* p = player -> cast_priest();

    t -> buffs.weakened_soul -> trigger();

    // Get PWS buff:
    buff_t* pws = 0;
    for ( unsigned int i = 0; i < t -> buffs.power_word_shield.size(); i++ )
    {
      if ( t -> buffs.power_word_shield[ i ] -> initial_source == player )
      {
        pws = t -> buffs.power_word_shield[ i ];
        break;
      }
    }
    assert( pws );

    pws -> trigger( 1, travel_dmg );
    stats -> add_result( travel_dmg, travel_dmg, STATS_ABSORB, impact_result );

    // Glyph
    if ( glyph_pws )
    {
      glyph_pws -> base_dd_min  = glyph_pws -> base_dd_max  = p -> glyphs.power_word_shield -> effect1().percent() * travel_dmg;
      glyph_pws -> heal_target.clear();
      glyph_pws -> heal_target.push_back( t );
      glyph_pws -> execute();
    }

    // Body and Soul
    if ( p -> talents.body_and_soul -> rank() )
      t -> buffs.body_and_soul -> trigger( 1, p -> talents.body_and_soul -> effect1().percent() );
  }

  virtual bool ready()
  {
    if ( ! ignore_debuff && target -> buffs.weakened_soul -> check() )
      return false;

    return priest_absorb_t::ready();
  }
};

// Prayer of Healing Spell ==================================================

struct glyph_prayer_of_healing_t : public priest_heal_t
{
  glyph_prayer_of_healing_t( player_t* player ) :
    priest_heal_t( "prayer_of_healing_glyph", player, 56161 )
  {
    proc       = true;
    background = true;
    may_crit   = false;

    num_ticks = 0; // coded as DD for now.
  }
};

struct prayer_of_healing_t : public priest_heal_t
{
  glyph_prayer_of_healing_t* glyph;

  prayer_of_healing_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "prayer_of_healing", p, "Prayer of Healing" ), glyph( 0 )
  {
    parse_options( NULL, options_str );

    if ( p -> glyphs.prayer_of_healing -> ok() )
    {
      glyph = new glyph_prayer_of_healing_t( p );
      add_child( glyph );
    }
  }

  virtual void init()
  {
    priest_heal_t::init();

    // PoH crits double the DA percentage.
    if ( da ) da -> shield_multiple *= 2;
  }

  virtual void execute()
  {
    priest_t* p = player -> cast_priest();

    // Choose Heal Targets
    heal_target.resize( 1 ); // Only leave the first target, reset the others.
    for ( player_t* q = sim -> player_list; q; q = q -> next )
    {
      if ( !q -> is_pet() && q != heal_target[0] && q -> get_player_distance( heal_target[0] ) < ( range * range ) && q -> party == heal_target[0] -> party )
      {
        heal_target.push_back( q );
        if ( heal_target.size() >= 5 ) break;
      }
    }

    priest_heal_t::execute();

    p -> buffs_serendipity -> expire();

    // Inner Focus
    if ( p -> buffs_inner_focus -> up() )
    {
      // Inner Focus cooldown starts when consumed.
      p -> cooldowns_inner_focus -> reset();
      p -> cooldowns_inner_focus -> duration = p -> buffs_inner_focus -> spell_id_t::cooldown();
      p -> cooldowns_inner_focus -> start();
      p -> buffs_inner_focus -> expire();
    }

    // Chakra
    if ( p -> buffs_chakra_pre -> up() )
    {
      p -> buffs_chakra_sanctuary -> trigger();

      p -> buffs_chakra_pre -> expire();

      p -> cooldowns_chakra -> reset();
      p -> cooldowns_chakra -> duration = p -> buffs_chakra_pre -> spell_id_t::cooldown();
      p -> cooldowns_chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_inspiration( impact_result, t );

    // Glyph
    if ( glyph )
    {
      priest_t* p = player -> cast_priest();

      // FIXME: Modelled as a Direct Heal instead of a dot
      glyph -> heal_target.clear(); glyph -> heal_target.push_back( t );
      glyph -> base_dd_min = glyph -> base_dd_max = travel_dmg * p -> glyphs.prayer_of_healing -> effect1().percent();
      glyph -> execute();
    }

    // Divine Aegis
    if ( impact_result != RESULT_CRIT )
    {
      trigger_divine_aegis( t, travel_dmg * 0.5 );
    }
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_inner_focus -> up() )
      player_crit += p -> buffs_inner_focus -> effect2().percent();

    if ( p -> buffs_chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p -> buffs_chakra_sanctuary -> effect1().percent();
  }

  virtual timespan_t execute_time() const
  {
    priest_t* p = player -> cast_priest();

    timespan_t c = priest_heal_t::execute_time();

    if ( p -> buffs_serendipity -> check() )
      c *= 1.0 + p -> buffs_serendipity -> effect1().percent();

    return c;
  }

  virtual double cost() const
  {
    priest_t* p = player -> cast_priest();

    double c = priest_heal_t::cost();

    if ( p -> buffs_serendipity -> check() )
      c *= 1.0 + p -> buffs_serendipity -> effect2().percent();

    if ( p -> buffs_inner_focus -> check() )
      c = 0;

    return c;
  }
};

// Prayer of Mending Spell ==================================================

struct prayer_of_mending_t : public priest_heal_t
{
  int single;
  prayer_of_mending_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "prayer_of_mending", p, "Prayer of Mending" ), single( false )
  {
    option_t options[] =
    {
      { "single", OPT_BOOL,       &single },
      { NULL,     OPT_UNKNOWN, NULL       }
    };
    parse_options( options, options_str );

    direct_power_mod = effect_coeff( 1 );
    base_dd_min = base_dd_max = effect_min( 1 );

    base_cost *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost  = floor( base_cost );

    can_trigger_DA = false;
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p -> buffs_chakra_sanctuary -> effect1().percent();
  }

  virtual void execute()
  {
    // Set Heal Targets
    heal_target.resize( 1 ); // Only leave the first target, reset the others.
    if ( ! single )
    {
      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        if ( !p -> is_pet() && p != heal_target[0] )
        {
          heal_target.push_back( p );
          if ( heal_target.size() >= 5 ) break;
        }
      }
    }

    priest_heal_t::execute();

    priest_t* p = player -> cast_priest();

    p -> buffs_divine_fire -> trigger();

    // Chakra
    if ( p -> buffs_chakra_pre -> up() )
    {
      p -> buffs_chakra_sanctuary -> trigger();

      p -> buffs_chakra_pre -> expire();

      p -> cooldowns_chakra -> reset();
      p -> cooldowns_chakra -> duration = p -> buffs_chakra_pre -> spell_id_t::cooldown();
      p -> cooldowns_chakra -> start();
    }
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    priest_heal_t::target_debuff( t, dmg_type );

    priest_t* p = player -> cast_priest();

    if ( p -> glyphs.prayer_of_mending -> ok() && t == heal_target[0] )
      target_multiplier *= 1.0 + p -> glyphs.prayer_of_mending -> effect1().percent();
  }
};

// Renew Spell ==============================================================

struct divine_touch_t : public priest_heal_t
{
  divine_touch_t( player_t* player ) :
    priest_heal_t( "divine_touch", player, 63544 )
  {
    school          = SCHOOL_HOLY;
    stats -> school = school;

    background = true;
    proc       = true;
  }
};

struct renew_t : public priest_heal_t
{
  divine_touch_t* dt;

  renew_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "renew", p, "Renew" ), dt( 0 )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    base_cost *= 1.0 + p -> talents.mental_agility -> mod_additive( P_RESOURCE_COST );
    base_cost  = floor( base_cost );

    base_multiplier *= 1.0 + p -> glyphs.renew -> effect1().percent();
    base_multiplier *= 1.0 + p -> talents.improved_renew -> effect1().percent();

    // Implement Divine Touch
    if ( p -> talents.divine_touch -> rank() )
    {
      dt = new divine_touch_t( p );
      add_child( dt );
    }

    if ( p -> talents.rapid_renewal -> rank() )
      trigger_gcd += p -> talents.rapid_renewal -> effect1().time_value();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    // Divine Touch
    if ( dt )
    {
      priest_t* p = player -> cast_priest();
      dt -> base_dd_min = dt -> base_dd_max = ( base_td + total_power() * tick_power_mod ) * hasted_num_ticks() * p -> talents.divine_touch -> effect1().percent();
      dt -> heal_target.clear();
      dt -> heal_target.push_back( t );
      dt -> execute();
    }
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    priest_t* p = player -> cast_priest();

    if ( p -> buffs_chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p -> buffs_chakra_sanctuary -> effect1().percent();
  }
};

// Tier 12 Healer 2pc Bonus =================================================

// Implementation not 100% correct. When the buff expires and is reapplied between 2 events, the event is doubled, which then doubles the mana gain.

// Event
struct tier12_heal_2pc_event_t : public event_t
{
  buff_t* buff;

  tier12_heal_2pc_event_t ( player_t* player,buff_t* b ) :
    event_t( player -> sim, player ), buff( 0 )
  {
    buff = b;
    name = "tier12_heal_2pc";
    sim -> add_event( this, timespan_t::from_seconds( 5.0 ) );
  }

  virtual void execute()
  {
    if ( buff -> check() )
    {
      priest_t* p = player -> cast_priest();
      player -> resource_gain( RESOURCE_MANA, player -> resource_base[ RESOURCE_MANA ] * p -> sets -> set( SET_T12_2PC_HEAL ) -> effect_base_value( 1 ) / 100.0, p -> gains_divine_fire );
      new ( sim ) tier12_heal_2pc_event_t( player, buff );
    }
  }
};

// Buff

struct tier12_heal_2pc_buff_t : public buff_t
{
  tier12_heal_2pc_buff_t( player_t* p, const uint32_t id, const std::string& n, double c ) :
    buff_t ( p, id, n, c )
  { }

  virtual void start( int stacks, double value )
  {
    new ( sim ) tier12_heal_2pc_event_t( player, this );
    buff_t::start( stacks, value );
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Priest Character Definition
// ==========================================================================

// priest_t::empowered_shadows_amount

double priest_t::empowered_shadows_amount() const
{
  double a = shadow_orb_amount();

  a += 0.10;

  return a;
}

// priest_t::shadow_orb_amount

double priest_t::shadow_orb_amount() const
{
  double a = composite_mastery() * constants.shadow_orb_mastery_value;

  return a;
}

void priest_t::trigger_cauterizing_flame()
{
  // Assuming you can only have 1 Cauterizing Flame
  if ( pet_cauterizing_flame && pet_cauterizing_flame -> sleeping &&
       rng_cauterizing_flame -> roll( sets -> set( SET_T12_4PC_HEAL ) -> proc_chance()  ) )
  {
    pet_cauterizing_flame -> summon( dbc.spell( 99136 ) -> duration() );
  }
}

// priest_t::primary_role

int priest_t::primary_role() const
{
  switch( player_t::primary_role() )
  {
  case ROLE_HEAL:
    return ROLE_HEAL;
  case ROLE_DPS:
  case ROLE_SPELL:
    return ROLE_SPELL;
  default:
    if ( primary_tree() == TREE_DISCIPLINE || primary_tree() == TREE_HOLY )
      return ROLE_HEAL;
    break;
  }

  return ROLE_SPELL;
}

// priest_t::composite_armor ================================================

double priest_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buffs_inner_fire -> up() )
    a *= 1.0 + buffs_inner_fire -> base_value( E_APPLY_AURA, A_MOD_RESISTANCE_PCT ) / 100.0 * ( 1.0 + glyphs.inner_fire -> effect1().percent() );

  return floor( a );
}

// priest_t::composite_spell_power ==========================================

double priest_t::composite_spell_power( const school_type school ) const
{
  double sp = player_t::composite_spell_power( school );

  if ( buffs_inner_fire -> up() )
    sp += buffs_inner_fire -> effect_min( 2 );

  return sp;
}

// priest_t::composite_spell_hit ============================================

double priest_t::composite_spell_hit() const
{
  double hit = player_t::composite_spell_hit();

  hit += ( ( spirit() - attribute_base[ ATTR_SPIRIT ] ) * constants.twisted_faith_dynamic_value ) / rating.spell_hit;

  return hit;
}

// priest_t::composite_spell_haste ==========================================

double priest_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  h *= constants.darkness_value;

  if ( buffs_borrowed_time -> up() )
    h *= constants.borrowed_time_value;

  return h;
}

// priest_t::composite_player_multiplier ====================================

double priest_t::composite_player_multiplier( const school_type school, action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( spell_id_t::is_school( school, SCHOOL_SHADOW ) )
  {
    m *= 1.0 + buffs_shadow_form -> check() * constants.shadow_form_value;
    m *= 1.0 + constants.twisted_faith_static_value;
  }
  if ( spell_id_t::is_school( school, SCHOOL_SHADOWLIGHT ) )
  {
    m *= 1.0 + constants.twin_disciplines_value;

    if ( buffs_chakra_chastise -> up() )
    {
      m *= 1.0 + 0.15;
    }
  }
  if ( spell_id_t::is_school( school, SCHOOL_MAGIC ) )
  {
    m *= 1.0 + constants.shadow_power_damage_value;
  }

  return m;
}

double priest_t::composite_player_td_multiplier( const school_type school, action_t* a ) const
{
  double player_multiplier = player_t::composite_player_td_multiplier( school, a );

  if ( school == SCHOOL_SHADOW )
  {
    // Shadow TD
    player_multiplier += buffs_empowered_shadow -> value();
    player_multiplier += buffs_dark_evangelism -> stack () * buffs_dark_evangelism -> effect1().percent();
  }

  return player_multiplier;
}

// priest_t::composite_player_heal_multiplier ===============================

double priest_t::composite_player_heal_multiplier( const school_type school ) const
{
  double m = player_t::composite_player_heal_multiplier( school );

  if ( spell_id_t::is_school( school, SCHOOL_SHADOWLIGHT ) )
    m *= 1.0 + constants.twin_disciplines_value;

  return m;
}

// priest_t::composite_movement_speed =======================================

double priest_t::composite_movement_speed() const
{
  double speed = player_t::composite_movement_speed();

  if ( buffs_inner_will -> up() )
    speed *= 1.0 + buffs_inner_will -> effect2().percent() + talents.inner_sanctum -> effect2().percent();

  return speed;
}

// priest_t::matching_gear_multiplier =======================================

double priest_t::matching_gear_multiplier( const attribute_type attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// priest_t::spirit =========================================================

double priest_t::spirit() const
{
  double spi = player_t::spirit();

  if ( set_bonus.tier11_4pc_heal() & ( buffs_chakra_serenity -> up() || buffs_chakra_sanctuary -> up() || buffs_chakra_chastise -> up() ) )
    spi += 540;

  return spi;
}

// priest_t::create_action ==================================================

action_t* priest_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  // Misc
  if ( name == "archangel"              ) return new archangel_t             ( this, options_str );
  if ( name == "chakra"                 ) return new chakra_t                ( this, options_str );
  if ( name == "dispersion"             ) return new dispersion_t            ( this, options_str );
  if ( name == "fortitude"              ) return new fortitude_t             ( this, options_str );
  if ( name == "hymn_of_hope"           ) return new hymn_of_hope_t          ( this, options_str );
  if ( name == "inner_fire"             ) return new inner_fire_t            ( this, options_str );
  if ( name == "inner_focus"            ) return new inner_focus_t           ( this, options_str );
  if ( name == "inner_will"             ) return new inner_will_t            ( this, options_str );
  if ( name == "pain_suppression"       ) return new pain_suppression_t      ( this, options_str );
  if ( name == "power_infusion"         ) return new power_infusion_t        ( this, options_str );
  if ( name == "shadow_form"            ) return new shadow_form_t           ( this, options_str );
  if ( name == "vampiric_embrace"       ) return new vampiric_embrace_t      ( this, options_str );

  // Damage
  if ( name == "devouring_plague"       ) return new devouring_plague_t      ( this, options_str );
  if ( name == "holy_fire"              ) return new holy_fire_t             ( this, options_str );
  if ( name == "mind_blast"             ) return new mind_blast_t            ( this, options_str );
  if ( name == "mind_flay"              ) return new mind_flay_t             ( this, options_str );
  if ( name == "mind_spike"             ) return new mind_spike_t            ( this, options_str );
  if ( name == "mind_sear"              ) return new mind_sear_t             ( this, options_str );
  if ( name == "penance"                ) return new penance_t               ( this, options_str );
  if ( name == "shadow_word_death"      ) return new shadow_word_death_t     ( this, options_str );
  if ( name == "shadow_word_pain"       ) return new shadow_word_pain_t      ( this, options_str );
  if ( name == "smite"                  ) return new smite_t                 ( this, options_str );
  if ( name == "shadow_fiend"           ) return new shadow_fiend_spell_t    ( this, options_str );
  if ( name == "vampiric_touch"         ) return new vampiric_touch_t        ( this, options_str );

  // Heals
  if ( name == "binding_heal"           ) return new binding_heal_t          ( this, options_str );
  if ( name == "circle_of_healing"      ) return new circle_of_healing_t     ( this, options_str );
  if ( name == "divine_hymn"            ) return new divine_hymn_t           ( this, options_str );
  if ( name == "flash_heal"             ) return new flash_heal_t            ( this, options_str );
  if ( name == "greater_heal"           ) return new greater_heal_t          ( this, options_str );
  if ( name == "guardian_spirit"        ) return new guardian_spirit_t       ( this, options_str );
  if ( name == "heal"                   ) return new _heal_t                 ( this, options_str );
  if ( name == "holy_word"              ) return new holy_word_t             ( this, options_str );
  if ( name == "lightwell"              ) return new lightwell_t             ( this, options_str );
  if ( name == "penance_heal"           ) return new penance_heal_t          ( this, options_str );
  if ( name == "power_word_shield"      ) return new power_word_shield_t     ( this, options_str );
  if ( name == "prayer_of_healing"      ) return new prayer_of_healing_t     ( this, options_str );
  if ( name == "prayer_of_mending"      ) return new prayer_of_mending_t     ( this, options_str );
  if ( name == "renew"                  ) return new renew_t                 ( this, options_str );

  return player_t::create_action( name, options_str );
}

// priest_t::create_pet =====================================================

pet_t* priest_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "shadow_fiend" ) return new shadow_fiend_pet_t( sim, this );
  if ( pet_name == "lightwell" ) return new lightwell_pet_t( sim, this );
  if ( pet_name == "cauterizing_flame" ) return new cauterizing_flame_pet_t( sim, this );

  return 0;
}

// priest_t::create_pets ====================================================

void priest_t::create_pets()
{
  pet_shadow_fiend      = create_pet( "shadow_fiend"      );
  pet_cauterizing_flame = create_pet( "cauterizing_flame" );
  pet_lightwell         = create_pet( "lightwell"         );
}

// priest_t::init_base ======================================================

void priest_t::init_base()
{
  player_t::init_base();

  base_attack_power = -10;
  attribute_multiplier_initial[ ATTR_INTELLECT ]   *= 1.0 + passive_spells.enlightenment -> effect1().percent();

  initial_attack_power_per_strength = 1.0;
  initial_spell_power_per_intellect = 1.0;

  mana_per_intellect = 15;

  diminished_kfactor    = 0.009830;
  diminished_dodge_capi = 0.006650;
  diminished_parry_capi = 0.006650;
}

// priest_t::init_gains =====================================================

void priest_t::init_gains()
{
  player_t::init_gains();

  gains_dispersion                = get_gain( "dispersion" );
  gains_shadow_fiend              = get_gain( "shadow_fiend" );
  gains_archangel                 = get_gain( "archangel" );
  gains_masochism                 = get_gain( "masochism" );
  gains_rapture                   = get_gain( "rapture" );
  gains_hymn_of_hope              = get_gain( "hymn_of_hope" );
  gains_divine_fire               = get_gain( "divine_fire" );
}

// priest_t::init_procs =====================================================

void priest_t::init_procs()
{
  player_t::init_procs();

  procs_shadowy_apparation     = get_proc( "shadowy_apparation_proc" );
  procs_surge_of_light         = get_proc( "surge_of_light" );
  procs_train_of_thought_gh    = get_proc( "train_of_thought_gh" );
  procs_train_of_thought_smite = get_proc( "train_of_thought_smite" );
}

// priest_t::init_scaling ===================================================

void priest_t::init_scaling()
{
  player_t::init_scaling();

  // An Atonement Priest might be Health-capped
  scales_with[ STAT_STAMINA ] = talents.atonement -> ok();

  // For a Shadow Priest Spirit is the same as Hit Rating so invert it.
  if ( ( talents.twisted_faith -> rank() ) && ( sim -> scaling -> scale_stat == STAT_SPIRIT ) )
  {
    double v = sim -> scaling -> scale_value;

    if ( ! sim -> scaling -> positive_scale_delta )
    {
      invert_scaling = 1;
      attribute_initial[ ATTR_SPIRIT ] -= v * 2;
    }
  }
}

// priest_t::init_uptimes ===================================================

void priest_t::init_benefits()
{
  player_t::init_benefits();

  uptimes_mind_spike[ 0 ] = get_benefit( "mind_spike_0" );
  uptimes_mind_spike[ 1 ] = get_benefit( "mind_spike_1" );
  uptimes_mind_spike[ 2 ] = get_benefit( "mind_spike_2" );
  uptimes_mind_spike[ 3 ] = get_benefit( "mind_spike_3" );

  uptimes_dark_flames     = get_benefit( "dark_flames" );

  uptimes_shadow_orb[ 0 ] = get_benefit( "Percentage of Mind Blasts benefiting from 0 Shadow Orbs" );
  uptimes_shadow_orb[ 1 ] = get_benefit( "Percentage of Mind Blasts benefiting from 1 Shadow Orbs" );
  uptimes_shadow_orb[ 2 ] = get_benefit( "Percentage of Mind Blasts benefiting from 2 Shadow Orbs" );
  uptimes_shadow_orb[ 3 ] = get_benefit( "Percentage of Mind Blasts benefiting from 3 Shadow Orbs" );

  uptimes_test_of_faith = get_benefit( "test_of_faith" );
}

// priest_t::init_rng =======================================================

void priest_t::init_rng()
{
  player_t::init_rng();

  rng_pain_and_suffering = get_rng( "pain_and_suffering" );
  rng_cauterizing_flame  = get_rng( "rng_cauterizing_flame" );
  rng_train_of_thought   = get_rng( "train_of_thought" );
  rng_tier13_4pc_heal    = get_rng( "tier13_4pc_heal" );
}

// priest_t::init_talents ===================================================

void priest_t::init_talents()
{
  // Discipline
  talents.improved_power_word_shield  = find_talent( "Improved Power Word: Shield" );
  talents.twin_disciplines            = find_talent( "Twin Disciplines" );
  talents.mental_agility              = find_talent( "Mental Agility" );
  talents.evangelism                  = find_talent( "Evangelism" );
  talents.archangel                   = find_talent( "Archangel" );
  talents.inner_sanctum               = find_talent( "Inner Sanctum" );
  talents.soul_warding                = find_talent( "Soul Warding" );
  talents.renewed_hope                = find_talent( "Renewed Hope" );
  talents.power_infusion              = find_talent( "Power Infusion" );
  talents.atonement                   = find_talent( "Atonement" );
  talents.inner_focus                 = find_talent( "Inner Focus" );
  talents.rapture                     = find_talent( "Rapture" );
  talents.borrowed_time               = find_talent( "Borrowed Time" );
  talents.strength_of_soul            = find_talent( "Strength of Soul" );
  talents.divine_aegis                = find_talent( "Divine Aegis" );
  talents.pain_suppression            = find_talent( "Pain Suppression" );
  talents.train_of_thought            = find_talent( "Train of Thought" );
  talents.grace                       = find_talent( "Grace" );
  talents.power_word_barrier          = find_talent( "Power Word: Barrier" );

  // Holy
  talents.improved_renew              = find_talent( "Improved Renew" );
  talents.empowered_healing           = find_talent( "Empowered Healing" );
  talents.divine_fury                 = find_talent( "Divine Fury" );
  // Desperate Prayer
  talents.surge_of_light              = find_talent( "Surge of Light" );
  talents.inspiration                 = find_talent( "Inspiration" );
  talents.divine_touch                = find_talent( "Divine Touch" );
  talents.holy_concentration          = find_talent( "Holy Concentration" );
  talents.lightwell                   = find_talent( "Lightwell" );
  talents.tome_of_light               = find_talent( "Tome of Light" );
  talents.rapid_renewal               = find_talent( "Rapid Renewal" );
  talents.serendipity                 = find_talent( "Serendipity" );
  talents.body_and_soul               = find_talent( "Body and Soul" );
  talents.chakra                      = find_talent( "Chakra" );
  talents.revelations                 = find_talent( "Revelations" );
  talents.test_of_faith               = find_talent( "Test of Faith" );
  talents.heavenly_voice              = find_talent( "Heavenly Voice" );
  talents.circle_of_healing           = find_talent( "Circle of Healing" );
  talents.guardian_spirit             = find_talent( "Guardian Spirit" );

  // Shadow
  talents.darkness                    = find_talent( "Darkness" );
  talents.improved_devouring_plague   = find_talent( "Improved Devouring Plague" );
  talents.improved_mind_blast         = find_talent( "Improved Mind Blast" );
  talents.mind_melt                   = find_talent( "Mind Melt" );
  talents.dispersion                  = find_talent( "Dispersion" );
  talents.improved_shadow_word_pain   = find_talent( "Improved Shadow Word: Pain" );
  talents.pain_and_suffering          = find_talent( "Pain and Suffering" );
  talents.masochism                   = find_talent( "Masochism" );
  talents.shadow_form                 = find_talent( "Shadowform" );
  talents.twisted_faith               = find_talent( "Twisted Faith" );
  talents.veiled_shadows              = find_talent( "Veiled Shadows" );
  talents.harnessed_shadows           = find_talent( "Harnessed Shadows" );
  talents.shadowy_apparition          = find_talent( "Shadowy Apparition" );
  talents.vampiric_embrace            = find_talent( "Vampiric Embrace" );
  talents.vampiric_touch              = find_talent( "Vampiric Touch" );
  talents.sin_and_punishment          = find_talent( "Sin and Punishment" );
  talents.paralysis                   = find_talent( "Paralysis" );
  talents.phantasm                    = find_talent( "Phantasm" );

  player_t::init_talents();
}

// priest_t::init_spells
void priest_t::init_spells()
{
  player_t::init_spells();

  // Passive Spells

  // Discipline
  passive_spells.enlightenment          = new passive_spell_t( this, "enlightenment", "Enlightenment" );
  passive_spells.meditation_disc        = new passive_spell_t( this, "meditation_disc", "Meditation" );

  // Holy
  passive_spells.spiritual_healing      = new passive_spell_t( this, "spiritual_healing", "Spiritual Healing" );
  passive_spells.meditation_holy        = new passive_spell_t( this, "meditation_holy", 95861 );

  // Shadow
  passive_spells.shadow_power           = new passive_spell_t( this, "shadow_power", "Shadow Power" );
  passive_spells.shadow_orbs            = new passive_spell_t( this, "shadow_orbs", "Shadow Orbs" );
  passive_spells.shadowy_apparition_num = new passive_spell_t( this, "shadowy_apparition_num", 78202 );

  // Mastery Spells
  mastery_spells.shield_discipline = new mastery_t( this, "shield_discipline", "Shield Discipline", TREE_DISCIPLINE );
  mastery_spells.echo_of_light     = new mastery_t( this, "echo_of_light", "Echo of Light", TREE_HOLY );
  mastery_spells.shadow_orb_power  = new mastery_t( this, "shadow_orb_power", "Shadow Orb Power", TREE_SHADOW );

  // Active Spells
  active_spells.mind_spike      = new active_spell_t( this, "mind_spike", "Mind Spike" );
  active_spells.shadow_fiend    = new active_spell_t( this, "shadow_fiend", "Shadowfiend" );
  active_spells.holy_archangel  = new active_spell_t( this, "holy_archangel", 87152 );
  active_spells.holy_archangel2 = new active_spell_t( this, "holy_archangel2", 81700 );
  active_spells.dark_archangel  = new active_spell_t( this, "dark_archangel", 87153 );

  dark_flames                   = spell_data_t::find( 99158, "Dark Flames", dbc.ptr );

  // Glyphs
  glyphs.circle_of_healing  = find_glyph( "Glyph of Circle of Healing" );
  glyphs.dispersion         = find_glyph( "Glyph of Dispersion" );
  glyphs.divine_accuracy    = find_glyph( "Glyph of Divine Accuracy" );
  glyphs.guardian_spirit    = find_glyph( "Glyph of Guardian Spirit" );
  glyphs.holy_nova          = find_glyph( "Glyph of Holy Nova" );
  glyphs.inner_fire         = find_glyph( "Glyph of Inner Fire" );
  glyphs.lightwell          = find_glyph( "Glyph of Lightwell" );
  glyphs.mind_flay          = find_glyph( "Glyph of Mind Flay" );
  glyphs.penance            = find_glyph( "Glyph of Penance" );
  glyphs.power_word_shield  = find_glyph( "Glyph of Power Word: Shield" );
  glyphs.prayer_of_healing  = find_glyph( "Glyph of Prayer of Healing" );
  glyphs.prayer_of_mending  = find_glyph( "Glyph of Prayer of Mending" );
  glyphs.renew              = find_glyph( "Glyph of Renew" );
  glyphs.shadow_word_death  = find_glyph( "Glyph of Shadow Word: Death" );
  glyphs.shadow_word_pain   = find_glyph( "Glyph of Shadow Word: Pain" );
  glyphs.smite              = find_glyph( "Glyph of Smite" );
  glyphs.spirit_tap         = find_glyph( "Glyph of Spirit Tap" );

  // Set Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P    M2P    M4P    T2P    T4P     H2P     H4P
    {  89915,  89922,     0,     0,     0,     0,  89910,  89911 }, // Tier11
    {  99154,  99157,     0,     0,     0,     0,  99134,  99135 }, // Tier12
    { 105843, 105844,     0,     0,     0,     0, 105827, 105832 }, // Tier13
    {      0,      0,     0,     0,     0,     0,      0,      0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// priest_t::init_buffs =====================================================

void priest_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )

  // Discipline
  buffs_borrowed_time              = new buff_t( this, talents.borrowed_time -> effect1().trigger_spell_id(), "borrowed_time", talents.borrowed_time -> rank() );
  // TEST: buffs_borrowed_time -> activated = false;
  buffs_dark_evangelism            = new buff_t( this, talents.evangelism -> rank() == 2 ? 87118 : 87117, "dark_evangelism", talents.evangelism -> rank() );
  buffs_dark_evangelism -> activated = false;
  buffs_holy_evangelism            = new buff_t( this, talents.evangelism -> rank() == 2 ? 81661 : 81660, "holy_evangelism", talents.evangelism -> rank() );
  buffs_holy_evangelism -> activated = false;
  buffs_dark_archangel             = new buff_t( this, 87153, "dark_archangel" );
  buffs_holy_archangel             = new buff_t( this, 81700, "holy_archangel" );
  buffs_inner_fire                 = new buff_t( this, "inner_fire", "Inner Fire" );
  buffs_inner_focus                = new buff_t( this, "inner_focus", "Inner Focus" );
  buffs_inner_focus -> cooldown -> duration = timespan_t::zero;
  buffs_inner_will                 = new buff_t( this, "inner_will", "Inner Will" );
  buffs_tier13_2pc_heal            = new buff_t( this, sets -> set( SET_T13_2PC_HEAL ) -> effect1().trigger_spell_id(), "tier13_2pc_heal", set_bonus.tier13_2pc_heal() );
  buffs_tier13_2pc_heal -> buff_duration = ( primary_tree() == TREE_DISCIPLINE ) ? timespan_t::from_seconds( 10.0 ) : buffs_tier13_2pc_heal -> buff_duration;

  // TODO: probably these should be moved into targetdata
  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    if ( talents.divine_aegis -> rank() )
    {
      buff_t* b = new buff_t( actor_pair_t( p, this ), 47753, "divine_aegis" );
      p -> buffs.divine_aegis.push_back( b );
      p -> absorb_buffs.push_back( b );
    }

    buff_t* b  = new buff_t( actor_pair_t( p, this ), 17, "power_word_shield" );
    p -> buffs.power_word_shield.push_back( b );
    p -> absorb_buffs.push_back( b );
  }

  // Holy
  buffs_chakra_pre                 = new buff_t( this, 14751, "chakra_pre" );
  buffs_chakra_chastise            = new buff_t( this, 81209, "chakra_chastise" );
  buffs_chakra_sanctuary           = new buff_t( this, 81206, "chakra_sanctuary" );
  buffs_chakra_serenity            = new buff_t( this, 81208, "chakra_serenity" );
  buffs_serendipity                = new buff_t( this, talents.serendipity -> effect1().trigger_spell_id(), "serendipity", talents.serendipity -> rank() );
  // TEST: buffs_serendipity -> activated = false;
  buffs_serenity                   = new buff_t( this, 88684, "serenity" );
  buffs_serenity -> cooldown -> duration = timespan_t::zero;
  // TEST: buffs_serenity -> activated = false;
  buffs_surge_of_light             = new buff_t( this, talents.surge_of_light, NULL );
  buffs_surge_of_light -> activated = false;
  // TEST: buffs_surge_of_light -> activated = false;

  // Shadow
  buffs_empowered_shadow           = new buff_t( this, 95799, "empowered_shadow" );
  buffs_glyph_of_shadow_word_death = new buff_t( this, "glyph_of_shadow_word_death", 1, timespan_t::from_seconds( 6.0 )  );
  buffs_mind_melt                  = new buff_t( this, talents.mind_melt -> effect2().trigger_spell_id(), "mind_melt"                 );
  buffs_mind_spike                 = new buff_t( this, "mind_spike",                 3, timespan_t::from_seconds( 12.0 ) );
  buffs_shadow_form                = new buff_t( this, "shadow_form", "Shadowform" );
  buffs_shadow_orb                 = new buff_t( this, passive_spells.shadow_orbs -> effect1().trigger_spell_id(), "shadow_orb" );
  buffs_shadow_orb -> activated = false;
  buffs_shadowfiend                = new buff_t( this, "shadowfiend", 1, timespan_t::from_seconds( 15.0 ) ); // Pet Tracking Buff
  buffs_glyph_of_spirit_tap        = new buff_t( this, 81301, "glyph_of_spirit_tap" ); // FIXME: implement actual mechanics
  buffs_vampiric_embrace           = new buff_t( this, talents.vampiric_embrace, NULL );

  // Set Bonus
  buffs_indulgence_of_the_penitent = new buff_t( this, 89913, "indulgence_of_the_penitent", set_bonus.tier11_4pc_heal() );
  buffs_divine_fire = new tier12_heal_2pc_buff_t( this, 99132, "divine_fire", set_bonus.tier12_2pc_heal() );
  buffs_cauterizing_flame          = new buff_t( this, "cauterizing_flame",          1                           );
}

// priest_t::init_actions ===================================================

void priest_t::init_actions()
{
  std::string& list_default = get_action_priority_list( "default" ) -> action_list_str;

  std::string& list_double_dot = get_action_priority_list( "double_dot" ) -> action_list_str;
  std::string& list_pws = get_action_priority_list( "pws" ) -> action_list_str;
  //std::string& list_aaa = get_action_priority_list( "aaa" ) -> action_list_str;
  //std::string& list_poh = get_action_priority_list( "poh" ) -> action_list_str;
  std::string buffer;

  if ( buffer.empty() )
  {
    if ( level > 80 )
    {
      buffer = "flask,type=draconic_mind/food,type=seafood_magnifique_feast";
    }
    else if ( level >= 75 )
    {
      buffer = "flask,type=frost_wyrm/food,type=fish_feast";
    }

    buffer += "/fortitude/inner_fire";

    if ( talents.shadow_form -> rank() )
    {
      buffer += "/shadow_form";
    }

    if ( talents.vampiric_embrace -> rank() )
      buffer += "/vampiric_embrace";

    buffer += "/snapshot_stats";

    buffer += init_use_item_actions();

    buffer += init_use_profession_actions();

    // Save & reset buffer =================================================
    for ( unsigned int i = 0; i < action_priority_list.size(); i++ )
    {
      action_priority_list_t* a = action_priority_list[i];
      a -> action_list_str += buffer;
    }
    buffer.clear();
    // ======================================================================

    switch ( primary_tree() )
    {
      // SHADOW =============================================================
    case TREE_SHADOW:
      if ( level > 80 )
      {
        buffer += "/volcanic_potion,if=!in_combat";
        buffer += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      }
      else if ( level >= 70 )
      {
        buffer += "/wild_magic_potion,if=!in_combat";
        buffer += "/speed_potion,if=buff.bloodlust.react|target.time_to_die<=20";
      }

      buffer += "/mind_blast";
      buffer += init_use_racial_actions();
      buffer += "/shadow_word_pain,if=(!ticking|dot.shadow_word_pain.remains<gcd+0.5)&miss_react";

      if ( level >= 28 )
        buffer += "/devouring_plague,if=(!ticking|dot.devouring_plague.remains<gcd+1.0)&miss_react";

      buffer += "/stop_moving,health_percentage<=25,if=cooldown.shadow_word_death.remains>=0.2";

      if ( talents.vampiric_touch -> rank() )
        buffer += "|dot.vampiric_touch.remains<cast_time+2.5";

      if ( talents.vampiric_touch -> rank() )
        buffer += "/vampiric_touch,if=(!ticking|dot.vampiric_touch.remains<cast_time+2.5)&miss_react";

      // Save  & reset buffer ===============================================
      for ( unsigned int i = 0; i < action_priority_list.size(); i++ )
      {
        action_priority_list_t* a = action_priority_list[i];
        a -> action_list_str += buffer;
      }
      buffer.clear();
      // ====================================================================

      if ( talents.vampiric_touch -> rank() )
        list_double_dot += "/vampiric_touch_2,if=(!ticking|dot.vampiric_touch_2.remains<cast_time+2.5)&miss_react";

      list_double_dot += "/shadow_word_pain_2,if=(!ticking|dot.shadow_word_pain_2.remains<gcd+0.5)&miss_react";


      if ( talents.archangel -> ok() )
      {
        buffer += "/archangel,if=buff.dark_evangelism.stack>=5";
        if ( talents.vampiric_touch -> rank() )
          buffer += "&dot.vampiric_touch.remains>5";

        buffer += "&dot.devouring_plague.remains>5";
      }

      buffer += "/start_moving,health_percentage<=25,if=cooldown.shadow_word_death.remains<=0.1";

      buffer += "/shadow_word_death,health_percentage<=25";
      if ( level >= 66 )
        buffer += "/shadow_fiend";

      // Save  & reset buffer ===============================================
      for ( unsigned int i = 0; i < action_priority_list.size(); i++ )
      {
        action_priority_list_t* a = action_priority_list[i];
        a -> action_list_str += buffer;
      }
      buffer.clear();
      // ====================================================================

      list_double_dot += "/mind_flay_2,if=(dot.shadow_word_pain_2.remains<dot.shadow_word_pain.remains)&miss_react";

      buffer += "/shadow_word_death,if=mana_pct<10";
      buffer += "/mind_flay";
      buffer += "/shadow_word_death,moving=1";
      if ( talents.improved_devouring_plague -> rank() )
        buffer += "/devouring_plague,moving=1,if=mana_pct>10";

      if ( talents.dispersion -> rank() )
        buffer += "/dispersion";
      break;
      // SHADOW END =========================================================


      // DISCIPLINE =========================================================
    case TREE_DISCIPLINE:

      // DAMAGE DISCIPLINE ==================================================
      if ( primary_role() != ROLE_HEAL )
      {
        buffer += "/volcanic_potion,if=!in_combat|buff.bloodlust.up|time_to_die<=40";
        if ( race == RACE_BLOOD_ELF )
          buffer += "/arcane_torrent,if=mana_pct<=90";
        if ( level >= 66 )
          buffer += "/shadow_fiend,if=mana_pct<=60";
        if ( level >= 64 )
          buffer += "/hymn_of_hope";
        if ( level >= 66 )
          buffer += ",if=pet.shadow_fiend.active&mana_pct<=20";
        if ( race == RACE_TROLL )
          buffer += "/berserking";
        if ( talents.power_infusion -> rank() )
          buffer += "/power_infusion";
        if ( talents.archangel -> ok() )
          buffer += "/archangel,if=buff.holy_evangelism.stack>=5";
        if ( talents.rapture -> ok() )
          buffer += "/power_word_shield,if=buff.weakened_soul.down";

        if ( level >= 28 )
          buffer += "/devouring_plague,if=miss_react&(remains<tick_time|!ticking)";

        buffer += "/shadow_word_pain,if=miss_react&(remains<tick_time|!ticking)";

        if ( talents.borrowed_time -> ok() )
          buffer += "/penance,if=buff.borrowed_time.up";

        buffer += "/holy_fire";
        buffer += "/penance";
        buffer += "/mind_blast";

        buffer += "/smite";
      }
      // DAMAGE DISCIPLINE END ==============================================

      // HEALER DISCIPLINE ==================================================
      else
      {
        // DEFAULT
        list_default += "/mana_potion,if=mana_pct<=75";
        if ( race == RACE_BLOOD_ELF )
          list_default  += "/arcane_torrent,if=mana_pct<=90";
        if ( level >= 66 )
          list_default += "/shadow_fiend,if=mana_pct<=20";
        if ( level >= 64 )
          list_default += "/hymn_of_hope";
        if ( level >= 66 )
          list_default += ",if=pet.shadow_fiend.active";
        if ( race == RACE_TROLL )
          list_default += "/berserking";
        if ( talents.inner_focus ->ok() )
          list_default += "/inner_focus";
        if ( talents.power_infusion -> ok() )
          list_default += "/power_infusion";
        list_default += "/power_word_shield";
        if ( talents.rapture -> ok() )
          list_default += ",if=!cooldown.rapture.remains";
        if ( talents.archangel -> ok() )
          list_default += "/archangel,if=buff.holy_evangelism.stack>=5";
        if ( talents.borrowed_time -> ok() )
        {
          list_default += "/penance_heal,if=buff.borrowed_time.up";
          if ( talents.grace -> ok() )
            list_default += "|buff.grace.down";
        }
        if ( talents.inner_focus -> ok() )
          list_default += "/greater_heal,if=buff.inner_focus.up";
        if ( talents.archangel -> ok() )
        {
          list_default += "/holy_fire";
          if ( talents.atonement -> ok() )
          {
            list_default += "/smite,if=";
            if ( glyphs.smite -> ok() )
              list_default += "dot.holy_fire.remains>cast_time&";
            list_default += "buff.holy_evangelism.stack<5&buff.holy_archangel.down";
          }
        }
        list_default += "/penance_heal";
        list_default += "/greater_heal";
        // DEFAULT END

        // PWS
        list_pws += "/mana_potion,if=mana_pct<=75";
        if ( race == RACE_BLOOD_ELF )
          list_pws  += "/arcane_torrent,if=mana_pct<=90";
        if ( level >= 66 )
          list_pws += "/shadow_fiend,if=mana_pct<=20";
        if ( level >= 64 )
          list_pws += "/hymn_of_hope";
        if ( level >= 66 )
          list_pws += ",if=pet.shadow_fiend.active";
        if ( race == RACE_TROLL )
          list_pws += "/berserking";
        if ( talents.inner_focus ->ok() )
          list_pws += "/inner_focus";
        if ( talents.power_infusion -> ok() )
          list_pws += "/power_infusion";

        list_pws += "/power_word_shield,ignore_debuff=1";

        // PWS END
      }
      break;
      // HEALER DISCIPLINE END ==============================================

      // HOLY
    case TREE_HOLY:
      // DAMAGE DEALER
      if ( primary_role() != ROLE_HEAL )
      {
                                                         list_default += "/mana_potion,if=mana_pct<=75";
        if ( race == RACE_BLOOD_ELF )                    list_default += "/arcane_torrent,if=mana_pct<=90";
        if ( level >= 66 )                               list_default += "/shadow_fiend,if=mana_pct<=50";
        if ( level >= 64 )                               list_default += "/hymn_of_hope";
        if ( level >= 66 )                               list_default += ",if=pet.shadow_fiend.active&time>200";
        if ( race == RACE_TROLL )                        list_default += "/berserking";
        if ( talents.chakra -> ok() )                    list_default += "/chakra";
        if ( talents.archangel -> ok() )                 list_default += "/archangel,if=buff.holy_evangelism.stack>=5";
                                                         list_default += "/holy_fire";
        if ( level >= 28 )                               list_default += "/devouring_plague,if=remains<tick_time|!ticking";
                                                         list_default += "/shadow_word_pain,if=remains<tick_time|!ticking";
        if ( ! talents.archangel -> ok() )               list_default += "/mind_blast";
                                                         list_default += "/smite";
      }
      // HEALER
      else
      {
                                                         list_default += "/mana_potion,if=mana_pct<=75";
        if ( race == RACE_BLOOD_ELF )                    list_default += "/arcane_torrent,if=mana_pct<80";
        if ( level >= 66 )                               list_default += "/shadow_fiend,if=mana_pct<=20";
        if ( level >= 64 )                               list_default += "/hymn_of_hope";
        if ( level >= 66 )                               list_default += ",if=pet.shadow_fiend.active";
        if ( race == RACE_TROLL )                        list_default += "/berserking";
      }
      break;
    default:
                                                         list_default += "/mana_potion,if=mana_pct<=75";
      if ( level >= 66 )                                 list_default += "/shadow_fiend,if=mana_pct<=50";
      if ( level >= 64 )                                 list_default += "/hymn_of_hope";
      if ( level >= 66 )                                 list_default += ",if=pet.shadow_fiend.active&time>200";
      if ( race == RACE_TROLL )                          list_default += "/berserking";
      if ( race == RACE_BLOOD_ELF )                      list_default += "/arcane_torrent,if=mana_pct<=90";
      if ( talents.archangel -> ok() )                   list_default += "/archangel,if=buff.holy_evangelism.stack>=5";
                                                         list_default += "/holy_fire";
      if ( level >= 28 )                                 list_default += "/devouring_plague,if=remains<tick_time|!ticking";
                                                         list_default += "/shadow_word_pain,if=remains<tick_time|!ticking";
                                                         list_default += "/mind_blast";
                                                         list_default += "/smite";
      break;
    }

    // Save & reset buffer =================================================
    for ( unsigned int i = 0; i < action_priority_list.size(); i++ )
    {
      action_priority_list_t* a = action_priority_list[i];
      a -> action_list_str += buffer;
    }
    buffer.clear();
    // ======================================================================

    action_list_default = 1;
  }

  player_t::init_actions();

  for ( action_t* a = action_list; a; a = a -> next )
  {
    double c = a -> cost();
    if ( c > max_mana_cost ) max_mana_cost = c;
  }
}

// priest_t::init_party =====================================================

void priest_t::init_party()
{
  party_list.clear();

  if ( party == 0 )
    return;

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( ( p != this ) && ( p -> party == party ) && ( ! p -> quiet ) && ( ! p -> is_pet() ) )
    {
      party_list.push_back( p );
    }
  }
}

// priest_t::init_values ====================================================

void priest_t::init_values()
{
  player_t::init_values();

  // Discipline/Holy
  constants.meditation_value                = passive_spells.meditation_disc    -> ok() ?
                                              passive_spells.meditation_disc  -> base_value( E_APPLY_AURA, A_MOD_MANA_REGEN_INTERRUPT ) :
                                              passive_spells.meditation_holy  -> base_value( E_APPLY_AURA, A_MOD_MANA_REGEN_INTERRUPT );

  // Discipline
  constants.twin_disciplines_value          = talents.twin_disciplines          -> base_value( E_APPLY_AURA, A_MOD_DAMAGE_PERCENT_DONE );
  constants.dark_archangel_damage_value     = active_spells.dark_archangel      -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 22 );
  constants.dark_archangel_mana_value       = active_spells.dark_archangel      -> effect_base_value( 3 ) / 100.0;
  constants.holy_archangel_value            = active_spells.holy_archangel2     -> base_value( E_APPLY_AURA, A_MOD_HEALING_DONE_PERCENT ) / 100.0;
  constants.archangel_mana_value            = active_spells.holy_archangel      -> base_value( E_ENERGIZE_PCT );
  constants.borrowed_time_value             = 1.0 / ( 1.0 + talents.borrowed_time -> effect1().percent() );

  // Holy
  constants.holy_concentration_value        = talents.holy_concentration        -> effect1().percent();

  // Shadow Core
  constants.shadow_power_damage_value       = passive_spells.shadow_power       -> effect1().percent();
  constants.shadow_power_crit_value         = passive_spells.shadow_power       -> effect2().percent();
  constants.shadow_orb_proc_value           = mastery_spells.shadow_orb_power   -> proc_chance();
  constants.shadow_orb_mastery_value        = mastery_spells.shadow_orb_power   -> base_value( E_APPLY_AURA, A_DUMMY, P_GENERIC );

  // Shadow
  constants.darkness_value                  = 1.0 / ( 1.0 + talents.darkness    -> effect1().percent() );
  constants.improved_shadow_word_pain_value = talents.improved_shadow_word_pain -> effect1().percent();
  constants.twisted_faith_static_value      = talents.twisted_faith             -> effect2().percent();
  constants.twisted_faith_dynamic_value     = talents.twisted_faith             -> effect1().percent();
  constants.shadow_form_value               = talents.shadow_form               -> effect2().percent();
  constants.harnessed_shadows_value         = talents.harnessed_shadows         -> effect1().percent();
  constants.pain_and_suffering_value        = talents.pain_and_suffering        -> proc_chance();
  constants.devouring_plague_health_mod     = 0.15;

  constants.max_shadowy_apparitions         = passive_spells.shadowy_apparition_num -> effect1().base_value();

  mana_regen_while_casting = constants.meditation_value + constants.holy_concentration_value;

  if ( set_bonus.pvp_2pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 90;

  if ( set_bonus.pvp_2pc_heal() )
    attribute_initial[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_heal() )
    attribute_initial[ ATTR_INTELLECT ] += 90;
}

// priest_t::reset ==========================================================

void priest_t::reset()
{
  player_t::reset();

  if ( talents.shadowy_apparition -> rank() )
  {
    while ( shadowy_apparition_active_list.size() )
    {
      spell_t* s = shadowy_apparition_active_list.front();

      shadowy_apparition_active_list.pop_front();

      shadowy_apparition_free_list.push( s );
    }

    priest_spell_t::add_more_shadowy_apparitions( this );
  }

  echo_of_light_merged = false;

  was_sub_25 = false;

  mana_resource.reset();

  heals_echo_of_light                  = 0;

  init_party();
}

// priest_t::demise =========================================================

void priest_t::demise()
{
  player_t::demise();
}

// priest_t::fixup_atonement_stats  =========================================

void priest_t::fixup_atonement_stats( const char* trigger_spell_name,
                                      const char* atonement_spell_name )
{
  if ( stats_t* trigger = get_stats( trigger_spell_name ) )
  {
    if ( stats_t* atonement = get_stats( atonement_spell_name ) )
    {
      // Copy stats from the trigger spell to the atonement spell
      // to get proper HPR and HPET reports.
      atonement -> resource_consumed = trigger -> resource_consumed;
      atonement -> total_execute_time = trigger -> total_execute_time;
      atonement -> total_tick_time = trigger -> total_tick_time;
    }
  }
}

// priest_t::pre_analyze_hook  ==============================================

void priest_t::pre_analyze_hook()
{
  if ( talents.atonement -> rank() )
  {
    fixup_atonement_stats( "smite", "atonement_smite" );
    fixup_atonement_stats( "holy_fire", "atonement_holy_fire" );
  }
}

// priest_t::resource_gain ==================================================

double priest_t::resource_gain( int       resource,
                                double    amount,
                                gain_t*   source,
                                action_t* action )
{
  double actual_amount = player_t::resource_gain( resource, amount, source, action );

  if ( resource == RESOURCE_MANA )
  {
    if ( source != gains_shadow_fiend &&
         source != gains_dispersion )
    {
      mana_resource.mana_gain += actual_amount;
    }
  }

  return actual_amount;
}

// priest_t::resource_loss ==================================================

double priest_t::resource_loss( int       resource,
                                double    amount,
                                action_t* action )
{
  double actual_amount = player_t::resource_loss( resource, amount, action );

  if ( resource == RESOURCE_MANA )
  {
    mana_resource.mana_loss += actual_amount;
  }

  return actual_amount;
}

// priest_t::target_mitigation ==============================================

double priest_t::target_mitigation( double            amount,
                                    const school_type school,
                                    int               dmg_type,
                                    int               result,
                                    action_t*         action )
{
  amount = player_t::target_mitigation( amount, school, dmg_type, result, action );

  if ( buffs_shadow_form -> check() )
  {
    amount *= 1.0 + buffs_shadow_form -> effect3().percent();
  }

  if ( talents.inner_sanctum -> rank() )
  {
    amount *= 1.0 - talents.inner_sanctum -> effect1().percent();
  }

  return amount;
}

// priest_t::create_options =================================================

void priest_t::create_options()
{
  player_t::create_options();

  option_t priest_options[] =
  {
    { "atonement_target",        OPT_STRING, &( atonement_target_str      ) },
    { "double_dot",              OPT_DEPRECATED, ( void* ) "action_list=double_dot"   },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, priest_options );
}

// priest_t::create_profile =================================================

bool priest_t::create_profile( std::string& profile_str, int save_type, bool save_html )
{
  player_t::create_profile( profile_str, save_type, save_html );

  if ( save_type == SAVE_ALL )
  {
    if ( ! atonement_target_str.empty() )
      profile_str += "atonement_target=" + atonement_target_str + "\n";
  }

  return true;
}

// priest_t::copy_from ======================================================

void priest_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  priest_t* source_p = source -> cast_priest();

  atonement_target_str      = source_p -> atonement_target_str;
}

// priest_t::decode_set =====================================================

int priest_t::decode_set( item_t& item )
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

  bool is_caster = false;
  bool is_healer = false;

  if ( strstr( s, "mercurial" ) )
  {
    is_caster = ( strstr( s, "hood"          ) ||
                  strstr( s, "shoulderwraps" ) ||
                  strstr( s, "vestment"      ) ||
                  strstr( s, "gloves"        ) ||
                  strstr( s, "leggings"      ) );
    if ( is_caster ) return SET_T11_CASTER;

    is_healer = ( strstr( s, "cowl"          ) ||
                  strstr( s, "mantle"        ) ||
                  strstr( s, "robes"         ) ||
                  strstr( s, "handwraps"     ) ||
                  strstr( s, "legwraps"      ) );
    if ( is_healer ) return SET_T11_HEAL;
  }

  if ( strstr( s, "_of_the_cleansing_flame" ) )
  {
    is_caster = ( strstr( s, "hood"          ) ||
                  strstr( s, "shoulderwraps" ) ||
                  strstr( s, "vestment"      ) ||
                  strstr( s, "gloves"        ) ||
                  strstr( s, "leggings"      ) );
    if ( is_caster ) return SET_T12_CASTER;

    is_healer = ( strstr( s, "cowl"          ) ||
                  strstr( s, "mantle"        ) ||
                  strstr( s, "robes"         ) ||
                  strstr( s, "handwraps"     ) ||
                  strstr( s, "legwraps"      ) );
    if ( is_healer ) return SET_T12_HEAL;
  }

  if ( strstr( s, "_of_dying_light" ) )
  {
    is_caster = ( strstr( s, "hood"          ) ||
                  strstr( s, "shoulderwraps" ) ||
                  strstr( s, "vestment"      ) ||
                  strstr( s, "gloves"        ) ||
                  strstr( s, "leggings"      ) );

    /* FIX-ME: Kludge due to caster shoulders and chests have the wrong name */
    const char* t = item.encoded_stats_str.c_str();

    if ( ( ( item.slot == SLOT_SHOULDERS ) && strstr( t, "crit"  ) ) ||
         ( ( item.slot == SLOT_CHEST     ) && strstr( t, "haste" ) ) )
    {
      is_caster = 1;
    }

    if ( is_caster ) return SET_T13_CASTER;




    is_healer = ( strstr( s, "cowl"          ) ||
                  strstr( s, "mantle"        ) ||
                  strstr( s, "robes"         ) ||
                  strstr( s, "handwraps"     ) ||
                  strstr( s, "legwraps"      ) );
    if ( is_healer ) return SET_T13_HEAL;
  }

  if ( strstr( s, "_gladiators_mooncloth_" ) ) return SET_PVP_HEAL;
  if ( strstr( s, "_gladiators_satin_"     ) ) return SET_PVP_CASTER;

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_priest  =================================================

player_t* player_t::create_priest( sim_t* sim, const std::string& name, race_type r )
{
  return new priest_t( sim, name, r );
}

// player_t::priest_init ====================================================

void player_t::priest_init( sim_t* sim )
{
  sim -> auras.mind_quickening = new aura_t( sim, "mind_quickening", 1, timespan_t::zero );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.fortitude        = new stat_buff_t( p, "fortitude", STAT_STAMINA, floor( sim -> dbc.effect_average( sim -> dbc.spell( 79104 ) -> effect1().id(), sim -> max_player_level ) ), ! p -> is_pet() );
    p -> buffs.guardian_spirit  = new      buff_t( p, 47788, "guardian_spirit", 1.0, timespan_t::zero ); // Let the ability handle the CD
    p -> buffs.pain_supression  = new      buff_t( p, 33206, "pain_supression", 1.0, timespan_t::zero ); // Let the ability handle the CD
    p -> buffs.power_infusion   = new      buff_t( p, "power_infusion", 1, timespan_t::from_seconds( 15.0 ), timespan_t::zero );
    p -> buffs.inspiration      = new      buff_t( p, "inspiration", 1, timespan_t::from_seconds( 15.0 ), timespan_t::zero );
    p -> buffs.weakened_soul    = new      buff_t( p, 6788, "weakened_soul" );
  }
}

// player_t::priest_combat_begin ============================================

void player_t::priest_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.mind_quickening ) sim -> auras.mind_quickening -> override();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.fortitude )
        p -> buffs.fortitude -> override( 1, floor( sim -> dbc.effect_average( sim -> dbc.spell( 79104 ) -> effect1().id(), sim -> max_player_level ) ) );
    }
  }
}
