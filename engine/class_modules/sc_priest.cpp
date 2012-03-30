// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#if SC_PRIEST == 1

namespace {
struct remove_dots_event_t;
}

struct priest_targetdata_t : public targetdata_t
{
  dot_t*  dots_shadow_word_pain;
  dot_t*  dots_vampiric_touch;
  dot_t*  dots_devouring_plague;
  dot_t*  dots_holy_fire;
  dot_t*  dots_renew;

  buff_t* buffs_power_word_shield;
  buff_t* buffs_divine_aegis;

  remove_dots_event_t* remove_dots_event;

  priest_targetdata_t( player_t* source, player_t* target )
    : targetdata_t( source, target ), remove_dots_event( NULL )
  {
    buffs_power_word_shield = add_aura( new buff_t( this, 17, "power_word_shield" ) );
    target -> absorb_buffs.push_back( buffs_power_word_shield );
    buffs_divine_aegis = add_aura( new buff_t( this, 47753, "divine_aegis" ) );
    target -> absorb_buffs.push_back( buffs_divine_aegis );
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

  REGISTER_BUFF( power_word_shield );
  REGISTER_BUFF( divine_aegis );
}

// ==========================================================================
// Priest
// ==========================================================================

struct priest_t : public player_t
{
  // Buffs

  struct buffs_t
  {
    // Discipline
    buff_t* dark_evangelism;
    buff_t* holy_evangelism;
    buff_t* dark_archangel;
    buff_t* holy_archangel;
    buff_t* inner_fire;
    buff_t* inner_focus;
    buff_t* inner_will;

    // Holy
    buff_t* chakra_pre;
    buff_t* chakra_chastise;
    buff_t* chakra_sanctuary;
    buff_t* chakra_serenity;
    buff_t* serenity;

    // Shadow
    buff_t* empowered_shadow;
    buff_t* glyph_of_shadow_word_death;
    buff_t* mind_spike;
    buff_t* glyph_mind_spike;
    buff_t* shadowform;
    buff_t* shadow_orb;
    buff_t* shadowfiend;
    buff_t* glyph_of_atonement;
    buff_t* vampiric_embrace;

    // Set Bonus
  } buffs;

  // Talents
  struct talents_list_t
  {
    talent_t* void_tendrils;
    talent_t* psyfiend;
    talent_t* dominate_mind;
    talent_t* body_and_soul;
    talent_t* path_of_the_devout;
    talent_t* phantasm;
    talent_t* from_darkness_comes_light;
    // "coming soon"
    talent_t* archangel;
    talent_t* desperate_prayer;
    talent_t* void_shift;
    talent_t* angelic_bulwark;
    talent_t* twist_of_fate;
    talent_t* power_infusion;
    talent_t* divine_insight;
    talent_t* cascade;
    talent_t* divine_star;
    talent_t* halo;
  };
  talents_list_t talents;

  // Specialization Spells
  struct specialization_spells_t
  {
    // Discipline
    spell_id_t* enlightenment;
    spell_id_t* meditation_disc;
    spell_id_t* divine_aegis;
    spell_id_t* grace;
    spell_id_t* evangelism;
    spell_id_t* train_of_thought;

    // Holy
    spell_id_t* spiritual_healing;
    spell_id_t* meditation_holy;
    spell_id_t* revelations;

    // Shadow
    spell_id_t* shadow_power;
    spell_id_t* shadow_orbs;
    spell_id_t* shadowy_apparition_num;
    spell_id_t* twisted_faith;
    spell_id_t* shadowform;
  };
  specialization_spells_t spec;

  // Mastery Spells
  struct mastery_spells_t
  {
    mastery_t* shield_discipline;
    mastery_t* echo_of_light;
    mastery_t* shadow_orb_power;
  };
  mastery_spells_t mastery_spells;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* mind_blast;
    cooldown_t* shadow_fiend;
    cooldown_t* chakra;
    cooldown_t* inner_focus;
    cooldown_t* penance;
  } cooldowns;

  // Gains
  struct gains_t
  {
    gain_t* dispersion;
    gain_t* shadow_fiend;
    gain_t* archangel;
    gain_t* hymn_of_hope;
  } gains;

  // Benefits
  struct benefits_t
  {
    benefit_t* mind_spike[ 4 ];
    benefit_t* shadow_orb[ 4 ];
  } benefits;

  // Procs
  struct procs_t
  {
    proc_t* shadowy_apparation;
  } procs;

  // Special
  std::queue<spell_t*> shadowy_apparition_free_list;
  std::list<spell_t*>  shadowy_apparition_active_list;
  heal_t* heals_echo_of_light;
  bool echo_of_light_merged;

  // Random Number Generators
  struct rngs_t
  {
  } rngs;

  // Pets
  struct pets_t
  {
    pet_t* shadow_fiend;
    pet_t* lightwell;
  } pets;

  // Options
  std::string atonement_target_str;

  std::vector<player_t *> party_list;

  // Glyphs
  struct glyphs_t
  {
    glyph_t* circle_of_healing;
    glyph_t* dispersion;
    glyph_t* holy_nova;
    glyph_t* inner_fire;
    glyph_t* lightwell;
    glyph_t* penance;
    glyph_t* power_word_shield;
    glyph_t* prayer_of_mending;
    glyph_t* renew;
    glyph_t* smite;

    // Mop
    glyph_t* atonement;
    glyph_t* mind_spike;
    glyph_t* strength_of_soul;
    glyph_t* inner_sanctum;
  };
  glyphs_t glyphs;

  // Constants
  struct constants_t
  {
    double meditation_value;

    // Discipline
    double inner_will_value;

    // Shadow
    double shadow_power_damage_value;
    double shadow_power_crit_value;
    double shadow_orb_proc_value;
    double shadow_orb_damage_value;
    double shadow_orb_mastery_value;

    double twisted_faith_static_value;
    double twisted_faith_dynamic_value;
    double shadowform_value;

    double devouring_plague_health_mod;

    uint32_t max_shadowy_apparitions;

    constants_t() { memset( ( void * ) this, 0x0, sizeof( constants_t ) ); }
  };
  constants_t constants;

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

    cooldowns.mind_blast                 = get_cooldown( "mind_blast" );
    cooldowns.shadow_fiend               = get_cooldown( "shadow_fiend" );
    cooldowns.chakra                     = get_cooldown( "chakra"   );
    cooldowns.inner_focus                = get_cooldown( "inner_focus" );
    cooldowns.penance                    = get_cooldown( "penance" );

