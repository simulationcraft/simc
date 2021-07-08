#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
namespace actions_demonology
{
using namespace actions;

struct demonology_spell_t : public warlock_spell_t
{
public:
  gain_t* gain;

  demonology_spell_t( warlock_t* p, util::string_view n ) : demonology_spell_t( n, p, p->find_class_spell( n ) )
  {
  }

  demonology_spell_t( warlock_t* p, util::string_view n, specialization_e s )
    : demonology_spell_t( n, p, p->find_class_spell( n, s ) )
  {
  }

  demonology_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : warlock_spell_t( token, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;
    weapon_multiplier = 0.0;
    gain              = player->get_gain( name_str );
  }

  void reset() override
  {
    warlock_spell_t::reset();
  }

  void init() override
  {
    warlock_spell_t::init();
  }

  void execute() override
  {
    warlock_spell_t::execute();
  }

  void consume_resource() override
  {
    warlock_spell_t::consume_resource();

    if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
    {
      if ( p()->buffs.nether_portal->up() )
      {
        p()->active.summon_random_demon->execute();
        p()->buffs.portal_summons->trigger();
        p()->procs.portal_summon->occur();
      }
    }
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( t );

    auto td = this->td( t );

    if ( td->debuffs_from_the_shadows->check() && data().affected_by( td->debuffs_from_the_shadows->data().effectN( 1 ) ) )
    {
      m *= 1.0 + td->debuffs_from_the_shadows->check_value();
    }

    return m;
  }

  double action_multiplier() const override
  {
    double pm = warlock_spell_t::action_multiplier();

    if ( this->data().affected_by( p()->mastery_spells.master_demonologist->effectN( 2 ) ) )
      pm *= 1.0 + p()->cache.mastery_value();

    return pm;
  }
};

struct shadow_bolt_t : public demonology_spell_t
{
  shadow_bolt_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Shadow Bolt", p, p->find_spell( 686 ) )
  {
    parse_options( options_str );
    energize_type     = action_energize::ON_CAST;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount   = 1;
  }

  void execute() override
  {
    demonology_spell_t::execute();

    if ( p()->talents.demonic_calling->ok() )
      p()->buffs.demonic_calling->trigger();

    if ( p()->legendary.balespiders_burning_core->ok() )
      p()->buffs.balespiders_burning_core->trigger();
  }

  double action_multiplier() const override
  {
    double m = demonology_spell_t::action_multiplier();

    if ( p()->talents.sacrificed_souls->ok() )
    {
      m *= 1.0 + p()->talents.sacrificed_souls->effectN( 1 ).percent() * p()->active_pets;
    }

    return m;
  }
};

struct hand_of_guldan_t : public demonology_spell_t
{
  struct hog_impact_t : public demonology_spell_t
  {
    int shards_used;
    const spell_data_t* summon_spell;
    timespan_t meteor_time;

    hog_impact_t( warlock_t* p, util::string_view options_str )
      : demonology_spell_t( "Hand of Gul'dan (Impact)", p, p->find_spell( 86040 ) ),
        shards_used( 0 ),
        summon_spell( p->find_spell( 104317 ) ),
        meteor_time( 400_ms )
    {
      parse_options(options_str);
      aoe = -1;
      dual = 1;

      parse_effect_data( s_data->effectN( 1 ) );
    }

    void execute() override
    {
      demonology_spell_t::execute();

      if ( p()->legendary.forces_of_the_horned_nightmare.ok() && rng().roll( p()->legendary.forces_of_the_horned_nightmare->effectN( 1 ).percent() ) )
      {
        p()->procs.horned_nightmare->occur();
        execute(); //TOCHECK: can the proc spawn additional procs? currently implemented as YES
      }
    }

    timespan_t travel_time() const override
    {
      return meteor_time;
    }

    double action_multiplier() const override
    {
      double m = demonology_spell_t::action_multiplier();

      m *= shards_used;

      return m;
    }

    void impact( action_state_t* s ) override
    {
      demonology_spell_t::impact( s );

      // Only trigger wild imps once for the original target impact.
      // Still keep it in impact instead of execute because of travel delay.
      if ( result_is_hit( s->result ) && s->target == target )
      {

        //Wild Imp spawns appear to have been sped up in Shadowlands. Last tested 2021-04-16.
        //Current behavior: HoG will spawn a meteor on cast finish. Travel time in spell data is 0.7 seconds.
        //However, damage event occurs before spell effect lands, happening 0.4 seconds after cast.
        //Imps then spawn roughly every 0.18 seconds seconds after the damage event.
        for ( int i = 1; i <= shards_used; i++ )
        {
          auto ev = make_event<imp_delay_event_t>( *sim, p(), rng().gauss( 180.0 * i, 25.0 ), 180.0 * i );
          this->p()->wild_imp_spawns.push_back( ev );
        }
      }
    }
  };

  hog_impact_t* impact_spell;

  hand_of_guldan_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( p, "Hand of Gul'dan" ),
      impact_spell( new hog_impact_t( p, options_str ) )
  {
    parse_options( options_str );

    //impact_spell->meteor_time = timespan_t::from_seconds( data().missile_speed() );
    impact_spell->meteor_time = 400_ms; //See comments in impact spell for current behavior
  }

  timespan_t travel_time() const override
  {
    return 0_ms;
  }

  bool ready() override
  {
    if ( p()->resources.current[ RESOURCE_SOUL_SHARD ] == 0.0 )
    {
      return false;
    }
    return demonology_spell_t::ready();
  }

  void execute() override
  {
    int shards_used = as<int>( cost() );
    impact_spell->shards_used = shards_used;

    demonology_spell_t::execute();

    if ( rng().roll( p()->conduit.borne_of_blood.percent() ) )
      p()->buffs.demonic_core->trigger();

    if ( p()->legendary.grim_inquisitors_dread_calling.ok() )
      p()->buffs.dread_calling->increment( shards_used, p()->buffs.dread_calling->default_value );
  }

  void consume_resource() override
  {
    demonology_spell_t::consume_resource();

    if ( last_resource_cost == 1.0 )
      p()->procs.one_shard_hog->occur();
    if ( last_resource_cost == 2.0 )
      p()->procs.two_shard_hog->occur();
    if ( last_resource_cost == 3.0 )
      p()->procs.three_shard_hog->occur();
  }

