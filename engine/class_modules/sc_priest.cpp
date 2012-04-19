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
  dot_t*  dots_holy_fire;
  dot_t*  dots_renew;

  buff_t* buffs_power_word_shield;
  buff_t* buffs_divine_aegis;

  remove_dots_event_t* remove_dots_event;

  priest_targetdata_t( priest_t* p, player_t* target );
};

void register_priest_targetdata( sim_t* sim )
{
  player_type_e t = PRIEST;
  typedef priest_targetdata_t type;

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
    buff_t* shadow_word_death_reset_cooldown;
    buff_t* mind_spike;
    buff_t* glyph_mind_spike;
    buff_t* shadowform;
    buff_t* shadowfiend;
    buff_t* vampiric_embrace;
    buff_t* shadow_of_death;
    buff_t* surge_of_darkness;
  } buffs;

  // Talents
  struct talents_t
  {
    const spell_data_t* void_tendrils;
    const spell_data_t* psyfiend;
    const spell_data_t* dominate_mind;
    const spell_data_t* body_and_soul;
    const spell_data_t* feathers_from_heaven;
    const spell_data_t* phantasm;
    const spell_data_t* from_darkness_comes_light;
    // "coming soon"
    const spell_data_t* archangel;
    const spell_data_t* desperate_prayer;
    const spell_data_t* void_shift;
    const spell_data_t* angelic_bulwark;
    const spell_data_t* twist_of_fate;
    const spell_data_t* power_infusion;
    const spell_data_t* divine_insight;
    const spell_data_t* cascade;
    const spell_data_t* divine_star;
    const spell_data_t* halo;
  } talents;

  // Specialization Spells
  struct specs_t
  {
    // General

    // Discipline
    const spell_data_t* meditation_disc;
    const spell_data_t* divine_aegis;
    const spell_data_t* grace;
    const spell_data_t* evangelism;
    const spell_data_t* train_of_thought;
    const spell_data_t* divine_fury;

    // Holy
    const spell_data_t* meditation_holy;
    const spell_data_t* revelations;
    const spell_data_t* chakra_chastise;
    const spell_data_t* chakra_sanctuary;
    const spell_data_t* chakra_serenity;

    // Shadow
    const spell_data_t* spiritual_precision;
    const spell_data_t* shadowform;
    const spell_data_t* shadowy_apparition;
  } spec;

  // Mastery Spells
  struct mastery_spells_t
  {
    const spell_data_t* shield_discipline;
    const spell_data_t* echo_of_light;
    const spell_data_t* shadow_orb_power;
  } mastery_spells;

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
    gain_t* shadow_orb_swp;
    gain_t* shadow_orb_mb;
    gain_t* shadow_orb_mastery_refund;
    gain_t* vampiric_touch_health;
    gain_t* vampiric_touch_mana;
  } gains;

  // Benefits
  struct benefits_t
  {
    std::array<benefit_t*, 4> mind_spike;
    benefits_t() { range::fill( mind_spike, 0 ); }
  } benefits;

  // Procs
  struct procs_t
  {
    proc_t* sa_shadow_orb_mastery;
  } procs;

  // Special

  struct spells_t
  {
    std::queue<spell_t*> apparitions_free;
    std::list<spell_t*>  apparitions_active;
    heal_t* echo_of_light;
    bool echo_of_light_merged;
    spells_t() : echo_of_light(), echo_of_light_merged() {}
  } spells;


  // Random Number Generators
  struct rngs_t
  {
    rng_t* sa_shadow_orb_mastery;
  } rngs;

  // Pets
  struct pets_t
  {
    pet_t* shadow_fiend;
    pet_t* lightwell;
  } pets;

  // Options
  int initial_shadow_orbs;
  std::string atonement_target_str;
  std::vector<player_t *> party_list;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* circle_of_healing;
    const spell_data_t* dispersion;
    const spell_data_t* holy_nova;
    const spell_data_t* inner_fire;
    const spell_data_t* lightwell;
    const spell_data_t* penance;
    const spell_data_t* power_word_shield;
    const spell_data_t* prayer_of_mending;
    const spell_data_t* renew;
    const spell_data_t* smite;

    // Mop
    const spell_data_t* atonement;
    const spell_data_t* holy_fire;
    const spell_data_t* mind_spike;
    const spell_data_t* strength_of_soul;
    const spell_data_t* inner_sanctum;

    const spell_data_t* mind_flay;
    const spell_data_t* mind_blast;
    const spell_data_t* vampiric_touch;
    const spell_data_t* vampiric_embrace;
    const spell_data_t* shadowy_apparition;
    const spell_data_t* fortitude;
  } glyphs;

  // Constants
  struct constants_t
  {
    double meditation_value;
  } constants;

  priest_t( sim_t* sim, const std::string& name, race_type_e r = RACE_NIGHT_ELF ) :
    player_t( sim, PRIEST, name, r ),
    // initialize containers. For POD containers this sets all elements to 0.
    buffs( buffs_t() ),
    talents( talents_t() ),
    spec( specs_t() ),
    mastery_spells( mastery_spells_t() ),
    cooldowns( cooldowns_t() ),
    gains( gains_t() ),
    benefits( benefits_t() ),
    procs( procs_t() ),
    spells( spells_t() ),
    rngs( rngs_t() ),
    pets( pets_t() ),
    initial_shadow_orbs(),
    glyphs( glyphs_t() ),
    constants( constants_t() )
  {
    distance                             = 40.0;
    default_distance                     = 40.0;

    cooldowns.mind_blast                 = get_cooldown( "mind_blast" );
    cooldowns.shadow_fiend               = get_cooldown( "shadow_fiend" );
    cooldowns.chakra                     = get_cooldown( "chakra"   );
    cooldowns.inner_focus                = get_cooldown( "inner_focus" );
    cooldowns.penance                    = get_cooldown( "penance" );

    create_options();
  }

  // Character Definition
  virtual priest_targetdata_t* new_targetdata( player_t* target )
  { return new priest_targetdata_t( this, target ); }
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_spells();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_actions();
  virtual void      init_procs();
  virtual void      init_scaling();
  virtual void      reset();
  virtual void      init_party();
  virtual void      create_options();
  virtual bool      create_profile( std::string& profile_str, save_type_e=SAVE_ALL, bool save_html=false );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual void      copy_from( player_t* source );
  virtual int       decode_set( const item_t& ) const;
  virtual resource_type_e primary_resource() const { return RESOURCE_MANA; }
  virtual role_type_e primary_role() const;
  virtual void      combat_begin();
  virtual double    composite_armor() const;
  virtual double    composite_spell_power( school_type_e school ) const;
  virtual double    composite_spell_hit() const;
  virtual double    composite_player_multiplier( school_type_e school, const action_t* a = NULL ) const;
  virtual double    composite_player_td_multiplier( school_type_e school, const action_t* a = NULL ) const;
  virtual double    composite_movement_speed() const;

  virtual double    matching_gear_multiplier( attribute_type_e attr ) const;

  virtual double    target_mitigation( double amount, school_type_e school, dmg_type_e, result_type_e, action_t* a=0 );

  virtual double    shadow_orb_amount() const;

  void fixup_atonement_stats( const std::string& trigger_spell_name, const std::string& atonement_spell_name );
  virtual void pre_analyze_hook();
};

priest_targetdata_t::priest_targetdata_t( priest_t* p, player_t* target ) :
  targetdata_t( p, target ), remove_dots_event( NULL )
{
  buffs_power_word_shield = add_aura( buff_creator_t( this, "power_word_shield", source -> find_spell( 17 ) ) );
  target -> absorb_buffs.push_back( buffs_power_word_shield );
  buffs_divine_aegis = add_aura( buff_creator_t( this, "divine_aegis", source -> find_spell( 47753 ) ) );
  target -> absorb_buffs.push_back( buffs_divine_aegis );
}

namespace // ANONYMOUS NAMESPACE ============================================
{

struct remove_dots_event_t : public event_t
{
private:
  priest_targetdata_t* const td;

  static void cancel_dot( dot_t* dot )
  {
    if ( dot -> ticking )
    {
      dot -> cancel();
      dot -> reset();
    }
  }

public:
  remove_dots_event_t( sim_t* sim, priest_t* pr, priest_targetdata_t* td ) :
    event_t( sim, pr, "mind_spike_remove_dots" ), td( td )
  {
    sim -> add_event( this, sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev ) );
  }

  virtual void execute()
  {
    td -> remove_dots_event = 0;
    cancel_dot( td -> dots_shadow_word_pain );
    cancel_dot( td -> dots_vampiric_touch );
  }
};

// ==========================================================================
// Priest Absorb
// ==========================================================================

struct priest_absorb_t : public absorb_t
{
  cooldown_t* min_interval;
  bool can_cancel_shadowform;
  bool castable_in_shadowform;

public:
  priest_absorb_t( const std::string& n, priest_t* player,
                   const spell_data_t* s = spell_data_t::nil(), school_type_e sc = SCHOOL_NONE ) :
    absorb_t( n, player, s, sc )
  {
    may_crit          = false;
    tick_may_crit     = false;
    may_miss          = false;
    min_interval      = player -> get_cooldown( "min_interval_" + name_str );
    can_cancel_shadowform = p() -> autoUnshift != 0;
    castable_in_shadowform = false;
    stateless         = true;
  }

  priest_targetdata_t* td( player_t* t = 0 ) const
  { return debug_cast<priest_targetdata_t*>( action_t::targetdata( t ) ); }

  priest_t* p() const
  { return static_cast<priest_t*>( player ); }

  inline bool check_shadowform()
  {
     return ( castable_in_shadowform || can_cancel_shadowform || ! p() -> buffs.shadowform -> check() );
  }

  inline void cancel_shadowform()
  {
     if ( ! castable_in_shadowform )
     {
       p() -> buffs.shadowform -> expire();
       if ( ! sim -> overrides.spell_haste ) sim -> auras.spell_haste -> decrement();
     }
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    return absorb_t::action_multiplier( s ) * ( 1.0 + ( p() -> composite_mastery() * p() -> mastery_spells.shield_discipline->effectN( 1 ).coeff() / 100.0 ) );
  }