    pets.shadow_fiend                     = NULL;
    pets.lightwell                        = NULL;

    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target )
  { return new priest_targetdata_t( source, target ); }
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
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( item_t& item );
  virtual resource_type primary_resource() const { return RESOURCE_MANA; }
  virtual int       primary_role() const;
  virtual double    composite_armor() const;
  virtual double    composite_spell_power( const school_type school ) const;
  virtual double    composite_spell_hit() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    composite_player_td_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    composite_movement_speed() const;

  virtual double    matching_gear_multiplier( const attribute_type attr ) const;

  virtual double    target_mitigation( double amount, const school_type school, int type, int result, action_t* a=0 );

  virtual double    empowered_shadows_amount() const;
  virtual double    shadow_orb_amount() const;

  void fixup_atonement_stats( const char* trigger_spell_name, const char* atonement_spell_name );
  virtual void pre_analyze_hook();
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
  priest_absorb_t( const std::string& n, priest_t* player, const char* sname, int t = TREE_NONE ) :
    absorb_t( n.c_str(), player, sname, t )
  {
    _init_priest_absorb_t();
  }

  priest_absorb_t( const std::string& n, priest_t* player, const uint32_t id, int t = TREE_NONE ) :
    absorb_t( n.c_str(), player, id, t )
  {
    _init_priest_absorb_t();
  }

  priest_t* p() const
  { return static_cast<priest_t*>( player ); }

  virtual void player_buff()
  {
    absorb_t::player_buff();

    player_multiplier *= 1.0 + ( p() -> composite_mastery() * p() -> mastery_spells.shield_discipline -> ok() * 2.5 / 100.0 );
  }

  virtual double cost() const
  {
    double c = absorb_t::cost();

    if ( ( base_execute_time <= timespan_t::zero ) && ! channeled )
    {
      c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> effect1().percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    absorb_t::consume_resource();

    if ( base_execute_time <= timespan_t::zero )
      p() -> buffs.inner_will -> up();
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
      check_talent( p->spec.divine_aegis->ok() );

      proc             = true;
      background       = true;
      direct_power_mod = 0;

      shield_multiple  = p -> spec.divine_aegis -> effect1().percent();
    }

    virtual void impact( player_t* t, int impact_result, double travel_dmg )
    {
      priest_targetdata_t* td = targetdata() -> cast_priest();

      double old_amount = td -> buffs_divine_aegis -> current_value;
      double new_amount = std::min( t -> resources.current[ RESOURCE_HEALTH ] * 0.4 - old_amount, travel_dmg );
      td -> buffs_divine_aegis -> trigger( 1, old_amount + new_amount );
      stats -> add_result( sim -> report_overheal ? new_amount : travel_dmg, travel_dmg, STATS_ABSORB, impact_result );
    }
  };

  bool can_trigger_DA;
  divine_aegis_t* da;
  cooldown_t* min_interval;

  void trigger_echo_of_light( priest_heal_t* a, player_t* /* t */ )
  {
    if ( ! a -> p() -> mastery_spells.echo_of_light -> ok() ) return;

    struct echo_of_light_t : public priest_heal_t
    {
      echo_of_light_t( priest_t* p ) :
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

    if ( ! p() -> heals_echo_of_light ) p() -> heals_echo_of_light = new echo_of_light_t( a -> p() );

    if ( p() -> heals_echo_of_light -> dot() -> ticking )
    {
      if ( p() -> echo_of_light_merged )
      {
        // The old tick_dmg is multiplied by the remaining ticks, added to the new complete heal, and then again divided by num_ticks
        p() -> heals_echo_of_light -> base_td = ( p() -> heals_echo_of_light -> base_td *
                                                p() -> heals_echo_of_light -> dot() -> ticks() +
                                                a -> direct_dmg * p() -> composite_mastery() *
                                                p() -> mastery_spells.echo_of_light -> effect_base_value( 2 ) / 10000.0 ) /
                                                p() -> heals_echo_of_light -> num_ticks;
        p() -> heals_echo_of_light -> dot() -> refresh_duration();
      }
      else
      {
        // The old tick_dmg is multiplied by the remaining ticks minus one!, added to the new complete heal, and then again divided by num_ticks
        p() -> heals_echo_of_light -> base_td = ( p() -> heals_echo_of_light -> base_td *
                                                ( p() -> heals_echo_of_light -> dot() -> ticks() - 1 ) +
                                                a -> direct_dmg * p() -> composite_mastery() *
                                                p() -> mastery_spells.echo_of_light -> effect_base_value( 2 ) / 10000.0 ) /
                                                p() -> heals_echo_of_light -> num_ticks;
        p() -> heals_echo_of_light -> dot() -> refresh_duration();
        p() -> echo_of_light_merged = true;
      }
    }
    else
    {
      p() -> heals_echo_of_light -> base_td = a -> direct_dmg * p() -> composite_mastery() *
                                              p() -> mastery_spells.echo_of_light -> effect_base_value( 2 ) / 10000.0 /
                                              p() -> heals_echo_of_light -> num_ticks;
      p() -> heals_echo_of_light -> execute();
      p() -> echo_of_light_merged = false;
    }
  }

  virtual void init()
  {
    heal_t::init();

    if ( can_trigger_DA && p() -> spec.divine_aegis -> ok() )
    {
      da = new divine_aegis_t( ( name_str + "_divine_aegis" ).c_str(), p() );
      add_child( da );
      da -> target = target;
    }
  }

  priest_heal_t( const std::string& n, priest_t* player, const char* sname, int t = TREE_NONE ) :
    heal_t( n.c_str(), player, sname, t ), can_trigger_DA( true ), da()
  {
    min_interval = player -> get_cooldown( "min_interval_" + name_str );
  }

  priest_heal_t( const std::string& n, priest_t* player, const uint32_t id, int t = TREE_NONE ) :
    heal_t( n.c_str(), player, id, t ), can_trigger_DA( true ), da()
  {
    min_interval = player -> get_cooldown( "min_interval_" + name_str );
  }

  priest_t* p() const
  { return static_cast<priest_t*>( player ); }

  virtual void player_buff();

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    heal_t::target_debuff( t, dmg_type );

    // Grace
    if ( p() -> spec.grace -> ok() )
      target_multiplier *= 1.0 + t -> buffs.grace -> check() * t -> buffs.grace -> value();
  }

  virtual double cost() const
  {
    double c = heal_t::cost();

    if ( ( base_execute_time <= timespan_t::zero ) && ! channeled )
    {
      c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> effect1().percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    heal_t::consume_resource();

    if ( base_execute_time <= timespan_t::zero )
      p() -> buffs.inner_will -> up();
  }

  void trigger_divine_aegis( player_t* t, double amount )
  {
    if ( da )
    {
      da -> base_dd_min = da -> base_dd_max = amount * da -> shield_multiple;
      da -> target = t;
      da -> execute();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    heal_t::impact( t, impact_result, travel_dmg );
    priest_targetdata_t* td = targetdata() -> cast_priest();

    if ( travel_dmg > 0 )
    {
      // Divine Aegis
      if ( impact_result == RESULT_CRIT )
      {
        trigger_divine_aegis( t, travel_dmg );
      }

      trigger_echo_of_light( this, t );

      if ( p() -> buffs.chakra_serenity -> up() && td -> dots_renew -> ticking )
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

  void trigger_grace( player_t* t )
  {
    if ( p() -> spec.grace -> ok() )
      t -> buffs.grace -> trigger( 1, p() -> dbc.effect( p() -> dbc.spell( p() -> spec.grace -> effect_trigger_spell( 1 ) ) -> effect1().id() ) -> base_value() / 100.0 );
  }

  void trigger_strength_of_soul( player_t* t )
  {
    if ( p() -> glyphs.strength_of_soul -> ok() && t -> buffs.weakened_soul -> up() )
      t -> buffs.weakened_soul -> extend_duration( p(), timespan_t::from_seconds( -1 * p() -> glyphs.strength_of_soul -> effect1().base_value() ) );
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
    atonement_dmg *= p() -> glyphs.atonement -> effect1().percent();
    double cap = p() -> resources.max[ RESOURCE_HEALTH ] * 0.3;

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

  virtual void execute()
  {
    target = find_lowest_player();

    priest_heal_t::execute();
  }

  virtual void tick( dot_t* d )
  {
    target = find_lowest_player();

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

    may_crit          = true;
    tick_may_crit     = true;

    dot_behavior      = DOT_REFRESH;
    weapon_multiplier = 0.0;

    crit_bonus_multiplier *= 1.0 + p() -> constants.shadow_power_crit_value;

    atonement         = 0;
    can_trigger_atonement = false;
  }

public:
  priest_spell_t( const std::string& n, priest_t* player, const school_type s, int t ) :
    spell_t( n.c_str(), player, RESOURCE_MANA, s, t ), atonement( 0 ), can_trigger_atonement( 0 )
  {
    _init_priest_spell_t();
  }

  priest_spell_t( const spell_id_t& s ) :
    spell_t( s ), atonement( 0 ), can_trigger_atonement( 0 )
  {
    _init_priest_spell_t();
  }

  priest_spell_t( const std::string& n, priest_t* player, const char* sname, int t = TREE_NONE ) :
    spell_t( n.c_str(), sname, player, t ), atonement( 0 ), can_trigger_atonement( 0 )
  {
    _init_priest_spell_t();
  }

  priest_spell_t( const std::string& n, priest_t* player, const uint32_t id, int t = TREE_NONE ) :
    spell_t( n.c_str(), id, player, t ), atonement( 0 ), can_trigger_atonement( 0 )
  {
    _init_priest_spell_t();
  }

  priest_t* p() const
  { return static_cast<priest_t*>( player ); }

  virtual void init()
  {
    spell_t::init();

    if ( can_trigger_atonement && p() -> glyphs.atonement -> ok() )
    {
      std::string n = "atonement_" + name_str;
      atonement = new atonement_heal_t( n.c_str(), p() );
    }
  }

  virtual double cost() const
  {
    double c = spell_t::cost();

    if ( ( base_execute_time <= timespan_t::zero ) && ! channeled )
    {
      c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> effect1().percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    spell_t::consume_resource();

    if ( base_execute_time <= timespan_t::zero )
      p() -> buffs.inner_will -> up();
  }

  virtual void assess_damage( player_t* t,
                              double amount,
                              int    dmg_type,
                              int    impact_result )
  {
    spell_t::assess_damage( t, amount, dmg_type, impact_result );

    if ( p() -> buffs.vampiric_embrace -> up() && result_is_hit( impact_result ) )
    {
      double a = amount;
      p() -> resource_gain( RESOURCE_HEALTH, a * 0.06, player -> gains.vampiric_embrace );

      pet_t* r = p() -> pet_list;

      while ( r )
      {
        r -> resource_gain( RESOURCE_HEALTH, a * 0.03, r -> gains.vampiric_embrace );
        r = r -> next_pet;
      }

      int num_players = ( int ) p() -> party_list.size();

      for ( int i=0; i < num_players; i++ )
      {
        player_t* q = p() -> party_list[ i ];

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

  static void trigger_shadowy_apparition( priest_t* player );
  static void add_more_shadowy_apparitions( priest_t* player );
};

// ==========================================================================
// Pet Shadow Fiend
// ==========================================================================

struct shadow_fiend_pet_t : public pet_t
{
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

      p -> buffs.shadowcrawl -> start( 1, p -> shadowcrawl -> effect_base_value( 2 ) / 100.0 );
    }
  };

  struct melee_t : public attack_t
  {
    melee_t( shadow_fiend_pet_t* player ) :
      attack_t( "melee", player, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      weapon_multiplier = 0;
      direct_power_mod = 0.0064 * player -> o() -> level;
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
    }

    void assess_damage( player_t* t, double amount, int dmg_type, int impact_result )
    {
      attack_t::assess_damage( t, amount, dmg_type, impact_result );

      shadow_fiend_pet_t* p = static_cast<shadow_fiend_pet_t*>( player -> cast_pet() );

      if ( result_is_hit( impact_result ) )
      {
        if ( p -> o() -> set_bonus.tier13_4pc_caster() )
          p -> o() -> buffs.shadow_orb -> trigger( 3, 1, p -> o() -> constants.shadow_orb_proc_value );

        p -> o() -> resource_gain( RESOURCE_MANA, p -> o() -> resources.max[ RESOURCE_MANA ] *
                                   p -> mana_leech -> effectN( 1 ).percent(),
                                   p -> o() -> gains.shadow_fiend );
      }
    }

    void player_buff()
    {
      attack_t::player_buff();

      shadow_fiend_pet_t* p = static_cast<shadow_fiend_pet_t*>( player -> cast_pet() );

      if ( p -> bad_swing )
        p -> bad_swing = false;

      player_multiplier *= 1.0 + p -> buffs.shadowcrawl -> value();
    }

    virtual void impact( player_t* t, int result, double dmg )
    {
      shadow_fiend_pet_t* p = static_cast<shadow_fiend_pet_t*>( player -> cast_pet() );

      attack_t::impact( t, result, dmg );

      // Needs testing
      if ( p -> o() -> set_bonus.tier13_4pc_caster() )
      {
        p -> o() -> buffs.shadow_orb -> trigger( 3, 1, 1.0 );
      }
    }
  };

  double bad_spell_power;
  struct buffs_t
  {
    buff_t* shadowcrawl;
  } buffs;

  spell_id_t* shadowcrawl;
  spell_id_t* mana_leech;
  bool bad_swing;
  bool extra_tick;

  shadow_fiend_pet_t( sim_t* sim, priest_t* owner ) :
    pet_t( sim, owner, "shadow_fiend" ),
    bad_spell_power( util_t::ability_rank( owner -> level,  370.0,85,  358.0,82,  352.0,80,  0.0,0 ) ),
    shadowcrawl( 0 ), mana_leech( 0 ),
    bad_swing( false ), extra_tick( false )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
    main_hand_weapon.school     = SCHOOL_SHADOW;

    stamina_per_owner           = 0.30;
    intellect_per_owner         = 0.50;

    action_list_str             = "/snapshot_stats/shadowcrawl/wait_for_shadowcrawl";
  }

  priest_t* o() const
  { return static_cast<priest_t*>( owner ); }

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

    shadowcrawl = new spell_id_t( this, "shadowcrawl", "Shadowcrawl" );
    mana_leech  = new spell_id_t( this, "mana_leech", 34650 );
  }

  virtual void init_base()
  {
    pet_t::init_base();

    attribute_base[ ATTR_STRENGTH  ]  = 0; // Unknown
    attribute_base[ ATTR_AGILITY   ]  = 0; // Unknown
    attribute_base[ ATTR_STAMINA   ]  = 0; // Unknown
    attribute_base[ ATTR_INTELLECT ]  = 0; // Unknown
    resources.base[ RESOURCE_HEALTH ]  = util_t::ability_rank( owner -> level,  18480.0,85,  7475.0,82,  6747.0,80,  100.0,0 );
    resources.base[ RESOURCE_MANA   ]  = util_t::ability_rank( owner -> level,  16828.0,85,  9824.0,82,  7679.0,80,  100.0,0 );
    base_attack_power                 = 0;  // Unknown
    base_attack_crit                  = 0.07; // Needs more testing
    initial_attack_power_per_strength = 0; // Unknown

    main_hand_attack = new melee_t( this );
  }

  virtual void init_buffs()
  {
    pet_t::init_buffs();

    buffs.shadowcrawl = new buff_t( this, "shadowcrawl", 1, shadowcrawl -> duration() );
  }

  virtual double composite_spell_power( const school_type school ) const
  {
    double sp;

    if ( bad_swing )
      sp = bad_spell_power;
    else
      sp = o() -> composite_spell_power( school ) * o() -> composite_spell_power_multiplier();

    return sp;
  }

  virtual double composite_attack_hit() const
  {
    return owner -> composite_spell_hit();
  }

  virtual double composite_attack_expertise( weapon_t* /* w */ ) const
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }

  virtual double composite_attack_crit( weapon_t* /* w */ ) const
  {
    double c = pet_t::composite_attack_crit();

    c += owner -> composite_spell_crit(); // Needs confirming that it benefits from ALL owner crit.

    return c;
  }

  virtual void summon( timespan_t duration )
  {
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

    o() -> buffs.shadowfiend -> start();
  }

  virtual void dismiss()
  {
    pet_t::dismiss();

    o() -> buffs.shadowfiend -> expire();
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
      may_crit = false;
      tick_may_crit = true;

      tick_power_mod = 0.308;
    }

    lightwell_pet_t* p()
    {
      return static_cast<lightwell_pet_t*>( player );
    }

    virtual void execute()
    {
      p() -> charges--;

      target = find_lowest_player();

      heal_t::execute();
    }

    virtual void last_tick( dot_t* d )
    {
      heal_t::last_tick( d );

      if ( p() -> charges <= 0 )
        p() -> dismiss();
    }

    virtual bool ready()
    {
      if ( p() -> charges <= 0 )
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
    priest_t* o = static_cast<priest_t*>( owner );

    spell_haste = o -> spell_haste;
    spell_power[ SCHOOL_HOLY ] = o -> composite_spell_power( SCHOOL_HOLY ) * o -> composite_spell_power_multiplier();

    charges = 10 + o -> glyphs.lightwell -> effect1().base_value();

    pet_t::summon( duration );
  }
};

// ==========================================================================
// Priest Spell Increments
// ==========================================================================

// Shadowy Apparition Spell =================================================

struct shadowy_apparition_spell_t : public priest_spell_t
{
  shadowy_apparition_spell_t( priest_t* player ) :
    priest_spell_t( "shadowy_apparition_spell", player, 87532 )
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
    priest_spell_t::impact( t, result, dmg );


    // Needs testing
    if ( p() -> set_bonus.tier13_4pc_caster() )
    {
      p() -> buffs.shadow_orb -> trigger( 3, 1, 1.0 );
    }

    // Cleanup. Re-add to free list.
    p() -> shadowy_apparition_active_list.remove( this );
    p() -> shadowy_apparition_free_list.push( this );
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();

    if ( player -> bugs )
    {
      player_multiplier /= 1.0 + p() -> constants.twisted_faith_static_value;
    }
  }
};

void priest_spell_t::trigger_shadowy_apparition( priest_t* p )
{
  if ( !p -> shadowy_apparition_free_list.empty() )
  {
    for ( ; p->buffs.shadow_orb -> check(); p->buffs.shadow_orb -> decrement() )
    {
      spell_t* s = p -> shadowy_apparition_free_list.front();

      p -> shadowy_apparition_free_list.pop();

      p -> shadowy_apparition_active_list.push_back( s );

      p -> procs.shadowy_apparation -> occur();

      s -> execute();
    }
  }
}

void priest_spell_t::add_more_shadowy_apparitions( priest_t* p )
{
  spell_t* s = NULL;

  if ( ! p -> shadowy_apparition_free_list.size() )
  {
    for ( uint32_t i = 0; i < p -> constants.max_shadowy_apparitions; i++ )
    {
      s = new shadowy_apparition_spell_t( p );
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

// Chakra_Pre Spell =========================================================

struct chakra_t : public priest_spell_t
{
  chakra_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "chakra", p, "Chakra" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    // FIXME: Doesn't the cooldown work like Inner Focus?
    cooldown -> duration = timespan_t::from_seconds( sim -> wheel_seconds );

    priest_spell_t::execute();

    p() -> buffs.chakra_pre -> start();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.chakra_pre -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Dispersion Spell =========================================================

struct dispersion_t : public priest_spell_t
{
  dispersion_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "dispersion", player, "Dispersion" )
  {
    parse_options( NULL, options_str );

    base_tick_time    = timespan_t::from_seconds( 1.0 );
    num_ticks         = 6;

    channeled         = true;
    harmful           = false;
    tick_may_crit     = false;

    cooldown -> duration += p() -> glyphs.dispersion -> effect1().time_value();
  }

  virtual void tick( dot_t* d )
  {
    double regen_amount = p() -> dbc.spell( 49766 ) -> effect1().percent() * p() -> resources.max[ RESOURCE_MANA ];

    p() -> resource_gain( RESOURCE_MANA, regen_amount, p() -> gains.dispersion );

    priest_spell_t::tick( d );
  }

};

// Fortitude Spell ==========================================================

struct fortitude_t : public priest_spell_t
{
  double bonus;

  fortitude_t( priest_t* player, const std::string& options_str ) :
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
        double before = p -> resources.current[ RESOURCE_HEALTH ];
        p -> buffs.fortitude -> trigger( 1, bonus );
        double  after = p -> resources.current[ RESOURCE_HEALTH ];
        p -> stat_gain( STAT_HEALTH, after - before );
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
  hymn_of_hope_tick_t( priest_t* player ) :
    priest_spell_t( "hymn_of_hope_tick", player, 64904 )
  {
    dual        = true;
    background  = true;
    may_crit    = true;
    direct_tick = true;

    harmful     = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> resource_gain( RESOURCE_MANA, effect1().percent() * p() -> resources.max[ RESOURCE_MANA ], p() -> gains.hymn_of_hope );

    // Hymn of Hope only adds +x% of the current_max mana, it doesn't change if afterwards max_mana changes.
    player -> buffs.hymn_of_hope -> trigger();
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

  virtual void init()
  {
    priest_spell_t::init();

    hymn_of_hope_tick -> stats = stats;
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
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    cooldown -> duration = timespan_t::from_seconds( sim -> wheel_seconds );

    priest_spell_t::execute();

    p() -> buffs.inner_focus -> trigger();
  }
};

// Inner Fire Spell =========================================================

struct inner_fire_t : public priest_spell_t
{
  inner_fire_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "inner_fire", player, "Inner Fire" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.inner_will -> expire ();
    p() -> buffs.inner_fire -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.inner_fire -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Inner Will Spell =========================================================

struct inner_will_t : public priest_spell_t
{
  inner_will_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "inner_will", player, "Inner Will" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.inner_fire -> expire();

    p() -> buffs.inner_will -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.inner_will -> check() )
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

    target -> buffs.power_infusion -> trigger();
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

struct shadowform_t : public priest_spell_t
{
  shadowform_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "shadowform", p, "Shadowform" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.shadowform -> trigger();

    sim -> auras.mind_quickening -> trigger();
  }

  virtual bool ready()
  {
    if (  p() -> buffs.shadowform -> check() )
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

    cooldown = p -> cooldowns.shadow_fiend;
    cooldown -> duration = spell_id_t::cooldown();

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> pets.shadow_fiend -> summon( duration() );
  }

  virtual bool ready()
  {
    if ( p() -> buffs.shadowfiend -> check() )
      return false;

    return priest_spell_t::ready();
  }
};


// ==========================================================================
// Priest Damage Spells
// ==========================================================================

// Mind Blast Spell =========================================================

struct mind_blast_t : public priest_spell_t
{
  stats_t* orb_stats[ 4 ];

  mind_blast_t( priest_t* player, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "mind_blast", player, "Mind Blast" )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new mind_blast_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void init()
  {
    for ( int i=0; i < 4; i++ )
    {
      std::string str = name_str + "_";
      orb_stats[ i ] = p() -> get_stats( str + ( char ) ( i + ( int ) '0' ) + ( is_dtr_action ? "_DTR" : "" ), this );
    }
    priest_spell_t::init();
  }

  virtual void execute()
  {
    timespan_t saved_cooldown = cooldown -> duration;

    priest_targetdata_t* td = targetdata() -> cast_priest();

    stats = orb_stats[ p() -> buffs.shadow_orb -> stack() ];

    priest_spell_t::execute();

    cooldown -> duration = saved_cooldown;

    for ( int i=0; i < 4; i++ )
    {
      p() -> benefits.mind_spike[ i ] -> update( i == p() -> buffs.mind_spike -> stack() );
    }
    for ( int i=0; i < 4; i++ )
    {
      p() -> benefits.shadow_orb[ i ] -> update( i == p() -> buffs.shadow_orb -> stack() );
    }

    p() -> buffs.mind_spike -> expire();
    p() -> buffs.mind_spike -> expire();

    if ( p() -> buffs.shadow_orb -> check() )
    {
      p() -> buffs.shadow_orb -> expire();

      p() -> buffs.empowered_shadow -> trigger( 1, p() -> empowered_shadows_amount() );
    }

    if ( result_is_hit() )
    {
      if ( td -> dots_vampiric_touch -> ticking )
        p() -> trigger_replenishment();
    }
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();

    player_multiplier *= 1.0 + ( p() -> buffs.shadow_orb -> stack() * p() -> shadow_orb_amount() );
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    priest_spell_t::target_debuff( t, dmg_type );

    target_crit       += p() -> buffs.mind_spike -> value() * p() -> buffs.mind_spike -> check();
  }

  virtual timespan_t execute_time() const
  {
    timespan_t a = priest_spell_t::execute_time();

    a *= 1 + ( p() -> buffs.mind_spike -> check() * p() -> glyphs.mind_spike -> effect2().percent() );

    return a;
  }
};

// Mind Flay Spell ==========================================================

struct mind_flay_t : public priest_spell_t
{
  timespan_t mb_wait;
  int    cut_for_mb;
  int    no_dmg;

  mind_flay_t( priest_t* p, const std::string& options_str,
               const char* name = "mind_flay" ) :
    priest_spell_t( name, p, "Mind Flay" ), mb_wait( timespan_t::zero ), cut_for_mb( 0 ), no_dmg( 0 )
  {
    option_t options[] =
    {
      { "cut_for_mb",  OPT_BOOL, &cut_for_mb  },
      { "mb_wait",     OPT_TIMESPAN,  &mb_wait     },
      { "no_dmg",      OPT_BOOL, &no_dmg      },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit     = false;
    channeled    = true;
    hasted_ticks = false;
  }

  virtual void execute()
  {
    if ( cut_for_mb )
      if ( p() -> cooldowns.mind_blast -> remains() <= ( 2 * base_tick_time * haste() ) )
        num_ticks = 2;

    priest_spell_t::execute();

    if ( result_is_hit() )
    {
      // Evangelism procs. off both the initial cast and each tick.
      p() -> buffs.dark_evangelism -> trigger();
    }
  }

  virtual double calculate_tick_damage()
  {
    if ( no_dmg )
      return 0.0;

    return priest_spell_t::calculate_tick_damage();
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    if ( result_is_hit() )
    {
      p() -> buffs.dark_evangelism -> trigger();
      p() -> buffs.shadow_orb  -> trigger( 1, 1, p() -> constants.shadow_orb_proc_value );
    }
  }

  virtual bool ready()
  {
    // If this option is set (with a value in seconds), don't cast Mind Flay if Mind Blast
    // is about to come off it's cooldown.
    if ( mb_wait != timespan_t::zero )
    {
      if ( p() -> cooldowns.mind_blast -> remains() < mb_wait )
        return false;
    }

    return priest_spell_t::ready();
  }
};

// Mind Spike Spell =========================================================

struct mind_spike_t : public priest_spell_t
{
  mind_spike_t( priest_t* player, const std::string& options_str, bool dtr=false ) :
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

    if ( p() -> buffs.chakra_pre -> up() )
    {
      p() -> buffs.chakra_chastise -> trigger();

      p() -> buffs.chakra_pre -> expire();

      p() -> cooldowns.chakra -> reset();
      p() -> cooldowns.chakra -> duration  = p() -> buffs.chakra_pre -> spell_id_t::cooldown();
      p() -> cooldowns.chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double dmg )
  {
    priest_spell_t::impact( t, impact_result, dmg );

    if ( result_is_hit( impact_result ) )
    {
      priest_targetdata_t* td = targetdata() -> cast_priest();
      for ( int i=0; i < 4; i++ )
      {
        p() -> benefits.shadow_orb[ i ] -> update( i == p() -> buffs.shadow_orb -> stack() );
      }

      p() -> buffs.mind_spike -> trigger( 1, 1.0 );

      if ( p() -> buffs.shadow_orb -> check() )
      {
        p() -> buffs.shadow_orb -> expire();
        p() -> buffs.empowered_shadow -> trigger( 1, p() -> empowered_shadows_amount() );
      }
      p() -> buffs.mind_spike -> trigger( 1, effect2().percent() );

      if ( ! td -> remove_dots_event )
      {
        td -> remove_dots_event = new ( sim ) remove_dots_event_t( sim, p(), td );
      }
    }
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();

    player_multiplier *= 1.0 + ( p() -> buffs.shadow_orb -> stack() * p() -> shadow_orb_amount() );
  }
};

// Mind Sear Spell ==========================================================

struct mind_sear_tick_t : public priest_spell_t
{
  mind_sear_tick_t( priest_t* player ) :
    priest_spell_t( "mind_sear_tick", player, 49821 )
  {
    background  = true;
    dual        = true;
    direct_tick = true;
  }
};

struct mind_sear_t : public priest_spell_t

{
  mind_sear_tick_t* mind_sear_tick;

  mind_sear_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "mind_sear", player, "Mind Sear" ),
    mind_sear_tick( 0 )
  {
    parse_options( NULL, options_str );

    channeled = true;
    may_crit  = false;

    mind_sear_tick = new mind_sear_tick_t( player );
  }

  virtual void init()
  {
    priest_spell_t::init();

    mind_sear_tick -> stats = stats;
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

    base_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * p -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 1 ) / 100.0;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new shadow_word_death_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    p() -> was_sub_25 = ! is_dtr_action && ( target -> health_percentage() <= 25 );

    priest_spell_t::execute();

    if ( result_is_hit() && p() -> was_sub_25 && ( target -> health_percentage() > 0 ) )
    {
      cooldown -> reset();
      p() -> buffs.glyph_of_shadow_word_death -> trigger();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_spell_t::impact( t, impact_result, travel_dmg );

    double health_loss = travel_dmg;

    // Needs testing
    if ( p() -> set_bonus.tier13_2pc_caster() )
    {
      health_loss *= 1.0 - p() -> sets -> set( SET_T13_2PC_CASTER ) -> effect_base_value( 2 ) / 100.0;
    }

    p() -> assess_damage( health_loss, school, DMG_DIRECT, RESULT_HIT, this );
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();

    if ( p() -> glyphs.mind_spike -> ok() && ( target -> health_percentage() <= p() -> glyphs.mind_spike-> effect3().base_value() ) )
      player_multiplier *= 1.0 + p() -> glyphs.mind_spike -> effect1().percent();

    // Hardcoded into the tooltip
    if ( target -> health_percentage() <= 25 )
      player_multiplier *= 3.0;
  }

  virtual bool ready()
  {
    if ( mb_min_wait != timespan_t::zero )
      if ( p() -> cooldowns.mind_blast -> remains() < mb_min_wait )
        return false;

    if ( mb_max_wait != timespan_t::zero )
      if ( p() -> cooldowns.mind_blast -> remains() > mb_max_wait )
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

    stats -> children.push_back( player -> get_stats( "shadowy_apparition", this ) );
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    if ( result_is_hit() )
    {
      p() -> buffs.shadow_orb  -> trigger( 1, 1, p() -> constants.shadow_orb_proc_value );
    }
  }
};

// Vampiric Embrace Spell ===================================================

struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "vampiric_embrace", p, "Vampiric Embrace" )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.vampiric_embrace -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buffs.vampiric_embrace -> check() )
      return false;

    return priest_spell_t::ready();
  }
};

// Vampiric Touch Spell =====================================================

struct vampiric_touch_t : public priest_spell_t
{
  vampiric_touch_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "vampiric_touch", player, "Vampiric Touch" )
  {
    parse_options( NULL, options_str );
    may_crit   = false;
  }
};

// Holy Fire Spell ==========================================================

struct holy_fire_t : public priest_spell_t
{
  holy_fire_t( priest_t* player, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "holy_fire", player, "Holy Fire" )
  {
    parse_options( NULL, options_str );

    base_hit += player -> glyphs.atonement -> effect1().percent();

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

    p() -> buffs.holy_evangelism  -> trigger();
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();

    player_multiplier *= 1.0 + ( p() -> buffs.holy_evangelism -> stack() * p() -> buffs.holy_evangelism -> effect1().percent() );
  }

  virtual double cost() const
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> effect2().percent() );

    return c;
  }
};