  void impact( action_state_t* s ) override
  {
    demonology_spell_t::impact( s );

    impact_spell->execute();
  }

};

struct demonbolt_t : public demonology_spell_t
{
  demonbolt_t( warlock_t* p, util::string_view options_str ) : demonology_spell_t( p, "Demonbolt" )
  {
    parse_options( options_str );
    energize_type     = action_energize::ON_CAST;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount   = 2;
  }

  timespan_t execute_time() const override
  {
    auto et = demonology_spell_t::execute_time();

    if ( p()->buffs.demonic_core->check() )
    {
      et *= 1.0 + p()->buffs.demonic_core->data().effectN( 1 ).percent();
    }

    return et;
  }

  void execute() override
  {
    demonology_spell_t::execute();

    p()->buffs.demonic_core->up();  // benefit tracking
    p()->buffs.demonic_core->decrement();

    if ( p()->talents.power_siphon->ok() )
      p()->buffs.power_siphon->decrement();

    if ( p()->talents.demonic_calling->ok() )
      p()->buffs.demonic_calling->trigger();

    p()->buffs.decimating_bolt->decrement();

    if ( p()->legendary.shard_of_annihilation.ok() )
      p()->buffs.shard_of_annihilation->decrement();
  }

  double action_multiplier() const override
  {
    double m = demonology_spell_t::action_multiplier();

    if ( p()->talents.sacrificed_souls->ok() )
    {
      m *= 1.0 + p()->talents.sacrificed_souls->effectN( 1 ).percent() * p()->active_pets;
    }

    if ( p()->talents.power_siphon->ok() && p()->buffs.power_siphon->check() )
    {
      m *= 1.0 + p()->buffs.power_siphon->default_value;
    }

    m *= 1.0 + p()->buffs.balespiders_burning_core->check_stack_value();

    m *= 1 + p()->buffs.decimating_bolt->check_value();

    return m;
  }

  double composite_crit_chance_multiplier() const override
  {
    double m = demonology_spell_t::composite_crit_chance_multiplier();

    if ( p()->legendary.shard_of_annihilation.ok() )
    {
      //PTR 2021-06-19: "Critical Strike chance increased by 100%" appears to be guaranteeing crits
      m += p()->buffs.shard_of_annihilation->data().effectN( 5 ).percent();
    }

    return m;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = demonology_spell_t::composite_crit_damage_bonus_multiplier();

    if ( p()->legendary.shard_of_annihilation.ok() )
      m += p()->buffs.shard_of_annihilation->data().effectN( 6 ).percent();

    return m;
  }
};

struct call_dreadstalkers_t : public demonology_spell_t
{
  int dreadstalker_count;

  call_dreadstalkers_t( warlock_t* p, util::string_view options_str ) : demonology_spell_t( p, "Call Dreadstalkers" )
  {
    parse_options( options_str );
    may_crit           = false;
    dreadstalker_count = as<int>( data().effectN( 1 ).base_value() );
    base_execute_time += p->spec.call_dreadstalkers_2->effectN( 1 ).time_value();
  }

  double cost() const override
  {
    double c = demonology_spell_t::cost();

    if ( p()->buffs.demonic_calling->check() )
    {
      c -= p()->talents.demonic_calling->effectN( 1 ).base_value();
    }

    return c;
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.demonic_calling->check() )
    {
      return timespan_t::zero();
    }

    return demonology_spell_t::execute_time();
  }

  void execute() override
  {
    demonology_spell_t::execute();

    auto dogs = p()->warlock_pet_list.dreadstalkers.spawn( as<unsigned>( dreadstalker_count ) );

    if ( p()->buffs.demonic_calling->up() )
    {  // benefit tracking

      //Despite having no cost when Demonic Calling is up, this spell will still proc effects based on shard spending (last checked 2021-03-11)
      double base_cost = demonology_spell_t::cost();

      if ( p()->legendary.wilfreds_sigil_of_superior_summoning->ok() )
        p()->cooldowns.demonic_tyrant->adjust( -base_cost * p()->legendary.wilfreds_sigil_of_superior_summoning->effectN( 2 ).time_value(), false );

      if ( p()->buffs.nether_portal->up() )
      {
        p()->active.summon_random_demon->execute();
        p()->buffs.portal_summons->trigger();
        p()->procs.portal_summon->occur();
      }

      if ( p()->talents.soul_conduit->ok() )
      {
        make_event<sc_event_t>( *p()->sim, p(), as<int>( base_cost ) );
      }

      p()->buffs.demonic_calling->decrement();
    }

    //TOCHECK: Verify only the new pair of dreadstalkers gets the buff
    if ( p()->legendary.grim_inquisitors_dread_calling.ok() )
    {
      for ( auto d : dogs )
      {
        //Only apply buff to dogs without a buff. If no stacks of the buff currently exist on the warlock, apply a buff with value of 0
        if ( d->is_active() && !d->buffs.grim_inquisitors_dread_calling->check() )
        {
          d->buffs.grim_inquisitors_dread_calling->trigger( 1, p()->buffs.dread_calling->check_stack_value() );
        }
      }

      p()->buffs.dread_calling->expire();
    }
  }
};

struct implosion_t : public demonology_spell_t
{
  struct implosion_aoe_t : public demonology_spell_t
  {
    double casts_left = 5.0;
    warlock_pet_t* next_imp;

    implosion_aoe_t( warlock_t* p ) : demonology_spell_t( "implosion_aoe", p, p->find_spell( 196278 ) )
    {
      aoe                = -1;
      dual               = true;
      background         = true;
      callbacks          = false;
      reduced_aoe_damage = false;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = demonology_spell_t::composite_target_multiplier( t );

      if ( t == this->target )
      {
        m *= ( casts_left / 5.0 );
      }

      return m;
    }

    void execute() override
    {
      demonology_spell_t::execute();
      next_imp->dismiss();
    }
  };

  implosion_aoe_t* explosion;

