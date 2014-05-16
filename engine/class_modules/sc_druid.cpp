// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Druid
// ==========================================================================

 /* WoD -- TODO:
    = General =
	  Dream of Cenarius
	    Verify Guardian DoC works
	  Glyphs
	    Travel
	  Tranquility
    Dash
    Stampeding Roar

    = Feral =
	  Level 100 Talents
	    Lunar Inspiration -- Mostly implemented, cannot work until the sim recognizes the talent correctly.
	  Combo Points as a resource

    = Balance =
    Perks
    Incarnation

    = Guardian =
    Perks
    Soul of the Forest

    = Restoration =
    Perks

*/

// Forward declarations
struct druid_t;
struct natures_vigil_proc_t;
struct ursocs_vigor_t;
struct cenarion_ward_hot_t;
struct leader_of_the_pack_t;
struct yseras_gift_t;
namespace buffs {
struct heart_of_the_wild_buff_t;
}

struct combo_points_t
{
public:
  static const int max_combo_points = 5;

  combo_points_t( druid_t& source, player_t& target );

  void add( int num, const std::string* source_name = nullptr );
  int consume( const std::string* source_name = nullptr );
  void clear() { count = 0; }
  int get() const { return count; }

  expr_t* count_expr() const
  { return make_ref_expr( "combo_points", count ); }
private:
  druid_t& source;
  player_t& target;

  proc_t* proc;
  proc_t* wasted;

  int count;
};

struct druid_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* lacerate;
    dot_t* lifebloom;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* rip;
    dot_t* sunfire;
    dot_t* wild_growth;
  } dots;

  struct buffs_t
  {
    buff_t* lifebloom;
  } buffs;

  int lacerate_stack;
  combo_points_t combo_points;

  druid_td_t( player_t& target, druid_t& source );

  bool hot_ticking()
  {
    return dots.regrowth      -> ticking ||
           dots.rejuvenation  -> ticking ||
           dots.lifebloom     -> ticking ||
           dots.wild_growth   -> ticking;
  }

  void reset()
  {
    lacerate_stack = 0;
    combo_points.clear();
  }
};

struct druid_t : public player_t
{
public:

  // Active
  action_t* t16_2pc_starfall_bolt;
  action_t* t16_2pc_sun_bolt;
  
  struct active_actions_t
  {
    natures_vigil_proc_t* natures_vigil;
    ursocs_vigor_t*       ursocs_vigor;
    cenarion_ward_hot_t*  cenarion_ward_hot;
    leader_of_the_pack_t* leader_of_the_pack;
    yseras_gift_t*        yseras_gift;
  } active;

  // Pets
  pet_t* pet_feral_spirit[ 2 ];
  pet_t* pet_mirror_images[ 3 ];
  pet_t* pet_force_of_nature[ 3 ];

  // Auto-attacks
  weapon_t cat_weapon;
  weapon_t bear_weapon;
  melee_attack_t* cat_melee_attack;
  melee_attack_t* bear_melee_attack;

  weapon_t caster_form_weapon;
  double equipped_weapon_dps;

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* barkskin;
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* dash;
    buff_t* cenarion_ward;
    buff_t* dream_of_cenarius;
    buff_t* frenzied_regeneration;
    buff_t* natures_swiftness;
    buff_t* natures_vigil;
    buff_t* omen_of_clarity;
    buff_t* prowl;
    buff_t* stampeding_shout;

    // Balance
    buff_t* astral_insight;
    buff_t* celestial_alignment;
    buff_t* chosen_of_elune;
    buff_t* eclipse_lunar;
    buff_t* eclipse_solar;
    buff_t* lunar_shower;
    buff_t* moonkin_form;
    buff_t* natures_grace;
    buff_t* shooting_stars;
    buff_t* starfall;

    // Feral
    buff_t* berserk;
    buff_t* king_of_the_jungle;
    buff_t* predatory_swiftness;
    buff_t* savage_roar;
    buff_t* tigers_fury;
    buff_t* tier15_4pc_melee;
    buff_t* feral_fury;
    buff_t* feral_rage;

    // Guardian
    buff_t* bladed_armor;
    buff_t* lacerate;
    buff_t* might_of_ursoc;
    buff_t* savage_defense;
    buff_t* son_of_ursoc;
    buff_t* survival_instincts;
    buff_t* tier15_2pc_tank;
    buff_t* tooth_and_claw;
    absorb_buff_t* tooth_and_claw_absorb;

    // Restoration
    buff_t* soul_of_the_forest;

    // NYI / Needs checking
    buff_t* harmony;
    buff_t* wild_mushroom;
    buff_t* tree_of_life;
    buffs::heart_of_the_wild_buff_t* heart_of_the_wild;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* natures_swiftness;
    cooldown_t* mangle;
    cooldown_t* pvp_4pc_melee;
    cooldown_t* starfall;
    cooldown_t* starsurge;
    cooldown_t* swiftmend;
  } cooldown;

  // Gains
  struct gains_t
  {
    // DONE
    gain_t* bear_form;
    gain_t* eclipse;
    gain_t* energy_refund;
    gain_t* frenzied_regeneration;
    gain_t* lotp_health;
    gain_t* lotp_mana;
    gain_t* omen_of_clarity;
    gain_t* primal_fury;
    gain_t* soul_of_the_forest;
    gain_t* tigers_fury;

    // NYI / Needs checking
    gain_t* bear_melee;
    gain_t* glyph_ferocious_bite;
    gain_t* mangle;
  } gain;

  // Perks
  struct
  {
    // Multiple Specs
    const spell_data_t* improved_healing_touch;
    
    // Feral
    const spell_data_t* enhanced_berserk;
    const spell_data_t* enhanced_cat_form;
    const spell_data_t* enhanced_prowl;
    const spell_data_t* enhanced_rejuvenation;
    const spell_data_t* enhanced_tigers_fury;
    const spell_data_t* improved_ferocious_bite;
    const spell_data_t* improved_pounce;
    const spell_data_t* improved_shred;

    // Balance
    
    // Guardian

    // Restoration
  } perk;

  // Glyphs
  struct glyphs_t
  {
    // DONE
    const spell_data_t* blooming;
    const spell_data_t* cat_form;
    const spell_data_t* ferocious_bite;
    const spell_data_t* frenzied_regeneration;
    const spell_data_t* healing_touch;
    const spell_data_t* maim;
    const spell_data_t* maul;
    const spell_data_t* ninth_life;
    //const spell_data_t* savagery; Missing from spell data at the moment. Don't think it's actually removed.
    const spell_data_t* skull_bash;
    const spell_data_t* ursols_defense;
    const spell_data_t* wild_growth;

    // NYI / Needs checking
    const spell_data_t* lifebloom;
    const spell_data_t* might_of_ursoc;
    const spell_data_t* regrowth;
    const spell_data_t* rejuvenation;
  } glyph;

  // Masteries
  struct masteries_t
  {
    // Done
    const spell_data_t* total_eclipse;
    const spell_data_t* razor_claws;

    // NYI / TODO!
    const spell_data_t* natures_guardian; // NYI
    const spell_data_t* harmony;
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* primal_fury;
    proc_t* wrong_eclipse_wrath;
    proc_t* wrong_eclipse_starfire;
    proc_t* unaligned_eclipse_gain;
    proc_t* combo_points;
    proc_t* combo_points_wasted;
    proc_t* shooting_stars_wasted;
    proc_t* tier15_2pc_melee;
    proc_t* tooth_and_claw;
  } proc;

  // Class Specializations
  struct specializations_t
  {
    // DONE
    // Generic
    const spell_data_t* leather_specialization;
    const spell_data_t* omen_of_clarity; // Feral and Resto have this
    const spell_data_t* primal_fury;
    const spell_data_t* killer_instinct;
    const spell_data_t* nurturing_instinct;

    // Feral / Guardian
    const spell_data_t* leader_of_the_pack;
    const spell_data_t* predatory_swiftness;
    const spell_data_t* critical_strikes;

    // Balance
    const spell_data_t* balance_of_power;
    const spell_data_t* celestial_focus;
    const spell_data_t* eclipse;
    const spell_data_t* euphoria;
    const spell_data_t* lunar_shower;
    const spell_data_t* owlkin_frenzy;
    const spell_data_t* shooting_stars;

    // Guardian
    const spell_data_t* thick_hide;
    const spell_data_t* tooth_and_claw;

    // Restoration
    const spell_data_t* living_seed;
    const spell_data_t* meditation;
    const spell_data_t* natural_insight;
    const spell_data_t* natures_focus;
  } spec;


  struct spells_t
  {
    const spell_data_t* berserk_bear; // Berserk bear mangler
    const spell_data_t* berserk_cat; // Berserk cat resource cost reducer
    const spell_data_t* bear_form; // Bear form bonuses
    const spell_data_t* combo_point; // Combo point spell
    const spell_data_t* eclipse; // Eclipse mana gain
    const spell_data_t* heart_of_the_wild; // HotW INT/AGI bonus
    const spell_data_t* leader_of_the_pack; // LotP aura
    const spell_data_t* mangle; // Mangle cooldown reset
    const spell_data_t* moonkin_form; // Moonkin form bonuses
    const spell_data_t* primal_fury; // Primal fury gain
    const spell_data_t* regrowth; // Old GoRegrowth
    const spell_data_t* survival_instincts; // Survival instincts aura
    const spell_data_t* swiftmend; // Swiftmend AOE heal trigger
    const spell_data_t* swipe; // Bleed damage multiplier for Shred etc
  } spell;

  // Eclipse Management
  int eclipse_bar_value; // Tracking the current value of the eclipse bar
  int eclipse_bar_direction; // Tracking the current direction of the eclipse bar

  int initial_eclipse;
  int default_initial_eclipse;

  // Talents
  struct talents_t
  {
    const spell_data_t* feline_swiftness;

    const spell_data_t* yseras_gift;
    const spell_data_t* renewal;
    const spell_data_t* cenarion_ward;

    const spell_data_t* soul_of_the_forest;
    const spell_data_t* incarnation;
    const spell_data_t* force_of_nature;

    const spell_data_t* heart_of_the_wild;
    const spell_data_t* dream_of_cenarius;
    const spell_data_t* natures_vigil;

    // Touch of Elune (Level 100 Slot 1)
    const spell_data_t* sunfall;
    const spell_data_t* lunar_inspiration;
    const spell_data_t* guardian_of_elune;
    const spell_data_t* moment_of_clarity;

    // Will of Malfurion (Level 100 Slot 2)
    const spell_data_t* insect_swarm;
    const spell_data_t* bloody_thrash;
    const spell_data_t* pulverize;
    const spell_data_t* germination;

    // Might of Malorne (Level 100 Slot 3)
    const spell_data_t* equinox;
    const spell_data_t* savagery;
    const spell_data_t* bristling_fur;
    const spell_data_t* rampant_growth;

    // MoP TODO: Fix/Implement
    const spell_data_t* displacer_beast;
    const spell_data_t* wild_charge;

    const spell_data_t* faerie_swarm;
    const spell_data_t* mass_entanglement;
    const spell_data_t* typhoon;

    const spell_data_t* disorienting_roar;
    const spell_data_t* ursols_vortex;
    const spell_data_t* mighty_bash;
  } talent;

  bool inflight_starsurge;

  druid_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DRUID, name, r ),
    t16_2pc_starfall_bolt( nullptr ),
    t16_2pc_sun_bolt( nullptr ),
    active( active_actions_t() ),
    caster_form_weapon(),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    glyph( glyphs_t() ),
    mastery( masteries_t() ),
    proc( procs_t() ),
    spec( specializations_t() ),
    spell( spells_t() ),
    talent( talents_t() ),
    inflight_starsurge( false )
  {
    t16_2pc_starfall_bolt = 0;
    t16_2pc_sun_bolt      = 0;

    eclipse_bar_value     = 0;
    eclipse_bar_direction = 0;
    
    initial_eclipse = 65535;
    default_initial_eclipse = -75;

    for (size_t i = 0; i < sizeof_array(pet_force_of_nature); i++)
    {
        pet_force_of_nature[i] = nullptr;
    }

    cooldown.natures_swiftness   = get_cooldown( "natures_swiftness"   );
    cooldown.mangle              = get_cooldown( "mangle"         );
    cooldown.pvp_4pc_melee       = get_cooldown( "pvp_4pc_melee"       );
    cooldown.pvp_4pc_melee -> duration = timespan_t::from_seconds( 30.0 );
    cooldown.starfall            = get_cooldown( "starfall"            );
    cooldown.starsurge           = get_cooldown( "starsurge"           );
    cooldown.swiftmend           = get_cooldown( "swiftmend"           );


    cat_melee_attack = 0;
    bear_melee_attack = 0;

    equipped_weapon_dps = 0;

    base.distance = ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) ? 3 : 30;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      create_buffs();
  virtual void      init_scaling();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      invalidate_cache( cache_e );
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      regen( timespan_t periodicity );
  virtual timespan_t available() const;
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_melee_attack_power() const;
  virtual double    composite_melee_crit() const;
  virtual double    composite_melee_hit() const;
  virtual double    composite_melee_expertise( weapon_t* ) const;
  virtual double    temporary_movement_modifier() const;
  virtual double    passive_movement_modifier() const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    composite_player_td_multiplier( school_e,  const action_t* ) const;
  virtual double    composite_player_heal_multiplier( school_e school ) const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_spell_hit() const;
  virtual double    composite_spell_power( school_e school ) const;
  virtual double    composite_attribute( attribute_e attr ) const;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual double    composite_parry() const { return 0; }
  virtual double    composite_block() const { return 0; }
  virtual double    composite_crit_avoidance() const;
  virtual double    composite_dodge() const;
  virtual double    composite_rating_multiplier( rating_e rating ) const;
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual set_e     decode_set( const item_t& ) const;
  virtual resource_e primary_resource() const;
  virtual role_e    primary_role() const;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual void      assess_damage( school_e school, dmg_e, action_state_t* );
  virtual void      assess_heal( school_e, dmg_e, action_state_t* );
  virtual void      create_options();
  virtual bool      create_profile( std::string& profile_str, save_e type = SAVE_ALL, bool save_html = false );

  void              apl_precombat();
  void              apl_default();
  void              apl_feral();
  void              apl_balance();
  void              apl_guardian();
  void              apl_restoration();
  virtual void      init_action_list();

  target_specific_t<druid_td_t*> target_data;

  virtual druid_td_t* get_target_data( player_t* target ) const
  {
    assert( target );
    druid_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new druid_td_t( *target, const_cast<druid_t&>(*this) );
    }
    return td;
  }

  void trigger_shooting_stars( result_e );
  void trigger_soul_of_the_forest();

  void init_beast_weapon( weapon_t& w, double swing_time )
  {
    w = main_hand_weapon;
    double mod = swing_time /  w.swing_time.total_seconds();
    w.type = WEAPON_BEAST;
    w.school = SCHOOL_PHYSICAL;
    w.min_dmg *= mod;
    w.max_dmg *= mod;
    w.damage *= mod;
    w.swing_time = timespan_t::from_seconds( swing_time );
  }
};

// Nature's Vigil Proc ======================================================

struct natures_vigil_proc_t : public spell_t
{
  struct natures_vigil_heal_t : public heal_t
  {
    double heal_coeff;
    double dmg_coeff;

    natures_vigil_heal_t( druid_t* p ) :
      heal_t( "natures_vigil_heal", p, p -> find_spell( 124988 ) ),
      heal_coeff( 0.0 ), dmg_coeff( 0.0 )
    {
      background = proc = dual = true;
      may_crit = may_miss      = false;
      trigger_gcd              = timespan_t::zero();
      heal_coeff               = p -> talent.natures_vigil -> effectN( 3 ).percent();
      dmg_coeff                = p -> talent.natures_vigil -> effectN( 4 ).percent();
    }

    void trigger( double amount, bool harmful )
    {
      double coeff = harmful ? dmg_coeff : heal_coeff;

      // set the heal size to be fixed based on the result of the action
      base_dd_min = base_dd_max = amount * coeff;

      target = find_lowest_target();

      if ( target )
        execute();
    }

  private:
    player_t* find_lowest_target()
    {
      // Ignoring range for the time being
      double lowest_health_pct_found = 100.1;
      player_t* lowest_player_found = nullptr;

      for ( size_t i = 0, size = sim -> player_no_pet_list.size(); i < size; i++ )
      {
        player_t* p = sim -> player_no_pet_list[ i ];

        // check their health against the current lowest
        if ( p -> health_percentage() < lowest_health_pct_found )
        {
          // if this player is lower, make them the current lowest
          lowest_health_pct_found = p -> health_percentage();
          lowest_player_found = p;
        }
      }
      return lowest_player_found;
    }
  };

  struct natures_vigil_damage_t : public spell_t
  {
    natures_vigil_damage_t( druid_t* p ) :
      spell_t( "natures_vigil_damage", p, p -> find_spell( 124991 ) )
    {
      background = proc = dual = true;
      may_crit = may_miss      = false;
      trigger_gcd              = timespan_t::zero();
      base_multiplier          = p -> talent.natures_vigil -> effectN( 3 ).percent();
    }

    void trigger( double amount )
    {
      // set the heal size to be fixed based on the result of the action
      base_dd_min = base_dd_max = amount;

      target = pick_random_target();

      if ( target )
        execute();
    }

  private:
    player_t* pick_random_target()
    {
      // Targeting is probably done by range, but since the sim doesn't really have
      // have a concept of that we'll just pick a target at random.

      unsigned t = static_cast<unsigned>( rng().range( 0, as<double>( sim -> target_list.size() ) ) );
      if ( t >= sim-> target_list.size() ) --t; // dsfmt range should not give a value actually equal to max, but be paranoid
      return sim-> target_list[ t ];
    }
  };

  natures_vigil_damage_t* damage;
  natures_vigil_heal_t*   healing;

  natures_vigil_proc_t( druid_t* p ) :
    spell_t( "natures_vigil", p, spell_data_t::nil() )
  {
    background = proc = true;
    trigger_gcd       = timespan_t::zero();
    may_crit = may_miss = false;

    damage  = new natures_vigil_damage_t( p );
    healing = new natures_vigil_heal_t( p );
  }

  void trigger( const action_t& action, bool harmful = true )
  {
    if ( action.execute_state -> result_amount <= 0 )
      return;
    if ( action.aoe != 0 )
      return;
    if ( ! action.special )
      return;

    if ( ! action.harmful || harmful )
      damage -> trigger( action.execute_state -> result_amount );
    healing -> trigger( action.execute_state -> result_amount, action.harmful );
  }
};

// Ursoc's Vigor ( tier16_4pc_tank ) =====================================

struct ursocs_vigor_t : public heal_t
{
  double ap_coefficient;
  int ticks_remain;

  ursocs_vigor_t( druid_t* p ) :
    heal_t( "ursocs_vigor", p, p -> find_spell( 144888 ) )
  {
    background = true;

    hasted_ticks = false;
    may_crit = tick_may_crit = false;

    base_td = 0;
    
    base_tick_time = timespan_t::from_seconds( 1.0 );
    num_ticks = 8;

    // store healing multiplier
    ap_coefficient = p -> find_spell( 144887 ) -> effectN( 1 ).percent();
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  void trigger_hot( double rage_consumed = 60.0 )
  {
    // Adjust the current healing remaining to be spread over the total duration of the hot.
    base_td *= (double) ticks_remain / (double) num_ticks;
    // Add the new amount of healing
    base_td += p() -> composite_melee_attack_power() * p() -> composite_attack_power_multiplier()
               * ap_coefficient
               * rage_consumed / 60
               / num_ticks;
    // Set the number of ticks remaining back to full
    ticks_remain = num_ticks;

    execute();
  }

  virtual void tick( dot_t *d )
  {
    heal_t::tick( d );

    ticks_remain -= 1;
  }
};

// Cenarion Ward HoT ========================================================

struct cenarion_ward_hot_t : public heal_t
{
  cenarion_ward_hot_t( druid_t* p ) :
    heal_t( "cenarion_ward_hot", p, p -> find_spell( 102352 ) )
  {
    background = true;
    harmful = false;
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual void execute()
  {
    heal_t::execute();

    p() -> buff.cenarion_ward -> expire();
  }

  // FIXME: AP is snapshotted with the trigger buff is applied NOT when the hot is applied.
};

// Leader of the Pack =======================================================

struct leader_of_the_pack_t : public heal_t
{
  leader_of_the_pack_t( druid_t* p ) :
    heal_t( "leader_of_the_pack", p, p -> find_spell( 17007 ) )
  {
    may_crit = false;
    background = true;
    proc = true;

    cooldown -> duration = timespan_t::from_seconds( 6.0 );
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual double base_da_min( const action_state_t* ) const
  {
    return p() -> resources.max[ RESOURCE_HEALTH ] *
           p() -> spell.leader_of_the_pack -> effectN( 2 ).percent();
  }

  virtual double base_da_max( const action_state_t* ) const
  {
    return p() -> resources.max[ RESOURCE_HEALTH ] *
           p() -> spell.leader_of_the_pack -> effectN( 2 ).percent();
  }

  virtual void execute()
  {
    heal_t::execute();

    // Trigger mana gain
    p() -> resource_gain( RESOURCE_MANA,
                        p() -> resources.max[ RESOURCE_MANA ] *
                        data().effectN( 1 ).percent(),
                        p() -> gain.lotp_mana );
  }
};

// Ysera's Gift ==============================================================

struct yseras_gift_t : public heal_t
{
  yseras_gift_t( druid_t* p ) :
    heal_t( "yseras_gift", p, p -> find_talent_spell( "Ysera's Gift" ) )
  {
    num_ticks      = 1;
    base_tick_time = data().effectN( 1 ).period();
    hasted_ticks   = false;
    tick_may_crit  = false;
    harmful        = false;
    background     = true;
    target         = p;
  }

  virtual void tick( dot_t* d )
  {
    druid_t* p = static_cast<druid_t*>( player );

    d -> current_tick = 0; // ticks indefinitely

    base_td = p -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();

    heal_t::tick( d );
  }
};

// Tooth and Claw Absorb ===========================================================

struct tooth_and_claw_t : public absorb_t
{
  tooth_and_claw_t( druid_t* p ) :
    absorb_t( "tooth_and_claw", p, p -> find_specialization_spell( "Tooth and Claw" ) )
  {
    harmful = false;
    special = false;
    background = true;
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual void execute()
  {
    absorb_t::execute();

    // max(2.2*(AP - 2*Agi), 2.5*Sta)*0.4
    double ap         = player -> composite_melee_attack_power() * player -> composite_attack_power_multiplier();
    double agility    = player -> composite_attribute( ATTR_AGILITY ) * player -> composite_attribute_multiplier( ATTR_AGILITY );
    double stamina    = player -> composite_attribute( ATTR_STAMINA ) * player -> composite_attribute_multiplier( ATTR_STAMINA );
    double absorb_amount =  floor(
                              std::max( ( ap - 2 * agility ) * 2.2, stamina * 2.5 )
                              * 0.4
                            );

    double total_absorb = absorb_amount + p() -> buff.tooth_and_claw_absorb -> current_value;

    if ( sim -> debug )
      sim -> out_debug.printf( "%s's Tooth and Claw value pre-application: %f",
                     player -> name(),
                     p() -> buff.tooth_and_claw_absorb -> current_value );

    p() -> buff.tooth_and_claw_absorb -> trigger( 1, total_absorb );

    if ( sim -> debug )
      sim -> out_debug.printf( "%s's Tooth and Claw value post-application: %f",
                     player -> name(),
                     p() -> buff.tooth_and_claw_absorb -> current_value );

    p() -> buff.tooth_and_claw -> expire();
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.tooth_and_claw -> check() )
      return false;

    return absorb_t::ready();
  }
};

namespace pets {


// ==========================================================================
// Pets and Guardians
// ==========================================================================

// Balance Force of Nature ==================================================

struct force_of_nature_balance_t : public pet_t
{
  struct wrath_t : public spell_t
  {
    wrath_t( force_of_nature_balance_t* player ) :
      spell_t( "wrath", player, player -> find_spell( 113769 ) )
    {
      if ( player -> o() -> pet_force_of_nature[ 0 ] )
        stats = player -> o() -> pet_force_of_nature[ 0 ] -> get_stats( "wrath" );
      may_crit = true;
    }
  };

  druid_t* o() { return static_cast< druid_t* >( owner ); }

  force_of_nature_balance_t( sim_t* sim, druid_t* owner ) :
    pet_t( sim, owner, "treant", true /*GUARDIAN*/, true )
  {
    owner_coeff.sp_from_sp = 1.0;
    action_list_str = "wrath";
  }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA   ] = 0;

    initial.stats.attribute[ ATTR_INTELLECT ] = 0;
    initial.spell_power_per_intellect = 0;
    intellect_per_owner = 0;
    stamina_per_owner = 0;
  }

  virtual resource_e primary_resource() const { return RESOURCE_MANA; }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "wrath"  ) return new wrath_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// Feral Force of Nature ====================================================

struct force_of_nature_feral_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    druid_t* owner;

    melee_t( force_of_nature_feral_t* p )
      : melee_attack_t( "melee", p, spell_data_t::nil() ), owner( 0 )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      special     = false;
      background  = true;
      repeating   = true;
      may_crit    = true;
      may_glance  = true;
      trigger_gcd = timespan_t::zero();
      owner       = p -> o();
    }

    force_of_nature_feral_t* p()
    { return static_cast<force_of_nature_feral_t*>( player ); }