// Penance Spell ============================================================

struct penance_t : public priest_spell_t
{
  struct penance_tick_t : public priest_spell_t
  {
    penance_tick_t( priest_t* player ) :
      priest_spell_t( "penance_tick", player, 47666 )
    {
      background  = true;
      dual        = true;
      direct_tick = true;
    }

    virtual void player_buff()
    {
      priest_spell_t::player_buff();

      player_multiplier *= 1.0 + ( p() -> buffs.holy_evangelism -> stack() * p() -> buffs.holy_evangelism -> effect1().percent() );
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

  virtual void init()
  {
    priest_spell_t::init();

    tick_spell -> stats = stats;
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

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> effect2().percent() );

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

    base_hit += p -> glyphs.atonement -> effect1().percent();

    can_trigger_atonement = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new smite_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.holy_evangelism -> trigger();

    // Chakra
    if ( p() -> buffs.chakra_pre -> up() )
    {
      p() -> buffs.chakra_chastise -> trigger();

      p() -> buffs.chakra_pre -> expire();

      p() -> cooldowns.chakra -> reset();
      p() -> cooldowns.chakra -> duration = p() -> buffs.chakra_pre -> spell_id_t::cooldown();
      p() -> cooldowns.chakra -> start();
    }

    // Train of Thought
    if ( p() -> spec.train_of_thought -> ok() )
    {
      if ( p() -> cooldowns.penance -> remains() > p() -> spec.train_of_thought -> effect2().time_value() )
        p() -> cooldowns.penance -> ready -= p() -> spec.train_of_thought -> effect2().time_value();
      else
        p() -> cooldowns.penance -> reset();
    }
  }