  implosion_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "implosion", p ), explosion( new implosion_aoe_t( p ) )
  {
    parse_options( options_str );
    add_child( explosion );
  }

  bool ready() override
  {
    bool r = demonology_spell_t::ready();

    if ( r )
    {
      return p()->warlock_pet_list.wild_imps.n_active_pets() > 0;
    }
    return false;
  }

  void execute() override
  {
    p()->buffs.implosive_potential->expire();
    p()->buffs.implosive_potential_small->expire();

    auto imps_consumed = p()->warlock_pet_list.wild_imps.n_active_pets();

    // Travel speed is not in spell data, in game test appears to be 65 yds/sec as of 2020-12-04
    timespan_t imp_travel_time = this->calc_imp_travel_time(65);

    int launch_counter = 0;
    for ( auto imp : p()->warlock_pet_list.wild_imps )
    {
      if ( !imp->is_sleeping() )
      {
        implosion_aoe_t* ex = explosion;
        player_t* tar       = this->target;
        double dist         = p()->get_player_distance( *tar );

        imp->trigger_movement( dist, movement_direction_type::TOWARDS );
        imp->interrupt();

        // Imps launched with Implosion appear to be staggered and snapshot when they impact
        // 2020-12-04: Implosion may have been made quicker in Shadowlands, too fast to easily discern with combat log
        // Going to set the interval to 10 ms, which should keep all but the most extreme imp counts from bleeding into the next GCD
        // TODO: There's an awkward possibility of Implosion seeming "ready" after casting it if all the imps have not imploded yet. Find a workaround
        make_event( sim, 10_ms * launch_counter + imp_travel_time, [ ex, tar, imp ] {
          if ( imp && !imp->is_sleeping() )
          {
            ex->casts_left = ( imp->resources.current[ RESOURCE_ENERGY ] / 20 );
            ex->set_target( tar );
            ex->next_imp = imp;
            ex->execute();
          }
        } );

        launch_counter++;
      }
    }

    if ( p()->legendary.implosive_potential.ok() && target_list().size() >= as<size_t>( p()->legendary.implosive_potential->effectN( 1 ).base_value() ) )
      p()->buffs.implosive_potential->trigger( imps_consumed );
    else if ( p()->legendary.implosive_potential.ok() )
      p()->buffs.implosive_potential_small->trigger( imps_consumed );

    demonology_spell_t::execute();
  }

  // A shortened version of action_t::travel_time() for use in calculating the time until the imp is dismissed.
  // Note that imps explode at the bottom of the target, so no height component is accounted for.
  timespan_t calc_imp_travel_time( double speed )
  {
    double t = 0;

    if ( speed > 0 )
    {
      double distance = player->get_player_distance( *target );

      if ( distance > 0 )
        t += distance / speed;
    }

    double v = sim->travel_variance;

    if ( v )
      t = rng().gauss( t, v );

    t = std::max( t, min_travel_time );

    return timespan_t::from_seconds( t );
  }
};

struct summon_demonic_tyrant_t : public demonology_spell_t
{
  double demonic_consumption_added_damage;
  double demonic_consumption_health_percentage;

  summon_demonic_tyrant_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "summon_demonic_tyrant", p, p->find_spell( 265187 ) ),
      demonic_consumption_added_damage( 0 ),
      demonic_consumption_health_percentage( 0 )
  {
    parse_options( options_str );
    harmful = may_crit = false;

    demonic_consumption_health_percentage = p->find_spell( 267971 )->effectN( 1 ).percent();
  }

  void execute() override
  {
    demonology_spell_t::execute();

    p()->warlock_pet_list.demonic_tyrants.spawn( data().duration() );

    p()->buffs.demonic_power->trigger();

    if ( p()->spec.summon_demonic_tyrant_2->ok() )
      p()->resource_gain( RESOURCE_SOUL_SHARD, p()->spec.summon_demonic_tyrant_2->effectN( 1 ).base_value() / 10.0,
                          p()->gains.summon_demonic_tyrant );

    demonic_consumption_added_damage = 0;

    for ( auto& pet : p()->pet_list )
    {
      auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );

      if ( lock_pet == nullptr )
        continue;
      if ( lock_pet->is_sleeping() )
        continue;

      if ( lock_pet->pet_type == PET_DEMONIC_TYRANT )
        continue;

      if ( lock_pet->expiration )
      {
        timespan_t new_time = lock_pet->expiration->time + p()->buffs.demonic_power->data().effectN( 3 ).time_value();
        lock_pet->expiration->reschedule_time = new_time;
      }

      if ( p()->talents.demonic_consumption->ok() )
      {
        //This is a hack to get around the fact we are not currently updating pet HP dynamically
        //TODO: Pet stats (especially HP) need more reliable modeling of caching/updating
        double available = p()->max_health() * pet->owner_coeff.health;

        // There is no spelldata for how health is converted into damage, current testing indicates the 15% of hp taken
        // from pets is divided by 10 and added to base damage 09-05-2020.
        // TODO - make it properly remove the health.
        demonic_consumption_added_damage += available * demonic_consumption_health_percentage / 10.0;
      }
    }

    if ( p()->talents.demonic_consumption->ok() )
    {
      for ( auto dt : p()->warlock_pet_list.demonic_tyrants )
      {
        if ( !dt->is_sleeping() )
        {
          dt->buffs.demonic_consumption->trigger( 1, demonic_consumption_added_damage );
        }
      }
    }

    p()->buffs.tyrant->trigger();
    if ( p()->buffs.dreadstalkers->check() )
    {
      p()->buffs.dreadstalkers->extend_duration( p(), p()->buffs.demonic_power->data().effectN( 3 ).time_value() );
    }
    if ( p()->buffs.grimoire_felguard->check() )
    {
      p()->buffs.grimoire_felguard->extend_duration( p(), p()->buffs.demonic_power->data().effectN( 3 ).time_value() );
    }
    if ( p()->buffs.vilefiend->check() )
    {
      p()->buffs.vilefiend->extend_duration( p(), p()->buffs.demonic_power->data().effectN( 3 ).time_value() );
    }
  }
};

