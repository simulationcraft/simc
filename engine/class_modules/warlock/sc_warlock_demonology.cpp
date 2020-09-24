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

    if (td->debuffs_from_the_shadows->check() && data().affected_by( td->debuffs_from_the_shadows->data().effectN( 1 ) ) )
    {
      m *= 1.0 + td->debuffs_from_the_shadows->data().effectN(1).percent();
    }
    else if (td->debuffs_from_the_shadows->check() && data().id() == 105174)
    {
      //Hand of Gul'dan spell data is structured weirdly, so 105174 doesn't contain the affecting spells info (see 86040 for that information)
      //Hardcoding checks for ID 105174 HoG to automatically go through for now
      m *= 1.0 + td->debuffs_from_the_shadows->data().effectN( 1 ).percent();
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
  struct umbral_blaze_t : public demonology_spell_t
  {
    umbral_blaze_t( warlock_t* p ) : demonology_spell_t( "Umbral Blaze", p, p->find_spell( 273526 ) )
    {
      base_td      = p->azerite.umbral_blaze.value();  // BFA - Azerite
      hasted_ticks = false;
    }
  };

  int shards_used;
  umbral_blaze_t* blaze;  // BFA - Azerite
  const spell_data_t* summon_spell;
  double borne_of_blood_chance;

  hand_of_guldan_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( p, "Hand of Gul'dan" ),
      shards_used( 0 ),
      blaze( new umbral_blaze_t( p ) ),
      summon_spell( p->find_spell( 104317 ) ),
      borne_of_blood_chance( 0 )
  {
    parse_options( options_str );
    aoe = -1;
    // BFA - Azerite
    if ( p->azerite.umbral_blaze.ok() )
    {
      add_child( blaze );
    }
    parse_effect_data( p->find_spell( 86040 )->effectN( 1 ) );

    //HoG parent spell (105174) is missing affecting spells info (see 86040), so we have to manually apply spec aura.
    //TODO - potentially rebuild HoG to find a way around this issue, as it also affects From the Shadows
    base_multiplier *= 1.0 + p->spec.demonology->effectN( 3 ).percent();

    if ( p->conduit.borne_of_blood->ok() )
      borne_of_blood_chance = p->conduit.borne_of_blood.percent();
  }

  timespan_t travel_time() const override
  {
    return timespan_t::from_millis( 700 );
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
    demonology_spell_t::execute();

    if ( p()->conduit.borne_of_blood->ok() && rng().roll( borne_of_blood_chance ) )
      p()->buffs.demonic_core->trigger();
  }

  double bonus_da( const action_state_t* s ) const override
  {
    double da = demonology_spell_t::bonus_da( s );
    // BFA - Azerite
    da += p()->azerite.demonic_meteor.value();
    return da;
  }

  double action_multiplier() const override
  {
    double m = demonology_spell_t::action_multiplier();

    m *= cost();

    return m;
  }

  void consume_resource() override
  {
    demonology_spell_t::consume_resource();

    shards_used = as<int>( last_resource_cost );

    // BFA - Azerite
    if ( rng().roll( p()->azerite.demonic_meteor.spell_ref().effectN( 2 ).percent() * shards_used ) )
      p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.demonic_meteor );

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

    // BFA - Trinket
    // Only trigger wild imps once for the original target impact.
    // Still keep it in impact instead of execute because of travel delay.
    if ( result_is_hit( s->result ) && s->target == target )
    {
      expansion::bfa::trigger_leyshocks_grand_compilation( STAT_HASTE_RATING, p() );

      for ( int i = 1; i <= shards_used; i++ )
      {
        auto ev = make_event<imp_delay_event_t>( *sim, p(), rng().gauss( 400.0 * i, 50.0 ), 400.0 * i );
        this->p()->wild_imp_spawns.push_back( ev );
      }

      // BFA - Azerite
      if ( p()->azerite.umbral_blaze.ok() && rng().roll( p()->find_spell( 273524 )->proc_chance() ) )
      {
        blaze->set_target( target );
        blaze->execute();
      }
    }
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

  double bonus_da( const action_state_t* s ) const override
  {
    double da = demonology_spell_t::bonus_da( s );

    da += p()->buffs.shadows_bite->check_value();

    return da;
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

    p()->buffs.balespiders_burning_core->expire();
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

    p()->warlock_pet_list.dreadstalkers.spawn( as<unsigned>( dreadstalker_count ) );

    p()->buffs.demonic_calling->up();  // benefit tracking
    p()->buffs.demonic_calling->decrement();

    //TOCHECK: This should really be applied by the pet(s) and not the Warlock?
    if ( p()->talents.from_the_shadows->ok() )
    {
      td( target )->debuffs_from_the_shadows->trigger();
    }
  }
};