  virtual void player_buff()
  {
    priest_spell_t::player_buff();

    priest_targetdata_t* td = targetdata() -> cast_priest();

    player_multiplier *= 1.0 + ( p() -> buffs.holy_evangelism -> stack() * p() -> buffs.holy_evangelism -> effect1().percent() );

    if ( td -> dots_holy_fire -> ticking && p() -> glyphs.smite -> ok() )
      player_multiplier *= 1.0 + p() -> glyphs.smite -> effect1().percent();
  }

  virtual double cost() const
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> effect2().percent() );

    return c;
  }
};

struct shadowy_apparition_t : priest_spell_t
{
  shadowy_apparition_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "shadowy_apparition", player, "Shadowy Apparition" )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    trigger_shadowy_apparition( p() );
  }
};

// ==========================================================================
// Priest Heal & Absorb Spells
// ==========================================================================

// priest_heal_t::player_buff - artifical separator =========================

void priest_heal_t::player_buff()
{
  heal_t::player_buff();

  player_multiplier *= 1.0 + p() -> spec.spiritual_healing -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) ;

  if ( p() -> buffs.chakra_serenity -> up() )
    player_crit += p() -> buffs.chakra_serenity -> effect1().percent();
  if ( p() -> buffs.serenity -> up() )
    player_crit += p() -> buffs.serenity -> effect2().percent();

  player_multiplier *= 1.0 + p() -> buffs.holy_archangel -> value();
}