// Talents
struct demonic_strength_t : public demonology_spell_t
{
  demonic_strength_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "demonic_strength", p, p->talents.demonic_strength )
  {
    parse_options( options_str );
  }

  bool ready() override
  {
    auto active_pet = p()->warlock_pet_list.active;

    if ( !active_pet )
      return false;

    if ( active_pet->pet_type != PET_FELGUARD )
      return false;
    if ( active_pet->find_action( "felstorm" )->get_dot()->is_ticking() )
      return false;
    if ( active_pet->find_action( "demonic_strength_felstorm" )->get_dot()->is_ticking() )
      return false;
    return spell_t::ready();
  }

  void execute() override
  {
    demonology_spell_t::execute();

    if ( p()->warlock_pet_list.active->pet_type == PET_FELGUARD )
    {
      p()->warlock_pet_list.active->buffs.demonic_strength->trigger();

      auto pet = debug_cast<pets::demonology::felguard_pet_t*>( p()->warlock_pet_list.active );
      pet->queue_ds_felstorm();
    }
  }
};

struct bilescourge_bombers_t : public demonology_spell_t
{
  struct bilescourge_bombers_tick_t : public demonology_spell_t
  {
    bilescourge_bombers_tick_t( warlock_t* p )
      : demonology_spell_t( "bilescourge_bombers_tick", p, p->find_spell( 267213 ) )
    {
      aoe        = -1;
      background = dual = direct_tick = true;
      callbacks                       = false;
      radius                          = p->talents.bilescourge_bombers->effectN( 1 ).radius();
    }
  };

  bilescourge_bombers_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "bilescourge_bombers", p, p->talents.bilescourge_bombers )
  {
    parse_options( options_str );
    dot_duration = timespan_t::zero();
    may_miss = may_crit = false;
    base_tick_time      = data().duration() / 12.0;
    base_execute_time   = timespan_t::zero();

    if ( !p->active.bilescourge_bombers )
    {
      p->active.bilescourge_bombers        = new bilescourge_bombers_tick_t( p );
      p->active.bilescourge_bombers->stats = stats;
    }
  }

  void execute() override
  {
    demonology_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( execute_state->target->x_position )
                                        .y( execute_state->target->y_position )
                                        .pulse_time( base_tick_time * player->cache.spell_haste() )
                                        .duration( data().duration() * player->cache.spell_haste() )
                                        .start_time( sim->current_time() )
                                        .action( p()->active.bilescourge_bombers ) );
  }
};

struct power_siphon_t : public demonology_spell_t
{
  power_siphon_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "power_siphon", p, p->talents.power_siphon )
  {
    parse_options( options_str );
    harmful               = false;
    ignore_false_positive = true;
  }

  bool ready() override
  {
    if ( p()->warlock_pet_list.wild_imps.n_active_pets() < 1 )
      return false;

    return demonology_spell_t::ready();
  }

  void execute() override
  {
    demonology_spell_t::execute();

    auto imps = p()->warlock_pet_list.wild_imps.active_pets();

    range::sort(
        imps, []( const pets::demonology::wild_imp_pet_t* imp1, const pets::demonology::wild_imp_pet_t* imp2 ) {
          double lv = imp1->resources.current[ RESOURCE_ENERGY ];
          double rv = imp2->resources.current[ RESOURCE_ENERGY ];

          if ( lv == rv )
          {
            timespan_t lr = imp1->expiration->remains();
            timespan_t rr = imp2->expiration->remains();
            if ( lr == rr )
            {
              return imp1->actor_spawn_index < imp2->actor_spawn_index;
            }

            return lr < rr;
          }

          return lv < rv;
        } );

    unsigned max_imps = as<int>( p()->talents.power_siphon->effectN( 1 ).base_value() );
    if ( imps.size() > max_imps )
      imps.resize( max_imps );

    while ( !imps.empty() )
    {
      p()->buffs.power_siphon->trigger();
      p()->buffs.demonic_core->trigger();
      pets::demonology::wild_imp_pet_t* imp = imps.front();
      imps.erase( imps.begin() );
      imp->power_siphon = true;
      imp->dismiss();
    }
  }
};

struct doom_t : public demonology_spell_t
{
  doom_t( warlock_t* p, util::string_view options_str ) : demonology_spell_t( "doom", p, p->talents.doom )
  {
    parse_options( options_str );

    energize_type     = action_energize::PER_TICK;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount   = 1;

    //TODO: Verify haste and refresh behaviors with changing stats while dot is already applied 
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return s->action->tick_time( s ); //Doom is a case where dot duration scales with haste so use the tick time to get the current correct value
  }
};

struct soul_strike_t : public demonology_spell_t
{
  soul_strike_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Soul Strike", p, p->talents.soul_strike )
  {
    parse_options( options_str );
    energize_type     = action_energize::ON_CAST;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount   = 1;
  }
  void execute() override
  {
    demonology_spell_t::execute();
    if ( p()->warlock_pet_list.active->pet_type == PET_FELGUARD )
    {
      auto pet = debug_cast<pets::demonology::felguard_pet_t*>( p()->warlock_pet_list.active );
      pet->soul_strike->set_target( execute_state->target );
      pet->soul_strike->execute();
    }
  }
  bool ready() override
  {
    if ( p()->warlock_pet_list.active->pet_type == PET_FELGUARD )
      return demonology_spell_t::ready();

    return false;
  }
};

struct summon_vilefiend_t : public demonology_spell_t
{
  summon_vilefiend_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "summon_vilefiend", p, p->talents.summon_vilefiend )
  {
    parse_options( options_str );
    harmful = may_crit = false;
  }

  void execute() override
  {
    demonology_spell_t::execute();
    p()->buffs.vilefiend->trigger();
    p()->warlock_pet_list.vilefiends.spawn( data().duration() );
  }
};

struct grimoire_felguard_t : public demonology_spell_t
{
  grimoire_felguard_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "grimoire_felguard", p, p->talents.grimoire_felguard )
  {
    parse_options( options_str );
    harmful = may_crit = false;
  }

  void execute() override
  {
    demonology_spell_t::execute();
    
    p()->warlock_pet_list.grimoire_felguards.spawn( data().duration() );
    p()->buffs.grimoire_felguard->trigger();
  }
};