    void init()
    {
      melee_attack_t::init();
      if ( ! player -> sim -> report_pets_separately && player != p() -> o() -> pet_force_of_nature[ 0 ] )
        stats = p() -> o() -> pet_force_of_nature[ 0 ] -> get_stats( name(), this );
    }
  };

  struct rake_t : public melee_attack_t
  {
    druid_t* owner;

    rake_t( force_of_nature_feral_t* p ) :
      melee_attack_t( "rake", p, p -> find_spell( 150017 ) ), owner( 0 )
    {
      dot_behavior     = DOT_REFRESH;
      special          = true;
      may_crit         = true;
      tick_may_crit    = true;
      owner            = p -> o();
    }

    force_of_nature_feral_t* p()
    { return static_cast<force_of_nature_feral_t*>( player ); }
    const force_of_nature_feral_t* p() const
    { return static_cast<force_of_nature_feral_t*>( player ); }

    void init()
    {
      melee_attack_t::init();
      if ( ! player -> sim -> report_pets_separately && player != p() -> o() -> pet_force_of_nature[ 0 ] )
        stats = p() -> o() -> pet_force_of_nature[ 0 ] -> get_stats( name(), this );
    }

    virtual double composite_ta_multiplier() const
    {
      double m = melee_attack_t::composite_ta_multiplier();

      if ( p() -> o() -> buff.cat_form -> check() && p() -> o() -> mastery.razor_claws -> ok() )
        m *= 1.0 + p() -> o() -> cache.mastery_value();

      return m;
    }

    // Treat direct damage as "bleed"
    // Must use direct damage because tick_zeroes cannot be blocked, and this attack is going to get blocked occasionally.
    double composite_da_multiplier() const
    {
      double m = melee_attack_t::composite_da_multiplier();

      if ( p() -> o() -> buff.cat_form -> check() && p() -> o() -> mastery.razor_claws -> ok() )
        m *= 1.0 + p() -> o() -> cache.mastery_value();

      return m;
    }

    virtual double target_armor( player_t* ) const
    { return 0.0; }

    virtual void execute()
    {
      melee_attack_t::execute();
      p() -> change_position( POSITION_BACK ); // After casting it's "opener" ability, move behind the target.
    }
  };
  
  melee_t* melee;

  force_of_nature_feral_t( sim_t* sim, druid_t* p ) :
    pet_t( sim, p, "treant", true, true ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg    = owner -> find_spell( 102703 ) -> effectN( 1 ).min( owner );
    main_hand_weapon.max_dmg    = owner -> find_spell( 102703 ) -> effectN( 1 ).max( owner );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    owner_coeff.ap_from_ap      = 1.0;
  }

  druid_t* o()
  { return static_cast< druid_t* >( owner ); }
  const druid_t* o() const
  { return static_cast<const druid_t* >( owner ); }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA   ] = 0;
    stamina_per_owner = 0.0;

    melee = new melee_t( this );
  }

  virtual void init_action_list()
  {
    action_list_str = "rake";

    pet_t::init_action_list();
  }
  
  action_t* create_action( const std::string& name,
                           const std::string& options_str )
  {
    if ( name == "rake" ) return new rake_t( this );

    return pet_t::create_action( name, options_str );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() )
  {
    pet_t::summon( duration );
    this -> change_position( POSITION_FRONT ); // Emulate Treant spawning in front of the target.
  }

  void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false )
  {
    // FIXME: Currently first swing happens after swingtime # seconds, it should happen after 1-1.5 seconds (random).
    if ( melee && ! melee -> execute_event )
      melee -> schedule_execute();

    pet_t::schedule_ready( delta_time, waiting );
  }
};

// Guardian Force of Nature ====================================================

struct force_of_nature_guardian_t : public pet_t
{
  melee_attack_t* melee;

  struct melee_t : public melee_attack_t
  {
    druid_t* owner;

    melee_t( force_of_nature_guardian_t* p )
      : melee_attack_t( "melee", p, spell_data_t::nil() ), owner( 0 )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      background = true;
      repeating  = true;
      may_crit   = true;
      trigger_gcd = timespan_t::zero();
      owner = p -> o();

      may_glance = true;
      special    = false;
    }
  };

  force_of_nature_guardian_t( sim_t* sim, druid_t* p ) :
    pet_t( sim, p, "treant", true, true ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg    = owner -> find_spell( 102706 ) -> effectN( 1 ).min( owner ) * 0.2;
    main_hand_weapon.max_dmg    = owner -> find_spell( 102706 ) -> effectN( 1 ).max( owner ) * 0.2;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    owner_coeff.ap_from_ap      = 0.2 * 1.2;
  }

  druid_t* o()
  { return static_cast< druid_t* >( owner ); }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA   ] = 0;
    stamina_per_owner = 0.0;

    melee = new melee_t( this );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() )
  {
    pet_t::summon( duration );
    schedule_ready();
  }

  virtual void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! melee -> execute_event ) melee -> execute();
  }
};

} // end namespace pets

namespace buffs {

template <typename BuffBase>
struct druid_buff_t : public BuffBase
{
protected:
  typedef druid_buff_t base_t;
  druid_t& druid;

  // Used when shapeshifting to switch to a new attack & schedule it to occur
  // when the current swing timer would have ended.
  void swap_melee( attack_t* new_attack, weapon_t& new_weapon )
  {
    if ( druid.main_hand_attack && druid.main_hand_attack -> execute_event )
    {
      new_attack -> base_execute_time = new_weapon.swing_time;
      new_attack -> execute_event = new_attack -> start_action_execute_event(
                                      druid.main_hand_attack -> execute_event -> remains() );
      druid.main_hand_attack -> cancel();
    }
    new_attack -> weapon = &new_weapon;
    druid.main_hand_attack = new_attack;
    druid.main_hand_weapon = new_weapon;
  }

public:
  druid_buff_t( druid_t& p, const buff_creator_basics_t& params ) :
    BuffBase( params ),
    druid( p )
  { }

  druid_t& p() const { return druid; }
};

// Barkskin Buff =================================================

struct barkskin_t : public druid_buff_t < buff_t >
{
  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  struct frenzied_regeneration_2pc_t : public heal_t
  {
    double maximum_rage_cost;

    frenzied_regeneration_2pc_t( druid_t* p ) :
      heal_t( "frenzied_regeneration", p, p -> find_class_spell( "Frenzied Regeneration" ) )
    {
      base_dd_min = base_dd_max = attack_power_mod.direct = spell_power_mod.direct = 0.0;

      harmful    = false;
      special    = false;
      proc       = true;
      background = true;

      if ( p -> sets.has_set_bonus( SET_T16_2PC_TANK ) )
        p -> active.ursocs_vigor = new ursocs_vigor_t( p );

      maximum_rage_cost = data().effectN( 1 ).base_value();
    }

    druid_t* p() const
    { return static_cast<druid_t*>( player ); }

    virtual double cost() const
    {
      return 0.0;
    }

    virtual double base_da_min( const action_state_t* ) const
    {
        // max(2.2*(AP - 2*Agi), 2.5*Sta)
        double ap      = p() -> composite_melee_attack_power() * p() -> composite_attack_power_multiplier();
        double agility = p() -> composite_attribute( ATTR_AGILITY ) * p() -> composite_attribute_multiplier( ATTR_AGILITY );
        double stamina = p() -> composite_attribute( ATTR_STAMINA ) * p() -> composite_attribute_multiplier( ATTR_STAMINA );
        return std::max( ( ap - 2 * agility ) * data().effectN( 2 ).percent(), stamina * data().effectN( 3 ).percent() )
               * ( 20 / maximum_rage_cost );
    }

    virtual double base_da_max( const action_state_t* ) const
    {
        // max(2.2*(AP - 2*Agi), 2.5*Sta)
        double ap      = p() -> composite_melee_attack_power() * p() -> composite_attack_power_multiplier();
        double agility = p() -> composite_attribute( ATTR_AGILITY ) * p() -> composite_attribute_multiplier( ATTR_AGILITY );
        double stamina = p() -> composite_attribute( ATTR_STAMINA ) * p() -> composite_attribute_multiplier( ATTR_STAMINA );
        return std::max( ( ap - 2 * agility ) * data().effectN( 2 ).percent(), stamina * data().effectN( 3 ).percent() )
               * ( 20 / maximum_rage_cost );
    }

    virtual void execute()
    {
      heal_t::execute();

      if ( p() -> sets.has_set_bonus( SET_T16_4PC_TANK ) )
        p() -> active.ursocs_vigor -> trigger_hot( 20.0 );
    }

    virtual bool ready()
    {
      return true;
    }
  };

  action_t* frenzied_regeneration;

  barkskin_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "barkskin", p.find_class_spell( "Barkskin" ) ) ),
    frenzied_regeneration( nullptr )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell

    if ( player -> sets.has_set_bonus( SET_T16_2PC_TANK ) )
      frenzied_regeneration = new frenzied_regeneration_2pc_t( static_cast<druid_t*>( player ) );
  }

  virtual void expire_override()
  {
    buff_t::expire_override();

    if ( p() -> sets.has_set_bonus( SET_T16_2PC_TANK ) )
    {
      // Trigger 3 seconds of Savage Defense
      if ( p() -> buff.savage_defense -> check() )
        p() -> buff.savage_defense -> extend_duration( p(), timespan_t::from_seconds( 3.0 ) );
      else
        p() -> buff.savage_defense -> trigger( 1, buff_t::DEFAULT_VALUE(), 1, timespan_t::from_seconds( 3.0 ) );

      // Trigger 4pc equal to the consumption of 30 rage.
      if ( p() -> sets.has_set_bonus( SET_T16_4PC_TANK ) )
        p() -> active.ursocs_vigor -> trigger_hot( 30.0 );

      // Trigger a 20 rage Frenzied Regeneration
      frenzied_regeneration -> execute();
    }
  }
};

// Celestial Alignment Buff =================================================

struct celestial_alignment_t : public druid_buff_t < buff_t >
{
  celestial_alignment_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "celestial_alignment", p.find_class_spell( "Celestial Alignment" ) ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
  }

  virtual void expire_override()
  {
    buff_t::expire_override();

    druid_t* p = static_cast<druid_t*>( player );
    p -> buff.eclipse_lunar -> expire();
    p -> buff.eclipse_solar -> expire();
  }
};

// Bear Form

struct bear_form_t : public druid_buff_t< buff_t >
{
public:
  bear_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "bear_form", p.find_class_spell( "Bear Form" ) ) ),
    rage_spell( p.find_spell( 17057 ) )
  {
    // HOTW will require invalidation of hit
    if ( druid.talent.heart_of_the_wild -> ok() )
    {
      add_invalidate( CACHE_HIT );
      add_invalidate( CACHE_EXP );
    }
    add_invalidate( CACHE_AGILITY );
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_HASTE );
    add_invalidate( CACHE_CRIT );
    add_invalidate( CACHE_STAMINA );
    add_invalidate( CACHE_ARMOR );
  }

  virtual void expire_override()
  {
    base_t::expire_override();

    druid.main_hand_weapon = druid.caster_form_weapon;

    sim -> auras.critical_strike -> decrement();

    druid.player_t::recalculate_resource_max( RESOURCE_HEALTH );

    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.vengeance_stop();

    druid.current.attack_power_per_agility -= 1.0;
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid.buff.moonkin_form -> expire();
    druid.buff.cat_form -> expire();

    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.vengeance_start();

    swap_melee( druid.bear_melee_attack, druid.bear_weapon );

    // Set rage to 0 and then gain rage to 10
    druid.resource_loss( RESOURCE_RAGE, druid.resources.current[ RESOURCE_RAGE ] );
    druid.resource_gain( RESOURCE_RAGE, rage_spell -> effectN( 1 ).base_value() / 10.0, druid.gain.bear_form );
    // TODO: Clear rage on bear form exit instead of entry.

    base_t::start( stacks, value, duration );

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();

    druid.player_t::recalculate_resource_max( RESOURCE_HEALTH );

    druid.current.attack_power_per_agility += 1.0;
  }
private:
  const spell_data_t* rage_spell;
};

// Berserk Buff ======================================================

struct berserk_buff_t : public druid_buff_t< buff_t >
{
  berserk_buff_t( druid_t& p ) :
    druid_buff_t( p, buff_creator_t( &p, "berserk", p.find_specialization_spell( "Berserk" ) ) )
  {}

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    if ( druid.perk.enhanced_berserk -> ok() )
      player -> resources.max[ RESOURCE_ENERGY ] += druid.perk.enhanced_berserk -> effectN( 1 ).base_value();

    return buff_t::trigger( stacks, value, chance, duration );
  }

  virtual void expire_override()
  {
    if ( druid.perk.enhanced_berserk -> ok() )
    {
      player -> resources.max[ RESOURCE_ENERGY ] -= druid.perk.enhanced_berserk -> effectN( 1 ).base_value();
      // Force energy down to cap if it's higher.
      player -> resources.current[ RESOURCE_ENERGY ] = std::min( player -> resources.current[ RESOURCE_ENERGY ], player -> resources.max[ RESOURCE_ENERGY ]);
    }

    buff_t::expire_override();
  }
};

// Cat Form

struct cat_form_t : public druid_buff_t< buff_t >
{
  cat_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "cat_form", p.find_class_spell( "Cat Form" ) ) )
  {
    // HOTW will require invalidation of various things, as the actor may
    // change shapeshift forms during HoTW, that affects the stats
    if ( druid.talent.heart_of_the_wild -> ok() )
    {
      add_invalidate( CACHE_HIT );
      add_invalidate( CACHE_EXP );
    }
    add_invalidate( CACHE_AGILITY );
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  virtual void expire_override()
  {
    base_t::expire_override();

    druid.main_hand_weapon = druid.caster_form_weapon;

    sim -> auras.critical_strike -> decrement();

    druid.current.attack_power_per_agility -= 1.0;
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid.buff.bear_form -> expire();
    druid.buff.moonkin_form -> expire();

    swap_melee( druid.cat_melee_attack, druid.cat_weapon );

    base_t::start( stacks, value, duration );

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();

    druid.current.attack_power_per_agility += 1.0;
  }
};

// Moonkin Form

struct moonkin_form_t : public druid_buff_t< buff_t >
{
  moonkin_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "moonkin_form", p.find_class_spell( "Moonkin Form" ) )
               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  { }

  virtual void expire_override()
  {
    base_t::expire_override();

    sim -> auras.haste -> decrement();
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid.buff.bear_form -> expire();
    druid.buff.cat_form  -> expire();

    base_t::start( stacks, value, duration );

    if ( ! sim -> overrides.haste )
      sim -> auras.haste -> trigger();
  }
};

// Eclipse (Lunar) Buff =================================================

struct eclipse_lunar_t : public druid_buff_t < buff_t >
{
  eclipse_lunar_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "lunar_eclipse", p.find_spell( 48518 ) )
               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  {}

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    if ( p() -> talent.dream_of_cenarius -> ok() )
      p() -> buff.dream_of_cenarius -> trigger();

    return druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );
  }
};

// Eclipse (Solar) Buff =================================================

struct eclipse_solar_t : public druid_buff_t < buff_t >
{
  eclipse_solar_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "solar_eclipse", p.find_spell( 48517 ) )
               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  {}

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    if ( p() -> talent.dream_of_cenarius -> ok() )
      p() -> buff.dream_of_cenarius -> trigger();

    return druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );
  }
};

// Might of Ursoc Buff ======================================================

struct might_of_ursoc_t : public buff_t
{
  double percent_gain;
  int health_gain;

  might_of_ursoc_t( druid_t* p, const uint32_t id, const std::string& /* n */ ) :
    buff_t( buff_creator_t( p, "might_of_ursoc", p -> find_spell( id ) ) ),
    health_gain( 0 )
  {
    percent_gain = data().effectN( 1 ).percent() + p -> glyph.might_of_ursoc -> effectN( 1 ).percent();
  }

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    health_gain = ( int ) floor( player -> resources.max[ RESOURCE_HEALTH ] * percent_gain );
    player -> stat_gain( STAT_MAX_HEALTH, health_gain );

    return buff_t::trigger( stacks, value, chance, duration );
  }

  virtual void expire_override()
  {
    player -> stat_loss( STAT_MAX_HEALTH, health_gain );

    buff_t::expire_override();
  }
};

// Tooth and Claw Absorb Buff

struct tooth_and_claw_absorb_t : public absorb_buff_t
{
  tooth_and_claw_absorb_t( druid_t* p ) :
    absorb_buff_t( absorb_buff_creator_t( p, "tooth_and_claw_absorb", p -> find_spell( 135597 ) )
                   .school( SCHOOL_PHYSICAL )
                   .source( p -> get_stats( "tooth_and_claw" ) )
                   .gain( p -> get_gain( "tooth_and_claw" ) )
    )
  {}

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual void absorb_used( double /* amount */ )
  {
    p() -> buff.tooth_and_claw_absorb -> expire();
  }
};

struct heart_of_the_wild_buff_t : public druid_buff_t < buff_t >
{
private:
  const spell_data_t* select_spell( const druid_t& p )
  {
    unsigned id = 0;
    if ( p.talent.heart_of_the_wild -> ok() )
    {
      switch ( p.specialization() )
      {
        case DRUID_BALANCE:     id = 108291; break;
        case DRUID_FERAL:       id = 108292; break;
        case DRUID_GUARDIAN:    id = 108293; break;
        case DRUID_RESTORATION: id = 108294; break;
        default: break;
      }
    }
    return p.find_spell( id );
  }

  bool all_but( specialization_e spec )
  { return check() > 0 && player -> specialization() != spec; }

public:
  heart_of_the_wild_buff_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "heart_of_the_wild" )
            .spell( select_spell( p ) ) )
  {
    add_invalidate( CACHE_AGILITY );
    add_invalidate( CACHE_HIT );
    add_invalidate( CACHE_EXP );
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
  }

  bool heals_are_free()
  { return all_but( DRUID_RESTORATION ); }

  bool damage_spells_are_free()
  { return all_but( DRUID_BALANCE ); }

  double damage_spell_multiplier()
  {
    if ( ! check() ) return 0.0;

    double m;
    switch ( player -> specialization() )
    {
      case DRUID_FERAL:
      case DRUID_RESTORATION:
        m = data().effectN( 4 ).percent();
        break;
      case DRUID_GUARDIAN:
        m = data().effectN( 5 ).percent();
        break;
      case DRUID_BALANCE:
      default:
        return 0.0;
    }

    return m;
  }

  double heal_multiplier()
  {
    if ( ! check() ) return 0.0;

    double m;
    switch ( player -> specialization() )
    {
      case DRUID_FERAL:
      case DRUID_GUARDIAN:
      case DRUID_BALANCE:
        m = data().effectN( 2 ).percent();
        break;
      case DRUID_RESTORATION:
      default:
        return 0.0;
    }

    return m;
  }

  double attack_hit_expertise()
  {
    if ( ! check() ) return 0.0;

    druid_t& p = this -> p();
    switch ( p.specialization() )
    {
      case DRUID_FERAL:
        if ( ! p.buff.bear_form -> check() ) return 0.0;
        break;
      case DRUID_GUARDIAN:
        if ( ! p.buff.cat_form  -> check() ) return 0.0;
        break;
      case DRUID_BALANCE:
      case DRUID_RESTORATION:
        if ( ! p.buff.bear_form -> check() && ! p.buff.cat_form -> check() ) return 0.0;
        break;
      default:
        return 0.0;
    }
    return 0.15;
  }

  double spell_hit()
  {
    if ( ! check() ) return 0.0;

    switch ( player -> specialization() )
    {
      case DRUID_FERAL:
      case DRUID_GUARDIAN:
      case DRUID_RESTORATION:
        return 0.15;
      default:
        return 0.0;
    }
  }

  double agility_multiplier()
  {
    if ( ! check() ) return 0.0;

    druid_t& p = this -> p();
    switch ( p.specialization() )
    {
      case DRUID_FERAL:
        if ( p.buff.bear_form -> check() )
          return 0.5;
        break;
      case DRUID_GUARDIAN:
        if ( p.buff.cat_form -> check() )
          return 1.1;
        break;
      case DRUID_BALANCE:
      case DRUID_RESTORATION:
        if ( p.buff.cat_form -> check() )
          return 1.1;
        break;
      default:
        break;
    }

    return 0.0;
  }
};
} // end namespace buffs

// Template for common druid action code. See priest_action_t.
template <class Base>
struct druid_action_t : public Base
{
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef druid_action_t base_t;

  druid_action_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  {
    ab::may_crit      = true;
    ab::tick_may_crit = true;
  }

  druid_t* p()
  { return static_cast<druid_t*>( ab::player ); }
  const druid_t* p() const
  { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  bool trigger_omen_of_clarity()
  {
    if ( ab::proc ) return false;

    return p() -> buff.omen_of_clarity -> trigger();
  }
};

// Druid melee attack base for cat_attack_t and bear_attack_t
template <class Base>
struct druid_attack_t : public druid_action_t< Base >
{
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_attack_t base_t;

  druid_attack_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  {
    ab::may_glance    = false;
    ab::special       = true;
  }

  virtual void execute()
  {
    ab::execute();

    if ( this -> p() -> buff.natures_vigil -> check() && this -> result_is_hit( this -> execute_state -> result ) )
      this -> p() -> active.natures_vigil -> trigger( *this );
  }

  virtual void tick( dot_t* d )
  {
    ab::tick( d );

    if ( this -> p() -> buff.natures_vigil -> up() )
      this -> p() -> active.natures_vigil -> trigger( *this );
  }

  void trigger_lotp( const action_state_t* s )
  {
    // Has to do damage and can't be a proc
    if ( s -> result_amount > 0 && ! ab::proc && this -> p() -> active.leader_of_the_pack -> cooldown -> up() )
      this -> p() -> active.leader_of_the_pack -> execute();
  }
};
namespace cat_attacks {

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

struct cat_attack_state_t : public action_state_t
{
  int cp;

  cat_attack_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), cp( 0 )
  { }

  void initialize()
  { action_state_t::initialize(); cp = 0; }

  std::ostringstream& debug_str( std::ostringstream& s )
  { action_state_t::debug_str( s ) << " cp=" << cp; return s; }

  void copy_state( const action_state_t* o )
  {
    action_state_t::copy_state( o );
    const cat_attack_state_t* st = debug_cast<const cat_attack_state_t*>( o );
    cp = st -> cp;
  }
};

struct cat_attack_t : public druid_attack_t<melee_attack_t>
{
  bool             requires_stealth_;
  bool             requires_combo_points;
  int              adds_combo_points;
  double           base_dd_bonus;
  double           base_td_bonus;

  cat_attack_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s ),
    requires_stealth_( false ),
    requires_combo_points( false ), adds_combo_points( 0 ),
    base_dd_bonus( 0.0 ), base_td_bonus( 0.0 )
  {
    parse_options( 0, options );

    parse_special_effect_data();
  }

private:
  void parse_special_effect_data()
  {
    for ( size_t i = 1; i <= data().effect_count(); i++ )
    {
      const spelleffect_data_t& ed = data().effectN( i );
      effect_type_t type = ed.type();

      if ( type == E_ADD_COMBO_POINTS )
        adds_combo_points = ed.base_value();
      else if ( type == E_APPLY_AURA && ed.subtype() == A_PERIODIC_DAMAGE )
      {
        snapshot_flags |= STATE_AP;
        base_td_bonus = ed.bonus( player );
      }
      else if ( type == E_SCHOOL_DAMAGE )
      {
        snapshot_flags |= STATE_AP;
        base_dd_bonus = ed.bonus( player );
      }
    }
  }
public:
  virtual double cost() const
  {
    double c = base_t::cost();

    if ( c == 0 )
      return 0;

    if ( harmful && p() -> buff.omen_of_clarity -> check() )
      return 0;

    if ( p() -> buff.berserk -> check() )
      c *= 1.0 + p() -> spell.berserk_cat -> effectN( 1 ).percent();

    return c;
  }

  const cat_attack_state_t* cat_state( const action_state_t* st ) const
  { return debug_cast< const cat_attack_state_t* >( st ); }

  cat_attack_state_t* cat_state( action_state_t* st ) const
  { return debug_cast< cat_attack_state_t* >( st ); }

  virtual double bonus_ta( const action_state_t* s ) const
  { return base_td_bonus * ( requires_combo_points ? cat_state( s ) -> cp : 1 ); }

  virtual double bonus_da( const action_state_t* s ) const
  { return base_dd_bonus * ( requires_combo_points ? cat_state( s ) -> cp : 1 ); }

  virtual action_state_t* new_state()
  { return new cat_attack_state_t( this, target ); }

  virtual void snapshot_state( action_state_t* state, dmg_e rt )
  {
    base_t::snapshot_state( state, rt );
    cat_state( state ) -> cp = td( state -> target ) -> combo_points.get();
  }

  void trigger_savagery()
  {
    // Bail out if we have savagery talent, buff should already be up so lets not mess with it.
    if ( p() -> talent.savagery -> ok() )
      return;

    timespan_t base_tick_time = p() -> find_specialization_spell( "Savage Roar" ) -> effectN( 3 ).time_value();
    timespan_t seconds_per_combo = timespan_t::from_seconds( 6.0 );
    timespan_t duration = p() -> find_specialization_spell( "Savage Roar" ) -> duration();

    if ( p() -> buff.savage_roar -> check() )
    {
      // Savage Roar behaves like a dot with DOT_REFRESH.
      // You will not lose your current 'tick' when refreshing.
      int result = static_cast<int>( p() -> buff.savage_roar -> remains() / base_tick_time );
      timespan_t carryover = p() -> buff.savage_roar -> remains();
      carryover -= base_tick_time * result;
      duration += carryover;
    }
    duration += seconds_per_combo * 5;

    p() -> buff.savage_roar -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  }