// Binding Heal Spell =======================================================

struct binding_heal_t : public priest_heal_t
{
  binding_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "binding_heal", p, "Binding Heal" )
  {
    parse_options( NULL, options_str );

    aoe = 1;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    // Inner Focus
    if ( p() -> buffs.inner_focus -> up() )
    {
      // Inner Focus cooldown starts when consumed.
      p() -> cooldowns.inner_focus -> reset();
      p() -> cooldowns.inner_focus -> duration = p() -> buffs.inner_focus -> spell_id_t::cooldown();
      p() -> cooldowns.inner_focus -> start();
      p() -> buffs.inner_focus -> expire();
    }

    // Chakra
    if ( p() -> buffs.chakra_pre -> up() )
    {
      p() -> buffs.chakra_serenity -> trigger();

      p() -> buffs.chakra_pre -> expire();

      p() -> cooldowns.chakra -> reset();
      p() -> cooldowns.chakra -> duration = p() -> buffs.chakra_pre -> spell_id_t::cooldown();
      p() -> cooldowns.chakra -> start();
    }
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    if ( p() -> buffs.inner_focus -> check() )
      player_crit += p() -> buffs.inner_focus -> effect2().percent();
  }

  virtual double cost() const
  {
    double c = priest_heal_t::cost();

    if ( p() -> buffs.inner_focus -> check() )
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
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.circle_of_healing -> effect2().percent();
    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );
    aoe = p -> glyphs.circle_of_healing -> ok() ? 5 : 4;
  }

  virtual void execute()
  {
    cooldown -> duration = spell_id_t::cooldown();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      cooldown -> duration +=  p() -> buffs.chakra_sanctuary -> effect2().time_value();

    // Choose Heal Target
    target = find_lowest_player();

    priest_heal_t::execute();
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p() -> buffs.chakra_sanctuary -> effect1().percent();
  }
};

// Divine Hymn Spell ========================================================

struct divine_hymn_tick_t : public priest_heal_t
{
  divine_hymn_tick_t( priest_t* player, int nr_targets ) :
    priest_heal_t( "divine_hymn_tick", player, 64844 )
  {
    background  = true;

    aoe = nr_targets - 1;
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p() -> buffs.chakra_sanctuary -> effect1().percent();
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

    divine_hymn_tick = new divine_hymn_tick_t( p, effect2().base_value() );
    add_child( divine_hymn_tick );
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
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    // Inner Focus
    if ( p() -> buffs.inner_focus -> up() )
    {
      // Inner Focus cooldown starts when consumed.
      p() -> cooldowns.inner_focus -> reset();
      p() -> cooldowns.inner_focus -> duration = p() -> buffs.inner_focus -> spell_id_t::cooldown();
      p() -> cooldowns.inner_focus -> start();
      p() -> buffs.inner_focus -> expire();
    }

    // Chakra
    if ( p() -> buffs.chakra_pre -> up() )
    {
      p() -> buffs.chakra_serenity -> trigger();

      p() -> buffs.chakra_pre -> expire();

      p() -> cooldowns.chakra -> reset();
      p() -> cooldowns.chakra -> duration = p() -> buffs.chakra_pre -> spell_id_t::cooldown();
      p() -> cooldowns.chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_grace( t );
    trigger_strength_of_soul( t );
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    if ( p() -> buffs.inner_focus -> up() )
      player_crit += p() -> buffs.inner_focus -> effect2().percent();
  }

  virtual double cost() const
  {
    double c = priest_heal_t::cost();

    if ( p() -> buffs.inner_focus -> check() )
      c = 0;

    return c;
  }
};

// Guardian Spirit ==========================================================

struct guardian_spirit_t : public priest_heal_t
{
  guardian_spirit_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "guardian_spirit", p, "Guardian Spirit" )
  {
    parse_options( NULL, options_str );

    // The absorb listed isn't a real absorb
    base_dd_min = base_dd_max = 0;

    harmful = false;
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
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    // Train of Thought
    // NOTE: Process Train of Thought _before_ Inner Focus: the GH that consumes Inner Focus does not
    //       reduce the cooldown, since Inner Focus doesn't go on cooldown until after it is consumed.
    if ( p() -> spec.train_of_thought -> ok() )
    {
      if ( p() -> cooldowns.inner_focus -> remains() > timespan_t::from_seconds( p() -> spec.train_of_thought -> effect1().base_value() ) )
        p() -> cooldowns.inner_focus -> ready -= timespan_t::from_seconds( p() -> spec.train_of_thought -> effect1().base_value() );
      else
        p() -> cooldowns.inner_focus -> reset();
    }

    // Inner Focus
    if ( p() -> buffs.inner_focus -> up() )
    {
      // Inner Focus cooldown starts when consumed.
      p() -> cooldowns.inner_focus -> reset();
      p() -> cooldowns.inner_focus -> duration = p() -> buffs.inner_focus -> spell_id_t::cooldown();
      p() -> cooldowns.inner_focus -> start();
      p() -> buffs.inner_focus -> expire();
    }

    // Chakra
    if ( p() -> buffs.chakra_pre -> up() )
    {
      p() -> buffs.chakra_serenity -> trigger();

      p() -> buffs.chakra_pre -> expire();

      p() -> cooldowns.chakra -> reset();
      p() -> cooldowns.chakra -> duration = p() -> buffs.chakra_pre -> spell_id_t::cooldown();
      p() -> cooldowns.chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_grace( t );
    trigger_strength_of_soul( t );
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    if ( p() -> buffs.inner_focus -> up() )
      player_crit += p() -> buffs.inner_focus -> effect2().percent();
  }

  virtual double cost() const
  {
    double c = priest_heal_t::cost();

    if ( p() -> buffs.inner_focus -> check() )
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
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    // Trigger Chakra
    if ( p() -> buffs.chakra_pre -> up() )
    {
      p() -> buffs.chakra_serenity -> trigger();

      p() -> buffs.chakra_pre -> expire();

      p() -> cooldowns.chakra -> reset();
      p() -> cooldowns.chakra -> duration = p() -> buffs.chakra_pre -> spell_id_t::cooldown();
      p() -> cooldowns.chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_grace( t );
    trigger_strength_of_soul( t );
  }
};

// Holy Word Sanctuary ======================================================

struct holy_word_sanctuary_t : public priest_heal_t
{
  struct holy_word_sanctuary_tick_t : public priest_heal_t
  {
    holy_word_sanctuary_tick_t( priest_t* player ) :
      priest_heal_t( "holy_word_sanctuary_tick", player, 88686 )
    {
      dual        = true;
      background  = true;
      direct_tick = true;
    }

    virtual void player_buff()
    {
      priest_heal_t::player_buff();

      if ( p() -> buffs.chakra_sanctuary -> up() )
        player_multiplier *= 1.0 + p() -> buffs.chakra_sanctuary -> effect1().percent();
    }
  };

  holy_word_sanctuary_tick_t* tick_spell;

  holy_word_sanctuary_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "holy_word_sanctuary", p, 88685 ),
    tick_spell( 0 )
  {
    check_talent( p -> spec.revelations -> ok() );

    parse_options( NULL, options_str );

    hasted_ticks = false;
    may_crit     = false;

    base_tick_time = timespan_t::from_seconds( 2.0 );
    num_ticks = 9;

    tick_spell = new holy_word_sanctuary_tick_t( p );

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;
  }

  virtual void init()
  {
    priest_heal_t::init();

    tick_spell -> stats = stats;
  }

  virtual void tick( dot_t* d )
  {
    tick_spell -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual bool ready()
  {
    if ( ! p() -> buffs.chakra_sanctuary -> check() )
      return false;

    return priest_heal_t::ready();
  }

  // HW: Sanctuary is treated as a instant cast spell, both affected by Inner Will and Mental Agility

  virtual double cost() const
  {
    double c = priest_heal_t::cost();

    // HW: Sanctuary is treated as a instant cast spell, both affected by Inner Will and Mental Agility
    // Implemented 06/12/2011 ( Patch 4.3 ),
    // see Issue1023 and http://elitistjerks.com/f77/t110245-cataclysm_holy_priest_compendium/p25/#post2054467

    c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> effect1().percent();
    c  = floor( c );

    return c;
  }

  virtual void consume_resource()
  {
    priest_heal_t::consume_resource();

    p() -> buffs.inner_will -> up();
  }
};

// Holy Word Chastise =======================================================

struct holy_word_chastise_t : public priest_spell_t
{
  holy_word_chastise_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "holy_word_chastise", p, "Holy Word: Chastise" )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;
  }