  virtual double cost() const
  {
    double c = absorb_t::cost();

    if ( ( base_execute_time <= timespan_t::zero() ) && ! channeled )
    {
      c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> data().effectN( 1 ).percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void schedule_execute()
  {
    cancel_shadowform();
    absorb_t::schedule_execute();
  }

  virtual void consume_resource()
  {
    absorb_t::consume_resource();

    if ( base_execute_time <= timespan_t::zero() )
      p() -> buffs.inner_will -> up();
  }

  bool ready()
  {
    if ( min_interval -> remains() > timespan_t::zero() )
      return false;

    if ( ! absorb_t::ready() )
      return false;

    return check_shadowform();
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

    if ( min_interval -> duration > timespan_t::zero() && ! dual )
    {
      min_interval -> start( timespan_t::min(), timespan_t::zero() );

      if ( sim -> debug ) log_t::output( sim, "%s starts min_interval for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready.total_seconds() );
    }
  }
};

// ==========================================================================
// Priest Heal
// ==========================================================================

struct priest_heal_t : public heal_t
{
  bool can_cancel_shadowform;
  bool castable_in_shadowform;

  inline bool check_shadowform()
  {
     return ( castable_in_shadowform || can_cancel_shadowform || ! p() -> buffs.shadowform -> check() );
  }

  inline void cancel_shadowform()
  {
     if ( ! castable_in_shadowform )
     {
       // FIX-ME: Needs to drop haste aura too.
       p() -> buffs.shadowform -> expire();
     }
  }

  struct divine_aegis_t : public priest_absorb_t
  {
    double shield_multiple;

    divine_aegis_t( const std::string& n, priest_t* p ) :
      priest_absorb_t( n, p, p -> find_spell( 47753 ) ), shield_multiple( 0 )
    {
      check_spell( p -> spec.divine_aegis );

      proc             = true;
      background       = true;
      direct_power_mod = 0;

      shield_multiple  = p -> spec.divine_aegis -> effectN( 1 ).percent();
    }

    virtual void impact_s( action_state_t* s )
    {
      double old_amount = td( s -> target ) -> buffs_divine_aegis -> current_value;
      double new_amount = std::min( s -> target -> resources.current[ RESOURCE_HEALTH ] * 0.4 - old_amount, s -> result_amount );
      td( s -> target ) -> buffs_divine_aegis -> trigger( 1, old_amount + new_amount );
      stats -> add_result( sim -> report_overheal ? new_amount : s -> result_amount, s -> result_amount, ABSORB, s -> result );
    }
  };

  bool can_trigger_DA;
  divine_aegis_t* da;
  cooldown_t* min_interval;

  void trigger_echo_of_light( priest_heal_t* a, player_t* t )
  {
    if ( ! p() -> mastery_spells.echo_of_light -> ok() )
      return;

    dot_t* d = p() -> spells.echo_of_light -> dot( t );
    if ( d -> ticking )
    {
      if ( p() -> spells.echo_of_light_merged )
      {
        // The old tick_dmg is multiplied by the remaining ticks, added to the new complete heal, and then again divided by num_ticks
        p() -> spells.echo_of_light -> base_td = ( p() -> spells.echo_of_light -> base_td *
                                                d -> ticks() +
                                                a -> direct_dmg * p() -> composite_mastery() *
                                                p() -> mastery_spells.echo_of_light -> effectN( 2 ).percent() / 100.0 ) /
                                                p() -> spells.echo_of_light -> num_ticks;
        d -> refresh_duration();
      }
      else
      {
        // The old tick_dmg is multiplied by the remaining ticks minus one!, added to the new complete heal, and then again divided by num_ticks
        p() -> spells.echo_of_light -> base_td = ( p() -> spells.echo_of_light -> base_td *
                                                ( d -> ticks() - 1 ) +
                                                a -> direct_dmg * p() -> composite_mastery() *
                                                p() -> mastery_spells.echo_of_light -> effectN( 2 ).percent() / 100.0 ) /
                                                p() -> spells.echo_of_light -> num_ticks;
        d -> refresh_duration();
        p() -> spells.echo_of_light_merged = true;
      }
    }
    else
    {
      p() -> spells.echo_of_light -> base_td = a -> direct_dmg * p() -> composite_mastery() *
                                              p() -> mastery_spells.echo_of_light -> effectN( 2 ).percent() / 100.0 /
                                              p() -> spells.echo_of_light -> num_ticks;
      p() -> spells.echo_of_light -> execute();
      p() -> spells.echo_of_light_merged = false;
    }
  }

  virtual void init()
  {
    heal_t::init();

    if ( can_trigger_DA && p() -> spec.divine_aegis -> ok() )
    {
      da = new divine_aegis_t( name_str + "_divine_aegis", p() );
      add_child( da );
      da -> target = target;
    }
  }

  priest_heal_t( const std::string& n, priest_t* player,
                 const spell_data_t* s = spell_data_t::nil(), school_type_e sc=SCHOOL_NONE ) :
    heal_t( n, player, s, sc ), can_trigger_DA( true ), da()
  {
    min_interval = player -> get_cooldown( "min_interval_" + name_str );
    can_cancel_shadowform = p() -> autoUnshift != 0;
    castable_in_shadowform = false;
    stateless = true;
  }

  priest_targetdata_t* td( player_t* t = 0 ) const
  { return debug_cast<priest_targetdata_t*>( action_t::targetdata( t ) ); }

  priest_t* p() const
  { return static_cast<priest_t*>( player ); }

  virtual double composite_crit( const action_state_t* s ) const
  {
    double cc = heal_t::composite_crit( s );

    if ( p() -> buffs.chakra_serenity -> up() )
      cc += p() -> buffs.chakra_serenity -> data().effectN( 1 ).percent();

    if ( p() -> buffs.serenity -> up() )
      cc += p() -> buffs.serenity -> data().effectN( 2 ).percent();

    return cc;
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    return heal_t::action_multiplier( s ) * ( 1.0 + p() -> buffs.holy_archangel -> value() );
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double ctm = heal_t::composite_target_multiplier( t );

    if ( p() -> spec.grace -> ok() )
      ctm *= 1.0 + t -> buffs.grace -> check() * t -> buffs.grace -> value();

    return ctm;
  }

  virtual void schedule_execute()
  {
    cancel_shadowform();
    heal_t::schedule_execute();
  }

  virtual double cost() const
  {
    double c = heal_t::cost();

    if ( ( base_execute_time <= timespan_t::zero() ) && ! channeled )
    {
      c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> data().effectN( 1 ).percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    heal_t::consume_resource();

    if ( base_execute_time <= timespan_t::zero() )
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

  virtual void impact_s( action_state_t* s )
  {
    heal_t::impact_s( s );

    if ( s -> result_amount > 0 )
    {
      // Divine Aegis
      if ( s -> result == RESULT_CRIT )
      {
        trigger_divine_aegis( s -> target, s -> result_amount );
      }

      trigger_echo_of_light( this, s -> target );

      if ( p() -> buffs.chakra_serenity -> up() && td( s -> target ) -> dots_renew -> ticking )
        td( s -> target ) -> dots_renew -> refresh_duration();
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
      t -> buffs.grace -> trigger( 1, p() -> spec.grace -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() / 100.0 );
  }

  void trigger_strength_of_soul( player_t* t )
  {
    if ( p() -> glyphs.strength_of_soul -> ok() && t -> buffs.weakened_soul -> up() )
      t -> buffs.weakened_soul -> extend_duration( p(), timespan_t::from_seconds( -1 * p() -> glyphs.strength_of_soul -> effectN( 1 ).base_value() ) );
  }

  void update_ready()
  {
    heal_t::update_ready();

    if ( min_interval -> duration > timespan_t::zero() && ! dual )
    {
      min_interval -> start( timespan_t::min(), timespan_t::zero() );

      if ( sim -> debug ) log_t::output( sim, "%s starts min_interval for %s (%s). Will be ready at %.4f", player -> name(), name(), cooldown -> name(), cooldown -> ready.total_seconds() );
    }
  }

  bool ready()
  {
    if ( ! heal_t::ready() )
      return false;

    if ( ! check_shadowform() )
      return false;

    return ( min_interval -> remains() <= timespan_t::zero() );
  }

  void parse_options( option_t*          options,
                      const std::string& options_str )
  {
    const option_t base_options[] =
    {
      { "min_interval", OPT_TIMESPAN, &( min_interval -> duration ) },
      { NULL,           OPT_UNKNOWN,  NULL      }
    };

    std::vector<option_t> merged_options;
    heal_t::parse_options( option_t::merge( merged_options, options, base_options ), options_str );
  }

  void consume_inner_focus();
};

struct echo_of_light_t : public priest_heal_t
{
  echo_of_light_t( priest_t* p ) :
    priest_heal_t( "echo_of_light", p, p -> find_spell( 77489 ) )
  {
    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks      = static_cast<int>( data().duration() / base_tick_time );

    background     = true;
    tick_may_crit  = false;
    hasted_ticks   = false;
    may_crit       = false;

  }
};
// Atonement heal ===========================================================

struct atonement_heal_t : public priest_heal_t
{
  atonement_heal_t( const std::string& n, priest_t* p ) :
    priest_heal_t( n, p, p -> find_spell( 81751 ) )
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

  void trigger( double atonement_dmg, int dmg_type_e, int result )
  {
    atonement_dmg *= p() -> glyphs.atonement -> effectN( 1 ).percent();
    double cap = p() -> resources.max[ RESOURCE_HEALTH ] * 0.3;

    if ( result == RESULT_CRIT )
    {
      // FIXME: Assume capped at 200% of the non-crit cap.
      cap *= 2.0;
    }

    if ( atonement_dmg > cap )
      atonement_dmg = cap;

    if ( dmg_type_e == DMG_OVER_TIME )
    {
      // num_ticks = 1;
      base_td = atonement_dmg;
      tick_may_crit = ( result == RESULT_CRIT );
      tick( dot() );
    }
    else
    {
      assert( dmg_type_e == DMG_DIRECT );
      // num_ticks = 0;
      base_dd_min = base_dd_max = atonement_dmg;
      may_crit = ( result == RESULT_CRIT );

      execute();
    }
  }

  virtual double total_crit_bonus() const
  { return 0; }

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

struct priest_spell_state_t : public action_state_t
{
  priest_spell_state_t( action_t* a, priest_t* t ) :
    action_state_t( a, t )
  { }
};

struct priest_spell_t : public spell_t
{
  atonement_heal_t* atonement;
  bool can_trigger_atonement;
  bool castable_in_shadowform;
  bool can_cancel_shadowform;
  buff_t* sform;

  inline bool check_shadowform()
  {
    return ( castable_in_shadowform || can_cancel_shadowform || ( sform -> current_stack == 0 ) );
  }

  inline void cancel_shadowform()
  {
    if ( ! castable_in_shadowform )
    {
      // FIX-ME: Needs to drop haste aura too.
      sform  -> expire();
    }
  }

  priest_spell_t( const std::string& n, priest_t* player,
                  const spell_data_t* s = spell_data_t::nil(), school_type_e sc = SCHOOL_NONE ) :
    spell_t( n, player, s, sc ),
    atonement( 0 ), can_trigger_atonement( 0 )
  {
    may_crit          = true;
    tick_may_crit     = true;
    stateless         = true;

    dot_behavior      = DOT_REFRESH;
    weapon_multiplier = 0.0;

    can_trigger_atonement = false;

    can_cancel_shadowform = p() -> autoUnshift != 0;
    castable_in_shadowform = true;
    sform = p() -> buffs.shadowform;
  }

  priest_t* p() const
  { return static_cast<priest_t*>( player ); }

  priest_targetdata_t* td( player_t* t = 0 ) const
  { return debug_cast<priest_targetdata_t*>( action_t::targetdata( t ) ); }

  virtual void schedule_execute()
  {
    cancel_shadowform();
    spell_t::schedule_execute();
  }

  virtual bool ready()
  {
    if ( ! spell_t::ready() )
      return false;

    return check_shadowform();
  }

  virtual void init()
  {
    spell_t::init();

    if ( can_trigger_atonement && p() -> glyphs.atonement -> ok() )
    {
      std::string n = "atonement_" + name_str;
      atonement = new atonement_heal_t( n, p() );
    }
  }

  virtual double cost() const
  {
    double c = spell_t::cost();

    if ( ( base_execute_time <= timespan_t::zero() ) && ! channeled )
    {
      c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> data().effectN( 1 ).percent();
      c  = floor( c );
    }

    return c;
  }

  virtual void consume_resource()
  {
    spell_t::consume_resource();

    if ( base_execute_time <= timespan_t::zero() )
      p() -> buffs.inner_will -> up();
  }

  virtual void assess_damage( player_t* t,
                              double amount,
                              dmg_type_e type,
                              result_type_e impact_result )
  {
    spell_t::assess_damage( t, amount, type, impact_result );

    if ( aoe == 0 && p() -> buffs.vampiric_embrace -> up() && result_is_hit( impact_result ) )
    {
      double a = amount * ( p() -> buffs.vampiric_embrace -> data().effectN( 1 ).percent() + p() -> glyphs.vampiric_embrace -> effectN( 2 ).percent() ) ;

      // Priest Heal
      p() -> resource_gain( RESOURCE_HEALTH, a, player -> gains.vampiric_embrace );

      // Pet Heal
      for ( size_t i = 0; i < player -> pet_list.size(); ++i )
      {
        pet_t* r = player -> pet_list[ i ];
        r -> resource_gain( RESOURCE_HEALTH, a, r -> gains.vampiric_embrace );
      }

      // Party Heal
      for ( size_t i = 0; i < p() -> party_list.size(); i++ )
      {
        player_t* q = p() -> party_list[ i ];

        q -> resource_gain( RESOURCE_HEALTH, a, q -> gains.vampiric_embrace );

        for ( size_t i = 0; i < player -> pet_list.size(); ++i )
        {
          pet_t* r = player -> pet_list[ i ];
          r -> resource_gain( RESOURCE_HEALTH, a, r -> gains.vampiric_embrace );
        }
      }
    }

    if ( atonement )
      atonement -> trigger( amount, type, impact_result );
  }

  static unsigned trigger_shadowy_apparition( priest_t* player );
private:
  static void add_more_shadowy_apparitions( priest_t*, size_t );
  friend void priest_t::init_spells();
public:
  static void generate_shadow_orb( action_t*, gain_t*, unsigned number=1 );
};

// ==========================================================================
// Pet Shadow Fiend
// ==========================================================================

struct shadow_fiend_pet_t : public pet_t
{
  struct shadowcrawl_t : public spell_t
  {
    shadowcrawl_t( shadow_fiend_pet_t* p ) :
      spell_t( "shadowcrawl", p, p -> shadowcrawl )
    {
      may_miss = false;
      stateless = true;
    }

    virtual void execute()
    {
      spell_t::execute();

      shadow_fiend_pet_t* p = static_cast<shadow_fiend_pet_t*>( player );

      p -> buffs.shadowcrawl -> trigger();
    }
  };

  struct melee_t : public attack_t
  {
    melee_t( shadow_fiend_pet_t* p ) :
      attack_t( "melee", p, spell_data_t::nil(), SCHOOL_SHADOW )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      weapon_multiplier = 0;
      direct_power_mod = 0.0063928 * p -> o() -> level;
      if ( harmful ) base_spell_power_multiplier = 1.0;
      base_attack_power_multiplier = 0.0;
      base_dd_min = util_t::ability_rank( player -> level,  290.0,85,  197.0,82,  175.0,80,  1.0,0 );
      base_dd_max = util_t::ability_rank( player -> level,  373.0,85,  245.0,82,  222.0,80,  2.0,0 );
      background = true;
      repeating  = true;
      may_dodge  = true;
      may_miss   = false;
      may_parry  = false; // Technically it can be parried on the first swing or if the rear isn't reachable
      may_crit   = true;
      may_block  = false; // Technically it can be blocked on the first swing or if the rear isn't reachable
      stateless  = true;
    }

    virtual double action_multiplier( const action_state_t* s ) const
    {
      double am = attack_t::action_multiplier( s );

      shadow_fiend_pet_t* p = static_cast<shadow_fiend_pet_t*>( player );

      am *= 1.0 + p -> buffs.shadowcrawl -> up() * p -> shadowcrawl -> effectN( 2 ).percent();

      return am;
    }

    virtual void execute()
    {
      attack_t::execute();

      shadow_fiend_pet_t* p = static_cast<shadow_fiend_pet_t*>( player );

      if ( p -> bad_swing )
        p -> bad_swing = false;
    }

    virtual void impact_s( action_state_t* s )
    {
      shadow_fiend_pet_t* p = static_cast<shadow_fiend_pet_t*>( player );

      attack_t::impact_s( s );

      if ( result_is_hit( s -> result ) )
      {
        p -> o() -> resource_gain( RESOURCE_MANA, p -> o() -> resources.max[ RESOURCE_MANA ] *
                                   p -> mana_leech -> effectN( 1 ).percent(),
                                   p -> o() -> gains.shadow_fiend );
      }
    }
  };

  double bad_spell_power;
  struct buffs_t
  {
    buff_t* shadowcrawl;
  } buffs;

  const spell_data_t* shadowcrawl;
  const spell_data_t* mana_leech;
  bool bad_swing;
  bool extra_tick;

  shadow_fiend_pet_t( sim_t* sim, priest_t* owner ) :
    pet_t( sim, owner, "shadow_fiend" ),
    bad_spell_power( util_t::ability_rank( owner -> level,  370.0,85,  358.0,82,  352.0,80,  0.0,0 ) ),
    shadowcrawl( spell_data_t::nil() ), mana_leech( spell_data_t::nil() ),
    bad_swing( false ), extra_tick( false )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
    main_hand_weapon.school     = SCHOOL_SHADOW;

    stamina_per_owner           = 0.30;
    intellect_per_owner         = 0.50;

    action_list_str             = "/snapshot_stats";
    action_list_str            += "/shadowcrawl";
    action_list_str            += "/wait_for_shadowcrawl";
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

    shadowcrawl = find_pet_spell( "Shadowcrawl" );
    mana_leech  = find_spell( 34650, "mana_leech" );
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

    buffs.shadowcrawl = buff_creator_t( this, "shadowcrawl", shadowcrawl );
  }

  virtual double composite_spell_power( school_type_e school ) const
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

  virtual double composite_attack_expertise( const weapon_t* /* w */ ) const
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }

  virtual double composite_attack_crit( const weapon_t* /* w */ ) const
  {
    double c = attack_crit;

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

  virtual void schedule_ready( timespan_t delta_time=timespan_t::zero(),
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
      heal_t( "lightwell_renew", player, player -> find_spell( 7001 ) )
    {
      may_crit = false;
      tick_may_crit = true;
      stateless = true;

      tick_power_mod = 0.308;
    }

    lightwell_pet_t* p()
    { return static_cast<lightwell_pet_t*>( player ); }

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

    action_list_str  = "/snapshot_stats";
    action_list_str += "/lightwell_renew";
    action_list_str += "/wait,sec=cooldown.lightwell_renew.remains";
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

    charges = 10 + o -> glyphs.lightwell -> effectN( 1 ).base_value();

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
    priest_spell_t( "shadowy_apparition_spell", player, player -> find_spell( 87532 ) )
  {
    background        = true;
    proc              = true;
    callbacks         = false;

    trigger_gcd       = timespan_t::zero();
    travel_speed      = 3.5;
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_spell_t::impact_s( s );

    // Cleanup. Re-add to free list.
    p() -> spells.apparitions_active.remove( this );
    p() -> spells.apparitions_free.push( this );
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double m = priest_spell_t::action_multiplier( s );

    // Bug: Last tested Build 15589
    if ( p() -> buffs.shadowform -> check() )
      m *= 1.15 / ( 1.0 + p() -> spec.shadowform -> effectN( 2 ).percent() );
    return m;
  }
};

unsigned priest_spell_t::trigger_shadowy_apparition( priest_t* p )
{
  // Trigger Shadowy apparitions and report back the number of successful Shadow Orb refunds
  unsigned shadow_orb_procs = 0;

  assert ( p -> spec.shadowy_apparition -> ok() );

  if ( ! p -> spells.apparitions_free.empty() )
  {
    for ( int i = static_cast<int>( p -> resources.current[ RESOURCE_SHADOW_ORB ] ); i > 0; i-- )
    {
      spell_t* s = p -> spells.apparitions_free.front();

      p -> spells.apparitions_free.pop();

      p -> spells.apparitions_active.push_back( s );

      if ( p -> rngs.sa_shadow_orb_mastery -> roll( p -> shadow_orb_amount() ) )
      {
        p -> procs.sa_shadow_orb_mastery -> occur();
        shadow_orb_procs++;
      }

      s -> execute();
    }
  }

  return shadow_orb_procs;
}

void priest_spell_t::add_more_shadowy_apparitions( priest_t* p, size_t n )
{
  spell_t* s = NULL;

  if ( ! p -> spells.apparitions_free.size() )
  {
    for ( size_t i = 0; i < n; i++ )
    {
      s = new shadowy_apparition_spell_t( p );
      p -> spells.apparitions_free.push( s );
    }
  }

  if ( p -> sim -> debug )
    log_t::output( p -> sim, "%s created %d shadowy apparitions", p -> name(), static_cast<unsigned>( p -> spells.apparitions_free.size() ) );
}

void priest_spell_t::generate_shadow_orb( action_t* s, gain_t* g, unsigned number )
{
  if ( s -> player -> primary_tree() != PRIEST_SHADOW )
    return;

  s -> player -> resource_gain( RESOURCE_SHADOW_ORB, number, g, s );
}

void trigger_chakra( priest_t* p, buff_t* chakra_buff )
{
  if ( p -> buffs.chakra_pre -> up() )
  {
    chakra_buff -> trigger();

    p -> buffs.chakra_pre -> expire();

    p -> cooldowns.chakra -> reset();
    p -> cooldowns.chakra -> duration = p -> buffs.chakra_pre -> data().cooldown();
    p -> cooldowns.chakra -> start();
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
  archangel_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "archangel", player, player -> find_talent_spell( "Archangel" ) )
  {
    parse_options( NULL, options_str );

    harmful           = false;

    if ( p() -> primary_tree() == PRIEST_SHADOW )
    {
      cooldown -> duration = p() -> buffs.dark_archangel -> buff_cooldown;
    }
    else
    {
      cooldown -> duration = p() -> buffs.holy_archangel -> buff_cooldown;
    }
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    if ( p() -> primary_tree() == PRIEST_SHADOW )
    {
      p() -> buffs.dark_archangel -> trigger( 1, p() -> buffs.dark_archangel -> default_value );
    }
    else
    {
      p() -> buffs.holy_archangel -> trigger( 1, p() -> buffs.holy_archangel -> default_value * p() -> buffs.holy_evangelism -> stack() );
      p() -> buffs.holy_evangelism -> expire();
    }
  }
};

// Chakra_Pre Spell =========================================================

struct chakra_t : public priest_spell_t
{
  chakra_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "chakra", p, p -> find_class_spell( "Chakra" ) )
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
    priest_spell_t( "dispersion", player, player -> find_class_spell( "Dispersion" ) )
  {
    parse_options( NULL, options_str );

    base_tick_time    = timespan_t::from_seconds( 1.0 );
    num_ticks         = 6;

    channeled         = true;
    harmful           = false;
    tick_may_crit     = false;

    cooldown -> duration += p() -> glyphs.dispersion -> effectN( 1 ).time_value();
  }

  virtual void tick( dot_t* d )
  {
    double regen_amount = p() -> find_spell( 49766 ) -> effectN( 1 ).percent() * p() -> resources.max[ RESOURCE_MANA ];

    p() -> resource_gain( RESOURCE_MANA, regen_amount, p() -> gains.dispersion );

    priest_spell_t::tick( d );
  }
};

// Fortitude Spell ==========================================================

struct fortitude_t : public priest_spell_t
{
  fortitude_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "fortitude", player, player -> find_class_spell( "Power Word: Fortitude" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    background = ( sim -> overrides.stamina != 0 );
  }

  virtual double cost() const
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + p() -> glyphs.fortitude -> effectN( 1 ).percent();

    return c;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    if ( ! sim -> overrides.stamina )
      sim -> auras.stamina -> trigger();

    /*
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
    */
  }
};

// Hymn of Hope Spell =======================================================

struct hymn_of_hope_tick_t : public priest_spell_t
{
  hymn_of_hope_tick_t( priest_t* player ) :
    priest_spell_t( "hymn_of_hope_tick", player, player -> find_spell( 64904 ) )
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

    p() -> resource_gain( RESOURCE_MANA, data().effectN( 1 ).percent() * p() -> resources.max[ RESOURCE_MANA ], p() -> gains.hymn_of_hope );

    // Hymn of Hope only adds +x% of the current_max mana, it doesn't change if afterwards max_mana changes.
    player -> buffs.hymn_of_hope -> trigger();
  }
};

struct hymn_of_hope_t : public priest_spell_t
{
  hymn_of_hope_tick_t* hymn_of_hope_tick;

  hymn_of_hope_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "hymn_of_hope", p, p -> find_class_spell( "Hymn of Hope" ) ),
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
    priest_spell_t( "inner_focus", p, p -> find_class_spell( "Inner Focus" ) )
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
    priest_spell_t( "inner_fire", player, player -> find_class_spell( "Inner Fire" ) )
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
    priest_spell_t( "inner_will", player, player -> find_class_spell( "Inner Will" ) )
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
    priest_spell_t( "pain_suppression", p, p -> find_class_spell( "Pain Suppression" ) )
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
    priest_spell_t( "power_infusion", p, p -> find_talent_spell( "Power Infusion" ) )
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
    priest_spell_t( "shadowform", p, p -> find_class_spell( "Shadowform" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.shadowform -> trigger();

    if ( ! sim -> overrides.spell_haste )
      sim -> auras.spell_haste -> trigger();
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
    priest_spell_t( "shadow_fiend", p, p -> find_class_spell( "Shadowfiend" ) )
  {
    parse_options( NULL, options_str );

    cooldown = p -> cooldowns.shadow_fiend;
    cooldown -> duration = data().cooldown();

    harmful = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> pets.shadow_fiend -> summon( data().duration() );
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
    priest_spell_t( "mind_blast", player, player -> find_class_spell( "Mind Blast" ) )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new mind_blast_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    timespan_t saved_cooldown = cooldown -> duration;

    priest_spell_t::execute();

    cooldown -> duration = saved_cooldown;

    for ( int i=0; i < 4; i++ )
    {
      p() -> benefits.mind_spike[ i ] -> update( i == p() -> buffs.mind_spike -> stack() );
    }

    p() -> buffs.mind_spike -> expire();
    p() -> buffs.glyph_mind_spike -> expire();

    generate_shadow_orb( this, p() -> gains.shadow_orb_mb );
  }

  virtual double action_multiplier( const action_state_t* a ) const
  {
    double m = priest_spell_t::action_multiplier( a );

    if ( p() -> buffs.dark_archangel -> check() )
    {
      m *= 1.0 + p() -> buffs.dark_archangel -> value();
    }

    return m;
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> buffs.shadow_of_death -> trigger();
    }
  }

  virtual double composite_crit( const action_state_t* s ) const
  {
    double c = priest_spell_t::composite_crit( s );

    c += p() -> buffs.mind_spike -> value() * p() -> buffs.mind_spike -> check();

    return c;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t a = priest_spell_t::execute_time();

    a *= 1 + ( p() -> buffs.glyph_mind_spike -> stack() * p() -> glyphs.mind_spike -> effect1().percent() );

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
               const std::string& name = "mind_flay" ) :
    priest_spell_t( name, p, p -> find_class_spell( "Mind Flay" ) ), mb_wait( timespan_t::zero() ), cut_for_mb( 0 ), no_dmg( 0 )
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
  }

  virtual double action_multiplier( const action_state_t* a ) const
  {
    double m = priest_spell_t::action_multiplier( a );

    if ( p() -> buffs.dark_archangel -> up() )
    {
      m *= 1.0 + p() -> buffs.dark_archangel -> value();
    }

    return m;
  }

  virtual double calculate_tick_damage( result_type_e r, double power, double multiplier )
  {
    if ( no_dmg )
      return 0.0;

    return priest_spell_t::calculate_tick_damage( r, power, multiplier );
  }

  virtual bool ready()
  {
    // If this option is set (with a value in seconds), don't cast Mind Flay if Mind Blast
    // is about to come off it's cooldown.
    if ( mb_wait != timespan_t::zero() )
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
    priest_spell_t( "mind_spike", player, player -> find_class_spell( "Mind Spike" ) )
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
    priest_spell_t::reset();

    td( target ) -> remove_dots_event = 0;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    if ( p() -> buffs.chakra_pre -> up() )
    {
      p() -> buffs.chakra_chastise -> trigger();

      p() -> buffs.chakra_pre -> expire();

      p() -> cooldowns.chakra -> reset();
      p() -> cooldowns.chakra -> duration  = p() -> buffs.chakra_pre -> data().cooldown();
      p() -> cooldowns.chakra -> start();
    }
  }

  virtual double action_multiplier( const action_state_t* a ) const
  {
    double m = priest_spell_t::action_multiplier( a );

    if ( p() -> buffs.dark_archangel -> up() )
    {
      m *= 1.0 + p() -> buffs.dark_archangel -> value();
    }

    return m;
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_spell_t::impact_s( s );

    if ( result_is_hit( s -> result ) )
    {

      p() -> buffs.mind_spike -> trigger( 1, data().effectN( 2 ).percent() );
      if ( p() -> glyphs.mind_spike -> ok() )
        p() -> buffs.glyph_mind_spike -> trigger();

      if ( p() -> buffs.surge_of_darkness -> up() )
      {
        p() -> buffs.surge_of_darkness -> expire();
      }
      else
      {
        if ( ! td( s -> target ) -> remove_dots_event )
        {
          td( s -> target ) -> remove_dots_event = new ( sim ) remove_dots_event_t( sim, p(), td( s -> target ) );
        }
      }
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t a = priest_spell_t::execute_time();

    if ( p() -> buffs.surge_of_darkness -> check() )
      a *= 0.0;

    return a;
  }
};

// Mind Sear Spell ==========================================================

struct mind_sear_tick_t : public priest_spell_t
{
  mind_sear_tick_t( priest_t* player ) :
    priest_spell_t( "mind_sear_tick", player, player -> find_spell( 49821 ) )
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
    priest_spell_t( "mind_sear", player, player -> find_class_spell( "Mind Sear" ) ),
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
  struct swd_state_t : public action_state_t
  {
    bool talent_proc;

    swd_state_t( action_t* a, player_t* t ) : action_state_t( a, t ),
      talent_proc ( false )
    {

    }
  };

  shadow_word_death_t( priest_t* p, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "shadow_word_death", p, p -> find_class_spell( "Shadow Word: Death" ) )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * p -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new shadow_word_death_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual action_state_t* new_state()
  {
    return new swd_state_t( this, target );
  }

  virtual void snapshot_state( action_state_t* state, uint32_t flags )
  {
    swd_state_t* swd = static_cast< swd_state_t* >( state );

    swd -> talent_proc = p() -> buffs.shadow_of_death -> up();

    priest_spell_t::snapshot_state( state, flags );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    if ( ! is_dtr_action && ! p() -> buffs.shadow_word_death_reset_cooldown -> up() )
    {
      cooldown -> reset();
      p() -> buffs.shadow_word_death_reset_cooldown -> trigger();
    }

    p() -> buffs.shadow_of_death -> expire();
  }

  virtual void impact_s( action_state_t* s )
  {
    swd_state_t* swds = static_cast< swd_state_t* >( s );

    s -> result_amount = floor( s -> result_amount );

    double health_loss = s -> result_amount;

    if ( ( target -> health_percentage() < 20.0 ) || ( swds -> talent_proc ) )
    {
      s -> result_amount *= 4.0;
    }

    priest_spell_t::impact_s( s );

    p() -> assess_damage( health_loss, school, DMG_DIRECT, RESULT_HIT, this );
  }

  virtual double action_multiplier( const action_state_t* a ) const
  {
    double m = priest_spell_t::action_multiplier( a );

    if ( p() -> buffs.dark_archangel -> up() )
    {
      m *= 1.0 + p() -> buffs.dark_archangel -> value();
    }

    return m;
  }

  virtual bool ready()
  {
    return priest_spell_t::ready();
  }
};

// Shadow Word Pain Spell ===================================================

struct shadow_word_pain_t : public priest_spell_t
{
  shadow_word_pain_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "shadow_word_pain", p, p -> find_class_spell( "Shadow Word: Pain" ) )
  {
    parse_options( NULL, options_str );

    may_crit   = false;
    tick_zero = true;

    base_multiplier *= 1.0 + p -> sets -> set( SET_T13_4PC_CASTER ) -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    generate_shadow_orb( this, p() -> gains.shadow_orb_swp );
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    if ( ( tick_dmg > 0 ) && ( p() -> talents.from_darkness_comes_light -> ok() ) )
    {
      p() -> buffs.surge_of_darkness -> trigger();
    }
  }
};

// Vampiric Embrace Spell ===================================================

struct vampiric_embrace_t : public priest_spell_t
{
  vampiric_embrace_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "vampiric_embrace", p, p -> find_class_spell( "Vampiric Embrace" ) )
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
    priest_spell_t( "vampiric_touch", player, player -> find_class_spell( "Vampiric Touch" ) )
  {
    parse_options( NULL, options_str );
    may_crit   = false;
  }

  virtual void tick( dot_t* d )
  {
    priest_spell_t::tick( d );

    double h = tick_dmg * ( data().effectN( 3 ).percent() + p() -> glyphs.vampiric_touch -> effectN( 1 ).percent() );
    player -> resource_gain( RESOURCE_HEALTH, h, p() -> gains.vampiric_touch_health, this );

    double m = player->resources.max[ RESOURCE_MANA ] * data().effectN( 1 ).percent();
    player -> resource_gain( RESOURCE_MANA, m, p() -> gains.vampiric_touch_mana, this );

    if ( ( tick_dmg > 0 ) && ( p() -> talents.from_darkness_comes_light -> ok() ) )
    {
      p() -> buffs.surge_of_darkness -> trigger();
    }

    /* FIX-ME: Make sure this is still the case when MoP goes live */
    /* VT ticks are triggering OnHarmfulSpellCast procs. Twice at that..... */
    if ( callbacks )
    {
      action_callback_t::trigger( player -> harmful_spell_callbacks[ RESULT_NONE ], this );
      action_callback_t::trigger( player -> harmful_spell_callbacks[ RESULT_NONE ], this );
    }
  }
};

// Holy Fire Spell ==========================================================

struct holy_fire_t : public priest_spell_t
{
  holy_fire_t( priest_t* player, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "holy_fire", player, player -> find_class_spell( "Holy Fire" ) )
  {
    parse_options( NULL, options_str );

    base_hit += p() -> spec.divine_fury -> effectN( 1 ).percent();

    can_trigger_atonement = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new holy_fire_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }

    castable_in_shadowform = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.holy_evangelism  -> trigger();
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double m = priest_spell_t::action_multiplier( s );

    m *= 1.0 + ( p() -> buffs.holy_evangelism -> stack() * p() -> buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    return m;
  }

  virtual double cost() const
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    return c;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t a = priest_spell_t::execute_time();

    if ( p() -> glyphs.holy_fire -> ok() )
      a *= 0.0;

    return a;
  }

};

// Penance Spell ============================================================

struct penance_t : public priest_spell_t
{
  struct penance_tick_t : public priest_spell_t
  {
    penance_tick_t( priest_t* p ) :
      priest_spell_t( "penance_tick", p, p -> find_spell( 47666 ) )
    {
        background  = true;
      dual        = true;
      direct_tick = true;
      base_hit += p -> spec.divine_fury -> effectN( 1 ).percent();
    }

    virtual double action_multiplier( const action_state_t* s ) const
    {
      double m = priest_spell_t::action_multiplier( s );

      m *= 1.0 + ( p() -> buffs.holy_evangelism -> stack() * p() -> buffs.holy_evangelism -> data().effectN( 1 ).percent() );

      return m;
    }
  };

  penance_tick_t* tick_spell;

  penance_t( priest_t* p, const std::string& options_str ) :
    priest_spell_t( "penance", p, p -> find_class_spell( "Penance" ) ),
    tick_spell( 0 )
  {
    parse_options( NULL, options_str );

    may_crit       = false;
    may_miss       = false;
    channeled      = true;
    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

    cooldown -> duration = data().cooldown() + p -> glyphs.penance -> effectN( 2 ).time_value();

    tick_spell = new penance_tick_t( p );

    castable_in_shadowform = false;
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

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    c *= 1.0 + p() -> glyphs.penance -> effectN( 1 ).percent();

    return c;
  }
};