  virtual void execute()
  {
    //if ( p() -> buff.prowl -> check() && p() -> glyph.savagery -> ok() )
      //trigger_savagery(); Glyph is changed in WoD

    base_t::execute();

    if ( ( cat_state( execute_state ) -> cp > 0 || this -> name_str == "savage_roar" ) && requires_combo_points )
    {
      if ( p() -> buff.feral_rage -> up() ) // tier16_4pc_melee
      {
        td( target ) -> combo_points.add( p() -> buff.feral_rage -> data().effectN( 1 ).base_value(), &p() -> buff.feral_rage -> name_str );
        p() -> buff.feral_rage -> expire();
      }
    }

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( adds_combo_points )
      {
        td( target ) -> combo_points.add( adds_combo_points, &name_str );

        if ( p() -> spell.primal_fury -> ok() && execute_state -> result == RESULT_CRIT )
        {
          p() -> proc.primal_fury -> occur();
          td( target ) -> combo_points.add( p() -> find_spell( 16953 ) -> effectN( 1 ).base_value(), &name_str );
        }
      }

      if ( execute_state -> result == RESULT_CRIT )
        trigger_lotp( execute_state );

      if ( ( cat_state( execute_state ) -> cp > 0 || this -> name_str == "savage_roar" ) && requires_combo_points )
      {
        if ( player -> sets.has_set_bonus( SET_T15_2PC_MELEE ) &&
            rng().roll( cat_state( execute_state ) -> cp * 0.15 ) )
        {
          p() -> proc.tier15_2pc_melee -> occur();
          td( target ) -> combo_points.add( 1, &name_str );
        }
      }
    }
    else
    {
      trigger_energy_refund();
    }

    if ( harmful )
      p() -> buff.prowl -> expire();
  }

  virtual void impact( action_state_t* s )
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      druid_td_t& td = *this -> td( s -> target );
      if ( td.combo_points.get() > 0 && requires_combo_points && p() -> spec.predatory_swiftness -> ok() )
        p() -> buff.predatory_swiftness -> trigger( 1, 1, td.combo_points.get() * 0.20 );
    }
  }

  virtual void consume_resource()
  {
    int combo_points_spent = 0;
    base_t::consume_resource();

    if ( requires_combo_points && result_is_hit( execute_state -> result ) )
    {
      combo_points_spent = td( execute_state -> target ) -> combo_points.consume( &name_str );
    }

    if ( harmful && p() -> buff.omen_of_clarity -> up() )
    {
      // Treat the savings like a energy gain.
      double amount = melee_attack_t::cost();
      if ( amount > 0 )
      {
        p() -> gain.omen_of_clarity -> add( RESOURCE_ENERGY, amount );
        p() -> buff.omen_of_clarity -> expire();
      }
    }

    if ( combo_points_spent > 0 && p() -> talent.soul_of_the_forest -> ok() )
    {
      p() -> resource_gain( RESOURCE_ENERGY,
                            combo_points_spent * p() -> talent.soul_of_the_forest -> effectN( 1 ).base_value(),
                            p() -> gain.soul_of_the_forest );
    }
  }

  virtual bool ready()
  {
    if ( ! base_t::ready() )
      return false;

    if ( ! p() -> buff.cat_form -> check() )
      return false;

    if ( requires_stealth() )
      if ( ! p() -> buff.prowl -> check() )
        return false;

    if ( requires_combo_points && ! td( target ) -> combo_points.get() )
      return false;

    return true;
  }

  virtual bool   requires_stealth() const
  {
    if ( p() -> buff.king_of_the_jungle -> check() )
      return false;

    return requires_stealth_;
  }

  void trigger_energy_refund()
  {
    double energy_restored = resource_consumed * 0.80;

    player -> resource_gain( RESOURCE_ENERGY, energy_restored, p() -> gain.energy_refund );
  }

  void extend_rip( action_state_t& s )
  {
    if ( result_is_hit( s.result ) )
    {
      if ( td( s.target ) -> dots.rip -> ticking &&
           td( s.target ) -> dots.rip -> added_ticks < 4 )
      {
        // In-game adds 3 seconds per extend, to model we'll add 1/2/1 ticks. Can't use extend_duration_seconds for this since it rounds down to ticks.
        int extra_ticks = ( td( s.target ) -> dots.rip -> added_ticks % 3 ) ? 2 : 1;
        td( s.target ) -> dots.rip -> extend_duration( extra_ticks, false, 0 );
      }
    }
  }
}; // end druid_cat_attack_t


// Cat Melee Attack =========================================================

struct cat_melee_t : public cat_attack_t
{
  cat_melee_t( druid_t* player ) :
    cat_attack_t( "cat_melee", player, spell_data_t::nil(), "" )
  {
    school = SCHOOL_PHYSICAL;
    may_glance  = true;
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    special = false;
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return cat_attack_t::execute_time();
  }

  virtual double action_multiplier() const
  {
    double cm = cat_attack_t::action_multiplier();

    if ( p() -> buff.cat_form -> check() )
      cm *= 1.0 + p() -> buff.cat_form -> data().effectN( 3 ).percent();

    return cm;
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    if ( result_is_hit( state -> result ) && trigger_omen_of_clarity() && p() -> sets.has_set_bonus( SET_T16_2PC_MELEE ) )
      p() -> buff.feral_fury -> trigger();
  }
};

// Feral Charge (Cat) =======================================================

struct feral_charge_cat_t : public cat_attack_t
{
  // TODO: Figure out Wild Charge
  feral_charge_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "feral_charge_cat", p, p -> talent.wild_charge, options_str )
  {
    may_miss   = false;
    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
    special = false;
  }

  virtual bool ready()
  {
    bool ranged = ( player -> position() == POSITION_RANGED_FRONT ||
                    player -> position() == POSITION_RANGED_BACK );

    if ( player -> in_combat && ! ranged )
    {
      return false;
    }

    return cat_attack_t::ready();
  }
};

// Ferocious Bite ===========================================================

struct ferocious_bite_t : public cat_attack_t
{
  double excess_energy;
  double max_excess_energy;
  double ap_per_point;

  ferocious_bite_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "ferocious_bite", p, p -> find_class_spell( "Ferocious Bite" ), options_str ),
    excess_energy( 0 ), max_excess_energy( 0 ), ap_per_point( 0.0 )
  {
    ap_per_point          = 0.196; // FIXME: Figure out where the hell this is in the spell data...
    max_excess_energy     = 25.0;
    requires_combo_points = true;
    special               = true;
    base_multiplier      *= 1.0 + p -> perk.improved_ferocious_bite -> effectN( 1 ).percent();
  }

  double attack_direct_power_coefficient( const action_state_t* state ) const
  { return cat_state( state ) -> cp * ap_per_point; }

  virtual void execute()
  {
    // Berserk does affect the additional energy consumption.
    if ( p() -> buff.berserk -> check() )
      max_excess_energy *= 1.0 + p() -> spell.berserk_cat -> effectN( 1 ).percent();

    excess_energy = std::min( max_excess_energy,
                              ( p() -> resources.current[ RESOURCE_ENERGY ] - cat_attack_t::cost() ) );


    cat_attack_t::execute();

    if ( p() -> buff.tier15_4pc_melee -> up() )
      p() -> buff.tier15_4pc_melee -> decrement();

    max_excess_energy = 25.0;
  }

  void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( p() -> glyph.ferocious_bite -> ok() )
      {
        double heal_pct = p() -> glyph.ferocious_bite -> effectN( 1 ).percent() *
                          ( excess_energy + cost() ) /
                          p() -> glyph.ferocious_bite -> effectN( 2 ).base_value();
        double amount = p() -> resources.max[ RESOURCE_HEALTH ] * heal_pct;
        p() -> resource_gain( RESOURCE_HEALTH, amount, p() -> gain.glyph_ferocious_bite );
      }

      double health_percentage = 25.0;
      if ( p() -> sets.has_set_bonus( SET_T13_2PC_MELEE ) )
        health_percentage = p() -> sets.set( SET_T13_2PC_MELEE ) -> effectN( 2 ).base_value();

      if ( state -> target -> health_percentage() <= health_percentage )
      {
        if ( td( state -> target ) -> dots.rip -> ticking )
          td( state -> target ) -> dots.rip -> refresh_duration( 0 );
      }
    }
  }

  virtual void consume_resource()
  {
    // Ferocious Bite consumes 25+x energy, with 0 <= x <= 25.
    // Consumes the base_cost and handles Omen of Clarity
    cat_attack_t::consume_resource();

    if ( result_is_hit( execute_state -> result ) )
    {
      // Let the additional energy consumption create it's own debug log entries.
      if ( sim -> debug )
        sim -> out_debug.printf( "%s consumes an additional %.1f %s for %s", player -> name(),
                       excess_energy, util::resource_type_string( current_resource() ), name() );

      player -> resource_loss( current_resource(), excess_energy );
      stats -> consume_resource( current_resource(), excess_energy );
    }
  }

  double action_multiplier() const
  {
    double dm = cat_attack_t::action_multiplier();

    dm *= 1.0 + excess_energy / max_excess_energy;

    return dm;
  }

  double composite_target_crit( player_t* t ) const
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( t -> debuffs.bleeding -> check() )
      tc += data().effectN( 2 ).percent();

    if ( p() -> buff.tier15_4pc_melee -> check() )
      tc += p() -> buff.tier15_4pc_melee -> data().effectN( 1 ).percent();

    return tc;
  }
};

// Maim =====================================================================

struct maim_t : public cat_attack_t
{
  maim_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "maim", player, player -> find_class_spell( "Maim" ), options_str )
  {
    requires_combo_points = true;
    special               = true;
    base_multiplier      *= 1.0 + player -> glyph.maim -> effectN( 1 ).percent();
  }
};

// Pounce ===================================================================

struct pounce_bleed_t : public cat_attack_t
{
  pounce_bleed_t( druid_t* player ) :
    cat_attack_t( "pounce", player, player -> find_spell( 9007 ) )
  {
    background = dual = true;
    attack_power_mod.tick = data().effectN( 1 ).ap_coeff();

    base_multiplier *= 1.0 + player -> perk.improved_pounce -> effectN( 1 ).percent();
  }
};

struct pounce_t : public cat_attack_t
{
  pounce_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "pounce", p, p -> find_class_spell( "Pounce" ), options_str )
  {
    special = true;
    execute_action = new pounce_bleed_t( p );
    requires_stealth_ = true;
    may_dodge = may_block = may_parry = false;
  }
};

// Rake DoT =================================================================

struct rake_bleed_t : public cat_attack_t {
  rake_bleed_t( druid_t* p ) :
    cat_attack_t( "rake", p, p -> find_spell( 155722 ) )
  {
    dual = background     = true;
    dot_behavior          = DOT_REFRESH;
    attack_power_mod.tick = data().effectN( 1 ).ap_coeff();
    may_miss = may_dodge = may_parry = false;
  }
};

// Rake =====================================================================
/* TODO: Find out whether the direct damage can proc effects BEFORE the periodic damage is applied.
   This is how it is handled in the sim currently. */

struct rake_t : public cat_attack_t
{
  action_t* rake_bleed;

  rake_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rake", p, p -> find_class_spell( "Rake" ), options_str )
  {
    special                 = true;
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();

    rake_bleed = new rake_bleed_t( p );
  }

  // Treat direct damage as "bleed"
  // Must use direct damage because tick_zeroes cannot be blocked, and
  // this attack can be blocked if the druid is in front of the target.
  double composite_da_multiplier() const
  {
    double m = melee_attack_t::composite_da_multiplier();

    if ( p() -> buff.cat_form -> up() && p() -> mastery.razor_claws -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      rake_bleed -> target = state -> target;
      rake_bleed -> execute();
    }
  }

  virtual double target_armor( player_t* ) const
  { return 0.0; }
};

// Ravage ===================================================================

struct ravage_t : public cat_attack_t
{
  int extends_rip;
  double extra_crit_amount;
  double extra_crit_threshold;

  ravage_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "ravage", player, player -> find_class_spell( "Ravage" ) ),
    extends_rip( 0 ), extra_crit_amount( 0.0 ), extra_crit_threshold( 0.0 )
  {
    option_t options[] =
    {
      opt_bool( "extend_rip", extends_rip ),
      opt_null()
    };
    parse_options( options, options_str );
    requires_stealth_  = true;
    special            = true;

    const spell_data_t* extra_crit = player -> find_spell( 16974 );

    extra_crit_amount    = extra_crit -> effectN( 1 ).percent();
    extra_crit_threshold = extra_crit -> effectN( 2 ).base_value();
  }

  virtual bool   requires_stealth() const
  {
    if ( p() -> sets.has_set_bonus( SET_PVP_4PC_MELEE ) )
      if ( p() -> cooldown.pvp_4pc_melee -> up() )
        return false;

    return cat_attack_t::requires_stealth();
  }

  virtual double cost() const
  {
    if ( p() -> sets.has_set_bonus( SET_PVP_4PC_MELEE ) )
      if ( p() -> cooldown.pvp_4pc_melee -> up() )
        return 0;

    return cat_attack_t::cost();
  }

  double composite_target_crit( player_t* t ) const
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( target && ( target -> is_enemy() || target -> is_add() ) && ( target -> health_percentage() > extra_crit_threshold ) )
      tc += extra_crit_amount;

    if ( p() -> buff.tier15_4pc_melee -> check() )
      tc += p() -> buff.tier15_4pc_melee -> data().effectN( 1 ).percent();

    return tc;
  }

  double composite_da_multiplier() const
  {
    double m = cat_attack_t::composite_da_multiplier();

    if ( p() -> buff.feral_fury -> check() )
      m *= 1.0 + p() -> buff.feral_fury -> data().effectN( 1 ).percent();

    return m;
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    if ( p() -> sets.has_set_bonus( SET_PVP_4PC_MELEE ) )
      if ( p() -> cooldown.pvp_4pc_melee -> up() )
        p() -> cooldown.pvp_4pc_melee -> start();

    extend_rip( *state );
  }

  virtual bool ready()
  {
    if ( extends_rip )
      if ( ! td( target ) -> dots.rip -> ticking ||
           ( td( target ) -> dots.rip -> added_ticks == 4 ) )
        return false;

    return cat_attack_t::ready();
  }

  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.feral_fury -> up();

    if ( p() -> buff.tier15_4pc_melee -> up() )
      p() -> buff.tier15_4pc_melee -> decrement();
  }
};

// Rip ======================================================================

struct rip_t : public cat_attack_t
{
  double ap_per_point;

  rip_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rip", p, p -> find_class_spell( "Rip" ), options_str ),
    ap_per_point( 0.0 )
  {
    ap_per_point          = data().effectN( 1 ).ap_coeff();
    requires_combo_points = true;
    may_crit              = false;
    dot_behavior          = DOT_REFRESH;
    special               = true;

    num_ticks += ( int ) (  player -> sets.set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value() / base_tick_time );
  }

  double attack_tick_power_coefficient( const action_state_t* state ) const
  { return cat_state( state ) -> cp * ap_per_point; }
};

// Savage Roar ==============================================================

struct savage_roar_t : public cat_attack_t
{
  timespan_t seconds_per_combo;

  savage_roar_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "savage_roar", p, p -> find_class_spell( "Savage Roar" ), options_str ),
    seconds_per_combo( timespan_t::from_seconds( 6.0 ) ) // plus 6s per cp used. Must change this value in cat_attack_t::trigger_savagery() as well.
  {
    may_miss              = false;
    harmful               = false;
    requires_combo_points = true;
    num_ticks             = 0;
    special               = false;
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    timespan_t duration = data().duration();

    if ( p() -> buff.savage_roar -> check() )
    {
      // Savage Roar behaves like a dot with DOT_REFRESH.
      // You will not lose your current 'tick' when refreshing.
      int result = static_cast<int>( p() -> buff.savage_roar -> remains() / base_tick_time );
      timespan_t carryover = p() -> buff.savage_roar -> remains();
      carryover -= base_tick_time * result;
      duration += carryover;
    }
    duration += seconds_per_combo * td( state -> target ) -> combo_points.get();


    p() -> buff.savage_roar -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  }

  virtual bool ready()
  {
    if ( p() -> talent.savagery -> ok() )
      return false;
    else if ( data().duration() + seconds_per_combo * td( target ) -> combo_points.get() > p() -> buff.savage_roar -> remains() )
      return cat_attack_t::ready();
    else
      return false;
  }
};

// Shred ====================================================================

struct shred_t : public cat_attack_t
{
  int extends_rip;

  shred_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "shred", p, p -> find_class_spell( "Shred" ) ),
    extends_rip( 0 )
  {
    option_t options[] =
    {
      opt_bool( "extend_rip", extends_rip ),
      opt_null()
    };
    parse_options( options, options_str );

    base_multiplier *= 1.0 + p -> perk.improved_shred -> effectN( 1 ).percent() + player -> sets.set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();
    special          = true;
  }

  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.feral_fury -> up();

    if ( p() -> buff.tier15_4pc_melee -> up() )
      p() -> buff.tier15_4pc_melee -> decrement();
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    extend_rip( *state );
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + p() -> spell.swipe -> effectN( 2 ).percent();

    return tm;
  }

  double composite_target_crit( player_t* t ) const
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( p() -> buff.tier15_4pc_melee -> check() )
      tc += p() -> buff.tier15_4pc_melee -> data().effectN( 1 ).percent();

    return tc;
  }

  double composite_da_multiplier() const
  {
    double m = cat_attack_t::composite_da_multiplier();

    if ( p() -> buff.feral_fury -> check() )
      m *= 1.0 + p() -> buff.feral_fury -> data().effectN( 1 ).percent();

    return m;
  }

  virtual bool ready()
  {
    if ( extends_rip )
      if ( ! td( target ) -> dots.rip -> ticking ||
           ( td( target ) -> dots.rip -> added_ticks == 4 ) )
        return false;

    return cat_attack_t::ready();
  }
};

// Skull Bash (Cat) =========================================================

struct skull_bash_cat_t : public cat_attack_t
{
  skull_bash_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "skull_bash_cat", p, p -> find_specialization_spell( "Skull Bash" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    cooldown -> duration += p -> glyph.skull_bash -> effectN( 1 ).time_value();
    special = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return cat_attack_t::ready();
  }
};

// Swipe ==============================================================

struct swipe_t : public cat_attack_t
{
  swipe_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "swipe", player, player -> find_class_spell( "Swipe" ), options_str )
  {
    aoe     = -1;
    special = true;
  }

  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.feral_fury -> up();

    if ( p() -> buff.tier15_4pc_melee -> up() )
      p() -> buff.tier15_4pc_melee -> decrement();
  }

  double composite_da_multiplier() const
  {
    double m = cat_attack_t::composite_da_multiplier();

    if ( p() -> buff.feral_fury -> check() )
      m *= 1.0 + p() -> buff.feral_fury -> data().effectN( 1 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 2 ).percent();

    return tm;
  }

  double composite_target_crit( player_t* t ) const
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( p() -> buff.tier15_4pc_melee -> check() )
      tc += p() -> buff.tier15_4pc_melee -> data().effectN( 1 ).percent();

    return tc;
  }
};

// Thrash (Cat) =============================================================

struct thrash_cat_t : public cat_attack_t
{
  action_t* rake_bleed;

  thrash_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "thrash_cat", p, p -> find_spell( 106830 ), options_str )
  {
    aoe                     = -1;
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();
    attack_power_mod.tick   = data().effectN( 2 ).ap_coeff();

    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    dot_behavior      = DOT_REFRESH;
    special           = true;
    adds_combo_points = 0;

    if ( p -> talent.bloody_thrash -> ok() )
    {
      rake_bleed = new rake_bleed_t( p );
      adds_combo_points = 1;
    }
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    if ( p() -> talent.bloody_thrash -> ok() && result_is_hit( state -> result ) )
    {
      rake_bleed -> target = state -> target;
      rake_bleed -> execute();
    }
  }

  // Treat direct damage as "bleed"
  double composite_da_multiplier() const
  {
    double m = cat_attack_t::composite_da_multiplier();

    if ( p() -> buff.cat_form -> check() && p() -> mastery.razor_claws -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const
  { return 0.0; }

  virtual bool ready()
  {
    if ( ! p() -> buff.cat_form -> check() )
      return false;

    return cat_attack_t::ready();
  }
};

// Tiger's Fury =============================================================

struct tigers_fury_t : public cat_attack_t
{
  tigers_fury_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "tigers_fury", p, p -> find_specialization_spell( "Tiger's Fury" ), options_str )
  {
    harmful = false;
    special = false;
  }

  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.tigers_fury -> trigger();

    p() -> resource_gain( RESOURCE_ENERGY,
                          data().effectN( 2 ).resource( RESOURCE_ENERGY ),
                          p() -> gain.tigers_fury );

    if ( p() -> sets.has_set_bonus( SET_T13_4PC_MELEE ) )
      p() -> buff.omen_of_clarity -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );

    if ( p() -> sets.has_set_bonus( SET_T15_4PC_MELEE ) )
      p() -> buff.tier15_4pc_melee -> trigger( 3 );

    if ( p() -> sets.has_set_bonus( SET_T16_4PC_MELEE ) )
      p() -> buff.feral_rage -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buff.berserk -> check() )
      return false;

    return cat_attack_t::ready();
  }
};

} // end namespace cat_attacks