  virtual bool ready()
  {
    if ( p() -> spec.revelations -> ok() )
    {
      if ( p() -> buffs.chakra_sanctuary -> check() )
        return false;

      if ( p() -> buffs.chakra_serenity -> check() )
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
    check_talent( p -> spec.revelations -> ok() );

    parse_options( NULL, options_str );

    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    p() -> buffs.serenity -> trigger();
  }

  virtual bool ready()
  {
    if ( ! p() -> buffs.chakra_serenity -> check() )
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

  holy_word_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "holy_word", p, SCHOOL_HOLY, TREE_HOLY ),
    hw_sanctuary( 0 ), hw_chastise( 0 ), hw_serenity( 0 )
  {
    hw_sanctuary = new holy_word_sanctuary_t( p, options_str );
    hw_chastise  = new holy_word_chastise_t ( p, options_str );
    hw_serenity  = new holy_word_serenity_t ( p, options_str );
  }

  virtual void schedule_execute()
  {
    if ( p() -> spec.revelations -> ok() && p() -> buffs.chakra_serenity -> up() )
    {
      player -> last_foreground_action = hw_serenity;
      hw_serenity -> schedule_execute();
    }
    else if ( p() -> spec.revelations -> ok() && p() -> buffs.chakra_sanctuary -> up() )
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
    if ( p() -> spec.revelations -> ok() && p() -> buffs.chakra_serenity -> check() )
      return hw_serenity -> ready();

    else if ( p() -> spec.revelations -> ok() && p() -> buffs.chakra_sanctuary -> check() )
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
    priest_spell_t::execute();

    p() -> pets.lightwell -> get_cooldown( "lightwell_renew" ) -> duration = consume_interval;
    p() -> pets.lightwell -> summon( duration() );
  }
};

// Penance Heal Spell =======================================================

struct penance_heal_tick_t : public priest_heal_t
{
  penance_heal_tick_t( priest_t* player ) :
    priest_heal_t( "penance_heal_tick", player, 47750 )
  {
    background  = true;
    may_crit    = true;
    dual        = true;
    direct_tick = true;

    stats = player -> get_stats( "penance_heal", this );
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    trigger_grace( t );
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

    cooldown = p -> cooldowns.penance;
    cooldown -> duration = spell_id_t::cooldown() + p -> glyphs.penance -> effect1().time_value();

    penance_tick = new penance_heal_tick_t( p );
    penance_tick -> target = target;
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
    penance_tick -> target = target;
    penance_tick -> execute();
    stats -> add_tick( d -> time_to_tick );
  }

  virtual double cost() const
  {
    double c = priest_heal_t::cost();

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> effect2().percent() );

    return c;
  }
};

// Power Word: Shield Spell =================================================

struct glyph_power_word_shield_t : public priest_heal_t
{
  glyph_power_word_shield_t( priest_t* player ) :
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

    direct_power_mod = 0.87; // hardcoded into tooltip

    if ( p -> glyphs.power_word_shield -> ok() )
    {
      glyph_pws = new glyph_power_word_shield_t( p );
      glyph_pws -> target = target;
      add_child( glyph_pws );
    }
  }

  virtual double cost() const
  {
    double c = priest_absorb_t::cost();

    return c;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_targetdata_t* td = targetdata() -> cast_priest();

    t -> buffs.weakened_soul -> trigger();

    td -> buffs_power_word_shield -> trigger( 1, travel_dmg );

    stats -> add_result( travel_dmg, travel_dmg, STATS_ABSORB, impact_result );

    // Glyph
    if ( glyph_pws )
    {
      glyph_pws -> base_dd_min  = glyph_pws -> base_dd_max  = p() -> glyphs.power_word_shield -> effect1().percent() * travel_dmg;
      glyph_pws -> target = t;
      glyph_pws -> execute();
    }
  }

  virtual bool ready()
  {
    if ( ! ignore_debuff && target -> buffs.weakened_soul -> check() )
      return false;

    return priest_absorb_t::ready();
  }
};

// Prayer of Healing Spell ==================================================

struct prayer_of_healing_t : public priest_heal_t
{
  prayer_of_healing_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "prayer_of_healing", p, "Prayer of Healing" )
  {
    parse_options( NULL, options_str );

    aoe = 4;
    group_only = true;
  }

  virtual void init()
  {
    priest_heal_t::init();

    // PoH crits double the DA percentage.
    if ( da ) da -> shield_multiple *= 2;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    // Inner Focus
    if ( p() -> buffs.inner_focus -> up() )
    {
      // Inner Focus cooldown starts when consumed.
      p() -> cooldowns.inner_focus -> reset();
      p() -> cooldowns.inner_focus -> duration = p() -> buffs.inner_focus -> spell_id_t::cooldown();
      p() -> cooldowns.inner_focus -> start();
      p() -> buffs.inner_focus -> expire();
    }

    // Chakra
    if ( p() -> buffs.chakra_pre -> up() )
    {
      p() -> buffs.chakra_sanctuary -> trigger();

      p() -> buffs.chakra_pre -> expire();

      p() -> cooldowns.chakra -> reset();
      p() -> cooldowns.chakra -> duration = p() -> buffs.chakra_pre -> spell_id_t::cooldown();
      p() -> cooldowns.chakra -> start();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    priest_heal_t::impact( t, impact_result, travel_dmg );

    // Divine Aegis
    if ( impact_result != RESULT_CRIT )
    {
      trigger_divine_aegis( t, travel_dmg * 0.5 );
    }
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    if ( p() -> buffs.inner_focus -> up() )
      player_crit += p() -> buffs.inner_focus -> effect2().percent();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p() -> buffs.chakra_sanctuary -> effect1().percent();
  }

  virtual double cost() const
  {
    double c = priest_heal_t::cost();

    if ( p() -> buffs.inner_focus -> check() )
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

    can_trigger_DA = false;

    aoe = 4;
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p() -> buffs.chakra_sanctuary -> effect1().percent();
  }

  virtual void execute()
  {

    priest_heal_t::execute();

    // Chakra
    if ( p() -> buffs.chakra_pre -> up() )
    {
      p() -> buffs.chakra_sanctuary -> trigger();

      p() -> buffs.chakra_pre -> expire();

      p() -> cooldowns.chakra -> reset();
      p() -> cooldowns.chakra -> duration = p() -> buffs.chakra_pre -> spell_id_t::cooldown();
      p() -> cooldowns.chakra -> start();
    }
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    priest_heal_t::target_debuff( t, dmg_type );

    if ( p() -> glyphs.prayer_of_mending -> ok() && t == target )
      target_multiplier *= 1.0 + p() -> glyphs.prayer_of_mending -> effect1().percent();
  }
};