// Smite Spell ==============================================================

struct smite_t : public priest_spell_t
{
  smite_t( priest_t* p, const std::string& options_str, bool dtr=false ) :
    priest_spell_t( "smite", p, p -> find_class_spell( "Smite" ) )
  {
    parse_options( NULL, options_str );

    base_hit += p -> spec.divine_fury -> effectN( 1 ).percent();

    can_trigger_atonement = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new smite_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }

    castable_in_shadowform = false;
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> buffs.holy_evangelism -> trigger();

    trigger_chakra( p(), p() -> buffs.chakra_chastise );

    // Train of Thought
    if ( p() -> spec.train_of_thought -> ok() )
    {
      if ( p() -> cooldowns.penance -> remains() > p() -> spec.train_of_thought -> effectN( 2 ).time_value() )
        p() -> cooldowns.penance -> ready -= p() -> spec.train_of_thought -> effectN( 2 ).time_value();
      else
        p() -> cooldowns.penance -> reset();
    }
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double am = priest_spell_t::action_multiplier( s );

    am *= 1.0 + ( p() -> buffs.holy_evangelism -> stack() * p() -> buffs.holy_evangelism -> data().effectN( 1 ).percent() );

    if ( td( s -> target ) -> dots_holy_fire -> ticking && p() -> glyphs.smite -> ok() )
      am *= 1.0 + p() -> glyphs.smite -> effectN( 1 ).percent();