struct nether_portal_t : public demonology_spell_t
{
  nether_portal_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "nether_portal", p, p->talents.nether_portal )
  {
    parse_options( options_str );
    harmful = may_crit = may_miss = false;
  }

  void execute() override
  {
    p()->buffs.nether_portal->trigger();
    demonology_spell_t::execute();
  }
};

struct summon_random_demon_t : public demonology_spell_t
{
  enum class random_pet_type : int
  {
    shivarra           = 0,
    darkhounds         = 1,
    bilescourges       = 2,
    urzuls             = 3,
    void_terrors       = 4,
    wrathguards        = 5,
    vicious_hellhounds = 6,
    illidari_satyrs    = 7,
    eyes_of_guldan     = 8,
    prince_malchezaar  = 9,
  };

  timespan_t summon_duration;
  summon_random_demon_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "summon_random_demon", p ), summon_duration( timespan_t::from_seconds( 15 ) )
  {
    parse_options( options_str );
    background = true;
  }

  void execute() override
  {
    demonology_spell_t::execute();

    auto random_pet = roll_random_pet();
    summon_random_pet( random_pet );
    p()->procs.summon_random_demon->occur();
  }

private:
  /**
   * Helper function to actually summon 'num_summons' random pets from a given pet list.
   */
  template <typename pet_list_type>
  void summon_random_pet_helper( pet_list_type& pet_list, int num_summons = 1 )
  {
    pet_list.spawn( summon_duration, as<unsigned>( num_summons ) );
  }

  /**
   * Summon the random pet(s) specified.
   */
  void summon_random_pet( random_pet_type random_pet )
  {
    switch ( random_pet )
    {
      case random_pet_type::prince_malchezaar:
        summon_random_pet_helper( p()->warlock_pet_list.prince_malchezaar );
        break;
      case random_pet_type::eyes_of_guldan:
      {
        // TODO - eyes appear to spawn in #s randomly between 1 and 4 in SL beta 09-24-2020.
        // eyes summon in groups of 4. Confirmed by pip 2018-06-23.
        summon_random_pet_helper( p()->warlock_pet_list.eyes_of_guldan, 4 );
        break;
        case random_pet_type::shivarra:
          summon_random_pet_helper( p()->warlock_pet_list.shivarra );
          break;
        case random_pet_type::darkhounds:
          summon_random_pet_helper( p()->warlock_pet_list.darkhounds );
          break;
        case random_pet_type::bilescourges:
          summon_random_pet_helper( p()->warlock_pet_list.bilescourges );
          break;
        case random_pet_type::urzuls:
          summon_random_pet_helper( p()->warlock_pet_list.urzuls );
          break;
        case random_pet_type::void_terrors:
          summon_random_pet_helper( p()->warlock_pet_list.void_terrors );
          break;
        case random_pet_type::wrathguards:
          summon_random_pet_helper( p()->warlock_pet_list.wrathguards );
          break;
        case random_pet_type::vicious_hellhounds:
          summon_random_pet_helper( p()->warlock_pet_list.vicious_hellhounds );
          break;
        case random_pet_type::illidari_satyrs:
          summon_random_pet_helper( p()->warlock_pet_list.illidari_satyrs );
          break;
        default:
          assert( false && "trying to summon invalid random demon." );
          break;
      }
    }
  }

  /**
   * Roll the dice and determine which random pet(s) to summon.
   */
  random_pet_type roll_random_pet()
  {
    int demon_int = rng().range( 9 ); //Malchezaar is disabled in instances. TODO: Add option to reenable?
    int rare_check;
    if ( demon_int > 7 )
    {
      rare_check = rng().range( 10 );
      if ( rare_check > 0 )
      {
        demon_int = rng().range( 8 );
      }
    }
    assert( demon_int >= 0 && demon_int <= 9 );
    return static_cast<random_pet_type>( demon_int );
  }
};

}  // namespace actions_demonology

namespace buffs
{
}  // namespace buffs

pet_t* warlock_t::create_demo_pet( util::string_view pet_name, util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );
  if ( p )
    return p;
  using namespace pets;

  if ( pet_name == "felguard" )
    return new pets::demonology::felguard_pet_t( this, pet_name );

  return nullptr;
}
// add actions
action_t* warlock_t::create_action_demonology( util::string_view action_name, const std::string& options_str )
{
  using namespace actions_demonology;

  if ( action_name == "shadow_bolt" )
    return new shadow_bolt_t( this, options_str );
  if ( action_name == "demonbolt" )
    return new demonbolt_t( this, options_str );
  if ( action_name == "hand_of_guldan" )
    return new hand_of_guldan_t( this, options_str );
  if ( action_name == "implosion" )
    return new implosion_t( this, options_str );

  if ( action_name == "demonic_strength" )
    return new demonic_strength_t( this, options_str );
  if ( action_name == "bilescourge_bombers" )
    return new bilescourge_bombers_t( this, options_str );
  if ( action_name == "doom" )
    return new doom_t( this, options_str );
  if ( action_name == "power_siphon" )
    return new power_siphon_t( this, options_str );
  if ( action_name == "soul_strike" )
    return new soul_strike_t( this, options_str );
  if ( action_name == "nether_portal" )
    return new nether_portal_t( this, options_str );
  if ( action_name == "call_dreadstalkers" )
    return new call_dreadstalkers_t( this, options_str );
  if ( action_name == "summon_felguard" )
    return new summon_main_pet_t( "felguard", this );
  if ( action_name == "summon_demonic_tyrant" )
    return new summon_demonic_tyrant_t( this, options_str );
  if ( action_name == "summon_vilefiend" )
    return new summon_vilefiend_t( this, options_str );
  if ( action_name == "grimoire_felguard" )
    return new grimoire_felguard_t( this, options_str );

  return nullptr;
}