// Renew Spell ==============================================================

struct divine_touch_t : public priest_heal_t
{
  divine_touch_t( priest_t* player ) :
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
  renew_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "renew", p, "Renew" )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    base_multiplier *= 1.0 + p -> glyphs.renew -> effect1().percent();
  }

  virtual void player_buff()
  {
    priest_heal_t::player_buff();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      player_multiplier *= 1.0 + p() -> buffs.chakra_sanctuary -> effect1().percent();
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

// priest_t::primary_role

int priest_t::primary_role() const
{
  switch ( player_t::primary_role() )
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

  if ( buffs.inner_fire -> up() )
    a *= 1.0 + buffs.inner_fire -> base_value( E_APPLY_AURA, A_MOD_RESISTANCE_PCT ) / 100.0 * ( 1.0 + glyphs.inner_fire -> effect1().percent() );

  return floor( a );
}

// priest_t::composite_spell_power ==========================================

double priest_t::composite_spell_power( const school_type school ) const
{
  double sp = player_t::composite_spell_power( school );

  if ( buffs.inner_fire -> up() )
    sp += buffs.inner_fire -> effect_min( 2 );

  return sp;
}

// priest_t::composite_spell_hit ============================================

double priest_t::composite_spell_hit() const
{
  double hit = player_t::composite_spell_hit();

  hit += ( ( spirit() - attribute_base[ ATTR_SPIRIT ] ) * constants.twisted_faith_dynamic_value ) / rating.spell_hit;

  return hit;
}

// priest_t::composite_player_multiplier ====================================

double priest_t::composite_player_multiplier( const school_type school, action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( spell_id_t::is_school( school, SCHOOL_SHADOW ) )
  {
    m *= 1.0 + buffs.shadowform -> check() * constants.shadowform_value;
    m *= 1.0 + constants.twisted_faith_static_value;
  }
  if ( spell_id_t::is_school( school, SCHOOL_SHADOWLIGHT ) )
  {
    if ( buffs.chakra_chastise -> up() )
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
    player_multiplier += buffs.empowered_shadow -> value();
    player_multiplier += buffs.dark_evangelism -> stack () * buffs.dark_evangelism -> effect1().percent();
  }

  return player_multiplier;
}

// priest_t::composite_movement_speed =======================================

double priest_t::composite_movement_speed() const
{
  double speed = player_t::composite_movement_speed();

  if ( buffs.inner_will -> up() )
    speed *= 1.0 + buffs.inner_will -> effect2().percent() + glyphs.inner_sanctum -> effect2().percent();

  return speed;
}

// priest_t::matching_gear_multiplier =======================================

double priest_t::matching_gear_multiplier( const attribute_type attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return 0.05;

  return 0.0;
}

// priest_t::create_action ==================================================

action_t* priest_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  // Misc
  if ( name == "chakra"                 ) return new chakra_t                ( this, options_str );
  if ( name == "dispersion"             ) return new dispersion_t            ( this, options_str );
  if ( name == "fortitude"              ) return new fortitude_t             ( this, options_str );
  if ( name == "hymn_of_hope"           ) return new hymn_of_hope_t          ( this, options_str );
  if ( name == "inner_fire"             ) return new inner_fire_t            ( this, options_str );
  if ( name == "inner_focus"            ) return new inner_focus_t           ( this, options_str );
  if ( name == "inner_will"             ) return new inner_will_t            ( this, options_str );
  if ( name == "pain_suppression"       ) return new pain_suppression_t      ( this, options_str );
  if ( name == "power_infusion"         ) return new power_infusion_t        ( this, options_str );
  if ( name == "shadowform"            ) return new shadowform_t           ( this, options_str );
  if ( name == "vampiric_embrace"       ) return new vampiric_embrace_t      ( this, options_str );

  // Damage
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

  return 0;
}

// priest_t::create_pets ====================================================

void priest_t::create_pets()
{
  pets.shadow_fiend      = create_pet( "shadow_fiend"      );
  pets.lightwell         = create_pet( "lightwell"         );
}

// priest_t::init_base ======================================================

void priest_t::init_base()
{
  player_t::init_base();

  base_attack_power = -10;
  attribute_multiplier_initial[ ATTR_INTELLECT ]   *= 1.0 + spec.enlightenment -> effect1().percent();

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

  gains.dispersion                = get_gain( "dispersion" );
  gains.shadow_fiend              = get_gain( "shadow_fiend" );
  gains.archangel                 = get_gain( "archangel" );
  gains.hymn_of_hope              = get_gain( "hymn_of_hope" );
}

// priest_t::init_procs. =====================================================

void priest_t::init_procs()
{
  player_t::init_procs();

  procs.shadowy_apparation     = get_proc( "shadowy_apparation_proc" );
}

// priest_t::init_scaling ===================================================

void priest_t::init_scaling()
{
  player_t::init_scaling();

  // An Atonement Priest might be Health-capped
  scales_with[ STAT_STAMINA ] = glyphs.atonement -> ok();

  // For a Shadow Priest Spirit is the same as Hit Rating so invert it.
  if ( ( spec.twisted_faith -> ok() ) && ( sim -> scaling -> scale_stat == STAT_SPIRIT ) )
  {
    double v = sim -> scaling -> scale_value;

    if ( ! sim -> scaling -> positive_scale_delta )
    {
      invert_scaling = 1;
      attribute_initial[ ATTR_SPIRIT ] -= v * 2;
    }
  }
}

// priest_t::init_benefits ===================================================

void priest_t::init_benefits()
{
  player_t::init_benefits();

  benefits.mind_spike[ 0 ] = get_benefit( "mind_spike_0" );
  benefits.mind_spike[ 1 ] = get_benefit( "mind_spike_1" );
  benefits.mind_spike[ 2 ] = get_benefit( "mind_spike_2" );
  benefits.mind_spike[ 3 ] = get_benefit( "mind_spike_3" );

  benefits.shadow_orb[ 0 ] = get_benefit( "Percentage of Mind Blasts benefiting from 0 Shadow Orbs" );
  benefits.shadow_orb[ 1 ] = get_benefit( "Percentage of Mind Blasts benefiting from 1 Shadow Orbs" );
  benefits.shadow_orb[ 2 ] = get_benefit( "Percentage of Mind Blasts benefiting from 2 Shadow Orbs" );
  benefits.shadow_orb[ 3 ] = get_benefit( "Percentage of Mind Blasts benefiting from 3 Shadow Orbs" );
}

// priest_t::init_rng =======================================================

void priest_t::init_rng()
{
  player_t::init_rng();

}

// priest_t::init_talents ===================================================

void priest_t::init_talents()
{
  talents.void_tendrils               = find_talent( "Void Tendrils" );
  talents.psyfiend                    = find_talent( "Psyfiend" );
  talents.dominate_mind               = find_talent( "Dominate Mind" );
  talents.body_and_soul               = find_talent( "Body and Soul" );
  talents.path_of_the_devout          = find_talent( "Path of the Devout" );
  talents.phantasm                    = find_talent( "Phantasm" );
  talents.from_darkness_comes_light   = find_talent( "From Darkness, Comes Light" );
  // "coming soon"
  talents.archangel                   = find_talent( "Archangel" );
  talents.desperate_prayer            = find_talent( "Desperate Prayer" );
  talents.void_shift                  = find_talent( "Void Shift" );
  talents.angelic_bulwark             = find_talent( "Angelic Bulwark" );
  talents.twist_of_fate               = find_talent( "Twist of Fate" );
  talents.power_infusion              = find_talent( "Power Infusion" );
  talents.divine_insight              = find_talent( "Divine Insight" );
  talents.cascade                     = find_talent( "Cascade" );
  talents.divine_star                 = find_talent( "Divine Star" );
  talents.halo                        = find_talent( "Halo" );

  player_t::init_talents();
}