    return am;
  }

  virtual double cost() const
  {
    double c = priest_spell_t::cost();

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    return c;
  }
};

struct shadowy_apparition_t : priest_spell_t
{
  unsigned triggered_shadow_orb_mastery;

  shadowy_apparition_t( priest_t* player, const std::string& options_str ) :
    priest_spell_t( "shadowy_apparition", player, player -> find_class_spell( "Shadowy Apparition" ), SCHOOL_SHADOW ),
    triggered_shadow_orb_mastery( 0 )
  {
    parse_options( NULL, options_str );

    may_crit = false;
  }

  virtual void init()
  {
    priest_spell_t::init();

    stats_t* s = player -> find_stats( "shadowy_apparition_spell" );
    if ( s )
      stats -> add_child( s );
  }

  virtual void schedule_travel_s( action_state_t* s )
  {
    priest_spell_t::schedule_travel_s( s );

    triggered_shadow_orb_mastery = trigger_shadowy_apparition( p() );
  }

  virtual void consume_resource()
  {
    priest_spell_t::consume_resource();

    // implement mastery shadow orb restore, after the original resource got consumed,
    // but directly at spell execution, not when shadowy apparitions reach the target.
    // tested in mop beta, 02/04/2012 by philoptik@gmail.com

    priest_spell_t::generate_shadow_orb( this, p() -> gains.shadow_orb_mastery_refund, triggered_shadow_orb_mastery );

    triggered_shadow_orb_mastery = 0;
  }

  virtual double cost() const
  {
    return std::max( 1.0, player -> resources.current[ RESOURCE_SHADOW_ORB ] );
  }
};

// ==========================================================================
// Priest Heal & Absorb Spells
// ==========================================================================

void priest_heal_t::consume_inner_focus()
{
  if ( p() -> buffs.inner_focus -> up() )
  {
    // Inner Focus cooldown starts when consumed.
    p() -> cooldowns.inner_focus -> reset();
    p() -> cooldowns.inner_focus -> duration = p() -> buffs.inner_focus -> data().cooldown();
    p() -> cooldowns.inner_focus -> start();
    p() -> buffs.inner_focus -> expire();
  }
}

// Binding Heal Spell =======================================================

struct binding_heal_t : public priest_heal_t
{
  binding_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "binding_heal", p, p -> find_class_spell( "Binding Heal" ) )
  {
    parse_options( NULL, options_str );

    aoe = 1;
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    priest_heal_t::consume_inner_focus();

    trigger_chakra( p(), p() -> buffs.chakra_serenity );
  }

  virtual double composite_crit( const action_state_t* s ) const
  {
    double cc = priest_heal_t::composite_crit( s );

    if ( p() -> buffs.inner_focus -> check() )
      cc += p() -> buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
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
    priest_heal_t( "circle_of_healing", p, p -> find_class_spell( "Circle of Healing" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ] *= 1.0 + p -> glyphs.circle_of_healing -> effectN( 2 ).percent();
    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );
    aoe = p -> glyphs.circle_of_healing -> ok() ? 5 : 4;
  }

  virtual void execute()
  {
    cooldown -> duration = data().cooldown();

    if ( p() -> buffs.chakra_sanctuary -> up() )
      cooldown -> duration +=  p() -> buffs.chakra_sanctuary -> data().effectN( 2 ).time_value();

    // Choose Heal Target
    target = find_lowest_player();

    priest_heal_t::execute();
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double am = priest_heal_t::action_multiplier( s );

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};

// Divine Hymn Spell ========================================================

struct divine_hymn_tick_t : public priest_heal_t
{
  divine_hymn_tick_t( priest_t* player, int nr_targets ) :
    priest_heal_t( "divine_hymn_tick", player, player -> find_spell( 64844 ) )
  {
    background  = true;

    aoe = nr_targets - 1;
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double am = priest_heal_t::action_multiplier( s );

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};

struct divine_hymn_t : public priest_heal_t
{
  divine_hymn_tick_t* divine_hymn_tick;