void warlock_t::create_buffs_demonology()
{
  buffs.demonic_core = make_buff( this, "demonic_core", find_spell( 264173 ) );

  buffs.power_siphon = make_buff( this, "power_siphon", find_spell( 334581 ) )
                            ->set_default_value( find_spell( 334581 )->effectN( 1 ).percent() );

  buffs.demonic_power = make_buff( this, "demonic_power", find_spell( 265273 ) )
                            ->set_default_value( find_spell( 265273 )->effectN( 2 ).percent() )
                            ->set_cooldown( timespan_t::zero() );

  // Talents
  buffs.demonic_calling = make_buff( this, "demonic_calling", talents.demonic_calling->effectN( 1 ).trigger() )
                              ->set_chance( talents.demonic_calling->proc_chance() );

  buffs.inner_demons = make_buff( this, "inner_demons", find_spell( 267216 ) )
                           ->set_period( talents.inner_demons->effectN( 1 ).period() )
                           ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                           ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                             warlock_pet_list.wild_imps.spawn();
                             if ( rng().roll( talents.inner_demons->effectN( 1 ).percent() ) )
                             {
                               active.summon_random_demon->execute();
                             }
                           } );

  buffs.nether_portal =
      make_buff( this, "nether_portal", talents.nether_portal )->set_duration( talents.nether_portal->duration() );

  // Conduits
  buffs.tyrants_soul = make_buff( this, "tyrants_soul", find_spell( 339784 ) )
                           ->set_default_value( conduit.tyrants_soul.percent() );

  // Legendaries
  buffs.balespiders_burning_core =
      make_buff( this, "balespiders_burning_core", legendary.balespiders_burning_core->effectN( 1 ).trigger() )
          ->set_trigger_spell( legendary.balespiders_burning_core )
          ->set_default_value( legendary.balespiders_burning_core->effectN( 1 ).trigger()->effectN( 1 ).percent() );

  //TODO: SL Beta - check max stacks, refresh behavior, etc
  buffs.implosive_potential = make_buff<buff_t>(this, "implosive_potential", find_spell(337139))
          ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
          ->set_default_value( legendary.implosive_potential->effectN( 2 ).percent() )
          ->set_max_stack( 40 ); //Using the other wild imp simc max for now

  //For ease of use and tracking, the lesser version will have (small) appended to a separate buff
  buffs.implosive_potential_small = make_buff<buff_t>(this, "implosive_potential_small", find_spell(337139))
          ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
          ->set_default_value( legendary.implosive_potential->effectN( 3 ).percent() )
          ->set_max_stack( 40 ); //Using the other wild imp simc max for now

  buffs.dread_calling = make_buff<buff_t>( this, "dread_calling", find_spell( 342997 ) )
                            ->set_default_value( legendary.grim_inquisitors_dread_calling->effectN( 1 ).percent() );

  // to track pets
  buffs.wild_imps = make_buff( this, "wild_imps" )->set_max_stack( 40 );

  buffs.dreadstalkers = make_buff( this, "dreadstalkers" )->set_max_stack( 4 )
                        ->set_duration( find_spell( 193332 )->duration() );

  buffs.vilefiend = make_buff( this, "vilefiend" )->set_max_stack( 1 )
                    ->set_duration( talents.summon_vilefiend->duration() );

  buffs.tyrant = make_buff( this, "tyrant" )->set_max_stack( 1 )
                 ->set_duration( find_spell( 265187 )->duration() );

  buffs.grimoire_felguard = make_buff( this, "grimoire_felguard" )->set_max_stack( 1 )
                            ->set_duration( talents.grimoire_felguard->duration() );

  buffs.prince_malchezaar = make_buff( this, "prince_malchezaar" )->set_max_stack( 1 );

  buffs.eyes_of_guldan = make_buff( this, "eyes_of_guldan" )->set_max_stack( 4 );

  buffs.portal_summons = make_buff( this, "portal_summons" )
                             ->set_duration( timespan_t::from_seconds( 15 ) )
                             ->set_max_stack( 40 )
                             ->set_refresh_behavior( buff_refresh_behavior::DURATION );
}

void warlock_t::init_spells_demonology()
{
  spec.demonology                    = find_specialization_spell( 137044 );
  mastery_spells.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY );

  spec.call_dreadstalkers_2    = find_specialization_spell( 334727 );
  spec.fel_firebolt_2          = find_specialization_spell( 334591 );
  spec.summon_demonic_tyrant_2 = find_specialization_spell( 334585 );

  // spells
  // Talents
  talents.dreadlash           = find_talent_spell( "Dreadlash" );
  talents.demonic_strength    = find_talent_spell( "Demonic Strength" );
  talents.bilescourge_bombers = find_talent_spell( "Bilescourge Bombers" );
  talents.demonic_calling     = find_talent_spell( "Demonic Calling" );
  talents.power_siphon        = find_talent_spell( "Power Siphon" );
  talents.doom                = find_talent_spell( "Doom" );
  talents.from_the_shadows    = find_talent_spell( "From the Shadows" );
  talents.soul_strike         = find_talent_spell( "Soul Strike" );
  talents.summon_vilefiend    = find_talent_spell( "Summon Vilefiend" );
  talents.inner_demons        = find_talent_spell( "Inner Demons" );
  talents.grimoire_felguard   = find_talent_spell( "Grimoire: Felguard" );
  talents.sacrificed_souls    = find_talent_spell( "Sacrificed Souls" );
  talents.demonic_consumption = find_talent_spell( "Demonic Consumption" );
  talents.nether_portal       = find_talent_spell( "Nether Portal" );

  // Legendaries
  legendary.balespiders_burning_core       = find_runeforge_legendary( "Balespider's Burning Core" );
  legendary.forces_of_the_horned_nightmare = find_runeforge_legendary( "Forces of the Horned Nightmare" );
  legendary.grim_inquisitors_dread_calling = find_runeforge_legendary( "Grim Inquisitor's Dread Calling" );
  legendary.implosive_potential            = find_runeforge_legendary( "Implosive Potential" );

  // Conduits
  conduit.borne_of_blood       = find_conduit_spell( "Borne of Blood" );
  conduit.carnivorous_stalkers = find_conduit_spell( "Carnivorous Stalkers" );
  conduit.fel_commando         = find_conduit_spell( "Fel Commando" );
  conduit.tyrants_soul         = find_conduit_spell( "Tyrant's Soul" );

  active.summon_random_demon = new actions_demonology::summon_random_demon_t( this, "" );

  // Initialize some default values for pet spawners
  auto imp_summon_spell = find_spell( 104317 );
  warlock_pet_list.wild_imps.set_default_duration( imp_summon_spell->duration() );

  auto dreadstalker_spell = find_spell( 193332 );
  warlock_pet_list.dreadstalkers.set_default_duration( dreadstalker_spell->duration() );
}