// priest_t::init_spells
void priest_t::init_spells()
{
  player_t::init_spells();

  // Passive Spells

  // Discipline
  spec.enlightenment          = new spell_id_t( this, "enlightenment", "Enlightenment" );
  spec.meditation_disc        = new spell_id_t( this, "meditation_disc", "Meditation" );
  spec.divine_aegis           = find_specialization_spell( "Divine Aegis" );
  spec.grace = find_specialization_spell( "Grace" );
  spec.evangelism = find_specialization_spell( "Evangelism" );
  spec.train_of_thought = find_specialization_spell( "Train of Thought" );

  // Holy
  spec.spiritual_healing      = new spell_id_t( this, "spiritual_healing", "Spiritual Healing" );
  spec.meditation_holy        = new spell_id_t( this, "meditation_holy", 95861 );
  spec.revelations            = find_specialization_spell( "Revelations" );

  // Shadow
  spec.shadow_power           = new spell_id_t( this, "shadow_power", "Shadow Power" );
  spec.shadow_orbs            = new spell_id_t( this, "shadow_orbs", "Shadow Orbs" );
  spec.shadowy_apparition_num = new spell_id_t( this, "shadowy_apparition_num", 78202 );
  spec.twisted_faith = find_specialization_spell( "Twisted Faith" );
  spec.shadowform = find_specialization_spell( "Shadowform" );

  // Mastery Spells
  mastery_spells.shield_discipline = new mastery_t( this, "shield_discipline", "Shield Discipline", TREE_DISCIPLINE );
  mastery_spells.echo_of_light     = new mastery_t( this, "echo_of_light", "Echo of Light", TREE_HOLY );
  mastery_spells.shadow_orb_power  = new mastery_t( this, "shadow_orb_power", "Shadow Orb Power", TREE_SHADOW );

  // Glyphs
  glyphs.circle_of_healing  = find_glyph( "Glyph of Circle of Healing" );
  glyphs.dispersion         = find_glyph( "Glyph of Dispersion" );
  glyphs.holy_nova          = find_glyph( "Glyph of Holy Nova" );
  glyphs.inner_fire         = find_glyph( "Glyph of Inner Fire" );
  glyphs.lightwell          = find_glyph( "Glyph of Lightwell" );
  glyphs.penance            = find_glyph( "Glyph of Penance" );
  glyphs.power_word_shield  = find_glyph( "Glyph of Power Word: Shield" );
  glyphs.prayer_of_mending  = find_glyph( "Glyph of Prayer of Mending" );
  glyphs.renew              = find_glyph( "Glyph of Renew" );
  glyphs.smite              = find_glyph( "Glyph of Smite" );
  glyphs.atonement         = find_glyph( "Atonement" ); //find_glyph( "Glyph of Spirit Tap" );
  glyphs.mind_spike          = find_glyph( "Glyph of Mind Spike" );
  glyphs.strength_of_soul   = find_glyph( "Glyph of Strength of Soul" );
  glyphs.inner_sanctum      = find_glyph( "Glyph of Inner Sanctum" );

  // Set Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P    M2P    M4P    T2P    T4P     H2P     H4P
    { 105843, 105844,     0,     0,     0,     0, 105827, 105832 }, // Tier13
    {      0,      0,     0,     0,     0,     0,      0,      0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// priest_t::init_buffs =====================================================

void priest_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rngs.type=rngs.CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rngs.type=rngs.CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rngs.type=rngs.CYCLIC, activated=true )

  // Discipline
  buffs.dark_evangelism            = new buff_t( this, 87117, "dark_evangelism", spec.evangelism -> ok() );
  buffs.dark_evangelism -> activated = false;
  buffs.holy_evangelism            = new buff_t( this, 81660, "holy_evangelism", spec.evangelism -> ok() );
  buffs.holy_evangelism -> activated = false;
  buffs.dark_archangel             = new buff_t( this, 87153, "dark_archangel" );
  buffs.holy_archangel             = new buff_t( this, 81700, "holy_archangel" );
  buffs.inner_fire                 = new buff_t( this, "inner_fire", "Inner Fire" );
  buffs.inner_focus                = new buff_t( this, "inner_focus", "Inner Focus" );
  buffs.inner_focus -> cooldown -> duration = timespan_t::zero;
  buffs.inner_will                 = new buff_t( this, "inner_will", "Inner Will" );
  // Holy
  buffs.chakra_pre                 = new buff_t( this, 14751, "chakra_pre" );
  buffs.chakra_chastise            = new buff_t( this, 81209, "chakra_chastise" );
  buffs.chakra_sanctuary           = new buff_t( this, 81206, "chakra_sanctuary" );
  buffs.chakra_serenity            = new buff_t( this, 81208, "chakra_serenity" );
  buffs.serenity                   = new buff_t( this, 88684, "serenity" );
  buffs.serenity -> cooldown -> duration = timespan_t::zero;
  // TEST: buffs.serenity -> activated = false;

  // Shadow
  buffs.empowered_shadow           = new buff_t( this, 95799, "empowered_shadow" );
  buffs.glyph_of_shadow_word_death = new buff_t( this, "glyph_of_shadow_word_death", 1, timespan_t::from_seconds( 6.0 )  );
  buffs.glyph_mind_spike                  = new buff_t( this, glyphs.mind_spike -> effect2().trigger_spell_id(), "mind_spike"                 );
  buffs.mind_spike                 = new buff_t( this, "mind_spike",                 3, timespan_t::from_seconds( 12.0 ) );
  buffs.shadowform                = new buff_t( this, "shadowform", "Shadowform" );
  buffs.shadow_orb                 = new buff_t( this, spec.shadow_orbs -> effect1().trigger_spell_id(), "shadow_orb" );
  buffs.shadow_orb -> activated = false;
  buffs.shadowfiend                = new buff_t( this, "shadowfiend", 1, timespan_t::from_seconds( 15.0 ) ); // Pet Tracking Buff
  buffs.glyph_of_atonement        = new buff_t( this, 81301, "glyph_of_atonement" ); // FIXME: implement actual mechanics
  buffs.vampiric_embrace           = new buff_t( this, "vampiric_embrace", "Vampiric Embrace" );

  // Set Bonus
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

    buffer += "/shadowform";

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

      buffer += "|dot.vampiric_touch.remains<cast_time+2.5";

      buffer += "/vampiric_touch,if=(!ticking|dot.vampiric_touch.remains<cast_time+2.5)&miss_react";

      // Save  & reset buffer ===============================================
      for ( unsigned int i = 0; i < action_priority_list.size(); i++ )
      {
        action_priority_list_t* a = action_priority_list[i];
        a -> action_list_str += buffer;
      }
      buffer.clear();
      // ====================================================================

      list_double_dot += "/vampiric_touch_2,if=(!ticking|dot.vampiric_touch_2.remains<cast_time+2.5)&miss_react";

      list_double_dot += "/shadow_word_pain_2,if=(!ticking|dot.shadow_word_pain_2.remains<gcd+0.5)&miss_react";

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
        buffer += "/power_infusion";
        buffer += "/power_word_shield,if=buff.weakened_soul.down";

        if ( level >= 28 )
          buffer += "/devouring_plague,if=miss_react&(remains<tick_time|!ticking)";

        buffer += "/shadow_word_pain,if=miss_react&(remains<tick_time|!ticking)";

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
        list_default += "/inner_focus";
        list_default += "/power_infusion";
        list_default += "/power_word_shield";
        list_default += "/greater_heal,if=buff.inner_focus.up";

        list_default += "/holy_fire";
        if ( glyphs.atonement -> ok() )
        {
          list_default += "/smite,if=";
          if ( glyphs.smite -> ok() )
            list_default += "dot.holy_fire.remains>cast_time&";
          list_default += "buff.holy_evangelism.stack<5&buff.holy_archangel.down";
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
        list_pws += "/inner_focus";
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
                                                         list_default += "/chakra";
                                                         list_default += "/holy_fire";
        if ( level >= 28 )                               list_default += "/devouring_plague,if=remains<tick_time|!ticking";
                                                         list_default += "/shadow_word_pain,if=remains<tick_time|!ticking";
                                                         list_default += "/mind_blast";
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
/*
  for ( action_t* a = action_list; a; a = a -> next )
  {
    double c = a -> cost();
    if ( c > max_mana_cost ) max_mana_cost = c;
  }*/
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
  constants.meditation_value                = spec.meditation_disc    -> ok() ?
                                              spec.meditation_disc  -> base_value( E_APPLY_AURA, A_MOD_MANA_REGEN_INTERRUPT ) :
                                              spec.meditation_holy  -> base_value( E_APPLY_AURA, A_MOD_MANA_REGEN_INTERRUPT );

  // Shadow Core
  constants.shadow_power_damage_value       = spec.shadow_power       -> effect1().percent();
  constants.shadow_power_crit_value         = spec.shadow_power       -> effect2().percent();
  constants.shadow_orb_proc_value           = mastery_spells.shadow_orb_power   -> proc_chance();
  constants.shadow_orb_mastery_value        = mastery_spells.shadow_orb_power   -> base_value( E_APPLY_AURA, A_DUMMY, P_GENERIC );

  // Shadow
  constants.twisted_faith_static_value      = spec.twisted_faith             -> effect2().percent();
  constants.twisted_faith_dynamic_value     = spec.twisted_faith             -> effect1().percent();
  constants.shadowform_value               = spec.shadowform               -> effect2().percent();
  constants.devouring_plague_health_mod     = 0.15;

  constants.max_shadowy_apparitions         = spec.shadowy_apparition_num -> effect1().base_value();

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

  // FIXME: bring back conditional
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
      atonement->resource_gain->merge( trigger->resource_gain );
      atonement -> total_execute_time = trigger -> total_execute_time;
      atonement -> total_tick_time = trigger -> total_tick_time;
    }
  }
}

// priest_t::pre_analyze_hook  ==============================================

void priest_t::pre_analyze_hook()
{
  if ( glyphs.atonement -> ok() )
  {
    fixup_atonement_stats( "smite", "atonement_smite" );
    fixup_atonement_stats( "holy_fire", "atonement_holy_fire" );
  }
}

// priest_t::target_mitigation ==============================================

double priest_t::target_mitigation( double            amount,
                                    const school_type school,
                                    int               dmg_type,
                                    int               result,
                                    action_t*         action )
{
  amount = player_t::target_mitigation( amount, school, dmg_type, result, action );

  if ( buffs.shadowform -> check() )
  { amount *= 1.0 + buffs.shadowform -> effect3().percent(); }

  if ( glyphs.inner_sanctum -> ok() )
  { amount *= 1.0 - glyphs.inner_sanctum -> effect1().percent(); }

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

  priest_t* source_p = dynamic_cast<priest_t*>( source );

  assert( source_p );

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

#endif // SC_PRIEST

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_priest  =================================================

player_t* player_t::create_priest( sim_t* sim, const std::string& name, race_type r )
{
  SC_CREATE_PRIEST( sim, name, r );
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