namespace bear_attacks {

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct bear_attack_t : public druid_attack_t<melee_attack_t>
{
  bear_attack_t( const std::string& token, druid_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( token, p, s )
  {
    parse_options( 0, options );
  }

  bear_attack_t( druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( "", p, s )
  {
    parse_options( 0, options );
  }

  virtual void impact( action_state_t* s )
  {
    base_t::impact( s );

    if ( s -> result == RESULT_CRIT )
      trigger_lotp( s );
  }

  void trigger_rage_gain()
  {
    double rage = 10.85;

    p() -> resource_gain( RESOURCE_RAGE, rage, p() -> gain.bear_melee );
  }
}; // end druid_bear_attack_t

// Bear Melee Attack ========================================================

struct bear_melee_t : public bear_attack_t
{
  bear_melee_t( druid_t* player ) :
    bear_attack_t( "bear_melee", player )
  {
    school      = SCHOOL_PHYSICAL;
    may_glance  = true;
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    special     = false;
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return bear_attack_t::execute_time();
  }

  virtual void impact( action_state_t* state )
  {
    bear_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rage_gain();
      if ( rng().roll( p() -> spec.tooth_and_claw -> proc_chance() ) )
      {
        p() -> buff.tooth_and_claw -> trigger();
        p() -> proc.tooth_and_claw -> occur();
      }
    }
    
    if ( p() -> spell.primal_fury -> ok() && state -> result == RESULT_CRIT )
    {
      p() -> resource_gain( RESOURCE_RAGE,
                            p() -> find_spell( 16959 ) -> effectN( 1 ).resource( RESOURCE_RAGE ),
                            p() -> gain.primal_fury );
      p() -> proc.primal_fury -> occur();
    }
  }
};

// Feral Charge (Bear) ======================================================

struct feral_charge_bear_t : public bear_attack_t
{
  // TODO: Get beta, figure it out
  feral_charge_bear_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( p, p -> talent.wild_charge, options_str )
  {
    may_miss   = false;
    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
    special    = false;
  }

  virtual bool ready()
  {
    bool ranged = ( player -> position() == POSITION_RANGED_FRONT ||
                    player -> position() == POSITION_RANGED_BACK );

    if ( player -> in_combat && ! ranged )
      return false;

    return bear_attack_t::ready();
  }
};

// Lacerate =================================================================

struct lacerate_t : public bear_attack_t
{
  lacerate_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( p, p -> find_class_spell( "Lacerate" ), options_str )
  {
    attack_power_mod.direct     = 0.616;
    attack_power_mod.tick       = 0.0512;
    dot_behavior         = DOT_REFRESH;
    special              = true;
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( p() -> buff.son_of_ursoc -> check() )
      cooldown -> reset( false );
  }

  virtual void impact( action_state_t* state )
  {
    if ( result_is_hit( state -> result ) )
    {
      if ( td( state -> target ) -> lacerate_stack < 3 )
        td( state -> target ) -> lacerate_stack++;
      p() -> buff.lacerate -> trigger();

      if ( rng().roll( 0.25 ) )
        p() -> cooldown.mangle -> reset( true );
    }

    bear_attack_t::impact( state );
  }

  virtual double composite_target_ta_multiplier( player_t* t ) const
  {
    double tm = bear_attack_t::composite_target_ta_multiplier( t );

    tm *= td( t ) -> lacerate_stack;

    return tm;
  }

  virtual void last_tick( dot_t* d )
  {
    bear_attack_t::last_tick( d );

    p() -> buff.lacerate -> expire();
    td( target ) -> lacerate_stack = 0;
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Mangle ============================================================

struct mangle_t : public bear_attack_t
{
  mangle_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "mangle", player, player -> find_spell( 33878 ), options_str )
  {}

  virtual void execute()
  {
    if ( p() -> buff.berserk -> up() )
      aoe = p() -> spell.berserk_bear -> effectN( 1 ).base_value();

    bear_attack_t::execute();

    aoe     = 0;
    special = true;
    if ( p() -> buff.berserk -> check() || p() -> buff.son_of_ursoc -> check() )
      cooldown -> reset( false );
  }

  virtual void impact( action_state_t* state )
  {
    bear_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      double rage = data().effectN( 3 ).resource( RESOURCE_RAGE );

      if ( p() -> talent.soul_of_the_forest -> ok() )
        rage *= 1.0 + p() -> talent.soul_of_the_forest -> effectN( 5 ).percent();

      p() -> resource_gain( RESOURCE_RAGE, rage, p() -> gain.mangle );
    }

    if ( state -> result == RESULT_CRIT )
    {
      p() -> resource_gain( RESOURCE_RAGE, p() -> spell.primal_fury -> effectN( 1 ).resource( RESOURCE_RAGE ), p() -> gain.primal_fury );
      p() -> proc.primal_fury -> occur();
    }
  }

  virtual double action_da_multiplier() const
  {
    double adm = bear_attack_t::action_da_multiplier();

    if ( p() -> talent.soul_of_the_forest -> ok() )
      adm *= 1.0 + p() -> talent.soul_of_the_forest -> effectN( 6 ).percent();

    return adm;
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Maul =====================================================================

struct maul_t : public bear_attack_t
{
  tooth_and_claw_t* absorb;
  maul_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( player, player -> find_class_spell( "Maul" ), options_str )
  {
    weapon = &( player -> main_hand_weapon );

    aoe = player -> glyph.maul -> effectN( 1 ).base_value();
    base_add_multiplier = player -> glyph.maul -> effectN( 3 ).percent();
    use_off_gcd = true;
    special     = true;

    if ( p() -> spec.tooth_and_claw -> ok() )
      absorb = new tooth_and_claw_t( player );
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( p() -> buff.son_of_ursoc -> check() )
      cooldown -> reset( false );
  }

  virtual void impact( action_state_t* s )
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> buff.tooth_and_claw -> up() )
      absorb -> execute();
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = bear_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + p() -> spell.swipe -> effectN( 2 ).percent();

    return tm;
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Skull Bash (Bear) ========================================================

struct skull_bash_bear_t : public bear_attack_t
{
  skull_bash_bear_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( player, player -> find_class_spell( "Skull Bash" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    special = false;
    use_off_gcd = true;

    cooldown -> duration += player -> glyph.skull_bash -> effectN( 1 ).time_value();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Thrash (Bear) ============================================================

struct thrash_bear_t : public bear_attack_t
{
  thrash_bear_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "thrash_bear", player, player -> find_spell( 77758 ), options_str )
  {
    aoe               = -1;
    attack_power_mod.direct  = data().effectN( 3 ).base_value() / 1000.0;
    attack_power_mod.tick    = data().effectN( 4 ).base_value() / 1000.0;

    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    dot_behavior      = DOT_REFRESH;
    special           = true;
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( p() -> buff.son_of_ursoc -> check() )
      cooldown -> reset( false );
  }

  virtual void impact( action_state_t* state )
  {
    bear_attack_t::impact( state );

    if ( rng().roll( 0.25 ) )
      p() -> cooldown.mangle -> reset( true );
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const
  { return 0.0; }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Savage Defense ===========================================================

struct savage_defense_t : public bear_attack_t
{
  savage_defense_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "savage_defense", player, player -> find_class_spell( "Savage Defense" ), options_str )
  {
    parse_options( NULL, options_str );
    harmful = false;
    special = false;
    cooldown -> duration = timespan_t::from_seconds( 9.0 );
    cooldown -> charges = 3;
    use_off_gcd = true;

    if ( player -> sets.has_set_bonus( SET_T16_2PC_TANK ) )
      player -> active.ursocs_vigor = new ursocs_vigor_t( player );
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( p() -> buff.savage_defense -> check() )
      p() -> buff.savage_defense -> extend_duration( p(), timespan_t::from_seconds( 6.0 ) );
    else
      p() -> buff.savage_defense -> trigger();

    if ( p() -> sets.has_set_bonus( SET_T16_4PC_TANK ) )
      p() -> active.ursocs_vigor -> trigger_hot();
  }
};

// Might of Ursoc ===========================================================

struct might_of_ursoc_t : public bear_attack_t
{
  might_of_ursoc_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "might_of_ursoc", player, player -> find_class_spell( "Might of Ursoc" ), options_str )
  {
    parse_options( NULL, options_str );
    cooldown -> duration = data().cooldown();
    cooldown -> duration += player -> glyph.might_of_ursoc -> effectN( 2 ).time_value();
    harmful = false;
    special = false;
    use_off_gcd = true;
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( ! p() -> buff.bear_form -> check() )
      p() -> buff.bear_form -> start();
    p() -> buff.might_of_ursoc -> trigger();
  }
};

} // end namespace bear_attacks

// Druid "Spell" Base for druid_spell_t, druid_heal_t ( and potentially druid_absorb_t )
template <class Base>
struct druid_spell_base_t : public druid_action_t< Base >
{
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_spell_base_t base_t;

  bool consume_ooc;

  druid_spell_base_t( const std::string& n, druid_t* player,
                      const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    consume_ooc( false )
  {
  }

  virtual void consume_resource()
  {
    ab::consume_resource();
    druid_t& p = *this -> p();

    if ( consume_ooc && p.buff.omen_of_clarity -> up() && this -> execute_time() != timespan_t::zero() )
    {
      // Treat the savings like a mana gain.
      double amount = ab::cost();
      if ( amount > 0 )
      {
        p.gain.omen_of_clarity -> add( RESOURCE_MANA, amount );
        p.buff.omen_of_clarity -> expire();
      }
    }
  }

  virtual double cost() const
  {
    if ( consume_ooc && this -> p() -> buff.omen_of_clarity -> check() && this -> execute_time() != timespan_t::zero() )
      return 0;

    return std::max( 0.0, ab::cost() * ( 1.0 + cost_reduction() ) );
  }

  virtual double cost_reduction() const
  { return 0.0; }

  virtual double composite_haste() const
  {
    double h = ab::composite_haste();

    h *= 1.0 / ( 1.0 +  this -> p() -> buff.natures_grace -> data().effectN( 1 ).percent() );

    return h;
  }

  virtual void tick( dot_t* d )
  {
    ab::tick( d );

    if ( this -> p() -> buff.natures_vigil -> check() )
      this -> p() -> active.natures_vigil -> trigger( *this, d -> current_action -> harmful );
  }

  virtual void execute()
  {
    ab::execute();

    if ( this -> p() -> buff.natures_vigil -> check() && ab::result_is_hit( ab::execute_state -> result ) )
      this -> p() -> active.natures_vigil -> trigger( *this );
  }
};

namespace heals {

// ==========================================================================
// Druid Heal
// ==========================================================================

struct druid_heal_t : public druid_spell_base_t<heal_t>
{
  action_t* living_seed;

  druid_heal_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s ),
    living_seed( nullptr )
  {
    parse_options( 0, options );

    dot_behavior      = DOT_REFRESH;
    may_miss          = false;
    weapon_multiplier = 0;
    harmful           = false;
  }
    
protected:
  void init_living_seed();

public:
  virtual double cost() const
  {
    if ( p() -> buff.heart_of_the_wild -> heals_are_free() && current_resource() == RESOURCE_MANA )
      return 0;

    return base_t::cost();
  }

  virtual void execute()
  {
    base_t::execute();

    if ( base_execute_time > timespan_t::zero() )
    {
      p() -> buff.soul_of_the_forest -> expire();

      if ( p() -> buff.natures_swiftness -> check() )
      {
        p() -> buff.natures_swiftness -> expire();
        // NS cd starts when the buff is consumed.
        p() -> cooldown.natures_swiftness -> start();
      }
    }

    if ( base_dd_min > 0 && ! background )
      p() -> buff.harmony -> trigger( 1, p() -> mastery.harmony -> ok() ? p() -> cache.mastery_value() : 0.0 );
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buff.natures_swiftness -> check() )
      return timespan_t::zero();

    return base_t::execute_time();
  }

  virtual double composite_haste() const
  {
    double h = base_t::composite_haste();

    h *= 1.0 / ( 1.0 + p() -> buff.soul_of_the_forest -> value() );

    return h;
  }

  virtual double action_da_multiplier() const
  {
    double adm = base_t::action_da_multiplier();

    if ( p() -> buff.tree_of_life -> up() )
      adm *= 1.0 + p() -> buff.tree_of_life -> data().effectN( 1 ).percent();

    if ( p() -> buff.natures_swiftness -> check() && base_execute_time > timespan_t::zero() )
      adm *= 1.0 + p() -> buff.natures_swiftness -> data().effectN( 2 ).percent();

    if ( p() -> mastery.harmony -> ok() )
      adm *= 1.0 + p() -> cache.mastery_value();

    return adm;
  }

  virtual double action_ta_multiplier() const
  {
    double adm = base_t::action_ta_multiplier();

    if ( p() -> buff.tree_of_life -> up() )
      adm += p() -> buff.tree_of_life -> data().effectN( 2 ).percent();

    if ( p() -> buff.natures_swiftness -> check() && base_execute_time > timespan_t::zero() )
      adm += p() -> buff.natures_swiftness -> data().effectN( 3 ).percent();

    adm += p() -> buff.harmony -> value();

    return adm;
  }

  void trigger_lifebloom_refresh( action_state_t* s )
  {
    druid_td_t& td = *this -> td( s -> target );

    if ( td.dots.lifebloom -> ticking )
    {
      td.dots.lifebloom -> refresh_duration();

      if ( td.buffs.lifebloom -> check() )
        td.buffs.lifebloom -> refresh();
    }
  }

  void trigger_living_seed( action_state_t* s )
  {
    // Technically this should be a buff on the target, then bloom when they're attacked
    // For simplicity we're going to assume it always heals the target
    if ( living_seed )
    {
      living_seed -> base_dd_min = s -> result_amount;
      living_seed -> base_dd_max = s -> result_amount;
      living_seed -> execute();
    }
  }
}; // end druid_heal_t

struct living_seed_t : public druid_heal_t
{
  living_seed_t( druid_t* player ) :
    druid_heal_t( "living_seed", player, player -> find_specialization_spell( "Living Seed" ) )
  {
    background = true;
    may_crit   = false;
    proc       = true;
    school     = SCHOOL_NATURE;
  }

  double composite_da_multiplier() const
  {
    return data().effectN( 1 ).percent();
  }
};

void druid_heal_t::init_living_seed()
{
  if ( p() -> specialization() == DRUID_RESTORATION )
    living_seed = new living_seed_t( p() );
}

// Frenzied Regeneration ====================================================

struct frenzied_regeneration_t : public druid_heal_t
{
  double maximum_rage_cost;

  frenzied_regeneration_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "frenzied_regeneration", p, p -> find_class_spell( "Frenzied Regeneration" ), options_str ),
    maximum_rage_cost( 0.0 )
  {
    base_dd_min = base_dd_max = attack_power_mod.direct = spell_power_mod.direct = 0.0;

    special = false;
    use_off_gcd = true;

    if ( p -> glyph.frenzied_regeneration -> ok() )
      base_costs[ RESOURCE_RAGE ] = p -> glyph.frenzied_regeneration -> effectN( 3 ).resource( RESOURCE_RAGE );
    else
      base_costs[ RESOURCE_RAGE ] = 0;

    if ( p -> sets.has_set_bonus( SET_T16_2PC_TANK ) )
      p -> active.ursocs_vigor = new ursocs_vigor_t( p );

    maximum_rage_cost = data().effectN( 1 ).base_value();
  }

  virtual double cost() const
  {
    if ( ! p() -> glyph.frenzied_regeneration -> ok() )
      const_cast<frenzied_regeneration_t*>(this) -> base_costs[ RESOURCE_RAGE ] = std::min( p() -> resources.current[ RESOURCE_RAGE ],
                                              maximum_rage_cost );

    return druid_heal_t::cost();
  }

  virtual double base_da_min( const action_state_t* ) const
  {
      if ( p() -> glyph.frenzied_regeneration -> ok() )
        return 0.0;

      // max(2.2*(AP - 2*Agi), 2.5*Sta)
      double ap      = p() -> composite_melee_attack_power() * p() -> composite_attack_power_multiplier();
      double agility = p() -> composite_attribute( ATTR_AGILITY ) * p() -> composite_attribute_multiplier( ATTR_AGILITY );
      double stamina = p() -> composite_attribute( ATTR_STAMINA ) * p() -> composite_attribute_multiplier( ATTR_STAMINA );
      return std::max( ( ap - 2 * agility ) * data().effectN( 2 ).percent(), stamina * data().effectN( 3 ).percent() )
             * ( resource_consumed / maximum_rage_cost )
             * ( 1.0 + p() -> buff.tier15_2pc_tank -> stack() * p() -> buff.tier15_2pc_tank -> data().effectN( 1 ).percent() );
  }

  virtual double base_da_max( const action_state_t* ) const
  {
      if ( p() -> glyph.frenzied_regeneration -> ok() )
        return 0.0;

      // max(2.2*(AP - 2*Agi), 2.5*Sta)
      double ap      = p() -> composite_melee_attack_power() * p() -> composite_attack_power_multiplier();
      double agility = p() -> composite_attribute( ATTR_AGILITY ) * p() -> composite_attribute_multiplier( ATTR_AGILITY );
      double stamina = p() -> composite_attribute( ATTR_STAMINA ) * p() -> composite_attribute_multiplier( ATTR_STAMINA );
      return std::max( ( ap - 2 * agility ) * data().effectN( 2 ).percent(), stamina * data().effectN( 3 ).percent() )
             * ( resource_consumed / maximum_rage_cost )
             * ( 1.0 + p() -> buff.tier15_2pc_tank -> stack() * p() -> buff.tier15_2pc_tank -> data().effectN( 1 ).percent() );
  }

  virtual void execute()
  {
    if ( p() -> glyph.frenzied_regeneration -> ok() )
      p() -> buff.frenzied_regeneration -> trigger();
    else
      p() -> buff.tier15_2pc_tank -> expire();

    druid_heal_t::execute();

    if ( p() -> sets.has_set_bonus( SET_T16_4PC_TANK ) )
      p() -> active.ursocs_vigor -> trigger_hot( resource_consumed );
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    if ( p() -> resources.current[ RESOURCE_RAGE ] < 1 )
      return false;

    return druid_heal_t::ready();
  }
};

// Healing Touch ============================================================

struct healing_touch_t : public druid_heal_t
{
  healing_touch_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "healing_touch", p, p -> find_class_spell( "Healing Touch" ), options_str )
  {
    consume_ooc      = true;
    base_multiplier *= 1.0 + p -> perk.improved_healing_touch -> effectN( 1 ).percent();

    init_living_seed();
  }

  double spell_direct_power_coefficient( const action_state_t* /* state */ ) const
  {
    if ( p() -> specialization() == DRUID_GUARDIAN && p() -> buff.dream_of_cenarius -> check() )
      return 0.0;
    return data().effectN( 1 ).coeff();
  }

  double attack_direct_power_coefficient( const action_state_t* /* state */ ) const
  {
    if ( p() -> specialization() == DRUID_GUARDIAN && p() -> buff.dream_of_cenarius -> check() )
      return data().effectN( 1 ).coeff();
    return 0.0;
  }

  virtual double cost() const
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return 0;

    if ( p() -> buff.natures_swiftness -> check() )
      return 0;

    return druid_heal_t::cost();
  }

  virtual double action_da_multiplier() const
  {
    double adm = base_t::action_da_multiplier();

    if ( p() -> talent.dream_of_cenarius -> ok() ) {
      if ( p() -> specialization() == DRUID_FERAL || p() -> specialization() == DRUID_BALANCE )
        adm *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 1 ).percent();
      else if ( p() -> specialization() == DRUID_GUARDIAN )
        adm *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 2 ).percent();
    }

    return adm;
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buff.predatory_swiftness -> check() || p() -> buff.dream_of_cenarius -> up() )
      return timespan_t::zero();

    return druid_heal_t::execute_time();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );

    if ( ! p() -> glyph.blooming -> ok() )
      trigger_lifebloom_refresh( state );

    if ( state -> result == RESULT_CRIT )
      trigger_living_seed( state );

    p() -> cooldown.natures_swiftness -> adjust( timespan_t::from_seconds( - p() -> glyph.healing_touch -> effectN( 1 ).base_value() ) );
  }

  virtual void execute()
  {
    druid_heal_t::execute();

    /* FIXME: Dream of Cenarius buff states that the cooldown is reset,
       talent states that the next cast does not trigger cooldown.
       Sticking to what the buff states for now. */
    if ( p() -> buff.dream_of_cenarius -> up() )
      p() -> cooldown.starsurge -> reset( false );

    p() -> buff.predatory_swiftness -> expire();
    p() -> buff.dream_of_cenarius -> expire();
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_heal_t::schedule_execute( state );

    if ( ! p() -> buff.natures_swiftness -> up() &&
         ! p() -> buff.predatory_swiftness -> up() &&
         ! ( p() -> buff.dream_of_cenarius -> check() && p() -> specialization() == DRUID_GUARDIAN ) )
    {
      p() -> buff.cat_form         -> expire();
      p() -> buff.bear_form        -> expire();
    }
  }

  virtual timespan_t gcd() const
  {
    const druid_t& p = *this -> p();
    if ( p.buff.cat_form -> check() )
      if ( timespan_t::from_seconds( 1.0 ) < druid_heal_t::gcd() )
        return timespan_t::from_seconds( 1.0 );

    return druid_heal_t::gcd();
  }
};

// Lifebloom ================================================================

struct lifebloom_bloom_t : public druid_heal_t
{
  lifebloom_bloom_t( druid_t* p ) :
    druid_heal_t( "lifebloom_bloom", p, p -> find_class_spell( "Lifebloom" ) )
  {
    background       = true;
    dual             = true;
    num_ticks        = 0;
    base_td          = 0;
    attack_power_mod.tick   = 0;
    base_dd_min      = data().effectN( 2 ).min( p );
    base_dd_max      = data().effectN( 2 ).max( p );
    attack_power_mod.direct = data().effectN( 2 ).coeff();
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double ctm = druid_heal_t::composite_target_multiplier( target );

    ctm *= 1.0 + td( target ) -> buffs.lifebloom -> check();

    return ctm;
  }

  virtual double composite_da_multiplier() const
  {
    double cdm = druid_heal_t::composite_da_multiplier();

    cdm *= 1.0 + p() -> glyph.blooming -> effectN( 1 ).percent();

    return cdm;
  }
};

struct lifebloom_t : public druid_heal_t
{
  lifebloom_bloom_t* bloom;

  lifebloom_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "lifebloom", p, p -> find_class_spell( "Lifebloom" ), options_str ),
    bloom( new lifebloom_bloom_t( p ) )
  {
    may_crit   = false;

    // TODO: this can be only cast on one target, unless Tree of Life is up
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double ctm = druid_heal_t::composite_target_multiplier( target );

    ctm *= 1.0 + td( target ) -> buffs.lifebloom -> check();

    return ctm;
  }

  virtual void impact( action_state_t* state )
  {
    // Cancel Dot/td-buff on all targets other than the one we impact on
    for ( size_t i = 0; i < sim -> actor_list.size(); ++i )
    {
      player_t* t = sim -> actor_list[ i ];
      if ( state -> target == t )
        continue;
      get_dot( t ) -> cancel();
      td( t ) -> buffs.lifebloom -> expire();
    }

    druid_heal_t::impact( state );

    td( state -> target ) -> buffs.lifebloom -> trigger();
  }

  virtual void last_tick( dot_t* d )
  {
    if ( ! d -> state -> target -> is_sleeping() ) // Prevent crash at end of simulation
      bloom -> execute();
    td( d -> state -> target ) -> buffs.lifebloom -> expire();

    druid_heal_t::last_tick( d );
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );

    p() -> buff.omen_of_clarity -> trigger();
  }
};

// Regrowth =================================================================

struct regrowth_t : public druid_heal_t
{
  regrowth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "regrowth", p, p -> find_class_spell( "Regrowth" ), options_str )
  {
    base_crit   += 0.6;
    consume_ooc  = true;

    if ( p -> glyph.regrowth -> ok() )
    {
      base_crit += p -> glyph.regrowth -> effectN( 1 ).percent();
      base_td    = 0;
      num_ticks  = 0;
    }

    init_living_seed();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );

    if ( ! p() -> glyph.blooming -> ok() )
      trigger_lifebloom_refresh( state );

    if ( state -> result == RESULT_CRIT )
      trigger_living_seed( state );
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );

    if ( d -> state -> target -> health_percentage() <= p() -> spell.regrowth -> effectN( 1 ).percent() &&
         td( d -> state -> target ) -> dots.regrowth -> ticking )
    {
      td( d -> state -> target )-> dots.regrowth -> refresh_duration();
    }
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buff.tree_of_life -> check() )
      return timespan_t::zero();

    return druid_heal_t::execute_time();
  }
};

// Rejuvenation =============================================================

struct rejuvenation_t : public druid_heal_t
{
  rejuvenation_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "rejuvenation", p, p -> find_class_spell( "Rejuvenation" ), options_str )
  {
    tick_zero = true;
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_heal_t::schedule_execute( state );

    if ( ! p() -> perk.enhanced_rejuvenation -> ok() )
      p() -> buff.cat_form  -> expire();
    if ( ! p() -> buff.heart_of_the_wild -> check() )
      p() -> buff.bear_form -> expire();
  }

  virtual double action_ta_multiplier() const
  {
    double atm = base_t::action_ta_multiplier();

    if ( p() -> talent.dream_of_cenarius -> ok() && p() -> specialization() == DRUID_FERAL )
        atm *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 2 ).percent();

    return atm;
  }

};

// Renewal ============================================================

struct renewal_t : public druid_heal_t
{
  renewal_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "renewal", p, p -> find_spell( 108238 ), options_str )
  {}

  virtual void execute()
  {
    base_dd_min = base_dd_max = p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();

    druid_heal_t::execute();
  }
};

// Swiftmend ================================================================

// TODO: in game, you can swiftmend other druids' hots, which is not supported here
struct swiftmend_t : public druid_heal_t
{
  struct swiftmend_aoe_heal_t : public druid_heal_t
  {
    swiftmend_aoe_heal_t( druid_t* p, const spell_data_t* s ) :
      druid_heal_t( "swiftmend_aoe", p, s )
    {
      aoe            = 3;
      background     = true;
      base_tick_time = timespan_t::from_seconds( 1.0 );
      hasted_ticks   = true;
      may_crit       = false;
      num_ticks      = ( int ) p -> spell.swiftmend -> duration().total_seconds();
      proc           = true;
      tick_may_crit  = false;
    }
  };

  swiftmend_aoe_heal_t* aoe_heal;

  swiftmend_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "swiftmend", p, p -> find_class_spell( "Swiftmend" ), options_str ),
    aoe_heal( new swiftmend_aoe_heal_t( p, &data() ) )
  {
    consume_ooc = true;

    init_living_seed();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );

    if ( state -> result == RESULT_CRIT )
      trigger_living_seed( state );

    if ( p() -> talent.soul_of_the_forest -> ok() )
      p() -> buff.soul_of_the_forest -> trigger();

    aoe_heal -> execute();
  }

  virtual bool ready()
  {
    player_t* t = ( execute_state ) ? execute_state -> target : target;

    // Note: with the glyph you can use other people's regrowth/rejuv
    if ( ! ( td( t ) -> dots.regrowth -> ticking ||
             td( t ) -> dots.rejuvenation -> ticking ) )
      return false;

    return druid_heal_t::ready();
  }
};

// Tranquility ==============================================================

struct tranquility_t : public druid_heal_t
{
  tranquility_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "tranquility", p, p -> find_class_spell( "Tranquility" ), options_str )
  {
    aoe               = data().effectN( 3 ).base_value(); // Heals 5 targets
    base_execute_time = data().duration();
    channeled         = true;

    // Healing is in spell effect 1
    parse_spell_data( ( *player -> dbc.spell( data().effectN( 1 ).trigger_spell_id() ) ) );

    // FIXME: The hot should stack
  }
};

// Wild Growth ==============================================================

struct wild_growth_t : public druid_heal_t
{
  wild_growth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "wild_growth", p, p -> find_class_spell( "Wild Growth" ), options_str )
  {
    aoe = data().effectN( 3 ).base_value() + p -> glyph.wild_growth -> effectN( 1 ).base_value();
    cooldown -> duration = data().cooldown() + p -> glyph.wild_growth -> effectN( 2 ).time_value();
  }

  virtual void execute()
  {
    int save = aoe;
    if ( p() -> buff.tree_of_life -> check() )
      aoe += 2;

    druid_heal_t::execute();

    // Reset AoE
    aoe = save;
  }
};

} // end namespace heals