void warlock_t::init_gains_demonology()
{
  gains.summon_demonic_tyrant = get_gain( "summon_demonic_tyrant" );
}

void warlock_t::init_rng_demonology()
{
}

void warlock_t::init_procs_demonology()
{
  procs.summon_random_demon = get_proc( "summon_random_demon" );
}

void warlock_t::create_options_demonology()
{
}

void warlock_t::create_apl_demonology()
{
  action_priority_list_t* def    		= get_action_priority_list( "default" );
  action_priority_list_t* cov   		= get_action_priority_list( "covenant_ability" );
  action_priority_list_t* tyrant		= get_action_priority_list( "tyrant_setup" );
  action_priority_list_t* ogcd    		= get_action_priority_list( "ogcd" );
  action_priority_list_t* trinks    	= get_action_priority_list( "trinkets" );
  action_priority_list_t* five_y    	= get_action_priority_list( "5y_per_sec_trinkets" );
  action_priority_list_t* hp    		= get_action_priority_list( "hp_trinks" );
  action_priority_list_t* dmg    		= get_action_priority_list( "pure_damage_trinks" );

  def->add_action( "call_action_list,name=trinkets" );
  def->add_action( "interrupt,if=target.debuff.casting.react" );
  def->add_action( "doom,if=refreshable" );
  def->add_action( "call_action_list,name=covenant_ability,if=soulbind.grove_invigoration|soulbind.field_of_blossoms|soulbind.combat_meditation|covenant.necrolord" );
  def->add_action( "call_action_list,name=tyrant_setup" );
  def->add_action( "potion,if=(cooldown.summon_demonic_tyrant.remains_expected<10&time>variable.first_tyrant_time-10|soulbind.refined_palate&cooldown.summon_demonic_tyrant.remains_expected<38)" );
  def->add_action( "call_action_list,name=ogcd,if=pet.demonic_tyrant.active" );
  def->add_action( "demonic_strength,if=(!runeforge.wilfreds_sigil_of_superior_summoning&cooldown.summon_demonic_tyrant.remains_expected>9)|(pet.demonic_tyrant.active&pet.demonic_tyrant.remains<6*gcd.max)" );
  def->add_action( "call_dreadstalkers,if=cooldown.summon_demonic_tyrant.remains_expected>20-5*!runeforge.wilfreds_sigil_of_superior_summoning" );
  def->add_action( "power_siphon,if=buff.wild_imps.stack>1&buff.demonic_core.stack<3" );
  def->add_action( "bilescourge_bombers,if=buff.tyrant.down&cooldown.summon_demonic_tyrant.remains_expected>5" );
  def->add_action( "implosion,if=active_enemies>1+(1*talent.sacrificed_souls.enabled)&buff.wild_imps.stack>=6&buff.tyrant.down&cooldown.summon_demonic_tyrant.remains_expected>5" );
  def->add_action( "implosion,if=active_enemies>2&buff.wild_imps.stack>=6&buff.tyrant.down&cooldown.summon_demonic_tyrant.remains_expected>5&!runeforge.implosive_potential&(!talent.from_the_shadows.enabled|buff.from_the_shadows.up)" );
  def->add_action( "implosion,if=active_enemies>2&buff.wild_imps.stack>=6&buff.implosive_potential.remains<2&runeforge.implosive_potential" );
  def->add_action( "implosion,if=buff.wild_imps.stack>=12&talent.soul_conduit.enabled&talent.from_the_shadows.enabled&runeforge.implosive_potential&buff.tyrant.down&cooldown.summon_demonic_tyrant.remains_expected>5" );
  def->add_action( "grimoire_felguard,if=time_to_die<30" );
  def->add_action( "summon_vilefiend,if=time_to_die<28" );
  def->add_action( "summon_demonic_tyrant,if=time_to_die<15" );
  def->add_action( "hand_of_guldan,if=soul_shard=5" );
  def->add_action( "hand_of_guldan,if=soul_shard>=3&(pet.dreadstalker.active|pet.demonic_tyrant.active)", "If Dreadstalkers are already active, no need to save shards" );
  def->add_action( "hand_of_guldan,if=soul_shard>=1&buff.nether_portal.up&cooldown.call_dreadstalkers.remains>2*gcd.max" );
  def->add_action( "hand_of_guldan,if=soul_shard>=1&cooldown.summon_demonic_tyrant.remains_expected<gcd.max&time>variable.first_tyrant_time-gcd.max" );
  def->add_action( "call_action_list,name=covenant_ability,if=!covenant.venthyr" );
  def->add_action( "soul_strike,if=!talent.sacrificed_souls.enabled", "Without Sacrificed Souls, Soul Strike is stronger than Demonbolt, so it has a higher priority" );
  def->add_action( "demonbolt,if=buff.demonic_core.react&soul_shard<4&cooldown.summon_demonic_tyrant.remains_expected>20", "Spend Demonic Cores for Soul Shards until Tyrant cooldown is close to ready" );
  def->add_action( "demonbolt,if=buff.demonic_core.react&soul_shard<4&cooldown.summon_demonic_tyrant.remains_expected<12", "During Tyrant setup, spend Demonic Cores for Soul Shards" );
  def->add_action( "demonbolt,if=buff.demonic_core.react&soul_shard<4&(buff.demonic_core.stack>2|talent.sacrificed_souls.enabled)" );
  def->add_action( "demonbolt,if=buff.demonic_core.react&soul_shard<4&active_enemies>1" );
  def->add_action( "soul_strike" );
  def->add_action( "call_action_list,name=covenant_ability" );
  def->add_action( "hand_of_guldan,if=soul_shard>=3&cooldown.summon_demonic_tyrant.remains_expected>25&(talent.demonic_calling.enabled|cooldown.call_dreadstalkers.remains>((5-soul_shard)*action.shadow_bolt.execute_time)+action.hand_of_guldan.execute_time)", "If you can get back to 5 Soul Shards before Dreadstalkers cooldown is ready, it's okay to spend them now" );
  def->add_action( "doom,cycle_targets=1,if=refreshable&time>variable.first_tyrant_time" );
  def->add_action( "shadow_bolt" );

  cov->add_action( "soul_rot,if=soulbind.grove_invigoration&(cooldown.summon_demonic_tyrant.remains_expected<20|cooldown.summon_demonic_tyrant.remains_expected>30)" );
  cov->add_action( "soul_rot,if=soulbind.field_of_blossoms&pet.demonic_tyrant.active" );
  cov->add_action( "soul_rot,if=soulbind.wild_hunt_tactics" );
  cov->add_action( "decimating_bolt,if=(soulbind.lead_by_example|soulbind.kevins_oozeling)&(pet.demonic_tyrant.active&soul_shard<2|cooldown.summon_demonic_tyrant.remains_expected>40)" );
  cov->add_action( "decimating_bolt,if=(soulbind.forgeborne_reveries|(soulbind.volatile_solvent&!soulbind.kevins_oozeling))&!pet.demonic_tyrant.active" );
  cov->add_action( "fleshcraft,if=soulbind.volatile_solvent,cancel_if=buff.volatile_solvent_humanoid.up" );
  cov->add_action( "scouring_tithe,if=soulbind.combat_meditation&pet.demonic_tyrant.active" );
  cov->add_action( "scouring_tithe,if=!soulbind.combat_meditation" );
  cov->add_action( "impending_catastrophe,if=pet.demonic_tyrant.active&soul_shard=0" );

  tyrant->add_action( "nether_portal,if=cooldown.summon_demonic_tyrant.remains_expected<15" );
  tyrant->add_action( "grimoire_felguard,if=cooldown.summon_demonic_tyrant.remains_expected<17-(action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time)&(cooldown.call_dreadstalkers.remains<17-(action.summon_demonic_tyrant.execute_time+action.summon_vilefiend.execute_time+action.shadow_bolt.execute_time)|pet.dreadstalker.remains>cooldown.summon_demonic_tyrant.remains_expected+action.summon_demonic_tyrant.execute_time)" );
  tyrant->add_action( "summon_vilefiend,if=(cooldown.summon_demonic_tyrant.remains_expected<15-(action.summon_demonic_tyrant.execute_time)&(cooldown.call_dreadstalkers.remains<15-(action.summon_demonic_tyrant.execute_time+action.summon_vilefiend.execute_time)|pet.dreadstalker.remains>cooldown.summon_demonic_tyrant.remains_expected+action.summon_demonic_tyrant.execute_time))|(!runeforge.wilfreds_sigil_of_superior_summoning&cooldown.summon_demonic_tyrant.remains_expected>40)" );
  tyrant->add_action( "call_dreadstalkers,if=cooldown.summon_demonic_tyrant.remains_expected<12-(action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time)&time>variable.first_tyrant_time-12-action.call_dreadstalkers.execute_time+action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time" );
  tyrant->add_action( "summon_demonic_tyrant,if=time>variable.first_tyrant_time&(pet.dreadstalker.active&pet.dreadstalker.remains>action.summon_demonic_tyrant.execute_time)&(!talent.summon_vilefiend.enabled|pet.vilefiend.active)&(soul_shard=0|(pet.dreadstalker.active&pet.dreadstalker.remains<action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time)|(pet.vilefiend.active&pet.vilefiend.remains<action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time)|(buff.grimoire_felguard.up&buff.grimoire_felguard.remains<action.summon_demonic_tyrant.execute_time+action.shadow_bolt.execute_time))" );

  ogcd->add_action( "berserking" );
  ogcd->add_action( "blood_fury" );
  ogcd->add_action( "fireblood" );
  ogcd->add_action( "use_items" );

  trinks->add_action( "use_item,name=shadowed_orb_of_torment,if=cooldown.summon_demonic_tyrant.remains_expected<15" );
  trinks->add_action( "call_action_list,name=hp_trinks,if=talent.demonic_consumption.enabled&cooldown.summon_demonic_tyrant.remains_expected<20" );
  trinks->add_action( "call_action_list,name=5y_per_sec_trinkets", "Effects that travel slowly to target require additional, separate handling" );
  trinks->add_action( "use_item,name=overflowing_anima_cage,if=pet.demonic_tyrant.active" );
  trinks->add_action( "use_item,slot=trinket1,if=trinket.1.has_use_buff&pet.demonic_tyrant.active" );
  trinks->add_action( "use_item,slot=trinket2,if=trinket.2.has_use_buff&pet.demonic_tyrant.active" );
  trinks->add_action( "call_action_list,name=pure_damage_trinks,if=time>variable.first_tyrant_time&cooldown.summon_demonic_tyrant.remains_expected>20" );

  five_y->add_action( "use_item,name=soulletting_ruby,if=cooldown.summon_demonic_tyrant.remains_expected<target.distance%5&time>variable.first_tyrant_time-(target.distance%5)" );
  five_y->add_action( "use_item,name=sunblood_amethyst,if=cooldown.summon_demonic_tyrant.remains_expected<target.distance%5&time>variable.first_tyrant_time-(target.distance%5)" );
  five_y->add_action( "use_item,name=empyreal_ordnance,if=cooldown.summon_demonic_tyrant.remains_expected<(target.distance%5)+12&cooldown.summon_demonic_tyrant.remains_expected>(((target.distance%5)+12)-15)&time>variable.first_tyrant_time-((target.distance%5)+12)", "Ordnance has a 12 second delay and is therefore skipped for first Tyrant to line up with the rest" );

  hp->add_action( "use_item,name=sinful_gladiators_emblem" );
  hp->add_action( "use_item,name=sinful_aspirants_emblem" );

  dmg->add_action( "use_item,name=dreadfire_vessel" );
  dmg->add_action( "use_item,name=soul_igniter" );
  dmg->add_action( "use_item,name=glyph_of_assimilation,if=active_enemies=1" );
  dmg->add_action( "use_item,name=darkmoon_deck_putrescence" );
  dmg->add_action( "use_item,name=ebonsoul_vise" );
  dmg->add_action( "use_item,name=unchained_gladiators_shackles" );
}
}  // namespace warlock