  divine_hymn_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "divine_hymn", p, p -> find_class_spell( "Divine Hymn" ) ),
    divine_hymn_tick( 0 )
  {
    parse_options( NULL, options_str );

    harmful = false;
    channeled = true;

    divine_hymn_tick = new divine_hymn_tick_t( p, data().effectN( 2 ).base_value() );
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
    priest_heal_t( "flash_heal", p, p -> find_class_spell( "Flash Heal" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    priest_heal_t::consume_inner_focus();

    trigger_chakra( p(), p() -> buffs.chakra_serenity );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    trigger_grace( s -> target );
    trigger_strength_of_soul( s -> target );
  }

  virtual double composite_crit( const action_state_t* s ) const
  {
    double cc = priest_heal_t::composite_crit( s );

    if ( p() -> buffs.inner_focus -> check() )
      cc += p() -> buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
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
    priest_heal_t( "guardian_spirit", p, p -> find_class_spell( "Guardian Spirit" ) )
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
    priest_heal_t( "greater_heal", p, p -> find_class_spell( "Greater Heal" ) )
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
      if ( p() -> cooldowns.inner_focus -> remains() > timespan_t::from_seconds( p() -> spec.train_of_thought -> effectN( 1 ).base_value() ) )
        p() -> cooldowns.inner_focus -> ready -= timespan_t::from_seconds( p() -> spec.train_of_thought -> effectN( 1 ).base_value() );
      else
        p() -> cooldowns.inner_focus -> reset();
    }

    priest_heal_t::consume_inner_focus();

    trigger_chakra( p(), p() -> buffs.chakra_serenity );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    trigger_grace( s -> target );
    trigger_strength_of_soul( s -> target );
  }

  virtual double composite_crit( const action_state_t* s ) const
  {
    double cc = priest_heal_t::composite_crit( s );

    if ( p() -> buffs.inner_focus -> check() )
      cc += p() -> buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
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
    priest_heal_t( "heal", p, p -> find_class_spell( "Heal" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    priest_heal_t::execute();

    trigger_chakra( p(), p() -> buffs.chakra_serenity );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    trigger_grace( s -> target );
    trigger_strength_of_soul( s -> target );
  }
};

// Holy Word Sanctuary ======================================================

struct holy_word_sanctuary_t : public priest_heal_t
{
  struct holy_word_sanctuary_tick_t : public priest_heal_t
  {
    holy_word_sanctuary_tick_t( priest_t* player ) :
      priest_heal_t( "holy_word_sanctuary_tick", player, player -> find_spell( 88686 ) )
    {
        dual        = true;
      background  = true;
      direct_tick = true;
    }

    virtual double action_multiplier( const action_state_t* s ) const
    {
      double am = priest_heal_t::action_multiplier( s );

      if ( p() -> buffs.chakra_sanctuary -> up() )
        am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

      return am;
    }
  };

  holy_word_sanctuary_tick_t* tick_spell;

  holy_word_sanctuary_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "holy_word_sanctuary", p, p -> find_spell( 88685 ) ),
    tick_spell( 0 )
  {
    check_spell( p -> spec.revelations );

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

    c *= 1.0 + p() -> buffs.inner_will -> check() * p() -> buffs.inner_will -> data().effectN( 1 ).percent();
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
    priest_spell_t( "holy_word_chastise", p, p -> find_class_spell( "Holy Word: Chastise" ) )
  {
    parse_options( NULL, options_str );

    base_costs[ current_resource() ]  = floor( base_costs[ current_resource() ] );

    // Needs testing
    cooldown -> duration *= 1.0 + p -> set_bonus.tier13_4pc_heal() * -0.2;

    castable_in_shadowform = false;
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
    priest_heal_t( "holy_word_serenity", p, p -> find_spell( 88684 ) )
  {
    check_spell( p -> spec.revelations );

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
    priest_spell_t( "holy_word", p, spell_data_t::nil(), SCHOOL_HOLY ),
    hw_sanctuary( 0 ), hw_chastise( 0 ), hw_serenity( 0 )
  {
    hw_sanctuary = new holy_word_sanctuary_t( p, options_str );
    hw_chastise  = new holy_word_chastise_t ( p, options_str );
    hw_serenity  = new holy_word_serenity_t ( p, options_str );

    castable_in_shadowform = false;
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
    priest_spell_t( "lightwell", p, p -> find_class_spell( "Lightwell" ) ), consume_interval( timespan_t::from_seconds( 10 ) )
  {
    option_t options[] =
    {
      { "consume_interval", OPT_TIMESPAN,     &consume_interval },
      { NULL,               OPT_UNKNOWN, NULL              }
    };
    parse_options( options, options_str );

    harmful = false;

    castable_in_shadowform = false;

    assert( consume_interval > timespan_t::zero() && consume_interval < cooldown -> duration );
  }

  virtual void execute()
  {
    priest_spell_t::execute();

    p() -> pets.lightwell -> get_cooldown( "lightwell_renew" ) -> duration = consume_interval;
    p() -> pets.lightwell -> summon( data().duration() );
  }
};

// Penance Heal Spell =======================================================

struct penance_heal_tick_t : public priest_heal_t
{
  penance_heal_tick_t( priest_t* player ) :
    priest_heal_t( "penance_heal_tick", player, player -> find_spell( 47750 ) )
  {
    background  = true;
    may_crit    = true;
    dual        = true;
    direct_tick = true;

    school = SCHOOL_HOLY;
    stats = player -> get_stats( "penance_heal", this );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    trigger_grace( s -> target );
  }
};

struct penance_heal_t : public priest_heal_t
{
  penance_heal_tick_t* penance_tick;

  penance_heal_t( priest_t* p, const std::string& options_str ) :
    priest_heal_t( "penance_heal", p, p -> find_class_spell( "Penance" ) ), penance_tick( 0 )
  {
    parse_options( NULL, options_str );

    may_crit       = false;
    channeled      = true;
    tick_zero      = true;
    num_ticks      = 2;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

    cooldown = p -> cooldowns.penance;
    cooldown -> duration = data().cooldown() + p -> glyphs.penance -> effectN( 2 ).time_value();

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

    c *= 1.0 + ( p() -> buffs.holy_evangelism -> check() * p() -> buffs.holy_evangelism -> data().effectN( 2 ).percent() );

    c *= 1.0 + p() -> glyphs.penance -> effectN( 1 ).percent();

    return c;
  }
};

// Power Word: Shield Spell =================================================

struct glyph_power_word_shield_t : public priest_heal_t
{
  glyph_power_word_shield_t( priest_t* player ) :
    priest_heal_t( "power_word_shield_glyph", player, player -> find_spell( 55672 ) )
  {
    school          = SCHOOL_HOLY;
    stats -> school = school;

    background = true;
    proc       = true;

    castable_in_shadowform = true;
  }
};

struct power_word_shield_t : public priest_absorb_t
{
  glyph_power_word_shield_t* glyph_pws;
  int ignore_debuff;

  power_word_shield_t( priest_t* p, const std::string& options_str ) :
    priest_absorb_t( "power_word_shield", p, p -> find_class_spell( "Power Word: Shield" ) ), glyph_pws( 0 ), ignore_debuff( 0 )
  {
    option_t options[] =
    {
      { "ignore_debuff", OPT_BOOL,    &ignore_debuff },
      { NULL,            OPT_UNKNOWN, NULL           }
    };
    parse_options( options, options_str );

    // Tooltip is wrong.
    // direct_power_mod = 0.87; // hardcoded into tooltip
    direct_power_mod = 1.8709; // matches in-game actual value

    if ( p -> glyphs.power_word_shield -> ok() )
    {
      glyph_pws = new glyph_power_word_shield_t( p );
      glyph_pws -> target = target;
      add_child( glyph_pws );
    }

    castable_in_shadowform = true;
  }

  virtual double cost() const
  {
    double c = priest_absorb_t::cost();

    return c;
  }

  virtual void impact_s( action_state_t* s )
  {

    s -> target -> buffs.weakened_soul -> trigger();

    // Glyph
    if ( glyph_pws )
    {
      glyph_pws -> base_dd_min  = glyph_pws -> base_dd_max  = p() -> glyphs.power_word_shield -> effectN( 1 ).percent() * s -> result_amount;
      glyph_pws -> target = s -> target;
      glyph_pws -> execute();
    }

    td( s -> target ) -> buffs_power_word_shield -> trigger( 1, s -> result_amount );
    stats -> add_result( s -> result_amount, s -> result_amount, ABSORB, s -> result );
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
    priest_heal_t( "prayer_of_healing", p, p -> find_class_spell( "Prayer of Healing" ) )
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

    priest_heal_t::consume_inner_focus();

    trigger_chakra( p(), p() -> buffs.chakra_sanctuary );
  }

  virtual void impact_s( action_state_t* s )
  {
    priest_heal_t::impact_s( s );

    // Divine Aegis
    if ( s -> result != RESULT_CRIT )
    {
      trigger_divine_aegis( s -> target, s -> result_amount * 0.5 );
    }
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double am = priest_heal_t::action_multiplier( s );

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }

  virtual double composite_crit( const action_state_t* s ) const
  {
    double cc = priest_heal_t::composite_crit( s );

    if ( p() -> buffs.inner_focus -> check() )
      cc += p() -> buffs.inner_focus -> data().effectN( 2 ).percent();

    return cc;
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
    priest_heal_t( "prayer_of_mending", p, p -> find_class_spell( "Prayer of Mending" ) ), single( false )
  {
    option_t options[] =
    {
      { "single", OPT_BOOL,       &single },
      { NULL,     OPT_UNKNOWN, NULL       }
    };
    parse_options( options, options_str );

    direct_power_mod = data().effectN( 1 ).coeff();
    base_dd_min = base_dd_max = data().effectN( 1 ).min( p );

    can_trigger_DA = false;

    aoe = 4;
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double am = priest_heal_t::action_multiplier( s );

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }

  virtual void execute()
  {

    priest_heal_t::execute();

    trigger_chakra( p(), p() -> buffs.chakra_sanctuary );
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double ctm = priest_heal_t::composite_target_multiplier( t );

    if ( p() -> glyphs.prayer_of_mending -> ok() && t == target )
      ctm *= 1.0 + p() -> glyphs.prayer_of_mending -> effectN( 1 ).percent();

    return ctm;
  }
};

// Renew Spell ==============================================================

struct divine_touch_t : public priest_heal_t
{
  divine_touch_t( priest_t* player ) :
    priest_heal_t( "divine_touch", player, player -> find_spell( 63544 ) )
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
    priest_heal_t( "renew", p, p -> find_class_spell( "Renew" ) )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    base_multiplier *= 1.0 + p -> glyphs.renew -> effectN( 1 ).percent();

    num_ticks       += (int) ( p -> glyphs.renew -> effectN( 2 ).time_value() / base_tick_time );
  }

  virtual double action_multiplier( const action_state_t* s ) const
  {
    double am = priest_heal_t::action_multiplier( s );

    if ( p() -> buffs.chakra_sanctuary -> up() )
      am *= 1.0 + p() -> buffs.chakra_sanctuary -> data().effectN( 1 ).percent();

    return am;
  }
};
} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Priest Character Definition
// ==========================================================================

// priest_t::shadow_orb_amount ==============================================

double priest_t::shadow_orb_amount() const
{
  return mastery_spells.shadow_orb_power -> effectN( 1 ).coeff() / 100.0 * composite_mastery();
}

// priest_t::primary_role ===================================================

role_type_e priest_t::primary_role() const
{
  switch ( player_t::primary_role() )
  {
  case ROLE_HEAL:
    return ROLE_HEAL;
  case ROLE_DPS:
  case ROLE_SPELL:
    return ROLE_SPELL;
  default:
    if ( primary_tree() == PRIEST_DISCIPLINE || primary_tree() == PRIEST_HOLY )
      return ROLE_HEAL;
    break;
  }

  return ROLE_SPELL;
}

// priest_t::combat_begin ===================================================

void priest_t::combat_begin()
{
  player_t::combat_begin();

  resources.current[ RESOURCE_SHADOW_ORB ] = ( ( initial_shadow_orbs >= 0 ) && ( initial_shadow_orbs <= resources.base[ RESOURCE_SHADOW_ORB ] ) ) ? initial_shadow_orbs : 0;
}
// priest_t::composite_armor ================================================

double priest_t::composite_armor() const
{
  double a = player_t::composite_armor();

  if ( buffs.inner_fire -> up() )
    a *= 1.0 + buffs.inner_fire -> data().effectN( 1 ).base_value() / 100.0 * ( 1.0 + glyphs.inner_fire -> effectN( 1 ).percent() );

  return floor( a );
}

// priest_t::composite_spell_power ==========================================

double priest_t::composite_spell_power( const school_type_e school ) const
{
  double sp = player_t::composite_spell_power( school );

  if ( buffs.inner_fire -> up() )
    sp *= 1.0 + buffs.inner_fire -> data().effectN( 2 ).percent();

  return sp;
}

// priest_t::composite_spell_hit ============================================

double priest_t::composite_spell_hit() const
{
  double hit = player_t::composite_spell_hit();

  hit += ( ( spirit() - attribute_base[ ATTR_SPIRIT ] ) * spec.spiritual_precision -> effectN( 1 ).percent() ) / rating.spell_hit;

  return hit;
}

// priest_t::composite_player_multiplier ====================================

double priest_t::composite_player_multiplier( const school_type_e school, const action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( spell_data_t::is_school( school, SCHOOL_SHADOW ) )
  {
    m *= 1.0 + buffs.shadowform -> check() * spec.shadowform -> effectN( 2 ).percent();
  }
  if ( spell_data_t::is_school( school, SCHOOL_SHADOWLIGHT ) )
  {
    if ( buffs.chakra_chastise -> up() )
    {
      m *= 1.0 + 0.15;
    }
  }
  if ( talents.twist_of_fate -> ok() && ( a -> target -> health_percentage() < 20.0 ) )
  {
    m *= 1.0 + talents.twist_of_fate -> effect1().percent();
  }

  return m;
}

// priest_t::composite_player_td_multiplier =================================

double priest_t::composite_player_td_multiplier( const school_type_e school, const action_t* a ) const
{
  double player_multiplier = player_t::composite_player_td_multiplier( school, a );

  if ( school == SCHOOL_SHADOW )
  {
    // Shadow TD
    player_multiplier *= 1.0 + shadow_orb_amount();
  }

  return player_multiplier;
}

// priest_t::composite_movement_speed =======================================

double priest_t::composite_movement_speed() const
{
  double speed = player_t::composite_movement_speed();

  if ( buffs.inner_will -> up() )
    speed *= 1.0 + buffs.inner_will -> data().effectN( 2 ).percent() + glyphs.inner_sanctum -> effectN( 2 ).percent();

  return speed;
}

// priest_t::matching_gear_multiplier =======================================

double priest_t::matching_gear_multiplier( const attribute_type_e attr ) const
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
  if ( name == "shadowform"             ) return new shadowform_t            ( this, options_str );
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
  if ( name == "shadowy_apparition"     ) return new shadowy_apparition_t    ( this, options_str );

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

  base_attack_power = 0;

  resources.base[ RESOURCE_SHADOW_ORB ] = 3;

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
  gains.shadow_orb_swp            = get_gain( "Shadow Orbs from Shadow Word: Pain" );
  gains.shadow_orb_mb             = get_gain( "Shadow Orbs from Mind Blast" );
  gains.shadow_orb_mastery_refund = get_gain( "Shadow Orbs refunded from Shadow Orb Mastery" );
  gains.vampiric_touch_health     = get_gain( "Vampiric Touch Health" );
  gains.vampiric_touch_mana       = get_gain( "Vampiric Touch Mana" );
}

// priest_t::init_procs. =====================================================

void priest_t::init_procs()
{
  player_t::init_procs();

  procs.sa_shadow_orb_mastery = get_proc( "shadowy_apparation_shadow_orb_mastery_proc" );
}

// priest_t::init_scaling ===================================================

void priest_t::init_scaling()
{
  player_t::init_scaling();

  // An Atonement Priest might be Health-capped
  scales_with[ STAT_STAMINA ] = glyphs.atonement -> ok();

  // For a Shadow Priest Spirit is the same as Hit Rating so invert it.
  if ( ( spec.spiritual_precision -> ok() ) && ( sim -> scaling -> scale_stat == STAT_SPIRIT ) )
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

  for ( size_t i = 0; i < 4; ++i )
    benefits.mind_spike[ i ] = get_benefit( "mind_spike_" + util_t::to_string( i ) );
}

// priest_t::init_rng =======================================================

void priest_t::init_rng()
{
  player_t::init_rng();

  rngs.sa_shadow_orb_mastery = get_rng( "sa_shadow_orb_mastery" );
}

// priest_t::init_spells
void priest_t::init_spells()
{
  player_t::init_spells();

  // Talents
  talents.void_tendrils               = find_talent_spell( "Void Tendrils" );
  talents.psyfiend                    = find_talent_spell( "Psyfiend" );
  talents.dominate_mind               = find_talent_spell( "Dominate Mind" );
  talents.body_and_soul               = find_talent_spell( "Body and Soul" );
  talents.feathers_from_heaven        = find_talent_spell( "Feathers From Heaven" );
  talents.phantasm                    = find_talent_spell( "Phantasm" );
  talents.from_darkness_comes_light   = find_talent_spell( "From Darkness, Comes Light" );
  // "coming soon"
  talents.archangel                   = find_talent_spell( "Archangel" );
  talents.desperate_prayer            = find_talent_spell( "Desperate Prayer" );
  talents.void_shift                  = find_talent_spell( "Void Shift" );
  talents.angelic_bulwark             = find_talent_spell( "Angelic Bulwark" );
  talents.twist_of_fate               = find_talent_spell( "Twist of Fate" );
  talents.power_infusion              = find_talent_spell( "Power Infusion" );
  talents.divine_insight              = find_talent_spell( "Divine Insight" );
  talents.cascade                     = find_talent_spell( "Cascade" );
  talents.divine_star                 = find_talent_spell( "Divine Star" );
  talents.halo                        = find_talent_spell( "Halo" );

  // Passive Spells

  // General Spells

  // Discipline
  spec.meditation_disc                = find_specialization_spell( "Meditation", "meditation_disc", PRIEST_DISCIPLINE );
  spec.divine_aegis                   = find_specialization_spell( "Divine Aegis" );
  spec.grace                          = find_specialization_spell( "Grace" );
  spec.evangelism                     = find_specialization_spell( "Evangelism" );
  spec.train_of_thought               = find_specialization_spell( "Train of Thought" );
  spec.divine_fury                    = find_specialization_spell( "Divine Fury" );

  // Holy
  spec.meditation_holy                = find_specialization_spell( "Meditation", "meditation_holy", PRIEST_HOLY );
  spec.revelations                    = find_specialization_spell( "Revelations" );
  spec.chakra_chastise                = find_class_spell( "Chakra: Chastise" );
  spec.chakra_sanctuary               = find_class_spell( "Chakra: Sanctuary" );
  spec.chakra_serenity                = find_class_spell( "Chakra: Serenity" );

  // Shadow
  spec.spiritual_precision            = find_specialization_spell( "Spiritual Precision" );
  spec.shadowform                     = find_class_spell( "Shadowform" );
  spec.shadowy_apparition             = find_class_spell( "Shadowy Apparition" );

  // Mastery Spells
  mastery_spells.shield_discipline    = find_mastery_spell( PRIEST_DISCIPLINE );
  mastery_spells.echo_of_light        = find_mastery_spell( PRIEST_HOLY );
  mastery_spells.shadow_orb_power     = find_mastery_spell( PRIEST_SHADOW );

  // Glyphs
  glyphs.circle_of_healing            = find_glyph_spell( "Glyph of Circle of Healing" );
  glyphs.dispersion                   = find_glyph_spell( "Glyph of Dispersion" );
  glyphs.holy_nova                    = find_glyph_spell( "Glyph of Holy Nova" );
  glyphs.inner_fire                   = find_glyph_spell( "Glyph of Inner Fire" );
  glyphs.lightwell                    = find_glyph_spell( "Glyph of Lightwell" );
  glyphs.penance                      = find_glyph_spell( "Glyph of Penance" );
  glyphs.power_word_shield            = find_glyph_spell( "Glyph of Power Word: Shield" );
  glyphs.prayer_of_mending            = find_glyph_spell( "Glyph of Prayer of Mending" );
  glyphs.renew                        = find_glyph_spell( "Glyph of Renew" );
  glyphs.smite                        = find_glyph_spell( "Glyph of Smite" );
  glyphs.holy_fire                    = find_glyph_spell( "Glyph of Holy Fire" );
  glyphs.atonement                    = find_glyph_spell( "Atonement" );
  glyphs.mind_spike                   = find_glyph_spell( "Glyph of Mind Spike" );
  glyphs.strength_of_soul             = find_glyph_spell( "Glyph of Strength of Soul" );
  glyphs.inner_sanctum                = find_glyph_spell( "Glyph of Inner Sanctum" );
  glyphs.mind_flay                    = find_glyph_spell( "Glyph of Mind Flay" );
  glyphs.mind_blast                   = find_glyph_spell( "Glyph of Mind Blast" );
  glyphs.vampiric_touch               = find_glyph_spell( "Glyph of Vampiric Touch" );
  glyphs.vampiric_embrace             = find_glyph_spell( "Glyph of Vampiric Embrace" );
  glyphs.shadowy_apparition           = find_glyph_spell( "Glyph of Shadowy Apparition" );
  glyphs.fortitude                    = find_glyph_spell( "Glyph of Fortitude" );

  if ( mastery_spells.echo_of_light -> ok() )
    spells.echo_of_light = new echo_of_light_t( this );
  else
    spells.echo_of_light = NULL;

  if ( spec.shadowy_apparition -> ok() )
  {
    priest_spell_t::add_more_shadowy_apparitions( this, 15 );
  }

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
  buffs.holy_evangelism                  = buff_creator_t( this, 81661, "holy_evangelism" )
                                           .chance( spec.evangelism -> ok() )
                                           .activated( false );
  buffs.dark_archangel                   = buff_creator_t( this, "dark_archangel", find_spell( 87153 ) )
                                           .chance( talents.archangel -> ok() )
                                           .default_value( find_spell( 87153 ) -> effect1().percent() );
  buffs.holy_archangel                   = buff_creator_t( this, "holy_archangel", find_spell( 81700 ) )
                                           .chance( talents.archangel -> ok() )
                                           .default_value( find_spell( 81700 ) -> effect1().percent() );
  buffs.inner_fire                       = buff_creator_t( this, "inner_fire", find_class_spell( "Inner Fire" ) );
  buffs.inner_focus                      = buff_creator_t( this, "inner_focus", find_class_spell( "Inner Focus" ) )
                                                           .cd( timespan_t::zero() );
  buffs.inner_will                       = buff_creator_t( this, "inner_will", find_class_spell( "Inner Will" ) );
  // Holy
  buffs.chakra_pre                       = buff_creator_t( this, "chakra_pre", find_spell( 14751 ) );
  buffs.chakra_chastise                  = buff_creator_t( this, "chakra_chastise", find_spell( 81209 ) );
  buffs.chakra_sanctuary                 = buff_creator_t( this, "chakra_sanctuary", find_spell( 81206 ) );
  buffs.chakra_serenity                  = buff_creator_t( this, "chakra_serenity", find_spell( 81208 ) );
  buffs.serenity                         = buff_creator_t( this, "serenity", find_spell( 88684 ) )
                                                           .cd( timespan_t::zero() )
                                                           .activated( false );

  // Shadow
  buffs.shadowform                       = buff_creator_t( this, "shadowform", find_class_spell( "Shadowform" ) );
  buffs.vampiric_embrace                 = buff_creator_t( this, "vampiric_embrace", find_class_spell( "Vampiric Embrace" ) )
                                           .duration( find_class_spell( "Vampiric Embrace" ) -> duration() + glyphs.vampiric_embrace -> effectN( 1 ).time_value() );
  buffs.glyph_mind_spike                 = buff_creator_t( this, "glyph_mind_spike", glyphs.mind_spike ).max_stack( 2 ).duration( timespan_t::from_seconds( 6.0 ) );

  buffs.shadow_word_death_reset_cooldown = buff_creator_t( this, "shadow_word_death_reset_cooldown").
                                                           max_stack( 1 ).duration( timespan_t::from_seconds( 6.0 ) );
  buffs.mind_spike                       = buff_creator_t( this, "mind_spike" ).
                                                           max_stack( 3 ).duration( timespan_t::from_seconds( 12.0 ) );
  buffs.shadowfiend                      = buff_creator_t( this, "shadowfiend" ).
                                                           max_stack( 1 ).duration( timespan_t::from_seconds( 15.0 ) ); // Pet Tracking Buff
  buffs.shadow_of_death                  = buff_creator_t( this, "shadow_of_death", talents.divine_insight )
                                                           .chance( talents.divine_insight -> effectN( 1 ).percent() )
                                                           .max_stack( 1 )
                                                           .duration( timespan_t::from_seconds( 10.0 ) );
  buffs.surge_of_darkness                = buff_creator_t( this, "surge_of_darkness", talents.from_darkness_comes_light )
                                                           .duration( find_spell( 114257 ) -> duration() );


  // Set Bonus
}

// priest_t::init_actions ===================================================

void priest_t::init_actions()
{
  std::string& list_default = get_action_priority_list( "default" ) -> action_list_str;

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

    if ( find_class_spell( "Power Word: Fortitude" ) -> ok() )
      buffer += "/fortitude,if=!aura.stamina.up";

    if ( find_class_spell( "Inner Fire" ) -> ok() )
      buffer += "/inner_fire";

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
    case PRIEST_SHADOW:

      if ( find_class_spell( "Shadowform" ) -> ok() )
        buffer += "/shadowform";

      if ( level > 80 )
      {
        buffer += "/volcanic_potion,if=!in_combat";
        buffer += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      }

      /*
      if ( find_class_spell( "Vampiric Embrace" ) -> ok() )
        buffer += "/vampiric_embrace";
      */

      if ( find_talent_spell( "Archangel" ) -> ok() )
        buffer += "/archangel";

      if ( find_class_spell( "Mind Blast" ) -> ok() && find_class_spell( "Mind Spike" ) -> ok() )
        buffer += "/mind_blast,if=buff.mind_spike.react>=1";


      if ( find_class_spell( "Shadow Word: Death" ) -> ok() )
      {
        buffer += "/shadow_word_death,if=target.health.pct<=20";
        if ( find_talent_spell( "Divine Insight" ) -> ok() )
          buffer += "|buff.shadow_of_death.react";
      }

      if ( find_class_spell( "Mind Blast" ) -> ok() )
        buffer += "/mind_blast";

      if ( find_class_spell( "Mind Spike" ) -> ok() && find_talent_spell( "From Darkness, Comes Light" ) -> ok() )
        buffer += "/mind_spike,if=buff.surge_of_darkness.react";

      buffer += init_use_racial_actions();

      if ( find_talent_spell( "Power Infusion" ) -> ok() )
        buffer += "/power_infusion";

      if ( find_class_spell( "Vampiric Touch" ) -> ok() )
        buffer += "/vampiric_touch,if=(!ticking|dot.vampiric_touch.remains<cast_time+2.5)&miss_react";

      if ( find_class_spell( "Shadow Word: Pain" ) -> ok() )
        buffer += "/shadow_word_pain,if=(!ticking|dot.shadow_word_pain.remains<gcd+0.5)&miss_react";

      if ( find_class_spell( "Shadowfiend" ) -> ok() )
        buffer += "/shadow_fiend";

      if ( find_class_spell( "Shadowy Apparition" ) -> ok() )
        buffer += "/shadowy_apparition,if=shadow_orb=3";

      if ( find_class_spell( "Mind Flay" ) -> ok() )
        buffer += "/mind_flay";

      if ( find_class_spell( "Shadow Word: Death" ) -> ok() )
      {
        buffer += "/shadow_word_death,moving=1";
      }

      if ( find_class_spell( "Shadow Word: Pain" ) -> ok() )
        buffer += "/shadow_word_pain,moving=1";

      if ( find_class_spell( "Dispersion" ) -> ok() )
        buffer += "/dispersion";
      break;
      // SHADOW END =========================================================


      // DISCIPLINE =========================================================
    case PRIEST_DISCIPLINE:

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
    case PRIEST_HOLY:
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
  constants.meditation_value                = spec.meditation_disc -> ok() ?
                                              spec.meditation_disc -> effectN( 1 ).base_value() :
                                              spec.meditation_holy -> effectN( 1 ).base_value();

  // Shadow
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

  if ( spec.shadowy_apparition -> ok() )
  {
    while ( spells.apparitions_active.size() )
    {
      spell_t* s = spells.apparitions_active.front();

      spells.apparitions_active.pop_front();

      spells.apparitions_free.push( s );
    }
  }

  spells.echo_of_light_merged = false;

  init_party();
}

// priest_t::fixup_atonement_stats  =========================================

void priest_t::fixup_atonement_stats( const std::string& trigger_spell_name,
                                      const std::string& atonement_spell_name )
{
  if ( stats_t* trigger = find_stats( trigger_spell_name ) )
  {
    if ( stats_t* atonement = get_stats( atonement_spell_name ) )
    {
      // Copy stats from the trigger spell to the atonement spell
      // to get proper HPR and HPET reports.
      atonement -> resource_gain.merge( trigger->resource_gain );
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

double priest_t::target_mitigation( double        amount,
                                    school_type_e school,
                                    dmg_type_e    dt,
                                    result_type_e result,
                                    action_t*     action )
{
  amount = player_t::target_mitigation( amount, school, dt, result, action );

  if ( buffs.shadowform -> check() )
  { amount *= 1.0 + buffs.shadowform -> data().effectN( 3 ).percent(); }

  if ( buffs.inner_fire -> check() && glyphs.inner_sanctum -> ok() && ( spell_data_t::get_school_mask( school ) & SCHOOL_MAGIC_MASK ) )
  { amount *= 1.0 - glyphs.inner_sanctum -> effectN( 1 ).percent(); }

  return amount;
}

// priest_t::create_options =================================================

void priest_t::create_options()
{
  player_t::create_options();

  option_t priest_options[] =
  {
    { "atonement_target",        OPT_STRING,            &( atonement_target_str     ) },
    { "double_dot",              OPT_DEPRECATED, ( void* ) "action_list=double_dot"   },
    { "initial_shadow_orbs",     OPT_INT,               &( initial_shadow_orbs      ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, priest_options );
}

// priest_t::create_profile =================================================

bool priest_t::create_profile( std::string& profile_str, save_type_e type, bool save_html )
{
  player_t::create_profile( profile_str, type, save_html );

  if ( type == SAVE_ALL )
  {
    if ( ! atonement_target_str.empty() )
      profile_str += "atonement_target=" + atonement_target_str + "\n";

    if ( initial_shadow_orbs != 0 )
    {
      profile_str += "initial_shadow_orbs=";
      profile_str += util_t::to_string( initial_shadow_orbs );
      profile_str += "\n";
    }
  }

  return true;
}

// priest_t::copy_from ======================================================

void priest_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  priest_t* source_p = debug_cast<priest_t*>( source );

  atonement_target_str = source_p -> atonement_target_str;
}

// priest_t::decode_set =====================================================

int priest_t::decode_set( const item_t& item ) const
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

player_t* player_t::create_priest( sim_t* sim, const std::string& name, race_type_e r )
{
  SC_CREATE_PRIEST( sim, name, r );
}

// player_t::priest_init ====================================================

void player_t::priest_init( sim_t* sim )
{
  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[ i ];
    p -> buffs.guardian_spirit  = buff_creator_t( p, "guardian_spirit", p -> find_spell( 47788 ) ); // Let the ability handle the CD
    p -> buffs.pain_supression  = buff_creator_t( p, "pain_supression", p -> find_spell( 33206 ) ); // Let the ability handle the CD
    p -> buffs.power_infusion   = buff_creator_t( p, "power_infusion", p  -> find_spell( 10060 ) ).max_stack( 1 ).duration( timespan_t::from_seconds( 15.0 ) );
    p -> buffs.weakened_soul    = buff_creator_t( p, "weakened_soul", p -> find_spell( 6788 ) );
  }
}

// player_t::priest_combat_begin ============================================

void player_t::priest_combat_begin( sim_t* )
{}