namespace spells {

// ==========================================================================
// Druid Spells
// ==========================================================================

struct druid_spell_t : public druid_spell_base_t<spell_t>
{
  druid_spell_t( const std::string& token, druid_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( token, p, s )
  {
    parse_options( 0, options );
  }

  virtual void init()
  {
    base_t::init();
    consume_ooc = harmful;
  }

  virtual double cost() const
  {
    if ( harmful && p() -> buff.heart_of_the_wild -> damage_spells_are_free() )
      return 0;

    return base_t::cost();
  }

  void trigger_eclipse_energy_gain( int gain )
  {
    if ( gain == 0 )
      return;

    if ( p() -> buff.celestial_alignment -> check() )
      return;

    // Gain will only happen if it is either aligned with the bar direction or
    // the bar direction has not been set yet.
    if ( p() -> eclipse_bar_direction == -1 && gain > 0 )
    {
      p() -> proc.unaligned_eclipse_gain -> occur();
      return;
    }
    else if ( p() -> eclipse_bar_direction ==  1 && gain < 0 )
    {
      p() -> proc.unaligned_eclipse_gain -> occur();
      return;
    }

    int old_eclipse_bar_value = p() -> eclipse_bar_value;
    p() -> eclipse_bar_value += gain;

    // When a Lunar Eclipse ends you gain 20 Solar Energy and when a Solar Eclipse
    // ends you gain 20 Lunar Energy.
    // bool soul_of_the_forest = false;
    if ( p() -> eclipse_bar_value <= 0 )
    {
      if ( p() -> buff.eclipse_solar -> check() )
      {
        p() -> buff.eclipse_solar -> expire();
        // soul_of_the_forest = true;
      }
      if ( p() -> eclipse_bar_value < p() -> spec.eclipse -> effectN( 2 ).base_value() )
        p() -> eclipse_bar_value = p() -> spec.eclipse -> effectN( 2 ).base_value();
    }

    if ( p() -> eclipse_bar_value >= 0 )
    {
      if ( p() -> buff.eclipse_lunar -> check() )
      {
        p() -> buff.eclipse_lunar -> expire();
        // soul_of_the_forest = true;
      }
      if ( p() -> eclipse_bar_value > p() -> spec.eclipse -> effectN( 1 ).base_value() )
        p() -> eclipse_bar_value = p() -> spec.eclipse -> effectN( 1 ).base_value();
    }

    int actual_gain = p() -> eclipse_bar_value - old_eclipse_bar_value;
    if ( p() -> sim -> log )
    {
      p() -> sim -> out_log.printf( "%s gains %d (%d) %s from %s (%d)",
                            p() -> name(), actual_gain, gain,
                            "Eclipse", name(),
                            p() -> eclipse_bar_value );
    }

    // Eclipse proc:
    // Procs when you reach 100 and only then, you have to have an
    // actual gain of eclipse energy (bar value)
    if ( actual_gain != 0 )
    {
      if ( p() -> eclipse_bar_value ==
           p() -> spec.eclipse -> effectN( 1 ).base_value() )
      {
        if ( p() -> buff.eclipse_solar -> trigger() )
        {
          trigger_eclipse_proc();
          // Solar proc => bar direction changes to -1 (towards Lunar)
          p() -> eclipse_bar_direction = -1;
        }
      }
      else if ( p() -> eclipse_bar_value ==
                p() -> spec.eclipse -> effectN( 2 ).base_value() )
      {
        if ( p() -> buff.eclipse_lunar -> trigger() )
        {
          trigger_eclipse_proc();
          p() -> cooldown.starfall -> reset( false );
          // Lunar proc => bar direction changes to +1 (towards Solar)
          p() -> eclipse_bar_direction = 1;
        }
      }
    }
  }

  void trigger_eclipse_proc()
  {
    // All extra procs when eclipse pops
    p() -> resource_gain( RESOURCE_MANA,
                          p() -> resources.max[ RESOURCE_MANA ] * p() -> spell.eclipse -> effectN( 1 ).resource( RESOURCE_MANA ),
                          p() -> gain.eclipse );

    p() -> buff.natures_grace -> trigger();
  }

  void test_to_extend( action_state_t* s, dot_t* dot )
  {
    if ( s -> result == RESULT_CRIT
         && p() -> spec.eclipse -> ok()
         && dot -> ticking )

      dot -> extend_duration_seconds( timespan_t::from_seconds( 2 ), STATE_HASTE );
  }

  void trigger_t16_2pc_balance( bool nature = false )
  {
    if ( ! p() -> sets.has_set_bonus( SET_T16_2PC_CASTER ) )
      return;
    if ( nature )
    {
      if ( p() -> buff.eclipse_solar -> check() )
      {
        p() -> t16_2pc_sun_bolt -> execute();
      }
    }
    else
    {
      if ( p() -> buff.eclipse_lunar -> check() )
      {
        p() -> t16_2pc_starfall_bolt -> execute();
      }
    }
  }
}; // end druid_spell_t

// Auto Attack ==============================================================

struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( druid_t* player, const std::string& options_str ) :
    melee_attack_t( "auto_attack", player, spell_data_t::nil() )
  {
    parse_options( 0, options_str );

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    player -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
    player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;
    player -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( player -> is_moving() )
      return false;

    if ( ! player -> main_hand_attack )
      return false;

    return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Astral Communion =========================================================

struct astral_communion_t : public druid_spell_t
{
  int starting_direction;

  astral_communion_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "astral_communion", player, player -> find_class_spell( "Astral Communion" ), options_str ),
    starting_direction( 0 )
  {
    harmful      = false;
    hasted_ticks = false;
    channeled    = true;
    may_miss     = false;
  }

  virtual double composite_haste() const
  { return 1.0; }

  virtual void execute()
  {
    if ( p() -> eclipse_bar_direction == 0 )
      starting_direction = 1;
    else
      starting_direction = p() -> eclipse_bar_direction;

    int tmp_ticks = num_ticks;
    if ( p() -> buff.astral_insight -> up() )
    {
      num_ticks = 0;
      p() -> buff.astral_insight -> expire();
      trigger_eclipse_energy_gain( p() -> buff.astral_insight -> data().effectN( 1 ).base_value() * starting_direction );
      druid_spell_t::execute();
      num_ticks = tmp_ticks;
    }
    else
    {
      druid_spell_t::execute();
    }
  }

  virtual void tick( dot_t* /* d */ )
  {
    trigger_eclipse_energy_gain( data().effectN( 1 ).base_value() * starting_direction );
    // This should still call druid_spell_t::tick( d ); no?
  }
};

// Astral Storm =============================================================

struct astral_storm_tick_t : public druid_spell_t
{
  astral_storm_tick_t( druid_t* player, const spell_data_t* s  ) :
    druid_spell_t( "astral_storm", player, s )
  {
    background = true;
    aoe = -1;
  }
};

struct astral_storm_t : public druid_spell_t
{
  astral_storm_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "astral_storm", player, player -> find_class_spell( "Astral Storm" ) )
  {
    parse_options( NULL, options_str );
    channeled   = true;

    tick_action = new astral_storm_tick_t( player, data().effectN( 3 ).trigger() );
    dynamic_tick_action = true;
  }

  virtual bool ready()
  {
    if ( p() -> eclipse_bar_direction != 1 )
      return false;

    return druid_spell_t::ready();
  }
};

// Barkskin =================================================================

struct barkskin_t : public druid_spell_t
{
  barkskin_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "barkskin", player, player -> find_class_spell( "Barkskin" ), options_str )
  {
    harmful = false;
    use_off_gcd = true;

    if ( player -> spec.thick_hide )
      cooldown -> duration += player -> spec.thick_hide -> effectN( 6 ).time_value();
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.barkskin -> trigger();
  }
};

// Bear Form Spell ==========================================================

struct bear_form_t : public druid_spell_t
{
  bear_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bear_form", player, player -> find_class_spell( "Bear Form" ), options_str )
  {
    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );

    if ( ! player -> bear_melee_attack )
    {
      player -> init_beast_weapon( player -> bear_weapon, 2.5 );
      player -> bear_melee_attack = new bear_attacks::bear_melee_t( player );
    }
  }

  virtual void execute()
  {
    spell_t::execute();

    p() -> buff.bear_form -> start();
  }

  virtual bool ready()
  {
    if ( p() -> buff.bear_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Berserk ==================================================================

struct berserk_t : public druid_spell_t
{
  berserk_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "berserk", player, player -> find_class_spell( "Berserk" ), options_str  )
  {
    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( p() -> buff.bear_form -> check() )
    {
      p() -> buff.berserk -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, p() -> spell.berserk_bear -> duration() );
      p() -> cooldown.mangle -> reset( false );
    }
    else if ( p() -> buff.cat_form -> check() )
      p() -> buff.berserk -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, p() -> spell.berserk_cat -> duration() );
  }
};

// Cat Form Spell ===========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "cat_form", player, player -> find_class_spell( "Cat Form" ), options_str )
  {
    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );

    if ( ! player -> cat_melee_attack )
    {
      player -> init_beast_weapon( player -> cat_weapon, 1.0 );
      player -> cat_melee_attack = new cat_attacks::cat_melee_t( player );
    }
  }

  virtual void execute()
  {
    spell_t::execute();

    p() -> buff.cat_form -> start();
  }

  virtual bool ready()
  {
    if ( p() -> buff.cat_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};


// Celestial Alignment ======================================================

struct celestial_alignment_t : public druid_spell_t
{
  celestial_alignment_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "celestial_alignment", player, player -> find_class_spell( "Celestial Alignment" ), options_str )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    // http://elitistjerks.com/f73/t126893-mists_pandaria_all_specs/p11/#post2136096
    // I can confirm that CA works EXACTLY like combining the two eclipses
    // (starfall reset, dot refresh, SotF gives 20 energy afterwards, you
    // gain 35% mana back as you would from eclipse).
    p() -> buff.celestial_alignment -> trigger();

    // CA consumes ALL current eclipse energy, so just set the bar to 0
    p() -> eclipse_bar_value = 0;

    if ( ! p() -> buff.eclipse_lunar -> check() )
      p() -> buff.eclipse_lunar -> trigger();

    if ( ! p() -> buff.eclipse_solar -> check() )
      p() -> buff.eclipse_solar -> trigger();

    p() -> cooldown.starfall -> reset( false );

    trigger_eclipse_proc();
  }
};

// Cenarion Ward ============================================================

struct cenarion_ward_t : public druid_spell_t
{
  cenarion_ward_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "cenarion_ward", p, p -> find_talent_spell( "Cenarion Ward" ), options_str )
  {
    harmful    = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.cenarion_ward -> trigger();
  }
};

// Faerie Fire Spell ========================================================

struct faerie_fire_t : public druid_spell_t
{
  faerie_fire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "faerie_fire", player, player -> find_class_spell( "Faerie Fire" ) )
  {
    parse_options( NULL, options_str );
    cooldown -> duration = timespan_t::from_seconds( 6.0 );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && ! sim -> overrides.physical_vulnerability )
      target -> debuffs.physical_vulnerability -> trigger();
  }

  virtual void update_ready( timespan_t )
  {
    timespan_t cd = cooldown -> duration;

    if ( ! ( p() -> buff.bear_form -> check() || p() -> buff.cat_form -> check() ) )
      cd = timespan_t::zero();

    druid_spell_t::update_ready( cd );
  }

  virtual double action_multiplier() const
  {
    if ( p() -> buff.bear_form -> check() )
      return druid_spell_t::action_multiplier();
    else
      return 0.0;
  }

  virtual void impact( action_state_t* state )
  {
    druid_spell_t::impact( state );

    if ( p() -> buff.bear_form -> check() )
    {
      if ( result_is_hit( state -> result ) )
        if ( rng().roll( 0.25 ) )
          p() -> cooldown.mangle -> reset( true );
    }
  }

  virtual resource_e current_resource() const
  {
    if ( p() -> buff.bear_form -> check() )
      return RESOURCE_RAGE;
    else if ( p() -> buff.cat_form -> check() )
      return RESOURCE_ENERGY;

    return RESOURCE_MANA;
  }
};

// Heart of the Wild Spell ==================================================

struct heart_of_the_wild_t : public druid_spell_t
{
  heart_of_the_wild_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "heart_of_the_wild", player, player -> talent.heart_of_the_wild )
  {
    parse_options( NULL, options_str );
    harmful = may_hit = may_crit = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    p() -> buff.heart_of_the_wild -> trigger();
  }
};

// Hurricane ================================================================

struct hurricane_tick_t : public druid_spell_t
{
  hurricane_tick_t( druid_t* player, const spell_data_t* s  ) :
    druid_spell_t( "hurricane", player, s )
  {
    background = true;
    aoe = -1;
  }
};

struct hurricane_t : public druid_spell_t
{
  hurricane_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "hurricane", player, player -> find_class_spell( "Hurricane" ) )
  {
    parse_options( NULL, options_str );
    channeled   = true;

    tick_action = new hurricane_tick_t( player, data().effectN( 3 ).trigger() );
    dynamic_tick_action = true;
  }

  virtual bool ready()
  {
    if ( p() -> eclipse_bar_direction == 1 )
      return false;

    return druid_spell_t::ready();
  }

  virtual double action_multiplier() const
  {
    return druid_spell_t::action_multiplier()
           * ( 1.0 + p() -> buff.heart_of_the_wild -> damage_spell_multiplier() );
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_spell_t::schedule_execute( state );

    p() -> buff.cat_form  -> expire();
    p() -> buff.bear_form -> expire();
  }
};

// Incarnation ==============================================================

struct incarnation_t : public druid_spell_t
{
  incarnation_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "incarnation", player, player -> talent.incarnation, options_str )
  {
    parse_options( NULL, options_str );
    harmful = false;
    /*
    Buff Spell: http://mop.wowhead.com/spell=117679

    Balance Passive: http://mop.wowhead.com/spell=122114

    Bear Passive: http://mop.wowhead.com/spell=113711

    Cat Passive: http://mop.wowhead.com/spell=102548
    Seems to replaces 3 spells:
      New Prowl http://mop.wowhead.com/spell=102547
      New Pounce http://mop.wowhead.com/spell=102546
      New Ravage http://mop.wowhead.com/spell=102545

    Resto Passive: http://mop.wowhead.com/spell=5420
    */
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    switch ( p() -> specialization() )
    {
      case DRUID_BALANCE:
        p() -> buff.chosen_of_elune -> trigger();
        break;
      case DRUID_FERAL:
        p() -> buff.king_of_the_jungle -> trigger();
        break;
      case DRUID_GUARDIAN:
        p() -> buff.son_of_ursoc -> trigger();
        break;
      case DRUID_RESTORATION:
        p() -> buff.tree_of_life -> trigger();
        break;
      default: break;
    }

    if ( p() -> buff.bear_form -> check() )
      p() -> cooldown.mangle -> reset( false );
  }
};

// Mark of the Wild Spell ===================================================

struct mark_of_the_wild_t : public druid_spell_t
{
  mark_of_the_wild_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "mark_of_the_wild", player, player -> find_class_spell( "Mark of the Wild" )  )
  {
    parse_options( NULL, options_str );

    trigger_gcd = timespan_t::zero();
    //base_costs[ current_resource() ]  *= 1.0 + p -> glyph.mark_of_the_wild -> mod_additive( P_RESOURCE_COST ) / 100.0;
    harmful     = false;
    background  = ( sim -> overrides.str_agi_int != 0 );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    if ( ! sim -> overrides.str_agi_int )
      sim -> auras.str_agi_int -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, player -> dbc.spell( 79060 ) -> duration() );
  }
};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  // Moonfire also applies the Sunfire DoT during Celestial Alignment.
  struct sunfire_CA_t : public druid_spell_t
  {
    sunfire_CA_t( druid_t* player ) :
      druid_spell_t( "sunfire", player, player -> find_class_spell( "Sunfire" ) )
    {
      assert( player -> specialization() == DRUID_BALANCE );

      dot_behavior = DOT_REFRESH;

      background = true;
      may_crit = false;
      may_miss = true; // Bug?

      num_ticks += ( int ) (  player -> sets.set( SET_T14_4PC_CASTER ) -> effectN( 1 ).time_value() / base_tick_time );

      // Does no direct damage, costs no mana
      attack_power_mod.direct = 0;
      base_dd_min = base_dd_max = 0;
      range::fill( base_costs, 0 );
    }

    virtual void tick( dot_t* d )
    {
      druid_spell_t::tick( d );
      // Todo: Does this sunfire proc SS?
      p() -> trigger_shooting_stars( d -> state -> result );
    }
  };

  action_t* sunfire;

  moonfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonfire", player, player -> find_class_spell( "Moonfire" ) ),
    // player -> talent.lunar_inspiration -> ok() ? player -> find_spell( 155625 ) : player -> find_class_spell( "Moonfire" )
    sunfire( nullptr )
  {
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;

    num_ticks += ( int ) (  player -> sets.set( SET_T14_4PC_CASTER ) -> effectN( 1 ).time_value() / base_tick_time );

    if ( player -> specialization() == DRUID_BALANCE )
      sunfire = new sunfire_CA_t( player );
  }

  virtual double action_da_multiplier() const
  {
    double m = druid_spell_t::action_da_multiplier();

    m *= 1.0 + ( p() -> buff.lunar_shower -> data().effectN( 1 ).percent() * p() -> buff.lunar_shower -> check() );

    return m;
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );
    p() -> trigger_shooting_stars( d -> state -> result );
  }

  virtual void execute()
  {
    p() -> buff.lunar_shower -> up();

    druid_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> spec.lunar_shower -> ok() )
      {
        p() -> buff.lunar_shower -> trigger();
      }
    }

    trigger_t16_2pc_balance( false );
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_spell_t::schedule_execute( state );
    
    p() -> buff.bear_form -> expire();
    if ( ! p() -> talent.lunar_inspiration -> ok() )
      p() -> buff.cat_form -> expire();
  }

  virtual void impact( action_state_t* s )
  {
    // The Sunfire hits BEFORE the moonfire!
    if ( sunfire && result_is_hit( s -> result ) )
    {
      if ( p() -> buff.celestial_alignment -> check() )
      {
        sunfire -> target = s -> target;
        sunfire -> execute();
      }
    }
    druid_spell_t::impact( s );

    if ( p() -> talent.lunar_inspiration -> ok() && result_is_hit( s -> result ) )
      {
        int cp = data().effectN( 3 ).base_value(); // Since this isn't in cat_attack_t, we need to account for the base combo points.
        if ( p() -> spell.primal_fury -> ok() && s -> result == RESULT_CRIT )
          cp += p() -> find_spell( 16953 ) -> effectN( 1 ).base_value();
        td( s -> target ) -> combo_points.add( cp, &name_str );
      }
  }

  virtual double cost_reduction() const
  {
    double cr = druid_spell_t::cost_reduction();

    cr += ( p() -> buff.lunar_shower -> data().effectN( 2 ).percent() * p() -> buff.lunar_shower -> check() );

    return cr;
  }
};

// Moonkin Form Spell =======================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonkin_form", player, player -> find_class_spell( "Moonkin Form" )  )
  {
    parse_options( NULL, options_str );

    harmful           = false;
  }

  virtual void execute()
  {
    spell_t::execute();

    p() -> buff.moonkin_form -> start();
  }

  virtual bool ready()
  {
    if ( p() -> buff.moonkin_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Natures Swiftness Spell ==================================================

struct druids_swiftness_t : public druid_spell_t
{
  druids_swiftness_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "natures_swiftness", player, player -> find_specialization_spell( "Nature's Swiftness" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    if ( ! ( player -> specialization() == DRUID_RESTORATION || player -> specialization() == DRUID_BALANCE ) )
      background = true;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.natures_swiftness -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buff.natures_swiftness -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Nature's Vigil ===========================================================

struct natures_vigil_t : public druid_spell_t
{
  natures_vigil_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "natures_vigil", player, player -> find_spell( 124974 ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    update_ready();
    p() -> buff.natures_vigil -> trigger();
  }
};

// Starfire Spell ===========================================================

struct starfire_t : public druid_spell_t
{
  starfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starfire", player, player -> find_class_spell( "Starfire" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void impact( action_state_t* s )
  {
    druid_spell_t::impact( s );
    test_to_extend( s, td( s -> target ) -> dots.moonfire );
  }

  virtual double action_multiplier() const
  {
    double m = druid_spell_t::action_multiplier();

    m *= 1.0 + p() -> sets.set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();

    return m;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    // Cast starfire, but solar eclipse was up?
    if ( p() -> buff.eclipse_solar -> check() && ! p() -> buff.celestial_alignment -> check() )
      p() -> proc.wrong_eclipse_starfire -> occur();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> spec.eclipse -> ok() )
      {
        if ( ! p() -> buff.eclipse_solar -> check() )
        {
          int gain = data().effectN( 2 ).base_value();

          if ( ! p() -> buff.eclipse_lunar -> check() )
          {
            gain *= 2;
          }
          trigger_eclipse_energy_gain( gain );
        }
      }
    }

    trigger_t16_2pc_balance( false );

    p() -> trigger_soul_of_the_forest();
  }
};

// Starfall Spell ===========================================================

struct starfall_star_t : public druid_spell_t
{
  starfall_star_t( druid_t* player, uint32_t spell_id ) :
    druid_spell_t( "starfall_star", player, player -> find_spell( spell_id ) )
  {
    background  = true;
    direct_tick = true;
    aoe         = 2;
  }
};

struct starfall_t : public druid_spell_t
{
  starfall_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starfall", player, player -> find_class_spell( "Starfall" ) )
  {
    parse_options( NULL, options_str );

    num_ticks      = 10;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

    harmful = false;

    base_multiplier *= 1.0 + player -> sets.set( SET_T14_2PC_CASTER ) -> effectN( 1 ).percent();

    // Starfall triggers a spell each second, that triggers the damage spell.
    const spell_data_t* stars_trigger_spell = data().effectN( 1 ).trigger();
    if ( ! stars_trigger_spell -> ok() )
    {
      background = true;
    }
    dynamic_tick_action = true;
    tick_action = new starfall_star_t( player, stars_trigger_spell -> effectN( 1 ).base_value() );
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    p() -> buff.starfall -> trigger();
  }
};

// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{
  starsurge_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starsurge", player, player -> find_class_spell( "Starsurge" ) )
  {
    parse_options( NULL, options_str );

    cooldown -> duration += player -> sets.set( SET_T13_4PC_CASTER ) -> effectN( 1 ).time_value();
    base_multiplier *= 1.0 + player -> sets.set( SET_T13_4PC_CASTER ) -> effectN( 2 ).percent();

    base_multiplier *= 1.0 + p() -> sets.set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();

    base_crit += p() -> sets.set( SET_T15_2PC_CASTER ) -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* s )
  {
    p() -> inflight_starsurge = false;

    druid_spell_t::impact( s );

    test_to_extend( s, td( s -> target ) -> dots.moonfire );
    test_to_extend( s, td( s -> target ) -> dots.sunfire );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> inflight_starsurge = true;

      if ( p() -> spec.eclipse -> ok() )
      {
        // Direction not set and bar at 0: +gain
        // Direction not set and bar < 0: -gain
        // Direction not set and bar > 0: +gain
        // Else gain is aligned with direction
        int gain = data().effectN( 2 ).base_value();
        if ( p() -> eclipse_bar_direction == -1 )
        {
          gain = -gain;
        }
        else if ( p() -> eclipse_bar_direction == 0 && p() -> eclipse_bar_value < 0 )
        {
          gain = -gain;
        }

        if ( ! p() -> buff.eclipse_lunar -> check() && ! p() -> buff.eclipse_solar -> check() )
        {
          gain *= 2;
        }
        trigger_eclipse_energy_gain( gain );
      }
    }
    p() -> trigger_soul_of_the_forest();

    // Starsurge gives a bolt in either Eclipse, and two bolts in CA.
    trigger_t16_2pc_balance( false );
    trigger_t16_2pc_balance( true );
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    p() -> buff.shooting_stars -> up();

    druid_spell_t::schedule_execute( state );

    p() -> buff.shooting_stars -> expire();
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buff.shooting_stars -> check() )
      return timespan_t::zero();

    return druid_spell_t::execute_time();
  }

  virtual bool ready()
  {
    // Druids can only have 1 Starsurge in the air at a time
    if ( p() -> inflight_starsurge )
      return false;

    return druid_spell_t::ready();
  }

};

// Prowl ==================================================================

struct prowl_t : public druid_spell_t
{
  prowl_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "prowl", player, player -> find_class_spell( "Prowl" )  )
  {
    parse_options( NULL, options_str );

    trigger_gcd = timespan_t::zero();
    harmful     = false;
  }

  virtual void execute()
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    p() -> buff.prowl -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buff.prowl -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Sunfire Spell ============================================================

struct sunfire_t : public druid_spell_t
{
  // Sunfire also applies the Moonfire DoT during Celestial Alignment.
  struct moonfire_CA_t : public druid_spell_t
  {
    moonfire_CA_t( druid_t* player ) :
      druid_spell_t( "moonfire", player, player -> find_class_spell( "Moonfire" ) )
    {
      assert( player -> specialization() == DRUID_BALANCE );

      dot_behavior = DOT_REFRESH;

      background = true;
      may_crit = false;
      may_miss = true; // Bug?

      num_ticks += ( int ) (  player -> sets.set( SET_T14_4PC_CASTER ) -> effectN( 1 ).time_value() / base_tick_time );

      // Does no direct damage, costs no mana
      attack_power_mod.direct = 0;
      base_dd_min = base_dd_max = 0;
      range::fill( base_costs, 0 );
    }

    virtual void tick( dot_t* d )
    {
      druid_spell_t::tick( d );
      // Todo: Does this dot proc SS?
      p() -> trigger_shooting_stars( d -> state -> result );
    }
  };

  action_t* moonfire;

  sunfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "sunfire", player, player -> find_class_spell( "Sunfire" ) ),
    moonfire( nullptr )
  {
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;

    num_ticks += ( int ) (  player -> sets.set( SET_T14_4PC_CASTER ) -> effectN( 1 ).time_value() / base_tick_time );

    if ( player -> specialization() == DRUID_BALANCE )
      moonfire = new moonfire_CA_t( player );
  }

  virtual double action_da_multiplier() const
  {
    double m = druid_spell_t::action_da_multiplier();

    m *= 1.0 + ( p() -> buff.lunar_shower -> data().effectN( 1 ).percent() * p() -> buff.lunar_shower -> check() );

    return m;
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );

    p() -> trigger_shooting_stars( d -> state -> result );
  }

  virtual void execute()
  {
    p() -> buff.lunar_shower -> up();

    druid_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> spec.lunar_shower -> ok() )
      {
        p() -> buff.lunar_shower -> trigger( 1 );
      }
    }

    trigger_t16_2pc_balance( true );
  }

  virtual void impact( action_state_t* s )
  {
    if ( moonfire && result_is_hit( s -> result ) )
    {
      if ( p() -> buff.celestial_alignment -> check() )
      {
        moonfire -> target = s -> target;
        moonfire -> execute();
      }
    }
    druid_spell_t::impact( s );
  }

  virtual double cost_reduction() const
  {
    double cr = druid_spell_t::cost_reduction();

    cr += ( p() -> buff.lunar_shower -> data().effectN( 2 ).percent() * p() -> buff.lunar_shower -> check() );

    return cr;
  }
};

// Survival Instincts =======================================================

struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "survival_instincts", player, player -> find_specialization_spell( "Survival Instincts" ), options_str )
  {
    harmful = false;
    use_off_gcd = true;
    cooldown -> duration = timespan_t::from_seconds( 120.0 ); // Spell data has wrong cooldown, as of 4/12/14

    if ( player -> specialization() == DRUID_FERAL || player -> specialization() == DRUID_GUARDIAN )
      cooldown -> charges = 2;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.survival_instincts -> trigger(); // DBC value is 60 for some reason
  }
};

// T16 Balance 2P Bonus =====================================================

struct t16_2pc_starfall_bolt_t : public druid_spell_t
{
  t16_2pc_starfall_bolt_t( druid_t* player ) :
    druid_spell_t( "t16_2pc_starfall_bolt", player, player -> find_spell( 144770 ) )
  {
    background  = true;
  }
};

struct t16_2pc_sun_bolt_t : public druid_spell_t
{
  t16_2pc_sun_bolt_t( druid_t* player ) :
    druid_spell_t( "t16_2pc_sun_bolt", player, player -> find_spell( 144772 ) )
  {
    background  = true;
  }
};

// Stampeding Shout =========================================================

struct stampeding_shout_t : public druid_spell_t
{
  stampeding_shout_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "stampeding_shout", p, p -> find_class_spell( "Stampeding Shout" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if( p -> is_enemy() || p -> type == PLAYER_GUARDIAN )
        break;

      p -> buffs.stampeding_shout -> trigger();
    }
  }
};

// Force of Nature Spell ====================================================

struct force_of_nature_spell_t : public druid_spell_t
{
  force_of_nature_spell_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "force_of_nature", player, player -> talent.force_of_nature )
  {
    parse_options( NULL, options_str );

    harmful = false;
    cooldown -> charges = 3;
    cooldown -> duration = timespan_t::from_seconds( 20.0 );
    use_off_gcd = true;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( p() -> pet_force_of_nature[ 0 ] )
    {
      for ( int i = 0; i < 3; i++ )
      {
        if ( p() -> pet_force_of_nature[ i ] -> is_sleeping() )
        {
          p() -> pet_force_of_nature[ i ] -> summon( p() -> talent.force_of_nature -> duration() );
          return;
        }
      }

      p() -> sim -> errorf( "Player %s ran out of treants.\n", p() -> name() );
      assert( false ); // Will only get here if there are no available treants
    }
  }
};

// Typhoon ==================================================================

struct typhoon_t : public druid_spell_t
{
  typhoon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "typhoon", player, player -> talent.typhoon )
  {
    parse_options( NULL, options_str );
  }
};

// Wild Mushroom ============================================================

struct wild_mushroom_t : public druid_spell_t
{
  wild_mushroom_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wild_mushroom", player, player -> find_class_spell( "Wild Mushroom" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.wild_mushroom -> trigger( !p() -> in_combat ? p() -> buff.wild_mushroom -> max_stack() : 1 );
  }
};

// Wild Mushroom: Detonate ==================================================

struct wild_mushroom_detonate_damage_t : public druid_spell_t
{
  wild_mushroom_detonate_damage_t( druid_t* player ) :
    druid_spell_t( "wild_mushroom_detonate", player, player -> find_spell( 78777 ) )
  {
    aoe        = -1;
    background = true;
    dual       = true;
  }
};

struct wild_mushroom_detonate_t : public druid_spell_t
{
  action_t* detonation;

  wild_mushroom_detonate_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wild_mushroom_detonate", player, player -> find_class_spell( "Wild Mushroom: Detonate" ) ),
    detonation( new wild_mushroom_detonate_damage_t( player ) )
  {
    parse_options( NULL, options_str );

    // Actual ability is 88751, all damage is in spell 78777
    school             = detonation -> school;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    while ( p() -> buff.wild_mushroom -> check() )
    {
      detonation -> execute();
      p() -> buff.wild_mushroom -> decrement();
    }
  }

  virtual bool ready()
  {

    if ( ! p() -> buff.wild_mushroom -> stack() )
      return false;

    return druid_spell_t::ready();
  }
};

// Wrath Spell ==============================================================

struct wrath_t : public druid_spell_t
{
  wrath_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wrath", player, player -> find_class_spell( "Wrath" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void impact( action_state_t* s )
  {
    druid_spell_t::impact( s );
    test_to_extend( s, td( s -> target ) -> dots.sunfire );
  }

  virtual double action_multiplier() const
  {
    double m = druid_spell_t::action_multiplier();

    m *= 1.0 + p() -> sets.set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();

    m *= 1.0 + p() -> buff.heart_of_the_wild -> damage_spell_multiplier();

    if ( p() -> talent.dream_of_cenarius && p() -> specialization() == DRUID_RESTORATION )
      m *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 1 ).percent();

    return m;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    // Cast wrath, but lunar eclipse was up?
    if ( p() -> buff.eclipse_lunar -> check() && ! p() -> buff.celestial_alignment -> check() )
      p() -> proc.wrong_eclipse_wrath -> occur();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> spec.eclipse -> ok() )
      {
        if ( p() -> eclipse_bar_direction <= 0 )
        {
          int gain = data().effectN( 2 ).base_value();

          if ( ! p() -> buff.eclipse_solar -> check() )
          {
            gain *= 2;
          }
          trigger_eclipse_energy_gain( -gain );
        }
      }
    }

    trigger_t16_2pc_balance( true );

    p() -> trigger_soul_of_the_forest();
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_spell_t::schedule_execute( state );

    p() -> buff.cat_form  -> expire();
    p() -> buff.bear_form -> expire();
  }
};

} // end namespace spells



// ==========================================================================
// Druid Character Definition
// ==========================================================================

void druid_t::trigger_shooting_stars( result_e result )
{
  if ( result == RESULT_CRIT )
  {
    int stack = buff.shooting_stars -> check();
    if ( buff.shooting_stars -> trigger() )
    {
      if ( stack == buff.shooting_stars -> check() )
        proc.shooting_stars_wasted -> occur();
      cooldown.starsurge -> reset( true );
    }
  }
}

void druid_t::trigger_soul_of_the_forest()
{
  if ( ! talent.soul_of_the_forest -> ok() )
    return;

  // 5.4 mechanic: Your Wrath, Starfire, and Starsurge casts have a 8%
  // chance to cause your next Astral Communion to instantly advance you to
  // the next Eclipse.
  buff.astral_insight -> trigger();
}

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace heals;
  using namespace spells;

  if ( name == "astral_communion"       ) return new       astral_communion_t( this, options_str );
  if ( name == "astral_storm"           ) return new           astral_storm_t( this, options_str );
  if ( name == "auto_attack"            ) return new            auto_attack_t( this, options_str );
  if ( name == "barkskin"               ) return new               barkskin_t( this, options_str );
  if ( name == "berserk"                ) return new                berserk_t( this, options_str );
  if ( name == "bear_form"              ) return new              bear_form_t( this, options_str );
  if ( name == "cat_form"               ) return new               cat_form_t( this, options_str );
  if ( name == "celestial_alignment"    ) return new    celestial_alignment_t( this, options_str );
  if ( name == "cenarion_ward"          ) return new          cenarion_ward_t( this, options_str );
  if ( name == "faerie_fire"            ) return new            faerie_fire_t( this, options_str );
  if ( name == "ferocious_bite"         ) return new         ferocious_bite_t( this, options_str );
  if ( name == "frenzied_regeneration"  ) return new  frenzied_regeneration_t( this, options_str );
  if ( name == "healing_touch"          ) return new          healing_touch_t( this, options_str );
  if ( name == "hurricane"              ) return new              hurricane_t( this, options_str );
  if ( name == "heart_of_the_wild"      ) return new      heart_of_the_wild_t( this, options_str );
  if ( name == "incarnation"            ) return new            incarnation_t( this, options_str );
  if ( name == "lacerate"               ) return new               lacerate_t( this, options_str );
  if ( name == "lifebloom"              ) return new              lifebloom_t( this, options_str );
  if ( name == "maim"                   ) return new                   maim_t( this, options_str );
  if ( name == "mangle"                 ) return new                 mangle_t( this, options_str );
  if ( name == "mark_of_the_wild"       ) return new       mark_of_the_wild_t( this, options_str );
  if ( name == "maul"                   ) return new                   maul_t( this, options_str );
  if ( name == "might_of_ursoc"         ) return new         might_of_ursoc_t( this, options_str );
  if ( name == "moonfire"               ) return new               moonfire_t( this, options_str );
  if ( name == "moonkin_form"           ) return new           moonkin_form_t( this, options_str );
  if ( name == "natures_swiftness"      ) return new       druids_swiftness_t( this, options_str );
  if ( name == "natures_vigil"          ) return new          natures_vigil_t( this, options_str );
  if ( name == "pounce"                 ) return new                 pounce_t( this, options_str );
  if ( name == "rake"                   ) return new                   rake_t( this, options_str );
  if ( name == "ravage"                 ) return new                 ravage_t( this, options_str );
  if ( name == "renewal"                ) return new                renewal_t( this, options_str );
  if ( name == "regrowth"               ) return new               regrowth_t( this, options_str );
  if ( name == "rejuvenation"           ) return new           rejuvenation_t( this, options_str );
  if ( name == "rip"                    ) return new                    rip_t( this, options_str );
  if ( name == "savage_roar"            ) return new            savage_roar_t( this, options_str );
  if ( name == "savage_defense"         ) return new         savage_defense_t( this, options_str );
  if ( name == "shred"                  ) return new                  shred_t( this, options_str );
  if ( name == "skull_bash_bear"        ) return new        skull_bash_bear_t( this, options_str );
  if ( name == "skull_bash_cat"         ) return new         skull_bash_cat_t( this, options_str );
  if ( name == "stampeding_shout"       ) return new       stampeding_shout_t( this, options_str );
  if ( name == "starfire"               ) return new               starfire_t( this, options_str );
  if ( name == "starfall"               ) return new               starfall_t( this, options_str );
  if ( name == "starsurge"              ) return new              starsurge_t( this, options_str );
  if ( name == "prowl"                  ) return new                  prowl_t( this, options_str );
  if ( name == "sunfire"                ) return new                sunfire_t( this, options_str );
  if ( name == "survival_instincts"     ) return new     survival_instincts_t( this, options_str );
  if ( name == "swipe"                  ) return new                  swipe_t( this, options_str );
  if ( name == "swiftmend"              ) return new              swiftmend_t( this, options_str );
  if ( name == "tigers_fury"            ) return new            tigers_fury_t( this, options_str );
  if ( name == "thrash_bear"            ) return new            thrash_bear_t( this, options_str );
  if ( name == "thrash_cat"             ) return new             thrash_cat_t( this, options_str );
  if ( name == "force_of_nature"        ) return new  force_of_nature_spell_t( this, options_str );
  if ( name == "tranquility"            ) return new            tranquility_t( this, options_str );
  if ( name == "typhoon"                ) return new                typhoon_t( this, options_str );
  if ( name == "wild_growth"            ) return new            wild_growth_t( this, options_str );
  if ( name == "wild_mushroom"          ) return new          wild_mushroom_t( this, options_str );
  if ( name == "wild_mushroom_detonate" ) return new wild_mushroom_detonate_t( this, options_str );
  if ( name == "wrath"                  ) return new                  wrath_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name,
                            const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  return 0;
}

// druid_t::create_pets =====================================================

void druid_t::create_pets()
{
  if ( specialization() == DRUID_BALANCE )
  {
    for ( int i = 0; i < 3; ++i )
      pet_force_of_nature[ i ] = new pets::force_of_nature_balance_t( sim, this );
  }
  else if ( specialization() == DRUID_FERAL )
  {
    for ( int i = 0; i < 3; ++i )
      pet_force_of_nature[ i ] = new pets::force_of_nature_feral_t( sim, this );
  }
  else if ( specialization() == DRUID_GUARDIAN )
  {
    for ( size_t i = 0; i < sizeof_array( pet_force_of_nature ); i++ )
      pet_force_of_nature[ i ] = new pets::force_of_nature_guardian_t( sim, this );
  }
}

// druid_t::init_spells =====================================================

void druid_t::init_spells()
{
  player_t::init_spells();

  // Specializations
  // Generic / Multiple specs
  spec.leather_specialization = find_specialization_spell( "Leather Specialization" );
  spec.omen_of_clarity        = find_specialization_spell( "Omen of Clarity" );
  spec.killer_instinct        = find_specialization_spell( "Killer Instinct" );
  spec.nurturing_instinct     = find_specialization_spell( "Nurturing Instinct" );
  spec.critical_strikes       = find_specialization_spell( "Critical Strikes" );

  // Balance
  // Eclipse are 2 spells, the mana energize is not in the main spell!
  // http://mop.wowhead.com/spell=79577 => Specialization spell
  // http://mop.wowhead.com/spell=81070 => The mana gain energize spell
  // Moonkin is also split up into two spells
  spec.balance_of_power       = find_specialization_spell( "Balance of Power" );
  spec.celestial_focus        = find_specialization_spell( "Celestial Focus" );
  spec.eclipse                = find_specialization_spell( "Eclipse" );
  spec.euphoria               = find_specialization_spell( "Euphoria" );
  spec.lunar_shower           = find_specialization_spell( "Lunar Shower" );
  spec.owlkin_frenzy          = find_specialization_spell( "Owlkin Frenzy" );
  spec.shooting_stars         = find_specialization_spell( "Shooting Stars" );

  // Feral
  spec.predatory_swiftness    = find_specialization_spell( "Predatory Swiftness" );

  // Guardian
  spec.leader_of_the_pack     = find_specialization_spell( "Leader of the Pack" );
  spec.thick_hide             = find_specialization_spell( "Thick Hide" );
  spec.tooth_and_claw         = find_specialization_spell( "Tooth and Claw" );

  // Restoration
  spec.living_seed            = find_specialization_spell( "Living Seed" );
  spec.meditation             = find_specialization_spell( "Meditation" );
  spec.natural_insight        = find_specialization_spell( "Natural Insight" );
  spec.natures_focus          = find_specialization_spell( "Nature's Focus" );

  if ( spec.leader_of_the_pack -> ok() )
    active.leader_of_the_pack = new leader_of_the_pack_t( this );

  // Talents
  talent.feline_swiftness   = find_talent_spell( "Feline Swiftness" );
  talent.displacer_beast    = find_talent_spell( "Displacer Beast" );
  talent.wild_charge        = find_talent_spell( "Wild Charge" );

  talent.yseras_gift        = find_talent_spell( "Ysera's Gift" );
  talent.renewal            = find_talent_spell( "Renewal" );
  talent.cenarion_ward      = find_talent_spell( "Cenarion Ward" );

  talent.faerie_swarm       = find_talent_spell( "Faerie Swarm" );
  talent.mass_entanglement  = find_talent_spell( "Mass Entanglement" );
  talent.typhoon            = find_talent_spell( "Typhoon" );

  talent.soul_of_the_forest = find_talent_spell( "Soul of the Forest" );
  talent.incarnation        = find_talent_spell( "Incarnation" );
  talent.force_of_nature    = find_talent_spell( "Force of Nature" );

  talent.disorienting_roar  = find_talent_spell( "Disorienting Roar" );
  talent.ursols_vortex      = find_talent_spell( "Ursol's Vortex" );
  talent.mighty_bash        = find_talent_spell( "Mighty Bash" );

  talent.heart_of_the_wild  = find_talent_spell( "Heart of the Wild" );
  talent.dream_of_cenarius  = find_talent_spell( "Dream of Cenarius" );
  talent.natures_vigil      = find_talent_spell( "Nature's Vigil" );

    // Touch of Elune (Level 100 Slot 1)
  talent.sunfall            = find_talent_spell( "Sunfall" );
  talent.lunar_inspiration  = find_talent_spell( "Lunar Inspiration" );
  talent.guardian_of_elune  = find_talent_spell( "Guardian of Elune" );
  talent.moment_of_clarity  = find_talent_spell( "Moment of Clarity" );

  // Will of Malfurion (Level 100 Slot 2)
  talent.insect_swarm       = find_talent_spell( "Insect Swarm" );
  talent.bloody_thrash      = find_talent_spell( "Bloody Thrash" );
  talent.pulverize          = find_talent_spell( "Pulverize" );
  talent.germination        = find_talent_spell( "Germination" );

  // Might of Malorne (Level 100 Slot 3)
  talent.equinox            = find_talent_spell( "Equinox" );
  talent.savagery           = find_talent_spell( "Savagery" );
  talent.bristling_fur      = find_talent_spell( "Bristling Fur" );
  talent.rampant_growth     = find_talent_spell( "Rampant Growth" );

  // Active actions
  if ( talent.natures_vigil -> ok() )
    active.natures_vigil     = new natures_vigil_proc_t( this );
  if ( talent.cenarion_ward -> ok() )
    active.cenarion_ward_hot = new cenarion_ward_hot_t( this );
  if ( talent.yseras_gift -> ok() )
    active.yseras_gift       = new yseras_gift_t( this );

  // TODO: Check if this is really the passive applied, the actual shapeshift
  // only has data of shift, polymorph immunity and the general armor bonus

  spell.bear_form                       = find_class_spell( "Bear Form"                   ) -> ok() ? find_spell( 1178   ) : spell_data_t::not_found(); // This is the passive applied on shapeshift!
  spell.berserk_bear                    = find_class_spell( "Berserk"                     ) -> ok() ? find_spell( 50334  ) : spell_data_t::not_found(); // Berserk bear mangler
  spell.berserk_cat                     = find_class_spell( "Berserk"                     ) -> ok() ? find_spell( 106951 ) : spell_data_t::not_found(); // Berserk cat resource cost reducer
  spell.combo_point                     = find_class_spell( "Cat Form"                    ) -> ok() ? find_spell( 34071  ) : spell_data_t::not_found(); // Combo point add "spell", weird
  spell.eclipse                         = spec.eclipse -> ok() ? find_spell( 81070  ) : spell_data_t::not_found(); // Eclipse mana gain trigger
  spell.heart_of_the_wild               = talent.heart_of_the_wild -> ok() ? find_spell( 17005  ) : spell_data_t::not_found(); // HotW INT/AGI bonus
  spell.leader_of_the_pack              = spec.leader_of_the_pack -> ok() ? find_spell( 24932  ) : spell_data_t::not_found(); // LotP aura
  spell.mangle                          = find_class_spell( "Lacerate"                    ) -> ok() ||
                                          find_specialization_spell( "Thrash"             ) -> ok() ? find_spell( 93622  ) : spell_data_t::not_found(); // Lacerate mangle cooldown reset
  spell.moonkin_form                    = find_class_spell( "Moonkin Form"                ) -> ok() ? find_spell( 24905  ) : spell_data_t::not_found(); // This is the passive applied on shapeshift!
  spell.regrowth                        = find_class_spell( "Regrowth"                    ) -> ok() ? find_spell( 93036  ) : spell_data_t::not_found(); // Regrowth refresh
  spell.survival_instincts              = find_class_spell( "Survival Instincts"          ) -> ok() ? find_spell( 50322  ) : spell_data_t::not_found(); // Survival Instincts aura
  spell.swiftmend                       = find_spell( 81262 );
  spell.swipe                           = find_class_spell( "Maul"                        ) -> ok() ||
                                          find_class_spell( "Shred"                       ) -> ok() ? find_spell( 62078  ) : spell_data_t::not_found(); // Bleed damage multiplier for Shred etc.

  if ( specialization() == DRUID_FERAL )
    spell.primal_fury = find_spell( 16953 );
  else if ( specialization() == DRUID_GUARDIAN )
    spell.primal_fury = find_spell( 16959 );

  // Masteries
  mastery.total_eclipse    = find_mastery_spell( DRUID_BALANCE );
  mastery.razor_claws      = find_mastery_spell( DRUID_FERAL );
  mastery.harmony          = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian = find_mastery_spell( DRUID_GUARDIAN );

  // Perks
  perk.enhanced_berserk        = find_perk_spell( 0, DRUID_FERAL );
  perk.enhanced_cat_form       = find_perk_spell( 1, DRUID_FERAL );
  perk.enhanced_prowl          = find_perk_spell( 2, DRUID_FERAL );
  perk.enhanced_rejuvenation   = find_perk_spell( 3, DRUID_FERAL );
  perk.enhanced_tigers_fury    = find_perk_spell( 4, DRUID_FERAL );
  perk.improved_ferocious_bite = find_perk_spell( 5, DRUID_FERAL );
  perk.improved_pounce         = find_perk_spell( 6, DRUID_FERAL );
  perk.improved_shred          = find_perk_spell( 7, DRUID_FERAL );
  perk.improved_healing_touch  = find_perk_spell( 8, DRUID_FERAL );

  // Glyphs
  glyph.blooming              = find_glyph_spell( "Glyph of Blooming" );
  glyph.cat_form              = find_glyph_spell( "Glyph of Cat Form" );
  glyph.ferocious_bite        = find_glyph_spell( "Glyph of Ferocious Bite" );
  glyph.frenzied_regeneration = find_glyph_spell( "Glyph of Frenzied Regeneration" );
  glyph.healing_touch         = find_glyph_spell( "Glyph of Healing Touch" );
  glyph.lifebloom             = find_glyph_spell( "Glyph of Lifebloom" );
  glyph.maim                  = find_glyph_spell( "Glyph of Maim" );
  glyph.maul                  = find_glyph_spell( "Glyph of Maul" );
  glyph.might_of_ursoc        = find_glyph_spell( "Glyph of Might of Ursoc" );
  glyph.ninth_life            = find_glyph_spell( "Glyph of the Ninth Life" );
  glyph.regrowth              = find_glyph_spell( "Glyph of Regrowth" );
  glyph.rejuvenation          = find_glyph_spell( "Glyph of Rejuvenation" );
  //glyph.savagery              = find_glyph_spell( "Glyph of Savagery" ); Missing in spell data 4/26/2014
  glyph.skull_bash            = find_glyph_spell( "Glyph of Skull Bash" );
  glyph.ursols_defense        = find_glyph_spell( "Glyph of Ursol's Defense" );
  glyph.wild_growth           = find_glyph_spell( "Glyph of Wild Growth" );

  // Tier Bonuses
  static const set_bonus_description_t set_bonuses =
  {
    //   C2P     C4P     M2P     M4P    T2P    T4P     H2P     H4P
    { 105722, 105717, 105725, 105735,      0,      0, 105715, 105770 }, // Tier13
    { 123082, 123083, 123084, 123085, 123086, 123087, 123088, 123089 }, // Tier14
    { 138348, 138350, 138352, 138357, 138216, 138222, 138284, 138286 }, // Tier15
    { 144767, 144756, 144864, 144841, 144879, 144887, 144869, 144875 }, // Tier16
  };

  sets.register_spelldata( set_bonuses );

  if ( sets.has_set_bonus( SET_T16_2PC_CASTER ) )
  {
    t16_2pc_starfall_bolt = new spells::t16_2pc_starfall_bolt_t( this );
    t16_2pc_sun_bolt = new spells::t16_2pc_sun_bolt_t( this );
  }
}

// druid_t::init_base =======================================================

