
// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
/*
]NOTES:

- To evaluate Combo Strikes in the APL, use:
    if=combo_break    true if action is a repeat and can combo strike
    if=combo_strike   true if action is not a repeat or can't combo strike

- To show CJL can be interupted in the APL, use:
     &!prev_gcd.crackling_jade_lightning,interrupt=1

TODO:

GENERAL:
- See other options of modeling Spinning Crane Kick

WINDWALKER:
- Add Cyclone Strike Counter as an expression
- See about removing tick part of Crackling Tiger Lightning

MISTWEAVER:
- Essence Font - See if the implementation can be corrected to the intended design.
- Life Cocoon - Double check if the Enveloping Mists and Renewing Mists from Mists of Life proc the mastery or not.
- Not Modeled:

BREWMASTER:
*/
#include "sc_monk.hpp"

#include "action/action_callback.hpp"
#include "class_modules/apl/apl_monk.hpp"
#include "player/pet.hpp"
#include "player/pet_spawner.hpp"
#include "report/charts.hpp"
#include "report/highchart.hpp"
#include "sc_enums.hpp"

#include <deque>

#include "simulationcraft.hpp"

// ==========================================================================
// Monk
// ==========================================================================

namespace monk
{
namespace actions
{
// ==========================================================================
// Monk Actions
// ==========================================================================
// Template for common monk action code. See priest_action_t.

template <class Base>
struct monk_action_t : public Base
{
  sef_ability_e sef_ability;
  // Whether the ability is affected by the Windwalker's Mastery.
  bool ww_mastery;
  // Whether the ability triggers Windwalker's Combo Strike
  bool may_combo_strike;
  // Whether the ability triggers Invoke Chi-Ji Gust's of Mist
  bool trigger_chiji;
  // Whether the ability can reset Faeline Stomp
  bool trigger_faeline_stomp;
  // Whether the ability can trigger the Legendary Bountiful Brew.
  bool trigger_bountiful_brew;
  // Whether the ability can trigger the Legendary Sinister Teaching Cooldown Reduction
  bool trigger_sinister_teaching_cdr;
  // Whether the ability can trigger Resonant Fists
  bool trigger_resonant_fists;
  // Whether the ability can be used during Spinning Crane Kick
  bool cast_during_sck;

  // Bron's Call to Action trigger overrides
  bool may_proc_bron;
  proc_t* bron_proc;

  // Keefer's Skyreach trigger
  proc_t* keefers_skyreach_proc;

  // Affect flags for various dynamic effects
  struct
  {
    bool serenity;
  } affected_by;

private:
  std::array<resource_e, MONK_MISTWEAVER + 1> _resource_by_stance;
  using ab = Base;  // action base, eg. spell_t
public:
  using base_t = monk_action_t<Base>;

  monk_action_t( util::string_view n, monk_t* player, const spell_data_t* s = spell_data_t::nil() )
    : ab( n, player, s ),
      sef_ability( sef_ability_e::SEF_NONE ),
      ww_mastery( false ),
      may_combo_strike( false ),
      trigger_chiji( false ),
      trigger_faeline_stomp( false ),
      trigger_bountiful_brew( false ),
      trigger_sinister_teaching_cdr( true ),
      trigger_resonant_fists( false ),
      cast_during_sck( false ),
      may_proc_bron( false ),
      bron_proc( nullptr ),
      keefers_skyreach_proc( nullptr ),
      affected_by()
  {
    range::fill( _resource_by_stance, RESOURCE_MAX );
    // Resonant Fists talent
    if ( player->talent.general.resonant_fists->ok() )
    {
      auto trigger = player->talent.general.resonant_fists.spell();

      trigger_resonant_fists = ab::harmful && ab::may_hit && ( trigger->proc_flags() & ( 1 << ab::proc_type() ) );
    }
  }

  std::string full_name() const
  {
    std::string n = ab::data().name_cstr();
    return n.empty() ? ab::name_str : n;
  }

  monk_t* p()
  {
    return debug_cast<monk_t*>( ab::player );
  }
  const monk_t* p() const
  {
    return debug_cast<monk_t*>( ab::player );
  }

  monk_td_t* get_td( player_t* t ) const
  {
    return p()->get_target_data( t );
  }

  const monk_td_t* find_td( player_t* t ) const
  {
    return p()->find_target_data( t );
  }

  // Utility function to search spell data for matching effect.
  // NOTE: This will return the FIRST effect that matches parameters.
  const spelleffect_data_t* find_spelleffect( const spell_data_t* spell, effect_subtype_t subtype,
                                                        int misc_value, const spell_data_t* affected,
                                                        effect_type_t type )
  {
    for ( size_t i = 1; i <= spell->effect_count(); i++ )
    {
      const auto& eff = spell->effectN( i );

      if ( affected->ok() && !affected->affected_by_all( eff ) )
        continue;

      if ( eff.type() == type && eff.subtype() == subtype )
      {
        if ( misc_value != 0 )
        {
          if ( eff.misc_value1() == misc_value )
            return &eff;
        }
        else
          return &eff;
      }
    }

    return &spelleffect_data_t::nil();
  }

  // Return the appropriate spell when `base` is overriden to another spell by `passive`
  const spell_data_t* find_spell_override( const spell_data_t* base, const spell_data_t* passive )
  {
    if ( !passive->ok() )
      return base;

    auto id = as<unsigned>( find_spelleffect( passive, A_OVERRIDE_ACTION_SPELL, base->id() )->base_value() );
    if ( !id )
      return base;

    return find_spell( id );
  }


  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    if ( name_str == "combo_strike" )
      return make_mem_fn_expr( name_str, *this, &monk_action_t::is_combo_strike );
    else if ( name_str == "combo_break" )
      return make_mem_fn_expr( name_str, *this, &monk_action_t::is_combo_break );
    else if ( name_str == "cap_energy" )
      return make_mem_fn_expr( name_str, *this, &monk_action_t::cap_energy );
    else if ( name_str == "tp_fill" )
      return make_mem_fn_expr( name_str, *this, &monk_action_t::tp_fill );
    return ab::create_expression( name_str );
  }

  bool ready() override
  {
    return ab::ready();
  }

  void init() override
  {
    ab::init();

    /* Iterate through power entries, and find if there are resources linked to one of our stances
     */
    for ( const spellpower_data_t& pd : ab::data().powers() )
    {
      switch ( pd.aura_id() )
      {
        case 137023:
          assert( _resource_by_stance[ dbc::spec_idx( MONK_BREWMASTER ) ] == RESOURCE_MAX &&
                  "Two power entries per aura id." );
          _resource_by_stance[ dbc::spec_idx( MONK_BREWMASTER ) ] = pd.resource();
          break;
        case 137024:
          assert( _resource_by_stance[ dbc::spec_idx( MONK_MISTWEAVER ) ] == RESOURCE_MAX &&
                  "Two power entries per aura id." );
          _resource_by_stance[ dbc::spec_idx( MONK_MISTWEAVER ) ] = pd.resource();
          break;
        case 137025:
          assert( _resource_by_stance[ dbc::spec_idx( MONK_WINDWALKER ) ] == RESOURCE_MAX &&
                  "Two power entries per aura id." );
          _resource_by_stance[ dbc::spec_idx( MONK_WINDWALKER ) ] = pd.resource();
          break;
        default:
          break;
      }
    }

    // If may_proc_bron is not overridden to trigger, check if it can trigger
    if ( !may_proc_bron )
      may_proc_bron = !this->background &&
                      ( this->spell_power_mod.direct || this->spell_power_mod.tick || this->attack_power_mod.direct ||
                        this->attack_power_mod.tick || this->base_dd_min || this->base_dd_max || this->base_td );

    // Resonant Fists talent
    if ( p()->talent.general.resonant_fists->ok() )
    {
      auto trigger = p()->talent.general.resonant_fists.spell();

      trigger_resonant_fists = ab::harmful && ab::may_hit &&
        ( trigger->proc_flags() & ( 1 << ab::proc_type() ) );
    }
  }

  void init_finished() override
  {
    ab::init_finished();

    // Allow this ability to be cast during SCK
    if ( this->cast_during_sck && !this->background && !this->dual )
    {
      if ( this->usable_while_casting )
      {
        this->cast_during_sck = false;
        p()->sim->print_debug( "{}: cast_during_sck ignored because usable_while_casting = true", this->full_name() );
      }
      else
      {
        this->usable_while_casting = true;
        this->use_while_casting    = true;
      }
    }

    if ( may_proc_bron )
      bron_proc = p()->get_proc( std::string( "Bron's Call to Action: " ) + full_name() );

    if ( ab::data().affected_by( p()->passives.keefers_skyreach_debuff->effectN( 1 ) ) )
      keefers_skyreach_proc = p()->get_proc( std::string( "Keefer's Skyreach: " ) + full_name() );
  }

  void reset_swing()
  {
    if ( p()->main_hand_attack && p()->main_hand_attack->execute_event )
    {
      p()->main_hand_attack->cancel();
      p()->main_hand_attack->schedule_execute();
    }
    if ( p()->off_hand_attack && p()->off_hand_attack->execute_event )
    {
      p()->off_hand_attack->cancel();
      p()->off_hand_attack->schedule_execute();
    }
  }

  resource_e current_resource() const override
  {
    if ( p()->specialization() == SPEC_NONE )
    {
      return ab::current_resource();
    }

    resource_e resource_by_stance = _resource_by_stance[ dbc::spec_idx( p()->specialization() ) ];

    if ( resource_by_stance == RESOURCE_MAX )
      return ab::current_resource();

    return resource_by_stance;
  }

  // Allow resource capping during BDB
  bool cap_energy()
  {
    return bonedust_brew_zone() == bonedust_brew_zone_results_e::CAP;
  }

  // Break mastery during BDB
  bool tp_fill()
  {
    auto result = bonedust_brew_zone();
    return ( ( result == bonedust_brew_zone_results_e::TP_FILL1 ) ||
             ( result == bonedust_brew_zone_results_e::TP_FILL2 ) );
  }

  // Check if the combo ability under consideration is different from the last
  bool is_combo_strike()
  {
    if ( !may_combo_strike )
      return false;

    // We don't know if the first attack is a combo or not, so assume it
    // is. If you change this, also change is_combo_break so that it
    // doesn't combo break on the first attack.
    if ( p()->combo_strike_actions.empty() )
      return true;

    if ( p()->combo_strike_actions.back()->id != this->id )
      return true;

    return false;
  }

  // This differs from combo_strike when the ability can't combo strike. In
  // that case both is_combo_strike and is_combo_break are false.
  bool is_combo_break()
  {
    if ( !may_combo_strike )
      return false;

    return !is_combo_strike();
  }

  // Trigger Windwalker's Combo Strike Mastery, the Hit Combo talent,
  // and other effects that trigger from combo strikes.
  // Triggers from execute() on abilities with may_combo_strike = true
  // Side effect: modifies combo_strike_actions
  void combo_strikes_trigger()
  {
    if ( !p()->mastery.combo_strikes->ok() )
      return;

    if ( is_combo_strike() )
    {
      p()->buff.combo_strikes->trigger();

      if ( p()->talent.windwalker.hit_combo->ok() )
        p()->buff.hit_combo->trigger();

      if ( p()->shared.xuens_bond && p()->shared.xuens_bond->ok() )
          p()->cooldown.invoke_xuen->adjust( p()->shared.xuens_bond->effectN( 2 ).time_value(), true );  // Saved as -100

      if ( p()->talent.windwalker.meridian_strikes->ok() )
          p()->cooldown.touch_of_death->adjust( -10 * p()->talent.windwalker.meridian_strikes->effectN( 2 ).time_value(), true ); // Saved as 35

      if ( p()->talent.windwalker.fury_of_xuen->ok() && p()->cooldown.fury_of_xuen->up() )
      {
        p()->cooldown.fury_of_xuen->start( p()->talent.windwalker.fury_of_xuen->internal_cooldown() );
        p()->buff.fury_of_xuen_stacks->trigger();
      }
    }
    else
    {
      p()->combo_strike_actions.clear();
      p()->buff.combo_strikes->expire();
      p()->buff.hit_combo->expire();
    }

    // Record the current action in the history.
    p()->combo_strike_actions.push_back( this );
  }

  // Reduces Brewmaster Brew cooldowns by the time given
  void brew_cooldown_reduction( double time_reduction )
  {
    // we need to adjust the cooldown time DOWNWARD instead of UPWARD so multiply the time_reduction by -1
    time_reduction *= -1;

    if ( p()->cooldown.purifying_brew->down() )
      p()->cooldown.purifying_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->cooldown.celestial_brew->down() )
      p()->cooldown.celestial_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->cooldown.fortifying_brew->down() )
      p()->cooldown.fortifying_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->cooldown.black_ox_brew->down() )
      p()->cooldown.black_ox_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( p()->shared.bonedust_brew && p()->shared.bonedust_brew->ok() && p()->cooldown.bonedust_brew->down() )
      p()->cooldown.bonedust_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

  }

 bonedust_brew_zone_results_e bonedust_brew_zone()
  {
    // This function is derived from the Google collab by Tostad0ra found here
    // https://colab.research.google.com/drive/1IlNnwzigBG_xa0VdXhiofvuy-mgJAhGa?usp=sharing

    std::vector<player_t*> target_list = p()->sim->target_non_sleeping_list.data();

    auto target_count           = static_cast<int>( target_list.size() );
    auto targets_affected       = 0;

    if ( p()->specialization() != MONK_WINDWALKER || target_count < 2 )
      return bonedust_brew_zone_results_e::NONE;

    for ( player_t* target : target_list )
    {
      if ( auto td = p()->find_target_data( target ) )
      {
        if ( td->debuff.bonedust_brew->check() )
          targets_affected++;
      }
    }

    if ( targets_affected == 0 )
      return bonedust_brew_zone_results_e::NONE;

    auto haste_bonus   = 1 / p()->composite_melee_haste();
    auto mastery_bonus = 1 + p()->composite_mastery_value();

    // Delay when chaining SCK (approximately 200ms from in game testing)
    auto player_delay = 0.2;

    // Tiger Palm 
    auto tiger_palm_AP_ratio_by_aura = p()->spec.tiger_palm->effectN( 1 ).ap_coeff() *
                                       ( 1 + p()->spec.windwalker_monk->effectN( 1 ).percent() ) *
                                       ( 1 + p()->spec.windwalker_monk->effectN( 5 ).percent() ) *
                                       ( 1 + p()->spec.windwalker_monk->effectN( 16 ).percent() ) *
                                       ( 1 + p()->spec.windwalker_monk->effectN( 18 ).percent() );
  
    // SCK
    auto SCK_AP_ratio_by_aura = p()->spec.spinning_crane_kick->effectN( 1 ).trigger()->effectN( 1 ).ap_coeff() *
                                        // 4 ticks
                                        4 * ( 1 + p()->spec.windwalker_monk->effectN( 2 ).percent() ) *
                                        ( 1 + p()->spec.windwalker_monk->effectN( 8 ).percent() ) *
                                        ( 1 + p()->spec.windwalker_monk->effectN( 22 ).percent() );

    // SQRT Scaling
    if ( target_count > p()->spec.spinning_crane_kick->effectN( 1 ).base_value() )
      SCK_AP_ratio_by_aura *= std::sqrt( p()->spec.spinning_crane_kick->effectN( 1 ).base_value() /
                                         std::min<int>( p()->sim->max_aoe_enemies, target_count ) );

    SCK_AP_ratio_by_aura *= p()->sck_modifier();

    auto SCK_dmg_total = ( target_count * SCK_AP_ratio_by_aura );
    auto tp_over_sck = tiger_palm_AP_ratio_by_aura / SCK_dmg_total;

    // Damage amplifiers

    auto total_damage_amplifier = 1 + p()->composite_damage_versatility();

    if ( p()->buff.invoke_xuen->check() )
    {
      // Tiger Lightning
      total_damage_amplifier *= ( 1 + p()->talent.windwalker.empowered_tiger_lightning->effectN( 2 ).percent() );
    }

    if ( p()->buff.storm_earth_and_fire->check() )
    {
      // Base 135% from SEF
      auto sef_multiplier = ( 1 + p()->talent.windwalker.storm_earth_and_fire->effectN( 1 ).percent() ) * 3;
      total_damage_amplifier *= sef_multiplier;

      // Coordinated Offensive
      if ( p()->conduit.coordinated_offensive->ok() )
        total_damage_amplifier *= ( 2 * ( 1 + p()->conduit.coordinated_offensive.percent() ) + 1 ) / 3;
    }

    total_damage_amplifier *=
        1 + ( p()->covenant.necrolord->proc_chance() *                                                           // Chance for bonus damage from Bonedust Brew
              p()->covenant.necrolord->effectN( 1 ).percent() *                                                 // Damage amplifier from Bonedust Brew
              ( 1 + ( p()->conduit.bone_marrow_hops->ok()
                          ? p()->conduit.bone_marrow_hops.percent()                                            // Damage amplifier from Bone Marrow Hops conduit
                          : 0 ) ) *  
          
              ( static_cast<double>( targets_affected ) / target_count ) );                                   // Delta targets affected by Bonedust Brew

    // Generate a lambda to refactor these expressions for ease of use and legibility
    // TODO:
    // all of this could maybe be automated to recieve
    // any combination of monk_spell_t structures but we only care about these 3 scenarios currently

    struct Action
    {
      double idps;
      double rdps;
    };

    auto DefineAction = [ &, this ]( auto damage, auto execution_time, auto net_chi, auto net_energy,
                                     bool capped = false ) {
      Action new_action;
      auto eps = ( capped ? net_energy
                          : net_energy + ( p()->resource_regen_per_second( RESOURCE_ENERGY ) * execution_time ) ) /
                 execution_time;

      new_action.idps = damage / execution_time;
      new_action.rdps = new_action.idps + 0.5 * mastery_bonus * ( net_chi / execution_time ) +
                        0.02 * mastery_bonus * ( 1 + tp_over_sck ) * eps;

      return new_action;
    };

    // TP->SCK
    Action TP_SCK = DefineAction( total_damage_amplifier * mastery_bonus * ( 1 + tp_over_sck ), 2.0, 1,
                                  -1 * p()->spec.tiger_palm->powerN( power_e::POWER_ENERGY ).cost() );

    // SCK->SCK (capped)
    Action rSCK_cap = DefineAction( total_damage_amplifier, 1.5 / haste_bonus + player_delay, -1, 0, true );

    // SCK->SCK (uncapped)
    Action rSCK_unc = DefineAction( total_damage_amplifier, 1.5 / haste_bonus + player_delay, -1, 0 );

    if ( rSCK_unc.rdps > TP_SCK.rdps )
    {
      auto regen      = p()->resource_regen_per_second( RESOURCE_ENERGY ) * 2;
      auto N_oc_expr  = ( 1 - 2 * regen ) / ( 1.5 + haste_bonus * player_delay ) / ( regen / haste_bonus );
      auto w_oc_expr  = 1 / ( 1 + N_oc_expr );
      auto rdps_nocap = w_oc_expr * TP_SCK.rdps + ( 1 - w_oc_expr ) * rSCK_unc.rdps;

      // Purple
      if ( rSCK_cap.rdps > rdps_nocap )
        return bonedust_brew_zone_results_e::CAP;

      // Red
      return bonedust_brew_zone_results_e::NO_CAP;
    }
    else
    {
      // Blue
      if ( rSCK_unc.idps < TP_SCK.idps )
        return bonedust_brew_zone_results_e::TP_FILL1;

      // Green
      return bonedust_brew_zone_results_e::TP_FILL2;
    }
  }

  void trigger_shuffle( double time_extension )
  {
    if ( p()->specialization() == MONK_BREWMASTER && p()->talent.brewmaster.shuffle->ok() )
    {
      timespan_t base_time = timespan_t::from_seconds( time_extension );

      if ( p()->talent.brewmaster.quick_sip->ok() )
      {
          p()->shuffle_count_secs += time_extension;

          double quick_sip_seconds = p()->talent.brewmaster.quick_sip->effectN( 2 ).base_value(); // Saved as 3
     
          if ( p()->shuffle_count_secs >= quick_sip_seconds )
          {
              // Reduce stagger damage
              auto amount_cleared =
                  p()->active_actions.stagger_self_damage->clear_partial_damage_pct( p()->talent.brewmaster.quick_sip->effectN(1).percent() );
              p()->sample_datas.quick_sip_cleared->add( amount_cleared );
              p()->proc.quick_sip->occur();

              p()->shuffle_count_secs -= quick_sip_seconds;
          }
      }

      if ( p()->buff.shuffle->up() )
      {
        timespan_t max_time     = p()->buff.shuffle->buff_duration();
        timespan_t old_duration = p()->buff.shuffle->remains();
        timespan_t new_length   = std::min( max_time, base_time + old_duration );
        p()->buff.shuffle->refresh( 1, buff_t::DEFAULT_VALUE(), new_length );
      }
      else
      {
        p()->buff.shuffle->trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, base_time );
      }

      if ( p()->shared.walk_with_the_ox && p()->shared.walk_with_the_ox->ok() && p()->cooldown.invoke_niuzao->down() )
        p()->cooldown.invoke_niuzao->adjust( p()->shared.walk_with_the_ox->effectN( 2 ).time_value(), true );
      
    }
  }

  bool has_covenant()
  {
    if ( p()->covenant.kyrian->ok() || p()->covenant.necrolord->ok() || p()->covenant.night_fae->ok() ||
         p()->covenant.venthyr->ok() )
    {
      return true;
    }
    return false;
  }

  double cost() const override
  {
    double c = ab::cost();

    if ( c == 0 )
      return c;

    c *= 1.0 + cost_reduction();
    if ( c < 0 )
      c = 0;

    return c;
  }

  virtual double cost_reduction() const
  {
    double c = 0.0;

    if ( p()->specialization() == MONK_MISTWEAVER )
    {
      if ( p()->buff.mana_tea->check() && ab::data().affected_by( p()->talent.mistweaver.mana_tea->effectN( 1 ) ) )
        c += p()->buff.mana_tea->check_value();  // saved as -50%
    }

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.serenity->check() && ab::data().affected_by( p()->talent.windwalker.serenity->effectN( 1 ) ) )
        c += p()->talent.windwalker.serenity->effectN( 1 ).percent();  // Saved as -100
      else if ( p()->buff.bok_proc->check() && ab::data().affected_by( p()->passives.bok_proc->effectN( 1 ) ) )
        c += p()->passives.bok_proc->effectN( 1 ).percent();  // Saved as -100
    }

    return c;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm = ab::recharge_multiplier( cd );

    return rm;
  }

  void consume_resource() override
  {
    ab::consume_resource();

    if ( !ab::execute_state )  // Fixes rare crashes at combat_end.
      return;

    if (current_resource() == RESOURCE_CHI)
    {
        if ( ab::cost() > 0 )
        {
            if ( p()->talent.windwalker.spiritual_focus->ok() )
            {
              if ( p()->talent.windwalker.storm_earth_and_fire->ok() )
              {
                p()->spiritual_focus_count += ab::cost();

                if ( p()->spiritual_focus_count >= p()->talent.windwalker.spiritual_focus->effectN( 1 ).base_value() )
                {
                  p()->cooldown.storm_earth_and_fire->adjust(
                      -1 * p()->talent.windwalker.spiritual_focus->effectN( 2 ).time_value() );
                  p()->spiritual_focus_count -= p()->talent.windwalker.spiritual_focus->effectN( 1 ).base_value();
                }
              }
              // Serenity brings all spells down to 0 chi spent so don't reduce while Serenity is up
              if ( p()->talent.windwalker.serenity->ok() && !p()->buff.serenity->up() )
              {
                p()->spiritual_focus_count += ab::cost();

                if ( p()->spiritual_focus_count >= p()->talent.windwalker.spiritual_focus->effectN( 1 ).base_value() )
                {
                  p()->cooldown.serenity->adjust( -1 *
                                                  p()->talent.windwalker.spiritual_focus->effectN( 3 ).time_value() );
                  p()->spiritual_focus_count -= p()->talent.windwalker.spiritual_focus->effectN( 1 ).base_value();
                }
              }
            }

            if ( p()->talent.windwalker.drinking_horn_cover->ok() && p()->cooldown.drinking_horn_cover->up() )
            {
                if ( p()->buff.storm_earth_and_fire->up() )
                {
                    auto time_extend = p()->talent.windwalker.drinking_horn_cover->effectN( 1 ).base_value() / 10;
                    time_extend *= ab::cost();

                    p()->buff.storm_earth_and_fire->extend_duration( p(), timespan_t::from_seconds( time_extend ) );

                    p()->find_pet( "earth_spirit" )->adjust_duration( timespan_t::from_seconds( time_extend ) );
                    p()->find_pet( "fire_spirit" )->adjust_duration( timespan_t::from_seconds( time_extend ) );

                    p()->cooldown.drinking_horn_cover->start(
                        p()->talent.windwalker.drinking_horn_cover->internal_cooldown() );
                }
                if ( p()->buff.serenity->up() )
                {
                  p()->buff.serenity->extend_duration(
                      p(), timespan_t::from_seconds(
                               p()->talent.windwalker.drinking_horn_cover->effectN( 2 ).base_value() / 10 ) );

                    p()->cooldown.drinking_horn_cover->start(
                      p()->talent.windwalker.drinking_horn_cover->internal_cooldown() );
                }
            }
        }

      if ( p()->shared.last_emperors_capacitor && p()->shared.last_emperors_capacitor->ok() )
        p()->buff.the_emperors_capacitor->trigger();

      // Chi Savings on Dodge & Parry & Miss
      if ( ab::last_resource_cost > 0 )
      {
        double chi_restored = ab::last_resource_cost;
        if ( !ab::aoe && ab::result_is_miss( ab::execute_state->result ) )
          p()->resource_gain( RESOURCE_CHI, chi_restored, p()->gain.chi_refund );
      }
    }

    // Energy refund, estimated at 80%
    if ( current_resource() == RESOURCE_ENERGY && ab::last_resource_cost > 0 && !ab::hit_any_target )
    {
      double energy_restored = ab::last_resource_cost * 0.8;

      p()->resource_gain( RESOURCE_ENERGY, energy_restored, p()->gain.energy_refund );
    }
  }

  void execute() override
  {
    
    if ( p()->specialization() == MONK_MISTWEAVER )
    {
      if ( trigger_chiji && p()->buff.invoke_chiji->up() )
        p()->buff.invoke_chiji_evm->trigger();
    }
    else if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( may_combo_strike )
        combo_strikes_trigger();

      if ( auto* td = this->get_td( p()->target ) )
      {
        if ( td->debuff.keefers_skyreach->check() && ab::data().affected_by( p()->passives.keefers_skyreach_debuff->effectN( 1 ) ) )
          keefers_skyreach_proc->occur();
      }
    }

    ab::execute();

    trigger_storm_earth_and_fire( this );

    if ( p()->buff.faeline_stomp )
    {
      if ( p()->buff.faeline_stomp->up() && trigger_faeline_stomp &&
        p()->rng().roll( p()->user_options.faeline_stomp_uptime ) )
      {
        double reset_value = p()->buff.faeline_stomp->value();

        if ( p()->shared.faeline_harmony && p()->shared.faeline_harmony->ok() )
          reset_value *= 1 + p()->shared.faeline_harmony->effectN( 2 ).percent();

        if ( p()->rng().roll( reset_value ) )
        {
          p()->cooldown.faeline_stomp->reset( true, 1 );
          p()->buff.faeline_stomp_reset->trigger();
        }
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    if ( s->action->school == SCHOOL_PHYSICAL )
      trigger_mystic_touch( s );

    // Don't want to cause the buff to be cast and then used up immediately.
    if ( current_resource() == RESOURCE_CHI )
    {
      // Dance of Chi-Ji talent triggers from spending chi
      if ( p()->talent.windwalker.dance_of_chiji->ok() )
        p()->buff.dance_of_chiji->trigger();

    }

    ab::impact( s );

    if ( p()->talent.general.resonant_fists->ok() && trigger_resonant_fists ) 
    {
      if ( p()->cooldown.resonant_fists->up() && p()->rng().roll( p()->talent.general.resonant_fists.spell()->proc_chance() ) )
      {
          p()->active_actions.resonant_fists->set_target( s->target );
          p()->active_actions.resonant_fists->execute();
          p()->proc.resonant_fists->occur();
          p()->cooldown.resonant_fists->start( p()->talent.general.resonant_fists.spell()->internal_cooldown() );
      }
    }

    if ( p()->shared.bountiful_brew && p()->shared.bountiful_brew->ok() && trigger_bountiful_brew && p()->cooldown.bountiful_brew->up() &&
         p()->rppm.bountiful_brew->trigger() )
    {
      p()->cooldown.bountiful_brew->start( p()->shared.bountiful_brew->internal_cooldown() );

      p()->active_actions.bountiful_brew->execute();
      p()->proc.bountiful_brew_proc->occur();
    }

    if ( p()->legendary.sinister_teachings->ok() )
    {
      if ( trigger_sinister_teaching_cdr && s->result_total >= 0 && s->result == RESULT_CRIT &&
           p()->buff.fallen_order->up() && p()->cooldown.sinister_teachings->up() )
      {
        if ( p()->specialization() == MONK_MISTWEAVER )
          p()->cooldown.fallen_order->adjust( -1 * p()->legendary.sinister_teachings->effectN( 4 ).time_value() );
        else
          p()->cooldown.fallen_order->adjust( -1 * p()->legendary.sinister_teachings->effectN( 3 ).time_value() );

        p()->cooldown.sinister_teachings->start( p()->legendary.sinister_teachings->internal_cooldown() );
        p()->proc.sinister_teaching_reduction->occur();
      }
    }

    trigger_exploding_keg_proc( s );

    p()->trigger_empowered_tiger_lightning( s, true, true );

    if ( p()->bugs && get_td( s->target )->debuff.bonedust_brew->up() )
      p()->bonedust_brew_assessor( s );
  }

  void tick(dot_t* dot) override
  {
    ab::tick(dot);

    if ( !p()->bugs && !ab::result_is_miss( dot->state->result ) && get_td( dot->state->target )->debuff.bonedust_brew->up() )
        p()->bonedust_brew_assessor(dot->state);
  }

  void last_tick( dot_t* dot ) override
  {
    ab::last_tick( dot );
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = ab::composite_persistent_multiplier( action_state );

    if ( p()->talent.general.ferocity_of_xuen->ok() )
        pm *= 1 + p()->talent.general.ferocity_of_xuen->effectN( 1 ).percent();

    return pm;
  }

  void trigger_storm_earth_and_fire( const action_t* a )
  {
    p()->trigger_storm_earth_and_fire( a, sef_ability );
  }


  void trigger_mystic_touch( action_state_t* s )
  {
    if ( ab::sim->overrides.mystic_touch )
    {
      return;
    }

    if ( ab::result_is_miss( s->result ) )
    {
      return;
    }

    if ( s->result_amount == 0.0 )
    {
      return;
    }

    if ( s->target->debuffs.mystic_touch && p()->spec.mystic_touch->ok() )
    {
      s->target->debuffs.mystic_touch->trigger();
    }
  }

  void trigger_exploding_keg_proc( action_state_t* s )
  {
    if ( p()->specialization() != MONK_BREWMASTER )
      return;

    // Blacklist spells
    if ( s->action->id == 388867 || // Exploding Keg Proc
         s->action->id == 124255 || // Stagger
         s->action->id == 123725 || // Breath of Fire dot
         s->action->id == 196608 || // Eye of the Tiger
         s->action->id == 196733 || // Special Delivery
         s->action->id == 325217 || // Bonedust Brew
         s->action->id == 338141 || // Charred Passion
         s->action->id == 387621 || // Dragonfire Brew
         s->action->id == 393786 // Chi Surge
       ) 
      return;

    if ( p()->buff.exploding_keg->up() && s->action->harmful )
    {
      p()->active_actions.exploding_keg->target = s->target;
      p()->active_actions.exploding_keg->execute();
    }
  }
};

struct monk_spell_t : public monk_action_t<spell_t>
{
  monk_spell_t( util::string_view n, monk_t* player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s )
  {
    ap_type = attack_power_type::WEAPON_MAINHAND;
  }

  bool ready() override
  {
    // Spell data nil or not_found
    if ( data().id() == 0 )
      return false;

    return monk_action_t::ready();
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = base_t::composite_target_crit_chance( target );

    if ( auto* td = this->find_td( target ) )
    {
      if ( td->debuff.keefers_skyreach->check() &&
           base_t::data().affected_by( p()->passives.keefers_skyreach_debuff->effectN( 1 ) ) )
      {
        c += td->debuff.keefers_skyreach->check_value();
      }
    }

    return c;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = base_t::composite_persistent_multiplier( action_state );

    // Windwalker Mastery: Combo Strikes
    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( ww_mastery && p()->buff.combo_strikes->check() )
        pm *= 1 + p()->cache.mastery_value();
    }

    // Brewmaster Tier Set
    if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->buff.brewmasters_rhythm->check() && base_t::data().affected_by( p()->buff.brewmasters_rhythm->data().effectN( 1 ) ) )
        pm *= 1 + p()->buff.brewmasters_rhythm->check_stack_value();
    }

    return pm;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      // Storm, Earth, and Fire
      if ( p()->buff.storm_earth_and_fire->check() && p()->affected_by_sef( base_t::data() ) )
          am *= 1 + p()->talent.windwalker.storm_earth_and_fire->effectN( 1 ).percent();

      // Serenity
      if ( p()->buff.serenity->check() )
      {
        if ( base_t::data().affected_by( p()->talent.windwalker.serenity->effectN( 2 ) ) )
        {
          double serenity_multiplier = p()->talent.windwalker.serenity->effectN( 2 ).percent();

          if ( p()->conduit.coordinated_offensive->ok() )
            serenity_multiplier += p()->conduit.coordinated_offensive.percent();

          am *= 1 + serenity_multiplier;
        }
      }
    }

    return am;
  }
};

struct monk_heal_t : public monk_action_t<heal_t>
{
  monk_heal_t( util::string_view n, monk_t& p, const spell_data_t* s = spell_data_t::nil() ) : base_t( n, &p, s )
  {
    harmful = false;
    ap_type = attack_power_type::WEAPON_MAINHAND;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = base_t::composite_target_multiplier( target );

    return m;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = base_t::composite_target_crit_chance( target );

    if ( auto* td = this->find_td( target ) )
    {
      if ( td->debuff.keefers_skyreach->check() &&
           base_t::data().affected_by( p()->passives.keefers_skyreach_debuff->effectN( 1 ) ) )
      {
        c += td->debuff.keefers_skyreach->check_value();
      }
    }

    return c;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( p()->talent.general.grace_of_the_crane->ok() )
      am *= 1 + p()->talent.general.grace_of_the_crane->effectN( 1 ).percent();

    player_t* t = ( execute_state ) ? execute_state->target : target;

    switch ( p()->specialization() )
    {
      case MONK_MISTWEAVER:      

        if ( auto td = this->get_td( t ) )  // Use get_td since we can have a ticking dot without target-data
        {
          if ( td->dots.enveloping_mist->is_ticking() )
          {
            if ( p()->talent.mistweaver.mist_wrap->ok() )
              am *=
              1.0 + p()->spec.enveloping_mist->effectN( 2 ).percent() + p()->talent.mistweaver.mist_wrap->effectN( 2 ).percent();
            else
              am *= 1.0 + p()->spec.enveloping_mist->effectN( 2 ).percent();
          }
        }

        if ( p()->buff.life_cocoon->check() )
          am *= 1.0 + p()->spec.life_cocoon->effectN( 2 ).percent();

        if ( p()->buff.fae_exposure->check() )
          am *= 1.0 + p()->passives.fae_exposure_heal->effectN( 1 ).percent();

        break;

      case MONK_WINDWALKER:

        // Storm, Earth, and Fire
        if ( p()->buff.storm_earth_and_fire->check() && p()->affected_by_sef( base_t::data() ) )
          am *= 1 + p()->talent.windwalker.storm_earth_and_fire->effectN( 1 ).percent();

        // Serenity
        if ( p()->buff.serenity->check() )
        {
          if ( base_t::data().affected_by( p()->talent.windwalker.serenity->effectN( 2 ) ) )
          {
            double serenity_multiplier = p()->talent.windwalker.serenity->effectN( 2 ).percent();

            if ( p()->conduit.coordinated_offensive->ok() )
              serenity_multiplier += p()->conduit.coordinated_offensive.percent();

            am *= 1 + serenity_multiplier;
          }
        }

        break;

      case MONK_BREWMASTER:

        break;

      default:
        assert( 0 );
        break;
    }

    return am;
  }
};

struct monk_absorb_t : public monk_action_t<absorb_t>
{
  monk_absorb_t( util::string_view n, monk_t& player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, &player, s )
  {
  }
};

struct monk_snapshot_stats_t : public snapshot_stats_t
{
  monk_snapshot_stats_t( monk_t* player, util::string_view options ) : snapshot_stats_t( player, options )
  {
  }

  void execute() override
  {
    snapshot_stats_t::execute();

    auto* monk = debug_cast<monk_t*>( player );

    monk->sample_datas.buffed_stagger_base             = monk->stagger_base_value();
    monk->sample_datas.buffed_stagger_pct_player_level = monk->stagger_pct( player->level() );
    monk->sample_datas.buffed_stagger_pct_target_level = monk->stagger_pct( target->level() );
  }
};

namespace pet_summon
{
struct summon_pet_t : public monk_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  pet_t* pet;

public:
  summon_pet_t( util::string_view n, util::string_view pname, monk_t* p, const spell_data_t* sd = spell_data_t::nil() )
    : monk_spell_t( n, p, sd ), summoning_duration( timespan_t::zero() ), pet_name( pname ), pet( nullptr )
  {
    harmful = false;
  }

  void init_finished() override
  {
    pet = player->find_pet( pet_name );
    if ( !pet )
    {
      background = true;
    }

    monk_spell_t::init_finished();
  }

  void execute() override
  {
    pet->summon( summoning_duration );

    monk_spell_t::execute();
  }

  bool ready() override
  {
    if ( !pet )
    {
      return false;
    }
    return monk_spell_t::ready();
  }
};

// ==========================================================================
// Storm, Earth, and Fire
// ==========================================================================

struct storm_earth_and_fire_t : public monk_spell_t
{
  storm_earth_and_fire_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "storm_earth_and_fire", p, p->talent.windwalker.storm_earth_and_fire )
  {
    parse_options( options_str );

    // Forcing the minimum GCD to 750 milliseconds
    min_gcd   = timespan_t::from_millis( 750 );
    gcd_type  = gcd_haste_type::ATTACK_HASTE;

    cast_during_sck = true;
    callbacks = harmful = may_miss = may_crit = may_dodge = may_parry = may_block = false;


    cooldown->charges += (int)p->spec.storm_earth_and_fire_2->effectN( 1 ).base_value();
  }

  void update_ready( timespan_t cd_duration = timespan_t::min() ) override
  {
    // While pets are up, don't trigger cooldown since the sticky targeting does not consume charges
    if ( p()->buff.storm_earth_and_fire->check() )
    {
      cd_duration = timespan_t::zero();
    }

    monk_spell_t::update_ready( cd_duration );
  }

  bool target_ready( player_t* candidate_target ) override
  {
    // Don't let user needlessly trigger SEF sticky targeting mode, if the user would just be
    // triggering it on the same sticky target
    if ( p()->buff.storm_earth_and_fire->check() )
    {
      auto fixate_target = p()->storm_earth_and_fire_fixate_target( sef_pet_e::SEF_EARTH );
      if ( fixate_target && candidate_target == fixate_target )
      {
        return false;
      }
    }

    return monk_spell_t::target_ready( candidate_target );
  }

  bool ready() override
  {
    if ( p()->talent.windwalker.serenity->ok() )
      return false;

    return monk_spell_t::ready();
  }

  // Monk used SEF while pets are up to sticky target them into an enemy
  void sticky_targeting()
  {
    p()->storm_earth_and_fire_fixate( target );
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( !p()->buff.storm_earth_and_fire->check() )
    {
      p()->summon_storm_earth_and_fire( data().duration() );
    }
    else
    {
      sticky_targeting();
    }
  }
};

struct storm_earth_and_fire_fixate_t : public monk_spell_t
{
  storm_earth_and_fire_fixate_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "storm_earth_and_fire_fixate", p, p->find_spell( (int)p->talent.windwalker.storm_earth_and_fire->effectN( 5 ).base_value() ) )
  {
    parse_options( options_str );

    cast_during_sck    = true;
    callbacks          = false;
    trigger_gcd        = timespan_t::zero();
    cooldown->duration = timespan_t::zero();
  }

  bool target_ready( player_t* target ) override
  {
    if ( !p()->storm_earth_and_fire_fixate_ready( target ) )
    {
      return false;
    }

    return monk_spell_t::target_ready( target );
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->storm_earth_and_fire_fixate( target );
  }
};

}  // namespace pet_summon

namespace attacks
{
struct monk_melee_attack_t : public monk_action_t<melee_attack_t>
{
  weapon_t* mh;
  weapon_t* oh;

  monk_melee_attack_t( util::string_view n, monk_t* player, const spell_data_t* s = spell_data_t::nil() )
    : base_t( n, player, s ), mh( nullptr ), oh( nullptr )
  {
    special    = true;
    may_glance = false;
  }

  bool ready() override
  {
    // Spell data nil or not_found
    if ( data().id() == 0 )
      return false;
      
    // These abilities are able to be used during Spinning Crane Kick
    if ( cast_during_sck )
      usable_while_casting = p()->channeling && ( p()->channeling->id == 101546 /* Windwalker + Mistweaver */ || p()->channeling->id == 322729 /* Brewmaster */ );
      
    return monk_action_t::ready();
  }

  void init_finished() override
  {
    if ( affected_by.serenity )
    {
      std::vector<cooldown_t*> cooldowns = p()->serenity_cooldowns;
      if ( std::find( cooldowns.begin(), cooldowns.end(), cooldown ) == cooldowns.end() )
        p()->serenity_cooldowns.push_back( cooldown );
    }

    base_t::init_finished();
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double c = base_t::composite_target_crit_chance( target );

    if ( auto* td = this->find_td( target ) )
    {
      if ( td->debuff.keefers_skyreach->check() &&
           base_t::data().affected_by( p()->passives.keefers_skyreach_debuff->effectN( 1 ) ) )
      {
        c += td->debuff.keefers_skyreach->check_value();
      }
    }

    return c;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = base_t::composite_persistent_multiplier( action_state );

    return pm;
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( ww_mastery && p()->buff.combo_strikes->check() )
        am *= 1 + p()->cache.mastery_value();

      // Storm, Earth, and Fire
      if ( p()->buff.storm_earth_and_fire->check() && p()->affected_by_sef( base_t::data() ) )
        am *= 1 + p()->talent.windwalker.storm_earth_and_fire->effectN( 1 ).percent();

      // Serenity
      if ( p()->buff.serenity->check() )
      {
        if ( base_t::data().affected_by( p()->talent.windwalker.serenity->effectN( 2 ) ) )
        {
          double serenity_multiplier = p()->talent.windwalker.serenity->effectN( 2 ).percent();

          if ( p()->conduit.coordinated_offensive->ok() )
            serenity_multiplier += p()->conduit.coordinated_offensive.percent();

          am *= 1 + serenity_multiplier;
        }
      }
    }

    // Increases just physical damage
    if ( p()->specialization() == MONK_MISTWEAVER )
    {
      if ( p()->buff.touch_of_death_mw->check() )
        am *= 1 + p()->buff.touch_of_death_mw->check_value();
    }

    return am;
  }

  // Physical tick_action abilities need amount_type() override, so the
  // tick_action are properly physically mitigated.
  result_amount_type amount_type( const action_state_t* state, bool periodic ) const override
  {
    if ( tick_action && tick_action->school == SCHOOL_PHYSICAL )
    {
      return result_amount_type::DMG_DIRECT;
    }
    else
    {
      return base_t::amount_type( state, periodic );
    }
  }

  void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( !sim->overrides.mystic_touch && s->action->result_is_hit( s->result ) && p()->passives.mystic_touch->ok() &&
         s->result_amount > 0.0 )
    {
      s->target->debuffs.mystic_touch->trigger();
    }
  }

  void apply_dual_wield_two_handed_scaling()
  {
    ap_type = attack_power_type::WEAPON_BOTH;

    if ( player->main_hand_weapon.group() == WEAPON_2H )
    {
      ap_type = attack_power_type::WEAPON_MAINHAND;
      // 0.98 multiplier found only in tooltip
      base_multiplier *= 0.98;
    }
  }
};

// ==========================================================================
// Close to Heart Aura Toggle
// ==========================================================================

struct close_to_heart_aura_t : public monk_spell_t
{
  close_to_heart_aura_t( monk_t* player ) : monk_spell_t( "close_to_heart_aura_toggle", player )
  {
    harmful     = false;
    background  = true;
    trigger_gcd = timespan_t::zero();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    tl.clear();

    for ( auto t : sim->player_non_sleeping_list )
    {
      tl.push_back( t );
    }

    return tl.size();
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    size_t i = tl.size();
    while ( i > 0 )
    {
      i--;
      player_t* target_to_buff = tl[ i ];

      if ( p()->get_player_distance( *target_to_buff ) > 10.0 )
        tl.erase( tl.begin() + i );
    }

    return tl;
  }
};

// ==========================================================================
// Generous Pour Aura Toggle
// ==========================================================================

struct generous_pour_aura_t : public monk_spell_t
{
  generous_pour_aura_t( monk_t* player ) : monk_spell_t( "generous_pour_aura_toggle", player )
  {
    harmful     = false;
    background  = true;
    trigger_gcd = timespan_t::zero();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    tl.clear();

    for ( auto t : sim->player_non_sleeping_list )
    {
      tl.push_back( t );
    }

    return tl.size();
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    size_t i = tl.size();
    while ( i > 0 )
    {
      i--;
      player_t* target_to_buff = tl[ i ];

      if ( p()->get_player_distance( *target_to_buff ) > 10.0 )
        tl.erase( tl.begin() + i );
    }

    return tl;
  }
};

// ==========================================================================
// Windwalking Aura Toggle
// ==========================================================================

struct windwalking_aura_t : public monk_spell_t
{
  windwalking_aura_t( monk_t* player ) : monk_spell_t( "windwalking_aura_toggle", player )
  {
    harmful     = false;
    background  = true;
    trigger_gcd = timespan_t::zero();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    tl.clear();

    for ( auto t : sim->player_non_sleeping_list )
    {
      tl.push_back( t );
    }

    return tl.size();
  }

  std::vector<player_t*>& check_distance_targeting( std::vector<player_t*>& tl ) const override
  {
    size_t i = tl.size();
    while ( i > 0 )
    {
      i--;
      player_t* target_to_buff = tl[ i ];

      if ( p()->get_player_distance( *target_to_buff ) > 10.0 )
        tl.erase( tl.begin() + i );
    }

    return tl;
  }
};

// ==========================================================================
// Tiger Palm
// ==========================================================================

// Eye of the Tiger ========================================================
struct eye_of_the_tiger_heal_tick_t : public monk_heal_t
{
  eye_of_the_tiger_heal_tick_t( monk_t& p, util::string_view name )
    : monk_heal_t( name, p, p.talent.general.eye_of_the_tiger->effectN( 1 ).trigger() )
  {
    trigger_bountiful_brew        = true;
    trigger_sinister_teaching_cdr = false;
    background                    = true;
    hasted_ticks                  = false;
    may_crit = tick_may_crit = true;
    target                   = player;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.storm_earth_and_fire->check() )
      {
        // Hard code Patch 9.0.5
        // Eye of the Tiger's heal is now increased by 35% when Storm, Earth, and Fire is out
        am *= ( 1 + p()->talent.windwalker.storm_earth_and_fire->effectN( 1 ).percent() ) * 3;  // Results in 135%
      }
    }

    return am;
  }
};

struct eye_of_the_tiger_dmg_tick_t : public monk_spell_t
{
  eye_of_the_tiger_dmg_tick_t( monk_t* player, util::string_view name )
    : monk_spell_t( name, player, player->talent.general.eye_of_the_tiger->effectN( 1 ).trigger() )
  {
    trigger_bountiful_brew        = true;
    trigger_sinister_teaching_cdr = false;
    background                    = true;
    hasted_ticks                  = false;
    may_crit = tick_may_crit = true;
    aoe                      = 1;
    attack_power_mod.direct  = 0;
    attack_power_mod.tick    = data().effectN( 2 ).ap_coeff();
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    if ( p()->specialization() == MONK_WINDWALKER )
    {

      if ( p()->buff.storm_earth_and_fire->check() )
      {
        // Hard code Patch 9.0.5
        // Eye of the Tiger's damage is now increased by 35% when Storm, Earth, and Fire is out
        am *= ( 1 + p()->talent.windwalker.storm_earth_and_fire->effectN( 1 ).percent() ) * 3;  // Results in 135%
      }
    }

    return am;
  }
};

// Tiger Palm base ability ===================================================
struct tiger_palm_t : public monk_melee_attack_t
{
  heal_t* eye_of_the_tiger_heal;
  spell_t* eye_of_the_tiger_damage;
  bool face_palm;
  bool power_strikes;

  tiger_palm_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "tiger_palm", p, p->spec.tiger_palm ),
      eye_of_the_tiger_heal( new eye_of_the_tiger_heal_tick_t( *p, "eye_of_the_tiger_heal" ) ),
      eye_of_the_tiger_damage( new eye_of_the_tiger_dmg_tick_t( p, "eye_of_the_tiger_damage" ) ),
      face_palm( false ),
      power_strikes( false )
  {
    parse_options( options_str );

    ww_mastery                  = true;
    may_combo_strike            = true;
    trigger_chiji               = true;
    trigger_faeline_stomp       = true;
    trigger_bountiful_brew      = true;
    sef_ability                 = sef_ability_e::SEF_TIGER_PALM;
    cast_during_sck             = true;
    
    add_child( eye_of_the_tiger_damage );
    add_child( eye_of_the_tiger_heal );

    if ( p->specialization() == MONK_WINDWALKER )
      energize_amount = p->spec.windwalker_monk->effectN( 4 ).base_value();
    else
      energize_type = action_energize::NONE;

    spell_power_mod.direct = 0.0;

    if ( p->shared.skyreach && p->shared.skyreach->ok() )
      range += p->shared.skyreach->effectN( 1 ).base_value();
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->buff.blackout_combo->check() )
        am *= 1 + p()->buff.blackout_combo->data().effectN( 1 ).percent();

      if ( face_palm )
        am *= 1 + p()->shared.face_palm->effectN( 2 ).percent();

      if ( p()->buff.counterstrike->check() )
        am *= 1 + p()->buff.counterstrike->data().effectN( 1 ).percent();
    }
    else if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( power_strikes )
        am *= 1 + p()->talent.windwalker.power_strikes->effectN( 2 ).percent();

      if ( p()->talent.windwalker.touch_of_the_tiger->ok() )
        am *= 1 + p()->talent.windwalker.touch_of_the_tiger->effectN( 1 ).percent();
    }

    return am;
  }

  void execute() override
  {
    //============
    // Pre-Execute
    //============

    if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->shared.face_palm && p()->shared.face_palm->ok() && rng().roll( p()->shared.face_palm->effectN( 1 ).percent() ) )
      {
        face_palm = true;
        p()->proc.face_palm->occur();
      }
    }
    else if ( p()->specialization() == MONK_WINDWALKER )
    {
        if (p()->buff.power_strikes->up())
          power_strikes = true;
    }

    //------------

    monk_melee_attack_t::execute();
 
    if ( result_is_miss( execute_state->result ) )
      return;

    if ( power_strikes )
    {
      auto chi_gain = p()->passives.power_strikes_chi->effectN( 1 ).base_value();
      p()->resource_gain( RESOURCE_CHI, chi_gain, p()->gain.power_strikes );
      p()->buff.power_strikes->expire();
    }

    //-----------

    //============
    // Post-hit
    //============

    if ( p()->talent.general.eye_of_the_tiger->ok() )
    {
      // Need to remove any Eye of the Tiger on targets that are not the current target
      // Only the damage dot needs to be removed. The healing buff gets pandemic refreshed
      for ( auto non_sleeping_target : p()->sim->target_non_sleeping_list )
      {
        if ( target == non_sleeping_target )
          continue;

        get_td( non_sleeping_target )->dots.eye_of_the_tiger_damage->cancel();
      }

      eye_of_the_tiger_heal->execute();
      eye_of_the_tiger_damage->execute();
    }

    switch ( p()->specialization() )
    {
      case MONK_MISTWEAVER:
      {
        if ( p()->talent.mistweaver.teachings_of_the_monastery->ok() )
          p()->buff.teachings_of_the_monastery->trigger();
        break;
      }
      case MONK_WINDWALKER:
      {

        // Combo Breaker calculation
        if ( p()->spec.combo_breaker->ok() && p()->buff.bok_proc->trigger() )
        {
          if ( p()->buff.storm_earth_and_fire->up() )
          {
            p()->trigger_storm_earth_and_fire_bok_proc( sef_pet_e::SEF_FIRE );
            p()->trigger_storm_earth_and_fire_bok_proc( sef_pet_e::SEF_EARTH );
          }
        }
        if ( p()->talent.windwalker.teachings_of_the_monastery->ok() )
          p()->buff.teachings_of_the_monastery->trigger();
        break;
      }
      case MONK_BREWMASTER:
      {
        // Reduces the remaining cooldown on your Brews by 1 sec
        brew_cooldown_reduction( p()->spec.tiger_palm->effectN( 3 ).base_value() );

        if ( p()->buff.blackout_combo->up() )
        {
          p()->proc.blackout_combo_tiger_palm->occur();
          p()->buff.blackout_combo->expire();
        }

        if ( p()->buff.counterstrike->up() )
        {
          p()->proc.counterstrike_tp->occur();
          p()->buff.counterstrike->expire();
        }

      if ( face_palm )
          brew_cooldown_reduction( p()->shared.face_palm->effectN( 3 ).base_value() );
        break;
      }
      default:
        break;
    }

    face_palm      = false;
    power_strikes  = false;
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    // Apply Mark of the Crane
    if ( p()->specialization() == MONK_WINDWALKER && result_is_hit( s->result ) &&
         p()->talent.windwalker.mark_of_the_crane->ok() )
      p()->trigger_mark_of_the_crane( s );

    if ( p()->specialization() == MONK_BREWMASTER )
    {
      // Bonedust Brew
      if ( get_td( s->target )->debuff.bonedust_brew->up() )
      {
        if ( p()->shared.bonedust_brew && p()->shared.bonedust_brew->ok() )
          brew_cooldown_reduction( p()->shared.bonedust_brew->effectN( 3 ).base_value() );
      }

      if ( p()->sets->has_set_bonus( MONK_BREWMASTER, T29, B2 ) && p()->cooldown.brewmasters_rhythm->up() ) {
        p()->buff.brewmasters_rhythm->trigger();
        // ICD on the set bonus is set to 0.1 seconds but in-game testing shows to be a 1 second ICD
        p()->cooldown.brewmasters_rhythm->start( timespan_t::from_seconds( 1 ) );
      }

    }

    p()->trigger_keefers_skyreach( s );
  }
};

// ==========================================================================
// Rising Sun Kick
// ==========================================================================

// Glory of the Dawn =================================================
struct glory_of_the_dawn_t : public monk_melee_attack_t
{
  glory_of_the_dawn_t( monk_t* p, const std::string& name )
    : monk_melee_attack_t( name, p, p->passives.glory_of_the_dawn_damage )
  {
    background                = true;
    ww_mastery                = true;
    //trigger_faeline_stomp   = TODO;
    //trigger_bountiful_brew  = TODO;

    apply_dual_wield_two_handed_scaling();
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    // 2019-02-14
    // In 8.1.5, the Glory of the Dawn artifact trait will now deal 35% increased damage while Storm, Earth, and Fire is
    // active. https://www.wowhead.com/bluetracker?topic=95585&region=us
    // https://us.forums.blizzard.com/en/wow/t/ww-sef-bugs-and-more/95585/10
    // The 35% cannot be located in any effect (whether it's Windwalker aura, SEF's spell, or in either of GotD's
    // spells) Using SEF' damage reduction times 3 for future proofing (1 + -55%) = 45%; 45% * 3 = 135%
    if ( p()->buff.storm_earth_and_fire->check() )
      am *= ( 1 + p()->talent.windwalker.storm_earth_and_fire->effectN( 1 ).percent() ) * 3;

    return am;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    p()->resource_gain( RESOURCE_CHI,
      p()->passives.glory_of_the_dawn_damage->effectN( 3 ).base_value() );// , p()->gain.glory_of_the_dawn );
  }
};

// Rising Sun Kick Damage Trigger ===========================================

struct rising_sun_kick_dmg_t : public monk_melee_attack_t
{
  rising_sun_kick_dmg_t( monk_t* p, util::string_view name )
    : monk_melee_attack_t( name, p, p->talent.general.rising_sun_kick->effectN( 1 ).trigger() )
  {
    ww_mastery             = true;
    trigger_faeline_stomp  = true;
    trigger_bountiful_brew = true;

    background = dual = true;
    may_crit          = true;
    trigger_chiji     = true;

    if ( p->specialization() == MONK_WINDWALKER || p->specialization() == MONK_BREWMASTER )
      apply_dual_wield_two_handed_scaling();
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->spec.rising_sun_kick_2->ok() )
      am *= 1 + p()->spec.rising_sun_kick_2->effectN( 1 ).percent();

    if ( p()->talent.general.fast_feet->ok() )
      am *= 1 + p()->talent.general.fast_feet->effectN( 1 ).percent();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->talent.windwalker.rising_star->ok() )
        am *= 1 + p()->talent.windwalker.rising_star->effectN( 1 ).percent();
    }

    if ( p()->buff.kicks_of_flowing_momentum->check() )
      am *= 1 + p()->buff.kicks_of_flowing_momentum->check_value();

    return am;
  }

  double composite_crit_chance() const override
  {
    double c = monk_melee_attack_t::composite_crit_chance();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.pressure_point->check() )
        c += p()->buff.pressure_point->check_value();
    }

    return c;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->specialization() == MONK_MISTWEAVER )
    {
      if ( p()->buff.thunder_focus_tea->up() )
      {
        p()->cooldown.rising_sun_kick->adjust(
          p()->talent.mistweaver.thunder_focus_tea->effectN( 1 ).time_value(), true );

        p()->buff.thunder_focus_tea->decrement();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    if ( p()->specialization() == MONK_MISTWEAVER )
    {
      if ( p()->buff.teachings_of_the_monastery->up() )
      {
        p()->buff.teachings_of_the_monastery->expire();

        // Spirit of the Crane does not have a buff associated with it. Since
        // this is tied with Teachings of the Monastery, tacking
        // this onto the removal of that buff.
        if ( p()->talent.mistweaver.spirit_of_the_crane->ok() )
          p()->resource_gain( RESOURCE_MANA,
            ( p()->resources.max[RESOURCE_MANA] * p()->passives.spirit_of_the_crane->effectN( 1 ).percent() ),
            p()->gain.spirit_of_the_crane );
      }
    }
    else if ( p()->specialization() == MONK_WINDWALKER )
    {

      if ( p()->talent.windwalker.transfer_the_power->ok() )
        p()->buff.transfer_the_power->trigger( 2 ); // Not documented anywhere but applying 2 stacks in game

      if ( p()->shared.xuens_battlegear && p()->shared.xuens_battlegear->ok() && ( s->result == RESULT_CRIT ) )
      {
        p()->cooldown.fists_of_fury->adjust( -1 * p()->shared.xuens_battlegear->effectN( 2 ).time_value(), true );
        p()->proc.xuens_battlegear_reduction->occur();
      }

      // Apply Mark of the Crane
      if ( p()->talent.windwalker.mark_of_the_crane->ok() )
        p()->trigger_mark_of_the_crane( s );

      if (p()->buff.kicks_of_flowing_momentum->up()) {
        p()->buff.kicks_of_flowing_momentum->decrement();

        if ( p()->sets->has_set_bonus(MONK_WINDWALKER, T29, B4) ) {
          p()->buff.fists_of_flowing_momentum->trigger();
        }
      }
    }
  }
};

struct rising_sun_kick_t : public monk_melee_attack_t
{
  rising_sun_kick_dmg_t* trigger_attack;
  glory_of_the_dawn_t* gotd;

  rising_sun_kick_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "rising_sun_kick", p, p->talent.general.rising_sun_kick )
  {
    parse_options( options_str );

    may_combo_strike            = true;
    may_proc_bron               = true;
    trigger_faeline_stomp       = true;
    trigger_bountiful_brew      = true;
    sef_ability                 = sef_ability_e::SEF_RISING_SUN_KICK;
    affected_by.serenity        = true;
    ap_type                     = attack_power_type::NONE;
    cast_during_sck             = true;

    attack_power_mod.direct = 0;

    trigger_attack        = new rising_sun_kick_dmg_t( p, "rising_sun_kick_dmg" );
    trigger_attack->stats = stats;

    if ( p->talent.windwalker.glory_of_the_dawn->ok() )
    {
      gotd = new glory_of_the_dawn_t( p, "glory_of_the_dawn" );

      add_child( gotd );
    }
  }

  double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.weapons_of_order_ww->check() )
        c += p()->buff.weapons_of_order_ww->check_value();  // saved as -1
    }

    if ( c < 0 )
      c = 0;

    return c;
  }

  void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      // Register how much chi is saved without actually refunding the chi
      if ( p()->buff.serenity->up() )
      {
        if ( p()->buff.weapons_of_order_ww->up() )
          p()->gain.serenity->add( RESOURCE_CHI, base_costs[RESOURCE_CHI] + p()->buff.weapons_of_order_ww->value() );
        else
          p()->gain.serenity->add( RESOURCE_CHI, base_costs[RESOURCE_CHI] );
      }
    }
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    trigger_attack->set_target( target );
    trigger_attack->execute();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->talent.windwalker.glory_of_the_dawn->ok() &&
        rng().roll( p()->talent.windwalker.glory_of_the_dawn->effectN( 3 ).percent() ) )
      {
        // TODO check if the damage values are being applied correctly
        gotd->target = p()->target;
        gotd->execute();
      }

      if ( p()->talent.windwalker.whirling_dragon_punch->ok() && p()->cooldown.fists_of_fury->down() )
      {
        if ( this->cooldown_duration() <= p()->cooldown.fists_of_fury->remains() )
          p()->buff.whirling_dragon_punch->set_duration( this->cooldown_duration() );
        else
          p()->buff.whirling_dragon_punch->set_duration( p()->cooldown.fists_of_fury->remains() );

        p()->buff.whirling_dragon_punch->trigger();
      }

      if ( p()->talent.windwalker.transfer_the_power->ok() )
        p()->buff.transfer_the_power->trigger();

      if ( p()->buff.weapons_of_order->up() )
        p()->buff.weapons_of_order_ww->trigger();
    }
  }
};

// ==========================================================================
// Blackout Kick
// ==========================================================================

// Blackout Kick Proc from Teachings of the Monastery =======================
struct blackout_kick_totm_proc_t : public monk_melee_attack_t
{
  blackout_kick_totm_proc_t( monk_t* p )
    : monk_melee_attack_t( "blackout_kick_totm_proc", p, p->passives.totm_bok_proc )
  {
    sef_ability         = sef_ability_e::SEF_BLACKOUT_KICK_TOTM;
    ww_mastery          = false;
    cooldown->duration  = timespan_t::zero();
    background = dual   = true;
    trigger_chiji       = true;
    trigger_gcd         = timespan_t::zero();
  }

  void init_finished() override
  {
    monk_melee_attack_t::init_finished();
    action_t* bok = player->find_action( "blackout_kick" );
    if ( bok )
    {
      base_multiplier        = bok->base_multiplier;
      spell_power_mod.direct = bok->spell_power_mod.direct;

      bok->add_child( this );
    }
  }

  // Force 100 milliseconds for the animation, but not delay the overall GCD
  timespan_t execute_time() const override
  {
    return timespan_t::from_millis( 100 );
  }

  double cost() const override
  {
    return 0;
  }

  double composite_crit_chance() const override
  {
    double c = monk_melee_attack_t::composite_crit_chance();

    if ( p()->specialization() == MONK_WINDWALKER && p()->talent.windwalker.hardened_soles->ok() )
      c += p()->talent.windwalker.hardened_soles->effectN( 1 ).percent();

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = monk_melee_attack_t::composite_crit_damage_bonus_multiplier();

    if ( p()->specialization() == MONK_WINDWALKER && p()->talent.windwalker.hardened_soles->ok() )
      m *= 1 + p()->talent.windwalker.hardened_soles->effectN( 2 ).percent();

    return m;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state->result ) )
      return;

    if ( p()->conduit.tumbling_technique->ok() &&
      rng().roll( p()->conduit.tumbling_technique->effectN( 1 ).percent() ) )
    {
      if ( p()->talent.general.chi_torpedo->ok() )
      {
        p()->cooldown.chi_torpedo->reset( true, 1 );
        p()->proc.tumbling_technique_chi_torpedo->occur();
      }
      else
      {
        p()->cooldown.roll->reset( true, 1 );
        p()->proc.tumbling_technique_roll->occur();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    switch ( p()->specialization() )
    {
      case MONK_MISTWEAVER:

        if ( p()->talent.mistweaver.spirit_of_the_crane->ok() )
          p()->resource_gain(
            RESOURCE_MANA,
            ( p()->resources.max[RESOURCE_MANA] * p()->passives.spirit_of_the_crane->effectN( 1 ).percent() ),
            p()->gain.spirit_of_the_crane );
        break;
  
      case MONK_BREWMASTER:

        if ( p()->talent.brewmaster.staggering_strikes->ok() )
        {
          auto ap = s->composite_attack_power();
          auto amount_cleared = p()->partial_clear_stagger_amount( ap * p()->talent.brewmaster.staggering_strikes->effectN( 2 ).percent() );
          p()->sample_datas.staggering_strikes_cleared->add( amount_cleared );
        }
        break;

      case MONK_WINDWALKER:

        // Transfer the power triggers from ToTM hits but only on the primary target
        if ( s->target == target )
        {
          if ( p()->talent.windwalker.transfer_the_power->ok() )
            p()->buff.transfer_the_power->trigger();

          if ( p()->talent.windwalker.mark_of_the_crane->ok() )
            p()->trigger_mark_of_the_crane( s );
        }

        break;

      default:
        assert( 0 );
        break;
    }
  }
};

// Charred Passions ============================================================
struct charred_passions_bok_t : public monk_spell_t
{
  charred_passions_bok_t( monk_t* p ) : monk_spell_t( "charred_passions_bok", p, p->passives.charred_passions_dmg )
  {
    background = dual = true;
    proc              = true;
    may_crit          = false;
  }
};

// Blackout Kick Baseline ability =======================================
struct blackout_kick_t : public monk_melee_attack_t
{
  blackout_kick_totm_proc_t* bok_totm_proc;
  charred_passions_bok_t* charred_passions;

  blackout_kick_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t(
          "blackout_kick", p,
          ( p->specialization() == MONK_BREWMASTER ? p->spec.blackout_kick_brm : p->spec.blackout_kick ) )
  {
    ww_mastery = true;

    parse_options( options_str );
    sef_ability                 = sef_ability_e::SEF_BLACKOUT_KICK;
    may_combo_strike            = true;
    trigger_chiji               = true;
    trigger_faeline_stomp       = true;
    trigger_bountiful_brew      = true;
    cast_during_sck             = true;
    
    switch ( p->specialization() )
    {
      case MONK_BREWMASTER: {
        if ( p->talent.brewmaster.shadowboxing_treads->ok() )
          aoe = 1 + (int)p->talent.brewmaster.shadowboxing_treads->effectN( 1 ).base_value();
        if ( p->talent.brewmaster.fluidity_of_motion->ok() )
          cooldown->duration += p->talent.brewmaster.fluidity_of_motion->effectN( 1 ).time_value();

        if ( p->talent.brewmaster.charred_passions->ok() )
        {
          charred_passions = new charred_passions_bok_t( p );

          add_child( charred_passions );
        }

        apply_dual_wield_two_handed_scaling();
        break;
      }
      case MONK_MISTWEAVER: {
        if ( p->talent.mistweaver.teachings_of_the_monastery->ok() )
        {
          bok_totm_proc = new blackout_kick_totm_proc_t( p );

          add_child( bok_totm_proc );
        }
        break;
      }
      case MONK_WINDWALKER: {
        if ( p->spec.blackout_kick_2 )
          // Saved as -2
          base_costs[ RESOURCE_CHI ] +=
              p->spec.blackout_kick_2->effectN( 1 ).base_value();  // Reduce base from 3 chi to 1

        if ( p->talent.windwalker.shadowboxing_treads->ok() )
          aoe = 1 + (int)p->talent.windwalker.shadowboxing_treads->effectN( 1 ).base_value();

        if ( p->talent.windwalker.teachings_of_the_monastery->ok() )
        {
          bok_totm_proc = new blackout_kick_totm_proc_t( p );

          add_child( bok_totm_proc );
        }

        apply_dual_wield_two_handed_scaling();
        break;
      }
      default:
        break;
    }
  }

  double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.weapons_of_order_ww->check() )
        c += p()->buff.weapons_of_order_ww->check_value();
    }

    if ( c < 0 )
      return 0;

    return c;
  }

  void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      // Register how much chi is saved without actually refunding the chi
      if ( p()->buff.serenity->up() )
      {
        if ( p()->buff.weapons_of_order_ww->up() )
          p()->gain.serenity->add( RESOURCE_CHI, base_costs[RESOURCE_CHI] + p()->buff.weapons_of_order_ww->value() );
        else
          p()->gain.serenity->add( RESOURCE_CHI, base_costs[RESOURCE_CHI] );
      }

      // Register how much chi is saved without actually refunding the chi
      if ( p()->buff.bok_proc->up() )
      {
        p()->buff.bok_proc->expire();
        if ( !p()->buff.serenity->up() )
          p()->gain.bok_proc->add( RESOURCE_CHI, base_costs[RESOURCE_CHI] );
      }
    }
  }

  double composite_crit_chance() const override
  {
    double c = monk_melee_attack_t::composite_crit_chance();

    if ( p()->specialization() == MONK_WINDWALKER && p()->talent.windwalker.hardened_soles->ok() )
      c += p()->talent.windwalker.hardened_soles->effectN( 1 ).percent();

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = monk_melee_attack_t::composite_crit_damage_bonus_multiplier();

    if ( p()->specialization() == MONK_WINDWALKER && p()->talent.windwalker.hardened_soles->ok() )
      m *= 1 + p()->talent.windwalker.hardened_soles->effectN( 2 ).percent();

    return m;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    switch ( p()->specialization() )
    {
      case MONK_BREWMASTER:
        if ( p()->talent.brewmaster.shadowboxing_treads->ok() )
          am *= 1 + p()->talent.brewmaster.shadowboxing_treads->effectN( 2 ).percent();

        if ( p()->talent.brewmaster.fluidity_of_motion->ok() )
          am *= 1 + p()->talent.brewmaster.fluidity_of_motion->effectN( 2 ).percent();

        if ( p()->talent.brewmaster.elusive_footwork->ok() )
          am *= 1 + p()->talent.brewmaster.elusive_footwork->effectN( 3 ).percent();
        break;

      case MONK_WINDWALKER:
        if ( p()->talent.windwalker.shadowboxing_treads->ok() )
          am *= 1 + p()->talent.windwalker.shadowboxing_treads->effectN( 2 ).percent();
        break;

      case MONK_MISTWEAVER:

        break;

      default:
        assert( 0 );
        break;
    }
    return am;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( result_is_miss( execute_state->result ) )
      return;

    // Teachings of the Monastery
    // Used by both Windwalker and Mistweaver
    if ( p()->buff.teachings_of_the_monastery && p()->buff.teachings_of_the_monastery->up() )
      p()->buff.teachings_of_the_monastery->expire();

    switch ( p()->specialization() )
    {
      case MONK_BREWMASTER:
      {
        if ( p()->mastery.elusive_brawler->ok() )
          p()->buff.elusive_brawler->trigger();

        if ( p()->talent.brewmaster.blackout_combo->ok() )
          p()->buff.blackout_combo->trigger();

        if ( p()->talent.brewmaster.shuffle->ok() )
          trigger_shuffle( p()->talent.brewmaster.shuffle->effectN( 1 ).base_value() );

        if ( p()->talent.brewmaster.hit_scheme->ok() )
          p()->buff.hit_scheme->trigger();
        break;
      }

      case MONK_MISTWEAVER:
      {
        break;
      }

      case MONK_WINDWALKER:
      {
        if ( p()->spec.blackout_kick_3->ok() )
        {
          // Reduce the cooldown of Rising Sun Kick and Fists of Fury
          timespan_t cd_reduction = -1 * p()->spec.blackout_kick->effectN( 3 ).time_value();
          if ( p()->buff.weapons_of_order->up() )
          {
            cd_reduction += ( -1 * p()->shared.weapons_of_order->effectN( 8 ).time_value() );
            if ( p()->buff.serenity->up() )
              p()->proc.blackout_kick_cdr_serenity_with_woo->occur();
            else
              p()->proc.blackout_kick_cdr_with_woo->occur();
          }
          else
          {
            if ( p()->buff.serenity->up() )
              p()->proc.blackout_kick_cdr_serenity->occur();
            else
              p()->proc.blackout_kick_cdr->occur();
          }

          if ( p()->talent.windwalker.transfer_the_power->ok() )
            p()->buff.transfer_the_power->trigger();

          p()->cooldown.rising_sun_kick->adjust( cd_reduction, true );
          p()->cooldown.fists_of_fury->adjust( cd_reduction, true );
        }

        break;
      }

      default:
        assert( 0 );
        break;
    }

    if ( p()->conduit.tumbling_technique->ok() && rng().roll( p()->conduit.tumbling_technique.percent() ) )
    {
      if ( p()->talent.general.chi_torpedo->ok() )
      {
        p()->cooldown.chi_torpedo->reset( true, 1 );
        p()->proc.tumbling_technique_chi_torpedo->occur();
      }
      else
      {
        p()->cooldown.roll->reset( true, 1 );
        p()->proc.tumbling_technique_roll->occur();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( !result_is_hit( s->result ) )
      return;

    // Teachings of the Monastery
    // Used by both Windwalker and Mistweaver
    if ( p()->specialization() == MONK_WINDWALKER || p()->specialization() == MONK_MISTWEAVER )
    {
      player_talent_t totm = p()->specialization() == MONK_WINDWALKER ? p()->talent.windwalker.teachings_of_the_monastery
                                                                      : p()->talent.mistweaver.teachings_of_the_monastery;
      if ( totm->ok()  )
      {
        if ( p()->buff.teachings_of_the_monastery->up() )
        {
          int stacks = p()->buff.teachings_of_the_monastery->current_stack;

          bok_totm_proc->set_target( s->target );

          for ( int i = 0; i < stacks; i++ )
            bok_totm_proc->execute();

          // Each initial hit from blackout kick has an individual chance to reset
          if ( rng().roll( totm->effectN( 1 ).percent() ) )
          {
            p()->cooldown.rising_sun_kick->reset( true );
            p()->proc.rsk_reset_totm->occur();
          }
        }
      }
    }

    switch ( p()->specialization() )
    {
      case MONK_WINDWALKER:

        if ( p()->talent.windwalker.mark_of_the_crane->ok() )
          p()->trigger_mark_of_the_crane( s );
        break;

      case MONK_BREWMASTER:

        if ( p()->buff.charred_passions->up() )
        {

          double dmg_percent = p()->shared.charred_passions->effectN( 1 ).percent();

          charred_passions->base_dd_min = s->result_amount * dmg_percent;
          charred_passions->base_dd_max = s->result_amount * dmg_percent;
          charred_passions->s_data = p()->shared.charred_passions->effectN( 1 ).trigger();
          charred_passions->execute();

          p()->proc.charred_passions_bok->occur();

          if ( get_td( s->target )->dots.breath_of_fire->is_ticking() && p()->cooldown.charred_passions->up() )
          {
            get_td( s->target )->dots.breath_of_fire->refresh_duration();

            p()->cooldown.charred_passions->start( p()->shared.charred_passions->effectN( 1 ).trigger()->internal_cooldown() );
          }
        }

        if ( p()->talent.brewmaster.elusive_footwork->ok() && s->result == RESULT_CRIT )
        {
          p()->buff.elusive_brawler->trigger( (int)p()->talent.brewmaster.elusive_footwork->effectN( 2 ).base_value() );
          p()->proc.elusive_footwork_proc->occur();
        }

        break;

      case MONK_MISTWEAVER:

        break;

      default:
        assert( 0 );
        break;
    }

  }
};

// ==========================================================================
// Rushing Jade Wind
// ==========================================================================

struct rjw_tick_action_t : public monk_melee_attack_t
{
  rjw_tick_action_t( util::string_view name, monk_t* p, const spell_data_t* data )
    : monk_melee_attack_t( name, p, data )
  {
    ww_mastery = true;

    dual = background   = true;
    aoe                 = -1;
    reduced_aoe_targets = data->effectN( 1 ).base_value();
    radius              = data->effectN( 1 ).radius();

    // Reset some variables to ensure proper execution
    dot_duration       = timespan_t::zero();
    cooldown->duration = timespan_t::zero();
  }
};

struct rushing_jade_wind_t : public monk_melee_attack_t
{
  rushing_jade_wind_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "rushing_jade_wind", p, p->shared.rushing_jade_wind )
  {
    parse_options( options_str );
    sef_ability                     = sef_ability_e::SEF_RUSHING_JADE_WIND;
    may_combo_strike                = true;
    may_proc_bron                   = true;
    trigger_faeline_stomp           = true;
    trigger_bountiful_brew          = true;
    gcd_type                        = gcd_haste_type::NONE;

    // Set dot data to 0, since we handle everything through the buff.
    base_tick_time = timespan_t::zero();
    dot_duration   = timespan_t::zero();

    if ( !p->active_actions.rushing_jade_wind )
    {
      p->active_actions.rushing_jade_wind =
            new rjw_tick_action_t( "rushing_jade_wind_tick", p, data().effectN( 1 ).trigger() );
      p->active_actions.rushing_jade_wind->stats = stats;
    }
  }

  void init() override
  {
    monk_melee_attack_t::init();

    update_flags &= ~STATE_HASTE;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    p()->buff.rushing_jade_wind->trigger();

    // Currently this triggers once on execute and not on ticks
    if ( p()->specialization() == MONK_WINDWALKER )
    {
      // Currently this triggers once on execute and not on ticks
      if ( p()->talent.windwalker.transfer_the_power->ok() )
        p()->buff.transfer_the_power->trigger();
    }
  }
};

// ==========================================================================
// Spinning Crane Kick
// ==========================================================================

// Jade Ignition Legendary
struct chi_explosion_t : public monk_spell_t
{
  chi_explosion_t( monk_t* player ) : monk_spell_t( "chi_explosion", player, player->passives.chi_explosion )
  {
    dual = background = true;
    aoe               = -1;
    school            = SCHOOL_NATURE;
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    if ( p()->buff.chi_energy->check() )
      am *= 1 + p()->buff.chi_energy->check_stack_value();

    return am;
  }
};

// Charred Passions ============================================================
struct charred_passions_sck_t : public monk_spell_t
{
  charred_passions_sck_t( monk_t* p ) : monk_spell_t( "charred_passions_sck", p, p->passives.charred_passions_dmg )
  {
    background = dual = true;
    proc              = true;
    may_crit          = false;
  }
};

struct sck_tick_action_t : public monk_melee_attack_t
{
  sck_tick_action_t( util::string_view name, monk_t* p, const spell_data_t* data )
    : monk_melee_attack_t( name, p, data )
  {
    ww_mastery    = true;
    trigger_chiji = true;

    dual = background   = true;
    aoe                 = -1;
    reduced_aoe_targets = p->spec.spinning_crane_kick->effectN( 1 ).base_value();
    radius              = data->effectN( 1 ).radius();

    if ( p->talent.windwalker.widening_whirl.ok() )
        radius *= 1 + p->talent.windwalker.widening_whirl->effectN( 1 ).percent();

    if ( p->specialization() == MONK_WINDWALKER )
      apply_dual_wield_two_handed_scaling();

    // Reset some variables to ensure proper execution
    dot_duration                  = timespan_t::zero();
    school                        = SCHOOL_PHYSICAL;
    cooldown->duration            = timespan_t::zero();
    base_costs[ RESOURCE_ENERGY ] = 0;
  }
  
  int motc_counter() const
  {    
    if ( p()->specialization() != MONK_WINDWALKER )
      return 0;

    if ( !p()->talent.windwalker.mark_of_the_crane->ok() )
      return 0;

    if ( p()->user_options.motc_override > 0 )
      return p()->user_options.motc_override;

    int count = 0;

    for ( player_t* target : target_list() )
    {
      if ( this->find_td( target ) && this->find_td( target )->debuff.mark_of_the_crane->check() )
        count++;

      if ( count == (int)p()->passives.cyclone_strikes->max_stacks() )
        break;
    }

    return count;
  }

  double cost() const override
  {
    return 0;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      int motc_stacks = motc_counter();

      if ( motc_stacks > 0 && p()->spec.spinning_crane_kick_2_ww->ok() )
      {
        double motc_multiplier = p()->passives.cyclone_strikes->effectN( 1 ).percent();

        if ( p()->conduit.calculated_strikes->ok() )
          motc_multiplier += p()->conduit.calculated_strikes.percent();
        /*else if ( p()->talent.windwalker.calculated_strikes->ok() )
          motc_multiplier += p()->talent.windwalker.calculated_strikes->effectN( 1 ).percent();
        */

        am *= 1 + ( motc_counter() * motc_multiplier );
      }

      if ( p()->buff.dance_of_chiji_hidden->check() )
        am *= 1 + p()->talent.windwalker.dance_of_chiji->effectN( 1 ).percent();

      if ( p()->talent.windwalker.crane_vortex->ok() )
        am *= 1 + p()->talent.windwalker.crane_vortex->effectN( 1 ).percent();

      if ( p()->buff.kicks_of_flowing_momentum->check() )
        am *= 1 + p()->buff.kicks_of_flowing_momentum->check_value();
    }
    else if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->buff.counterstrike->check() )
        am *= 1 + p()->buff.counterstrike->data().effectN( 2 ).percent();
    }

    if ( p()->talent.general.fast_feet->ok() )
      am *= 1 + p()->talent.general.fast_feet->effectN( 2 ).percent();

    return am;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    if ( p()->spec.spinning_crane_kick_2_brm->ok() )
      trigger_shuffle( p()->spec.spinning_crane_kick_2_brm->effectN( 1 ).base_value() );
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->buff.charred_passions->up() )
      {
        double dmg_percent = p()->shared.charred_passions->effectN( 1 ).percent();

        p()->active_actions.charred_passions->base_dd_min = s->result_amount * dmg_percent;
        p()->active_actions.charred_passions->base_dd_max = s->result_amount * dmg_percent;
        p()->active_actions.charred_passions->s_data = p()->shared.charred_passions->effectN( 1 ).trigger();
        p()->active_actions.charred_passions->execute();

        p()->proc.charred_passions_sck->occur();

        if ( get_td( s->target )->dots.breath_of_fire->is_ticking() && p()->cooldown.charred_passions->up() )
        {
          get_td( s->target )->dots.breath_of_fire->refresh_duration();

          p()->cooldown.charred_passions->start( p()->shared.charred_passions->effectN( 1 ).trigger()->internal_cooldown() );
        }
      }

      if ( p()->sets->has_set_bonus( MONK_BREWMASTER, T29, B2 ) && p()->cooldown.brewmasters_rhythm->up() ) {
        p()->buff.brewmasters_rhythm->trigger();
        // ICD on the set bonus is set to 0.1 seconds but in-game testing shows to be a 1 second ICD
        p()->cooldown.brewmasters_rhythm->start( timespan_t::from_seconds( 1 ) );
      }
    }
  }
};

struct spinning_crane_kick_t : public monk_melee_attack_t
{
  struct spinning_crane_kick_state_t : public action_state_t
  {
    spinning_crane_kick_state_t( action_t* a, player_t* target ) : action_state_t( a, target )
    {
    }

    proc_types2 cast_proc_type2() const override
    {
      // Spinning Crane Kick seems to trigger Bron's Call to Action (and possibly other
      // effects that care about casts).
      return PROC2_CAST;
    }
  };

  chi_explosion_t* chi_x;

  spinning_crane_kick_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t(
          "spinning_crane_kick", p,
          ( p->specialization() == MONK_BREWMASTER ? p->spec.spinning_crane_kick_brm : p->spec.spinning_crane_kick ) ),
    chi_x( nullptr )
  {
    parse_options( options_str );

    sef_ability                     = sef_ability_e::SEF_SPINNING_CRANE_KICK;
    may_combo_strike                = true;
    may_proc_bron                   = true;
    trigger_faeline_stomp           = true;
    trigger_bountiful_brew          = true;

    may_crit = may_miss = may_block = may_dodge = may_parry = false;
    tick_zero = hasted_ticks = channeled = interrupt_auto_attack = true;

    // Does not incur channel lag when interrupted by a cast-thru ability
    ability_lag                     = p->world_lag;
    ability_lag_stddev              = p->world_lag_stddev;

    spell_power_mod.direct = 0.0;

    tick_action =
        new sck_tick_action_t( "spinning_crane_kick_tick", p, p->spec.spinning_crane_kick->effectN( 1 ).trigger() );

    // Brewmaster can use SCK again after the GCD
    if ( p->specialization() == MONK_BREWMASTER )
    {
      dot_behavior    = DOT_EXTEND;
      cast_during_sck = true;

      if ( p->shared.charred_passions && p->shared.charred_passions->ok() )
      {
        add_child( p->active_actions.charred_passions );
      }
    }
    else if ( p->specialization() == MONK_WINDWALKER )
    {
      if ( p->shared.jade_ignition && p->shared.jade_ignition->ok() )
      {
        chi_x = new chi_explosion_t( p );

        add_child( chi_x );
      }
    }
  }

  action_state_t* new_state() override
  {
    return new spinning_crane_kick_state_t( this, p()->target );
  }

  // N full ticks, but never additional ones.
  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return dot_duration * ( tick_time( s ) / base_tick_time );
  }

  double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.weapons_of_order_ww->check() )
        c += p()->buff.weapons_of_order_ww->check_value();

      if ( p()->buff.dance_of_chiji_hidden->check() )
        c += p()->passives.dance_of_chiji->effectN( 1 ).base_value();  // saved as -2
    }

    if ( c < 0 )
      c = 0;

    return c;
  }

  void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    // Register how much chi is saved without actually refunding the chi
    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.serenity->up() )
      {
        double cost = base_costs[RESOURCE_CHI];

        if ( p()->buff.weapons_of_order_ww->up() )
          cost += p()->buff.weapons_of_order_ww->value();

        if ( p()->buff.dance_of_chiji_hidden->up() )
          cost += p()->buff.dance_of_chiji->value();

        if ( cost < 0 )
          cost = 0;

        p()->gain.serenity->add( RESOURCE_CHI, cost );
      }
    }
  }

  void execute() override
  {
    //===========
    // Pre-Execute
    //===========

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.dance_of_chiji->up() )
      {
        p()->buff.dance_of_chiji->expire();
        p()->buff.dance_of_chiji_hidden->trigger();
      }
    }

    monk_melee_attack_t::execute();

    //===========
    // Post-Execute
    //===========

    timespan_t buff_duration = composite_dot_duration( execute_state );

    p()->buff.spinning_crane_kick->trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, buff_duration );

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( chi_x && p()->buff.chi_energy->up() )
        chi_x->execute();

      // Bonedust Brew
      // Chi refund is triggering once on the trigger spell and not from tick spells.
      if ( get_td( execute_state->target )->debuff.bonedust_brew->up() )
          p()->resource_gain( RESOURCE_CHI, p()->passives.bonedust_brew_chi->effectN( 1 ).base_value(), p()->gain.bonedust_brew );
    }
    else if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->buff.celestial_flames->up() )
      {
        p()->active_actions.breath_of_fire->target = execute_state->target;
        p()->active_actions.breath_of_fire->execute();
      }
    }

  }

  void last_tick( dot_t* dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.dance_of_chiji_hidden->up() )
        p()->buff.dance_of_chiji_hidden->expire();

      if ( p()->buff.chi_energy->up() )
        p()->buff.chi_energy->expire();

      if ( p()->buff.kicks_of_flowing_momentum->up() )
      {
        p()->buff.kicks_of_flowing_momentum->decrement();

        if ( p()->sets->has_set_bonus( MONK_WINDWALKER, T29, B4 ) )
          p()->buff.fists_of_flowing_momentum->trigger();
      }
    }
    else if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->buff.counterstrike->up() )
      {
        p()->proc.counterstrike_sck->occur();
        p()->buff.counterstrike->expire();
      }
    }
  }
};

// ==========================================================================
// Fists of Fury
// ==========================================================================

struct fists_of_fury_tick_t : public monk_melee_attack_t
{
  fists_of_fury_tick_t( monk_t* p, util::string_view name )
    : monk_melee_attack_t( name, p, p->passives.fists_of_fury_tick )
  {
    background          = true;
    aoe                 = -1;
    reduced_aoe_targets = p->talent.windwalker.fists_of_fury->effectN( 1 ).base_value();
    full_amount_targets = 1;
    ww_mastery          = true;

    attack_power_mod.direct    = p->talent.windwalker.fists_of_fury->effectN( 5 ).ap_coeff();
    base_costs[ RESOURCE_CHI ] = 0;
    dot_duration               = timespan_t::zero();
    trigger_gcd                = timespan_t::zero();
    apply_dual_wield_two_handed_scaling();
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = monk_melee_attack_t::composite_target_multiplier( target );

    // 2022-05-27 Patch 9.2.5 added an -11% effect that is to offset an increased to the single target damage
    // while trying to keep AoE damage the same.
    // ( 70% - 11% ) * 120% = 70.8%
    if ( target != p()->target )
      m *= ( p()->talent.windwalker.fists_of_fury->effectN( 6 ).percent() + p()->spec.windwalker_monk->effectN( 20 ).percent() );

    return m;
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->conduit.inner_fury->ok() )
      am *= 1 + p()->conduit.inner_fury.percent();

    if ( p()->talent.windwalker.flashing_fists.ok() )
      am *= 1 + p()->talent.windwalker.flashing_fists->effectN(1 ).percent();

    if ( p()->buff.transfer_the_power->check() )
      am *= 1 + p()->buff.transfer_the_power->check_stack_value();

    if ( p()->talent.windwalker.open_palm_strikes->ok() )
      am *= 1 + p()->talent.windwalker.open_palm_strikes->effectN( 4 ).percent();

    if ( p()->buff.fists_of_flowing_momentum_fof->check() )
      am *= 1 + p()->buff.fists_of_flowing_momentum_fof->check_value();

    return am;
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->shared.jade_ignition && p()->shared.jade_ignition->ok() )
      p()->buff.chi_energy->trigger();

    if ( p()->talent.windwalker.open_palm_strikes.ok() && rng().roll( p()->talent.windwalker.open_palm_strikes->effectN( 2 ).percent() ) )
        p()->resource_gain( RESOURCE_CHI, p()->talent.windwalker.open_palm_strikes->effectN( 3 ).base_value(), p()->gain.open_palm_strikes );
  }
};

struct fists_of_fury_t : public monk_melee_attack_t
{
  fists_of_fury_t( monk_t* p, util::string_view options_str, bool canceled = false )
    : monk_melee_attack_t( ( canceled ? "fists_of_fury_cancel" : "fists_of_fury" ), p, p->talent.windwalker.fists_of_fury )
  {
    parse_options( options_str );

    cooldown                        = p->cooldown.fists_of_fury;
    sef_ability                     = sef_ability_e::SEF_FISTS_OF_FURY;
    may_combo_strike                = true;
    may_proc_bron                   = true;
    trigger_faeline_stomp           = true;
    trigger_bountiful_brew          = true;
    affected_by.serenity            = true;

    channeled = tick_zero = true;
    interrupt_auto_attack = true;

    attack_power_mod.direct = 0;
    weapon_power_mod        = 0;

    may_crit = may_miss = may_block = may_dodge = may_parry = callbacks = false;

    // Effect 1 shows a period of 166 milliseconds which appears to refer to the visual and not the tick period
    base_tick_time = dot_duration / 4;

    // Using fists_of_fury_cancel in APL will cancel immediately after the GCD regardless of priority
    if ( canceled ) 
      dot_duration = trigger_gcd;

    tick_action = new fists_of_fury_tick_t( p, "fists_of_fury_tick" );

  }

  double cost() const override
  {
    double c = monk_melee_attack_t::cost();

    if ( p()->buff.weapons_of_order_ww->check() )
      c += p()->buff.weapons_of_order_ww->check_value();

    if ( c <= 0 )
      return 0;

    return c;
  }

  void consume_resource() override
  {
    monk_melee_attack_t::consume_resource();

    // Register how much chi is saved without actually refunding the chi
    if ( p()->buff.serenity->up() )
    {
      if ( p()->buff.weapons_of_order_ww->up() )
        p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] + p()->buff.weapons_of_order_ww->value() );
      else
        p()->gain.serenity->add( RESOURCE_CHI, base_costs[ RESOURCE_CHI ] );
    }
  }

  bool usable_moving() const override
  {
    return true;
  }

  void execute() override
  {
    if ( p()->sets->has_set_bonus( MONK_WINDWALKER, T29, B2 ) )
      p()->buff.kicks_of_flowing_momentum->trigger();

    if ( p()->buff.fists_of_flowing_momentum->up() )
    {
      p()->buff.fists_of_flowing_momentum_fof->trigger( 1, p()->buff.fists_of_flowing_momentum->stack_value(), -1,
                                                         data().duration() );
      p()->buff.fists_of_flowing_momentum->expire();
    }

    monk_melee_attack_t::execute();

    if ( p()->buff.fury_of_xuen_stacks->up() && rng().roll( p()->buff.fury_of_xuen_stacks->stack_value() ) )
        p()->buff.fury_of_xuen_stacks->expire();

    if ( p()->talent.windwalker.whirling_dragon_punch->ok() && p()->cooldown.rising_sun_kick->down() )
    {
      if ( this->cooldown_duration() <= p()->cooldown.rising_sun_kick->remains() )
        p()->buff.whirling_dragon_punch->set_duration( this->cooldown_duration() );
      else
        p()->buff.whirling_dragon_punch->set_duration( p()->cooldown.rising_sun_kick->remains() );
      p()->buff.whirling_dragon_punch->trigger();
    }
  }

  void last_tick( dot_t* dot ) override
  {
    monk_melee_attack_t::last_tick( dot );

    if ( p()->buff.fists_of_flowing_momentum_fof->up() )
      p()->buff.fists_of_flowing_momentum_fof->expire();

    if ( p()->shared.xuens_battlegear && p()->shared.xuens_battlegear->ok() )
      p()->buff.pressure_point->trigger();

    if ( p()->buff.transfer_the_power->up() )
      p()->buff.transfer_the_power->expire();

    // If Fists of Fury went the full duration
    if ( dot->current_tick == dot->num_ticks() ) {
    }
  }
};

// ==========================================================================
// Whirling Dragon Punch
// ==========================================================================

struct whirling_dragon_punch_tick_t : public monk_melee_attack_t
{
  timespan_t delay;
  whirling_dragon_punch_tick_t( util::string_view name, monk_t* p, const spell_data_t* s, timespan_t delay )
    : monk_melee_attack_t( name, p, s ), delay( delay )
  {
    ww_mastery            = true;
    trigger_faeline_stomp = true;

    background = true;
    aoe        = -1;
    radius     = s->effectN( 1 ).radius();
    apply_dual_wield_two_handed_scaling();
  }
};

struct whirling_dragon_punch_t : public monk_melee_attack_t
{
  struct whirling_dragon_punch_state_t : public action_state_t
  {
    whirling_dragon_punch_state_t( action_t* a, player_t* target ) : action_state_t( a, target )
    {
    }

    proc_types2 cast_proc_type2() const override
    {
      // Whirling Dragon Punch seems to trigger Bron's Call to Action (and possibly other
      // effects that care about casts).
      return PROC2_CAST;
    }
  };

  std::array<whirling_dragon_punch_tick_t*, 3> ticks;

  struct whirling_dragon_punch_tick_event_t : public event_t
  {
    whirling_dragon_punch_tick_t* tick;

    whirling_dragon_punch_tick_event_t( whirling_dragon_punch_tick_t* tick, timespan_t delay )
      : event_t( *tick->player, delay ), tick( tick )
    {
    }

    void execute() override
    {
      tick->execute();
    }
  };

  whirling_dragon_punch_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "whirling_dragon_punch", p, p->talent.windwalker.whirling_dragon_punch )
  {
    sef_ability = sef_ability_e::SEF_WHIRLING_DRAGON_PUNCH;

    parse_options( options_str );
    interrupt_auto_attack       = false;
    channeled                   = false;
    may_combo_strike            = true;
    may_proc_bron               = true;
    trigger_faeline_stomp       = true;
    trigger_bountiful_brew      = true;
    cast_during_sck             = true;

    spell_power_mod.direct = 0.0;

    // 3 server-side hardcoded ticks
    for ( size_t i = 0; i < ticks.size(); ++i )
    {
      auto delay = base_tick_time * i;
      ticks[ i ] = new whirling_dragon_punch_tick_t( "whirling_dragon_punch_tick", p,
                                                     p->passives.whirling_dragon_punch_tick, delay );
                                                     
      add_child( ticks[ i ] );                                                 
    }
  }

  action_state_t* new_state() override
  {
    return new whirling_dragon_punch_state_t( this, p()->target );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    for ( auto& tick : ticks )
    {
      make_event<whirling_dragon_punch_tick_event_t>( *sim, tick, tick->delay );
    }

  }

  bool ready() override
  {
    // Only usable while Fists of Fury and Rising Sun Kick are on cooldown.
    if ( p()->buff.whirling_dragon_punch->up() )
      return monk_melee_attack_t::ready();

    return false;
  }
};

// ==========================================================================
// Strike of the Windlord
// ==========================================================================
// Off hand hits first followed by main hand
// The ability does NOT require an off-hand weapon to be executed.
// The ability uses the main-hand weapon damage for both attacks

struct strike_of_the_windlord_main_hand_t : public monk_melee_attack_t
{
  strike_of_the_windlord_main_hand_t( monk_t* p, const char* name, const spell_data_t* s )
    : monk_melee_attack_t( name, p, s )
  {
    sef_ability            = sef_ability_e::SEF_STRIKE_OF_THE_WINDLORD;
    ww_mastery             = true;
    may_proc_bron          = false;  // Only the first hit from SotWTL triggers Bron
    trigger_faeline_stomp  = true;
    trigger_bountiful_brew = true;
    ap_type                = attack_power_type::WEAPON_MAINHAND;

    aoe                                          = -1;
    may_dodge = may_parry = may_block = may_miss = true;
    dual = background                            = true;
  }

  // Damage must be divided on non-main target by the number of targets
  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    if ( state->target != target )
    {
      return 1.0 / state->n_targets;
    }

    return 1.0;
  }
};

struct strike_of_the_windlord_off_hand_t : public monk_melee_attack_t
{
  strike_of_the_windlord_off_hand_t( monk_t* p, const char* name, const spell_data_t* s )
    : monk_melee_attack_t( name, p, s )
  {
    sef_ability             = sef_ability_e::SEF_STRIKE_OF_THE_WINDLORD_OH;
    ww_mastery              = true;
    may_proc_bron           = true;
    trigger_faeline_stomp   = true;
    trigger_bountiful_brew  = true;
    ap_type                 = attack_power_type::WEAPON_OFFHAND;

    aoe                                          = -1;
    may_dodge = may_parry = may_block = may_miss  = true;
    dual = background                             = true;
  }

  // Damage must be divided on non-main target by the number of targets
  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    if ( state->target != target )
    {
      return 1.0 / state->n_targets;
    }

    return 1.0;
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->talent.windwalker.thunderfist->ok() )
      p()->buff.thunderfist->trigger();
  }
};

struct strike_of_the_windlord_t : public monk_melee_attack_t
{
  strike_of_the_windlord_main_hand_t* mh_attack;
  strike_of_the_windlord_off_hand_t* oh_attack;

  strike_of_the_windlord_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "strike_of_the_windlord", p, p->talent.windwalker.strike_of_the_windlord ),
      mh_attack( nullptr ),
      oh_attack( nullptr )
  {
    cast_during_sck             = true;
    affected_by.serenity        = false;
    cooldown->hasted            = false;
    trigger_gcd                 = data().gcd();

    parse_options( options_str );

    oh_attack = new strike_of_the_windlord_off_hand_t(
        p, "strike_of_the_windlord_offhand", data().effectN( 4 ).trigger() );
    mh_attack = new strike_of_the_windlord_main_hand_t(
        p, "strike_of_the_windlord_mainhand", data().effectN( 3 ).trigger() );

    add_child( oh_attack );
    add_child( mh_attack );
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    // Off-hand attack hits first
    oh_attack->execute();

    if ( result_is_hit( oh_attack->execute_state->result ) )
      mh_attack->execute();
  }
};
    
// ==========================================================================
// Thunderfist
// ==========================================================================

struct thunderfist_t : public monk_spell_t
{
  thunderfist_t( monk_t* player ) : monk_spell_t( "thunderfist", player, player->passives.thunderfist->effectN( 1 ).trigger() )
  {
    background = true;
    may_crit   = true;
  }

  virtual void execute() override
  {
    monk_spell_t::execute();

    p()->buff.thunderfist->decrement( 1 );
  }
};

// ==========================================================================
// Melee
// ==========================================================================

struct melee_t : public monk_melee_attack_t
{
  int sync_weapons;
  bool first;
  melee_t( util::string_view name, monk_t* player, int sw )
    : monk_melee_attack_t( name, player, spell_data_t::nil() ), sync_weapons( sw ), first( true )
  {
    background = repeating = may_glance = true;
    may_crit                            = true;
    trigger_sinister_teaching_cdr       = false;
    trigger_gcd                         = timespan_t::zero();
    special                             = false;
    school                              = SCHOOL_PHYSICAL;
    weapon_multiplier                   = 1.0;
    allow_class_ability_procs           = true;
    not_a_proc                          = true;

    if ( player->main_hand_weapon.group() == WEAPON_1H )
    {
      if ( player->specialization() != MONK_MISTWEAVER )
        base_hit -= 0.19;
    }
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.storm_earth_and_fire->check() )
        am *= 1.0 + p()->talent.windwalker.storm_earth_and_fire->effectN( 3 ).percent();

      if ( p()->buff.serenity->check() )
        am *= 1 + p()->talent.windwalker.serenity->effectN( 7 ).percent();
    }

    return am;
  }

  void reset() override
  {
    monk_melee_attack_t::reset();
    first = true;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = monk_melee_attack_t::execute_time();

    if ( first )
      return ( weapon->slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t / 2, timespan_t::zero() ) : t / 2 )
                                               : timespan_t::zero();
    else
      return t;
  }

  void execute() override
  {
    if ( first )
      first = false;

    monk_melee_attack_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( p()->buff.thunderfist->up() )
      {
        p()->passive_actions.thunderfist->target = s->target;
        p()->passive_actions.thunderfist->schedule_execute();
      }
    }
  }
};

// ==========================================================================
// Auto Attack
// ==========================================================================

struct auto_attack_t : public monk_melee_attack_t
{
  int sync_weapons;
  auto_attack_t( monk_t* player, util::string_view options_str )
    : monk_melee_attack_t( "auto_attack", player, spell_data_t::nil() ), sync_weapons( 0 )
  {
    add_option( opt_bool( "sync_weapons", sync_weapons ) );
    parse_options( options_str );
    ignore_false_positive         = true;
    trigger_sinister_teaching_cdr = false;

    p()->main_hand_attack                    = new melee_t( "melee_main_hand", player, sync_weapons );
    p()->main_hand_attack->weapon            = &( player->main_hand_weapon );
    p()->main_hand_attack->base_execute_time = player->main_hand_weapon.swing_time;

    if ( player->off_hand_weapon.type != WEAPON_NONE )
    {
      if ( !player->dual_wield() )
        return;

      p()->off_hand_attack                    = new melee_t( "melee_off_hand", player, sync_weapons );
      p()->off_hand_attack->weapon            = &( player->off_hand_weapon );
      p()->off_hand_attack->base_execute_time = player->off_hand_weapon.swing_time;
      p()->off_hand_attack->id                = 1;
    }

    trigger_gcd = timespan_t::zero();
  }

  bool ready() override
  {
    if ( p()->current.distance_to_move > 5 )
      return false;

    return ( p()->main_hand_attack->execute_event == nullptr );  // not swinging
  }

  void execute() override
  {
    if ( player->main_hand_attack )
      p()->main_hand_attack->schedule_execute();

    if ( player->off_hand_attack )
      p()->off_hand_attack->schedule_execute();
  }
};

// ==========================================================================
// Keg Smash
// ==========================================================================
struct keg_smash_t : public monk_melee_attack_t
{
  keg_smash_t( monk_t& p, util::string_view options_str )
    : monk_melee_attack_t( "keg_smash", &p, p.talent.brewmaster.keg_smash )
  {
    parse_options( options_str );

    aoe                    = -1;
    reduced_aoe_targets    = p.talent.brewmaster.keg_smash->effectN( 7 ).base_value();
    full_amount_targets    = 1;
    trigger_faeline_stomp  = true;
    trigger_bountiful_brew = true;
    cast_during_sck        = true;

    attack_power_mod.direct = p.talent.brewmaster.keg_smash->effectN( 2 ).ap_coeff();
    radius                  = p.talent.brewmaster.keg_smash->effectN( 2 ).radius();

    cooldown->duration = p.talent.brewmaster.keg_smash->cooldown();
    cooldown->duration = p.talent.brewmaster.keg_smash->charge_cooldown();

    if ( p.shared.stormstouts_last_keg && p.shared.stormstouts_last_keg->ok() )
      cooldown->charges += (int)p.shared.stormstouts_last_keg->effectN( 2 ).base_value();

    // Keg Smash does not appear to be picking up the baseline Trigger GCD reduction
    // Forcing the trigger GCD to 1 second.
    trigger_gcd = timespan_t::from_seconds( 1 );
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    if ( p()->shared.stormstouts_last_keg && p()->shared.stormstouts_last_keg->ok() )
      am *= 1 + p()->shared.stormstouts_last_keg->effectN( 1 ).percent();

    if ( p()->shared.scalding_brew && p()->shared.scalding_brew->ok() )
    {
      if ( auto* td = this->get_td( p()->target ) )
      {
        if ( td->dots.breath_of_fire->is_ticking() )
          am *= 1 + ( p()->conduit.scalding_brew->ok() ? p()->conduit.scalding_brew.percent() : p()->shared.scalding_brew->effectN( 1 ).percent() );
      }
    }

    if ( p()->buff.hit_scheme->check() )
      am *= 1 + p()->buff.hit_scheme->check_value();

    return am;
  }

  void execute() override
  {
    if ( p()->shared.scalding_brew && p()->shared.scalding_brew->ok() )
    {
      if ( auto* td = this->get_td( p()->target ) )
      {
        if ( td->dots.breath_of_fire->is_ticking() )
          p()->proc.keg_smash_scalding_brew->occur();
      }
    }

    monk_melee_attack_t::execute();

    if ( p()->talent.brewmaster.salsalabims_strength->ok() )
    {
      p()->cooldown.breath_of_fire->reset( true, 1 );
      p()->proc.salsalabim_bof_reset->occur();
    }

    // Reduces the remaining cooldown on your Brews by 4 sec.
    double time_reduction = p()->talent.brewmaster.keg_smash->effectN( 4 ).base_value();

    // Blackout Combo talent reduces Brew's cooldown by 2 sec.
    if ( p()->buff.blackout_combo->up() )
    {
      time_reduction += p()->buff.blackout_combo->data().effectN( 3 ).base_value();
      p()->proc.blackout_combo_keg_smash->occur();
      p()->buff.blackout_combo->expire();
    }

    trigger_shuffle( p()->talent.brewmaster.keg_smash->effectN( 6 ).base_value() );

    brew_cooldown_reduction( time_reduction );
  }

  void impact( action_state_t* s ) override
  {
    monk_melee_attack_t::impact( s );

    get_td( s->target )->debuff.keg_smash->trigger();

    if ( p()->buff.weapons_of_order->up() )
      get_td( s->target )->debuff.weapons_of_order->trigger();

    // Bonedust Brew
    if ( get_td( s->target )->debuff.bonedust_brew->up() )
    {
      if ( p()->shared.bonedust_brew && p()->shared.bonedust_brew->ok() )
        brew_cooldown_reduction( p()->shared.bonedust_brew->effectN( 3 ).base_value() );
    }
  }
};

// ==========================================================================
// Touch of Death
// ==========================================================================
struct touch_of_death_t : public monk_melee_attack_t
{
  touch_of_death_t( monk_t& p, util::string_view options_str )
    : monk_melee_attack_t( "touch_of_death", &p, p.spec.touch_of_death )
  {
    ww_mastery                  = true;
    may_crit = hasted_ticks     = false;
    may_combo_strike            = true;
    may_proc_bron               = true;
    trigger_faeline_stomp       = true;
    trigger_bountiful_brew      = true;
    cast_during_sck             = true;
    parse_options( options_str );

    cooldown->duration = data().cooldown();

    if ( p.shared.fatal_touch && p.shared.fatal_touch->ok() )
      cooldown->duration += p.shared.fatal_touch->effectN( 1 ).time_value(); // -45000, -90000

    if ( p.specialization() == MONK_WINDWALKER && p.talent.windwalker.fatal_flying_guillotine->ok() )
      aoe = 1 + (int)p.talent.windwalker.fatal_flying_guillotine->effectN( 1 ).base_value();

  }

  void init() override
  {
    monk_melee_attack_t::init();

    snapshot_flags = update_flags = 0;
  }

  double composite_target_armor( player_t* ) const override
  {
    return 0;
  }
    
  bool ready() override
  {
    // Deals damage equal to 35% of your maximum health against players and stronger creatures under 15% health
    if ( target->true_level > p()->true_level && p()->talent.general.improved_touch_of_death->ok() &&
         ( target->health_percentage() < p()->talent.general.improved_touch_of_death->effectN( 1 ).base_value() ) )
      return monk_melee_attack_t::ready();

    // You exploit the enemy target's weakest point, instantly killing creatures if they have less health than you
    // Only applicable in health based sims
    if ( target->current_health() > 0 && target->current_health() <= p()->resources.max[ RESOURCE_HEALTH ] )
      return monk_melee_attack_t::ready();

    return false;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    switch ( p()->specialization() )
    {
      case MONK_WINDWALKER:

        if ( p()->talent.windwalker.forbidden_technique->ok() )
        {
          if ( p()->buff.hidden_masters_forbidden_touch->up() )
            p()->buff.hidden_masters_forbidden_touch->expire();
          else
          {
            p()->buff.hidden_masters_forbidden_touch->trigger();
            p()->cooldown.touch_of_death->reset( true );
          }
        }

        if ( p()->spec.touch_of_death_3_ww->ok() )
          p()->buff.touch_of_death_ww->trigger();

        break;

      case MONK_MISTWEAVER:

        if ( p()->spec.touch_of_death_3_mw->ok() )
          p()->buff.touch_of_death_mw->trigger();

        break;

      case MONK_BREWMASTER:

        break;

      default:
        assert( 0 );
        break;
    }
  }

  void impact( action_state_t* s ) override
  {
    // In execute range ToD deals the target health in damage
    double amount = target->current_health();

    // Not in execute range
    // or not a health-based fight style
    if ( target->current_health() == 0 || target->current_health() > p()->resources.max[RESOURCE_HEALTH] )
    {
      // Damage is associated with the players non-buffed max HP
      // Meaning using Fortifying Brew does not affect ToD's damage
      double amount = p()->resources.initial[RESOURCE_HEALTH];

      if ( target->true_level > p()->true_level )
        amount *= p()->talent.general.improved_touch_of_death->effectN( 2 ).percent();  // 35% HP

      if ( p()->specialization() == MONK_WINDWALKER )
      {
        if ( p()->talent.windwalker.meridian_strikes.ok() )
          amount *= 1 + p()->talent.windwalker.meridian_strikes->effectN( 1 ).percent();

        if ( p()->talent.windwalker.forbidden_technique->ok() )
          amount *= 1 + p()->talent.windwalker.forbidden_technique->effectN( 2 ).percent();

        // Damage is only affected by Windwalker's Mastery
        // Versatility does not affect the damage of Touch of Death.
        if ( p()->buff.combo_strikes->up() )
          amount *= 1 + p()->cache.mastery_value();
      }

      s->result_total = s->result_raw = amount;
    }

    monk_melee_attack_t::impact( s );

    if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->spec.touch_of_death_3_brm->ok() )
        p()->partial_clear_stagger_amount( amount * p()->spec.touch_of_death_3_brm->effectN( 1 ).percent() );
    }
  }
};

// ==========================================================================
// Touch of Karma
// ==========================================================================
// When Touch of Karma (ToK) is activated, two spells are placed. A buff on the player (id: 125174), and a
// debuff on the target (id: 122470). Whenever the player takes damage, a dot (id: 124280) is placed on
// the target that increases as the player takes damage. Each time the player takes damage, the dot is refreshed
// and recalculates the dot size based on the current dot size. Just to make it easier to code, I'll wait until
// the Touch of Karma buff expires before placing a dot on the target. Net result should be the same.

// 8.1 Good Karma - If the player still has the ToK buff on them, each time the target hits the player, the amount
// absorbed is immediatly healed by the Good Karma spell (id: 285594)

struct touch_of_karma_dot_t : public residual_action::residual_periodic_action_t<monk_melee_attack_t>
{
  touch_of_karma_dot_t( monk_t* p ) : base_t( "touch_of_karma", p, p->passives.touch_of_karma_tick )
  {
    may_miss = may_crit = false;
    dual                = true;
    ap_type             = attack_power_type::NO_WEAPON;
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    residual_action::residual_periodic_action_t<monk_melee_attack_t>::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags = update_flags = 0;
    snapshot_flags |= STATE_VERSATILITY;
  }
};

struct touch_of_karma_t : public monk_melee_attack_t
{
  double interval;
  double interval_stddev;
  double interval_stddev_opt;
  double pct_health;
  touch_of_karma_dot_t* touch_of_karma_dot;
  touch_of_karma_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "touch_of_karma", p, p->talent.windwalker.touch_of_karma ),
      interval( 100 ),
      interval_stddev( 0.05 ),
      interval_stddev_opt( 0 ),
      pct_health( 0.5 ),
      touch_of_karma_dot( new touch_of_karma_dot_t( p ) )
  {

    add_option( opt_float( "interval", interval ) );
    add_option( opt_float( "interval_stddev", interval_stddev_opt ) );
    add_option( opt_float( "pct_health", pct_health ) );
    parse_options( options_str );

    cooldown->duration        = data().cooldown();
    base_dd_min = base_dd_max = 0;
    ap_type                   = attack_power_type::NO_WEAPON;
    cast_during_sck           = true;

    double max_pct = data().effectN( 3 ).percent();

    if ( pct_health > max_pct )  // Does a maximum of 50% of the monk's HP.
      pct_health = max_pct;

    if ( interval < cooldown->duration.total_seconds() )
    {
      sim->error( "{} minimum interval for Touch of Karma is 90 seconds.", *player );
      interval = cooldown->duration.total_seconds();
    }

    if ( interval_stddev_opt < 1 )
      interval_stddev = interval * interval_stddev_opt;
    // >= 1 seconds is used as a standard deviation normally
    else
      interval_stddev = interval_stddev_opt;

    trigger_gcd = timespan_t::zero();
    may_crit = may_miss = may_dodge = may_parry = false;
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    monk_melee_attack_t::init();
    // disable the snapshot_flags for all multipliers
    snapshot_flags = update_flags = 0;
  }

  void execute() override
  {
    timespan_t new_cd        = timespan_t::from_seconds( rng().gauss( interval, interval_stddev ) );
    timespan_t data_cooldown = data().cooldown();
    if ( new_cd < data_cooldown )
      new_cd = data_cooldown;

    cooldown->duration = new_cd;

    monk_melee_attack_t::execute();

    if ( pct_health > 0 )
    {
      double damage_amount = pct_health * player->resources.max[ RESOURCE_HEALTH ];

      // Redirects 70% of the damage absorbed
      damage_amount *= data().effectN( 4 ).percent();

      residual_action::trigger( touch_of_karma_dot, execute_state->target, damage_amount );
    }
  }
};

// ==========================================================================
// Provoke
// ==========================================================================

struct provoke_t : public monk_melee_attack_t
{
  provoke_t( monk_t* p, util::string_view options_str ) : monk_melee_attack_t( "provoke", p, p->spec.provoke )
  {
    parse_options( options_str );
    use_off_gcd           = true;
    ignore_false_positive = true;
  }

  void impact( action_state_t* s ) override
  {
    if ( s->target->is_enemy() )
      target->taunt( player );

    monk_melee_attack_t::impact( s );
  }
};

// ==========================================================================
// Spear Hand Strike
// ==========================================================================

struct spear_hand_strike_t : public monk_melee_attack_t
{
  spear_hand_strike_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "spear_hand_strike", p, p->talent.general.spear_hand_strike )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    is_interrupt          = true;
    cast_during_sck       = true;
    may_miss = may_block = may_dodge = may_parry = false;
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    //    if ( p()->level() <= 50 )
    //      p()->trigger_sephuzs_secret( execute_state, MECHANIC_INTERRUPT );
  }
};

// ==========================================================================
// Leg Sweep
// ==========================================================================

struct leg_sweep_t : public monk_melee_attack_t
{
  leg_sweep_t( monk_t* p, util::string_view options_str ) : monk_melee_attack_t( "leg_sweep", p, p->spec.leg_sweep )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;
    cast_during_sck = true;

    if (p->talent.general.tiger_tail_sweep->ok() )
    {
      radius += p->talent.general.tiger_tail_sweep->effectN( 1 ).base_value();
      cooldown->duration += p->talent.general.tiger_tail_sweep->effectN( 2 ).time_value(); // Saved as -10000
    }
  }

  void execute() override
  {
    monk_melee_attack_t::execute();

    //    if ( p()->level() <= 50 )
    //      p()->trigger_sephuzs_secret( execute_state, MECHANIC_STUN );
  }
};

// ==========================================================================
// Paralysis
// ==========================================================================

struct paralysis_t : public monk_melee_attack_t
{
  paralysis_t( monk_t* p, util::string_view options_str ) : monk_melee_attack_t( "paralysis", p, p->talent.general.paralysis )
  {
    parse_options( options_str );
    ignore_false_positive = true;
    may_miss = may_block = may_dodge = may_parry = false;

    if ( p->talent.general.improved_paralysis->ok() )
      cooldown->duration += p->talent.general.improved_paralysis->effectN( 1 ).time_value();
  }
};

// ==========================================================================
// Flying Serpent Kick
// ==========================================================================

struct flying_serpent_kick_t : public monk_melee_attack_t
{
  bool first_charge;
  double movement_speed_increase;
  flying_serpent_kick_t( monk_t* p, util::string_view options_str )
    : monk_melee_attack_t( "flying_serpent_kick", p, p->talent.windwalker.flying_serpent_kick ),
      first_charge( true ),
      movement_speed_increase( p->talent.windwalker.flying_serpent_kick->effectN( 1 ).percent() )
  {
    parse_options( options_str );
    may_crit                        = true;
    ww_mastery                      = true;
    may_combo_strike                = true;
    ignore_false_positive           = true;
    movement_directionality         = movement_direction_type::OMNI;
    attack_power_mod.direct         = p->passives.flying_serpent_kick_damage->effectN( 1 ).ap_coeff();
    aoe                             = -1;
    p->cooldown.flying_serpent_kick = cooldown;

    if ( p->spec.flying_serpent_kick_2 )
      p->cooldown.flying_serpent_kick->duration +=
          p->spec.flying_serpent_kick_2->effectN( 1 ).time_value();  // Saved as -5000
  }

  void reset() override
  {
    monk_melee_attack_t::reset();
    first_charge = true;
  }

  bool ready() override
  {
    if ( first_charge )  // Assumes that we fsk into combat, instead of setting initial distance to 20 yards.
      return monk_melee_attack_t::ready();

    return monk_melee_attack_t::ready();
  }

  double action_multiplier() const override
  {
    double am = monk_melee_attack_t::action_multiplier();

    am *= 1 + p()->spec.windwalker_monk->effectN( 1 ).percent();

    return am;
  }

  void execute() override
  {
    if ( p()->current.distance_to_move >= 0 )
    {
      p()->buff.flying_serpent_kick_movement->trigger(
          1, movement_speed_increase, 1,
          timespan_t::from_seconds( std::min(
              1.5, p()->current.distance_to_move / ( p()->base_movement_speed * ( 1 + p()->passive_movement_modifier() +
                                                                                  movement_speed_increase ) ) ) ) );
      p()->current.moving_away = 0;
    }

    monk_melee_attack_t::execute();

    if ( first_charge )
    {
      first_charge = !first_charge;
    }
  }

  void impact( action_state_t* state ) override
  {
    monk_melee_attack_t::impact( state );

    get_td( state->target )->debuff.flying_serpent_kick->trigger();
  }
};
}  // namespace attacks

namespace spells
{

// ==========================================================================
// Resonant Fists
// ==========================================================================

struct resonant_fists_t : public monk_spell_t
{
  resonant_fists_t( monk_t& p )
    : monk_spell_t( "resonant_fists", &p, p.talent.general.resonant_fists->effectN( 2 ).trigger() )
  {
    background = true;
    aoe        = -1;
  }

  void init() override
  {
    monk_spell_t::init();

    trigger_resonant_fists = false;
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    am *= 1 + p()->talent.general.resonant_fists->effectN( 1 ).percent();
    
    return am;
  }
};

// ==========================================================================
// Special Delivery
// ==========================================================================

struct special_delivery_t : public monk_spell_t
{
  special_delivery_t( monk_t& p ) : monk_spell_t( "special_delivery", &p, p.passives.special_delivery )
  {
    may_block = may_dodge = may_parry = true;
    background                        = true;
    trigger_gcd                       = timespan_t::zero();
    aoe                               = -1;
    attack_power_mod.direct           = data().effectN( 1 ).ap_coeff();
    radius                            = data().effectN( 1 ).radius();
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( p()->talent.brewmaster.special_delivery->effectN( 1 ).base_value() );
  }

  double cost() const override
  {
    return 0;
  }
};

// ==========================================================================
// Black Ox Brew
// ==========================================================================

struct black_ox_brew_t : public monk_spell_t
{
  special_delivery_t* delivery;
  black_ox_brew_t( monk_t& player, util::string_view options_str )
    : monk_spell_t( "black_ox_brew", &player, player.talent.brewmaster.black_ox_brew ),
      delivery( new special_delivery_t( player ) )
  {
    parse_options( options_str );

    harmful       = false;
    use_off_gcd   = true;
    energize_type = action_energize::NONE;  // disable resource gain from spell data
  }

  void execute() override
  {
    monk_spell_t::execute();

    // Refill Purifying Brew charges.
    p()->cooldown.purifying_brew->reset( true, -1 );

    // Refills Celestial Brew charges
    p()->cooldown.celestial_brew->reset( true, -1 );

    p()->resource_gain( RESOURCE_ENERGY, p()->talent.brewmaster.black_ox_brew->effectN( 1 ).base_value(),
                        p()->gain.black_ox_brew_energy );

    if ( p()->talent.brewmaster.special_delivery->ok() )
    {
      delivery->set_target( target );
      delivery->execute();
    }

    if ( p()->talent.brewmaster.celestial_flames->ok() )
      p()->buff.celestial_flames->trigger();
  }
};

// ==========================================================================
// Roll
// ==========================================================================

struct roll_t : public monk_spell_t
{
  roll_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "roll", player, ( player->talent.general.chi_torpedo ? spell_data_t::nil() : player->spec.roll ) )
  {
    cast_during_sck = true;

    parse_options( options_str );

    if ( player->talent.general.improved_roll )
        cooldown->charges += 1;

    if ( player->talent.general.celerity )
    {
      cooldown->duration += player->talent.general.celerity->effectN( 1 ).time_value();
      cooldown->charges += (int)player->talent.general.celerity->effectN( 2 ).base_value();
    }

    if ( player->legendary.swiftsure_wraps.ok() )
      cooldown->charges += (int)player->legendary.swiftsure_wraps->effectN( 1 ).base_value();

  }

};

// ==========================================================================
// Chi Torpedo
// ==========================================================================

struct chi_torpedo_t : public monk_spell_t
{
  chi_torpedo_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "chi_torpedo", player, player->talent.general.chi_torpedo )
  {
    parse_options( options_str );

    cast_during_sck = true;

    if ( player->talent.general.improved_roll )
        cooldown->charges += 1;

    if ( player->legendary.swiftsure_wraps.ok() )
      cooldown->charges += (int)player->legendary.swiftsure_wraps->effectN( 1 ).base_value();

  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.chi_torpedo->trigger();
  }
};

// ==========================================================================
// Serenity
// ==========================================================================

struct serenity_t : public monk_spell_t
{
  serenity_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "serenity", player, player->talent.windwalker.serenity )
  {
    parse_options( options_str );
    harmful         = false;
    cast_during_sck = true;
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.serenity->trigger();
  }
};

// ==========================================================================
// Crackling Jade Lightning
// ==========================================================================

struct crackling_jade_lightning_t : public monk_spell_t
{
  crackling_jade_lightning_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "crackling_jade_lightning", &p, p.spec.crackling_jade_lightning )
  {
    sef_ability                     = sef_ability_e::SEF_CRACKLING_JADE_LIGHTNING;
    may_combo_strike                = true;
    trigger_faeline_stomp           = true;
    trigger_bountiful_brew          = true;

    parse_options( options_str );

    channeled = tick_zero = tick_may_crit = true;
    dot_duration                          = data().duration();
    hasted_ticks = false;  // Channeled spells always have hasted ticks. Use hasted_ticks = false to disable the
                           // increase in the number of ticks.
    interrupt_auto_attack = true;
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;

  }

  timespan_t tick_time( const action_state_t* state ) const override
  {
    timespan_t t = base_tick_time;
    if ( channeled || hasted_ticks )
    {
      t *= state->haste;
    }
    return t;
  }

  double cost_per_tick( resource_e resource ) const override
  {
    double c = monk_spell_t::cost_per_tick( resource );

    if ( p()->buff.the_emperors_capacitor->check() && resource == RESOURCE_ENERGY )
      c *= 1 + ( p()->buff.the_emperors_capacitor->current_stack *
                 p()->buff.the_emperors_capacitor->data().effectN( 2 ).percent() );

    return c;
  }

  double cost() const override
  {
    double c = monk_spell_t::cost();

    if ( p()->buff.the_emperors_capacitor->check() )
      c *= 1 + ( p()->buff.the_emperors_capacitor->current_stack *
                 p()->buff.the_emperors_capacitor->data().effectN( 2 ).percent() );

    return c;
  }

  double composite_persistent_multiplier( const action_state_t* action_state ) const override
  {
    double pm = monk_spell_t::composite_persistent_multiplier( action_state );

    if ( p()->buff.the_emperors_capacitor->check() )
      pm *= 1 + p()->buff.the_emperors_capacitor->check_stack_value();

    return pm;
  }

  void last_tick( dot_t* dot ) override
  {
    monk_spell_t::last_tick( dot );

    if ( p()->buff.the_emperors_capacitor->up() )
      p()->buff.the_emperors_capacitor->expire();

    // Reset swing timer
    if ( player->main_hand_attack )
    {
      player->main_hand_attack->cancel();
      if ( !player->main_hand_attack->target->is_sleeping() )
      {
        player->main_hand_attack->schedule_execute();
      }
    }

    if ( player->off_hand_attack )
    {
      player->off_hand_attack->cancel();
      if ( !player->off_hand_attack->target->is_sleeping() )
      {
        player->off_hand_attack->schedule_execute();
      }
    }
  }
};

// ==========================================================================
// Breath of Fire
// ==========================================================================

// Dragonfire Brew ==========================================================
struct dragonfire_brew_t : public monk_spell_t
{
  dragonfire_brew_t( monk_t& p ) : monk_spell_t( "dragonfire_brew", &p, p.passives.dragonfire_brew )
  {
    background    = true;
    aoe        = -1;
  }
};

struct breath_of_fire_dot_t : public monk_spell_t
{
  breath_of_fire_dot_t( monk_t& p ) : monk_spell_t( "breath_of_fire_dot", &p, p.passives.breath_of_fire_dot )
  {
    background    = true;
    tick_may_crit = may_crit = true;
    hasted_ticks             = false;
    reduced_aoe_targets      = 1.0;
    full_amount_targets      = 1;
  }
};

struct breath_of_fire_t : public monk_spell_t
{
  dragonfire_brew_t* dragonfire;
  breath_of_fire_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "breath_of_fire", &p, p.talent.brewmaster.breath_of_fire ), 
      dragonfire( new dragonfire_brew_t( p ) )
  {
    parse_options( options_str );
    gcd_type = gcd_haste_type::NONE;

    aoe                    = -1;
    reduced_aoe_targets    = 1.0;
    full_amount_targets    = 1;
    trigger_faeline_stomp  = true;
    trigger_bountiful_brew = true;
    cast_during_sck        = true;

    add_child( p.active_actions.breath_of_fire );
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown->duration;

    // Update the cooldown if Blackout Combo is up
    if ( p()->buff.blackout_combo->up() )
    {
      // Saved as 3 seconds
      cd += ( -1 * timespan_t::from_seconds( p()->buff.blackout_combo->data().effectN( 2 ).base_value() ) );
      p()->proc.blackout_combo_breath_of_fire->occur();
      p()->buff.blackout_combo->expire();
    }

    monk_spell_t::update_ready( cd );
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    if ( p()->talent.brewmaster.dragonfire_brew->ok() )
    {
      // Currently value is saved as 100% and each of the values is a divisable of 33%
      auto bof_stagger_bonus = p()->talent.brewmaster.dragonfire_brew->effectN( 2 ).percent();
      if ( p()->buff.light_stagger->check() )
        am *= 1 + ( ( 1 / 3 ) * bof_stagger_bonus);
      if ( p()->buff.moderate_stagger->check() )
        am *= 1 + ( ( 2 / 3 ) * bof_stagger_bonus );
      if ( p()->buff.heavy_stagger->check() )
        am *= 1 + bof_stagger_bonus;
    }
    am *= p()->cache.mastery_value();

    return am;
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( p()->shared.charred_passions && p()->shared.charred_passions->ok() )
      p()->buff.charred_passions->trigger();
  }

  void impact( action_state_t* s ) override
  {
    if ( p()->user_options.no_bof_dot == 1 )
      s->result_amount = 0;

    monk_spell_t::impact( s );

    monk_td_t& td = *this->get_td( s->target );

    if ( p()->user_options.no_bof_dot == 0 && ( td.debuff.keg_smash->up() || td.debuff.fallen_monk_keg_smash->up() ||
         td.debuff.sinister_teaching_fallen_monk_keg_smash->up() ) )
    {
      p()->active_actions.breath_of_fire->target = s->target;
      if ( p()->buff.blackout_combo->up() )
        p()->active_actions.breath_of_fire->base_multiplier =
            1 + p()->buff.blackout_combo->data().effectN( 5 ).percent();
      else
        p()->active_actions.breath_of_fire->base_multiplier = 1; 
      p()->active_actions.breath_of_fire->execute();
    }

    if ( p()->talent.brewmaster.dragonfire_brew->ok() )
    {
      for ( int i = 0; i < (int)p()->talent.brewmaster.dragonfire_brew->effectN( 1 ).base_value(); i++ )
        dragonfire->execute();
    }
  }
};

// ==========================================================================
// Fortifying Brew
// ==========================================================================

struct fortifying_ingredients_t : public monk_absorb_t
{
  fortifying_ingredients_t( monk_t& p )
    : monk_absorb_t( "fortifying_ingredients", p, p.passives.fortifying_ingredients )
  {
    harmful = may_crit = false;
  }
};

struct fortifying_brew_t : public monk_spell_t
{
  special_delivery_t* delivery;
  fortifying_ingredients_t* fortifying_ingredients;

  fortifying_brew_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "fortifying_brew", &p, p.find_spell( 115203 ) ),
      delivery( new special_delivery_t( p ) ),
      fortifying_ingredients( new fortifying_ingredients_t( p ) )
  {
    cast_during_sck = true;

    parse_options( options_str );

    harmful = may_crit = false;

    if ( p.talent.general.expeditious_fortification )
      cooldown->duration += p.talent.general.expeditious_fortification->effectN( 1 ).time_value();
  }

  bool ready() override
  {
    if ( !p()->talent.general.fortifying_brew->ok() )
      return false;

    return monk_spell_t::ready();
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.fortifying_brew->trigger();

    if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->talent.brewmaster.special_delivery->ok() )
      {
        delivery->set_target( target );
        delivery->execute();
      }

      if ( p()->talent.brewmaster.celestial_flames->ok() )
        p()->buff.celestial_flames->trigger();
    }

    if ( p()->conduit.fortifying_ingredients->ok() )
    {
      double absorb_percent = p()->conduit.fortifying_ingredients.percent();
      fortifying_ingredients->base_dd_min = p()->resources.max[RESOURCE_HEALTH] * absorb_percent;
      fortifying_ingredients->base_dd_max = p()->resources.max[RESOURCE_HEALTH] * absorb_percent;
      fortifying_ingredients->execute();
    }

  }
};

// ==========================================================================
// Exploding Keg
// ==========================================================================
// Exploding Keg Proc ==============================================
struct exploding_keg_proc_t : public monk_spell_t
{
  exploding_keg_proc_t( monk_t* p ) : monk_spell_t( "exploding_keg_proc", p, p->find_spell( 388867 ) )
  {
    background = dual = true;
    proc              = true;
  }
};

struct exploding_keg_t : public monk_spell_t
{
  exploding_keg_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "exploding_keg", &p, p.talent.brewmaster.exploding_keg )
  {
    cast_during_sck = true;

    parse_options( options_str );

    aoe             = -1;
    radius          = data().effectN( 1 ).radius();
    range           = data().max_range();

    if ( p.talent.brewmaster.exploding_keg->ok() )
      add_child( p.active_actions.exploding_keg );
  }

  void execute() override
  {
    p()->buff.exploding_keg->trigger();

    monk_spell_t::execute();
  }

  timespan_t travel_time() const override
  {
    // Always has the same time to land regardless of distance, probably represented there.
    return timespan_t::from_seconds( data().missile_speed() );
  }

  // Ensuring that we can't cast on a target that is too close
  bool target_ready( player_t* candidate_target ) override
  {
    if ( player->get_player_distance( *candidate_target ) < data().min_range() )
    {
      return false;
    }

    return monk_spell_t::target_ready( candidate_target );
  }
};

// ==========================================================================
// Stagger Damage
// ==========================================================================

struct stagger_self_damage_t : public residual_action::residual_periodic_action_t<monk_spell_t>
{
  stagger_self_damage_t( monk_t* p ) : base_t( "stagger_self_damage", p, p->passives.stagger_self_damage )
  {
    // Just get dot duration from heavy stagger spell data
    auto s = p->find_spell( 124273 );
    assert( s );

    dot_duration = s->duration();
    dot_duration += timespan_t::from_seconds( p->talent.brewmaster.bob_and_weave->effectN( 1 ).base_value() / 10 );
    base_tick_time = timespan_t::from_millis( 500 );
    hasted_ticks = tick_may_crit = false;
    target                       = p;
  }

  void impact( action_state_t* s ) override
  {
    if ( p()->conduit.evasive_stride->ok() )
    {
      // Tooltip shows this as (Value / 10)%; ie: (25 / 10)% = 2.5%.
      // For roll purpose we need this to go to 0.025 or value / 1000
      if ( p()->buff.shuffle->up() && p()->buff.heavy_stagger->up() &&
           rng().roll( p()->conduit.evasive_stride.value() / 1000 ) )
      {
        p()->active_actions.evasive_stride->base_dd_min = s->result_amount;
        p()->active_actions.evasive_stride->base_dd_max = s->result_amount;
        p()->active_actions.evasive_stride->execute();
        s->result_amount = 0;
      }
    }

    base_t::impact( s );
    p()->buff.shuffle->up();  // benefit tracking
    p()->stagger_damage_changed();
  }

  void assess_damage( result_amount_type type, action_state_t* s ) override
  {
    base_t::assess_damage( type, s );

    p()->stagger_tick_damage.push_back( { s->result_amount } );

    p()->sample_datas.stagger_effective_damage_timeline.add( sim->current_time(), s->result_amount );
    p()->sample_datas.stagger_damage->add( s->result_amount );

    if ( p()->buff.light_stagger->check() )
    {
      p()->sample_datas.light_stagger_damage->add( s->result_amount );
    }
    else if ( p()->buff.moderate_stagger->check() )
    {
      p()->sample_datas.moderate_stagger_damage->add( s->result_amount );
    }
    else if ( p()->buff.heavy_stagger->check() )
    {
      p()->sample_datas.heavy_stagger_damage->add( s->result_amount );
    }
  }

  void last_tick( dot_t* d ) override
  {
    base_t::last_tick( d );
    p()->stagger_damage_changed( true );
  }

  proc_types proc_type() const override
  {
    return PROC1_ANY_DAMAGE_TAKEN;
  }

  void init() override
  {
    base_t::init();

    // We don't want this counted towards our dps
    stats->type = stats_e::STATS_NEUTRAL;
  }

  void delay_tick( timespan_t seconds )
  {
    dot_t* d = get_dot();
    if ( d->is_ticking() )
    {
      if ( d->tick_event )
      {
        d->tick_event->reschedule( d->tick_event->remains() + seconds );
        if ( d->end_event )
        {
          d->end_event->reschedule( d->end_event->remains() + seconds );
        }
      }
    }
  }

  /* Clears the dot and all damage. Used by Purifying Brew
   * Returns amount purged
   */
  double clear_all_damage()
  {
    dot_t* d              = get_dot();
    double amount_cleared = amount_remaining();

    d->cancel();
    cancel();
    p()->stagger_damage_changed();

    return amount_cleared;
  }

  /* Clears part of the stagger dot. Used by Purifying Brew
   * Returns amount purged
   */
  double clear_partial_damage_pct( double percent_amount )
  {
    dot_t* d              = get_dot();
    double amount_cleared = 0.0;

    if ( d->is_ticking() )
    {
      debug_cast<residual_action::residual_periodic_state_t*>( d->state );

      auto ticks_left                   = d->ticks_left();
      auto damage_remaining_initial     = amount_remaining();
      auto damage_remaining_after_clear = damage_remaining_initial * ( 1.0 - percent_amount );
      amount_cleared                    = damage_remaining_initial - damage_remaining_after_clear;
      set_tick_amount( damage_remaining_after_clear / ticks_left );
      assert( std::fabs( amount_remaining() - damage_remaining_after_clear ) < 1.0 &&
              "stagger remaining amount after clear does not match" );

      sim->print_debug( "{} partially clears stagger by {} (requested:  {:.2f}%). Damage remaining is {}.",
                        player->name(), amount_cleared, percent_amount * 100.0, damage_remaining_after_clear );
    }
    else
    {
      sim->print_debug( "{} no active stagger to clear (requested pct: {}).", player->name(), percent_amount * 100.0 );
    }

    p()->stagger_damage_changed();

    return amount_cleared;
  }

  /* Clears part of the stagger dot. Used by Staggering Strikes Azerite Trait
   * Returns amount purged
   */
  double clear_partial_damage_amount( double amount )
  {
    dot_t* d              = get_dot();
    double amount_cleared = 0.0;

    if ( d->is_ticking() )
    {
      debug_cast<residual_action::residual_periodic_state_t*>( d->state );
      auto ticks_left                   = d->ticks_left();
      auto damage_remaining_initial     = amount_remaining();
      auto damage_remaining_after_clear = std::fmax( damage_remaining_initial - amount, 0 );
      amount_cleared                    = damage_remaining_initial - damage_remaining_after_clear;
      set_tick_amount( damage_remaining_after_clear / ticks_left );
      assert( std::fabs( amount_remaining() - damage_remaining_after_clear ) < 1.0 &&
              "stagger remaining amount after clear does not match" );

      sim->print_debug( "{} partially clears stagger by {} (requested: {}). Damage remaining is {}.", player->name(),
                        amount_cleared, amount, damage_remaining_after_clear );
    }
    else
    {
      sim->print_debug( "{} no active stagger to clear (requested amount: {}).", player->name(), amount );
    }

    p()->stagger_damage_changed();

    return amount_cleared;
  }

  // adds amount to residual actions tick amount
  void set_tick_amount( double amount )
  {
    dot_t* dot = get_dot();
    if ( dot )
    {
      auto rd_state         = debug_cast<residual_action::residual_periodic_state_t*>( dot->state );
      rd_state->tick_amount = amount;
    }
  }

  bool stagger_ticking()
  {
    dot_t* d = get_dot();
    return d->is_ticking();
  }

  double tick_amount()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return base_ta( d->state );
    return 0;
  }

  double tick_ratio_to_hp()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return base_ta( d->state ) / p()->resources.max[ RESOURCE_HEALTH ];
    return 0;
  }

  double amount_remaining()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return base_ta( d->state ) * d->ticks_left();
    return 0;
  }

  double stagger_total()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return base_ta( d->state ) * static_cast<double>( dot_duration / base_tick_time );
    return 0;
  }

  double amount_remaining_to_total()
  {
    dot_t* d = get_dot();
    if ( d && d->state )
      return ( base_ta( d->state ) * d->ticks_left() ) /
             ( base_ta( d->state ) * static_cast<double>( dot_duration / base_tick_time ) );
    return 0;
  }
};

// ==========================================================================
// Purifying Brew
// ==========================================================================

// Gai Plin's Imperial Brew Heal ============================================
struct gai_plins_imperial_brew_heal_t : public monk_heal_t
{
  gai_plins_imperial_brew_heal_t( monk_t& p ) : 
      monk_heal_t( "gai_plins_imperial_brew_heal", p, p.passives.gai_plins_imperial_brew_heal )
  {
    background = true;
  }

  void init() override
  {
    monk_heal_t::init();

    snapshot_flags = update_flags = 0;
  }
};

struct purifying_brew_t : public monk_spell_t
{
  special_delivery_t* delivery;
  gai_plins_imperial_brew_heal_t* gai_plin;

  purifying_brew_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "purifying_brew", &p, p.talent.brewmaster.purifying_brew ), 
      delivery( new special_delivery_t( p ) ),
      gai_plin( new gai_plins_imperial_brew_heal_t( p ) )
  {
    parse_options( options_str );

    harmful         = false;
    cast_during_sck = true;

    cooldown->charges += (int)p.talent.brewmaster.improved_purifying_brew->effectN( 1 ).base_value();

    if ( p.talent.brewmaster.light_brewing->ok() )
      cooldown->duration *= 1 + p.talent.brewmaster.light_brewing->effectN( 2 ).percent();  // -20
  }

  bool ready() override
  {
    // Irrealistic of in-game, but let's make sure stagger is actually present
    if ( !p()->active_actions.stagger_self_damage->stagger_ticking() )
      return false;

    return monk_spell_t::ready();
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( p()->talent.brewmaster.pretense_of_instability->ok() )
      p()->buff.pretense_of_instability->trigger();

    if ( p()->talent.brewmaster.special_delivery->ok() )
    {
      delivery->set_target( target );
      delivery->execute();
    }

    if ( p()->talent.brewmaster.celestial_flames->ok() )
      p()->buff.celestial_flames->trigger();

    if ( p()->buff.blackout_combo->up() )
    {
      p()->active_actions.stagger_self_damage->delay_tick( timespan_t::from_seconds(
          p()->buff.blackout_combo->data().effectN( 4 ).base_value() ) );
      p()->proc.blackout_combo_purifying_brew->occur();
      p()->buff.blackout_combo->expire();
    }

    if ( p()->talent.brewmaster.improved_celestial_brew->ok() )
    {
      int count = 1;
      for ( auto&& buff : { p()->buff.light_stagger, p()->buff.moderate_stagger, p()->buff.heavy_stagger } )
      {
        if ( buff && buff->check() )
        {
          p()->buff.purified_chi->trigger( count );
        }
        count++;
      }
    }

    if ( p()->legendary.celestial_infusion->ok() &&
         rng().roll( p()->legendary.celestial_infusion->effectN( 2 ).percent() ) )
      p()->cooldown.purifying_brew->reset( true, 1 );

    // Reduce stagger damage
    auto purifying_percent = data().effectN( 1 ).percent();
    if ( p()->buff.brewmasters_rhythm->up() && p()->sets->has_set_bonus( MONK_BREWMASTER, T29, B4 ) )
      purifying_percent +=
          p()->buff.brewmasters_rhythm->stack() * p()->sets->set( MONK_BREWMASTER, T29, B4 )->effectN( 1 ).percent();

    auto amount_cleared =
        p()->active_actions.stagger_self_damage->clear_partial_damage_pct( purifying_percent );
    p()->sample_datas.purified_damage->add( amount_cleared );
    p()->buff.recent_purifies->trigger( 1, amount_cleared );

    if ( p()->talent.brewmaster.gai_plins_imperial_brew->ok() )
    {
      auto amount_healed = amount_cleared * p()->talent.brewmaster.gai_plins_imperial_brew->effectN( 1 ).percent();
      gai_plin->base_dd_min = amount_healed;
      gai_plin->base_dd_max = amount_healed;
      gai_plin->target      = p();
      gai_plin->execute();
    }
  }
};

// ==========================================================================
// Mana Tea
// ==========================================================================
// Manatee
//                   _.---.._
//     _        _.-'         ''-.
//   .'  '-,_.-'                 '''.
//  (       _                     o  :
//   '._ .-'  '-._         \  \-  ---]
//                 '-.___.-')  )..-'
//                          (_/lame

struct mana_tea_t : public monk_spell_t
{
  mana_tea_t( monk_t& p, util::string_view options_str ) : monk_spell_t( "mana_tea", &p, p.talent.mistweaver.mana_tea )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.mana_tea->trigger();
  }
};

// ==========================================================================
// Thunder Focus Tea
// ==========================================================================

struct thunder_focus_tea_t : public monk_spell_t
{
  thunder_focus_tea_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "Thunder_focus_tea", &p, p.spec.thunder_focus_tea )
  {
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.thunder_focus_tea->trigger( p()->buff.thunder_focus_tea->max_stack() );
  }
};

// ==========================================================================
// Dampen Harm
// ==========================================================================

struct dampen_harm_t : public monk_spell_t
{
  dampen_harm_t( monk_t& p, util::string_view options_str ) : monk_spell_t( "dampen_harm", &p, p.talent.general.dampen_harm )
  {
    parse_options( options_str );
    cast_during_sck = true;
    harmful     = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->buff.dampen_harm->trigger();
  }
};

// ==========================================================================
// Diffuse Magic
// ==========================================================================

struct diffuse_magic_t : public monk_spell_t
{
  diffuse_magic_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "diffuse_magic", &p, p.talent.general.diffuse_magic )
  {
    parse_options( options_str );
    cast_during_sck = true;
    harmful         = false;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  void execute() override
  {
    p()->buff.diffuse_magic->trigger();
    monk_spell_t::execute();
  }
};

// ==========================================================================
// Invoke Xuen, the White Tiger
// ==========================================================================

struct xuen_spell_t : public monk_spell_t
{
  xuen_spell_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "invoke_xuen_the_white_tiger", p, p->talent.windwalker.invoke_xuen_the_white_tiger )
  {
    parse_options( options_str );

    cast_during_sck = true;
    harmful         = false;
    may_proc_bron   = true;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.xuen.spawn( p()->talent.windwalker.invoke_xuen_the_white_tiger->duration(), 1 );

    p()->buff.invoke_xuen->trigger();

    if ( p()->shared.invokers_delight && p()->shared.invokers_delight->ok() )
        p()->buff.invokers_delight->trigger();
  }
};

struct empowered_tiger_lightning_t : public monk_spell_t
{
  empowered_tiger_lightning_t( monk_t& p )
    : monk_spell_t( "empowered_tiger_lightning", &p, p.passives.empowered_tiger_lightning )
  {
    background = true;
    may_crit   = false;
    may_miss   = true;
  }

  // For some reason this is a yellow spell that is not following the normal hit rules
  double miss_chance( double hit, player_t* t ) const override
  {
    double miss = monk_spell_t::miss_chance( hit, t );
    miss += 0.03 + ( 0.015 * ( t->level() - p()->level() ) );
    return miss;
  }

  bool ready() override
  {
    return p()->talent.windwalker.empowered_tiger_lightning->ok();
  }
};

struct call_to_arms_empowered_tiger_lightning_t : public monk_spell_t
{
  call_to_arms_empowered_tiger_lightning_t( monk_t& p )
    : monk_spell_t( "empowered_tiger_lightning_call_to_arms", &p, p.passives.call_to_arms_empowered_tiger_lightning )
  {
    background = true;
    may_crit   = false;
    may_miss   = true;
  }

  // For some reason this is a yellow spell that is not following the normal hit rules
  double miss_chance( double hit, player_t* t ) const override
  {
    double miss = monk_spell_t::miss_chance( hit, t );
    miss += 0.03 + ( 0.015 * ( t->level() - p()->level() ) );
    return miss;
  }

  bool ready() override
  {
    return p()->shared.call_to_arms->ok() && p()->talent.windwalker.empowered_tiger_lightning->ok();
  }
};

struct fury_of_xuen_empowered_tiger_lightning_t : public monk_spell_t
{
  fury_of_xuen_empowered_tiger_lightning_t( monk_t& p )
    : monk_spell_t( "empowered_tiger_lightning_fury_of_xuen", &p, p.passives.empowered_tiger_lightning )
  {
    background = true;
    may_crit   = false;
    may_miss   = true;
  }

  // For some reason this is a yellow spell that is not following the normal hit rules
  double miss_chance( double hit, player_t* t ) const override
  {
    double miss = monk_spell_t::miss_chance( hit, t );
    miss += 0.03 + ( 0.015 * ( t->level() - p()->level() ) );
    return miss;
  }

  bool ready() override
  {
    return p()->talent.windwalker.empowered_tiger_lightning->ok();
  }
};

// ==========================================================================
// Invoke Niuzao, the Black Ox
// ==========================================================================

struct niuzao_spell_t : public monk_spell_t
{
  niuzao_spell_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "invoke_niuzao_the_black_ox", p, p->talent.brewmaster.invoke_niuzao_the_black_ox )
  {
    parse_options( options_str );

    cast_during_sck = true;
    harmful         = false;
    may_proc_bron   = true;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.niuzao.spawn( p()->talent.brewmaster.invoke_niuzao_the_black_ox->duration(), 1 );

    p()->buff.invoke_niuzao->trigger();

    if ( p()->shared.invokers_delight && p()->shared.invokers_delight->ok() )
      p()->buff.invokers_delight->trigger();
  }
};

// ==========================================================================
// Invoke Chi-Ji, the Red Crane
// ==========================================================================

struct chiji_spell_t : public monk_spell_t
{
  chiji_spell_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "invoke_chiji_the_red_crane", p, p->talent.mistweaver.invoke_chi_ji_the_red_crane )
  {
    parse_options( options_str );

    harmful       = false;
    may_proc_bron = true;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.chiji.spawn( p()->talent.mistweaver.invoke_chi_ji_the_red_crane->duration(), 1 );

    p()->buff.invoke_chiji->trigger();

    if ( p()->shared.invokers_delight && p()->shared.invokers_delight->ok() )
        p()->buff.invokers_delight->trigger();
  }
};

// ==========================================================================
// Invoke Yu'lon, the Jade Serpent
// ==========================================================================

struct yulon_spell_t : public monk_spell_t
{
  yulon_spell_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "invoke_yulon_the_jade_serpent", p, p->spec.invoke_yulon )
  {
    parse_options( options_str );

    harmful       = false;
    may_proc_bron = true;
    // Forcing the minimum GCD to 750 milliseconds
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  bool ready() override
  {
    if ( p()->talent.mistweaver.invoke_chi_ji_the_red_crane->ok() )
      return false;

    return monk_spell_t::ready();
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.yulon.spawn( p()->spec.invoke_yulon->duration(), 1 );

    if ( p()->shared.invokers_delight && p()->shared.invokers_delight->ok() )
        p()->buff.invokers_delight->trigger();
  }
};

// ==========================================================================
// Summon White Tiger Statue
// ==========================================================================

struct summon_white_tiger_statue_spell_t : public monk_spell_t
{
  summon_white_tiger_statue_spell_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "summon_white_tiger_statue", p, p->talent.general.summon_white_tiger_statue )
  {
    parse_options( options_str );

    harmful               = false;
    may_proc_bron         = false; // TODO Check if this procs Bron
    trigger_faeline_stomp = true;
  }

  void execute() override
  {
    monk_spell_t::execute();

    p()->pets.white_tiger_statue.spawn( p()->talent.general.summon_white_tiger_statue->duration(), 1 );
  }
};

// ==========================================================================
// Chi Surge
// ==========================================================================

struct chi_surge_t : public monk_spell_t
{
  chi_surge_t( monk_t& p ) : monk_spell_t( "chi_surge", &p, p.talent.brewmaster.chi_surge->effectN( 1 ).trigger() )
  {
    harmful           = true;
    dual = background = true;
    aoe               = -1;
    school            = SCHOOL_NATURE;

    spell_power_mod.tick =
      data().effectN( 2 ).base_value() / 100; // Saved as 45
  }

  double composite_spell_power() const override
  {
    return std::max( monk_spell_t::composite_spell_power(), monk_spell_t::composite_attack_power() );
  }

  double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    return monk_spell_t::composite_persistent_multiplier( s ) / s->n_targets;
  }

  void execute() override
  {
    monk_spell_t::execute();

    int targets_hit = std::min( 5U, execute_state->n_targets );

    if ( targets_hit > 0 )
    {
      double cdr = p()->talent.brewmaster.chi_surge->effectN( 1 ).base_value(); // Saved as 4
      p()->cooldown.weapons_of_order->adjust( timespan_t::from_seconds( -1 * cdr * targets_hit ) );
      p()->proc.chi_surge->occur();
    }
  }
};

// ==========================================================================
// Weapons of Order - Kyrian Covenant Ability
// ==========================================================================

struct weapons_of_order_t : public monk_spell_t
{
  chi_surge_t* chi_surge;

  weapons_of_order_t( monk_t& p, util::string_view options_str )
    : monk_spell_t( "weapons_of_order", &p, p.shared.weapons_of_order ),
      chi_surge( new chi_surge_t( p ) )
  {
    parse_options( options_str );
    may_combo_strike            = true;
    harmful                     = false;
    base_dd_min                 = 0;
    base_dd_max                 = 0;
    cast_during_sck             = true;

    add_child( chi_surge );
  }

  void execute() override
  {
    p()->buff.weapons_of_order->trigger();

    monk_spell_t::execute();

    if ( p()->specialization() == MONK_BREWMASTER )
    {
      p()->cooldown.keg_smash->reset( true, 1 );

      // TODO: Does this stack with Effusive Anima Accelerator?
      if ( p()->talent.brewmaster.chi_surge->ok() )
      {
        chi_surge->set_target( target );
        chi_surge->execute();
      }
    }

    if ( p()->shared.call_to_arms && p()->shared.call_to_arms->ok() )
    {
      switch ( p()->specialization() )
      {
        case MONK_BREWMASTER:
          p()->pets.call_to_arms_niuzao.spawn( p()->passives.call_to_arms_invoke_niuzao->duration(), 1 );
          p()->buff.invoke_niuzao->trigger( p()->passives.call_to_arms_invoke_niuzao->duration() );
          break;
        case MONK_MISTWEAVER:
        {
          if ( p()->talent.mistweaver.invoke_chi_ji_the_red_crane->ok() )
            p()->pets.chiji.spawn( p()->passives.call_to_arms_invoke_chiji->duration(), 1 );
          else
            p()->pets.yulon.spawn( p()->passives.call_to_arms_invoke_yulon->duration(), 1 );
          break;
        }
        case MONK_WINDWALKER:
        {
          p()->pets.call_to_arms_xuen.spawn( p()->passives.call_to_arms_invoke_xuen->duration(), 1 );
          p()->buff.invoke_xuen_call_to_arms->trigger();
          break;
        }
        default:
          break;
      }
    }
  }
};

// ==========================================================================
// Bonedust Brew - Necrolord Covenant Ability
// ==========================================================================
struct bountiful_brew_t : public monk_spell_t
{
  buff_t* lead_by_example;

  bountiful_brew_t( monk_t& p )
    : monk_spell_t( "bountiful_brew", &p, p.shared.bountiful_brew )
  {
    harmful            = false;
    cooldown->duration = timespan_t::zero();
    aoe                = -1;
    base_dd_min        = 0;
    base_dd_max        = 0;
    may_miss           = false;
    may_parry          = true;
  }

  void init_finished() override
  {
    monk_spell_t::init_finished();

    lead_by_example = buff_t::find( player, "lead_by_example" );
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    monk_spell_t::init();
    // disable the snapshot_flags for all multipliers except for crit
    snapshot_flags = update_flags = 0;
    snapshot_flags |= STATE_CRIT;
    snapshot_flags |= STATE_TGT_CRIT;
  }

  void execute() override
  {
    if ( !p()->covenant.necrolord )
      p()->buff.bonedust_brew_attenuation_hidden->trigger();
    else
      p()->buff.bonedust_brew_grounding_breath_hidden->trigger();

    monk_spell_t::execute();

    if ( p()->bugs )
      p()->buff.bonedust_brew->trigger( data().effectN( 1 ).time_value() );
    else
      p()->buff.bonedust_brew->extend_duration_or_trigger( data().effectN( 1 ).time_value() );

    // Force trigger Lead by Example Buff
    if ( lead_by_example )
      lead_by_example->trigger( data().effectN( 1 ).time_value() );
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    get_td( s->target )->debuff.bonedust_brew->extend_duration_or_trigger( data().effectN( 1 ).time_value() );
  }
};

struct bonedust_brew_t : public monk_spell_t
{
  bonedust_brew_t( monk_t& p, util::string_view options_str )
      : monk_spell_t( "bonedust_brew", &p, p.shared.bonedust_brew )
  {
    parse_options( options_str );
    may_combo_strike            = true;
    harmful                     = false;
    aoe                         = -1;
    base_dd_min                 = 0;
    base_dd_max                 = 0;
    cast_during_sck             = true;
    may_miss                    = false;
    may_parry                   = true;

    if ( p.talent.windwalker.dust_in_the_wind->ok() )
      radius *= 1 + p.talent.windwalker.dust_in_the_wind->effectN( 1 ).percent();
  }

  void execute() override
  {
    if ( !p()->covenant.necrolord )
      p()->buff.bonedust_brew_attenuation_hidden->trigger();
    else
      p()->buff.bonedust_brew_grounding_breath_hidden->trigger();

    monk_spell_t::execute();

    p()->buff.bonedust_brew->trigger();
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    get_td( s->target )->debuff.bonedust_brew->trigger();
  }
};

// ==========================================================================
// Bonedust Brew - Necrolord Covenant Damage
// ==========================================================================

struct bonedust_brew_damage_t : public monk_spell_t
{
  bonedust_brew_damage_t( monk_t& p ) : monk_spell_t( "bonedust_brew_dmg", &p, p.passives.bonedust_brew_dmg )
  {
    background = true;
  }

  void execute() override
  {
    monk_spell_t::execute();

    auto attenuation = p()->shared.attenuation;

    if ( attenuation && attenuation->ok() )
    {
      auto cooldown_reduction = attenuation->effectN( 2 ).time_value();

      if ( p()->buff.bonedust_brew_grounding_breath_hidden->up() )
      {
        p()->cooldown.bonedust_brew->adjust( cooldown_reduction, true );

        p()->buff.bonedust_brew_grounding_breath_hidden->decrement();
        p()->proc.bonedust_brew_reduction->occur();
      }
      else if ( p()->buff.bonedust_brew_attenuation_hidden->up() )
      {
        p()->cooldown.bonedust_brew->adjust( cooldown_reduction, true );

        p()->buff.bonedust_brew_attenuation_hidden->decrement();
        p()->proc.attenuation->occur();
      }
    }
  }
};

// ==========================================================================
// Bonedust Brew - Necrolord Covenant Heal
// ==========================================================================

struct bonedust_brew_heal_t : public monk_heal_t
{
  bonedust_brew_heal_t( monk_t& p ) : monk_heal_t( "bonedust_brew_heal", p, p.passives.bonedust_brew_heal )
  {
    background = true;
  }

  void execute() override
  {
    monk_heal_t::execute();

    auto attenuation = p()->shared.attenuation;

    if ( attenuation && attenuation->ok() )
    {
      auto cooldown_reduction = attenuation->effectN( 2 ).time_value();

      if ( p()->buff.bonedust_brew_grounding_breath_hidden->up() )
      {
        p()->cooldown.bonedust_brew->adjust( cooldown_reduction, true );

        p()->buff.bonedust_brew_grounding_breath_hidden->decrement();
        p()->proc.bonedust_brew_reduction->occur();
      }
      else if ( p()->buff.bonedust_brew_attenuation_hidden->up() )
      {
        p()->cooldown.bonedust_brew->adjust( cooldown_reduction, true );

        p()->buff.bonedust_brew_attenuation_hidden->decrement();
        p()->proc.attenuation->occur();
      }
    }
  }
};

// ==========================================================================
// Faeline Stomp - Ardenweald Covenant
// ==========================================================================

struct faeline_stomp_ww_damage_t : public monk_spell_t
{
  faeline_stomp_ww_damage_t( monk_t& p )
    : monk_spell_t( "faeline_stomp_ww_dmg", &p, p.passives.faeline_stomp_ww_damage )
  {
    background = true;
    ww_mastery = true;
  }
};

struct faeline_stomp_damage_t : public monk_spell_t
{
  faeline_stomp_damage_t( monk_t& p ) : monk_spell_t( "faeline_stomp_dmg", &p, p.passives.faeline_stomp_damage )
  {
    background = true;
    ww_mastery = true;

    attack_power_mod.direct = p.passives.faeline_stomp_damage->effectN( 1 ).ap_coeff();
    spell_power_mod.direct  = 0;
  }

  double composite_aoe_multiplier( const action_state_t* state ) const override
  {
    double cam = monk_spell_t::composite_aoe_multiplier( state );

    const std::vector<player_t*>& targets = state->action->target_list();

    if ( p()->shared.way_of_the_fae && p()->shared.way_of_the_fae->ok() && !targets.empty() )
    {
      double percent = ( p()->conduit.way_of_the_fae->ok() ? p()->conduit.way_of_the_fae.percent() : p()->shared.way_of_the_fae->effectN( 1 ).percent() );
      cam *= 1 + ( percent * std::min( (double)targets.size(), p()->shared.way_of_the_fae->effectN( 2 ).base_value() ) );
    }

    return cam;
  }
};

struct faeline_stomp_heal_t : public monk_heal_t
{
  faeline_stomp_heal_t( monk_t& p ) : monk_heal_t( "faeline_stomp_heal", p, p.passives.faeline_stomp_damage )
  {
    background = true;

    attack_power_mod.direct = 0;
    spell_power_mod.direct  = p.passives.faeline_stomp_damage->effectN( 2 ).sp_coeff();
  }

  void impact( action_state_t* s ) override
  {
    monk_heal_t::impact( s );

    if ( p()->shared.faeline_harmony && p()->shared.faeline_harmony->ok() )
      p()->buff.fae_exposure->trigger();
  }
};

struct faeline_stomp_t : public monk_spell_t
{
  faeline_stomp_damage_t* damage;
  faeline_stomp_heal_t* heal;
  faeline_stomp_ww_damage_t* ww_damage;
  int aoe_initial_cap;
  int ww_aoe_cap;
  faeline_stomp_t( monk_t& p, util::string_view options_str )
      : monk_spell_t( "faeline_stomp", &p, p.shared.faeline_stomp ),
      damage( new faeline_stomp_damage_t( p ) ),
      heal( new faeline_stomp_heal_t( p ) ),
      ww_damage( new faeline_stomp_ww_damage_t( p ) ),
      aoe_initial_cap( 0 ),
      ww_aoe_cap( 0 )
  {
    parse_options( options_str );
    may_combo_strike = true;
    cast_during_sck  = true;

    aoe = (int)data().effectN( 3 ).base_value();

    trigger_bountiful_brew      = true;
  }

  void execute() override
  {
    // Values are hard coded into the tooltip.
    // The initial hit is bugged and hitting 6 targets instead of 5
    aoe_initial_cap = ( p()->bugs ? 6 : 5 );
    ww_aoe_cap      = 5;

    monk_spell_t::execute();

    p()->buff.faeline_stomp_reset->expire();

    p()->buff.faeline_stomp->trigger();

    if ( p()->shared.faeline_harmony && p()->shared.faeline_harmony->ok() )
      p()->buff.fae_exposure->trigger();
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    // Only the first 5 targets are hit with any damage or healing
    if ( aoe_initial_cap > 0 )
    {
      heal->execute();

      damage->set_target( s->target );
      damage->execute();

      if ( p()->specialization() == MONK_WINDWALKER && ww_aoe_cap > 0 )
      {
        ww_damage->set_target( s->target );
        ww_damage->execute();
        ww_aoe_cap--;
      }
    }

    // The Stagger debuff is being applied to all targets even if they are damaged or not
    if ( p()->specialization() == MONK_BREWMASTER )
    {
      get_td( s->target )->debuff.faeline_stomp_brm->trigger();
      p()->buff.faeline_stomp_brm->trigger();
    }

    // Fae Exposure is applied to all targets, even if they are healed/damage or not
    if ( p()->shared.faeline_harmony && p()->shared.faeline_harmony->ok() )
      get_td( s->target )->debuff.fae_exposure->trigger();

    aoe_initial_cap--;
  }
};

// ==========================================================================
// Fallen Order - Venthyr Covenant Ability
// ==========================================================================

struct fallen_order_t : public monk_spell_t
{
  struct fallen_order_event_t : public event_t
  {
    std::vector<std::pair<specialization_e, timespan_t>> fallen_monks;
    timespan_t summon_interval;
    monk_t* p;

    fallen_order_event_t( monk_t* monk, std::vector<std::pair<specialization_e, timespan_t>> fm, timespan_t interval )
      : event_t( *monk, interval ), fallen_monks( std::move( fm ) ), summon_interval( interval ), p( monk )
    {
    }

    const char* name() const override
    {
      return "fallen_order_summon";
    }

    void execute() override
    {
      if ( fallen_monks.empty() )
        return;

      std::pair<specialization_e, timespan_t> fallen_monk_pair;

      fallen_monk_pair = fallen_monks.front();

      switch ( fallen_monk_pair.first )
      {
        case MONK_WINDWALKER:
        {
          p->pets.tiger_adept.spawn( fallen_monk_pair.second, 1 );
          p->buff.windwalking_venthyr->trigger();
          break;
        }
        case MONK_BREWMASTER:
          p->pets.ox_adept.spawn( fallen_monk_pair.second, 1 );
          break;
        case MONK_MISTWEAVER:
          p->pets.crane_adept.spawn( fallen_monk_pair.second, 1 );
          break;
        default:
          break;
      }
      fallen_monks.erase( fallen_monks.begin() );

      if ( !fallen_monks.empty() )
        make_event<fallen_order_event_t>( sim(), p, std::move( fallen_monks ), summon_interval );
    }
  };

  fallen_order_t( monk_t& p, util::string_view options_str ) : monk_spell_t( "fallen_order", &p, p.covenant.venthyr )
  {
    parse_options( options_str );
    harmful         = false;
    cast_during_sck = true;
    base_dd_min = 0;
    base_dd_max = 0;
  }

  void execute() override
  {
    monk_spell_t::execute();

    specialization_e spec       = p()->specialization();
    timespan_t summon_duration  = timespan_t::from_seconds( p()->covenant.venthyr->effectN( 4 ).base_value() );
    timespan_t primary_duration = p()->passives.fallen_monk_spec_duration->duration();
    std::vector<std::pair<specialization_e, timespan_t>> fallen_monks;

    // Monks alternate summoning primary spec and non-primary spec
    // 8 summons in total (4 primary and a mix of 4 non-primary)
    // for non-primary, each non-primary is summoned twice
    for ( int i = 0; i < 8; i++ )
    {
      switch ( spec )
      {
        case MONK_WINDWALKER:
        {
          if ( i % 2 )
            fallen_monks.emplace_back( MONK_WINDWALKER, primary_duration );
          else if ( i % 3 )
            fallen_monks.emplace_back( MONK_BREWMASTER, summon_duration );
          else
            fallen_monks.emplace_back( MONK_MISTWEAVER, summon_duration );
          break;
        }
        case MONK_BREWMASTER:
        {
          if ( i % 2 )
            fallen_monks.emplace_back( MONK_BREWMASTER, primary_duration );
          else if ( i % 3 )
            fallen_monks.emplace_back( MONK_WINDWALKER, summon_duration );
          else
            fallen_monks.emplace_back( MONK_MISTWEAVER, summon_duration );
          break;
        }
        case MONK_MISTWEAVER:
        {
          if ( i % 2 )
            fallen_monks.emplace_back( MONK_MISTWEAVER, primary_duration );
          else if ( i % 3 )
            fallen_monks.emplace_back( MONK_WINDWALKER, summon_duration );
          else
            fallen_monks.emplace_back( MONK_BREWMASTER, summon_duration );
          break;
        }
        default:
          break;
      }
    }

    p()->buff.fallen_order->trigger();

    if ( p()->legendary.sinister_teachings->ok() )
    {
      switch ( spec )
      {
        case MONK_BREWMASTER:
          p()->pets.sinister_teaching_ox_adept.spawn(
              timespan_t::from_seconds( p()->legendary.sinister_teachings->effectN( 2 ).base_value() ), 1 );
          break;
        case MONK_MISTWEAVER:
          p()->pets.sinister_teaching_crane_adept.spawn(
              timespan_t::from_seconds( p()->legendary.sinister_teachings->effectN( 2 ).base_value() ), 1 );
          break;
        case MONK_WINDWALKER:
          p()->pets.sinister_teaching_tiger_adept.spawn(
              timespan_t::from_seconds( p()->legendary.sinister_teachings->effectN( 2 ).base_value() ), 1 );
          break;
        default:
          break;
      }
    }

    make_event<fallen_order_event_t>( *sim, p(), std::move( fallen_monks ),
                                      p()->covenant.venthyr->effectN( 1 ).period() * 3 );
  }
};
}  // namespace spells

namespace heals
{
// ==========================================================================
// Soothing Mist
// ==========================================================================

struct soothing_mist_t : public monk_heal_t
{
  soothing_mist_t( monk_t& p ) : monk_heal_t( "soothing_mist", p, p.passives.soothing_mist_heal )
  {
    background = dual = true;

    tick_zero = true;
  }

  bool ready() override
  {
    if ( p()->buff.channeling_soothing_mist->check() )
      return false;

    return monk_heal_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    monk_heal_t::impact( s );

    p()->buff.channeling_soothing_mist->trigger();
  }

  void last_tick( dot_t* d ) override
  {
    monk_heal_t::last_tick( d );

    p()->buff.channeling_soothing_mist->expire();
  }
};

// ==========================================================================
// Gust of Mists
// ==========================================================================
// The mastery actually affects the Spell Power Coefficient but I am not sure if that
// would work normally. Using Action Multiplier since it APPEARS to calculate the same.
//
// TODO: Double Check if this works.

struct gust_of_mists_t : public monk_heal_t
{
  gust_of_mists_t( monk_t& p ) : monk_heal_t( "gust_of_mists", p, p.mastery.gust_of_mists->effectN( 2 ).trigger() )
  {
    background = dual      = true;
    spell_power_mod.direct = 1;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    am *= p()->cache.mastery_value();

    // Mastery's Effect 3 gives a flat add modifier of 0.1 Spell Power co-efficient
    // TODO: Double check calculation

    return am;
  }
};

// ==========================================================================
// Enveloping Mist
// ==========================================================================

struct enveloping_mist_t : public monk_heal_t
{
  gust_of_mists_t* mastery;

  enveloping_mist_t( monk_t& p, util::string_view options_str )
    : monk_heal_t( "enveloping_mist", p, p.spec.enveloping_mist )
  {
    parse_options( options_str );

    may_miss = false;

    dot_duration = p.spec.enveloping_mist->duration();
    if ( p.talent.mistweaver.mist_wrap )
      dot_duration += p.talent.mistweaver.mist_wrap->effectN( 1 ).time_value();

    mastery = new gust_of_mists_t( p );
  }

  double cost() const override
  {
    double c = monk_heal_t::cost();

    if ( p()->buff.lifecycles_enveloping_mist->check() )
      c *= 1 + p()->buff.lifecycles_enveloping_mist->check_value();  // saved as -20%

    return c;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = monk_heal_t::execute_time();

    if ( p()->buff.invoke_chiji_evm->check() )
      et *= 1 + p()->buff.invoke_chiji_evm->check_stack_value();

    if ( p()->buff.thunder_focus_tea->check() )
      et *= 1 + p()->spec.thunder_focus_tea->effectN( 3 ).percent();  // saved as -100

    return et;
  }

  void execute() override
  {
    monk_heal_t::execute();

    if ( p()->talent.mistweaver.lifecycles->ok() )
    {
      if ( p()->buff.lifecycles_enveloping_mist->up() )
        p()->buff.lifecycles_enveloping_mist->expire();
      p()->buff.lifecycles_vivify->trigger();
    }

    if ( p()->buff.thunder_focus_tea->up() )
    {
      p()->buff.thunder_focus_tea->decrement();
    }

    mastery->execute();
  }
};

// ==========================================================================
// Renewing Mist
// ==========================================================================
/*
Bouncing only happens when overhealing, so not going to bother with bouncing
*/
struct renewing_mist_t : public monk_heal_t
{
  gust_of_mists_t* mastery;

  renewing_mist_t( monk_t& p, util::string_view options_str ) : monk_heal_t( "renewing_mist", p, p.spec.renewing_mist )
  {
    parse_options( options_str );
    may_crit = may_miss = false;
    dot_duration        = p.passives.renewing_mist_heal->duration();

    mastery = new gust_of_mists_t( p );
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown->duration;

    if ( p()->buff.thunder_focus_tea->check() )
      cd *= 1 + p()->spec.thunder_focus_tea->effectN( 1 ).percent();

    monk_heal_t::update_ready( cd );
  }

  void execute() override
  {
    monk_heal_t::execute();

    mastery->execute();

    if ( p()->buff.thunder_focus_tea->up() )
    {
      p()->buff.thunder_focus_tea->decrement();
    }
  }

  void tick( dot_t* d ) override
  {
    monk_heal_t::tick( d );

    p()->buff.uplifting_trance->trigger();
  }
};

// ==========================================================================
// Vivify
// ==========================================================================

struct vivify_t : public monk_heal_t
{
  gust_of_mists_t* mastery;

  vivify_t( monk_t& p, util::string_view options_str ) : monk_heal_t( "vivify", p, p.spec.vivify )
  {
    parse_options( options_str );

    spell_power_mod.direct = data().effectN( 2 ).sp_coeff();

    if ( p.talent.general.vivacious_vivification->ok() )
      base_execute_time += p.talent.general.vivacious_vivification->effectN( 1 ).time_value();

    mastery = new gust_of_mists_t( p );
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p()->talent.general.improved_vivify->ok() )
      am *= 1 + p()->talent.general.improved_vivify->effectN( 1 ).percent();

    if ( p()->buff.uplifting_trance->check() )
      am *= 1 + p()->buff.uplifting_trance->check_value();

    return am;
  }

  double cost() const override
  {
    double c = monk_heal_t::cost();

    if ( p()->buff.thunder_focus_tea->check() && p()->talent.mistweaver.thunder_focus_tea->ok() )
      c *= 1 + p()->talent.mistweaver.thunder_focus_tea->effectN( 2 ).percent();  // saved as -100

    if ( p()->buff.lifecycles_vivify->check() )
      c *= 1 + p()->buff.lifecycles_vivify->check_value();  // saved as -25%

    return c;
  }

  void execute() override
  {
    monk_heal_t::execute();

    if ( p()->buff.thunder_focus_tea->up() )
    {
      p()->buff.thunder_focus_tea->decrement();
    }

    if ( p()->talent.mistweaver.lifecycles )
    {
      if ( p()->buff.lifecycles_vivify->up() )
        p()->buff.lifecycles_vivify->expire();

      p()->buff.lifecycles_enveloping_mist->trigger();
    }

    if ( p()->buff.uplifting_trance->up() )
      p()->buff.uplifting_trance->expire();

    if ( p()->mastery.gust_of_mists->ok() )
      mastery->execute();
  }
};

// ==========================================================================
// Essence Font
// ==========================================================================
// The spell only hits each player 3 times no matter how many players are in group
// The intended model is every 1/6 of a sec, it fires a bolt at a single target
// that is randomly selected from the pool of [allies that are within range that
// have not been the target of any of the 5 previous ticks]. If there only 3
// potential allies, then that set is empty half the time, and a bolt doesn't fire.

struct essence_font_t : public monk_spell_t
{
  struct essence_font_heal_t : public monk_heal_t
  {
    essence_font_heal_t( monk_t& p )
      : monk_heal_t( "essence_font_heal", p, p.spec.essence_font->effectN( 1 ).trigger() )
    {
      background = dual = true;
      aoe               = (int)p.spec.essence_font->effectN( 1 ).base_value();
    }

    double action_multiplier() const override
    {
      double am = monk_heal_t::action_multiplier();

      if ( p()->buff.refreshing_jade_wind->check() )
        am *= 1 + p()->buff.refreshing_jade_wind->check_value();

      return am;
    }
  };

  essence_font_heal_t* heal;

  essence_font_t( monk_t* p, util::string_view options_str )
    : monk_spell_t( "essence_font", p, p->spec.essence_font ), heal( new essence_font_heal_t( *p ) )
  {
    parse_options( options_str );

    may_miss = hasted_ticks = false;
    tick_zero               = true;

    base_tick_time = data().effectN( 1 ).base_value() * data().effectN( 1 ).period();

    add_child( heal );
  }
};

// ==========================================================================
// Revival
// ==========================================================================

struct revival_t : public monk_heal_t
{
  revival_t( monk_t& p, util::string_view options_str ) : monk_heal_t( "revival", p, p.spec.revival )
  {
    parse_options( options_str );

    may_miss = false;
    aoe      = -1;

    if ( sim->pvp_mode )
      base_multiplier *= 2;  // 2016-08-03
  }
};

// ==========================================================================
// Gift of the Ox
// ==========================================================================

struct gift_of_the_ox_t : public monk_heal_t
{
  gift_of_the_ox_t( monk_t& p, util::string_view options_str )
    : monk_heal_t( "gift_of_the_ox", p, p.passives.gift_of_the_ox_heal )
  {
    parse_options( options_str );
    harmful     = false;
    background  = true;
    target      = &p;
    trigger_gcd = timespan_t::zero();
  }

  bool ready() override
  {
    if ( !p()->talent.brewmaster.gift_of_the_ox->ok() )
      return false;

    return p()->buff.gift_of_the_ox->check();
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p()->talent.brewmaster.gift_of_the_ox->ok() )
      am *= p()->talent.brewmaster.gift_of_the_ox->effectN( 2 ).percent();

    return am;
  }

  void execute() override
  {
    monk_heal_t::execute();

    p()->buff.gift_of_the_ox->decrement();

    if ( p()->talent.brewmaster.tranquil_spirit->ok() )
    {
      // Reduce stagger damage
      auto amount_cleared = p()->active_actions.stagger_self_damage->clear_partial_damage_pct(
          p()->talent.brewmaster.tranquil_spirit->effectN( 1 ).percent() );
      p()->sample_datas.tranquil_spirit->add( amount_cleared );
      p()->proc.tranquil_spirit_goto->occur();
    }
  }
};

struct gift_of_the_ox_trigger_t : public monk_heal_t
{
  gift_of_the_ox_trigger_t( monk_t& p ) : monk_heal_t( "gift_of_the_ox_trigger", p, p.find_spell( 124507 ) )
  {
    background  = true;
    target      = &p;
    trigger_gcd = timespan_t::zero();
  }
};

struct gift_of_the_ox_expire_t : public monk_heal_t
{
  gift_of_the_ox_expire_t( monk_t& p ) : monk_heal_t( "gift_of_the_ox_expire", p, p.find_spell( 178173 ) )
  {
    background  = true;
    target      = &p;
    trigger_gcd = timespan_t::zero();
  }
};

// ==========================================================================
// Expel Harm
// ==========================================================================

struct expel_harm_dmg_t : public monk_spell_t
{
  expel_harm_dmg_t( monk_t* player ) : monk_spell_t( "expel_harm_damage", player, player->find_spell( 115129 ) )
  {
    background = true;
    may_crit   = false;
  }
};

struct expel_harm_t : public monk_heal_t
{
  expel_harm_dmg_t* dmg;
  expel_harm_t( monk_t& p, util::string_view options_str )
    : monk_heal_t( "expel_harm", p, p.spec.expel_harm ),
      dmg( new expel_harm_dmg_t( &p ) )
  {
    parse_options( options_str );

    target           = player;
    may_combo_strike = true;

    if ( p.spec.expel_harm_2_brm->ok() )
      cooldown->duration += p.spec.expel_harm_2_brm->effectN( 1 ).time_value();

    if ( p.talent.general.profound_rebuttal->ok() )
      crit_multiplier *= 1 + p.talent.general.profound_rebuttal->effectN( 1 ).percent();

    add_child( dmg );
  }

  double composite_crit_chance() const override
  {
    auto mm = monk_heal_t::composite_crit_chance();

    if ( p()->talent.general.vigorous_expulsion->ok() )
      mm += p()->talent.general.vigorous_expulsion->effectN( 2 ).percent();

    return mm;
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    if ( p()->conduit.harm_denial->ok() )
      am *= 1 + p()->conduit.harm_denial.percent();

    if ( p()->talent.general.vigorous_expulsion->ok() )
      am *= 1 + p()->talent.general.vigorous_expulsion->effectN( 1 ).percent();

    return am;
  }

  void execute() override
  {
    monk_heal_t::execute();

    if ( p()->specialization() == MONK_WINDWALKER && p()->spec.expel_harm_2_ww->ok() )
        p()->resource_gain( RESOURCE_CHI, p()->spec.expel_harm_2_ww->effectN( 1 ).base_value(), p()->gain.expel_harm );

    if ( p()->talent.brewmaster.tranquil_spirit->ok() )
    {
      // Reduce stagger damage
      auto amount_cleared = p()->active_actions.stagger_self_damage->clear_partial_damage_pct(
          p()->talent.brewmaster.tranquil_spirit->effectN( 1 ).percent() );
      p()->sample_datas.tranquil_spirit->add( amount_cleared );
      p()->proc.tranquil_spirit_expel_harm->occur();
    }
  }

  void impact( action_state_t* s ) override
  {
    double health_difference =
        p()->resources.max[ RESOURCE_HEALTH ] - std::max( p()->resources.current[ RESOURCE_HEALTH ], 0.0 );

    if ( p()->talent.general.strength_of_spirit->ok() )
    {
      double health_percent = health_difference / p()->resources.max[RESOURCE_HEALTH];
      s->result_total *= 1 + ( health_percent * p()->talent.general.strength_of_spirit->effectN( 1 ).percent() );
    }

    monk_heal_t::impact( s );

    double result = s->result_total;

    // Harm Denial only increases the healing, not the damage
    if ( p()->conduit.harm_denial->ok() )
      result /= 1 + p()->conduit.harm_denial.percent();

    result *= p()->spec.expel_harm->effectN( 2 ).percent();

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      // Have to manually set the combo strike mastery multiplier
      if ( p()->buff.combo_strikes->up() )
        result *= 1 + p()->cache.mastery_value();

      // Windwalker health difference will almost always be zero. So using the Expel Harm Effectiveness
      // option to simulate the amount of time that the results will use the full amount.
      if ( health_difference < result || !rng().roll( p()->user_options.expel_harm_effectiveness ) )
      {
        double min_amount = 1 / p()->spec.expel_harm->effectN( 2 ).percent();
        // Normally this would be using health_difference, but since Windwalkers will almost always be set
        // to zero, we want to use a range of 10 and the result to simulate varying amounts of health.
        result = rng().range( min_amount, result );
      }
    }

    if ( p()->specialization() == MONK_BREWMASTER )
    {
      if ( p()->buff.gift_of_the_ox->up() && p()->spec.expel_harm_2_brm->ok() )
      {
        double goto_heal = p()->passives.gift_of_the_ox_heal->effectN( 1 ).ap_coeff();
        goto_heal *= p()->buff.gift_of_the_ox->check();
        result += goto_heal;
      }

      for ( int i = 0; i < p()->buff.gift_of_the_ox->check(); i++ )
      {
        p()->buff.gift_of_the_ox->decrement();
      }
    }

    dmg->base_dd_min = result;
    dmg->base_dd_max = result;
    dmg->execute();
  }
};

// ==========================================================================
// Zen Pulse
// ==========================================================================

struct zen_pulse_heal_t : public monk_heal_t
{
  zen_pulse_heal_t( monk_t& p ) : monk_heal_t( "zen_pulse_heal", p, p.passives.zen_pulse_heal )
  {
    background = true;
    may_crit = may_miss = false;
    target              = &( p );
  }
};

struct zen_pulse_dmg_t : public monk_spell_t
{
  zen_pulse_heal_t* heal;
  zen_pulse_dmg_t( monk_t* player ) : monk_spell_t( "zen_pulse_damage", player, player->talent.mistweaver.zen_pulse )
  {
    background = true;
    aoe        = -1;

    heal = new zen_pulse_heal_t( *player );
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    heal->base_dd_min = s->result_amount;
    heal->base_dd_max = s->result_amount;
    heal->execute();
  }
};

struct zen_pulse_t : public monk_spell_t
{
  spell_t* damage;
  zen_pulse_t( monk_t* player ) : monk_spell_t( "zen_pulse", player, player->talent.mistweaver.zen_pulse )
  {
    may_miss = may_dodge = may_parry = false;
    damage                           = new zen_pulse_dmg_t( player );
  }

  void execute() override
  {
    monk_spell_t::execute();

    if ( result_is_miss( execute_state->result ) )
      return;

    damage->execute();
  }
};

// ==========================================================================
// Chi Wave
// ==========================================================================

struct chi_wave_heal_tick_t : public monk_heal_t
{
  chi_wave_heal_tick_t( monk_t& p, util::string_view name ) : monk_heal_t( name, p, p.passives.chi_wave_heal )
  {
    background = direct_tick = true;
    target                   = player;
  }
};

struct chi_wave_dmg_tick_t : public monk_spell_t
{
  chi_wave_dmg_tick_t( monk_t* player, util::string_view name )
    : monk_spell_t( name, player, player->passives.chi_wave_damage )
  {
    background              = true;
    ww_mastery              = true;
    attack_power_mod.direct = player->passives.chi_wave_damage->effectN( 1 ).ap_coeff();
    attack_power_mod.tick   = 0;
  }

  double action_multiplier() const override
  {
    double am = monk_spell_t::action_multiplier();

    return am;
  }
};

struct chi_wave_t : public monk_spell_t
{
  heal_t* heal;
  spell_t* damage;
  bool dmg;
  chi_wave_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "chi_wave", player, player->talent.general.chi_wave ),
      heal( new chi_wave_heal_tick_t( *player, "chi_wave_heal" ) ),
      damage( new chi_wave_dmg_tick_t( player, "chi_wave_damage" ) ),
      dmg( true )
  {
    sef_ability                 = sef_ability_e::SEF_CHI_WAVE;
    may_combo_strike            = true;
    may_proc_bron               = true;
    trigger_faeline_stomp       = true;
    trigger_bountiful_brew      = true;
    cast_during_sck             = true;
    parse_options( options_str );
    hasted_ticks = harmful = false;
    cooldown->hasted       = false;
    dot_duration           = timespan_t::from_seconds( player->talent.general.chi_wave->effectN( 1 ).base_value() );
    base_tick_time         = timespan_t::from_seconds( 1.0 );
    add_child( heal );
    add_child( damage );
    tick_zero = true;
    radius    = player->find_spell( 132466 )->effectN( 2 ).base_value();
    gcd_type  = gcd_haste_type::SPELL_HASTE;
  }

  void impact( action_state_t* s ) override
  {
    dmg = true;  // Set flag so that the first tick does damage

    monk_spell_t::impact( s );
  }

  void tick( dot_t* d ) override
  {
    monk_spell_t::tick( d );
    // Select appropriate tick action
    if ( dmg )
      damage->execute();
    else
      heal->execute();

    dmg = !dmg;  // Invert flag for next use
  }
};

// ==========================================================================
// Chi Burst
// ==========================================================================

struct chi_burst_heal_t : public monk_heal_t
{
  chi_burst_heal_t( monk_t& player ) : monk_heal_t( "chi_burst_heal", player, player.passives.chi_burst_heal )
  {
    background             = true;
    trigger_faeline_stomp  = true;
    trigger_bountiful_brew = true;
    target                 = p();
    // If we are using the user option, each heal just heals 1 target, otherwise use the old SimC code
    aoe = ( p()->user_options.chi_burst_healing_targets > 1 ? 1 : -1 );
    reduced_aoe_targets =
        ( p()->user_options.chi_burst_healing_targets > 1 ? 0.0 : p()->talent.general.chi_burst->effectN( 1 ).base_value() );
  }

  double action_multiplier() const override
  {
    double am = monk_heal_t::action_multiplier();

    // If we are using the Chi Burst Healing Target option we need to simulate the reduced healing beyond 6 targets
    double soft_cap = p()->talent.general.chi_burst->effectN( 1 ).base_value();
    if ( p()->user_options.chi_burst_healing_targets > soft_cap )
      am *= std::sqrt( soft_cap / p()->user_options.chi_burst_healing_targets );

    return am;
  }
};

struct chi_burst_damage_t : public monk_spell_t
{
  int num_hit;
  chi_burst_damage_t( monk_t& player )
    : monk_spell_t( "chi_burst_damage", &player, player.passives.chi_burst_damage ), num_hit( 0 )
  {
    background              = true;
    ww_mastery              = true;
    trigger_faeline_stomp   = true;
    trigger_bountiful_brew  = true;
    aoe                     = -1;
  }

  void execute() override
  {
    num_hit = 0;

    monk_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    monk_spell_t::impact( s );

    num_hit++;

    if ( p()->specialization() == MONK_WINDWALKER )
    {
      if ( num_hit <= p()->talent.general.chi_burst->effectN( 3 ).base_value() )
        p()->resource_gain( RESOURCE_CHI, p()->passives.chi_burst_energize->effectN( 1 ).base_value(),
                            p()->gain.chi_burst );
    }
  }
};

struct chi_burst_t : public monk_spell_t
{
  chi_burst_heal_t* heal;
  chi_burst_damage_t* damage;
  chi_burst_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "chi_burst", player, player->talent.general.chi_burst ),
      heal( new chi_burst_heal_t( *player ) ),
      damage( new chi_burst_damage_t( *player ) )
  {
    parse_options( options_str );
    may_combo_strike       = true;
    may_proc_bron          = true;
    trigger_faeline_stomp  = true;
    trigger_bountiful_brew = true;

    add_child( damage );
    add_child( heal );

    cooldown->hasted = false;

    attack_power_mod.direct = 0;
    weapon_power_mod        = 0;

    interrupt_auto_attack = false;
    // Forcing the minimum GCD to 750 milliseconds for all 3 specs
    min_gcd  = timespan_t::from_millis( 750 );
    gcd_type = gcd_haste_type::SPELL_HASTE;
  }

  bool ready() override
  {
    if ( p()->talent.general.chi_burst->ok() )
      return monk_spell_t::ready();

    return false;
  }

  void execute() override
  {
    // AoE healing is wonky in SimC. Try to simulate healing multiple players without over burdening sims
    if ( p()->user_options.chi_burst_healing_targets > 1 )
    {
      int healing_targets = p()->user_options.chi_burst_healing_targets;
      for ( int i = 0; i < healing_targets; i++ )
        heal->execute();
    }
    else
      heal->execute();
    damage->execute();

    monk_spell_t::execute();
  }
};

// ==========================================================================
// Healing Elixirs
// ==========================================================================

struct healing_elixir_t : public monk_heal_t
{
  healing_elixir_t( monk_t& p ) : monk_heal_t( "healing_elixir", p, p.shared.healing_elixir )
  {
    harmful = may_crit = false;
    target             = &p;
    base_pct_heal      = data().effectN( 1 ).percent();
  }
};

// ==========================================================================
// Refreshing Jade Wind
// ==========================================================================

struct refreshing_jade_wind_heal_t : public monk_heal_t
{
  refreshing_jade_wind_heal_t( monk_t& player )
    : monk_heal_t( "refreshing_jade_wind_heal", player, player.talent.mistweaver.refreshing_jade_wind->effectN( 1 ).trigger() )
  {
    background = true;
    aoe        = 6;
  }
};

struct refreshing_jade_wind_t : public monk_spell_t
{
  refreshing_jade_wind_heal_t* heal;
  refreshing_jade_wind_t( monk_t* player, util::string_view options_str )
    : monk_spell_t( "refreshing_jade_wind", player, player->talent.mistweaver.refreshing_jade_wind )
  {
    parse_options( options_str );
    heal = new refreshing_jade_wind_heal_t( *player );
  }

  void tick( dot_t* d ) override
  {
    monk_spell_t::tick( d );

    heal->execute();
  }
};

// ==========================================================================
// Celestial Fortune
// ==========================================================================
// This is a Brewmaster-specific critical strike effect

struct celestial_fortune_t : public monk_heal_t
{
  celestial_fortune_t( monk_t& p ) : monk_heal_t( "celestial_fortune", p, p.passives.celestial_fortune )
  {
    background = true;
    proc       = true;
    target     = player;
    may_crit   = false;

    base_multiplier = p.spec.celestial_fortune->effectN( 1 ).percent();
  }

  // Need to disable multipliers in init() so that it doesn't double-dip on anything
  void init() override
  {
    monk_heal_t::init();
    // disable the snapshot_flags for all multipliers, but specifically allow
    // action_multiplier() to be called so we can override.
    snapshot_flags &= STATE_NO_MULTIPLIER;
    snapshot_flags |= STATE_MUL_DA;
  }
};

// ==========================================================================
// Evasive Stride Conduit
// ==========================================================================

struct evasive_stride_t : public monk_heal_t
{
  evasive_stride_t( monk_t& p ) : monk_heal_t( "evasive_stride", p, p.passives.evasive_stride )
  {
    background = true;
    proc       = true;
    target     = player;
  }
};
}  // namespace heals

namespace absorbs
{
// ==========================================================================
// Celestial Brew
// ==========================================================================

struct special_delivery_t : public monk_spell_t
{
  special_delivery_t( monk_t& p ) : monk_spell_t( "special_delivery", &p, p.passives.special_delivery )
  {
    may_block = may_dodge = may_parry = true;
    background                        = true;
    trigger_gcd                       = timespan_t::zero();
    aoe                               = -1;
    attack_power_mod.direct           = data().effectN( 1 ).ap_coeff();
    radius                            = data().effectN( 1 ).radius();
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_seconds( p()->talent.brewmaster.special_delivery->effectN( 1 ).base_value() );
  }

  double cost() const override
  {
    return 0;
  }
};

struct celestial_brew_t : public monk_absorb_t
{
  struct celestial_brew_t_state_t : public action_state_t
  {
    celestial_brew_t_state_t( action_t* a, player_t* target ) : action_state_t( a, target )
    {
    }

    proc_types2 cast_proc_type2() const override
    {
      // Celestial Brew seems to trigger Bron's Call to Action (and possibly other
      // effects that care about casts).
      return PROC2_CAST_HEAL;
    }
  };

  special_delivery_t* delivery;

  celestial_brew_t( monk_t& p, util::string_view options_str )
    : monk_absorb_t( "celestial_brew", p, p.talent.brewmaster.celestial_brew ), delivery( new special_delivery_t( p ) )
  {
    parse_options( options_str );
    harmful = may_crit = false;
    may_proc_bron      = true;
    callbacks          = true;
    cast_during_sck    = true;

    if ( p.talent.brewmaster.light_brewing->ok() )
      cooldown->duration *= 1 + p.talent.brewmaster.light_brewing->effectN( 2 ).percent();  // -20
  }

  action_state_t* new_state() override
  {
    return new celestial_brew_t_state_t( this, player );
  }

  double action_multiplier() const override
  {
    double am = base_t::action_multiplier();

    if ( p()->buff.purified_chi->check() )
      am *= 1 + p()->buff.purified_chi->check_stack_value();

    return am;
  }

  void execute() override
  {
    monk_absorb_t::execute();

    if ( p()->buff.purified_chi->up() )
      p()->buff.purified_chi->expire();

    if ( p()->talent.brewmaster.pretense_of_instability->ok() )
      p()->buff.pretense_of_instability->trigger();

    if ( p()->talent.brewmaster.special_delivery->ok() )
    {
      delivery->set_target( target );
      delivery->execute();
    }

    if ( p()->talent.brewmaster.celestial_flames->ok() )
      p()->buff.celestial_flames->trigger();

    if ( p()->buff.blackout_combo->up() )
    {
      // Currently on Alpha, Blackout Combo Celestial Brew is overriding any current Purrifying Chi
      if ( p()->bugs && p()->buff.purified_chi->up() )
          p()->buff.purified_chi->expire();

      p()->buff.purified_chi->trigger( (int)p()->talent.brewmaster.blackout_combo->effectN( 6 ).base_value() );
      p()->proc.blackout_combo_celestial_brew->occur();
      p()->buff.blackout_combo->expire();
    }

    if ( p()->legendary.celestial_infusion->ok() )
      p()->buff.mighty_pour->trigger();
  }
};

// ==========================================================================
// Life Cocoon
// ==========================================================================
struct life_cocoon_t : public monk_absorb_t
{
  life_cocoon_t( monk_t& p, util::string_view options_str ) : monk_absorb_t( "life_cocoon", p, p.spec.life_cocoon )
  {
    parse_options( options_str );
    harmful = may_crit = false;

    base_dd_min = p.resources.max[ RESOURCE_HEALTH ] * p.spec.life_cocoon->effectN( 3 ).percent();
    base_dd_max = base_dd_min;
  }

  void impact( action_state_t* s ) override
  {
    p()->buff.life_cocoon->trigger( 1, s->result_amount );
    stats->add_result( 0.0, s->result_amount, result_amount_type::ABSORB, s->result, s->block_result, s->target );
  }
};
}  // namespace absorbs

using namespace pets;
using namespace pet_summon;
using namespace attacks;
using namespace spells;
using namespace heals;
using namespace absorbs;
}  // namespace actions

namespace buffs
{
// ==========================================================================
// Monk Buffs
// ==========================================================================

template <typename buff_t>
struct monk_buff_t : public buff_t
{
public:
  using base_t = monk_buff_t;

  monk_buff_t( monk_td_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
               const item_t* item = nullptr )
    : buff_t( p, name, s, item )
  {
  }

  monk_buff_t( monk_t& p, util::string_view name, const spell_data_t* s = spell_data_t::nil(),
               const item_t* item = nullptr )
    : buff_t( &p, name, s, item )
  {
  }

  monk_td_t& get_td( player_t* t )
  {
    return *( p().get_target_data( t ) );
  }

  const monk_td_t* find_td( player_t* t ) const
  {
    return p()->find_target_data( t );
  }

  monk_t& p()
  {
    return *debug_cast<monk_t*>( buff_t::source );
  }

  const monk_t& p() const
  {
    return *debug_cast<monk_t*>( buff_t::source );
  }
};

// ===============================================================================
// Fortifying Brew Buff
// ===============================================================================
struct fortifying_brew_t : public monk_buff_t<buff_t>
{
  int health_gain;
  fortifying_brew_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s ), health_gain( 0 )
  {
    cooldown->duration = timespan_t::zero();
    set_trigger_spell( p.talent.general.fortifying_brew );
    add_invalidate( CACHE_DODGE );
    add_invalidate( CACHE_ARMOR );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    double health_multiplier = ( p().bugs ? 0.2 : 0.15 );  // p().spec.fortifying_brew_mw_ww->effectN( 1 ).percent();

    if ( p().talent.brewmaster.fortifying_brew_determination->ok() )
    {
      // The tooltip is hard-coded with 20% if Brewmaster Rank 2 is activated
      // Currently it's bugged and giving 17.39% HP instead of the intended 20%
      // The intended calculation is:
      // health_multiplier = ( 1 + health_multiplier ) * p().passives.fortifying_brew->effectN( 5 ).percent() * ( 1 / (
      // 1 + health_multiplier ) );
      health_multiplier = ( p().bugs ? 0.1739 : 0.2);  // p().passives.fortifying_brew->effectN( 5 ).percent() * ( 1 / ( 1 + health_multiplier ) );
    }

    // Extra Health is set by current max_health, doesn't change when max_health changes.
    health_gain = static_cast<int>( p().resources.max[ RESOURCE_HEALTH ] * health_multiplier );

    p().stat_gain( STAT_MAX_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    p().stat_gain( STAT_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    p().stat_loss( STAT_MAX_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
    p().stat_loss( STAT_HEALTH, health_gain, (gain_t*)nullptr, (action_t*)nullptr, true );
  }
};

// ===============================================================================
// Serenity Buff
// ===============================================================================
struct serenity_buff_t : public monk_buff_t<buff_t>
{
  monk_t& m;
  serenity_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s ), m( p )
  {
    set_default_value_from_effect( 2 );
    set_cooldown( timespan_t::zero() );

    set_duration( s->duration() );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
    set_stack_change_callback( [ this ]( buff_t* /* b */, int /* old */, int new_ ) {
      double recharge_mult = 1.0 / ( 1 + m.talent.windwalker.serenity->effectN( 4 ).percent() );
      int label            = data().effectN( 4 ).misc_value1();
      for ( auto a : m.action_list )
      {
        if ( a->data().affected_by_label( label ) && a->cooldown->duration != 0_ms )
        {
          if ( new_ > 0 )
            a->dynamic_recharge_rate_multiplier *= recharge_mult;
          else
            a->dynamic_recharge_rate_multiplier /= recharge_mult;

          a->cooldown->adjust_recharge_multiplier();
        }
      }
    } );
  }
};

// ===============================================================================
// Touch of Karma Buff
// ===============================================================================
struct touch_of_karma_buff_t : public monk_buff_t<buff_t>
{
  touch_of_karma_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    default_value = 0;
    set_cooldown( timespan_t::zero() );
    set_trigger_spell( p.talent.windwalker.touch_of_karma );

    set_duration( s->duration() );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    // Make sure the value is reset upon each trigger
    current_value = 0;

    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Rushing Jade Wind Buff
// ===============================================================================
struct rushing_jade_wind_buff_t : public monk_buff_t<buff_t>
{
  // gonna assume this is 1 buff per monk combatant
  timespan_t _period;

  static void rjw_callback( buff_t* b, int, timespan_t )
  {
    auto* p = debug_cast<monk_t*>( b->player );

    p->active_actions.rushing_jade_wind->execute();
  }

  rushing_jade_wind_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_can_cancel( true );
    set_tick_zero( true );
    set_cooldown( timespan_t::zero() );
    set_trigger_spell( p.shared.rushing_jade_wind );

    set_period( s->effectN( 1 ).period() );
    set_tick_time_behavior( buff_tick_time_behavior::CUSTOM );
    set_tick_time_callback( [ & ]( const buff_t*, unsigned int ) { return _period; } );
    set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
    set_partial_tick( true );

    if ( p.specialization() == MONK_BREWMASTER )
      set_duration_multiplier( 1 + p.spec.brewmaster_monk->effectN( 9 ).percent() );

    set_tick_callback( rjw_callback );
    set_tick_behavior( buff_tick_behavior::REFRESH );
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    duration = ( duration >= timespan_t::zero() ? duration : this->buff_duration() ) * p().cache.spell_speed();
    // RJW snapshots the tick period on cast. this + the tick_time
    // callback represent that behavior
    _period = this->buff_period * p().cache.spell_speed();
    return buff_t::trigger( stacks, value, chance, duration );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Gift of the Ox Buff
// ===============================================================================
struct gift_of_the_ox_buff_t : public monk_buff_t<buff_t>
{
  gift_of_the_ox_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_can_cancel( false );
    set_cooldown( timespan_t::zero() );
    set_trigger_spell( p.talent.brewmaster.gift_of_the_ox );
    stack_behavior = buff_stack_behavior::ASYNCHRONOUS;

    set_refresh_behavior( buff_refresh_behavior::NONE );

    set_duration( p.find_spell( 124503 )->duration() );
    set_max_stack( 99 );
  }

  void decrement( int stacks, double value ) override
  {
    if ( stacks > 0 )
    {
      p().active_actions.gift_of_the_ox_trigger->execute();

      buff_t::decrement( stacks, value );
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    p().active_actions.gift_of_the_ox_expire->execute();

    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Xuen Rank 2 Empowered Tiger Lightning Buff
// ===============================================================================
struct invoke_xuen_the_white_tiger_buff_t : public monk_buff_t<buff_t>
{
  static void invoke_xuen_callback( buff_t* b, int, timespan_t )
  {
    auto* p = debug_cast<monk_t*>( b->player );
    if ( p->talent.windwalker.empowered_tiger_lightning->ok() )
    {
      double empowered_tiger_lightning_multiplier =
          p->talent.windwalker.empowered_tiger_lightning->effectN( 2 ).percent();

      for ( auto target : p->sim->target_non_sleeping_list )
      {
        if ( p->find_target_data( target ) )
        {
          auto td = p->get_target_data( target );
          if ( td->debuff.empowered_tiger_lightning->up() )
          {
            double value                                        = td->debuff.empowered_tiger_lightning->check_value();
            td->debuff.empowered_tiger_lightning->current_value = 0;
            if ( value > 0 )
            {
              p->active_actions.empowered_tiger_lightning->set_target( target );
              p->active_actions.empowered_tiger_lightning->base_dd_min = value * empowered_tiger_lightning_multiplier;
              p->active_actions.empowered_tiger_lightning->base_dd_max = value * empowered_tiger_lightning_multiplier;
              p->active_actions.empowered_tiger_lightning->execute();
            }
          }
        }
      }
    }
  }

  invoke_xuen_the_white_tiger_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_cooldown( timespan_t::zero() );
    set_duration( s->duration() );

    set_period( s->effectN( 2 ).period() );

    set_tick_callback( invoke_xuen_callback );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Call To Arm Invoke Xuen Buff
// ===============================================================================
struct call_to_arms_xuen_buff_t : public monk_buff_t<buff_t>
{
  static void call_to_arm_callback( buff_t* b, int, timespan_t )
  {
    auto* p = debug_cast<monk_t*>( b->player );
    if ( p->talent.windwalker.empowered_tiger_lightning->ok() )
    {
      double empowered_tiger_lightning_multiplier =
          p->talent.windwalker.empowered_tiger_lightning->effectN( 2 ).percent();

      for ( auto target : p->sim->target_non_sleeping_list )
      {
        if ( p->find_target_data( target ) )
        {
          auto td = p->get_target_data( target );
          if ( td->debuff.call_to_arms_empowered_tiger_lightning->up() )
          {
            double value = td->debuff.call_to_arms_empowered_tiger_lightning->check_value();
            td->debuff.call_to_arms_empowered_tiger_lightning->current_value = 0;
            if ( value > 0 )
            {
              p->active_actions.call_to_arms_empowered_tiger_lightning->set_target( target );
              p->active_actions.call_to_arms_empowered_tiger_lightning->base_dd_min =
                  value * empowered_tiger_lightning_multiplier;
              p->active_actions.call_to_arms_empowered_tiger_lightning->base_dd_max =
                  value * empowered_tiger_lightning_multiplier;
              p->active_actions.call_to_arms_empowered_tiger_lightning->execute();
            }
          }
        }
      }
    }
  }

  call_to_arms_xuen_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_cooldown( timespan_t::zero() );
    set_duration( p.passives.call_to_arms_invoke_xuen->duration() );
    set_trigger_spell( p.covenant.kyrian );

    set_period( timespan_t::from_seconds( p.talent.windwalker.empowered_tiger_lightning->effectN( 1 ).base_value() ) );

    set_tick_callback( call_to_arm_callback );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Fury of Xuen Stacking Buff
// ===============================================================================
struct fury_of_xuen_stacking_buff_t : public monk_buff_t<buff_t>
{
  fury_of_xuen_stacking_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    // Currently this is saved as 100, but we need to utilize it as a percent so (100 / 100) = 1 * 0.01 = 1%
    set_default_value( ( p.passives.fury_of_xuen_stacking_buff->effectN( 3 ).base_value() / 100 ) * 0.01 );
    set_trigger_spell( p.talent.windwalker.fury_of_xuen );
    set_cooldown( timespan_t::zero() );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    p().buff.fury_of_xuen_haste->trigger();
    p().pets.fury_of_xuen_tiger.spawn( p().passives.fury_of_xuen_haste_buff->duration(), 1 );
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Fury of Xuen Stacking Buff
// ===============================================================================
struct fury_of_xuen_haste_buff_t : public monk_buff_t<buff_t>
{
  static void fury_of_xuen_callback( buff_t* b, int, timespan_t )
  {
    auto* p = debug_cast<monk_t*>( b->player );
    if ( p->talent.windwalker.empowered_tiger_lightning->ok() )
    {
      double empowered_tiger_lightning_multiplier =
          p->talent.windwalker.empowered_tiger_lightning->effectN( 2 ).percent();

      for ( auto target : p->sim->target_non_sleeping_list )
      {
        if ( p->find_target_data( target ) )
        {
          auto td = p->get_target_data( target );
          if ( td->debuff.fury_of_xuen_empowered_tiger_lightning->up() )
          {
            double value = td->debuff.fury_of_xuen_empowered_tiger_lightning->check_value();
            td->debuff.fury_of_xuen_empowered_tiger_lightning->current_value = 0;
            if ( value > 0 )
            {
              p->active_actions.fury_of_xuen_empowered_tiger_lightning->set_target( target );
              p->active_actions.fury_of_xuen_empowered_tiger_lightning->base_dd_min =
                  value * empowered_tiger_lightning_multiplier;
              p->active_actions.fury_of_xuen_empowered_tiger_lightning->base_dd_max =
                  value * empowered_tiger_lightning_multiplier;
              p->active_actions.fury_of_xuen_empowered_tiger_lightning->execute();
            }
          }
        }
      }
    }
  }
  fury_of_xuen_haste_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_cooldown( timespan_t::zero() );
    set_default_value_from_effect( 1 );
    set_pct_buff_type( STAT_PCT_BUFF_HASTE );
    set_trigger_spell( p.talent.windwalker.fury_of_xuen );
    add_invalidate( CACHE_ATTACK_HASTE );
    add_invalidate( CACHE_RPPM_HASTE );
    add_invalidate( CACHE_HASTE );
    add_invalidate( CACHE_SPELL_HASTE );

    set_period( p.passives.fury_of_xuen_haste_buff->effectN( 3 ).period() );

    set_tick_callback( fury_of_xuen_callback );
  }
};

// ===============================================================================
// Niuzao Rank 2 Purifying Buff
// ===============================================================================
struct purifying_buff_t : public monk_buff_t<buff_t>
{
  std::deque<double> values;
  // tracking variable for debug code
  bool ignore_empty;
  purifying_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_trigger_spell( p.talent.brewmaster.purifying_brew );
    set_can_cancel( true );
    set_quiet( true );
    set_cooldown( timespan_t::zero() );
    set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS );

    set_refresh_behavior( buff_refresh_behavior::NONE );

    set_duration( timespan_t::from_seconds( 6 ) );
    set_max_stack( 99 );

    ignore_empty = false;
  }

  bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    ignore_empty = false;
    p().sim->print_debug( "adding recent purify (amount: {})", value );
    // Make sure the value is reset upon each trigger
    current_value = 0;

    values.push_back( value );

    return buff_t::trigger( stacks, value, chance, duration );
  }

  double value() override
  {
    double total_value = 0;

    if ( !values.empty() )
    {
      for ( auto& i : values )
        total_value += i;
    }

    return total_value;
  }

  void decrement( int stacks, double value ) override
  {
    if ( values.empty() )
    {
      // decrement ends up being called after expire_override sometimes. if this
      // debug msg is showing, then we have an error besides that that is
      // leading to stack/queue mismatches
      if ( !ignore_empty )
      {
        p().sim->print_debug( "purifying_buff decrement called with no values in queue!" );
      }
    }
    else
    {
      values.pop_front();
    }
    buff_t::decrement( stacks, value );
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    ignore_empty = true;
    values.clear();
    buff_t::expire_override( expiration_stacks, remaining_duration );
  }
};

// ===============================================================================
// Touch of Death Windwalker Buff
// ===============================================================================
// This buff is set up so that it provides the chi from
// Windwalker's Touch of Death Rank 2. In-game, applying Touch of Death will spawn
// three chi orbs that the player can pick up whenever they do not have max
// Chi. Given we want to provide the chi but apply it slowly if the player is at
// max chi, then we need to set up so that it triggers on it's own.

struct touch_of_death_ww_buff_t : public monk_buff_t<buff_t>
{
  touch_of_death_ww_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_can_cancel( false );
    set_quiet( true );
    set_reverse( true );
    set_cooldown( timespan_t::zero() );
    set_trigger_spell( p.spec.touch_of_death_3_ww );

    set_duration( timespan_t::from_minutes( 3 ) );
    set_period( timespan_t::from_seconds( 1 ) );
    set_tick_zero( true );

    set_max_stack( (int)p.find_spell( 325215 )->effectN( 1 ).base_value() );
    set_reverse_stack_count( 1 );
  }

  void decrement( int stacks, double value ) override
  {
    if ( stacks > 0 && player->resources.current[ RESOURCE_CHI ] < player->resources.max[ RESOURCE_CHI ] )
    {
      buff_t::decrement( stacks, value );

      player->resource_gain( RESOURCE_CHI, 1, p().gain.touch_of_death_ww );
    }
  }
};

// ===============================================================================
// Close to Heart Buff
// ===============================================================================
struct close_to_heart_driver_t : public monk_buff_t<buff_t>
{
  double leech_increase;
  close_to_heart_driver_t( monk_t& p, util::string_view n, const spell_data_t* s )
    : monk_buff_t( p, n, s ), leech_increase( 0 )
  {
    set_tick_callback( [ &p, this ]( buff_t*, int /* total_ticks */, timespan_t /* tick_time */ ) {
      range::for_each( p.close_to_heart_aura->target_list(), [ this ]( player_t* target ) {
        target->buffs.close_to_heart_leech_aura->trigger( 1, ( leech_increase ), 1, timespan_t::from_seconds( 10 ) );
      } );
    } );
    set_trigger_spell( p.talent.general.close_to_heart );
    set_cooldown( timespan_t::zero() );
    set_duration( timespan_t::zero() );
    set_period( timespan_t::from_seconds( 1 ) );
    set_tick_behavior( buff_tick_behavior::CLIP );
    leech_increase = p.talent.general.close_to_heart->effectN( 1 ).percent();
  }
};

// ===============================================================================
// Generous Pour Buff
// ===============================================================================
struct generous_pour_driver_t : public monk_buff_t<buff_t>
{
  double avoidance_increase;
  generous_pour_driver_t( monk_t& p, util::string_view n, const spell_data_t* s )
    : monk_buff_t( p, n, s ), avoidance_increase( 0 )
  {
    set_tick_callback( [ &p, this ]( buff_t*, int /* total_ticks */, timespan_t /* tick_time */ ) {
      range::for_each( p.generous_pour_aura->target_list(), [ this ]( player_t* target ) {
        target->buffs.generous_pour_avoidance_aura->trigger( 1, ( avoidance_increase ), 1, timespan_t::from_seconds( 10 ) );
      } );
    } );
    set_trigger_spell( p.talent.general.generous_pour );
    set_cooldown( timespan_t::zero() );
    set_duration( timespan_t::zero() );
    set_period( timespan_t::from_seconds( 1 ) );
    set_tick_behavior( buff_tick_behavior::CLIP );
    avoidance_increase = p.talent.general.generous_pour->effectN( 1 ).percent();
  }
};  

// ===============================================================================
// Windwalking Buff
// ===============================================================================
struct windwalking_driver_t : public monk_buff_t<buff_t>
{
  double movement_increase;
  windwalking_driver_t( monk_t& p, util::string_view n, const spell_data_t* s )
    : monk_buff_t( p, n, s ), movement_increase( 0 )
  {
    set_tick_callback( [ &p, this ]( buff_t*, int /* total_ticks */, timespan_t /* tick_time */ ) {
      range::for_each( p.windwalking_aura->target_list(), [ this ]( player_t* target ) {
        target->buffs.windwalking_movement_aura->trigger( 1, ( movement_increase ), 1, timespan_t::from_seconds( 10 ) );
      } );
    } );
    set_trigger_spell( p.talent.general.windwalking );
    set_cooldown( timespan_t::zero() );
    set_duration( timespan_t::zero() );
    set_period( timespan_t::from_seconds( 1 ) );
    set_tick_behavior( buff_tick_behavior::CLIP );
    movement_increase = p.talent.general.windwalking->effectN( 1 ).percent();
  }
};

// ===============================================================================
// Stagger Buff
// ===============================================================================
struct stagger_buff_t : public monk_buff_t<buff_t>
{
  stagger_buff_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    timespan_t stagger_duration = s->duration();
    stagger_duration += timespan_t::from_seconds( p.talent.brewmaster.bob_and_weave->effectN( 1 ).base_value() / 10 );

    // set_duration(stagger_duration);
    set_duration( timespan_t::zero() );
    set_trigger_spell( p.talent.brewmaster.stagger );
    if ( p.talent.brewmaster.high_tolerance->ok() )
    {
      add_invalidate( CACHE_HASTE );
    }
  }
};

// ===============================================================================
// Hidden Master's Forbidden Touch Legendary
// ===============================================================================

struct hidden_masters_forbidden_touch_t : public monk_buff_t<buff_t>
{
  hidden_masters_forbidden_touch_t( monk_t& p, util::string_view n, const spell_data_t* s ) : monk_buff_t( p, n, s )
  {
    set_trigger_spell( p.talent.windwalker.forbidden_technique );
  }
  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );
    cooldown_t* touch_of_death = source->get_cooldown( "touch_of_death" );
    if ( touch_of_death->up() )
      touch_of_death->start();
  }
};
}  // namespace buffs

namespace items
{
// MONK MODULE INTERFACE ====================================================

void do_trinket_init( monk_t* player, specialization_e spec, const special_effect_t*& ptr,
                      const special_effect_t& effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( !player->find_spell( effect.spell_id )->ok() || player->specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

void init()
{
}
}  // namespace items

}  // end namespace monk

namespace monk
{
// ==========================================================================
// Monk Character Definition
// ==========================================================================

// Debuffs ==================================================================
monk_td_t::monk_td_t( player_t* target, monk_t* p ) : actor_target_data_t( target, p ), dots(), debuff(), monk( *p )
{
  if ( p->specialization() == MONK_WINDWALKER )
  {
    debuff.flying_serpent_kick = make_buff( *this, "flying_serpent_kick", p->passives.flying_serpent_kick_damage )
                                     ->set_default_value_from_effect( 2 );
    debuff.empowered_tiger_lightning = make_buff( *this, "empowered_tiger_lightning", spell_data_t::nil() )
                                           ->set_quiet( true )
                                           ->set_cooldown( timespan_t::zero() )
                                           ->set_refresh_behavior( buff_refresh_behavior::NONE )
                                           ->set_max_stack( 1 )
                                           ->set_default_value( 0 );
    debuff.fury_of_xuen_empowered_tiger_lightning = make_buff( *this, "empowered_tiger_lightning_fury_of_xuen", spell_data_t::nil() )
                                                        ->set_quiet( true )
                                                        ->set_cooldown( timespan_t::zero() )
                                                        ->set_refresh_behavior( buff_refresh_behavior::NONE )
                                                        ->set_max_stack( 1 )
                                                        ->set_default_value( 0 );

    debuff.mark_of_the_crane = make_buff( *this, "mark_of_the_crane", p->passives.mark_of_the_crane )
                                   ->set_default_value( p->passives.cyclone_strikes->effectN( 1 ).percent() )
                                   ->set_refresh_behavior( buff_refresh_behavior::DURATION );
    debuff.touch_of_karma = make_buff( *this, "touch_of_karma_debuff", p->talent.windwalker.touch_of_karma )
                                // set the percent of the max hp as the default value.
                                ->set_default_value_from_effect( 3 );
  }

  if ( p->specialization() == MONK_BREWMASTER )
  {
    debuff.keg_smash = make_buff( *this, "keg_smash", p->talent.brewmaster.keg_smash )
                           ->set_cooldown( timespan_t::zero() )
                           ->set_default_value_from_effect( 3 );
  }

  // Covenant Abilities
  debuff.bonedust_brew = make_buff( *this, "bonedust_brew_debuff", p->find_spell( 325216 ) )
                             ->set_chance( 1 )
                             ->set_cooldown( timespan_t::zero() )
                             ->set_default_value_from_effect( 3 );

  debuff.faeline_stomp = make_buff( *this, "faeline_stomp_debuff", p->find_spell( 327257 ) );

  debuff.faeline_stomp_brm = make_buff( *this, "faeline_stomp_stagger_debuff", p->passives.faeline_stomp_brm )
                                 ->set_default_value_from_effect( 1 );

  debuff.fallen_monk_keg_smash = make_buff( *this, "fallen_monk_keg_smash", p->passives.fallen_monk_keg_smash )
                                     ->set_default_value_from_effect( 3 );

  debuff.weapons_of_order =
      make_buff( *this, "weapons_of_order_debuff", p->find_spell( 312106 ) )->set_default_value_from_effect( 1 );

  // Shadowland Legendary
  debuff.call_to_arms_empowered_tiger_lightning =
      make_buff( *this, "empowered_tiger_lightning_call_to_arms", spell_data_t::nil() )
          ->set_quiet( true )
          ->set_cooldown( timespan_t::zero() )
          ->set_refresh_behavior( buff_refresh_behavior::NONE )
          ->set_max_stack( 1 )
          ->set_default_value( 0 );
  debuff.fae_exposure = make_buff( *this, "fae_exposure_damage", p->passives.fae_exposure_dmg )
                            ->set_default_value_from_effect( 1 )
                            ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                            ->add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
  debuff.keefers_skyreach = make_buff( *this, "keefers_skyreach", p->passives.keefers_skyreach_debuff )
                                ->set_default_value_from_effect( 1 )
                                ->add_invalidate( CACHE_ATTACK_CRIT_CHANCE )
                                ->set_refresh_behavior( buff_refresh_behavior::NONE );
  debuff.sinister_teaching_fallen_monk_keg_smash =
      make_buff( *this, "sinister_teaching_fallen_monk_keg_smash", p->passives.fallen_monk_keg_smash )
          ->set_default_value_from_effect( 3 );
  debuff.skyreach_exhaustion = make_buff( *this, "skyreach_exhaustion", p->find_spell( 337341 ) )
                                   ->set_refresh_behavior( buff_refresh_behavior::NONE );

  debuff.storm_earth_and_fire = make_buff( *this, "storm_earth_and_fire_target" )->set_cooldown( timespan_t::zero() );

  dots.breath_of_fire          = target->get_dot( "breath_of_fire_dot", p );
  dots.enveloping_mist         = target->get_dot( "enveloping_mist", p );
  dots.eye_of_the_tiger_damage = target->get_dot( "eye_of_the_tiger_damage", p );
  dots.eye_of_the_tiger_heal   = target->get_dot( "eye_of_the_tiger_heal", p );
  dots.renewing_mist           = target->get_dot( "renewing_mist", p );
  dots.rushing_jade_wind       = target->get_dot( "rushing_jade_wind", p );
  dots.soothing_mist           = target->get_dot( "soothing_mist", p );
  dots.touch_of_karma          = target->get_dot( "touch_of_karma", p );
}

monk_t::monk_t( sim_t* sim, util::string_view name, race_e r )
  : player_t( sim, MONK, name, r ),
    sample_datas( sample_data_t() ),
    active_actions(),
    passive_actions(),
    spiritual_focus_count( 0 ),
    shuffle_count_secs( 0 ),
    gift_of_the_ox_proc_chance(),
    buff(),
    gain(),
    proc(),
    talent(),
    spec(),
    mastery(),
    cooldown(),
    passives(),
    rppm(),
    covenant(),
    conduit(),
    legendary(),
    pets( this ),
    user_options( options_t() ),
    light_stagger_threshold( 0 ),
    moderate_stagger_threshold( 0.01666 ),  // Moderate transfers at 33.3% Stagger; 1.67% every 1/2 sec
    heavy_stagger_threshold( 0.03333 )      // Heavy transfers at 66.6% Stagger; 3.34% every 1/2 sec
{
  // actives
  close_to_heart_aura = nullptr;
  generous_pour_aura  = nullptr;
  windwalking_aura = nullptr;

  cooldown.anvil_and_stave         = get_cooldown( "anvil_and_stave" );
  cooldown.blackout_kick           = get_cooldown( "blackout_kick" );
  cooldown.black_ox_brew           = get_cooldown( "black_ox_brew" );
  cooldown.brewmaster_attack       = get_cooldown( "brewmaster_attack" );
  cooldown.breath_of_fire          = get_cooldown( "breath_of_fire" );
  cooldown.celestial_brew          = get_cooldown( "celestial_brew" );
  cooldown.chi_torpedo             = get_cooldown( "chi_torpedo" );
  cooldown.counterstrike           = get_cooldown( "counterstrike" );
  cooldown.drinking_horn_cover     = get_cooldown( "drinking_horn_cover" );
  cooldown.expel_harm              = get_cooldown( "expel_harm" );
  cooldown.fortifying_brew         = get_cooldown( "fortifying_brew" );
  cooldown.fists_of_fury           = get_cooldown( "fists_of_fury" );
  cooldown.fury_of_xuen            = get_cooldown( "fury_of_xuen" );      
  cooldown.healing_elixir          = get_cooldown( "healing_elixir" );
  cooldown.invoke_niuzao           = get_cooldown( "invoke_niuzao_the_black_ox" );
  cooldown.invoke_xuen             = get_cooldown( "invoke_xuen_the_white_tiger" );
  cooldown.keg_smash               = get_cooldown( "keg_smash" );
  cooldown.purifying_brew          = get_cooldown( "purifying_brew" );
  cooldown.resonant_fists          = get_cooldown( "resonant_fists" );
  cooldown.rising_sun_kick         = get_cooldown( "rising_sun_kick" );
  cooldown.refreshing_jade_wind    = get_cooldown( "refreshing_jade_wind" );
  cooldown.roll                    = get_cooldown( "roll" );
  cooldown.rushing_jade_wind_brm   = get_cooldown( "rushing_jade_wind" );
  cooldown.rushing_jade_wind_ww    = get_cooldown( "rushing_jade_wind" );
  cooldown.storm_earth_and_fire    = get_cooldown( "storm_earth_and_fire" );
  cooldown.thunder_focus_tea       = get_cooldown( "thunder_focus_tea" );
  cooldown.touch_of_death          = get_cooldown( "touch_of_death" );
  cooldown.serenity                = get_cooldown( "serenity" );

  // Covenants
  cooldown.weapons_of_order = get_cooldown( "weapons_of_order" );
  cooldown.bonedust_brew    = get_cooldown( "bonedust_brew" );
  cooldown.faeline_stomp    = get_cooldown( "faeline_stomp" );
  cooldown.fallen_order     = get_cooldown( "fallen_order" );

  // Legendary
  cooldown.charred_passions   = get_cooldown( "charred_passions" );
  cooldown.bountiful_brew     = get_cooldown( "bountiful_brew" );
  cooldown.sinister_teachings = get_cooldown( "sinister_teachings" );

  // T29 Set Bonus
  cooldown.brewmasters_rhythm = get_cooldown( "brewmasters_rhythm" );

  resource_regeneration = regen_type::DYNAMIC;
  if ( specialization() != MONK_MISTWEAVER )
  {
    regen_caches[ CACHE_HASTE ]        = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }
  user_options.initial_chi               = 1;
  user_options.expel_harm_effectiveness  = 0.25;
  user_options.faeline_stomp_uptime      = 1.0;
  user_options.chi_burst_healing_targets = 8;
  user_options.motc_override             = 0;
  user_options.no_bof_dot                = 0;
}

// monk_t::create_action ====================================================

action_t* monk_t::create_action( util::string_view name, util::string_view options_str )
{
  using namespace actions;
  // General
  if ( name == "snapshot_stats" )
    return new monk_snapshot_stats_t( this, options_str );
  if ( name == "auto_attack" )
    return new auto_attack_t( this, options_str );
  if ( name == "crackling_jade_lightning" )
    return new crackling_jade_lightning_t( *this, options_str );
  if ( name == "tiger_palm" )
    return new tiger_palm_t( this, options_str );
  if ( name == "blackout_kick" )
    return new blackout_kick_t( this, options_str );
  if ( name == "expel_harm" )
    return new expel_harm_t( *this, options_str );
  if ( name == "leg_sweep" )
    return new leg_sweep_t( this, options_str );
  if ( name == "paralysis" )
    return new paralysis_t( this, options_str );
  if ( name == "rising_sun_kick" )
    return new rising_sun_kick_t( this, options_str );
  if ( name == "roll" )
    return new roll_t( this, options_str );
  if ( name == "spear_hand_strike" )
    return new spear_hand_strike_t( this, options_str );
  if ( name == "spinning_crane_kick" )
    return new spinning_crane_kick_t( this, options_str );
  if ( name == "summon_white_tiger_statue" ) 
      return new summon_white_tiger_statue_spell_t( this, options_str );
  if ( name == "vivify" )
    return new vivify_t( *this, options_str );

  // Brewmaster
  if ( name == "breath_of_fire" )
    return new breath_of_fire_t( *this, options_str );
  if ( name == "celestial_brew" )
    return new celestial_brew_t( *this, options_str );
  if ( name == "exploding_keg" )
    return new exploding_keg_t( *this, options_str );
  if ( name == "fortifying_brew" )
    return new fortifying_brew_t( *this, options_str );
  if ( name == "gift_of_the_ox" )
    return new gift_of_the_ox_t( *this, options_str );
  if ( name == "invoke_niuzao" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "invoke_niuzao_the_black_ox" )
    return new niuzao_spell_t( this, options_str );
  if ( name == "keg_smash" )
    return new keg_smash_t( *this, options_str );
  if ( name == "purifying_brew" )
    return new purifying_brew_t( *this, options_str );
  if ( name == "provoke" )
    return new provoke_t( this, options_str );

  // Mistweaver
  if ( name == "enveloping_mist" )
    return new enveloping_mist_t( *this, options_str );
  if ( name == "essence_font" )
    return new essence_font_t( this, options_str );
  if ( name == "invoke_chiji" )
    return new chiji_spell_t( this, options_str );
  if ( name == "invoke_chiji_the_red_crane" )
    return new chiji_spell_t( this, options_str );
  if ( name == "invoke_yulon" )
    return new yulon_spell_t( this, options_str );
  if ( name == "invoke_yulon_the_jade_serpent" )
    return new yulon_spell_t( this, options_str );
  if ( name == "life_cocoon" )
    return new life_cocoon_t( *this, options_str );
  if ( name == "mana_tea" )
    return new mana_tea_t( *this, options_str );
  if ( name == "renewing_mist" )
    return new renewing_mist_t( *this, options_str );
  if ( name == "revival" )
    return new revival_t( *this, options_str );
  if ( name == "thunder_focus_tea" )
    return new thunder_focus_tea_t( *this, options_str );

  // Windwalker
  if ( name == "fists_of_fury" )
    return new fists_of_fury_t( this, options_str );
  if ( name == "fists_of_fury_cancel" )
    return new fists_of_fury_t( this, options_str, true );
  if ( name == "flying_serpent_kick" )
    return new flying_serpent_kick_t( this, options_str );
  if ( name == "touch_of_karma" )
    return new touch_of_karma_t( this, options_str );
  if ( name == "touch_of_death" )
    return new touch_of_death_t( *this, options_str );
  if ( name == "storm_earth_and_fire" )
    return new storm_earth_and_fire_t( this, options_str );
  if ( name == "storm_earth_and_fire_fixate" )
    return new storm_earth_and_fire_fixate_t( this, options_str );

  // Talents
  if ( name == "chi_burst" )
    return new chi_burst_t( this, options_str );
  if ( name == "chi_torpedo" )
    return new chi_torpedo_t( this, options_str );
  if ( name == "chi_wave" )
    return new chi_wave_t( this, options_str );
  if ( name == "black_ox_brew" )
    return new black_ox_brew_t( *this, options_str );
  if ( name == "dampen_harm" )
    return new dampen_harm_t( *this, options_str );
  if ( name == "diffuse_magic" )
    return new diffuse_magic_t( *this, options_str );
  if ( name == "strike_of_the_windlord" )
    return new strike_of_the_windlord_t( this, options_str );
  if ( name == "invoke_xuen" )
    return new xuen_spell_t( this, options_str );
  if ( name == "invoke_xuen_the_white_tiger" )
    return new xuen_spell_t( this, options_str );
  if ( name == "refreshing_jade_wind" )
    return new refreshing_jade_wind_t( this, options_str );
  if ( name == "rushing_jade_wind" )
    return new rushing_jade_wind_t( this, options_str );
  if ( name == "whirling_dragon_punch" )
    return new whirling_dragon_punch_t( this, options_str );
  if ( name == "serenity" )
    return new serenity_t( this, options_str );

  // Covenant Abilities
  if ( name == "bonedust_brew" )
    return new bonedust_brew_t( *this, options_str );
  if ( name == "faeline_stomp" )
    return new faeline_stomp_t( *this, options_str );
  if ( name == "fallen_order" )
    return new fallen_order_t( *this, options_str );
  if ( name == "weapons_of_order" )
    return new weapons_of_order_t( *this, options_str );

  return base_t::create_action( name, options_str );
}

void monk_t::trigger_celestial_fortune( action_state_t* s )
{
  if ( !spec.celestial_fortune->ok() || s->action == active_actions.celestial_fortune || s->result_raw == 0.0 )
  {
    return;
  }

  // flush out percent heals
  if ( s->action->type == ACTION_HEAL )
  {
    auto* heal_cast = debug_cast<heal_t*>( s->action );
    if ( ( s->result_type == result_amount_type::HEAL_DIRECT && heal_cast->base_pct_heal > 0 ) ||
         ( s->result_type == result_amount_type::HEAL_OVER_TIME && heal_cast->tick_pct_heal > 0 ) )
      return;
  }

  // Attempt to proc the heal
  if ( active_actions.celestial_fortune && rng().roll( composite_melee_crit_chance() ) )
  {
    active_actions.celestial_fortune->base_dd_max = active_actions.celestial_fortune->base_dd_min = s->result_amount;
    active_actions.celestial_fortune->schedule_execute();
  }
}

void monk_t::trigger_mark_of_the_crane( action_state_t* s )
{
  if ( get_target_data( s->target )->debuff.mark_of_the_crane->up() ||
       mark_of_the_crane_counter() < as<int>( passives.cyclone_strikes->max_stacks() ) )
    get_target_data( s->target )->debuff.mark_of_the_crane->trigger();
}

void monk_t::trigger_keefers_skyreach( action_state_t* s )
{
  if ( shared.skyreach && shared.skyreach->ok() )
  {
    if ( !get_target_data( s->target )->debuff.skyreach_exhaustion->up() )
    {
      get_target_data( s->target )->debuff.keefers_skyreach->trigger();
      get_target_data( s->target )->debuff.skyreach_exhaustion->trigger();
    }
  }
}

player_t* monk_t::next_mark_of_the_crane_target( action_state_t* state )
{
  std::vector<player_t*> targets = state->action->target_list();
  if ( targets.empty() )
  {
    return nullptr;
  }
  if ( targets.size() > 1 )
  {
    // Have the SEF converge onto the the cleave target if there are only 2 targets
    if ( targets.size() == 2 )
      return targets[ 1 ];
    // Don't move the SEF if there is only 3 targets
    if ( targets.size() == 3 )
      return state->target;

    // First of all find targets that do not have the cyclone strike debuff applied and send the SEF to those targets
    for ( player_t* target : targets )
    {
      if ( !get_target_data( target )->debuff.mark_of_the_crane->check() &&
           !get_target_data( target )->debuff.storm_earth_and_fire->check() )
      {
        // remove the current target as having an SEF on it
        get_target_data( state->target )->debuff.storm_earth_and_fire->expire();
        // make the new target show that a SEF is on the target
        get_target_data( target )->debuff.storm_earth_and_fire->trigger();
        return target;
      }
    }

    // If all targets have the debuff, find the lowest duration of cyclone strike debuff as well as not have a SEF
    // debuff (indicating that an SEF is not already on the target and send the SEF to that new target.
    player_t* lowest_duration = targets[ 0 ];

    // They should never attack the player target
    for ( player_t* target : targets )
    {
      if ( !get_target_data( target )->debuff.storm_earth_and_fire->check() )
      {
        if ( get_target_data( target )->debuff.mark_of_the_crane->remains() <
             get_target_data( lowest_duration )->debuff.mark_of_the_crane->remains() )
          lowest_duration = target;
      }
    }
    // remove the current target as having an SEF on it
    get_target_data( state->target )->debuff.storm_earth_and_fire->expire();
    // make the new target show that a SEF is on the target
    get_target_data( lowest_duration )->debuff.storm_earth_and_fire->trigger();
    return lowest_duration;
  }
  // otherwise, target the same as the player
  return targets.front();
}

int monk_t::mark_of_the_crane_counter()
{
  if ( specialization() != MONK_WINDWALKER )
    return 0;

  if ( !talent.windwalker.mark_of_the_crane->ok() )
    return 0;

  if ( user_options.motc_override > 0 )
    return user_options.motc_override;

  int count = 0;

  for ( player_t* target : sim->target_non_sleeping_list.data() )
  {
    if ( target_data[ target ] && target_data[ target ]->debuff.mark_of_the_crane->check() )
      count++;

    if ( count == (int)passives.cyclone_strikes->max_stacks() )
      break;
  }

  return count;
}

// Currently at maximum stacks for target count
bool monk_t::mark_of_the_crane_max()
{
  if ( !talent.windwalker.mark_of_the_crane->ok() )
    return true;

  int count = mark_of_the_crane_counter();
  int targets = (int)sim->target_non_sleeping_list.data().size();

  if ( count == 0 || ( ( targets - count ) > 0 && count < (int)passives.cyclone_strikes->max_stacks() ) )
    return false;

  return true;
}

// Current contributions to SCK ( not including DoCJ bonus )
double monk_t::sck_modifier()
{
    double current = 1;

    if ( specialization() == MONK_WINDWALKER )
    {
      double motc_multiplier = passives.cyclone_strikes->effectN( 1 ).percent();

      if ( conduit.calculated_strikes->ok() )
        motc_multiplier += conduit.calculated_strikes.percent();
      /*else if ( talent.windwalker.calculated_strikes->ok() )
        motc_multiplier += talent.windwalker.calculated_strikes->effectN( 1 ).percent();
      */

      if ( spec.spinning_crane_kick_2_ww->ok() )
        current *= 1 + ( mark_of_the_crane_counter() * motc_multiplier );

      if ( talent.windwalker.crane_vortex->ok() )
        current *= 1 + talent.windwalker.crane_vortex->effectN( 1 ).percent();

      if ( buff.kicks_of_flowing_momentum->check() )
        current *= 1 + buff.kicks_of_flowing_momentum->check_value();
    }
    else if ( specialization() == MONK_BREWMASTER )
    {
      if ( buff.counterstrike->check() )
        current *= 1 + buff.counterstrike->data().effectN( 2 ).percent();
    }

    if ( talent.general.fast_feet->ok() )
      current *= 1 + talent.general.fast_feet->effectN( 2 ).percent();

    return current;
}

// monk_t::activate =========================================================

void monk_t::activate()
{
  player_t::activate();

  if ( specialization() == MONK_WINDWALKER && find_action( "storm_earth_and_fire" ) )
  {
    sim->target_non_sleeping_list.register_callback( sef_despawn_cb_t( this ) );
  }
}

void monk_t::collect_resource_timeline_information()
{
  base_t::collect_resource_timeline_information();

  sample_datas.stagger_damage_pct_timeline.add( sim->current_time(), current_stagger_amount_remains_percent() * 100.0 );

  auto stagger_pct_val = stagger_pct( target->level() );
  sample_datas.stagger_pct_timeline.add( sim->current_time(), stagger_pct_val * 100.0 );
}

// monk_t::init_spells ======================================================

void monk_t::init_spells()
{
  base_t::init_spells();

  auto spec_tree = specialization();

  // Talents spells =====================================
  auto _CT = [this]( util::string_view name ) {
    return find_talent_spell( talent_tree::CLASS, name );
  };

  auto _CTID = [this]( int id ) {
    return find_talent_spell( talent_tree::CLASS, id );
  };

  auto _ST = [ this ]( util::string_view name ) { 
    return find_talent_spell( talent_tree::SPECIALIZATION, name ); 
  };

  auto _STID = [ this ]( int id ) { 
    return find_talent_spell( talent_tree::SPECIALIZATION, id );
  };
  
  // ========
  // General
  // ========
  // Row 1
  talent.general.soothing_mist                              = _CT( "Soothing Mist" );
  talent.general.rising_sun_kick                            = _CT( "Rising Sun Kick" );
  talent.general.tigers_lust                                = _CT( "Tiger's Lust" );
  // Row 2
  talent.general.improved_roll                              = _CT( "Improved Roll" );
  talent.general.calming_presence                           = _CT( "Calming Presence" );
  talent.general.disable                                    = _CT( "Disable" );
  // Row 3
  talent.general.tiger_tail_sweep                           = _CT( "Tiger Tail Sweep" );
  talent.general.vigorous_expulsion                         = _CT( "Vigorous Expulsion" );
  talent.general.improved_vivify                            = _CT( "Improved Vivify" );
  talent.general.detox                                      = _CT( "Detox" );
  talent.general.paralysis                                  = _CT( "Paralysis" );
  // Row 4
  talent.general.grace_of_the_crane                         = _CT( "Grace of the Crane" );
  talent.general.vivacious_vivification                     = _CT( "Vivacious Vivification" );
  talent.general.ferocity_of_xuen                           = _CT( "Ferocity of Xuen" );
  talent.general.improved_paralysis                         = _CT( "Improved Paralysis" );
  // Row 5
  talent.general.elusive_mists                              = _CT( "Elusive Mists" );
  talent.general.transcendence                              = _CT( "Transcendence" );
  talent.general.spear_hand_strike                          = _CT( "Spear Hand Strike" );
  talent.general.fortifying_brew                            = is_ptr() ? _CT( "Fortifying Brew" ) : _CTID( 388917 );
  // Row 6
  talent.general.chi_wave                                   = _CT( "Chi Wave" );
  talent.general.chi_burst                                  = _CT( "Chi Burst" );
  talent.general.hasty_provocation                          = _CT( "Hasty Provocation" );
  talent.general.ring_of_peace                              = _CT( "Ring of Peace" );
  talent.general.fast_feet                                  = _CT( "Fast Feet" );
  talent.general.celerity                                   = _CT( "Celerity" );
  talent.general.chi_torpedo                                = _CT( "Chi Torpedo" );
  talent.general.ironshell_brew                             = _CT( "Ironshell Brew" );
  talent.general.expeditious_fortification                  = _CT( "Expeditious Fortification" );
  // Row 7
  talent.general.profound_rebuttal                          = _CT( "Profound Rebuttal" );
  talent.general.diffuse_magic                              = _CT( "Diffuse Magic" );
  talent.general.eye_of_the_tiger                           = _CT( "Eye of the Tiger" );
  talent.general.dampen_harm                                = _CT( "Dampen Harm" );
  talent.general.improved_touch_of_death                    = _CT( "Improved Touch of Death" );
  talent.general.strength_of_spirit                         = _CT( "Strength of Spirit" );
  // Row 8
  talent.general.close_to_heart                             = _CT( "Close to Heart" );
  talent.general.escape_from_reality                        = _CT( "Escape from Reality" );
  talent.general.windwalking                                = _CT( "Windwalking" );
  talent.general.fatal_touch                                = _CT( "Fatal Touch" );
  talent.general.generous_pour                              = _CT( "Generous Pour" );
  // Row 9
  talent.general.save_them_all                              = _CT( "Save Them All" );
  talent.general.resonant_fists                             = _CT( "Resonant Fists" );
  talent.general.bounce_back                                = _CT( "Bounce Back" );
  // Row 10
  talent.general.summon_jade_serpent_statue                 = _CT( "Summon Jade Serpent Statue" );
  talent.general.summon_white_tiger_statue                  = _CT( "Summon White Tiger Statue" );
  talent.general.summon_black_ox_statue                     = _CT( "Summon Black Ox Statue" );

  // ========
  // Brewmaster
  // ========
  
  if ( spec_tree == MONK_BREWMASTER )
  {
      // Row 1
      talent.brewmaster.keg_smash                           = _ST( "Keg Smash" );
      // Row 2
      talent.brewmaster.stagger                             = _ST( "Stagger" );
      // Row 3
      talent.brewmaster.purifying_brew                      = _ST( "Purifying Brew" );
      talent.brewmaster.shuffle                             = _ST( "Shuffle" );
      // Row 4
      talent.brewmaster.hit_scheme                          = _ST( "Hit Scheme" );
      talent.brewmaster.gift_of_the_ox                      = _ST( "Gift of the Ox" );
      talent.brewmaster.healing_elixir                      = _ST( "Healing Elixir" );
      talent.brewmaster.quick_sip                           = _ST( "Quick Sip" );
      talent.brewmaster.rushing_jade_wind                   = _ST( "Rushing Jade Wind" );
      talent.brewmaster.special_delivery                    = _ST( "Special Delivery" );
      // Row 5
      talent.brewmaster.celestial_flames                    = _ST( "Celestial Flames" );
      talent.brewmaster.celestial_brew                      = _ST( "Celestial Brew" );
      talent.brewmaster.staggering_strikes                  = _ST( "Staggering Strikes" );
      talent.brewmaster.graceful_exit                       = _ST( "Graceful Exit" );
      talent.brewmaster.zen_meditation                      = _ST( "Zen Meditation" );
      talent.brewmaster.clash                               = _ST( "Clash" );
      // Row 6
      talent.brewmaster.breath_of_fire                      = _ST( "Breath of Fire" );
      talent.brewmaster.improved_celestial_brew             = _ST( "Improved Celestial Brew" );
      talent.brewmaster.improved_purifying_brew             = _ST( "Improved Purifying Brew" );
      talent.brewmaster.tranquil_spirit                     = _ST( "Tranquil Spirit" );
      talent.brewmaster.gai_plins_imperial_brew             = _ST( "Gai Plin's Imperial Brew" );
      talent.brewmaster.fundamental_observation             = _ST( "Fundamental Observation" );
      talent.brewmaster.shadowboxing_treads                 = _STID( 387638 );
      talent.brewmaster.fluidity_of_motion                  = _ST( "Fluidity of Motion" );
      // Row 7
      talent.brewmaster.scalding_brew                       = _ST( "Scalding Brew" );
      talent.brewmaster.salsalabims_strength                = _ST( "Sal'salabim's Strength" );
      talent.brewmaster.fortifying_brew_determination       = is_ptr() ? _ST( "Fortifying Brew: Determination" ) : _STID( 322960 );
      talent.brewmaster.black_ox_brew                       = _ST( "Black Ox Brew" );
      talent.brewmaster.bob_and_weave                       = _ST( "Bob and Weave" );
      talent.brewmaster.invoke_niuzao_the_black_ox          = is_ptr() ? _ST( "Invoke Niuzao, the Black Ox" ) : _STID( 132578 );
      talent.brewmaster.light_brewing                       = _ST( "Light Brewing" );
      talent.brewmaster.training_of_niuzao                  = _ST( "Training of Niuzao" );
      talent.brewmaster.pretense_of_instability             = _ST( "Pretense of Instability" );
      talent.brewmaster.face_palm                           = _ST( "Face Palm" );
      // Row 8
      talent.brewmaster.dragonfire_brew                     = _ST( "Dragonfire Brew" );
      talent.brewmaster.charred_passions                    = _ST( "Charred Passions" );
      talent.brewmaster.high_tolerance                      = _ST( "High Tolerance" );
      talent.brewmaster.walk_with_the_ox                    = _ST( "Walk with the Ox" );
      talent.brewmaster.elusive_footwork                    = _ST( "Elusive Footwork" );
      talent.brewmaster.anvil_and_stave                     = _ST( "Anvil & Stave" );
      talent.brewmaster.counterstrike                       = _ST( "Counterstrike" );
      // Row 9
      talent.brewmaster.bonedust_brew                       = _ST( "Bonedust Brew" );
      talent.brewmaster.improved_invoke_niuzao_the_black_ox = is_ptr() ? _ST( "Improved Invoke Niuzao, the Black Ox" ) : _STID( 322740 );
      talent.brewmaster.exploding_keg                       = _ST( "Exploding Keg" );
      talent.brewmaster.blackout_combo                      = _ST( "Blackout Combo" );
      talent.brewmaster.weapons_of_order                    = _ST( "Weapons of Order" );
      // Row 10
      talent.brewmaster.bountiful_brew                      = _ST( "Bountiful Brew" );
      talent.brewmaster.attenuation                         = _ST( "Attenuation" );
      talent.brewmaster.stormstouts_last_keg                = _ST( "Stormstout's Last Keg" );
      talent.brewmaster.call_to_arms                        = _ST( "Call to Arms" );
      talent.brewmaster.chi_surge                           = _ST( "Chi Surge" );
  }

  // ========
  // Mistweaver
  // ========

  if ( spec_tree == MONK_MISTWEAVER )
  {
      // Row 1
      talent.mistweaver.enveloping_mist                     = _ST( "Enveloping Mist" );
      // Row 2
      talent.mistweaver.essence_font                        = _ST( "Essence Font" );
      talent.mistweaver.renewing_mist                       = _ST( "Renewing Mist" );
      // Row 3
      talent.mistweaver.life_cocoon                         = _ST( "Life Cocoon" );
      talent.mistweaver.thunder_focus_tea                   = _ST( "Thunder Focus Tea" );
      talent.mistweaver.invigorating_mists                  = _ST( "Invigorating Mists" );
      // Row 4
      talent.mistweaver.teachings_of_the_monastery          = _ST( "Teachings of the Monastery" );
      talent.mistweaver.revival                             = _ST( "Revival" );
      talent.mistweaver.restoral                            = _ST( "Restoral" );
      talent.mistweaver.song_of_chi_ji                      = _ST( "Song of Chi-Ji" );
      talent.mistweaver.mastery_of_mist                     = _ST( "Mastery of Mist" );
      // Row 5
      talent.mistweaver.spirit_of_the_crane                 = _ST( "Spirit of the Crane" );
      talent.mistweaver.mists_of_life                       = _ST( "Mists of Life" );
      talent.mistweaver.uplifting_spirits                   = _ST( "Uplifting Spirits" );
      talent.mistweaver.font_of_life                        = _ST( "Font of Life" );
      talent.mistweaver.zen_pulse                           = _ST( "Zen Pulse" );
      talent.mistweaver.healing_elixir                      = _ST( "Healing Elixir" );
      // Row 6
      talent.mistweaver.nourishing_chi                      = _ST( "Nourishing Chi" );
      talent.mistweaver.overflowing_mists                   = _ST( "Overflowing Mists" );
      talent.mistweaver.invoke_yulon_the_jade_serpent       = _ST( "Invoke Yu'lon, the Jade Serpent" );
      talent.mistweaver.invoke_chi_ji_the_red_crane         = _ST( "Invoke Chi-Ji, the Red Crane" );
      talent.mistweaver.echoing_reverberation               = _ST( "Echoing Reverberation" );
      talent.mistweaver.accumulating_mist                   = _ST( "Accumulating Mist" );
      talent.mistweaver.rapid_diffusion                     = _ST( "Rapid Diffusion" );
      // Row 7
      talent.mistweaver.calming_coalescence                 = _ST( "Calming Coalescence" );
      talent.mistweaver.yulons_whisper                      = _ST( "Yu'lon's Whisper" );
      talent.mistweaver.mist_wrap                           = _ST( "Mist Wrap" );
      talent.mistweaver.refreshing_jade_wind                = _ST( "Refreshing Jade Wind" );
      talent.mistweaver.enveloping_breath                   = _ST( "Enveloping Breath" );
      talent.mistweaver.dancing_mists                       = _ST( "Dancing Mists" );
      talent.mistweaver.lifecycles                          = _ST( "Lifecycles" );
      talent.mistweaver.mana_tea                            = _ST( "Mana Tea" );
      // Row 8
      talent.mistweaver.faeline_stomp                       = _ST( "Faeline Stomp" );
      talent.mistweaver.ancient_teachings                   = _ST( "Ancient Teachings" );
      talent.mistweaver.clouded_focus                       = _ST( "Clouded Focus" );
      talent.mistweaver.jade_bond                           = _ST( "Jade Bond" );
      talent.mistweaver.gift_of_the_celestials              = _ST( "Gift of the Celestials" );
      talent.mistweaver.focused_thunder                     = _ST( "Focused Thunder" );
      talent.mistweaver.upwelling                           = _ST( "Upwelling" );
      talent.mistweaver.bonedust_brew                       = _ST( "Bonedust Brew" );
      // Row 9
      talent.mistweaver.ancient_concordance                 = _ST( "Ancient Concordance" );
      talent.mistweaver.resplendent_mist                    = _ST( "Resplendent Mist" );
      talent.mistweaver.secret_infusion                     = _ST( "Secret Infusion" );
      talent.mistweaver.misty_peaks                         = _ST( "Misty Peaks" );
      talent.mistweaver.peaceful_mending                    = _ST( "Peaceful Mending" );
      talent.mistweaver.bountiful_brew                      = _ST( "Bountiful Brew" );
      talent.mistweaver.attenuation                         = _ST( "Attenuation" );
      // Row 10
      talent.mistweaver.awakened_faeline                    = _ST( "Awakened Faeline" );
      talent.mistweaver.tea_of_serenity                     = _ST( "Tea of Serenity" );
      talent.mistweaver.tea_of_plenty                       = _ST( "Tea of Plenty" );
      talent.mistweaver.unison                              = _ST( "Unison" );
      talent.mistweaver.mending_proliferation               = _ST( "Mending Proliferation" );
      talent.mistweaver.invokers_delight                    = _ST( "Invoker's Delight" );
      talent.mistweaver.tear_of_morning                     = _ST( "Tear of Morning" );
      talent.mistweaver.rising_mist                         = _ST( "Rising Mist" );
  }

  // ========
  // Windwalker
  // ========

  if ( spec_tree == MONK_WINDWALKER )
  {
      // Row 1
      talent.windwalker.fists_of_fury                       = _ST( "Fists of Fury" );
      // Row 2
      talent.windwalker.touch_of_karma                      = _ST( "Touch of Karma" );
      talent.windwalker.ascension                           = _ST( "Ascension" );
      talent.windwalker.power_strikes                       = _ST( "Power Strikes" );
      // Row 3
      talent.windwalker.widening_whirl                      = _ST( "Widening Whirl" );
      talent.windwalker.touch_of_the_tiger                  = _ST( "Touch of the Tiger" );
      talent.windwalker.hardened_soles                      = _ST( "Hardened Soles" );
      talent.windwalker.flashing_fists                      = _ST( "Flashing Fists" );
      talent.windwalker.open_palm_strikes                   = _ST( "Open Palm Strikes" );
      // Row 4
      talent.windwalker.mark_of_the_crane                   = _ST( "Mark of the Crane" );
      talent.windwalker.flying_serpent_kick                 = _ST( "Flying Serpent Kick" );
      talent.windwalker.glory_of_the_dawn                   = _ST( "Glory of the Dawn" );
      // 8 Required
      // Row 5
      talent.windwalker.shadowboxing_treads                 = _STID( 392982 );;
      talent.windwalker.inner_peace                         = _ST( "Inner Peace" );
      talent.windwalker.storm_earth_and_fire                = _ST( "Storm, Earth, and Fire" );
      talent.windwalker.serenity                            = _ST( "Serenity" );
      talent.windwalker.meridian_strikes                    = _ST( "Meridian Strikes" );
      talent.windwalker.strike_of_the_windlord              = _ST( "Strike of the Windlord" );
      // Row 6
      talent.windwalker.dance_of_chiji                      = _ST( "Dance of Chi-Ji" );
      talent.windwalker.jade_ignition                       = _ST( "Jade Ignition" );     
      talent.windwalker.drinking_horn_cover                 = _ST( "Drinking Horn Cover" );
      talent.windwalker.spiritual_focus                     = _ST( "Spiritual Focus" );
      talent.windwalker.hit_combo                           = _ST( "Hit Combo" );
     
      // Row 7
      talent.windwalker.rushing_jade_wind                   = _ST( "Rushing Jade Wind" );
      talent.windwalker.forbidden_technique                 = _ST( "Forbidden Technique" );
      talent.windwalker.invoke_xuen_the_white_tiger         = _ST( "Invoke Xuen, the White Tiger" );
      talent.windwalker.teachings_of_the_monastery          = _ST( "Teachings of the Monastery" );
      talent.windwalker.thunderfist                         = _ST( "Thunderfist" );
      // 20 Required
      // Row 8
      talent.windwalker.crane_vortex                        = _ST( "Crane Vortex" );
      talent.windwalker.xuens_bond                          = _ST( "Xuen's Bond" );
      talent.windwalker.fury_of_xuen                        = _ST( "Fury of Xuen" );
      talent.windwalker.empowered_tiger_lightning           = _ST( "Empowered Tiger Lightning" );
      talent.windwalker.rising_star                         = _ST( "Rising Star" );
      // Row 9
      talent.windwalker.bonedust_brew                       = _ST( "Bonedust Brew" );
      talent.windwalker.fatal_flying_guillotine             = _ST( "Fatal Flying Guillotine" );
      talent.windwalker.last_emperors_capacitor             = _ST( "Last Emperor's Capacitor" );
      talent.windwalker.xuens_battlegear                    = _ST( "Xuen's Battlegear" );
      talent.windwalker.transfer_the_power                  = _ST( "Transfer the Power" );
      talent.windwalker.whirling_dragon_punch               = _ST( "Whirling Dragon Punch" );
      talent.windwalker.faeline_stomp                       = _ST( "Faeline Stomp" );
      // Row 10
      talent.windwalker.attenuation                         = _ST( "Attenuation" );
      talent.windwalker.dust_in_the_wind                    = _ST( "Dust in the Wind" );
      talent.windwalker.skyreach                            = _ST( "Skyreach" );
      talent.windwalker.invokers_delight                    = _ST( "Invoker's Delight" );
      talent.windwalker.way_of_the_fae                      = _ST( "Way of the Fae" );
      talent.windwalker.faeline_harmony                     = _ST( "Faeline Harmony" );
  }

  // Specialization spells ====================================
  // Multi-Specialization & Class Spells
  spec.blackout_kick             = find_class_spell( "Blackout Kick" );
  spec.blackout_kick_2           = find_rank_spell( "Blackout Kick", "Rank 2", MONK_WINDWALKER );
  spec.blackout_kick_3           = find_rank_spell( "Blackout Kick", "Rank 3", MONK_WINDWALKER );
  spec.blackout_kick_brm         = find_spell( 205523 );
  spec.crackling_jade_lightning  = find_class_spell( "Crackling Jade Lightning" );
  spec.critical_strikes          = find_specialization_spell( "Critical Strikes" );
  //spec.detox                     = find_specialization_spell( "Detox" ); // talent.general.detox
  spec.expel_harm                = find_spell( 322101 ); 
  spec.expel_harm_2_brm          = find_rank_spell( "Expel Harm", "Rank 2", MONK_BREWMASTER );
  spec.expel_harm_2_mw           = find_rank_spell( "Expel Harm", "Rank 2", MONK_MISTWEAVER );
  spec.expel_harm_2_ww           = find_rank_spell( "Expel Harm", "Rank 2", MONK_WINDWALKER );
  spec.fortifying_brew           = find_spell( 115203 );
  spec.leather_specialization    = find_specialization_spell( "Leather Specialization" );
  spec.leg_sweep                 = find_class_spell( "Leg Sweep" );
  spec.mystic_touch              = find_class_spell( "Mystic Touch" );
  //spec.paralysis                 = find_class_spell( "Paralysis" );// talent.general.paralysis
  spec.provoke                   = find_class_spell( "Provoke" ); 
  spec.provoke_2                 = find_rank_spell( "Provoke", "Rank 2" );
  spec.resuscitate               = find_class_spell( "Resuscitate" );
  //spec.rising_sun_kick           = find_specialization_spell( "Rising Sun Kick" ); // talent.general.rising_sun_kick
  spec.rising_sun_kick_2         = find_rank_spell( "Rising Sun Kick", "Rank 2" );
  spec.roll                      = find_class_spell( "Roll" );
  //spec.roll_2                    = find_rank_spell( "Roll", "Rank 2" ); // talent.general.roll
  //spec.spear_hand_strike         = find_specialization_spell( "Spear Hand Strike" ); // talent.general.spear_hand_strike
  spec.spinning_crane_kick       = find_class_spell( "Spinning Crane Kick" );
  spec.spinning_crane_kick_brm   = find_spell( 322729 );
  spec.spinning_crane_kick_2_brm = find_rank_spell( "Spinning Crane Kick", "Rank 2", MONK_BREWMASTER );
  spec.spinning_crane_kick_2_ww  = find_rank_spell( "Spinning Crane Kick", "Rank 2", MONK_WINDWALKER );
  spec.tiger_palm                = find_class_spell( "Tiger Palm" );
  spec.touch_of_death            = find_class_spell( "Touch of Death" );
  spec.touch_of_death_2          = find_rank_spell( "Touch of Death", "Rank 2" );
  spec.touch_of_death_3_brm      = find_rank_spell( "Touch of Death", "Rank 3", MONK_BREWMASTER );
  spec.touch_of_death_3_mw       = find_rank_spell( "Touch of Death", "Rank 3", MONK_MISTWEAVER );
  spec.touch_of_death_3_ww       = find_rank_spell( "Touch of Death", "Rank 3", MONK_WINDWALKER );
  spec.two_hand_adjustment       = find_specialization_spell( 346104 );
  spec.vivify                    = find_class_spell( "Vivify" );
  spec.vivify_2_brm              = find_rank_spell( "Vivify", "Rank 2", MONK_BREWMASTER );
  spec.vivify_2_mw               = find_rank_spell( "Vivify", "Rank 2", MONK_MISTWEAVER );
  spec.vivify_2_ww               = find_rank_spell( "Vivify", "Rank 2", MONK_WINDWALKER );

  // Brewmaster Specialization
  if ( spec_tree == MONK_BREWMASTER )
  {
      spec.bladed_armor         = find_specialization_spell( "Bladed Armor" );
      //spec.breath_of_fire      = find_specialization_spell( "Breath of Fire" ); // talent.brewmaster.breath_of_fire
      spec.brewmasters_balance  = find_specialization_spell( "Brewmaster's Balance" );
      spec.brewmaster_monk      = find_specialization_spell( "Brewmaster Monk" );
      //spec.celestial_brew      = find_specialization_spell( "Celestial Brew" ); // talent.brewmaster.celestial_brew
      //spec.celestial_brew_2    = find_rank_spell( "Celestial Brew", "Rank 2" ); // talent.brewmaster.celestial_brew_rank_2
      spec.celestial_fortune    = find_specialization_spell( "Celestial Fortune" );
      //spec.clash               = find_specialization_spell( "Clash" ); // talent.brewmaster.clash
      //spec.gift_of_the_ox      = find_specialization_spell( "Gift of the Ox" ); // talent.brewmaster.gift_of_the_ox
      //spec.invoke_niuzao       = find_specialization_spell( "Invoke Niuzao, the Black Ox" ); // talent.brewmaster.invoke_niuzao_the_black_ox
      //spec.invoke_niuzao_2     = find_specialization_spell( "Invoke Niuzao, the Black Ox", "Rank 2" ); talent.brewmaster.improved_invoke_niuzao_the_black_ox
      //spec.keg_smash           = find_specialization_spell( "Keg Smash" ); // talent.brewmaster.keg_smash
      //spec.purifying_brew      = find_specialization_spell( "Purifying Brew" ); // talent.brewmaster.purifying_brew
      //spec.purifying_brew_2    = find_rank_spell( "Purifying Brew", "Rank 2" ); // talent.brewmaster.purifying_brew_rank_2
      //spec.shuffle             = find_specialization_spell( "Shuffle" ); // talent.brewmaster.shuffle
      //spec.stagger             = find_specialization_spell( "Stagger" ); // talent.brewmaster.stagger
      spec.stagger_2            = find_rank_spell( "Stagger", "Rank 2" );
      //spec.zen_meditation      = find_specialization_spell( "Zen Meditation" ); // talent.brewmaster.zen_meditation
  }

  // Mistweaver Specialization
  if ( spec_tree == MONK_MISTWEAVER )
  {
      //spec.detox                      = find_specialization_spell( "Detox" ); // talent.general.detox
      spec.enveloping_mist              = find_specialization_spell( "Enveloping Mist" );
      spec.envoloping_mist_2            = find_rank_spell( "Enveloping Mist", "Rank 2" );
      spec.essence_font                 = find_specialization_spell( "Essence Font" );
      spec.essence_font_2               = find_rank_spell( "Essence Font", "Rank 2" );
      spec.invoke_yulon                 = find_specialization_spell( "Invoke Yu'lon, the Jade Serpent" );
      spec.life_cocoon                  = find_specialization_spell( "Life Cocoon" );
      spec.life_cocoon_2                = find_rank_spell( "Life Cocoon", "Rank 2" );
      spec.mistweaver_monk              = find_specialization_spell( "Mistweaver Monk" );
      spec.reawaken                     = find_specialization_spell( "Reawaken" );
      spec.renewing_mist                = find_specialization_spell( "Renewing Mist" );
      spec.renewing_mist_2              = find_rank_spell( "Renewing Mist", "Rank 2" );
      spec.revival                      = find_specialization_spell( "Revival" );
      spec.revival_2                    = find_rank_spell( "Revival", "Rank 2" );
      spec.soothing_mist                = find_specialization_spell( "Soothing Mist" );
      spec.teachings_of_the_monastery   = find_specialization_spell( "Teachings of the Monastery" );
      spec.thunder_focus_tea            = find_specialization_spell( "Thunder Focus Tea" );
      spec.thunder_focus_tea_2          = find_rank_spell( "Thunder Focus Tea", "Rank 2" );
  }

  // Windwalker Specialization
  if ( spec_tree == MONK_WINDWALKER )
  {
      spec.afterlife                    = find_specialization_spell( "Afterlife" );
      spec.afterlife_2                  = find_rank_spell( "Afterlife", "Rank 2" );
      spec.combat_conditioning          = find_specialization_spell( "Combat Conditioning" );
      spec.combo_breaker                = find_specialization_spell( "Combo Breaker" );
      spec.cyclone_strikes              = find_specialization_spell( "Cyclone Strikes" );
      //spec.disable                    = find_specialization_spell( "Disable" ); // talent.windwalker.disable
      spec.disable_2                    = find_rank_spell( "Disable", "Rank 2" );
      //spec.fists_of_fury              = find_specialization_spell( "Fists of Fury" ); // talent.windwalker.fists_of_fury
      //spec.flying_serpent_kick        = find_specialization_spell( "Flying Serpent Kick" ); // talent.windwalker.flying_serpent_kick
      spec.flying_serpent_kick_2        = find_rank_spell( "Flying Serpent Kick", "Rank 2" );
      //spec.invoke_xuen                = find_specialization_spell( "Invoke Xuen, the White Tiger" ); // talent.windwalker.invoke_xuen_the_white_tiger
      //spec.invoke_xuen_2              = find_rank_spell( "Invoke Xuen, the White Tiger", "Rank 2" ); // talent.windwalker.empowered_tiger_lightning
      spec.reverse_harm                 = find_spell(342928);
      spec.stance_of_the_fierce_tiger   = find_specialization_spell( "Stance of the Fierce Tiger" );
      //spec.storm_earth_and_fire       = find_specialization_spell( "Storm, Earth, and Fire" );
      spec.storm_earth_and_fire_2       = find_rank_spell( "Storm, Earth, and Fire", "Rank 2" );
      //spec.touch_of_karma             = find_specialization_spell( "Touch of Karma" ); // talent.windwalker.touch_of_karma
      spec.windwalker_monk = find_specialization_spell( "Windwalker Monk" );
      //spec.windwalking                = find_specialization_spell( "Windwalking" ); // talent.general.windwalking
  }

  // Covenant Abilities ================================

  covenant.kyrian    = find_covenant_spell( "Weapons of Order" );
  covenant.night_fae = find_covenant_spell( "Faeline Stomp" );
  covenant.venthyr = find_covenant_spell( "Fallen Order" );
  covenant.necrolord = find_covenant_spell( "Bonedust Brew" );

  // Soulbind Conduits Abilities =======================

  // General
  conduit.dizzying_tumble        = find_conduit_spell( "Dizzying Tumble" );
  conduit.fortifying_ingredients = find_conduit_spell( "Fortifying Ingredients" );
  conduit.grounding_breath       = find_conduit_spell( "Grounding Breath" );
  conduit.harm_denial            = find_conduit_spell( "Harm Denial" );
  conduit.lingering_numbness     = find_conduit_spell( "Lingering Numbness" );
  conduit.swift_transference     = find_conduit_spell( "Swift Transference" );
  conduit.tumbling_technique     = find_conduit_spell( "Tumbling Technique" );

  // Brewmaster
  conduit.celestial_effervescence = find_conduit_spell( "Celestial Effervescence" );
  conduit.evasive_stride          = find_conduit_spell( "Evasive Stride" );
  conduit.scalding_brew           = find_conduit_spell( "Scalding Brew" );
  conduit.walk_with_the_ox        = find_conduit_spell( "Walk with the Ox" );

  // Mistweaver
  conduit.jade_bond          = find_conduit_spell( "Jade Bond" );
  conduit.nourishing_chi     = find_conduit_spell( "Nourishing Chi" );
  conduit.rising_sun_revival = find_conduit_spell( "Rising Sun Revival" );
  conduit.resplendent_mist   = find_conduit_spell( "Resplendent Mist" );

  // Windwalker
  conduit.calculated_strikes    = find_conduit_spell( "Calculated Strikes" );
  conduit.coordinated_offensive = find_conduit_spell( "Coordinated Offensive" );
  conduit.inner_fury            = find_conduit_spell( "Inner Fury" );
  conduit.xuens_bond            = find_conduit_spell( "Xuen's Bond" );

  // Covenant
  conduit.strike_with_clarity = find_conduit_spell( "Strike with Clarity" );
  conduit.imbued_reflections  = find_conduit_spell( "Imbued Reflections" );
  conduit.bone_marrow_hops    = find_conduit_spell( "Bone Marrow Hops" );
  conduit.way_of_the_fae      = find_conduit_spell( "Way of the Fae" );

  // Shadowland Legendaries ============================

  // General
  legendary.fatal_touch         = find_runeforge_legendary( "Fatal Touch" );
  legendary.invokers_delight    = find_runeforge_legendary( "Invoker's Delight" );
  legendary.swiftsure_wraps     = find_runeforge_legendary( "Swiftsure Wraps" );
  legendary.escape_from_reality = find_runeforge_legendary( "Escape from Reality" );

  // Brewmaster
  legendary.charred_passions     = find_runeforge_legendary( "Charred Passions" );
  legendary.celestial_infusion   = find_runeforge_legendary( "Celestial Infusion" );
  legendary.shaohaos_might       = find_runeforge_legendary( "Shaohao's Might" );
  legendary.stormstouts_last_keg = find_runeforge_legendary( "Stormstout's Last Keg" );

  // Mistweaver
  legendary.ancient_teachings_of_the_monastery = find_runeforge_legendary( "Ancient Teachings of the Monastery" );
  legendary.clouded_focus                      = find_runeforge_legendary( "Clouded Focus" );
  legendary.tear_of_morning                    = find_runeforge_legendary( "Tear of Morning" );
  legendary.yulons_whisper                     = find_runeforge_legendary( "Yu'lon's Whisper" );

  // Windwalker
  legendary.jade_ignition           = find_runeforge_legendary( "Jade Ignition" );
  legendary.keefers_skyreach        = find_runeforge_legendary( "Keefer's Skyreach" );
  legendary.last_emperors_capacitor = find_runeforge_legendary( "Last Emperor's Capacitor" );
  legendary.xuens_battlegear        = find_runeforge_legendary( "Xuen's Treasure" );

  // Covenant
  legendary.bountiful_brew     = find_runeforge_legendary( "Bountiful Brew" );
  legendary.call_to_arms       = find_runeforge_legendary( "Call to Arms" );
  legendary.faeline_harmony    = find_runeforge_legendary( "Faeline Harmony" );
  legendary.sinister_teachings = find_runeforge_legendary( "Sinister Teachings" );

  // Passives =========================================
  // General
  passives.aura_monk               = find_spell( 137022 );
  passives.chi_burst_damage        = find_spell( 148135 );
  passives.chi_burst_energize      = find_spell( 261682 );
  passives.chi_burst_heal          = find_spell( 130654 );
  passives.chi_wave_damage         = find_spell( 132467 );
  passives.chi_wave_heal           = find_spell( 132463 );
  passives.claw_of_the_white_tiger = find_spell( 389541 );
  passives.fortifying_brew         = find_spell( 120954 );
  passives.healing_elixir =
      find_spell( 122281 );  // talent.healing_elixir -> effectN( 1 ).trigger() -> effectN( 1 ).trigger()
  passives.mystic_touch            = find_spell( 8647 );

  // Brewmaster
  passives.breath_of_fire_dot           = find_spell( 123725 );
  passives.celestial_fortune            = find_spell( 216521 );
  passives.dragonfire_brew              = find_spell( 387621 );
  passives.elusive_brawler              = find_spell( 195630 );
  passives.gai_plins_imperial_brew_heal = find_spell( 383701 );
  passives.gift_of_the_ox_heal          = find_spell( 124507 );
  passives.shuffle                      = find_spell( 215479 );
  passives.keg_smash_buff               = find_spell( 196720 );
  passives.special_delivery             = find_spell( 196733 );
  passives.stagger_self_damage          = find_spell( 124255 );
  passives.heavy_stagger                = find_spell( 124273 );
  passives.stomp                        = find_spell( 227291 );

  // Mistweaver
  passives.totm_bok_proc        = find_spell( 228649 );
  passives.renewing_mist_heal   = find_spell( 119611 );
  passives.soothing_mist_heal   = find_spell( 115175 );
  passives.soothing_mist_statue = find_spell( 198533 );
  passives.spirit_of_the_crane  = find_spell( 210803 );
  passives.zen_pulse_heal       = find_spell( 198487 );

  // Windwalker
  passives.bok_proc                         = find_spell( 116768 );
  passives.crackling_tiger_lightning        = find_spell( 123996 );
  passives.crackling_tiger_lightning_driver = find_spell( 123999 );
  passives.cyclone_strikes                  = find_spell( 220358 );
  passives.dance_of_chiji                   = find_spell( 325202 );
  passives.dance_of_chiji_bug               = find_spell( 286585 );
  passives.dizzying_kicks                   = find_spell( 196723 );
  passives.empowered_tiger_lightning        = find_spell( 335913 );
  passives.fists_of_fury_tick               = find_spell( 117418 );
  passives.flying_serpent_kick_damage       = find_spell( 123586 );
  passives.focus_of_xuen                    = find_spell( 252768 );
  passives.fury_of_xuen_stacking_buff       = find_spell( 396167 );
  passives.fury_of_xuen_haste_buff          = find_spell( 396168 );
  passives.glory_of_the_dawn_damage         = find_spell( 392959 );
  passives.hidden_masters_forbidden_touch   = find_spell( 213114 );
  passives.hit_combo                        = find_spell( 196741 );
  passives.keefers_skyreach_debuff          = find_spell( 344021 );
  passives.mark_of_the_crane                = find_spell( 228287 );
  passives.power_strikes_chi                = find_spell( 121283 );
  passives.thunderfist                      = find_spell( 242387 );
  passives.touch_of_karma_tick              = find_spell( 124280 );
  passives.whirling_dragon_punch_tick       = find_spell( 158221 );

  // Covenants
  passives.bonedust_brew_dmg                    = find_spell( 325217 );
  passives.bonedust_brew_heal                   = find_spell( 325218 );
  passives.bonedust_brew_chi                    = find_spell( 328296 );
  passives.bonedust_brew_grounding_breath       = find_spell( 342330 );
  passives.bonedust_brew_attenuation            = find_spell( 394514 );
  passives.faeline_stomp_damage                 = find_spell( 345727 );
  passives.faeline_stomp_ww_damage              = find_spell( 327264 );
  passives.faeline_stomp_brm                    = find_spell( 347480 );
  passives.fallen_monk_breath_of_fire           = find_spell( 330907 );
  passives.fallen_monk_clash                    = find_spell( 330909 );
  passives.fallen_monk_enveloping_mist          = find_spell( 344008 );
  passives.fallen_monk_fallen_brew              = find_spell( 363041 );
  passives.fallen_monk_fists_of_fury            = find_spell( 330898 );
  passives.fallen_monk_fists_of_fury_tick       = find_spell( 345714 );
  passives.fallen_monk_keg_smash                = find_spell( 330911 );
  passives.fallen_monk_soothing_mist            = find_spell( 328283 );
  passives.fallen_monk_spec_duration            = find_spell( 347826 );
  passives.fallen_monk_spinning_crane_kick      = find_spell( 330901 );
  passives.fallen_monk_spinning_crane_kick_tick = find_spell( 330903 );
  passives.fallen_monk_tiger_palm               = find_spell( 346602 );
  passives.fallen_monk_windwalking              = find_spell( 364944 );

  // Conduits
  passives.fortifying_ingredients = find_spell( 336874 );
  passives.evasive_stride         = find_spell( 343764 );

  // Shadowland Legendary
  passives.chi_explosion                          = find_spell( 337342 );
  passives.fae_exposure_dmg                       = find_spell( 356773 );
  passives.fae_exposure_heal                      = find_spell( 356774 );
  passives.shaohaos_might                         = find_spell( 337570 );
  passives.charred_passions_dmg                   = find_spell( 338141 );
  passives.call_to_arms_invoke_xuen               = find_spell( 358518 );
  passives.call_to_arms_invoke_niuzao             = find_spell( 358520 );
  passives.call_to_arms_invoke_yulon              = find_spell( 358521 );
  passives.call_to_arms_invoke_chiji              = find_spell( 358522 );
  passives.call_to_arms_empowered_tiger_lightning = find_spell( 360829 );

  // Tier 29
  passives.kicks_of_flowing_momentum = find_spell( 394944 );
  passives.fists_of_flowing_momentum = find_spell( 394949 );

  // Mastery spells =========================================
  mastery.combo_strikes   = find_mastery_spell( MONK_WINDWALKER );
  mastery.elusive_brawler = find_mastery_spell( MONK_BREWMASTER );
  mastery.gust_of_mists   = find_mastery_spell( MONK_MISTWEAVER );

  // Sample Data
  sample_datas.stagger_total_damage       = get_sample_data( "Damage added to stagger pool" );
  sample_datas.stagger_damage             = get_sample_data( "Effective stagger damage" );
  sample_datas.light_stagger_damage       = get_sample_data( "Effective light stagger damage" );
  sample_datas.moderate_stagger_damage    = get_sample_data( "Effective moderate stagger damage" );
  sample_datas.heavy_stagger_damage       = get_sample_data( "Effective heavy stagger damage" );
  sample_datas.purified_damage            = get_sample_data( "Stagger damage that was purified" );
  sample_datas.staggering_strikes_cleared = get_sample_data( "Stagger damage that was cleared by Staggering Strikes" );
  sample_datas.quick_sip_cleared          = get_sample_data( "Stagger damage that was cleared by Quick Sip" );
  sample_datas.tranquil_spirit            = get_sample_data( "Stagger damage that was cleared by Tranquil Spirit" );

  //================================================================================
  // Shared Spells
  // These spells share common effects but are unique in that you may only have one
  //================================================================================

  // Returns first valid spell in argument list, pass highest priority to first argument
  // Returns spell_data_t::nil() if none are valid
  auto _priority = [ ] ( auto ... spell_list )
  {
    for ( auto spell : { (const spell_data_t*)spell_list... } )
      if ( spell && spell->ok() )
        return spell;
    return spell_data_t::nil();
  };

  shared.attenuation =
    _priority( conduit.bone_marrow_hops, talent.windwalker.attenuation, talent.brewmaster.attenuation, talent.mistweaver.attenuation );

  shared.bonedust_brew =
    _priority( covenant.necrolord, talent.windwalker.bonedust_brew, talent.brewmaster.bonedust_brew, talent.mistweaver.bonedust_brew );

  shared.bountiful_brew =
    _priority( legendary.bountiful_brew, talent.brewmaster.bountiful_brew, talent.mistweaver.bountiful_brew );

  shared.call_to_arms =
    _priority( legendary.call_to_arms, talent.brewmaster.call_to_arms );

  shared.charred_passions =
    _priority( legendary.charred_passions, talent.brewmaster.charred_passions );

  shared.face_palm =
    _priority( legendary.shaohaos_might, talent.brewmaster.face_palm );

  shared.fatal_touch =
    _priority( legendary.fatal_touch, talent.general.fatal_touch );

  // Does Mistweaver Awakened Faeline stack with this effect? TODO: How is this handled?
  shared.faeline_harmony =
    _priority( legendary.faeline_harmony, talent.windwalker.faeline_harmony );

  shared.faeline_stomp =
    _priority( covenant.night_fae, talent.windwalker.faeline_stomp, talent.mistweaver.faeline_stomp );

  shared.healing_elixir =
    _priority( talent.brewmaster.healing_elixir, talent.mistweaver.healing_elixir );

  shared.invokers_delight =
    _priority( legendary.invokers_delight, talent.windwalker.invokers_delight, talent.mistweaver.invokers_delight );

  shared.jade_ignition =
    _priority( legendary.jade_ignition, talent.windwalker.jade_ignition );

  shared.last_emperors_capacitor =
    _priority( legendary.last_emperors_capacitor, talent.windwalker.last_emperors_capacitor );

  shared.rushing_jade_wind =
    _priority( talent.windwalker.rushing_jade_wind, talent.brewmaster.rushing_jade_wind );

  shared.scalding_brew =
    _priority( conduit.scalding_brew, talent.brewmaster.scalding_brew );

  shared.skyreach =
    _priority( legendary.keefers_skyreach, talent.windwalker.skyreach );

  shared.stormstouts_last_keg =
    _priority( legendary.stormstouts_last_keg, talent.brewmaster.stormstouts_last_keg );

  shared.teachings_of_the_monastery =
    _priority( talent.mistweaver.teachings_of_the_monastery, talent.windwalker.teachings_of_the_monastery );

  shared.walk_with_the_ox = 
    _priority( conduit.walk_with_the_ox, talent.brewmaster.walk_with_the_ox);

  shared.way_of_the_fae =
    _priority( conduit.way_of_the_fae, talent.windwalker.way_of_the_fae );

  shared.weapons_of_order =
    _priority( covenant.kyrian, talent.brewmaster.weapons_of_order );

  shared.xuens_battlegear =
    _priority( legendary.xuens_battlegear, talent.windwalker.xuens_battlegear );

  shared.xuens_bond =
    _priority( conduit.xuens_bond, talent.windwalker.xuens_bond );

  // Active Action Spells
  
  // General
  active_actions.resonant_fists = new actions::spells::resonant_fists_t( *this );
  close_to_heart_aura           = new actions::close_to_heart_aura_t( this );
  generous_pour_aura            = new actions::generous_pour_aura_t( this );
  windwalking_aura              = new actions::windwalking_aura_t( this );

  // Brewmaster
  if ( spec_tree == MONK_BREWMASTER )
  {
    active_actions.breath_of_fire         = new actions::spells::breath_of_fire_dot_t ( *this );
    active_actions.charred_passions       = new actions::attacks::charred_passions_sck_t( this );
    active_actions.celestial_fortune      = new actions::heals::celestial_fortune_t ( *this );
    active_actions.exploding_keg          = new actions::spells::exploding_keg_proc_t( this );
    active_actions.gift_of_the_ox_trigger = new actions::gift_of_the_ox_trigger_t ( *this );
    active_actions.gift_of_the_ox_expire  = new actions::gift_of_the_ox_expire_t ( *this );
    active_actions.stagger_self_damage    = new actions::stagger_self_damage_t ( this );
  }

  // Windwalker
  if ( spec_tree == MONK_WINDWALKER )
  {
    active_actions.empowered_tiger_lightning  = new actions::empowered_tiger_lightning_t ( *this );
    active_actions.fury_of_xuen_empowered_tiger_lightning = new actions::fury_of_xuen_empowered_tiger_lightning_t(*this);
  }

  // Conduit
  active_actions.evasive_stride = new actions::heals::evasive_stride_t( *this );

  // Covenant
  active_actions.bonedust_brew_dmg  = new actions::spells::bonedust_brew_damage_t( *this );
  active_actions.bonedust_brew_heal = new actions::spells::bonedust_brew_heal_t( *this );

  // Legendary
  active_actions.bountiful_brew = new actions::spells::bountiful_brew_t( *this );
  active_actions.call_to_arms_empowered_tiger_lightning = new actions::call_to_arms_empowered_tiger_lightning_t( *this );

  // Passive Action Spells
  passive_actions.thunderfist = new actions::thunderfist_t( this );
}

// monk_t::init_base ========================================================

void monk_t::init_base_stats()
{
  if ( base.distance < 1 )
  {
    if ( specialization() == MONK_MISTWEAVER )
      base.distance = 40;
    else
      base.distance = 5;
  }
  base_t::init_base_stats();

  base_gcd = timespan_t::from_seconds( 1.5 );

  switch ( specialization() )
  {
    case MONK_BREWMASTER:
    {
      base_gcd += spec.brewmaster_monk->effectN( 14 ).time_value();  // Saved as -500 milliseconds
      base.attack_power_per_agility                      = 1.0;
      base.spell_power_per_attack_power                  = spec.brewmaster_monk->effectN( 18 ).percent();
      resources.base[ RESOURCE_ENERGY ]                  = 100;
      resources.base[ RESOURCE_MANA ]                    = 0;
      resources.base[ RESOURCE_CHI ]                     = 0;
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10.0;
      resources.base_regen_per_second[ RESOURCE_MANA ]   = 0;
      break;
    }
    case MONK_MISTWEAVER:
    {
      base.spell_power_per_intellect                     = 1.0;
      base.attack_power_per_spell_power                  = spec.mistweaver_monk->effectN( 4 ).percent();
      resources.base[ RESOURCE_ENERGY ]                  = 0;
      resources.base[ RESOURCE_CHI ]                     = 0;
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 0;
      break;
    }
    case MONK_WINDWALKER:
    {
      if ( base.distance < 1 )
        base.distance = 5;
      // base_gcd += spec.windwalker_monk->effectN( 14 ).time_value();  // Saved as -500 milliseconds
      base.attack_power_per_agility     = 1.0;
      base.spell_power_per_attack_power = spec.windwalker_monk->effectN( 13 ).percent();
      resources.base[ RESOURCE_ENERGY ] = 100;
      resources.base[ RESOURCE_ENERGY ] += talent.windwalker.ascension->effectN( 3 ).base_value();
      resources.base[ RESOURCE_ENERGY ] += talent.windwalker.inner_peace->effectN( 1 ).base_value();
      resources.base[ RESOURCE_MANA ] = 0;
      resources.base[ RESOURCE_CHI ]  = 4;
      resources.base[ RESOURCE_CHI ] += spec.windwalker_monk->effectN( 11 ).base_value();
      resources.base[ RESOURCE_CHI ] += talent.windwalker.ascension->effectN( 1 ).base_value();
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10.0;
      resources.base_regen_per_second[ RESOURCE_MANA ]   = 0;
      break;
    }
    default:
      break;
  }

  resources.base_regen_per_second[ RESOURCE_CHI ] = 0;
}

// monk_t::init_scaling =====================================================

void monk_t::init_scaling()
{
  base_t::init_scaling();

  if ( specialization() != MONK_MISTWEAVER )
  {
    scaling->disable( STAT_INTELLECT );
    scaling->disable( STAT_SPELL_POWER );
    scaling->enable( STAT_AGILITY );
    scaling->enable( STAT_WEAPON_DPS );
  }
  else
  {
    scaling->disable( STAT_AGILITY );
    scaling->disable( STAT_MASTERY_RATING );
    scaling->disable( STAT_ATTACK_POWER );
    scaling->disable( STAT_WEAPON_DPS );
    scaling->enable( STAT_INTELLECT );
    scaling->enable( STAT_SPIRIT );
  }
  scaling->disable( STAT_STRENGTH );

  if ( specialization() == MONK_WINDWALKER )
  {
    // Touch of Death
    scaling->enable( STAT_STAMINA );
  }
  if ( specialization() == MONK_BREWMASTER )
  {
    scaling->enable( STAT_BONUS_ARMOR );
  }

  if ( off_hand_weapon.type != WEAPON_NONE )
    scaling->enable( STAT_WEAPON_OFFHAND_DPS );
}

// monk_t::create_buffs =====================================================

void monk_t::create_buffs ()
{
  base_t::create_buffs ();

  auto spec_tree = specialization ();

  // General
  buff.chi_torpedo = make_buff( this, "chi_torpedo", find_spell( 119085 ) )
      ->set_trigger_spell(talent.general.chi_torpedo)
      ->set_default_value_from_effect ( 1 );

  buff.close_to_heart_driver = new buffs::close_to_heart_driver_t( *this, "close_to_heart_aura_driver", find_spell( 389684 ) );

  buff.fortifying_brew = new buffs::fortifying_brew_t( *this, "fortifying_brew", passives.fortifying_brew );

  buff.generous_pour_driver = new buffs::generous_pour_driver_t( *this, "generous_pour_aura_driver", find_spell( 389685 ) );

  buff.rushing_jade_wind = new buffs::rushing_jade_wind_buff_t ( *this, "rushing_jade_wind", shared.rushing_jade_wind );

  buff.dampen_harm = make_buff ( this, "dampen_harm", talent.general.dampen_harm );

  buff.diffuse_magic = make_buff ( this, "diffuse_magic", talent.general.diffuse_magic )
      ->set_default_value_from_effect ( 1 );

  buff.spinning_crane_kick = make_buff ( this, "spinning_crane_kick", spec.spinning_crane_kick )
    ->set_default_value_from_effect ( 2 )
    ->set_refresh_behavior ( buff_refresh_behavior::PANDEMIC );

  buff.windwalking_driver = new buffs::windwalking_driver_t( *this, "windwalking_aura_driver", find_spell( 365080 ) );

  // Brewmaster
  if ( spec_tree == MONK_BREWMASTER )
  {
    buff.bladed_armor = make_buff( this, "bladed_armor", spec.bladed_armor )
      ->set_default_value_from_effect( 1 )
      ->add_invalidate( CACHE_ATTACK_POWER );

    buff.blackout_combo = make_buff( this, "blackout_combo", talent.brewmaster.blackout_combo->effectN( 5 ).trigger() );

    buff.celestial_brew = make_buff<absorb_buff_t>( this, "celestial_brew", talent.brewmaster.celestial_brew );
    buff.celestial_brew->set_absorb_source( get_stats( "celestial_brew" ) )
        ->set_cooldown( timespan_t::zero() );

    buff.counterstrike = make_buff( this, "counterstrike", talent.brewmaster.counterstrike->effectN( 1 ).trigger() );

    buff.celestial_flames = make_buff( this, "celestial_flames", talent.brewmaster.celestial_flames->effectN( 1 ).trigger() )
      ->set_chance( talent.brewmaster.celestial_flames->proc_chance() )
      ->set_default_value( talent.brewmaster.celestial_flames->effectN( 2 ).percent() );

    buff.elusive_brawler = make_buff( this, "elusive_brawler", mastery.elusive_brawler->effectN( 3 ).trigger() )
      ->add_invalidate( CACHE_DODGE );

    buff.exploding_keg = make_buff( this, "exploding_keg", find_spell( 325153 ) )
      ->set_trigger_spell( talent.brewmaster.exploding_keg )
      ->set_default_value_from_effect( 2 );

    buff.faeline_stomp_brm = make_buff( this, "faeline_stomp_brm", passives.faeline_stomp_brm )
      ->set_trigger_spell( covenant.night_fae )
      ->set_default_value_from_effect( 1 )
      ->set_max_stack( 99 )
      ->set_stack_behavior( buff_stack_behavior::ASYNCHRONOUS )
      ->set_quiet( true );

    buff.gift_of_the_ox = new buffs::gift_of_the_ox_buff_t( *this, "gift_of_the_ox", find_spell( 124503 ) );

    buff.graceful_exit = make_buff( this, "graceful_exit", talent.brewmaster.graceful_exit->effectN( 1 ).trigger() )
     ->add_invalidate( CACHE_RUN_SPEED );

    buff.invoke_niuzao = make_buff( this, "invoke_niuzao_the_black_ox", talent.brewmaster.invoke_niuzao_the_black_ox )
      ->set_default_value_from_effect( 2 )
      ->set_cooldown( timespan_t::zero() );

    buff.hit_scheme = make_buff( this, "hit_scheme", talent.brewmaster.hit_scheme->effectN( 1 ).trigger() )
      ->set_default_value_from_effect( 1 );

    buff.pretense_of_instability = make_buff( this, "pretense_of_instability", find_spell( 393515 ) )
      ->set_trigger_spell( talent.brewmaster.pretense_of_instability )  
      ->add_invalidate( CACHE_DODGE );

    buff.purified_chi = make_buff( this, "purified_chi", find_spell( 325092 ) )
        ->set_trigger_spell( talent.brewmaster.improved_celestial_brew )
        ->set_default_value_from_effect( 1 );

    buff.shuffle = make_buff( this, "shuffle", passives.shuffle )
      ->set_trigger_spell( talent.brewmaster.shuffle )
      ->set_duration_multiplier( 3 )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION );

    buff.training_of_niuzao = make_buff( this, "training_of_niuzao", find_spell( 383733 ) )
      ->set_trigger_spell( talent.brewmaster.training_of_niuzao )
      ->set_default_value( talent.brewmaster.training_of_niuzao->effectN( 1 ).percent() )
      ->add_invalidate( CACHE_MASTERY );

    buff.light_stagger = make_buff<buffs::stagger_buff_t>( *this, "light_stagger", find_spell( 124275 ) )
        ->set_trigger_spell( talent.brewmaster.stagger );
    buff.moderate_stagger = make_buff<buffs::stagger_buff_t>( *this, "moderate_stagger", find_spell( 124274 ) )
        ->set_trigger_spell( talent.brewmaster.stagger );
    buff.heavy_stagger = make_buff<buffs::stagger_buff_t>( *this, "heavy_stagger", passives.heavy_stagger )
        ->set_trigger_spell( talent.brewmaster.stagger );
    buff.recent_purifies = new buffs::purifying_buff_t( *this, "recent_purifies", spell_data_t::nil() );
  }

  // Mistweaver
  if ( spec_tree == MONK_MISTWEAVER )
  {
    buff.channeling_soothing_mist = make_buff ( this, "channeling_soothing_mist", passives.soothing_mist_heal );

    buff.invoke_chiji = make_buff ( this, "invoke_chiji", find_spell ( 343818 ) );

    buff.invoke_chiji_evm =
      make_buff ( this, "invoke_chiji_evm", find_spell ( 343820 ) )->set_default_value_from_effect ( 1 );

    buff.life_cocoon = make_buff<absorb_buff_t> ( this, "life_cocoon", spec.life_cocoon );
    buff.life_cocoon->set_absorb_source ( get_stats ( "life_cocoon" ) )->set_cooldown ( timespan_t::zero () );

    buff.mana_tea = make_buff ( this, "mana_tea", talent.mistweaver.mana_tea )->set_default_value_from_effect ( 1 );

    buff.lifecycles_enveloping_mist =
      make_buff ( this, "lifecycles_enveloping_mist", find_spell ( 197919 ) )->set_default_value_from_effect ( 1 );

    buff.lifecycles_vivify =
      make_buff ( this, "lifecycles_vivify", find_spell ( 197916 ) )->set_default_value_from_effect ( 1 );

    buff.refreshing_jade_wind = make_buff ( this, "refreshing_jade_wind", talent.mistweaver.refreshing_jade_wind )
      ->set_refresh_behavior ( buff_refresh_behavior::PANDEMIC );

    buff.teachings_of_the_monastery =
      make_buff ( this, "teachings_of_the_monastery", find_spell ( 202090 ) )->set_default_value_from_effect ( 1 );

    buff.thunder_focus_tea =
      make_buff ( this, "thunder_focus_tea", spec.thunder_focus_tea )
      ->modify_max_stack ( (int)( talent.mistweaver.focused_thunder ? talent.mistweaver.focused_thunder->effectN ( 1 ).base_value () : 0 ) );

    buff.touch_of_death_mw = make_buff ( this, "touch_of_death_mw", find_spell ( 344361 ) )
      ->set_default_value_from_effect ( 1 )
      ->add_invalidate ( CACHE_PLAYER_DAMAGE_MULTIPLIER );

    buff.uplifting_trance = make_buff ( this, "uplifting_trance", find_spell ( 197916 ) )
      ->set_chance ( spec.renewing_mist->effectN ( 2 ).percent () )
      ->set_default_value_from_effect ( 1 );
  }

  // Windwalker
  if ( spec_tree == MONK_WINDWALKER )
  {
    buff.bok_proc = make_buff( this, "bok_proc", passives.bok_proc )
      ->set_trigger_spell( spec.combo_breaker )
      ->set_chance ( spec.combo_breaker->effectN ( 1 ).percent () );

    buff.combo_strikes = make_buff ( this, "combo_strikes" )
      ->set_trigger_spell( mastery.combo_strikes )
      ->set_duration ( timespan_t::from_minutes ( 60 ) )
      ->set_quiet ( true )  // In-game does not show this buff but I would like to use it for background stuff
      ->add_invalidate ( CACHE_PLAYER_DAMAGE_MULTIPLIER );

    buff.dance_of_chiji = make_buff ( this, "dance_of_chiji", passives.dance_of_chiji )
      ->set_trigger_spell ( talent.windwalker.dance_of_chiji );

    buff.dance_of_chiji_hidden = make_buff ( this, "dance_of_chiji_hidden" )
      ->set_trigger_spell ( talent.windwalker.dance_of_chiji )
      ->set_duration ( timespan_t::from_seconds ( 1.5 ) )
      ->set_quiet ( true )
      ->set_default_value ( talent.windwalker.dance_of_chiji->effectN ( 1 ).percent () );

    buff.flying_serpent_kick_movement = make_buff ( this, "flying_serpent_kick_movement" ) // find_spell( 115057 )
      ->set_trigger_spell( talent.windwalker.flying_serpent_kick );

    buff.fury_of_xuen_stacks = new buffs::fury_of_xuen_stacking_buff_t ( *this, "fury_of_xuen_stacks", passives.fury_of_xuen_stacking_buff );

    buff.fury_of_xuen_haste = new buffs::fury_of_xuen_haste_buff_t ( *this, "fury_of_xuen_haste", passives.fury_of_xuen_haste_buff );

    buff.hidden_masters_forbidden_touch = new buffs::hidden_masters_forbidden_touch_t (
      *this, "hidden_masters_forbidden_touch", passives.hidden_masters_forbidden_touch );

    buff.hit_combo = make_buff( this, "hit_combo", passives.hit_combo )
      ->set_trigger_spell( talent.windwalker.hit_combo )
      ->set_default_value_from_effect( 1 )
      ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

    buff.invoke_xuen =
      new buffs::invoke_xuen_the_white_tiger_buff_t ( *this, "invoke_xuen_the_white_tiger", talent.windwalker.invoke_xuen_the_white_tiger );

    buff.serenity = new buffs::serenity_buff_t ( *this, "serenity", talent.windwalker.serenity );

    buff.storm_earth_and_fire = make_buff ( this, "storm_earth_and_fire", talent.windwalker.storm_earth_and_fire )
      ->add_invalidate ( CACHE_PLAYER_DAMAGE_MULTIPLIER )
      ->add_invalidate ( CACHE_PLAYER_HEAL_MULTIPLIER )
      ->set_can_cancel ( false )  // Undocumented hotfix 2018-09-28 - SEF can no longer be canceled.
      ->set_cooldown ( timespan_t::zero () );

    buff.teachings_of_the_monastery = make_buff( this, "teachings_of_the_monastery", find_spell( 202090 ) )
      ->set_trigger_spell( shared.teachings_of_the_monastery )
      ->set_default_value_from_effect( 1 );

    buff.thunderfist = make_buff( this, "thunderfist", passives.thunderfist )
      ->set_trigger_spell( talent.windwalker.thunderfist );

    buff.touch_of_death_ww = new buffs::touch_of_death_ww_buff_t ( *this, "touch_of_death_ww", spell_data_t::nil () );

    buff.touch_of_karma = new buffs::touch_of_karma_buff_t ( *this, "touch_of_karma", find_spell ( 125174 ) );

    buff.transfer_the_power = make_buff ( this, "transfer_the_power", find_spell ( 195321 ) )
      ->set_trigger_spell( talent.windwalker.transfer_the_power )
      ->set_default_value_from_effect ( 1 );

    buff.whirling_dragon_punch = make_buff ( this, "whirling_dragon_punch", find_spell ( 196742 ) )
      ->set_trigger_spell( talent.windwalker.whirling_dragon_punch )
      ->set_refresh_behavior ( buff_refresh_behavior::NONE );

    buff.power_strikes = make_buff( this, "power_strikes", find_spell( 129914 ) )
      ->set_trigger_spell( talent.windwalker.power_strikes )
      ->set_default_value_from_effect( 1 );

    // Covenant Abilities
    buff.weapons_of_order_ww = make_buff ( this, "weapons_of_order_ww", find_spell ( 311054 ) )
      ->set_trigger_spell( covenant.kyrian )
      ->set_default_value ( find_spell ( 311054 )->effectN ( 1 ).base_value () );
  }

  buff.windwalking_venthyr = make_buff( this, "windwalking_venthyr", passives.fallen_monk_windwalking )
      ->set_trigger_spell( covenant.venthyr )
      ->set_default_value_from_effect ( 1 );

  // Covenant Conduits
  buff.fortifying_ingrediences = make_buff<absorb_buff_t>( this, "fortifying_ingredients", find_spell( 336874 ) );

  buff.fortifying_ingrediences->set_absorb_source( get_stats( "fortifying_ingredients" ) )
      ->set_trigger_spell( conduit.fortifying_ingredients )
      ->set_cooldown( timespan_t::zero() );

  // Shadowland Legendaries
  // General
  buff.charred_passions = make_buff( this, "charred_passions", find_spell( 338140 ) )
      ->set_trigger_spell( shared.charred_passions );

  buff.invokers_delight = make_buff( this, "invokers_delight", find_spell( 388663 ) )
      ->set_trigger_spell( shared.invokers_delight )
      ->set_default_value_from_effect_type( A_HASTE_ALL )
      ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
      ->add_invalidate( CACHE_ATTACK_HASTE )
      ->add_invalidate( CACHE_HASTE )
      ->add_invalidate( CACHE_SPELL_HASTE );

  // Brewmaster
  buff.mighty_pour = make_buff( this, "mighty_pour", find_spell( 337994 ) )
      ->set_trigger_spell( legendary.celestial_infusion )
      ->add_invalidate( CACHE_ARMOR );

  // Mistweaver

  // Windwalker
  buff.chi_energy = make_buff( this, "chi_energy", find_spell( 337571 ) )
      ->set_trigger_spell( shared.jade_ignition )
      ->set_default_value_from_effect( 1 );

  buff.pressure_point = make_buff( this, "pressure_point", find_spell( 337482 ) )
      ->set_trigger_spell( shared.xuens_battlegear )
      ->set_default_value_from_effect( 1 )
      ->set_refresh_behavior( buff_refresh_behavior::NONE );

  buff.the_emperors_capacitor = make_buff( this, "the_emperors_capacitor", find_spell( 337291 ) )
      ->set_trigger_spell( shared.last_emperors_capacitor )
      ->set_default_value_from_effect( 1 );

  // Covenants

  buff.bonedust_brew = make_buff( this, "bonedust_brew", find_spell( 325216 ) )
      ->set_trigger_spell( shared.bonedust_brew )
      ->set_chance( 1 )
      ->set_cooldown( timespan_t::zero() )
      ->set_default_value_from_effect( 3 );

  buff.bonedust_brew_grounding_breath_hidden = make_buff( this, "bonedust_brew_hidden", passives.bonedust_brew_grounding_breath )
      ->set_trigger_spell( conduit.grounding_breath )
      ->set_quiet( true )
      ->set_reverse( true )
      ->set_reverse_stack_count( passives.bonedust_brew_grounding_breath->max_stacks() );

  buff.bonedust_brew_attenuation_hidden = make_buff( this, "bonedust_brew_attenuation_hidden", passives.bonedust_brew_attenuation )
      ->set_trigger_spell( shared.attenuation )
      ->set_quiet( true )
      ->set_reverse( true )
      ->set_reverse_stack_count( passives.bonedust_brew_attenuation->max_stacks() );

  buff.weapons_of_order = make_buff( this, "weapons_of_order", find_spell( 310454 ) )
      ->set_trigger_spell( shared.weapons_of_order )
      ->set_default_value( find_spell( 310454 )->effectN( 1 ).base_value() )
      ->set_duration( find_spell( 310454 )->duration() +
        ( conduit.strike_with_clarity->ok() ? conduit.strike_with_clarity->effectN( 2 ).time_value()
          : timespan_t::zero() ) )
      ->set_cooldown( timespan_t::zero() )
      ->add_invalidate( CACHE_MASTERY );

  buff.invoke_xuen_call_to_arms = new buffs::call_to_arms_xuen_buff_t( *this, "invoke_xuen_call_to_arms", passives.call_to_arms_invoke_xuen );

  buff.fae_exposure = make_buff( this, "fae_exposure_heal", passives.fae_exposure_heal )
      ->set_trigger_spell( shared.faeline_stomp )
      ->set_default_value_from_effect( 1 );

  buff.faeline_stomp = make_buff( this, "faeline_stomp", find_spell( 327104 ) )
      ->set_trigger_spell( shared.faeline_stomp )
      ->set_default_value_from_effect( 2 );

  buff.faeline_stomp_reset = make_buff( this, "faeline_stomp_reset", find_spell( 327276 ) )
      ->set_trigger_spell( shared.faeline_stomp );

  buff.fallen_order = make_buff( this, "fallen_order", find_spell( 326860 ) )
      ->set_trigger_spell( covenant.venthyr )
      ->set_cooldown( timespan_t::zero() );

  // Tier 29 Set Bonus
  buff.kicks_of_flowing_momentum = make_buff( this, "kicks_of_flowing_momentum", passives.kicks_of_flowing_momentum )
      ->set_trigger_spell( sets->set( MONK_WINDWALKER, T29, B2 ) )
      ->set_default_value_from_effect( 1 )
      ->set_reverse( true )
      ->set_max_stack( sets->has_set_bonus( MONK_WINDWALKER, T29, B4 )
        ? passives.kicks_of_flowing_momentum->max_stacks() : 2 )
      ->set_reverse_stack_count( sets->has_set_bonus( MONK_WINDWALKER, T29, B4 )
        ? passives.kicks_of_flowing_momentum->max_stacks() : 2 );

  buff.fists_of_flowing_momentum = make_buff( this, "fists_of_flowing_momentum", passives.fists_of_flowing_momentum )
      ->set_trigger_spell( sets->set( MONK_WINDWALKER, T29, B4 ) )
      ->set_default_value_from_effect( 1 );

  buff.fists_of_flowing_momentum_fof = make_buff( this, "fists_of_flowing_momentum_fof", find_spell( 394951 ) )
      ->set_trigger_spell( sets->set( MONK_WINDWALKER, T29, B4 ) );

  buff.brewmasters_rhythm = make_buff( this, "brewmasters_rhythm", find_spell( 394797 ) )
      ->set_trigger_spell( sets->set( MONK_BREWMASTER, T29, B2 ) )
      ->set_default_value_from_effect( 1 )
      ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
}

// monk_t::init_gains =======================================================

void monk_t::init_gains()
{
  base_t::init_gains();

  gain.black_ox_brew_energy     = get_gain( "black_ox_brew_energy" );
  gain.bok_proc                 = get_gain( "blackout_kick_proc" );
  gain.chi_refund               = get_gain( "chi_refund" );
  gain.chi_burst                = get_gain( "chi_burst" );
  gain.crackling_jade_lightning = get_gain( "crackling_jade_lightning" );
  gain.energizing_elixir_energy = get_gain( "energizing_elixir_energy" );
  gain.energizing_elixir_chi    = get_gain( "energizing_elixir_chi" );
  gain.energy_refund            = get_gain( "energy_refund" );
  gain.expel_harm               = get_gain( "expel_harm" );
  gain.focus_of_xuen            = get_gain( "focus_of_xuen" );
  gain.gift_of_the_ox           = get_gain( "gift_of_the_ox" );
  gain.glory_of_the_dawn        = get_gain( "glory_of_the_dawn" );
  gain.rushing_jade_wind_tick   = get_gain( "rushing_jade_wind_tick" );
  gain.serenity                 = get_gain( "serenity" );
  gain.spirit_of_the_crane      = get_gain( "spirit_of_the_crane" );
  gain.tiger_palm               = get_gain( "tiger_palm" );
  gain.touch_of_death_ww        = get_gain( "touch_of_death_ww" );
  gain.power_strikes            = get_gain( "power_strikes" );
  gain.open_palm_strikes        = get_gain( "open_palm_strikes" );

  // Azerite Traits
  gain.memory_of_lucid_dreams = get_gain( "memory_of_lucid_dreams_proc" );
  gain.lucid_dreams           = get_gain( "lucid_dreams" );

  // Covenants
  gain.bonedust_brew    = get_gain( "bonedust_brew" );
  gain.weapons_of_order = get_gain( "weapons_of_order" );
}

// monk_t::init_procs =======================================================

void monk_t::init_procs()
{
  base_t::init_procs();

  proc.anvil_and_stave                     = get_proc( "Anvil & Stave" );
  proc.attenuation                         = get_proc( "Attenuation" );
  proc.blackout_combo_tiger_palm           = get_proc( "Blackout Combo - Tiger Palm" );
  proc.blackout_combo_breath_of_fire       = get_proc( "Blackout Combo - Breath of Fire" );
  proc.blackout_combo_keg_smash            = get_proc( "Blackout Combo - Keg Smash" );
  proc.blackout_combo_celestial_brew       = get_proc( "Blackout Combo - Celestial Brew" );
  proc.blackout_combo_purifying_brew       = get_proc( "Blackout Combo - Purifying Brew" );
  proc.blackout_kick_cdr_with_woo          = get_proc( "Blackout Kick CDR with WoO" );
  proc.blackout_kick_cdr                   = get_proc( "Blackout Kick CDR" );
  proc.blackout_kick_cdr_serenity_with_woo = get_proc( "Blackout Kick CDR with Serenity with WoO" );
  proc.blackout_kick_cdr_serenity          = get_proc( "Blackout Kick CDR with Serenity" );
  proc.boiling_brew_healing_sphere         = get_proc( "Boiling Brew Healing Sphere" );
  proc.bonedust_brew_reduction             = get_proc( "Bonedust Brew SCK Reduction" );
  proc.bountiful_brew_proc                 = get_proc( "Bountiful Brew Trigger" );
  proc.charred_passions_bok                = get_proc( "Charred Passions - Blackout Kick" );
  proc.charred_passions_sck                = get_proc( "Charred Passions - Spinning Crane Kick" );
  proc.chi_surge                           = get_proc( "Chi Surge CDR" );
  proc.counterstrike_tp                    = get_proc( "Counterstrike - Tiger Palm" );
  proc.counterstrike_sck                   = get_proc( "Counterstrike - Spinning Crane Kick" );
  proc.elusive_footwork_proc               = get_proc( "Elusive Footwork" );
  proc.face_palm                           = get_proc( "Face Palm" );
  proc.glory_of_the_dawn                   = get_proc( "Glory of the Dawn" );
  proc.keg_smash_scalding_brew             = get_proc( "Keg Smash - Scalding Brew" );
  proc.quick_sip                           = get_proc( "Quick Sip" );
  proc.resonant_fists                      = get_proc( "Resonant Fists" );
  proc.rsk_reset_totm                      = get_proc( "Rising Sun Kick TotM Reset" );
  proc.salsalabim_bof_reset                = get_proc( "Sal'salabim Breath of Fire Reset" );
  proc.sinister_teaching_reduction         = get_proc( "Sinister Teaching CD Reduction" );
  proc.tranquil_spirit_expel_harm          = get_proc( "Tranquil Spirit - Expel Harm" );
  proc.tranquil_spirit_goto                = get_proc( "Tranquil Spirit - Gift of the Ox" );
  proc.tumbling_technique_chi_torpedo      = get_proc( "Tumbling Technique Chi Torpedo Reset" );
  proc.tumbling_technique_roll             = get_proc( "Tumbling Technique Roll Reset" );
  proc.xuens_battlegear_reduction          = get_proc( "Xuen's Battlegear CD Reduction" );
}

// monk_t::init_assessors ===================================================

void monk_t::init_assessors()
{
  base_t::init_assessors();

  auto assessor_fn = [ this ]( result_amount_type, action_state_t* s ) {
    if ( !bugs && get_target_data( s->target )->debuff.bonedust_brew->up() )
      bonedust_brew_assessor( s );
    return assessor::CONTINUE;
  };

  assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
}

// monk_t::init_rng =======================================================

void monk_t::init_rng()
{
  player_t::init_rng();
  if ( shared.bountiful_brew && shared.bountiful_brew->ok() )
    rppm.bountiful_brew = get_rppm( "bountiful_brew", find_spell( 356592 ) );
}

// monk_t::init_special_effects ===========================================

void monk_t::init_special_effects()
{
  player_t::init_special_effects();

  // Custom trigger condition for Bron's Call to Action. Completely overrides the trigger
  // behavior of the generic proc to get control back to the Monk class module in terms
  // of what triggers it.
  //
  // 2021-07-04 Eligible spells that can proc Bron's Call to Action:
  // - Any foreground amount spell / attack
  //
  // Note, also has to handle the ICD and pet-related trigger conditions.
  callbacks.register_callback_trigger_function(
      333950, dbc_proc_callback_t::trigger_fn_type::TRIGGER,
      [ this ]( const dbc_proc_callback_t* cb, action_t* a, action_state_t* ) {
        if ( cb->cooldown->down() )
          return false;

        // Defer finding the bron pet until the first proc attempt
        if ( !pets.bron )
        {
          pets.bron = find_pet( "bron" );
          assert( pets.bron );
        }

        if ( pets.bron->is_active() )
          return false;

        switch ( a->type )
        {
          case ACTION_ATTACK:
          {
            auto attack = dynamic_cast<monk::actions::monk_melee_attack_t*>( a );
            if ( attack && attack->may_proc_bron )
            {
              attack->bron_proc->occur();
              return true;
            }
            break;
          }
          case ACTION_SPELL:
          {
            auto spell = dynamic_cast<monk::actions::monk_spell_t*>( a );
            if ( spell && spell->may_proc_bron )
            {
              spell->bron_proc->occur();
              return true;
            }
            break;
          }
          case ACTION_HEAL:
          {
            auto heal = dynamic_cast<monk::actions::monk_heal_t*>( a );
            if ( heal && heal->may_proc_bron )
            {
              heal->bron_proc->occur();
              return true;
            }
            break;
          }
          case ACTION_ABSORB:
          {
            auto absorb = dynamic_cast<monk::actions::monk_absorb_t*>( a );
            if ( absorb && absorb->may_proc_bron )
            {
              absorb->bron_proc->occur();
              return true;
            }
            break;
          }
          default:
            break;
        }

        return false;
      } );

}

// monk_t::init_special_effect ============================================

void monk_t::init_special_effect( special_effect_t& effect )
{
  switch ( effect.driver()->id() )
  {
    // Monk module has custom triggering logic (defined above) so override the initial
    // proc flags so we get wider trigger attempts than the core implementation. The
    // overridden proc condition above will take care of filtering out actions that are
    // not allowed to proc it.
    //
    // Bron's Call to Action
    case 333950:
      effect.proc_flags2_ |= PF2_CAST;
      break;
    default:
      break;
  }
}

// monk_t::reset ============================================================

void monk_t::reset()
{
  base_t::reset();

  spiritual_focus_count = 0;
  combo_strike_actions.clear();
  stagger_tick_damage.clear();
}

// monk_t::matching_gear_multiplier =========================================

double monk_t::matching_gear_multiplier( attribute_e attr ) const
{
  switch ( specialization() )
  {
    case MONK_MISTWEAVER:
      if ( attr == ATTR_INTELLECT )
        return spec.leather_specialization->effectN( 1 ).percent();
      break;
    case MONK_WINDWALKER:
      if ( attr == ATTR_AGILITY )
        return spec.leather_specialization->effectN( 1 ).percent();
      break;
    case MONK_BREWMASTER:
      if ( attr == ATTR_STAMINA )
        return spec.leather_specialization->effectN( 1 ).percent();
      break;
    default:
      break;
  }

  return 0.0;
}

// monk_t::create_storm_earth_and_fire_target_list ====================================

std::vector<player_t*> monk_t::create_storm_earth_and_fire_target_list() const
{
  // Make a copy of the non sleeping target list
  std::vector<player_t*> l = sim->target_non_sleeping_list.data();

  // Sort the list by selecting non-cyclone striked targets first, followed by ascending order of
  // the debuff remaining duration
  range::sort( l, [ this ]( player_t* l, player_t* r ) {
    auto td_left  = find_target_data( l );
    auto td_right = find_target_data( r );
    bool lcs      = td_left ? td_left->debuff.mark_of_the_crane->check() : false;
    bool rcs      = td_right ? td_right->debuff.mark_of_the_crane->check() : false;
    // Neither has cyclone strike
    if ( !lcs && !rcs )
    {
      return false;
    }
    // Left side does not have cyclone strike, right side does
    else if ( !lcs && rcs )
    {
      return true;
    }
    // Left side has cyclone strike, right side does not
    else if ( lcs && !rcs )
    {
      return false;
    }

    // Both have cyclone strike, order by remaining duration, use actor index as a tiebreaker
    timespan_t lv = td_left ? td_left->debuff.mark_of_the_crane->remains() : 0_ms;
    timespan_t rv = td_right ? td_right->debuff.mark_of_the_crane->remains() : 0_ms;
    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv < rv;
  } );

  if ( sim->debug )
  {
    sim->out_debug.print( "{} storm_earth_and_fire target list, n_targets={}", *this, l.size() );
    range::for_each( l, [ this ]( player_t* t ) {
      auto td = find_target_data( t );
      sim->out_debug.print( "{} cs={}", *t, td ? td->debuff.mark_of_the_crane->remains().total_seconds() : 0.0 );
    } );
  }

  return l;
}

// monk_t::bonedust_brew_assessor ===========================================

void monk_t::bonedust_brew_assessor(action_state_t* s)
{
    // TODO: Update Whitelist with new spells and verify with logs

    // Ignore events with 0 amount
    if (s->result_amount <= 0)
        return;

    switch (s->action->id)
    {

        // Whitelisted spells 
        // verified with logs on 08/03/2022 ( Game Version: 9.2.5 )

    case 0: // auto attack
    case 123996: // crackling_tiger_lightning_tick
    case 185099: // rising_sun_kick_dmg
    case 117418: // fists_of_fury_tick
    case 158221: // whirling_dragon_punch_tick
    case 100780: // tiger_palm
    case 100784: // blackout_kick
    case 101545: // flying_serpent_kick
    case 115129: // expel_harm_damage
    case 322109: // touch_of_death
    case 122470: // touch_of_karma
    case 107270: // spinning_crane_kick_tick
    case 261947: // fist_of_the_white_tiger_offhand
    case 261977: // fist_of_the_white_tiger_mainhand
    case 132467: // chi_wave_damage 
    case 148187: // rushing_jade_wind_tick
    case 148135: // chi_burst_damage
    case 117952: // crackling_jade_lightning
    case 196608: // eye_of_the_tiger_damage
        break;

        // Known blacklist
        // we don't need to log these

    case 325217: // bonedust_brew_dmg
    case 325218: // bonedust_brew_heal
    case 335913: // empowered_tiger_lightning
    case 360829: // empowered_tiger_lightning_call_to_arms
    case 337342: // chi_explosion
        return;

    default:

        sim->print_debug( "Bad spell passed to BDB Assessor: {}, id: {}", s->action->name(), s->action->id);
        //return;
        break; // Until whitelist is populated for 10.0 let spells that aren't blacklisted pass through 
    }

    double proc_chance = 0;
    double percent = 0;

    auto bonedust_brew = shared.bonedust_brew;
    if ( bonedust_brew && bonedust_brew->ok() )
    {
      proc_chance = bonedust_brew->proc_chance();
      percent     = bonedust_brew->effectN( 1 ).percent();
    }

    if ( rng().roll( proc_chance ) )
    {
      double damage = s->result_amount * percent;

      auto attenuation = shared.attenuation;

      if ( attenuation && attenuation->ok() )
      {
        double attenuation_bonus = conduit.bone_marrow_hops->ok() ? conduit.bone_marrow_hops.percent() : attenuation->effectN( 1 ).percent();         
        damage *= 1 + attenuation_bonus;
      }

      active_actions.bonedust_brew_dmg->base_dd_min = damage;
      active_actions.bonedust_brew_dmg->base_dd_max = damage;
      active_actions.bonedust_brew_dmg->target = s->target;
      active_actions.bonedust_brew_dmg->execute();
    }
}

// monk_t::affected_by_sef ==================================================

bool monk_t::affected_by_sef( spell_data_t data ) const
{
  // Storm, Earth, and Fire (monk_spell_t)
  bool affected = data.affected_by( talent.windwalker.storm_earth_and_fire->effectN( 1 ) );

  // Chi Explosion IS affected by SEF but needs to be overriden here manually
  if ( data.id() == 337342 )
    affected = true;

  return affected;
}

// monk_t::retarget_storm_earth_and_fire ====================================

void monk_t::retarget_storm_earth_and_fire( pet_t* pet, std::vector<player_t*>& targets ) const
{
  // Clones attack your target if there are no other targets available
  if ( targets.size() == 1 )
    pet->target = pet->owner->target;
  else
  {
    // Clones will now only re-target when you use an ability that applies Mark of the Crane, and their current target
    // already has Mark of the Crane. https://us.battle.net/forums/en/wow/topic/20752377961?page=29#post-573
    auto td = find_target_data( pet->target );

    if ( !td || !td->debuff.mark_of_the_crane->check() )
      return;

    for ( auto it = targets.begin(); it != targets.end(); ++it )
    {
      player_t* candidate_target = *it;

      // Candidate target is a valid target
      if ( *it != pet->owner->target && *it != pet->target && !candidate_target->debuffs.invulnerable->check() )
      {
        pet->target = *it;
        // This target has been chosen, so remove from the list (so that the second pet can choose something else)
        targets.erase( it );
        break;
      }
    }
  }

  range::for_each( pet->action_list,
                   [ pet ]( action_t* a ) { a->acquire_target( retarget_source::SELF_ARISE, nullptr, pet->target ); } );
}

// monk_t::has_stagger ======================================================

bool monk_t::has_stagger()
{
  return active_actions.stagger_self_damage->stagger_ticking();
}

// monk_t::partial_clear_stagger_pct ====================================================

double monk_t::partial_clear_stagger_pct( double clear_percent )
{
  return active_actions.stagger_self_damage->clear_partial_damage_pct( clear_percent );
}

// monk_t::partial_clear_stagger_amount =================================================

double monk_t::partial_clear_stagger_amount( double clear_amount )
{
  return active_actions.stagger_self_damage->clear_partial_damage_amount( clear_amount );
}

// monk_t::clear_stagger ==================================================

double monk_t::clear_stagger()
{
  return active_actions.stagger_self_damage->clear_all_damage();
}

// monk_t::stagger_total ==================================================

double monk_t::stagger_total()
{
  return active_actions.stagger_self_damage->stagger_total();
}

/**
 * Haste modifiers affecting both melee_haste and spell_haste.
 */
double shared_composite_haste_modifiers( const monk_t& p, double h )
{
  if ( p.talent.brewmaster.high_tolerance->ok() )
  {
    int effect_index = 2;  // Effect index of HT affecting each stagger buff
    for ( auto* buff :
          std::initializer_list<const buff_t*>{ p.buff.light_stagger, p.buff.moderate_stagger, p.buff.heavy_stagger } )
    {
      if ( buff && buff->check() )
      {
        h *= 1.0 / ( 1.0 + p.talent.brewmaster.high_tolerance->effectN( effect_index ).percent() );
      }
      ++effect_index;
    }
  }

  return h;
}

// Reduces Brewmaster Brew cooldowns by the time given
void monk_t::brew_cooldown_reduction( double time_reduction )
  {
    // we need to adjust the cooldown time DOWNWARD instead of UPWARD so multiply the time_reduction by -1
    time_reduction *= -1;

    if ( cooldown.purifying_brew->down() )
      cooldown.purifying_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( cooldown.celestial_brew->down() )
      cooldown.celestial_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( cooldown.fortifying_brew->down() )
      cooldown.fortifying_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( cooldown.black_ox_brew->down() )
      cooldown.black_ox_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

    if ( shared.bonedust_brew && shared.bonedust_brew->ok() && cooldown.bonedust_brew->down() )
      cooldown.bonedust_brew->adjust( timespan_t::from_seconds( time_reduction ), true );

  }

// monk_t::composite_spell_haste =========================================

double monk_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  h = shared_composite_haste_modifiers( *this, h );

  return h;
}

// monk_t::composite_melee_haste =========================================

double monk_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  h = shared_composite_haste_modifiers( *this, h );

  return h;
}

// monk_t::composite_melee_crit_chance ============================================

double monk_t::composite_melee_crit_chance() const
{
  double crit = player_t::composite_melee_crit_chance();

  crit += spec.critical_strikes->effectN( 1 ).percent();

  return crit;
}

// monk_t::composite_spell_crit_chance ============================================

double monk_t::composite_spell_crit_chance() const
{
  double crit = player_t::composite_spell_crit_chance();

  crit += spec.critical_strikes->effectN( 1 ).percent();

  return crit;
}

// monk_t::composite_attribute_multiplier =====================================

double monk_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double cam = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STAMINA && specialization() == MONK_BREWMASTER )
    cam *= 1.0 + spec.brewmasters_balance->effectN( 3 ).percent();

  return cam;
}

// monk_t::composite_melee_expertise ========================================

double monk_t::composite_melee_expertise( const weapon_t* weapon ) const
{
  double e = player_t::composite_melee_expertise( weapon );

  if ( specialization() == MONK_BREWMASTER )
    e += spec.brewmaster_monk->effectN( 15 ).percent();

  return e;
}

// monk_t::composite_melee_attack_power ==================================

double monk_t::composite_melee_attack_power() const
{
  if ( base.attack_power_per_spell_power > 0 )
    return base.attack_power_per_spell_power * composite_spell_power_multiplier() * cache.spell_power( SCHOOL_MAX );

  return player_t::composite_melee_attack_power();
}

// monk_t::composite_melee_attack_power_by_type ==================================

double monk_t::composite_melee_attack_power_by_type( attack_power_type type ) const
{
  if ( base.attack_power_per_spell_power > 0 )
    return base.attack_power_per_spell_power * composite_spell_power_multiplier() * cache.spell_power( SCHOOL_MAX );

  return player_t::composite_melee_attack_power_by_type( type );
}

// monk_t::composite_spell_power ==============================================

double monk_t::composite_spell_power( school_e school ) const
{
  if ( base.spell_power_per_attack_power > 0 )
    return base.spell_power_per_attack_power *
           composite_melee_attack_power_by_type( attack_power_type::WEAPON_MAINHAND ) *
           composite_attack_power_multiplier();

  return player_t::composite_spell_power( school );
}

// monk_t::composite_spell_power_multiplier ================================

double monk_t::composite_spell_power_multiplier() const
{
  if ( specialization() == MONK_BREWMASTER || specialization() == MONK_WINDWALKER )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// monk_t::composite_attack_power_multiplier() ==========================

double monk_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.elusive_brawler->ok() )
    ap *= 1.0 + cache.mastery() * mastery.elusive_brawler->effectN( 2 ).mastery_value();

  return ap;
}

// monk_t::composite_dodge ==============================================

double monk_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  if ( specialization() == MONK_BREWMASTER )
  {
    if ( buff.elusive_brawler->check() )
      d += buff.elusive_brawler->current_stack * cache.mastery_value();

    if ( buff.pretense_of_instability->check() )
      d += buff.pretense_of_instability->data().effectN( 1 ).percent();
  }

  if ( buff.fortifying_brew->check() && talent.general.ironshell_brew->ok() )
    d += talent.general.ironshell_brew->effectN( 1 ).percent();

  return d;
}

// monk_t::composite_crit_avoidance =====================================

double monk_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  if ( specialization() == MONK_BREWMASTER )
    c += spec.brewmaster_monk->effectN( 13 ).percent();

  return c;
}

// monk_t::composite_mastery ===========================================

double monk_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  if ( buff.weapons_of_order->check() )
  {
    m += buff.weapons_of_order->check_value();
    if ( conduit.strike_with_clarity->ok() )
      m += conduit.strike_with_clarity.value();
  }

  if ( specialization() == MONK_BREWMASTER )
  {
    if ( buff.training_of_niuzao->check() )
      m += buff.training_of_niuzao->check_value();
  }

  return m;
}

// monk_t::composite_mastery_rating ====================================

double monk_t::composite_mastery_rating() const
{
  double m = player_t::composite_mastery_rating();

  return m;
}

// monk_t::composite_damage_versatility ====================================

double monk_t::composite_damage_versatility() const
{
  double m = player_t::composite_damage_versatility();

  return m;
}

// monk_t::composite_armor_multiplier ===================================

double monk_t::composite_base_armor_multiplier() const
{
  double a = player_t::composite_base_armor_multiplier();

  if ( specialization() == MONK_BREWMASTER )
    a *= 1 + spec.brewmasters_balance->effectN( 1 ).percent();

  if ( buff.mighty_pour->check() )
    a *= 1 + buff.mighty_pour->data().effectN( 1 ).percent();

  if ( buff.fortifying_brew->check() && talent.general.ironshell_brew->ok() )
    a *= 1 + talent.general.ironshell_brew->effectN( 2 ).percent();

  return a;
}

// monk_t::temporary_movement_modifier =====================================

double monk_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

  if ( buff.chi_torpedo->check() )
    active = std::max( buff.chi_torpedo->check_stack_value(), active );

  if ( specialization() == MONK_WINDWALKER )
  {
    if ( buff.flying_serpent_kick_movement->check() )
      active = std::max( buff.flying_serpent_kick_movement->check_value(), active );
  }

  return active;
}

// monk_t::composite_player_dd_multiplier ================================
double monk_t::composite_player_dd_multiplier( school_e school, const action_t* action ) const
{
  double multiplier = player_t::composite_player_dd_multiplier( school, action );

  if ( specialization() == MONK_WINDWALKER && buff.hit_combo->check() &&
       action->data().affected_by( passives.hit_combo->effectN( 1 ) ) )
    multiplier *= 1 + buff.hit_combo->check() * passives.hit_combo->effectN( 1 ).percent();

  return multiplier;
}

// monk_t::composite_player_td_multiplier ================================
double monk_t::composite_player_td_multiplier( school_e school, const action_t* action ) const
{
  double multiplier = player_t::composite_player_td_multiplier( school, action );

  if ( specialization() == MONK_WINDWALKER && buff.hit_combo->check() &&
       action->data().affected_by( passives.hit_combo->effectN( 2 ) ) )
    multiplier *= 1 + buff.hit_combo->check() * passives.hit_combo->effectN( 2 ).percent();

  return multiplier;
}

// monk_t::composite_player_target_multiplier ============================
double monk_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double multiplier = player_t::composite_player_target_multiplier( target, school );

  auto td = find_target_data( target );
  if ( td && td->debuff.fae_exposure->check() )
    multiplier *= 1 + passives.fae_exposure_dmg->effectN( 1 ).percent();

  if ( td && td->debuff.weapons_of_order->check() )
    multiplier *= 1 + td->debuff.weapons_of_order->check_stack_value();

  return multiplier;
}

// monk_t::composite_player_pet_damage_multiplier ========================
double monk_t::composite_player_pet_damage_multiplier( const action_state_t* state, bool guardian ) const
{
  double multiplier = player_t::composite_player_pet_damage_multiplier( state, guardian );

  if ( specialization() == MONK_WINDWALKER && buff.hit_combo->check() )
    multiplier *= 1 + buff.hit_combo->check() * passives.hit_combo->effectN( 4 ).percent();

  if ( specialization() == MONK_BREWMASTER )
    multiplier *= 1 + spec.brewmaster_monk->effectN( 3 ).percent();

  return multiplier;
}

// monk_t::composite_player_target_pet_damage_multiplier ========================
double monk_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double multiplier = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  auto td = find_target_data( target );
  if ( td && td->debuff.weapons_of_order->check() )
  {
    if ( guardian )
      multiplier *=
          1 + ( td->debuff.weapons_of_order->check() * td->debuff.weapons_of_order->data().effectN( 3 ).percent() );
    else
      multiplier *=
          1 + ( td->debuff.weapons_of_order->check() * td->debuff.weapons_of_order->data().effectN( 2 ).percent() );
  }

  if ( td && td->debuff.fae_exposure->check() )
  {
    if ( guardian )
      multiplier *= 1 + passives.fae_exposure_dmg->effectN( 3 ).percent();
    else
      multiplier *= 1 + passives.fae_exposure_dmg->effectN( 2 ).percent();
  }

  return multiplier;
}

// monk_t::invalidate_cache ==============================================

void monk_t::invalidate_cache( cache_e c )
{
  base_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_ATTACK_POWER:
    case CACHE_AGILITY:
      if ( specialization() == MONK_BREWMASTER || specialization() == MONK_WINDWALKER )
        player_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_SPELL_POWER:
    case CACHE_INTELLECT:
      if ( specialization() == MONK_MISTWEAVER )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    case CACHE_BONUS_ARMOR:
      if ( spec.bladed_armor->ok() )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    case CACHE_MASTERY:
      if ( specialization() == MONK_WINDWALKER )
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      else if ( specialization() == MONK_BREWMASTER )
      {
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
        player_t::invalidate_cache( CACHE_SPELL_POWER );
        player_t::invalidate_cache( CACHE_DODGE );
      }
      break;
    default:
      break;
  }
}

// monk_t::create_options ===================================================

void monk_t::create_options()
{
  base_t::create_options();

  add_option( opt_int( "monk.initial_chi", user_options.initial_chi, 0, 6 ) );
  add_option( opt_float( "monk.expel_harm_effectiveness", user_options.expel_harm_effectiveness, 0.0, 1.0 ) );
  add_option( opt_float( "monk.faeline_stomp_uptime", user_options.faeline_stomp_uptime, 0.0, 1.0 ) );
  add_option( opt_int( "monk.chi_burst_healing_targets", user_options.chi_burst_healing_targets, 0, 30 ) );
  add_option( opt_int( "monk.motc_override", user_options.motc_override, 0, 5 ) );
  add_option( opt_int( "monk.no_bof_dot", user_options.no_bof_dot, 0, 1 ) );
}

// monk_t::copy_from =========================================================

void monk_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  auto* source_p = debug_cast<monk_t*>( source );

  user_options = source_p->user_options;
}

// monk_t::primary_resource =================================================

resource_e monk_t::primary_resource() const
{
  if ( specialization() == MONK_MISTWEAVER )
    return RESOURCE_MANA;

  return RESOURCE_ENERGY;
}

// monk_t::primary_role =====================================================

role_e monk_t::primary_role() const
{
  if ( base_t::primary_role() == role_e::ROLE_DPS )
    return ROLE_HYBRID;

  if ( base_t::primary_role() == role_e::ROLE_TANK )
    return ROLE_TANK;

  if ( base_t::primary_role() == role_e::ROLE_HEAL )
    return ROLE_HYBRID;  // To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == specialization_e::MONK_BREWMASTER )
    return ROLE_TANK;

  if ( specialization() == specialization_e::MONK_MISTWEAVER )
    return ROLE_ATTACK;  // To prevent spawning healing_target, as there is no support for healing.

  if ( specialization() == specialization_e::MONK_WINDWALKER )
    return ROLE_DPS;

  return ROLE_HYBRID;
}

// monk_t::convert_hybrid_stat ==============================================

stat_e monk_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    case STAT_STR_AGI_INT:
      switch ( specialization() )
      {
        case MONK_MISTWEAVER:
          return STAT_INTELLECT;
        case MONK_BREWMASTER:
        case MONK_WINDWALKER:
          return STAT_AGILITY;
        default:
          return STAT_NONE;
      }
    case STAT_AGI_INT:
      if ( specialization() == MONK_MISTWEAVER )
        return STAT_INTELLECT;
      else
        return STAT_AGILITY;
    case STAT_STR_AGI:
      return STAT_AGILITY;
    case STAT_STR_INT:
      return STAT_INTELLECT;
    case STAT_SPIRIT:
      if ( specialization() == MONK_MISTWEAVER )
        return s;
      else
        return STAT_NONE;
    case STAT_BONUS_ARMOR:
      if ( specialization() == MONK_BREWMASTER )
        return s;
      else
        return STAT_NONE;
    default:
      return s;
  }
}

// monk_t::pre_analyze_hook  ================================================

void monk_t::pre_analyze_hook()
{
  sample_datas.stagger_effective_damage_timeline.adjust( *sim );
  sample_datas.stagger_damage_pct_timeline.adjust( *sim );
  sample_datas.stagger_pct_timeline.adjust( *sim );

  base_t::pre_analyze_hook();
}

// monk_t::energy_regen_per_second ==========================================

double monk_t::resource_regen_per_second( resource_e r ) const
{
  double reg = base_t::resource_regen_per_second( r );

  if ( r == RESOURCE_ENERGY )
  {
    if ( specialization() == MONK_WINDWALKER )
    {
      reg *= 1.0 + talent.windwalker.ascension->effectN( 2 ).percent();
    }
  }
  else if ( r == RESOURCE_MANA )
  {
    if ( specialization() == MONK_MISTWEAVER )
    {
      reg *= 1.0 + spec.mistweaver_monk->effectN( 8 ).percent();
    }
  }

  // Memory of Lucid Dreams
  if ( player_t::buffs.memory_of_lucid_dreams->check() )
    reg *= 1.0 + player_t::buffs.memory_of_lucid_dreams->data().effectN( 1 ).percent();

  return reg;
}

// monk_t::combat_begin ====================================================

void monk_t::combat_begin()
{
  base_t::combat_begin();
  
  if ( talent.general.close_to_heart->ok() )
  {
    if ( sim->distance_targeting_enabled )
    {
      buff.close_to_heart_driver->trigger();
    }
    else
    {
      buffs.close_to_heart_leech_aura->trigger( 1, buffs.close_to_heart_leech_aura->data().effectN( 1 ).percent(), 1,
                                                timespan_t::zero() );
    }
  }

   if ( talent.general.generous_pour->ok() )
  {
    if ( sim->distance_targeting_enabled )
    {
      buff.generous_pour_driver->trigger();
    }
    else
    {
      buffs.generous_pour_avoidance_aura->trigger( 1, buffs.generous_pour_avoidance_aura->data().effectN( 1 ).percent(), 1,
                                                timespan_t::zero() );
    }
  }

  if ( talent.general.windwalking->ok() )
  {
    if ( sim->distance_targeting_enabled )
    {
      buff.windwalking_driver->trigger();
    }
    else
    {
      buffs.windwalking_movement_aura->trigger( 1, buffs.windwalking_movement_aura->data().effectN( 1 ).percent(), 1,
                                                timespan_t::zero() );
    }
  }
 
  if ( specialization() == MONK_WINDWALKER )
  {
    if ( user_options.initial_chi > 0 )
    {
      resources.current[RESOURCE_CHI] =
        clamp( as<double>( user_options.initial_chi + resources.current[RESOURCE_CHI] ), 0.0,
          resources.max[RESOURCE_CHI] );
      sim->print_debug( "Combat starting chi has been set to {}", resources.current[RESOURCE_CHI] );
    }

    if (talent.windwalker.power_strikes->ok())
      make_repeating_event( sim, talent.windwalker.power_strikes->effectN( 2 ).period(),
                            [ this ]() { buff.power_strikes->trigger(); } );
  }

  if ( specialization () == MONK_BREWMASTER )
  {
    if ( spec.bladed_armor->ok () )
      buff.bladed_armor->trigger ();
  }

}

// monk_t::assess_damage ====================================================

void monk_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  buff.fortifying_brew->up();
  if ( specialization() == MONK_BREWMASTER )
  {
    if ( s->result == RESULT_DODGE )
    {
      if ( buff.elusive_brawler->up() )
        buff.elusive_brawler->expire();

      if ( talent.brewmaster.counterstrike->ok() && cooldown.counterstrike->up() )
      {
        buff.counterstrike->trigger();
        cooldown.counterstrike->start( talent.brewmaster.counterstrike->internal_cooldown() );
      }

      // Saved as 5/10 base values but need it as 0.5 and 1 base values
      if ( talent.brewmaster.anvil_and_stave->ok() && cooldown.anvil_and_stave->up() )
      {
        cooldown.anvil_and_stave->start( talent.brewmaster.anvil_and_stave->internal_cooldown() );
        proc.anvil_and_stave->occur();
        brew_cooldown_reduction( talent.brewmaster.anvil_and_stave->effectN( 1 ).base_value() / 10 );
      }

      if ( talent.brewmaster.graceful_exit->ok() )
        buff.graceful_exit->trigger();
    }
    if ( s->result == RESULT_MISS )
    {
      if ( talent.brewmaster.counterstrike->ok() && cooldown.counterstrike->up() )
      {
        buff.counterstrike->trigger();
        cooldown.counterstrike->start( talent.brewmaster.counterstrike->internal_cooldown() );
      }

      if ( talent.brewmaster.graceful_exit->ok() )
        buff.graceful_exit->trigger();
    }
  }

  if ( action_t::result_is_hit( s->result ) && s->action->id != passives.stagger_self_damage->id() )
  {
    // trigger the mastery if the player gets hit by a physical attack; but not from stagger
    if ( school == SCHOOL_PHYSICAL )
      buff.elusive_brawler->trigger();
  }

  base_t::assess_damage( school, dtype, s );
}

// monk_t::target_mitigation ====================================================

void monk_t::target_mitigation( school_e school, result_amount_type dt, action_state_t* s )
{
  auto target_data = get_target_data( s->action->player );

  // Gift of the Ox Trigger Calculations ===========================================================

  // Gift of the Ox is no longer a random chance, under the hood. When you are hit, it increments a counter by
  // (DamageTakenBeforeAbsorbsOrStagger / MaxHealth). It now drops an orb whenever that reaches 1.0, and decrements it
  // by 1.0. The tooltip still says ‘chance’, to keep it understandable.
  if ( s->action->id != passives.stagger_self_damage->id() )
  {
    double goto_proc_chance = s->result_amount / max_health();

    gift_of_the_ox_proc_chance += goto_proc_chance;

    if ( gift_of_the_ox_proc_chance > 1.0 )
    {
      buff.gift_of_the_ox->trigger();

      gift_of_the_ox_proc_chance -= 1.0;
    }
  }

  // Dampen Harm // Reduces hits by 20 - 50% based on the size of the hit
  // Works on Stagger
  if ( buff.dampen_harm->up() )
  {
    double dampen_max_percent = buff.dampen_harm->data().effectN( 3 ).percent();
    if ( s->result_amount >= max_health() )
      s->result_amount *= 1 - dampen_max_percent;
    else
    {
      double dampen_min_percent = buff.dampen_harm->data().effectN( 2 ).percent();
      s->result_amount *= 1 - ( dampen_min_percent +
                                ( ( dampen_max_percent - dampen_min_percent ) * ( s->result_amount / max_health() ) ) );
    }
  }

  // Stagger is not reduced by damage mitigation effects
  if ( s->action->id == passives.stagger_self_damage->id() )
  {
    return;
  }

  if ( spec.brewmasters_balance->ok() )
  {
    s->result_amount *= 1.0 + spec.brewmasters_balance->effectN( 2 ).percent();
  }

  // Calming Presence talent
  if ( talent.general.calming_presence->ok() )
    s->result_amount *= 1.0 + talent.general.calming_presence->effectN( 1 ).percent();

  // Diffuse Magic
  if ( buff.diffuse_magic->up() && school != SCHOOL_PHYSICAL )
    s->result_amount *= 1.0 + buff.diffuse_magic->default_value;  // Stored as -60%

  // If Breath of Fire is ticking on the source target, the player receives 5% less damage
  if ( target_data->dots.breath_of_fire->is_ticking() )
  {
    // Saved as -5
    double dmg_reduction = passives.breath_of_fire_dot->effectN( 2 ).percent();

    if ( buff.celestial_flames->up() )
      dmg_reduction -= buff.celestial_flames->value();  // Saved as 5
    s->result_amount *= 1.0 + dmg_reduction;
  }

  // Damage Reduction Cooldowns
  if ( talent.general.fortifying_brew->ok() && buff.fortifying_brew->up() )
  {
    double reduction = spec.fortifying_brew->effectN( 2 ).percent();  // Saved as -20%

    s->result_amount *= ( 1.0 + reduction );
  }

  if ( buff.brewmasters_rhythm->up() )
  {
    auto damage_reduction = 1 + ( buff.brewmasters_rhythm->stack() *
                                  buff.brewmasters_rhythm->data().effectN( 2 ).percent() );  // Saved as -1
    s->result_amount *= damage_reduction;
  }

  // Touch of Karma Absorbtion
  if ( talent.windwalker.touch_of_karma->ok() && buff.touch_of_karma->up() )
  {
    double percent_HP = talent.windwalker.touch_of_karma->effectN( 3 ).percent() * max_health();
    if ( ( buff.touch_of_karma->value() + s->result_amount ) >= percent_HP )
    {
      double difference = percent_HP - buff.touch_of_karma->value();
      buff.touch_of_karma->current_value += difference;
      s->result_amount -= difference;
      buff.touch_of_karma->expire();
    }
    else
    {
      buff.touch_of_karma->current_value += s->result_amount;
      s->result_amount = 0;
    }
  }

  player_t::target_mitigation( school, dt, s );
}

// monk_t::assess_damage_imminent_pre_absorb ==============================

void monk_t::assess_damage_imminent_pre_absorb( school_e school, result_amount_type dtype, action_state_t* s )
{
  base_t::assess_damage_imminent_pre_absorb( school, dtype, s );

  if ( specialization() == MONK_BREWMASTER )
  {
    // Stagger damage can't be staggered!
    if ( s->action->id == passives.stagger_self_damage->id() )
      return;

    // Stagger Calculation
    double stagger_dmg = 0;

    if ( s->result_amount > 0 )
    {
      if ( school == SCHOOL_PHYSICAL )
        stagger_dmg += s->result_amount * stagger_pct( s->target->level() );

      else if ( spec.stagger_2->ok() && school != SCHOOL_PHYSICAL )
      {
        double stagger_magic = stagger_pct( s->target->level() ) * spec.stagger_2->effectN( 1 ).percent();

        stagger_dmg += s->result_amount * stagger_magic;
      }

      s->result_amount -= stagger_dmg;
      s->result_mitigated -= stagger_dmg;
    }
    // Hook up Stagger Mechanism
    if ( stagger_dmg > 0 )
    {
      // Blizzard is putting a cap on how much damage can go into stagger
      double amount_remains = active_actions.stagger_self_damage->amount_remaining();
      double cap            = max_health() * talent.brewmaster.stagger->effectN( 4 ).percent();
      if ( amount_remains + stagger_dmg >= cap )
      {
        double diff = ( amount_remains + stagger_dmg ) - cap;
        s->result_amount += std::fmax( stagger_dmg - diff, 0 );
        s->result_mitigated += std::fmax( stagger_dmg - diff, 0 );
        stagger_dmg = std::fmax( stagger_dmg - diff, 0 );
      }
      sample_datas.stagger_total_damage->add( stagger_dmg );
      residual_action::trigger( active_actions.stagger_self_damage, this, stagger_dmg );
    }
  }
}

// monk_t::assess_heal ===================================================

void monk_t::assess_heal( school_e school, result_amount_type dmg_type, action_state_t* s )
{
  player_t::assess_heal( school, dmg_type, s );

  if ( specialization() == MONK_BREWMASTER )
    trigger_celestial_fortune( s );
}

// =========================================================================
// Monk APL
// =========================================================================

// monk_t::default_flask ===================================================
std::string monk_t::default_flask() const
{
  return monk_apl::flask( this );
}

// monk_t::default_potion ==================================================
std::string monk_t::default_potion() const
{
  return monk_apl::potion( this );
}

// monk_t::default_food ====================================================
std::string monk_t::default_food() const
{
  return monk_apl::food( this );
}

// monk_t::default_rune ====================================================
std::string monk_t::default_rune() const
{
  return monk_apl::rune( this );
}

// monk_t::temporary_enchant ===============================================
std::string monk_t::default_temporary_enchant() const
{
  return monk_apl::temporary_enchant( this );
}

// monk_t::init_actions =====================================================

void monk_t::init_action_list()
{
  // Mistweaver isn't supported atm
  if ( !sim->allow_experimental_specializations && specialization() == MONK_MISTWEAVER && role != ROLE_ATTACK )
  {
    if ( !quiet )
      sim->error( "Monk mistweaver healing for {} is not currently supported.", *this );

    quiet = true;
    return;
  }
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    if ( !quiet )
      sim->error( "{} has no weapon equipped at the Main-Hand slot.", *this );
    quiet = true;
    return;
  }
  if ( main_hand_weapon.group() == WEAPON_2H && off_hand_weapon.group() == WEAPON_1H )
  {
    if ( !quiet )
      sim->error( "{} has a 1-Hand weapon equipped in the Off-Hand while a 2-Hand weapon is equipped in the Main-Hand.",
                  *this );
    quiet = true;
    return;
  }
  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  // Combat
  switch ( specialization() )
  {
    case MONK_BREWMASTER:
      monk_apl::brewmaster( this );
      break;
    case MONK_WINDWALKER:
      monk_apl::windwalker( this );
      break;
    case MONK_MISTWEAVER:
      monk_apl::mistweaver( this );
      break;
    default:
      monk_apl::no_spec( this );
      break;
  }
  use_default_action_list = true;

  base_t::init_action_list();
}

double monk_t::stagger_base_value()
{
  double stagger_base = 0.0;

  if ( specialization() == MONK_BREWMASTER )  // no stagger when not in Brewmaster Specialization
  {
    stagger_base = agility() * talent.brewmaster.stagger->effectN( 1 ).percent();

    if ( talent.brewmaster.high_tolerance->ok() )
      stagger_base *= 1 + talent.brewmaster.high_tolerance->effectN( 5 ).percent();

    if ( talent.brewmaster.fortifying_brew_determination->ok() && buff.fortifying_brew->up() )
      stagger_base *= 1 + passives.fortifying_brew->effectN( 6 ).percent();

    // Hard coding the 125% multiplier until Blizzard fixes this
    if ( buff.faeline_stomp_brm->up() )
      stagger_base *= 1.0 + 0.25;

    if ( buff.shuffle->check() )
      stagger_base *= 1.0 + passives.shuffle->effectN( 1 ).percent();
  }

  return stagger_base;
}

/**
 * BFA stagger formula
 *
 * See https://us.battle.net/forums/en/wow/topic/20765536748#post-10
 * or http://blog.askmrrobot.com/diminishing-returns-other-bfa-tank-formulas/
 */
double monk_t::stagger_pct( int target_level )
{
  double stagger_base = stagger_base_value();
  // TODO: somehow pull this from "enemy_t::armor_coefficient( target_level, tank_dummy_e::MYTHIC )" without crashing
  double k            = dbc->armor_mitigation_constant( target_level );
  k *= ( is_ptr() ? 1.384 : 1.992 );  // Mythic Raid

  double stagger = stagger_base / ( stagger_base + k );

  return std::min( stagger, 0.99 );
}

// monk_t::current_stagger_tick_dmg ==================================================

double monk_t::current_stagger_tick_dmg()
{
  double dmg = 0;
  if ( active_actions.stagger_self_damage )
    dmg = active_actions.stagger_self_damage->tick_amount();

  if ( buff.invoke_niuzao->up() )
    dmg *= 1 - buff.invoke_niuzao->value();  // Saved as 25%

  return dmg;
}

void monk_t::stagger_damage_changed( bool last_tick )
{
  buff_t* previous_buff = nullptr;
  for ( auto* b : std::initializer_list<buff_t*>{ buff.light_stagger, buff.moderate_stagger, buff.heavy_stagger } )
  {
    if ( b->check() )
    {
      previous_buff = b;
      break;
    }
  }
  sim->print_debug( "Previous stagger buff was {}.", previous_buff ? previous_buff->name() : "none" );

  buff_t* new_buff = nullptr;
  dot_t* dot       = nullptr;
  int niuzao       = 0;
  if ( active_actions.stagger_self_damage )
    dot = active_actions.stagger_self_damage->get_dot();
  if ( !last_tick && dot && dot->is_ticking() )  // fake dot not active on last tick
  {
    auto current_tick_dmg                = current_stagger_tick_dmg();
    auto current_tick_dmg_per_max_health = current_stagger_tick_dmg_percent();
    sim->print_debug( "Stagger dmg: {} ({}%):", current_tick_dmg, current_tick_dmg_per_max_health * 100.0 );
    if ( current_tick_dmg_per_max_health > 0.045 )
    {
      new_buff = buff.heavy_stagger;
      niuzao   = 3;
    }
    else if ( current_tick_dmg_per_max_health > 0.03 )
    {
      new_buff = buff.moderate_stagger;
      niuzao   = 2;
    }
    else if ( current_tick_dmg_per_max_health > 0.0 )
    {
      new_buff = buff.light_stagger;
      niuzao   = 1;
    }
  }
  sim->print_debug( "Stagger new buff is {}.", new_buff ? new_buff->name() : "none" );

  if ( previous_buff && previous_buff != new_buff )
  {
    previous_buff->expire();
    if ( buff.training_of_niuzao->check() )
      buff.training_of_niuzao->expire();
  }
  if ( new_buff && previous_buff != new_buff )
  {
    new_buff->trigger();
    if ( talent.brewmaster.training_of_niuzao.ok() )
      buff.training_of_niuzao->trigger( 1, niuzao * talent.brewmaster.training_of_niuzao->effectN( 1 ).percent(), -1, timespan_t::min() );
  }
}

// monk_t::current_stagger_total ==================================================

double monk_t::current_stagger_amount_remains()
{
  double dmg = 0;
  if ( active_actions.stagger_self_damage )
    dmg = active_actions.stagger_self_damage->amount_remaining();
  return dmg;
}

// monk_t::current_stagger_amount_remains_to_total_percent ==========================================

double monk_t::current_stagger_amount_remains_to_total_percent()
{
  return active_actions.stagger_self_damage->amount_remaining_to_total();
}

// monk_t::current_stagger_dmg_percent ==================================================

double monk_t::current_stagger_tick_dmg_percent()
{
  return ( current_stagger_tick_dmg() / resources.max[ RESOURCE_HEALTH ] );
}

double monk_t::current_stagger_amount_remains_percent()
{
  return ( current_stagger_amount_remains() / resources.max[ RESOURCE_HEALTH ] );
}

// monk_t::current_stagger_dot_duration ==================================================

timespan_t monk_t::current_stagger_dot_remains()
{
  if ( active_actions.stagger_self_damage )
  {
    dot_t* dot = active_actions.stagger_self_damage->get_dot();

    return dot->remains();
  }

  return timespan_t::zero();
}

/**
 * Accumulated stagger tick damage of the last n ticks.
 */
double monk_t::calculate_last_stagger_tick_damage( int n ) const
{
  double amount = 0.0;

  assert( n > 0 );

  for ( size_t i = stagger_tick_damage.size(), j = n; i-- && j--; )
  {
    amount += stagger_tick_damage[ i ].value;
  }

  return amount;
}

void monk_t::trigger_empowered_tiger_lightning( action_state_t* s, bool trigger_invoke_xuen, bool trigger_call_to_arms )
{
  if ( specialization() != MONK_WINDWALKER )
    return;

  if ( talent.windwalker.empowered_tiger_lightning->ok() )
  {
    if ( !s->action->harmful )
      return;

    // Touch of Karma (id = 124280) does not contribute to Empowered Tiger Lightning
    // Bonedust Brew (id = 325217) does not contribute to Empowered Tiger Lightning
    if ( s->result_amount <= 0 || s->action->id == 124280 || s->action->id == 325217 )
      return;

    // Make sure Xuen is up and the action is not the Empowered Tiger Lightning itself (335913 & 360829)
    if ( s->action->id == 335913 || s->action->id == 360829 )
      return;

    if ( buff.invoke_xuen->check() && trigger_invoke_xuen )
    {
      auto td = get_target_data( s->target );

      if ( td->debuff.empowered_tiger_lightning->check() )
      {
        td->debuff.empowered_tiger_lightning->current_value += s->result_amount;
      }
      else
      {
        td->debuff.empowered_tiger_lightning->trigger( -1, s->result_amount, -1, buff.invoke_xuen->remains() );
      }
    }

    if ( buff.invoke_xuen_call_to_arms->check() && trigger_call_to_arms )
    {
      auto td = get_target_data( s->target );

      if ( td->debuff.call_to_arms_empowered_tiger_lightning->check() )
      {
        td->debuff.call_to_arms_empowered_tiger_lightning->current_value += s->result_amount;
      }
      else
      {
        td->debuff.call_to_arms_empowered_tiger_lightning->trigger( -1, s->result_amount, -1,
                                                                    buff.invoke_xuen_call_to_arms->remains() );
      }
    }

    if ( buff.fury_of_xuen_haste->check() )
    {
      auto td = get_target_data( s->target );

      if ( td->debuff.fury_of_xuen_empowered_tiger_lightning->check() )
      {
        td->debuff.fury_of_xuen_empowered_tiger_lightning->current_value += s->result_amount;
      }
      else
      {
        td->debuff.fury_of_xuen_empowered_tiger_lightning->trigger( -1, s->result_amount, -1,
                                                                    buff.invoke_xuen_call_to_arms->remains() );
      }
    }
  }
}

// monk_t::create_expression ==================================================

std::unique_ptr<expr_t> monk_t::create_expression( util::string_view name_str )
{
  auto splits = util::string_split<util::string_view>( name_str, "." );
  if ( splits.size() == 2 && splits[ 0 ] == "stagger" )
  {
    auto create_stagger_threshold_expression = []( util::string_view name_str, monk_t* p, double stagger_health_pct ) {
      return make_fn_expr(
          name_str, [ p, stagger_health_pct ] { return p->current_stagger_tick_dmg_percent() > stagger_health_pct; } );
    };

    if ( splits[ 1 ] == "light" )
    {
      return create_stagger_threshold_expression( name_str, this, light_stagger_threshold );
    }
    else if ( splits[ 1 ] == "moderate" )
    {
      return create_stagger_threshold_expression( name_str, this, moderate_stagger_threshold );
    }
    else if ( splits[ 1 ] == "heavy" )
    {
      return create_stagger_threshold_expression( name_str, this, heavy_stagger_threshold );
    }
    else if ( splits[ 1 ] == "amount" )
    {
      // WoW API has this as the 16th node from UnitDebuff
      return make_fn_expr( name_str, [ this ] { return current_stagger_tick_dmg(); } );
    }
    else if ( splits[ 1 ] == "pct" )
    {
      return make_fn_expr( name_str, [ this ] { return current_stagger_tick_dmg_percent() * 100; } );
    }
    else if ( splits[ 1 ] == "amounttototalpct" )
    {
      // This is the current stagger amount remaining compared to the total amount of the stagger dot
      return make_fn_expr( name_str, [ this ] { return current_stagger_amount_remains_to_total_percent() * 100; } );
    }
    else if ( splits[ 1 ] == "remains" )
    {
      return make_fn_expr( name_str, [ this ]() { return current_stagger_dot_remains(); } );
    }
    else if ( splits[ 1 ] == "amount_remains" )
    {
      return make_fn_expr( name_str, [ this ]() { return current_stagger_amount_remains(); } );
    }
    else if ( splits[ 1 ] == "ticking" )
    {
      return make_fn_expr( name_str, [ this ]() { return has_stagger(); } );
    }

    if ( util::str_in_str_ci( splits[ 1 ], "last_tick_damage_" ) )
    {
      auto parts = util::string_split<util::string_view>( splits[ 1 ], "_" );
      int n      = util::to_int( parts.back() );

      // skip construction if the duration is nonsensical
      if ( n > 0 )
      {
        return make_fn_expr( name_str, [ this, n ] { return calculate_last_stagger_tick_damage( n ); } );
      }
      else
      {
        throw std::invalid_argument( fmt::format( "Non-positive number of last stagger ticks '{}'.", n ) );
      }
    }
  }

  else if ( splits.size() == 2 && splits[ 0 ] == "spinning_crane_kick" )
  {
    if ( splits[1] == "count" )
      return make_fn_expr( name_str, [ this ] { return mark_of_the_crane_counter(); } );
    else if ( splits[1] == "modifier" )
      return make_fn_expr( name_str, [ this ] { return sck_modifier(); } );
    else if ( splits[1] == "max" )
      return make_fn_expr( name_str, [ this ] { return mark_of_the_crane_max(); } );
  }
  else if ( splits.size() == 1 && splits[ 0 ] == "no_bof_dot" )
    return make_fn_expr( name_str, [ this ] { return user_options.no_bof_dot; } );

  return base_t::create_expression( name_str );
}

void monk_t::apply_affecting_auras ( action_t& action )
{
  player_t::apply_affecting_auras ( action );

  action.apply_affecting_aura ( passives.aura_monk );

  switch ( specialization () )
  {
    case MONK_BREWMASTER:
      action.apply_affecting_aura ( spec.brewmaster_monk );
      break;
    case MONK_WINDWALKER:
      action.apply_affecting_aura ( spec.windwalker_monk );
      break;
    case MONK_MISTWEAVER:
      action.apply_affecting_aura ( spec.mistweaver_monk );
      break;
    default:
      assert( 0 );
      break;
}

  if ( main_hand_weapon.group() == weapon_e::WEAPON_1H )
  {
    action.apply_affecting_aura( spec.two_hand_adjustment );
  }
}

void monk_t::merge( player_t& other )
{
  base_t::merge( other );

  auto& other_monk = static_cast<const monk_t&>( other );

  sample_datas.stagger_effective_damage_timeline.merge( other_monk.sample_datas.stagger_effective_damage_timeline );
  sample_datas.stagger_damage_pct_timeline.merge( other_monk.sample_datas.stagger_damage_pct_timeline );
  sample_datas.stagger_pct_timeline.merge( other_monk.sample_datas.stagger_pct_timeline );
}

// monk_t::monk_report =================================================

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class monk_report_t : public player_report_extension_t
{
public:
  monk_report_t( monk_t& player ) : p( player )
  {
  }

  void html_customsection( report::sc_html_stream& os ) override
  {
    // Custom Class Section
    if ( p.specialization() == MONK_BREWMASTER )
    {
      double stagger_tick_dmg   = p.sample_datas.stagger_damage->mean();
      double purified_dmg       = p.sample_datas.purified_damage->mean();
      double staggering_strikes = p.sample_datas.staggering_strikes_cleared->mean();
      double quick_sip          = p.sample_datas.quick_sip_cleared->mean();
      double tranquil_spirit    = p.sample_datas.tranquil_spirit->mean();
      double stagger_total_dmg  = p.sample_datas.stagger_total_damage->mean();

      os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
         << "\t\t\t\t\t<h3 class=\"toggle open\">Stagger Analysis</h3>\n"
         << "\t\t\t\t\t<div class=\"toggle-content\">\n";

      highchart::time_series_t chart_stagger_damage( highchart::build_id( p, "stagger_damage" ), *p.sim );
      chart::generate_actor_timeline( chart_stagger_damage, p, "Stagger effective damage",
                                      color::resource_color( RESOURCE_HEALTH ),
                                      p.sample_datas.stagger_effective_damage_timeline );
      chart_stagger_damage.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart_stagger_damage.set( "chart.width", "575" );
      os << chart_stagger_damage.to_target_div();
      p.sim->add_chart_data( chart_stagger_damage );

      highchart::time_series_t chart_stagger_damage_pct( highchart::build_id( p, "stagger_damage_pct" ), *p.sim );
      chart::generate_actor_timeline( chart_stagger_damage_pct, p, "Stagger pool / health %",
                                      color::resource_color( RESOURCE_HEALTH ),
                                      p.sample_datas.stagger_damage_pct_timeline );
      chart_stagger_damage_pct.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart_stagger_damage_pct.set( "chart.width", "575" );
      os << chart_stagger_damage_pct.to_target_div();
      p.sim->add_chart_data( chart_stagger_damage_pct );

      highchart::time_series_t chart_stagger_pct( highchart::build_id( p, "stagger_pct" ), *p.sim );
      chart::generate_actor_timeline( chart_stagger_pct, p, "Stagger %", color::resource_color( RESOURCE_HEALTH ),
                                      p.sample_datas.stagger_pct_timeline );
      chart_stagger_pct.set( "tooltip.headerFormat", "<b>{point.key}</b> s<br/>" );
      chart_stagger_pct.set( "chart.width", "575" );
      os << chart_stagger_pct.to_target_div();
      p.sim->add_chart_data( chart_stagger_pct );

      fmt::print( os, "\t\t\t\t\t\t<p>Stagger base Unbuffed: {} Raid Buffed: {}</p>\n", p.stagger_base_value(),
                  p.sample_datas.buffed_stagger_base );

      fmt::print( os, "\t\t\t\t\t\t<p>Stagger pct (player level) Unbuffed: {:.2f}% Raid Buffed: {:.2f}%</p>\n",
                  100.0 * p.stagger_pct( p.level() ), 100.0 * p.sample_datas.buffed_stagger_pct_player_level );

      fmt::print( os, "\t\t\t\t\t\t<p>Stagger pct (target level) Unbuffed: {:.2f}% Raid Buffed: {:.2f}%</p>\n",
                  100.0 * p.stagger_pct( p.target->level() ), 100.0 * p.sample_datas.buffed_stagger_pct_target_level );

      fmt::print( os, "\t\t\t\t\t\t<p>Total Stagger damage added: {} / {:.2f}%</p>\n", stagger_total_dmg, 100.0 );
      fmt::print( os, "\t\t\t\t\t\t<p>Stagger cleared by Purifying Brew: {} / {:.2f}%</p>\n", purified_dmg,
                  ( purified_dmg / stagger_total_dmg ) * 100.0 );
      fmt::print( os, "\t\t\t\t\t\t<p>Stagger cleared by Quick Sip: {} / {:.2f}%</p>\n", quick_sip,
                  ( quick_sip / stagger_total_dmg ) * 100.0 );
      fmt::print( os, "\t\t\t\t\t\t<p>Stagger cleared by Staggering Strikes: {} / {:.2f}%</p>\n", staggering_strikes,
                  ( staggering_strikes / stagger_total_dmg ) * 100.0 );
      fmt::print( os, "\t\t\t\t\t\t<p>Stagger cleared by Quick Sip: {} / {:.2f}%</p>\n", tranquil_spirit,
                  ( tranquil_spirit / stagger_total_dmg ) * 100.0 );
      fmt::print( os, "\t\t\t\t\t\t<p>Stagger that directly damaged the player: {} / {:.2f}%</p>\n", stagger_tick_dmg,
                  ( stagger_tick_dmg / stagger_total_dmg ) * 100.0 );

      os << "\t\t\t\t\t\t<table class=\"sc\">\n"
         << "\t\t\t\t\t\t\t<tbody>\n"
         << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<th class=\"left\">Damage Stats</th>\n"
         << "\t\t\t\t\t\t\t\t\t<th>Total</th>\n"
         //        << "\t\t\t\t\t\t\t\t\t<th>DTPS%</th>\n"
         << "\t\t\t\t\t\t\t\t\t<th>Execute</th>\n"
         << "\t\t\t\t\t\t\t\t</tr>\n";

      // Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
         << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124255\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/ability_rogue_cheatdeath.gif) "
            "0% "
            "50% no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << ( p.sample_datas.stagger_damage->mean() )
         << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << p.sample_datas.stagger_damage->count()
         << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Light Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
         << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124275\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra_green.gif) "
            "0% "
            "50% no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Light Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << ( p.sample_datas.light_stagger_damage->mean() ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << p.sample_datas.light_stagger_damage->count()
         << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Moderate Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
         << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124274\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra.gif) 0% 50% "
            "no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Moderate Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << ( p.sample_datas.moderate_stagger_damage->mean() ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << p.sample_datas.moderate_stagger_damage->count() << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      // Heavy Stagger info
      os << "\t\t\t\t\t\t\t\t<tr>\n"
         << "\t\t\t\t\t\t\t\t\t<td class=\"left small\" rowspan=\"1\">\n"
         << "\t\t\t\t\t\t\t\t\t\t<span class=\"toggle - details\">\n"
         << "\t\t\t\t\t\t\t\t\t\t\t<a href = \"https://www.wowhead.com/spell=124273\" class = \" icontinyl icontinyl "
            "icontinyl\" "
         << "style = \"background: url(https://wowimg.zamimg.com/images/wow/icons/tiny/priest_icon_chakra_red.gif) 0% "
            "50% no-repeat;\"> "
         << "<span style = \"margin - left: 18px; \">Heavy Stagger</span></a></span>\n"
         << "\t\t\t\t\t\t\t\t\t</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">"
         << ( p.sample_datas.heavy_stagger_damage->mean() ) << "</td>\n";
      os << "\t\t\t\t\t\t\t\t\t<td class=\"right small\" rowspan=\"1\">" << p.sample_datas.heavy_stagger_damage->count()
         << "</td>\n";
      os << "\t\t\t\t\t\t\t\t</tr>\n";

      os << "\t\t\t\t\t\t\t</tbody>\n"
         << "\t\t\t\t\t\t</table>\n";

      os << "\t\t\t\t\t\t</div>\n"
         << "\t\t\t\t\t</div>\n";
    }
    else
      (void)p;
  }

private:
  monk_t& p;
};

struct monk_module_t : public module_t
{
  monk_module_t() : module_t( MONK )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r ) const override
  {
    auto p              = new monk_t( sim, name, r );
    p->report_extension = std::make_unique<monk_report_t>( *p );
    return p;
  }
  bool valid() const override
  {
    return true;
  }

  void static_init() const override
  {
    items::init();
  }

  void register_hotfixes() const override
  {
    /*    hotfix::register_effect( "Monk", "2020-11-21",
                                 "Manually set Direct Damage Windwalker Monk Two-Hand Adjustment by 2%", 872417 )
            .field( "base_value" )
            .operation( hotfix::HOTFIX_ADD )
            .modifier( 2 )
            .verification_value( 0 );
        hotfix::register_effect( "Monk", "2020-11-21",
                                 "Manually set Periodic Damage Windwalker Monk Two-Hand Adjustment by 2%", 872418 )
            .field( "base_value" )
            .operation( hotfix::HOTFIX_ADD )
            .modifier( 2 )
            .verification_value( 0 );
    */
  }

  void init( player_t* p ) const override
  {
    p->buffs.close_to_heart_leech_aura =
        make_buff( p, "close_to_heart_leech_aura", p->find_spell( 389684 ) )->add_invalidate( CACHE_LEECH );
    p->buffs.close_to_heart_leech_aura =
        make_buff( p, "generous_pour_avoidance_aura", p->find_spell( 389685 ) )->add_invalidate( CACHE_AVOIDANCE );
    p->buffs.windwalking_movement_aura =
        make_buff( p, "windwalking_movement_aura", p->find_spell( 365080 ) )->add_invalidate( CACHE_RUN_SPEED );
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

}  // end namespace monk

const module_t* module_t::monk()
{
  static monk::monk_module_t m;
  return &m;
}