struct implosion_t : public demonology_spell_t
{
  struct implosion_aoe_t : public demonology_spell_t
  {
    double casts_left = 5.0;
    pets::warlock_pet_t* next_imp;

    implosion_aoe_t( warlock_t* p ) : demonology_spell_t( "implosion_aoe", p, p->find_spell( 196278 ) )
    {
      aoe                = -1;
      dual               = true;
      background         = true;
      callbacks          = false;
      reduced_aoe_damage = false;

      p->spells.implosion_aoe = this;
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
    // Travel speed is not in spell data, in game test appears to be 40 yds/sec
    travel_speed = 40;
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
    warlock_spell_t::execute();

    auto imps_consumed = p()->warlock_pet_list.wild_imps.n_active_pets();

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
        make_event( sim, 100_ms * launch_counter + this->travel_time(), [ ex, tar, imp ] {
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

    // BFA - Azerite
    if ( p()->azerite.explosive_potential.ok() && imps_consumed >= 3 )
      p()->buffs.explosive_potential->trigger();
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
    // BFA - Essence
    cooldown->duration *= 1.0 + azerite::vision_of_perfection_cdr( p->azerite_essence.vision_of_perfection );
    demonic_consumption_health_percentage = p->find_spell( 267971 )->effectN( 1 ).percent();
  }

  void execute() override
  {
    demonology_spell_t::execute();

    p()->warlock_pet_list.demonic_tyrants.spawn( data().duration() );

    p()->buffs.demonic_power->trigger();

    if ( p()->spec.summon_demonic_tyrant_2->ok() )
      p()->resource_gain( RESOURCE_SOUL_SHARD, p()->spec.summon_demonic_tyrant_2->effectN( 1 ).base_value(),
                          p()->gains.summon_demonic_tyrant );

    // BFA - Azerite
    if ( p()->azerite.baleful_invocation.ok() )
      p()->resource_gain( RESOURCE_SOUL_SHARD, p()->find_spell( 287060 )->effectN( 1 ).base_value() / 10.0,
                          p()->gains.baleful_invocation );

    demonic_consumption_added_damage = 0;

    for ( auto& pet : p()->pet_list )
    {
      auto lock_pet = dynamic_cast<pets::warlock_pet_t*>( pet );

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
        double available = pet->resources.current[ RESOURCE_HEALTH ];

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

    p()->buffs.tyrant->set_duration( data().duration() );
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

  void execute() override
  {
    demonology_spell_t::execute();

    auto imps = p()->warlock_pet_list.wild_imps.active_pets();

    range::sort(
        imps, []( const pets::demonology::wild_imp_pet_t* imp1, const pets::demonology::wild_imp_pet_t* imp2 ) {
          double lv = imp1->resources.current[ RESOURCE_ENERGY ], rv = imp2->resources.current[ RESOURCE_ENERGY ];

          if ( lv == rv )
          {
            timespan_t lr = imp1->expiration->remains(), rr = imp2->expiration->remains();
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

    //TOCHECK: As of 9/21/2020, Power Siphon is bugged to always provide 2 stacks of the damage buff regardless of imps consumed
    //Move this inside the while loop (and remove the 2!) if it is ever changed to be based on imp count
    p()->buffs.power_siphon->trigger(2);

    while ( !imps.empty() )
    {
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

  void tick( dot_t* d ) override
  {
    demonology_spell_t::tick( d );

    expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, p() );
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
    p()->buffs.vilefiend->set_duration( data().duration() );
    p()->buffs.vilefiend->trigger();

    // Spawn a single vilefiend, and grab it's pointer so we can execute an instant bile split
    // TODO: The bile split execution should move to the pet itself, instead of here
    auto vilefiend = p()->warlock_pet_list.vilefiends.spawn( data().duration() ).front();
    vilefiend->bile_spit->set_target( execute_state->target );
    vilefiend->bile_spit->execute();
  }
};

struct grimoire_felguard_t : public summon_pet_t
{
  grimoire_felguard_t( warlock_t* p, util::string_view options_str )
    : summon_pet_t( "grimoire_felguard", p, p->talents.grimoire_felguard )
  {
    parse_options( options_str );
    cooldown->duration = data().cooldown();
    summoning_duration = data().duration() + timespan_t::from_millis( 1 );  // TODO: why?
  }

  void execute() override
  {
    summon_pet_t::execute();
    pet->buffs.grimoire_of_service->trigger();
    p()->buffs.grimoire_felguard->set_duration(
        timespan_t::from_seconds( p()->talents.grimoire_felguard->effectN( 1 ).base_value() ) );
    p()->buffs.grimoire_felguard->trigger();
  }

  void init_finished() override
  {
    if ( pet )
    {
      pet->summon_stats = stats;
    }

    summon_pet_t::init_finished();
  }
};

struct inner_demons_t : public demonology_spell_t
{
  inner_demons_t( warlock_t* p, util::string_view options_str ) : demonology_spell_t( "inner_demons", p )
  {
    parse_options( options_str );
    trigger_gcd           = timespan_t::zero();
    harmful               = false;
    ignore_false_positive = true;
    action_skill          = 1;
  }

  void execute() override
  {
    demonology_spell_t::execute();
    p()->buffs.inner_demons->trigger();
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
    prince_malchezaar  = 8,
    eyes_of_guldan     = 9,
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
    int demon_int = rng().range( 10 );
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
  if ( pet_name == "grimoire_felguard" )
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
  if ( action_name == "inner_demons" )
    return new inner_demons_t( this, options_str );
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
                             expansion::bfa::trigger_leyshocks_grand_compilation( STAT_MASTERY_RATING, this );
                           } );

  buffs.nether_portal =
      make_buff( this, "nether_portal", talents.nether_portal )->set_duration( talents.nether_portal->duration() );

  // Azerite
  buffs.shadows_bite = make_buff( this, "shadows_bite", azerite.shadows_bite )
                           ->set_duration( find_spell( 272945 )->duration() )
                           ->set_default_value( azerite.shadows_bite.value() );
  buffs.supreme_commander = make_buff<stat_buff_t>( this, "supreme_commander", azerite.supreme_commander )
                                ->add_stat( STAT_INTELLECT, azerite.supreme_commander.value() )
                                ->set_duration( find_spell( 279885 )->duration() );
  buffs.explosive_potential = make_buff<stat_buff_t>( this, "explosive_potential", find_spell( 275398 ) )
                                  ->add_stat( STAT_HASTE_RATING, azerite.explosive_potential.value() );

  // Legendaries
  buffs.balespiders_burning_core =
      make_buff( this, "balespiders_burning_core", legendary.balespiders_burning_core->effectN( 1 ).trigger() )
          ->set_trigger_spell( legendary.balespiders_burning_core )
          ->set_default_value( legendary.balespiders_burning_core->effectN( 1 ).trigger()->effectN( 1 ).percent() );

  // to track pets
  buffs.wild_imps = make_buff( this, "wild_imps" )->set_max_stack( 40 );

  buffs.dreadstalkers = make_buff( this, "dreadstalkers" )->set_max_stack( 4 );

  buffs.vilefiend = make_buff( this, "vilefiend" )->set_max_stack( 1 );

  buffs.tyrant = make_buff( this, "tyrant" )->set_max_stack( 1 );

  buffs.grimoire_felguard = make_buff( this, "grimoire_felguard" )->set_max_stack( 1 );

  buffs.prince_malchezaar = make_buff( this, "prince_malchezaar" )->set_max_stack( 1 );

  buffs.eyes_of_guldan = make_buff( this, "eyes_of_guldan" )->set_max_stack( 4 );

  buffs.portal_summons = make_buff( this, "portal_summons" )
                             ->set_duration( timespan_t::from_seconds( 15 ) )
                             ->set_max_stack( 40 )
                             ->set_refresh_behavior( buff_refresh_behavior::DURATION );
}

void warlock_t::vision_of_perfection_proc_demo()
{
  timespan_t summon_duration = find_spell( 265187 )->duration() * vision_of_perfection_multiplier;

  warlock_pet_list.demonic_tyrants.spawn( summon_duration, 1u );

  auto essence         = find_azerite_essence( "Vision of Perfection" );
  timespan_t extension = timespan_t::from_seconds(
      essence.spell_ref( essence.rank(), essence_type::MAJOR ).effectN( 2 ).base_value() / 1000 );

  if ( azerite.baleful_invocation.ok() )
    resource_gain(
        RESOURCE_SOUL_SHARD,
        std::round( find_spell( 287060 )->effectN( 1 ).base_value() / 10.0 * vision_of_perfection_multiplier ),
        gains.baleful_invocation );
  buffs.demonic_power->trigger( 1, buffs.demonic_power->DEFAULT_VALUE(), -1.0, summon_duration );

  // TOCHECK: Azerite traits, does proc tyrant extend summoned tyrant and vice versa?
  for ( auto& pet : pet_list )
  {
    auto lock_pet = dynamic_cast<pets::warlock_pet_t*>( pet );

    if ( lock_pet == nullptr )
      continue;
    if ( lock_pet->is_sleeping() )
      continue;

    if ( lock_pet->pet_type == PET_DEMONIC_TYRANT )
      continue;

    if ( lock_pet->expiration )
    {
      timespan_t new_time                   = lock_pet->expiration->time + extension;
      lock_pet->expiration->reschedule_time = new_time;
    }
  }

  buffs.tyrant->set_duration( std::max( buffs.tyrant->remains(), summon_duration ) );
  buffs.tyrant->trigger( 1, buffs.tyrant->DEFAULT_VALUE(), -1.0, summon_duration );
  if ( buffs.dreadstalkers->check() )
  {
    buffs.dreadstalkers->extend_duration( this, extension );
  }
  if ( buffs.grimoire_felguard->check() )
  {
    buffs.grimoire_felguard->extend_duration( this, extension );
  }
  if ( buffs.vilefiend->check() )
  {
    buffs.vilefiend->extend_duration( this, extension );
  }
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

  // BFA - Azerite
  azerite.demonic_meteor      = find_azerite_spell( "Demonic Meteor" );
  azerite.shadows_bite        = find_azerite_spell( "Shadow's Bite" );
  azerite.supreme_commander   = find_azerite_spell( "Supreme Commander" );
  azerite.umbral_blaze        = find_azerite_spell( "Umbral Blaze" );
  azerite.explosive_potential = find_azerite_spell( "Explosive Potential" );
  azerite.baleful_invocation  = find_azerite_spell( "Baleful Invocation" );

  // Legendaries
  legendary.balespiders_burning_core       = find_runeforge_legendary( "Balespider's Burning Core" );
  legendary.forces_of_horned_nightmare     = find_runeforge_legendary( "Forces of Horned Nighhtmare" );
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
  gains.demonic_meteor        = get_gain( "demonic_meteor" );
  gains.baleful_invocation    = get_gain( "baleful_invocation" );
  gains.summon_demonic_tyrant = get_gain( "summon_demonic_tyrant" );
}

void warlock_t::init_rng_demonology()
{
}

void warlock_t::init_procs_demonology()
{
  procs.dreadstalker_debug  = get_proc( "dreadstalker_debug" );
  procs.summon_random_demon = get_proc( "summon_random_demon" );
}

void warlock_t::create_options_demonology()
{
}

void warlock_t::create_apl_demonology()
{
  action_priority_list_t* def    = get_action_priority_list( "default" );
  action_priority_list_t* np     = get_action_priority_list( "nether_portal" );
  action_priority_list_t* npb    = get_action_priority_list( "nether_portal_building" );
  action_priority_list_t* npa    = get_action_priority_list( "nether_portal_active" );
  action_priority_list_t* bas    = get_action_priority_list( "build_a_shard" );
  action_priority_list_t* imp    = get_action_priority_list( "implosion" );
  action_priority_list_t* opener = get_action_priority_list( "opener" );

  def->add_action(
      "potion,if=pet.demonic_tyrant.active&(!essence.vision_of_perfection.major|!talent.demonic_consumption.enabled|"
      "cooldown.summon_demonic_tyrant.remains>=cooldown.summon_demonic_tyrant.duration-5)&(!talent.nether_portal."
      "enabled|cooldown.nether_portal.remains>160)|target.time_to_die<30" );
  def->add_action(
      "use_item,name=azsharas_font_of_power,if=cooldown.summon_demonic_tyrant.remains<=20&!talent.nether_portal."
      "enabled" );
  def->add_action(
      "use_items,if=pet.demonic_tyrant.active&(!essence.vision_of_perfection.major|!talent.demonic_consumption.enabled|"
      "cooldown.summon_demonic_tyrant.remains>=cooldown.summon_demonic_tyrant.duration-5)|target.time_to_die<=15" );
  def->add_action(
      "berserking,if=pet.demonic_tyrant.active&(!essence.vision_of_perfection.major|!talent.demonic_consumption."
      "enabled|cooldown.summon_demonic_tyrant.remains>=cooldown.summon_demonic_tyrant.duration-5)|target.time_to_die<="
      "15" );
  def->add_action(
      "blood_fury,if=pet.demonic_tyrant.active&(!essence.vision_of_perfection.major|!talent.demonic_consumption."
      "enabled|cooldown.summon_demonic_tyrant.remains>=cooldown.summon_demonic_tyrant.duration-5)|target.time_to_die<="
      "15" );
  def->add_action(
      "fireblood,if=pet.demonic_tyrant.active&(!essence.vision_of_perfection.major|!talent.demonic_consumption.enabled|"
      "cooldown.summon_demonic_tyrant.remains>=cooldown.summon_demonic_tyrant.duration-5)|target.time_to_die<=15" );
  def->add_action(
      "blood_of_the_enemy,if=pet.demonic_tyrant.active&pet.demonic_tyrant.remains<=15-gcd*3&(!essence.vision_of_"
      "perfection.major|!talent.demonic_consumption.enabled|cooldown.summon_demonic_tyrant.remains>=cooldown.summon_"
      "demonic_tyrant.duration-5)" );
  def->add_action(
      "worldvein_resonance,if=(pet.demonic_tyrant.active&(!essence.vision_of_perfection.major|!talent.demonic_"
      "consumption.enabled|cooldown.summon_demonic_tyrant.remains>=cooldown.summon_demonic_tyrant.duration-5)|target."
      "time_to_die<=15)" );
  def->add_action(
      "ripple_in_space,if=pet.demonic_tyrant.active&(!essence.vision_of_perfection.major|!talent.demonic_consumption."
      "enabled|cooldown.summon_demonic_tyrant.remains>=cooldown.summon_demonic_tyrant.duration-5)|target.time_to_die<="
      "15" );
  def->add_action(
      "use_item,name=pocketsized_computation_device,if=cooldown.summon_demonic_tyrant.remains>=20&cooldown.summon_"
      "demonic_tyrant.remains<=cooldown.summon_demonic_tyrant.duration-15|target.time_to_die<=30" );
  def->add_action(
      "use_item,name=rotcrusted_voodoo_doll,if=(cooldown.summon_demonic_tyrant.remains>=25|target.time_to_die<=30)" );
  def->add_action(
      "use_item,name=shiver_venom_relic,if=(cooldown.summon_demonic_tyrant.remains>=25|target.time_to_die<=30)" );
  def->add_action(
      "use_item,name=aquipotent_nautilus,if=(cooldown.summon_demonic_tyrant.remains>=25|target.time_to_die<=30)" );
  def->add_action(
      "use_item,name=tidestorm_codex,if=(cooldown.summon_demonic_tyrant.remains>=25|target.time_to_die<=30)" );
  def->add_action(
      "use_item,name=vial_of_storms,if=(cooldown.summon_demonic_tyrant.remains>=25|target.time_to_die<=30)" );
  def->add_action(
      "call_action_list,name=opener,if=!talent.nether_portal.enabled&time<30&!cooldown.summon_demonic_tyrant.remains" );
  def->add_action(
      "use_item,name=azsharas_font_of_power,if=(time>30|!talent.nether_portal.enabled)&talent.grimoire_felguard."
      "enabled&(target.time_to_die>120|target.time_to_die<cooldown.summon_demonic_tyrant.remains+15)|target.time_to_"
      "die<=35" );
  def->add_action(
      "hand_of_guldan,if=azerite.explosive_potential.rank&time<5&soul_shard>2&buff.explosive_potential.down&buff.wild_"
      "imps.stack<3&!prev_gcd.1.hand_of_guldan&&!prev_gcd.2.hand_of_guldan" );
  def->add_action( "demonbolt,if=soul_shard<=3&buff.demonic_core.up&buff.demonic_core.stack=4" );
  def->add_action(
      "implosion,if=azerite.explosive_potential.rank&buff.wild_imps.stack>2&buff.explosive_potential.remains<action."
      "shadow_bolt.execute_time&(!talent.demonic_consumption.enabled|cooldown.summon_demonic_tyrant.remains>12)" );
  def->add_action( "doom,if=!ticking&time_to_die>30&spell_targets.implosion<2&!buff.nether_portal.remains" );
  def->add_action(
      "bilescourge_bombers,if=azerite.explosive_potential.rank>0&time<10&spell_targets.implosion<2&buff.dreadstalkers."
      "remains&talent.nether_portal.enabled" );
  def->add_action( "demonic_strength,if=(buff.wild_imps.stack<6|buff.demonic_power.up)|spell_targets.implosion<2" );
  def->add_action( "call_action_list,name=nether_portal,if=talent.nether_portal.enabled&spell_targets.implosion<=2" );
  def->add_action( "call_action_list,name=implosion,if=spell_targets.implosion>1" );
  def->add_action( "guardian_of_azeroth,if=cooldown.summon_demonic_tyrant.remains<=15|target.time_to_die<=30" );
  def->add_action(
      "grimoire_felguard,if=(target.time_to_die>120|target.time_to_die<cooldown.summon_demonic_tyrant.remains+15|"
      "cooldown.summon_demonic_tyrant.remains<13)" );
  def->add_action(
      "summon_vilefiend,if=cooldown.summon_demonic_tyrant.remains>40|cooldown.summon_demonic_tyrant.remains<12" );
  def->add_action(
      "call_dreadstalkers,if=(cooldown.summon_demonic_tyrant.remains<9&buff.demonic_calling.remains)|(cooldown.summon_"
      "demonic_tyrant.remains<11&!buff.demonic_calling.remains)|cooldown.summon_demonic_tyrant.remains>14" );
  def->add_action( "the_unbound_force,if=buff.reckless_force.react" );
  def->add_action( "bilescourge_bombers" );
  def->add_action(
      "hand_of_guldan,if=(azerite.baleful_invocation.enabled|talent.demonic_consumption.enabled)&prev_gcd.1.hand_of_"
      "guldan&cooldown.summon_demonic_tyrant.remains<2" );
  def->add_action( this, "Summon Demonic Tyrant",
                   "if=soul_shard<3&(!talent.demonic_consumption.enabled|buff.wild_imps.stack+imps_spawned_during.2000%"
                   "spell_haste>=6&time_to_imps.all.remains<cast_time)|target.time_to_die<20",
                   "2000%spell_haste is shorthand for the cast time of Demonic Tyrant. The intent is to only begin "
                   "casting if a certain number of imps will be out by the end of the cast." );
  def->add_action(
      "power_siphon,if=buff.wild_imps.stack>=2&buff.demonic_core.stack<=2&buff.demonic_power.down&spell_targets."
      "implosion<2" );
  def->add_action( "doom,if=talent.doom.enabled&refreshable&time_to_die>(dot.doom.remains+30)" );
  def->add_action(
      "hand_of_guldan,if=soul_shard>=5|(soul_shard>=3&cooldown.call_dreadstalkers.remains>4&(cooldown.summon_demonic_"
      "tyrant.remains>20|(cooldown.summon_demonic_tyrant.remains<gcd*2&talent.demonic_consumption.enabled|cooldown."
      "summon_demonic_tyrant.remains<gcd*4&!talent.demonic_consumption.enabled))&(!talent.summon_vilefiend.enabled|"
      "cooldown.summon_vilefiend.remains>3))" );
  def->add_action( "soul_strike,if=soul_shard<5&buff.demonic_core.stack<=2" );
  def->add_action(
      "demonbolt,if=soul_shard<=3&buff.demonic_core.up&((cooldown.summon_demonic_tyrant.remains<6|cooldown.summon_"
      "demonic_tyrant.remains>22&!azerite.shadows_bite.enabled)|buff.demonic_core.stack>=3|buff.demonic_core.remains<5|"
      "time_to_die<25|buff.shadows_bite.remains)" );
  def->add_action( "focused_azerite_beam,if=!pet.demonic_tyrant.active" );
  def->add_action( "purifying_blast" );
  def->add_action( "blood_of_the_enemy" );
  def->add_action(
      "concentrated_flame,if=!dot.concentrated_flame_burn.remains&!action.concentrated_flame.in_flight&!pet.demonic_"
      "tyrant.active" );
  def->add_action( "reaping_flames,if=!pet.demonic_tyrant.active" );
  def->add_action( "call_action_list,name=build_a_shard" );

  np->add_action( "call_action_list,name=nether_portal_building,if=cooldown.nether_portal.remains<20" );
  np->add_action( "call_action_list,name=nether_portal_active,if=cooldown.nether_portal.remains>165" );

  npa->add_action( "bilescourge_bombers" );
  npa->add_action( "grimoire_felguard,if=cooldown.summon_demonic_tyrant.remains<13|!equipped.132369" );
  npa->add_action(
      "summon_vilefiend,if=cooldown.summon_demonic_tyrant.remains>40|cooldown.summon_demonic_tyrant.remains<12" );
  npa->add_action(
      "call_dreadstalkers,if=(cooldown.summon_demonic_tyrant.remains<9&buff.demonic_calling.remains)|(cooldown.summon_"
      "demonic_tyrant.remains<11&!buff.demonic_calling.remains)|cooldown.summon_demonic_tyrant.remains>14" );
  npa->add_action(
      "call_action_list,name=build_a_shard,if=soul_shard=1&(cooldown.call_dreadstalkers.remains<action.shadow_bolt."
      "cast_time|(talent.bilescourge_bombers.enabled&cooldown.bilescourge_bombers.remains<action.shadow_bolt.cast_time)"
      ")" );
  npa->add_action(
      "hand_of_guldan,if=((cooldown.call_dreadstalkers.remains>action.demonbolt.cast_time)&(cooldown.call_"
      "dreadstalkers.remains>action.shadow_bolt.cast_time))&cooldown.nether_portal.remains>(165+action.hand_of_guldan."
      "cast_time)" );
  npa->add_action( "summon_demonic_tyrant,if=buff.nether_portal.remains<5&soul_shard=0" );
  npa->add_action( "summon_demonic_tyrant,if=buff.nether_portal.remains<action.summon_demonic_tyrant.cast_time+0.5" );
  npa->add_action( "demonbolt,if=buff.demonic_core.up&soul_shard<=3" );
  npa->add_action( "call_action_list,name=build_a_shard" );

  npb->add_action( "use_item,name=azsharas_font_of_power,if=cooldown.nether_portal.remains<=5*spell_haste" );
  npb->add_action( "guardian_of_azeroth,if=!cooldown.nether_portal.remains&soul_shard>=5" );
  npb->add_action( "nether_portal,if=soul_shard>=5" );
  npb->add_action( "call_dreadstalkers,if=time>=30" );
  npb->add_action( "hand_of_guldan,if=time>=30&cooldown.call_dreadstalkers.remains>18&soul_shard>=3" );
  npb->add_action(
      "power_siphon,if=time>=30&buff.wild_imps.stack>=2&buff.demonic_core.stack<=2&buff.demonic_power.down&soul_shard>="
      "3" );
  npb->add_action( "hand_of_guldan,if=time>=30&soul_shard>=5" );
  npb->add_action( "call_action_list,name=build_a_shard" );

  opener->add_action( "hand_of_guldan,line_cd=30,if=azerite.explosive_potential.enabled" );
  opener->add_action(
      "implosion,if=azerite.explosive_potential.enabled&buff.wild_imps.stack>2&buff.explosive_potential.down" );
  opener->add_action( "doom,line_cd=30" );
  opener->add_action( "guardian_of_azeroth" );
  opener->add_action( "hand_of_guldan,if=prev_gcd.1.hand_of_guldan&soul_shard>0&prev_gcd.2.soul_strike" );
  opener->add_action(
      "demonic_strength,if=prev_gcd.1.hand_of_guldan&!prev_gcd.2.hand_of_guldan&(buff.wild_imps.stack>1&action.hand_of_"
      "guldan.in_flight)" );
  opener->add_action( "bilescourge_bombers" );
  opener->add_action( "soul_strike,line_cd=30,if=!buff.bloodlust.remains|time>5&prev_gcd.1.hand_of_guldan" );
  opener->add_action( "summon_vilefiend,if=soul_shard=5" );
  opener->add_action( "grimoire_felguard,if=soul_shard=5" );
  opener->add_action( "call_dreadstalkers,if=soul_shard=5" );
  opener->add_action( "hand_of_guldan,if=soul_shard=5" );
  opener->add_action(
      "hand_of_guldan,if=soul_shard>=3&prev_gcd.2.hand_of_guldan&time>5&(prev_gcd.1.soul_strike|!talent.soul_strike."
      "enabled&prev_gcd.1.shadow_bolt)" );
  opener->add_action( this, "Summon Demonic Tyrant",
                      "if=prev_gcd.1.demonic_strength|prev_gcd.1.hand_of_guldan&prev_gcd.2.hand_of_guldan|!talent."
                      "demonic_strength.enabled&buff.wild_imps.stack+imps_spawned_during.2000%spell_haste>=6",
                      "2000%spell_haste is shorthand for the cast time of Demonic Tyrant. The intent is to only begin "
                      "casting if a certain number of imps will be out by the end of the cast." );
  opener->add_action( "demonbolt,if=soul_shard<=3&buff.demonic_core.remains" );
  opener->add_action( "call_action_list,name=build_a_shard" );

  imp->add_action(
      "implosion,if=(buff.wild_imps.stack>=6&(soul_shard<3|prev_gcd.1.call_dreadstalkers|buff.wild_imps.stack>=9|prev_"
      "gcd.1.bilescourge_bombers|(!prev_gcd.1.hand_of_guldan&!prev_gcd.2.hand_of_guldan))&!prev_gcd.1.hand_of_guldan&!"
      "prev_gcd.2.hand_of_guldan&buff.demonic_power.down)|(time_to_die<3&buff.wild_imps.stack>0)|(prev_gcd.2.call_"
      "dreadstalkers&buff.wild_imps.stack>2&!talent.demonic_calling.enabled)" );
  imp->add_action( "grimoire_felguard,if=cooldown.summon_demonic_tyrant.remains<13|!equipped.132369" );
  imp->add_action(
      "call_dreadstalkers,if=(cooldown.summon_demonic_tyrant.remains<9&buff.demonic_calling.remains)|(cooldown.summon_"
      "demonic_tyrant.remains<11&!buff.demonic_calling.remains)|cooldown.summon_demonic_tyrant.remains>14" );
  imp->add_action( "summon_demonic_tyrant" );
  imp->add_action( "hand_of_guldan,if=soul_shard>=5" );
  imp->add_action(
      "hand_of_guldan,if=soul_shard>=3&(((prev_gcd.2.hand_of_guldan|buff.wild_imps.stack>=3)&buff.wild_imps.stack<9)|"
      "cooldown.summon_demonic_tyrant.remains<=gcd*2|buff.demonic_power.remains>gcd*2)" );
  imp->add_action(
      "demonbolt,if=prev_gcd.1.hand_of_guldan&soul_shard>=1&(buff.wild_imps.stack<=3|prev_gcd.3.hand_of_guldan)&soul_"
      "shard<4&buff.demonic_core.up" );
  imp->add_action(
      "summon_vilefiend,if=(cooldown.summon_demonic_tyrant.remains>40&spell_targets.implosion<=2)|cooldown.summon_"
      "demonic_tyrant.remains<12" );
  imp->add_action( "bilescourge_bombers,if=cooldown.summon_demonic_tyrant.remains>9" );
  imp->add_action( "focused_azerite_beam" );
  imp->add_action( "purifying_blast" );
  imp->add_action( "blood_of_the_enemy" );
  imp->add_action(
      "concentrated_flame,if=!dot.concentrated_flame_burn.remains&!action.concentrated_flame.in_flight&spell_targets."
      "implosion<5" );
  imp->add_action( "soul_strike,if=soul_shard<5&buff.demonic_core.stack<=2" );
  imp->add_action(
      "demonbolt,if=soul_shard<=3&buff.demonic_core.up&(buff.demonic_core.stack>=3|buff.demonic_core.remains<=gcd*5."
      "7)" );
  imp->add_action( "doom,cycle_targets=1,max_cycle_targets=7,if=refreshable" );
  imp->add_action( "call_action_list,name=build_a_shard" );

  bas->add_action( "memory_of_lucid_dreams,if=soul_shard<2" );
  bas->add_action(
      "soul_strike,if=!talent.demonic_consumption.enabled|time>15|prev_gcd.1.hand_of_guldan&!buff.bloodlust.remains" );
  bas->add_action( "shadow_bolt" );
}
}  // namespace warlock