void druid_t::init_base_stats()
{
  player_t::init_base_stats();

  //base.stats.attack_power = level * ( level > 80 ? 3.0 : 2.0 ); Removed in WoD, double check later.

  // TODO: Confirm that all druid specs get both of these things.
  base.attack_power_per_agility  = 0.0; // This is adjusted in cat_form_t and bear_form_t
  base.spell_power_per_intellect = 1.0;

  // Avoidance diminishing Returns constants/conversions
  base.miss  = 0.030;
  base.dodge = 0.030;
  base.parry = 0.000;

  // based on http://www.sacredduty.net/2013/08/08/updated-diminishing-returns-coefficients-all-tanks/
  diminished_kfactor   = 1.2220000;
  diminished_dodge_cap = 1.50375948;
  diminished_parry_cap = 1; // irrelevant for druids, cannot parry

  // note that these conversions are level-specific; these are L90 values
  base.dodge_per_agility = 1 / 95115.8596; // exact value given by Blizzard
  base.parry_per_strength = 0; // this is also the default, but just to be safe...

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base[ RESOURCE_RAGE   ] = 100;

  base_energy_regen_per_second = 10;

  // Natural Insight: +400% mana
  resources.base_multiplier[ RESOURCE_MANA ] = 1.0 + spec.natural_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.natural_insight -> effectN( 1 ).percent();

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// druid_t::init_buffs ======================================================

void druid_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // MoP checked

  // Generic / Multi-spec druid buffs
  buff.bear_form             = new bear_form_t( *this );
  buff.berserk               = new berserk_buff_t( *this );
  buff.cat_form              = new cat_form_t( *this );
  buff.dash                  = buff_creator_t( this, "dash", find_class_spell( "Dash" ) );
  buff.frenzied_regeneration = buff_creator_t( this, "frenzied_regeneration", find_class_spell( "Frenzied Regeneration" ) );
  buff.moonkin_form          = new moonkin_form_t( *this );
  buff.omen_of_clarity       = buff_creator_t( this, "omen_of_clarity", spec.omen_of_clarity -> effectN( 1 ).trigger() )
                               .chance( spec.omen_of_clarity -> ok() ? find_spell( 113043 ) -> proc_chance() : 0.0 );
  buff.soul_of_the_forest    = buff_creator_t( this, "soul_of_the_forest", talent.soul_of_the_forest -> ok() ? find_spell( 114108 ) : spell_data_t::not_found() )
                               .default_value( find_spell( 114108 ) -> effectN( 1 ).percent() );
  buff.prowl                 = buff_creator_t( this, "prowl", find_class_spell( "Prowl" ) );
  buff.wild_mushroom         = buff_creator_t( this, "wild_mushroom", find_class_spell( "Wild Mushroom" ) )
                               .max_stack( ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
                                           ? find_class_spell( "Wild Mushroom" ) -> effectN( 2 ).base_value()
                                           : 1 )
                               .quiet( true );

  // Talent buffs

  buff.cenarion_ward = buff_creator_t( this, "cenarion_ward", find_talent_spell( "Cenarion Ward" ) );

  // http://mop.wowhead.com/spell=122114 Chosen of Elune
  buff.chosen_of_elune    = buff_creator_t( this, "chosen_of_elune"   , talent.incarnation -> ok() ? find_spell( 122114 ) : spell_data_t::not_found() )
                            .duration( talent.incarnation -> duration() )
                            .chance( talent.incarnation -> ok() ? ( specialization() == DRUID_BALANCE ) : 0.0 )
                            .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // http://mop.wowhead.com/spell=102548 Incarnation: King of the Jungle
  buff.king_of_the_jungle = buff_creator_t( this, "king_of_the_jungle", talent.incarnation -> ok() ? find_spell( 102543 ) : spell_data_t::not_found() )
                            .duration( talent.incarnation -> duration() )
                            .chance( talent.incarnation -> ok() ? ( specialization() == DRUID_FERAL ) : 0.0 );

  // http://mop.wowhead.com/spell=113711 Incarnation: Son of Ursoc      Passive
  buff.son_of_ursoc       = buff_creator_t( this, "son_of_ursoc"      , talent.incarnation -> ok() ? find_spell( 102558 ) : spell_data_t::not_found() )
                            .duration( talent.incarnation -> duration() )
                            .chance( talent.incarnation -> ok() ?  ( specialization() == DRUID_GUARDIAN ) : 0.0 );

  // http://mop.wowhead.com/spell=5420 Incarnation: Tree of Life        Passive
  buff.tree_of_life       = buff_creator_t( this, "tree_of_life"      , talent.incarnation -> ok() ? find_spell( 5420 ) : spell_data_t::not_found() )
                            .duration( talent.incarnation -> duration() )
                            .chance( talent.incarnation -> ok() ?  ( specialization() == DRUID_RESTORATION ) : 0.0 );

  // Dream of Cenarius
  if ( talent.dream_of_cenarius -> ok() )
  {
    if ( specialization() == DRUID_BALANCE )
      buff.dream_of_cenarius = buff_creator_t( this, "dream_of_cenarius", find_spell( 154271 ) );
    else if ( specialization() == DRUID_GUARDIAN )
      buff.dream_of_cenarius = buff_creator_t( this, "dream_of_cenarius", find_spell( 145162 ) )
                               .chance( talent.dream_of_cenarius -> effectN( 1 ).percent() );
    else
      buff.dream_of_cenarius = buff_creator_t( this, "dream_of_cenarius", spell_data_t::not_found() );
  }
  else
    buff.dream_of_cenarius = buff_creator_t( this, "dream_of_cenarius", spell_data_t::not_found() );

  buff.natures_vigil      = buff_creator_t( this, "natures_vigil", talent.natures_vigil -> ok() ? find_spell( 124974 ) : spell_data_t::not_found() );
  buff.heart_of_the_wild  = new heart_of_the_wild_buff_t( *this );

  // Balance

  buff.astral_insight            = buff_creator_t( this, "astral_insight", talent.soul_of_the_forest -> ok() ? find_spell( 145138 ) : spell_data_t::not_found() )
                                   .chance( 0.08 );
  buff.celestial_alignment       = new celestial_alignment_t( *this );
  buff.eclipse_lunar             = new eclipse_lunar_t( *this );
                                   //.add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.eclipse_solar             = new eclipse_solar_t( *this );
                                   //.add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.lunar_shower              = buff_creator_t( this, "lunar_shower",   spec.lunar_shower -> effectN( 1 ).trigger() );
  buff.shooting_stars            = buff_creator_t( this, "shooting_stars", spec.shooting_stars -> effectN( 1 ).trigger() )
                                   .chance( spec.shooting_stars -> proc_chance() + sets.set( SET_T16_4PC_CASTER ) -> effectN( 1 ).percent() );
  buff.starfall                  = buff_creator_t( this, "starfall",       find_specialization_spell( "Starfall" ) )
                                   .cd( timespan_t::zero() );

  stat_buff_creator_t ng = stat_buff_creator_t( this, "natures_grace" )
                           .spell( find_specialization_spell( "Eclipse" ) -> ok() ? find_spell( 16886 ) : spell_data_t::not_found() );
  if ( sets.has_set_bonus( SET_T15_4PC_CASTER ) )
  {
    ng.add_stat( STAT_CRIT_RATING, sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).base_value() )
      .add_stat( STAT_MASTERY_RATING, sets.set( SET_T15_4PC_CASTER ) -> effectN( 1 ).base_value() );
  }
  buff.natures_grace = ng;

  // Feral
  buff.tigers_fury           = buff_creator_t( this, "tigers_fury", find_specialization_spell( "Tiger's Fury" ) )
                               .default_value( find_specialization_spell( "Tiger's Fury" ) -> effectN( 1 ).percent() )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                               .cd( timespan_t::zero() )
                               .chance( 1.0 )
                               .duration( find_specialization_spell( "Tiger's Fury" ) -> duration() + perk.enhanced_tigers_fury -> effectN( 1 ).time_value() );
  buff.savage_roar           = buff_creator_t( this, "savage_roar", find_specialization_spell( "Savage Roar" ) )
                               .default_value( find_spell( 62071 ) -> effectN( 1 ).percent() )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                               .duration( timespan_t::max() ); // Set base duration to infinite for Savagery talent. All other uses trigger with a set duration.
  buff.predatory_swiftness   = buff_creator_t( this, "predatory_swiftness", spec.predatory_swiftness -> ok() ? find_spell( 69369 ) : spell_data_t::not_found() );
  buff.tier15_4pc_melee      = buff_creator_t( this, "tier15_4pc_melee", find_spell( 138358 ) );
  buff.feral_fury            = buff_creator_t( this, "feral_fury", find_spell( 144865 ) ); // tier16_2pc_melee
  buff.feral_rage            = buff_creator_t( this, "feral_rage", find_spell( 146874 ) ); // tier16_4pc_melee

  // Guardian
  buff.bladed_armor           = buff_creator_t( this, "bladed_armor", find_specialization_spell( "Bladed Armor" ) )
                                .add_invalidate( CACHE_ATTACK_POWER );
  buff.barkskin              = new barkskin_t( *this );
  buff.lacerate              = buff_creator_t( this, "lacerate" , find_class_spell( "Lacerate" ) );
  buff.might_of_ursoc        = new might_of_ursoc_t( this, 106922, "might_of_ursoc" );
  buff.savage_defense        = buff_creator_t( this, "savage_defense", find_class_spell( "Savage Defense" ) -> ok() ? find_spell( 132402 ) : spell_data_t::not_found() )
                               .add_invalidate( CACHE_DODGE );
  buff.survival_instincts    = buff_creator_t( this, "survival_instincts", spell.survival_instincts )
                               .cd( timespan_t::zero() );
  buff.tier15_2pc_tank       = buff_creator_t( this, "tier15_2pc_tank", find_spell( 138217 ) );
  buff.tooth_and_claw        = buff_creator_t( this, "tooth_and_claw", find_spell( 135286 ) );
  buff.tooth_and_claw_absorb = new tooth_and_claw_absorb_t( this );

  // Restoration

  // Not checked for MoP

  buff.harmony               = buff_creator_t( this, "harmony", mastery.harmony -> ok() ? find_spell( 100977 ) : spell_data_t::not_found() );
  buff.natures_swiftness     = buff_creator_t( this, "natures_swiftness", find_specialization_spell( "Nature's Swiftness" ) )
                               .cd( timespan_t::zero() ); // Cooldown is handled in the spell
}

// ALL Spec Pre-Combat Action Priority List =================================

void druid_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask or Elixir
  if ( sim -> allow_flasks && level >= 80 )
  {
    if ( ( specialization() == DRUID_GUARDIAN && primary_role() == ROLE_TANK ) || primary_role() == ROLE_TANK ) {
      precombat -> add_action( "elixir,type=mad_hozen" );
      precombat -> add_action( "elixir,type=mantid" );
    } else {
      std::string flask_action = "flask,type=";
      if ( ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK ) || primary_role() == ROLE_ATTACK )
        flask_action += ( level > 85 ) ? "spring_blossoms" : "winds";
      else
        flask_action += ( level > 85 ) ? "warm_sun" : "draconic_mind";
      precombat -> add_action( flask_action );
    }
  }

  // Food
  if ( sim -> allow_food && level >= 80 )
  {
    std::string food_action = "food,type=";
    if ( level > 85 )
    {
      if ( specialization() == DRUID_FERAL )
        food_action += "sea_mist_rice_noodles";
      else if ( specialization() == DRUID_GUARDIAN )
        food_action += "chun_tian_spring_rolls";
      else
        food_action += "mogu_fish_stew";
    }
    else
      food_action += "seafood_magnifique_feast";
    precombat -> add_action( food_action );
  }

  // Mark of the Wild
  precombat -> add_action( this, "Mark of the Wild", "if=!aura.str_agi_int.up" );

  precombat -> add_action( this, "Wild Mushroom", "if=buff.wild_mushroom.stack<buff.wild_mushroom.max_stack" );

  // Forms
  if ( ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK ) || primary_role() == ROLE_ATTACK )
  {
    precombat -> add_action( this, "Cat Form" );
    precombat -> add_action( this, "Prowl" );
  }
  else if ( primary_role() == ROLE_TANK )
  {
    precombat -> add_action( this, "Bear Form" );
  }
  else if ( specialization() == DRUID_BALANCE && ( primary_role() == ROLE_DPS || primary_role() == ROLE_SPELL ) )
  {
    precombat -> add_action( this, "Moonkin Form" );
  }

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-Potion
  if ( sim -> allow_potions && level >= 80 )
  {
    if ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK )
      precombat -> add_action( ( level > 85 ) ? "virmens_bite_potion" : "tolvir_potion" );
    else if ( ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION ) && ( primary_role() == ROLE_SPELL || primary_role() == ROLE_HEAL ) )
      precombat -> add_action( ( level > 85 ) ? "jade_serpent_potion" : "volcanic_potion" );
  }
}

// NO Spec Combat Action Priority List ======================================

void druid_t::apl_default()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // Assemble Racials / On-Use Items / Professions
  std::string extra_actions = "";

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    extra_actions += add_action( racial_actions[ i ] );

  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    extra_actions += add_action( item_actions[ i ] );

  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    extra_actions += add_action( profession_actions[ i ] );

  if ( primary_role() == ROLE_SPELL )
  {
    def -> add_action( extra_actions );
    def -> add_action( this, "Moonfire", "if=!dot.moonfire.ticking" );
    def -> add_action( "Wrath" );
  }
  // Specless (or speced non-main role) druid who has a primary role of a melee
  else if ( primary_role() == ROLE_ATTACK )
  {
    def -> add_action( this, "Faerie Fire", "if=debuff.weakened_armor.stack<3" );
    def -> add_action( extra_actions );
    def -> add_action( this, "Rake", "if=!ticking|ticks_remain<2" );
    def -> add_action( this, "Shred" );
    def -> add_action( this, "Ferocious Bite", "if=combo_points>=5" );
  }
  // Specless (or speced non-main role) druid who has a primary role of a healer
  else if ( primary_role() == ROLE_HEAL )
  {
    def -> add_action( extra_actions );
    def -> add_action( this, "Rejuvenation", "if=!ticking|remains<tick_time" );
    def -> add_action( this, "Healing Touch", "if=mana.pct>=30" );
  }
}

// Feral Combat Action Priority List =======================================

void druid_t::apl_feral()
{
  action_priority_list_t* def    = get_action_priority_list( "default" );
  action_priority_list_t* filler = get_action_priority_list( "filler"  );

  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  if ( perk.improved_pounce -> ok() )
    def -> add_action( this, "Pounce", "if=buff.prowl.up" );
  else
    def -> add_action( this, "Ravage", "if=buff.prowl.up" );
  def -> add_action( "auto_attack" );
  def -> add_action( this, "Ferocious Bite", "cycle_targets=1,if=dot.rip.ticking&dot.rip.remains<=3&target.health.pct<=25",
                     "Keep Rip from falling off during execute range." );
  def -> add_action( this, "Faerie Fire", "cycle_targets=1,if=debuff.physical_vulnerability.down" );
  def -> add_action( this, "Savage Roar", "if=buff.savage_roar.remains<3" );
  std::string potion_name = level > 85 ? "virmens_bite_potion" : "tolvir_potion";
  def -> add_action( potion_name + ",if=target.time_to_die<=40" );
  // On-Use Items
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def -> add_action( item_actions[ i ] + ",sync=tigers_fury" );
  // Racials
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] + ",sync=tigers_fury" );
  def -> add_action( this, "Tiger's Fury", "if=energy<=35&!buff.omen_of_clarity.react" );
  def -> add_action( potion_name + ",sync=berserk,if=target.health.pct<25" );
  def -> add_action( this, "Berserk", "if=buff.tigers_fury.up" );
  def -> add_action( "thrash_cat,if=buff.omen_of_clarity.react&dot.thrash_cat.remains<3" );
  def -> add_action( this, "Rip", "cycle_targets=1,if=dot.rip.remains<2&combo_points>=5" );
  def -> add_action( this, "Rip", "cycle_targets=1,if=target.health.pct<25&combo_points>=5&action.rip.tick_multiplier>dot.rip.multiplier",
                     "Apply the strongest possible Rip during execute." );
  def -> add_action( this, "Rake", "cycle_targets=1,if=!talent.bloody_thrash.enabled&dot.rake.remains<3" );
  def -> add_action( this, "Rake", "cycle_targets=1,if=!talent.bloody_thrash.enabled&action.rake.tick_multiplier>dot.rake.multiplier" );
  def -> add_action( this, "Moonfire", "cycle_targets=1,if=talent.lunar_inspiration.enabled&dot.moonfire.remains<3" );
  def -> add_action( "pool_resource,for_next=1",
                     "Pool energy for Thrash." );
  def -> add_action( "thrash_cat,cycle_targets=1,if=talent.bloody_thrash.enabled&(dot.rake.remains<3|action.rake.tick_multiplier>dot.rake.multiplier)" );
  def -> add_action( this, "Ferocious Bite", "if=combo_points>=5&(energy.time_to_max<=1|buff.berserk.up)&dot.rip.remains>=4" );
  def -> add_action( "run_action_list,name=filler,if=combo_points<5",
                     "Cast a CP generator." );
  def -> add_talent( this, "Nature's Vigil" );
  def -> add_action( this, "Healing Touch", "if=buff.natures_vigil.up&buff.predatory_swiftness.up" );
  if ( perk.enhanced_rejuvenation -> ok() )
    def -> add_action( this, "Rejuvenation", "if=buff.natures_vigil.up&!ticking" );
  def -> add_action( this, "Faerie Fire", "cycle_targets=1,if=debuff.physical_vulnerability.remains>0" );

  filler -> add_action( "pool_resource,for_next=1",
                        "Pool energy for Ravage." );
  filler -> add_action( this, "Ravage" );
  filler -> add_action( "pool_resource,for_next=1",
                        "Pool energy for Swipe." );
  filler -> add_action( this, "Swipe", "if=active_enemies>1" );
  filler -> add_action( this, "Rake", "if=action.rake.hit_damage>=action.shred.hit_damage",
                        "Rake for CP if it hits harder than Shred." );
  filler -> add_action( this, "Shred" );
}

// Balance Combat Action Priority List ==============================

void druid_t::apl_balance()
{
  if ( sim -> allow_potions && level >= 80 )
  {
    action_list_str += ( level > 85 ) ? "/jade_serpent_potion" : "/volcanic_potion";
    action_list_str += ",if=buff.bloodlust.react|target.time_to_die<=40|buff.celestial_alignment.up";
  }
  action_list_str += "/starfall,if=!buff.starfall.up";
  action_list_str += "/force_of_nature,if=talent.force_of_nature.enabled";
  if ( race == RACE_TROLL && level >= 68 )
    action_list_str += "/berserking,if=buff.celestial_alignment.up";
  else
    action_list_str += init_use_racial_actions();
  action_list_str += init_use_item_actions( ",if=buff.celestial_alignment.up|cooldown.celestial_alignment.remains>30" );
  action_list_str += init_use_profession_actions( ",if=buff.celestial_alignment.up|cooldown.celestial_alignment.remains>30" );
  action_list_str += "/wild_mushroom_detonate,moving=0,if=buff.wild_mushroom.stack>0&buff.solar_eclipse.up";
  action_list_str += "/healing_touch,if=talent.dream_of_cenarius.enabled&buff.dream_of_cenarius.up&mana.pct>25&cooldown.starsurge.remains>10"; // FIXME: DPS gain or not?
  action_list_str += "/incarnation,if=talent.incarnation.enabled&(buff.lunar_eclipse.up|buff.solar_eclipse.up)";
  action_list_str += "/celestial_alignment,if=(!buff.lunar_eclipse.up&!buff.solar_eclipse.up)&(buff.chosen_of_elune.up|!talent.incarnation.enabled|cooldown.incarnation.remains>10)";
  action_list_str += "/natures_vigil,if=talent.natures_vigil.enabled";
  add_action( "Starsurge", "if=buff.shooting_stars.react&(active_enemies<5|!buff.solar_eclipse.up)" );
  action_list_str += "/moonfire,cycle_targets=1,if=buff.lunar_eclipse.up&(remains<(buff.natures_grace.remains-2+2*set_bonus.tier14_4pc_caster))";
  action_list_str += "/sunfire,cycle_targets=1,if=buff.solar_eclipse.up&(remains<(buff.natures_grace.remains-2+2*set_bonus.tier14_4pc_caster))";
  action_list_str += "/hurricane,if=active_enemies>4&buff.solar_eclipse.up&buff.natures_grace.up";
  action_list_str += "/moonfire,cycle_targets=1,if=active_enemies<5&(remains<(buff.natures_grace.remains-2+2*set_bonus.tier14_4pc_caster))";
  action_list_str += "/sunfire,cycle_targets=1,if=active_enemies<5&(remains<(buff.natures_grace.remains-2+2*set_bonus.tier14_4pc_caster))";
  action_list_str += "/hurricane,if=active_enemies>5&buff.solar_eclipse.up&mana.pct>25";
  action_list_str += "/moonfire,cycle_targets=1,if=buff.lunar_eclipse.up&ticks_remain<2";
  action_list_str += "/sunfire,cycle_targets=1,if=buff.solar_eclipse.up&ticks_remain<2";
  action_list_str += "/hurricane,if=active_enemies>4&buff.solar_eclipse.up&mana.pct>25";
  action_list_str += "/starsurge,if=cooldown_react";
  action_list_str += "/starfire,if=buff.celestial_alignment.up&cast_time<buff.celestial_alignment.remains";
  action_list_str += "/wrath,if=buff.celestial_alignment.up&cast_time<buff.celestial_alignment.remains";
  action_list_str += "/starfire,if=eclipse_dir=1|(eclipse_dir=0&eclipse>0)";
  action_list_str += "/wrath,if=eclipse_dir=-1|(eclipse_dir=0&eclipse<=0)";
  action_list_str += "/moonfire,moving=1,cycle_targets=1,if=ticks_remain<2";
  action_list_str += "/sunfire,moving=1,cycle_targets=1,if=ticks_remain<2";
  action_list_str += "/wild_mushroom,moving=1,if=buff.wild_mushroom.stack<buff.wild_mushroom.max_stack";
  action_list_str += "/starsurge,moving=1,if=buff.shooting_stars.react";
  action_list_str += "/moonfire,moving=1,if=buff.lunar_eclipse.up";
  action_list_str += "/sunfire,moving=1";
}

// Guardian Combat Action Priority List ==============================

void druid_t::apl_guardian()
{
  action_list_str += "/auto_attack";
  action_list_str += init_use_racial_actions();
  action_list_str += "/skull_bash_bear";
  add_action( "Frenzied Regeneration", "if=health.pct<100&action.savage_defense.charges=0&incoming_damage_5>0.2*health.max" );
  add_action( "Frenzied Regeneration", "if=health.pct<100&action.savage_defense.charges>0&incoming_damage_5>0.4*health.max" );
  add_action( "Savage Defense" );
  action_list_str += init_use_item_actions();
  action_list_str += init_use_profession_actions();
  add_action( "Barkskin" );
  action_list_str += "/renewal,if=talent.renewal.enabled&incoming_damage_5>0.8*health.max";
  action_list_str += "/natures_vigil,if=talent.natures_vigil.enabled&(!talent.incarnation.enabled|buff.son_of_ursoc.up|cooldown.incarnation.remains)";
  action_list_str += "/maul,if=buff.tooth_and_claw.react&buff.tooth_and_claw_absorb.down";
  action_list_str += "/lacerate,if=((dot.lacerate.remains<3)|(buff.lacerate.stack<3&dot.thrash_bear.remains>3))&(buff.son_of_ursoc.up|buff.berserk.up)";
  action_list_str += "/faerie_fire,if=debuff.weakened_armor.stack<3";
  action_list_str += "/thrash_bear,if=dot.thrash_bear.remains<3&(buff.son_of_ursoc.up|buff.berserk.up)";
  action_list_str += "/mangle";
  action_list_str += "/wait,sec=cooldown.mangle.remains,if=cooldown.mangle.remains<=0.5";
  action_list_str += "/cenarion_ward,if=talent.cenarion_ward.enabled";
  action_list_str += "/incarnation,if=talent.incarnation.enabled";
  action_list_str += "/lacerate,if=dot.lacerate.remains<3|buff.lacerate.stack<3";
  action_list_str += "/thrash_bear,if=dot.thrash_bear.remains<2";
  action_list_str += "/lacerate";
  action_list_str += "/faerie_fire,if=dot.thrash_bear.remains>6";
  action_list_str += "/thrash_bear";
}

// Restoration Combat Action Priority List ==============================

void druid_t::apl_restoration()
{
  if ( sim -> allow_potions && level >= 80 )
  {
    action_list_str += ( level > 85 ) ? "/jade_serpent_potion" : "/volcanic_potion";
    action_list_str += ",if=buff.bloodlust.react|target.time_to_die<=40";
  }
  action_list_str += init_use_racial_actions();
  action_list_str += init_use_item_actions();
  action_list_str += init_use_profession_actions();
  action_list_str += "/natures_swiftness";
  if ( talent.incarnation -> ok() )
    action_list_str += "/incarnation";
  action_list_str += "/healing_touch,if=buff.natures_swiftness.up";
  action_list_str += "/healing_touch,if=buff.omen_of_clarity.up";
  action_list_str += "/rejuvenation,if=!ticking|remains<tick_time";
  action_list_str += "/lifebloom,if=debuff.lifebloom.stack<3";
  action_list_str += "/swiftmend";
  action_list_str += "/healing_touch";
}

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time.total_seconds();

  // Technically weapon speed affects stormlash damage for feral and
  // guardian, but not a big enough deal to waste time simming it.
  scales_with[ STAT_WEAPON_SPEED  ] = false;

  if ( specialization() == DRUID_FERAL )
    scales_with[ STAT_SPIRIT ] = false;

  if ( specialization() == DRUID_GUARDIAN )
  {
    scales_with[ STAT_WEAPON_DPS ] = false;
    scales_with[ STAT_PARRY_RATING ] = false;
    scales_with[ STAT_BLOCK_RATING ] = false;
  }

  // Save a copy of the weapon
  caster_form_weapon = main_hand_weapon;
}

// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  gain.bear_melee            = get_gain( "bear_melee"            );
  gain.bear_form             = get_gain( "bear_form"             );
  gain.energy_refund         = get_gain( "energy_refund"         );
  gain.eclipse               = get_gain( "eclipse"               );
  gain.frenzied_regeneration = get_gain( "frenzied_regeneration" );
  gain.glyph_ferocious_bite  = get_gain( "glyph_ferocious_bite"  );
  gain.lotp_health           = get_gain( "lotp_health"           );
  gain.lotp_mana             = get_gain( "lotp_mana"             );
  gain.mangle                = get_gain( "mangle"                );
  gain.omen_of_clarity       = get_gain( "omen_of_clarity"       );
  gain.primal_fury           = get_gain( "primal_fury"           );
  gain.soul_of_the_forest    = get_gain( "soul_of_the_forest"    );
  gain.tigers_fury           = get_gain( "tigers_fury"           );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  proc.primal_fury              = get_proc( "primal_fury"            );
  proc.unaligned_eclipse_gain   = get_proc( "unaligned_eclipse_gain" );
  proc.wrong_eclipse_wrath      = get_proc( "wrong_eclipse_wrath"    );
  proc.wrong_eclipse_starfire   = get_proc( "wrong_eclipse_starfire" );
  proc.combo_points             = get_proc( "combo_points" );
  proc.combo_points_wasted      = get_proc( "combo_points_wasted" );
  proc.shooting_stars_wasted    = get_proc( "Shooting Stars overflow (buff already up)" );
  proc.tier15_2pc_melee         = get_proc( "tier15_2pc_melee"       );
  proc.tooth_and_claw           = get_proc( "tooth_and_claw"         );
}

// druid_t::init_benefits ===================================================

void druid_t::init_benefits()
{
  player_t::init_benefits();
}

// druid_t::init_actions ====================================================

void druid_t::init_action_list()
{
  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat(); // PRE-COMBAT

  switch ( specialization() )
  {
    case DRUID_FERAL:
      apl_feral(); // FERAL
      break;
    case DRUID_BALANCE:
      apl_balance();  // BALANCE
      break;
    case DRUID_GUARDIAN:
      apl_guardian(); // GUARDIAN
      break;
    case DRUID_RESTORATION:
      apl_restoration(); // RESTORATION
      break;
    default:
      apl_default(); // DEFAULT
      break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  inflight_starsurge = false;

  eclipse_bar_value     = 0;
  eclipse_bar_direction = 0;
  base_gcd = timespan_t::from_seconds( 1.5 );

  // Restore main hand attack / weapon to normal state
  main_hand_attack = 0;
  main_hand_weapon = caster_form_weapon;

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    druid_td_t* td = target_data[ sim -> actor_list[ i ] ];
    if ( td ) td -> reset();
  }
}

// druid_t::regen ===========================================================

void druid_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  // player_t::regen() only regens your primary resource, so we need to account for that here
  if ( primary_resource() != RESOURCE_MANA && mana_regen_per_second() )
    resource_gain( RESOURCE_MANA, mana_regen_per_second() * periodicity.total_seconds(), gains.mp5_regen );
  if ( primary_resource() != RESOURCE_ENERGY && energy_regen_per_second() )
    resource_gain( RESOURCE_ENERGY, energy_regen_per_second() * periodicity.total_seconds(), gains.energy_regen );
}

// druid_t::available =======================================================

timespan_t druid_t::available() const
{
  if ( primary_resource() != RESOURCE_ENERGY )
    return timespan_t::from_seconds( 0.1 );

  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 ) return timespan_t::from_seconds( 0.1 );

  return std::max(
           timespan_t::from_seconds( ( 25 - energy ) / energy_regen_per_second() ),
           timespan_t::from_seconds( 0.1 )
         );
}

// druid_t::combat_begin ====================================================

void druid_t::combat_begin()
{
  player_t::combat_begin();

  // Start the fight with 0 rage
  resources.current[ RESOURCE_RAGE ] = 0;

  if ( specialization() == DRUID_BALANCE )
  {
    int starting_eclipse = initial_eclipse;
    if ( starting_eclipse == 65535 )
      starting_eclipse = default_initial_eclipse;

    // If we start in the +, assume we want to to towards +100
    // Start at +100, trigger solar and put direction towards -
    // If we start in the -, assume we want to go towards -100
    // Start at -100, trigger lunar and put direction towards +
    // No Nature's Grace if we start the fight with eclipse

    if ( starting_eclipse >= 0 )
    {
      eclipse_bar_value = std::min( spec.eclipse -> effectN( 1 ).base_value(), starting_eclipse );
      if ( eclipse_bar_value == spec.eclipse -> effectN( 1 ).base_value() )
      {
        buff.eclipse_solar -> trigger();
        eclipse_bar_direction = -1;
      }
    }
    else
    {
      eclipse_bar_value = std::max( spec.eclipse -> effectN( 2 ).base_value(), starting_eclipse );
      if ( eclipse_bar_value == spec.eclipse -> effectN( 2 ).base_value() )
      {
        buff.eclipse_lunar -> trigger();
        eclipse_bar_direction = 1;
      }
    }
  }

  // If Ysera's Gift is talented, apply it upon entering combat
  if ( talent.yseras_gift -> ok() )
    active.yseras_gift -> execute();

  // If Savagery is talented, apply Savage Roar entering combat
  if ( talent.savagery -> ok() )
    buff.savage_roar -> trigger();
  
  // Apply Bladed Armor buff 
  if ( find_specialization_spell( "Bladed Armor" ) )
    buff.bladed_armor -> trigger();
}

// druid_t::invalidate_cache ================================================

void druid_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_AGILITY:
      if ( spec.nurturing_instinct -> ok() )
        player_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_INTELLECT:
      if ( spec.killer_instinct -> ok() )
        player_t::invalidate_cache( CACHE_AGILITY );
      break;
    case CACHE_MASTERY:
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      break;
    case CACHE_SPIRIT:
      if ( spec.balance_of_power -> ok() )
        player_t::invalidate_cache( CACHE_HIT );
      break;
    default: break;
  }
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form -> check() )
  {
    // Bear Form
    double bearMod = buff.bear_form -> data().effectN( 3 ).percent();
    if ( spec.thick_hide -> ok() )
      bearMod = spec.thick_hide -> effectN( 2 ).percent(); // Thick Hide changes the bear form multiplier TO x%.
    else if ( specialization() != DRUID_GUARDIAN )
      bearMod += glyph.ursols_defense -> effectN( 1 ).percent(); // Non-guardian glyph that adds to bear form multiplier.
    a *= 1.0 + bearMod;

    // Mastery: Nature's Guardian
    if ( mastery.natures_guardian -> ok() )
      a *= 1.0 + cache.mastery_value();
  }
  return a;
}

// druid_t::composite_player_multiplier =====================================

double druid_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && buff.cat_form -> up() )
  {
    m *= 1.0 + buff.tigers_fury -> value();
    m *= 1.0 + buff.savage_roar -> value();
  }

  if ( specialization() == DRUID_BALANCE )
  {
    if ( dbc::is_school( school, SCHOOL_SPELLSTORM ) )
    {
      if ( buff.eclipse_lunar -> up() || buff.eclipse_solar -> up() )
      {
        m *= 1.0 + buff.eclipse_lunar -> data().effectN( 1 ).percent()
                 + mastery.total_eclipse -> ok() * cache.mastery_value();
      }
    }
    else if ( dbc::is_school( school, SCHOOL_ARCANE ) )
    {
      if ( buff.eclipse_lunar -> up() )
      {
        m *= 1.0 + buff.eclipse_lunar -> data().effectN( 1 ).percent()
                 + mastery.total_eclipse -> ok() * cache.mastery_value();
      }
    }
    else if ( dbc::is_school( school, SCHOOL_NATURE ) )
    {
      if ( buff.eclipse_solar -> up() )
      {
        m *= 1.0 + buff.eclipse_solar -> data().effectN( 1 ).percent()
                 + mastery.total_eclipse -> ok() * cache.mastery_value();
      }
    }

    if ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_NATURE ) )
    {
      if ( buff.moonkin_form -> check() )
        m *= 1.0 + spell.moonkin_form -> effectN( 2 ).percent();

      // BUG? Incarnation won't apply during CA!
      if ( buff.chosen_of_elune -> up() &&
           ( buff.eclipse_lunar -> check() || buff.eclipse_solar -> check() ) )
      {
        m *= 1.0 + buff.chosen_of_elune -> data().effectN( 1 ).percent();
      }
    }
  }

  return m;
}

double druid_t::composite_player_td_multiplier( school_e school,  const action_t* a ) const
{
  double m = player_t::composite_player_td_multiplier( school, a );

  if ( school == SCHOOL_PHYSICAL && mastery.razor_claws -> ok() && buff.cat_form -> up()
    && a -> id != 146194 ) // Blacklist for Fen-yu Legendary cloak proc
    m *= 1.0 + cache.mastery_value();

  return m;
}

// druid_t::composite_player_heal_multiplier ================================

double druid_t::composite_player_heal_multiplier( school_e school ) const
{
  double m = player_t::composite_player_heal_multiplier( school );

  m *= 1.0 + buff.heart_of_the_wild -> heal_multiplier();

  return m;
}

// druid_t::composite_melee_attack_power ==================================

double druid_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += buff.bladed_armor -> data().effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// druid_t::composite_melee_crit ============================================

double druid_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();

  crit *= 1.0 + spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::composite_attack_hit ============================================

double druid_t::composite_melee_hit() const
{
  double hit = player_t::composite_melee_hit();

  if ( spec.balance_of_power -> ok() )
    hit += ( cache.spirit() - base.stats.get_stat( STAT_SPIRIT ) ) * ( spec.balance_of_power -> effectN( 1 ).percent() ) / current_rating().spell_hit;

  hit += buff.heart_of_the_wild -> attack_hit_expertise();

  return hit;
}

// druid_t::composite_attack_expertise ======================================

double druid_t::composite_melee_expertise( weapon_t* w ) const
{
  double exp = player_t::composite_melee_expertise( w );

  exp += buff.heart_of_the_wild -> attack_hit_expertise();

  return exp;
}

// druid_t::temporary_movement_modifier =========================================

double druid_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

   if( buff.dash -> up() )
     active = std::max( active, buff.dash -> data().effectN( 1 ).percent() );

  return active;
}

// druid_t::passive_movement_modifier ========================================

double druid_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( buff.cat_form -> up() )
  {
    ms += find_spell( 113636 ) -> effectN( 1 ).percent();
    if ( perk.enhanced_cat_form -> ok() )
      ms += perk.enhanced_cat_form -> effectN( 1 ).percent();
    if ( buff.prowl -> up() && ! perk.enhanced_prowl -> ok() )
      ms += buff.prowl -> data().effectN( 2 ).percent();
  }

  if ( talent.feline_swiftness -> ok() )
    ms += talent.feline_swiftness -> effectN( 1 ).percent();

  return ms;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit *= 1.0 + spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::composite_spell_hit =============================================

double druid_t::composite_spell_hit() const
{
  double hit = player_t::composite_spell_hit();

  if ( spec.balance_of_power -> ok() )
    hit += ( cache.spirit() - base.stats.get_stat( STAT_SPIRIT ) ) * ( spec.balance_of_power -> effectN( 1 ).percent() ) / current_rating().spell_hit;

  hit += buff.heart_of_the_wild -> spell_hit();

  return hit;
}

// druid_t::composite_spell_power ===========================================

double druid_t::composite_spell_power( school_e school ) const
{
  double p = player_t::composite_spell_power( school );

  switch ( school )
  {
    case SCHOOL_NATURE:
      if ( spec.nurturing_instinct -> ok() )
        p += spec.nurturing_instinct -> effectN( 1 ).percent() * player_t::composite_attribute( ATTR_AGILITY );
      break;
    default:
      break;
  }

  return p;
}

// druid_t::composite_attribute =============================================

double druid_t::composite_attribute( attribute_e attr ) const
{
  double a = player_t::composite_attribute( attr );

  switch ( attr )
  {
    case ATTR_AGILITY:
      if ( spec.killer_instinct -> ok() && ( buff.bear_form -> up() || buff.cat_form -> up() ) )
        a += spec.killer_instinct -> effectN( 1 ).percent() * player_t::composite_attribute( ATTR_INTELLECT );
      break;
    default:
      break;
  }

  return a;
}

// druid_t::composite_attribute_multiplier ==================================

double druid_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  switch ( attr )
  {
    case ATTR_STAMINA:
      if ( buff.bear_form -> check() )
        m *= 1.0 + spell.bear_form -> effectN( 2 ).percent();
      break;
    case ATTR_AGILITY:
      m *= 1.0 + buff.heart_of_the_wild -> agility_multiplier();
      break;
    default:
      break;
  }

  return m;
}

// druid_t::matching_gear_multiplier ========================================

double druid_t::matching_gear_multiplier( attribute_e attr ) const
{
  unsigned idx;

  switch ( attr )
  {
    case ATTR_AGILITY:
      idx = 1;
      break;
    case ATTR_INTELLECT:
      idx = 2;
      break;
    case ATTR_STAMINA:
      idx = 3;
      break;
    default:
      return 0;
  }

  return spec.leather_specialization -> effectN( idx ).percent();
}

// druid_t::composite_tank_crit =============================================

double druid_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.thick_hide -> effectN( 1 ).percent();

  return c;
}

// druid_t::composite_tank_dodge ============================================

double druid_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  if ( buff.savage_defense -> up() )
  {
    d += buff.savage_defense -> data().effectN( 1 ).percent();
    // TODO: Add Savage Defense dodge bonus granted by 4pT14 Guardian bonus.
  }

  return d;
}

// druid_t::composite_rating_multiplier =====================================

double druid_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      if ( buff.bear_form -> up() )
        m *= 1.0 + spell.bear_form -> effectN( 4 ).percent();
      break;
    case RATING_SPELL_CRIT:
    case RATING_MELEE_CRIT:
    case RATING_RANGED_CRIT:
      if ( buff.bear_form -> up() )
        m *= 1.0 + spell.bear_form -> effectN( 4 ).percent();
      break;
    default:
      break;
  }

  return m;
}

// druid_t::create_expression ===============================================

expr_t* druid_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "eclipse" )
  {
    return make_ref_expr( "eclipse", eclipse_bar_value );
  }
  else if ( name_str == "eclipse_dir" )
  {
    return make_ref_expr( "eclipse_dir", eclipse_bar_direction );
  }
  else if ( util::str_compare_ci( name_str, "combo_points" ) )
  {
    // If an action targets the druid, but checks for combo points, check
    // sim -> target instead. Quick fix so HT can use combo_points
    druid_td_t* td = get_target_data( ( a -> target == this ) ? sim -> target : a -> target  );
    return td -> combo_points.count_expr();
  }

  return player_t::create_expression( a, name_str );
}

// druid_t::create_options ==================================================

void druid_t::create_options()
{
  player_t::create_options();

  option_t druid_options[] =
  {
    opt_int( "initial_eclipse", initial_eclipse ),
    opt_null()
  };

  option_t::copy( options, druid_options );
}

// druid_t::create_profile ==================================================

bool druid_t::create_profile( std::string& profile_str, save_e type, bool save_html )
{
  player_t::create_profile( profile_str, type, save_html );

  if ( type == SAVE_ALL )
  {
    if ( specialization() == DRUID_BALANCE )
    {
      if ( initial_eclipse != 65535 )
      {
        profile_str += "initial_eclipse=";
        profile_str += util::to_string( initial_eclipse );
        profile_str += "\n";
      }
    }
  }

  return true;
}

// druid_t::decode_set ======================================================

set_e druid_t::decode_set( const item_t& item ) const
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

  if ( strstr( s, "deep_earth" ) )
  {
    bool is_caster = ( strstr( s, "cover"         ) ||
                       strstr( s, "shoulderwraps" ) ||
                       strstr( s, "vestment"      ) ||
                       strstr( s, "leggings"      ) ||
                       strstr( s, "gloves"        ) );

    bool is_melee = ( strstr( s, "headpiece"      ) ||
                      strstr( s, "spaulders"      ) ||
                      strstr( s, "raiment"        ) ||
                      strstr( s, "legguards"      ) ||
                      strstr( s, "grips"          ) );

    bool is_healer = ( strstr( s, "helm"          ) ||
                       strstr( s, "mantle"        ) ||
                       strstr( s, "robes"         ) ||
                       strstr( s, "legwraps"      ) ||
                       strstr( s, "handwraps"     ) );
    if ( is_caster ) return SET_T13_CASTER;
    if ( is_melee  ) return SET_T13_MELEE;
    if ( is_healer ) return SET_T13_HEAL;
  }

  if ( strstr( s, "eternal_blossom" ) )
  {
    bool is_caster = ( strstr( s, "cover"          ) ||
                       strstr( s, "shoulderwraps"  ) ||
                       strstr( s, "vestment"       ) ||
                       strstr( s, "leggings"       ) ||
                       strstr( s, "gloves"         ) );

    bool is_melee = ( strstr( s, "headpiece"       ) ||
                      strstr( s, "spaulders"       ) ||
                      strstr( s, "raiment"         ) ||
                      strstr( s, "legguards"       ) ||
                      strstr( s, "grips"           ) );

    bool is_healer = ( strstr( s, "helm"           ) ||
                       strstr( s, "mantle"         ) ||
                       strstr( s, "robes"          ) ||
                       strstr( s, "legwraps"       ) ||
                       strstr( s, "handwraps"      ) );

    bool is_tank   = ( strstr( s, "headguard"      ) ||
                       strstr( s, "shoulderguards" ) ||
                       strstr( s, "tunic"          ) ||
                       strstr( s, "breeches"       ) ||
                       strstr( s, "handguards"     ) );
    if ( is_caster ) return SET_T14_CASTER;
    if ( is_melee  ) return SET_T14_MELEE;
    if ( is_healer ) return SET_T14_HEAL;
    if ( is_tank   ) return SET_T14_TANK;
  }

  if ( strstr( s, "_of_the_haunted_forest" ) )
  {
    bool is_caster = ( strstr( s, "cover"          ) ||
                       strstr( s, "shoulderwraps"  ) ||
                       strstr( s, "vestment"       ) ||
                       strstr( s, "leggings"       ) ||
                       strstr( s, "gloves"         ) );

    bool is_melee = ( strstr( s, "headpiece"       ) ||
                      strstr( s, "spaulders"       ) ||
                      strstr( s, "raiment"         ) ||
                      strstr( s, "legguards"       ) ||
                      strstr( s, "grips"           ) );

    bool is_healer = ( strstr( s, "helm"           ) ||
                       strstr( s, "mantle"         ) ||
                       strstr( s, "robes"          ) ||
                       strstr( s, "legwraps"       ) ||
                       strstr( s, "handwraps"      ) );

    bool is_tank   = ( strstr( s, "headguard"      ) ||
                       strstr( s, "shoulderguards" ) ||
                       strstr( s, "tunic"          ) ||
                       strstr( s, "breeches"       ) ||
                       strstr( s, "handguards"     ) );
    if ( is_caster ) return SET_T15_CASTER;
    if ( is_melee  ) return SET_T15_MELEE;
    if ( is_healer ) return SET_T15_HEAL;
    if ( is_tank   ) return SET_T15_TANK;
  }

  if ( strstr( s, "of_the_shattered_vale" ) )
  {
    bool is_caster = ( strstr( s, "cover"          ) ||
                       strstr( s, "shoulderwraps"  ) ||
                       strstr( s, "vestment"       ) ||
                       strstr( s, "leggings"       ) ||
                       strstr( s, "gloves"         ) );

    bool is_melee = ( strstr( s, "headpiece"       ) ||
                      strstr( s, "spaulders"       ) ||
                      strstr( s, "raiment"         ) ||
                      strstr( s, "legguards"       ) ||
                      strstr( s, "grips"           ) );

    bool is_healer = ( strstr( s, "helm"           ) ||
                       strstr( s, "mantle"         ) ||
                       strstr( s, "robes"          ) ||
                       strstr( s, "legwraps"       ) ||
                       strstr( s, "handwraps"      ) );

    bool is_tank   = ( strstr( s, "headguard"      ) ||
                       strstr( s, "shoulderguards" ) ||
                       strstr( s, "tunic"          ) ||
                       strstr( s, "breeches"       ) ||
                       strstr( s, "handguards"     ) );
    if ( is_caster ) return SET_T16_CASTER;
    if ( is_melee  ) return SET_T16_MELEE;
    if ( is_healer ) return SET_T16_HEAL;
    if ( is_tank   ) return SET_T16_TANK;
  }

  if ( strstr( s, "_gladiators_kodohide_"   ) )   return SET_PVP_HEAL;
  if ( strstr( s, "_gladiators_wyrmhide_"   ) )   return SET_PVP_CASTER;
  if ( strstr( s, "_gladiators_dragonhide_" ) )   return SET_PVP_MELEE;

  return SET_NONE;
}

// druid_t::primary_role ====================================================

role_e druid_t::primary_role() const
{
  if ( specialization() == DRUID_BALANCE )
  {
    if ( player_t::primary_role() == ROLE_HEAL )
      return ROLE_HEAL;

    return ROLE_SPELL;
  }

  else if ( specialization() == DRUID_FERAL )
  {
    if ( player_t::primary_role() == ROLE_TANK )
      return ROLE_TANK;

    return ROLE_ATTACK;
  }

  else if ( specialization() == DRUID_GUARDIAN )
  {
    if ( player_t::primary_role() == ROLE_ATTACK )
      return ROLE_ATTACK;

    return ROLE_TANK;
  }

  else if ( specialization() == DRUID_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_HEAL;
  }

  return player_t::primary_role();
}

// druid_t::convert_hybrid_stat ==============================================

stat_e druid_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_AGI_INT: 
    if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
      return STAT_INTELLECT;
    else
      return STAT_AGILITY; 
  // This is a guess at how AGI/STR gear will work for Balance/Resto, TODO: confirm  
  case STAT_STR_AGI:
    return STAT_AGILITY;
  // This is a guess at how STR/INT gear will work for Feral/Guardian, TODO: confirm  
  // This should probably never come up since druids can't equip plate, but....
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
    if ( specialization() == DRUID_RESTORATION )
      return s;
    else
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
    if ( specialization() == DRUID_GUARDIAN )
      return s;
    else
      return STAT_NONE;     
  default: return s; 
  }
}

// druid_t::primary_resource ================================================

resource_e druid_t::primary_resource() const
{
  if ( primary_role() == ROLE_SPELL || primary_role() == ROLE_HEAL )
    return RESOURCE_MANA;

  if ( primary_role() == ROLE_TANK )
    return RESOURCE_RAGE;

  return RESOURCE_ENERGY;
}

// druid_t::assess_damage ===================================================

void druid_t::assess_damage( school_e school,
                             dmg_e    dtype,
                             action_state_t* s )
{
  if ( sets.has_set_bonus( SET_T15_2PC_TANK ) && s -> result == RESULT_DODGE && buff.savage_defense -> check() )
    buff.tier15_2pc_tank -> trigger();

  if ( buff.barkskin -> up() )
    s -> result_amount *= 1.0 + buff.barkskin -> value();

  if ( buff.survival_instincts -> up() )
    s -> result_amount *= 1.0 + buff.survival_instincts -> value();

  if ( glyph.ninth_life -> ok() )
    s -> result_amount *= 1.0 + glyph.ninth_life -> effectN( 1 ).base_value();

  if ( spec.thick_hide -> ok() )
  {
    if ( school == SCHOOL_PHYSICAL )
      s -> result_amount *= 1.0 + spec.thick_hide -> effectN( 5 ).percent();
    else if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
      s -> result_amount *= 1.0 + spec.thick_hide -> effectN( 3 ).percent();
  }

  if ( buff.cenarion_ward -> up() && s -> result_amount > 0 )
     active.cenarion_ward_hot -> execute();

  // Call here to benefit from -10% physical damage before SD is taken into account
  player_t::assess_damage( school, dtype, s );
}

// druid_t::assess_heal =====================================================

void druid_t::assess_heal( school_e school,
                           dmg_e    dmg_type,
                           action_state_t* s )
{
  s -> result_amount *= 1.0 + buff.frenzied_regeneration -> check() * glyph.frenzied_regeneration -> effectN( 1 ).percent();
  s -> result_amount *= 1.0 + buff.cat_form -> check() * glyph.cat_form -> effectN( 1 ).percent();

  player_t::assess_heal( school, dmg_type, s );
}

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_pair_t( &target, &source ),
    dots( dots_t() ),
    buffs( buffs_t() ),
    lacerate_stack( 1 ),
    combo_points( source, target )
{
  dots.lacerate     = target.get_dot( "lacerate",     &source );
  dots.lifebloom    = target.get_dot( "lifebloom",    &source );
  dots.moonfire     = target.get_dot( "moonfire",     &source );
  dots.rake         = target.get_dot( "rake",         &source );
  dots.regrowth     = target.get_dot( "regrowth",     &source );
  dots.rejuvenation = target.get_dot( "rejuvenation", &source );
  dots.rip          = target.get_dot( "rip",          &source );
  dots.sunfire      = target.get_dot( "sunfire",      &source );
  dots.wild_growth  = target.get_dot( "wild_growth",  &source );

  buffs.lifebloom = buff_creator_t( *this, "lifebloom", source.find_class_spell( "Lifebloom" ) );
}

// ==========================================================================
// Combo Point System Functions
// ==========================================================================

combo_points_t::combo_points_t( druid_t& source, player_t& target ) :
  source( source ),
  target( target ),
  proc( nullptr ),
  wasted( nullptr ),
  count( 0 )
{
  proc   = target.get_proc( source.name_str + ": combo_points" );
  wasted = target.get_proc( source.name_str + ": combo_points_wasted" );
}

void combo_points_t::add( int num, const std::string* source_name )
{
  int actual_num = clamp( num, 0, max_combo_points - count );
  int overflow   = num - actual_num;

  // we count all combo points gained in the proc
  for ( int i = 0; i < num; i++ )
    proc -> occur();

  // add actual combo points
  if ( actual_num > 0 )
  {
    count += actual_num;
    source.trigger_ready();
  }

  // count wasted combo points
  for ( int i = 0; i < overflow; i++ )
    wasted -> occur();

  sim_t& sim = *source.sim;
  if ( sim.log )
  {
    if ( source_name )
    {
      if ( actual_num > 0 )
        sim.out_log.printf( "%s gains %d (%d) combo_points from %s (%d)",
                    target.name(), actual_num, num, source_name -> c_str(), count );
    }
    else
    {
      if ( actual_num > 0 )
        sim.out_log.printf( "%s gains %d (%d) combo_points (%d)",
                    target.name(), actual_num, num, count );
    }
  }
}

int combo_points_t::consume( const std::string* source_name )
{
  if ( source.sim -> log )
  {
    if ( source_name )
      source.sim -> out_log.printf( "%s spends %d combo_points on %s",
                            target.name(), count, source_name -> c_str() );
    else
      source.sim -> out_log.printf( "%s loses %d combo_points",
                            target.name(), count );
  }

  int tmp_count = count;
  count = 0;
  return tmp_count;
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t : public player_report_extension_t
{
public:
  druid_report_t( druid_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  druid_t& p;
};

// DRUID MODULE INTERFACE ===================================================

struct druid_module_t : public module_t
{
  druid_module_t() : module_t( DRUID ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    druid_t* p = new druid_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new druid_report_t( *p ) );
    return p;
  }
  virtual bool valid() const { return true; }
  virtual void init( sim_t* sim ) const
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      p -> buffs.stampeding_shout        = buff_creator_t( p, "stampeding_shout", p -> find_spell( 77764 ) )
                                          .max_stack( 1 )
                                          .duration( timespan_t::from_seconds( 8.0 ) );
    }
  }
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}
