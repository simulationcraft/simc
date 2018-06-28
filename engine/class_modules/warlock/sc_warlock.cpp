#include "simulationcraft.hpp"
#include "sc_warlock.hpp"
namespace warlock
{ 
// Pets
  namespace pets
  {
    warlock_pet_t::warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian ) :
      pet_t( sim, owner, pet_name, pt, guardian ),
      special_action( nullptr ),
      special_action_two( nullptr ),
      melee_attack( nullptr ),
      summon_stats( nullptr ),
      ascendance( nullptr ),
      buffs(),
      active()
    {
      owner_coeff.ap_from_sp = 0.5;
      owner_coeff.sp_from_sp = 1.0;
      owner_coeff.health = 0.5;
    }

    namespace felhunter
    {
      struct shadow_bite_t : public warlock_pet_melee_attack_t
      {
        shadow_bite_t(warlock_pet_t* p) :
          warlock_pet_melee_attack_t(p, "Shadow Bite") 
        {
          
        }
      };

      felhunter_pet_t::felhunter_pet_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_FELHUNTER, name != "felhunter")
      {
        action_list_str = "shadow_bite";
      }

      void felhunter_pet_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        melee_attack = new warlock_pet_melee_t(this);
      }

      action_t* felhunter_pet_t::create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "shadow_bite") return new shadow_bite_t(this);
        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace imp
    {
      struct firebolt_t : public warlock_pet_spell_t
      {
        firebolt_t(warlock_pet_t* p) :
          warlock_pet_spell_t("Firebolt", p, p -> find_spell(3110)) { }
      };

      imp_pet_t::imp_pet_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_IMP, name != "imp")
      {
        action_list_str = "firebolt";
      }

      action_t* imp_pet_t::create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "firebolt") return new firebolt_t(this);
        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace succubus
    {
      struct lash_of_pain_t : public warlock_pet_spell_t
      {
        lash_of_pain_t(warlock_pet_t* p) :
          warlock_pet_spell_t(p, "Lash of Pain") { }
      };

      struct whiplash_t : public warlock_pet_spell_t
      {
        whiplash_t(warlock_pet_t* p) :
          warlock_pet_spell_t(p, "Whiplash")
        {
          aoe = -1;
        }
      };

      succubus_pet_t::succubus_pet_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_SUCCUBUS, name != "succubus")
      {
        main_hand_weapon.swing_time = timespan_t::from_seconds(3.0);
        action_list_str = "lash_of_pain";
      }

      void succubus_pet_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();

        main_hand_weapon.swing_time = timespan_t::from_seconds(3.0);
        melee_attack = new warlock_pet_melee_t(this);
      }

      action_t* succubus_pet_t::create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "lash_of_pain") return new lash_of_pain_t(this);

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace voidwalker
    {
      struct torment_t :
        public warlock_pet_spell_t
      {
        torment_t(warlock_pet_t* p) :
          warlock_pet_spell_t(p, "Torment") { }
      };

      voidwalker_pet_t::voidwalker_pet_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_VOIDWALKER, name != "voidwalker")
      {
        action_list_str = "torment";
      }

      void voidwalker_pet_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        melee_attack = new warlock_pet_melee_t(this);
      }

      action_t* voidwalker_pet_t::create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "torment") return new torment_t(this);
        return warlock_pet_t::create_action(name, options_str);
      }
    }

    void warlock_pet_t::init_base_stats()
    {
      pet_t::init_base_stats();

      resources.base[RESOURCE_ENERGY] = 200;
      resources.base_regen_per_second[ RESOURCE_ENERGY ] = 10;

      base.spell_power_per_intellect = 1;

      intellect_per_owner = 0;
      stamina_per_owner = 0;

      main_hand_weapon.type = WEAPON_BEAST;
      main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    }

    bool warlock_pet_t::create_actions()
    {
      bool check = pet_t::create_actions();
      if ( check )
      {
        return true;
      }
      else
        return false;
    }

    void warlock_pet_t::init_action_list()
    {
      if ( special_action )
      {
        if ( type == PLAYER_PET )
          special_action->background = true;
        else
          special_action->action_list = get_action_priority_list( "default" );
      }

      if ( special_action_two )
      {
        if ( type == PLAYER_PET )
          special_action_two->background = true;
        else
          special_action_two->action_list = get_action_priority_list( "default" );
      }

      pet_t::init_action_list();

      if ( summon_stats )
        for ( size_t i = 0; i < action_list.size(); ++i )
          summon_stats->add_child( action_list[i]->stats );
    }

    void warlock_pet_t::create_buffs()
    {
      pet_t::create_buffs();

      create_buffs_demonology();
      create_buffs_destruction();

      buffs.rage_of_guldan = make_buff( this, "rage_of_guldan", find_spell( 257926 ) )->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ); //change spell id to 253014 when whitelisted
    }

    void warlock_pet_t::init_spells()
    {
      pet_t::init_spells();

      if (o()->specialization() == WARLOCK_DEMONOLOGY)
        init_spells_demonology();
    }

    void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
    {
      dot_t* d;
      if ( melee_attack && !melee_attack->execute_event && !( special_action && ( d = special_action->get_dot() ) && d->is_ticking() ) )
      {
        melee_attack->schedule_execute();
      }

      pet_t::schedule_ready( delta_time, waiting );
    }

    double warlock_pet_t::composite_player_multiplier( school_e school ) const
    {
      double m = pet_t::composite_player_multiplier( school );

      if (o()->specialization() == WARLOCK_DEMONOLOGY)
      {
        m *= 1.0 + o()->cache.mastery_value();
        if (o()->buffs.demonic_power->check())
        {
          m *= 1.0 + o()->buffs.demonic_power->default_value;
        }
      }

      if ( buffs.rage_of_guldan->check() )
        m *= 1.0 + ( buffs.rage_of_guldan->default_value / 100 );

      if ( buffs.grimoire_of_service->check() )
      {
        m *= 1.0 + buffs.grimoire_of_service->data().effectN( 1 ).percent();
      }

      m *= 1.0 + o()->buffs.sindorei_spite->check_stack_value();
      m *= 1.0 + o()->buffs.lessons_of_spacetime->check_stack_value();

      return m;
    }

    double warlock_pet_t::composite_melee_crit_chance() const
    {
      double mc = pet_t::composite_melee_crit_chance();
      return mc;
    }

    double warlock_pet_t::composite_spell_crit_chance() const
    {
      double sc = pet_t::composite_spell_crit_chance();
      return sc;
    }

    double warlock_pet_t::composite_melee_haste() const
    {
      double mh = pet_t::composite_melee_haste();
      return mh;
    }

    double warlock_pet_t::composite_spell_haste() const
    {
      double sh = pet_t::composite_spell_haste();
      return sh;
    }

    double warlock_pet_t::composite_melee_speed() const
    {
      double cmh = pet_t::composite_melee_speed();
      return cmh;
    }

    double warlock_pet_t::composite_spell_speed() const
    {
      double css = pet_t::composite_spell_speed();
      return css;
    }
  } // end namespace pets

  void parse_spell_coefficient( action_t& a )
  {
    for ( size_t i = 1; i <= a.data()._effects->size(); i++ )
    {
      if ( a.data().effectN( i ).type() == E_SCHOOL_DAMAGE )
        a.spell_power_mod.direct = a.data().effectN( i ).sp_coeff();
      else if ( a.data().effectN( i ).type() == E_APPLY_AURA && a.data().effectN( i ).subtype() == A_PERIODIC_DAMAGE )
        a.spell_power_mod.tick = a.data().effectN( i ).sp_coeff();
    }
  }
// Spells
  namespace actions
  {
    void warlock_spell_t::consume_resource()
    {
      spell_t::consume_resource();

      if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
      {
        if ( p()->talents.soul_conduit->ok() )
        {
          if ( p()->specialization() == WARLOCK_DEMONOLOGY )
          {
            struct demo_sc_event :
              public player_event_t
            {
              gain_t* shard_gain;
              warlock_t* pl;
              int shards_used;

              demo_sc_event( warlock_t* p, int c ) :
                player_event_t( *p, timespan_t::from_millis( 100 ) ), shard_gain( p -> gains.soul_conduit ), pl( p ), shards_used( c ) { }

              virtual const char* name() const override
              {
                return "demonology_sc_event";
              }

              virtual void execute() override
              {
                double soul_conduit_rng = pl->talents.soul_conduit->effectN( 1 ).percent() + pl->spec.destruction->effectN( 4 ).percent();

                for ( int i = 0; i < shards_used; i++ ) {
                  if ( rng().roll( soul_conduit_rng ) ) {
                    pl->resource_gain( RESOURCE_SOUL_SHARD, 1.0, pl->gains.soul_conduit );
                    pl->procs.soul_conduit->occur();
                  }
                }
              }
            };

            make_event<demo_sc_event>( *p()->sim, p(), as<int>(last_resource_cost) );
          }

          else
          {
            double soul_conduit_rng = p()->talents.soul_conduit->effectN( 1 ).percent() + p()->spec.destruction->effectN( 4 ).percent();

            for ( int i = 0; i < last_resource_cost; i++ )
            {
              if ( rng().roll( soul_conduit_rng ) )
              {
                p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.soul_conduit );
                p()->procs.soul_conduit->occur();
              }
            }
          }
        }

        if ( p()->legendary.wakeners_loyalty_enabled && p()->specialization() == WARLOCK_DEMONOLOGY )
        {
          for ( int i = 0; i < last_resource_cost; i++ )
          {
            p()->buffs.wakeners_loyalty->trigger();
          }
        }

        p()->buffs.demonic_speed->trigger();

        if (p()->talents.grimoire_of_supremacy->ok())
        {
          for (auto& infernal : p()->warlock_pet_list.infernals)
          {
            if (!infernal->is_sleeping())
            {
              p()->buffs.grimoire_of_supremacy->trigger(as<int>(last_resource_cost));
            }
          }
        }

        if (p()->buffs.nether_portal->up())
        {
          p()->active.summon_random_demon->execute();
          p()->buffs.portal_summons->trigger();
          p()->procs.portal_summon->occur();
        }

        if (p()->talents.soul_fire->ok())
          p()->cooldowns.soul_fire->adjust((-1 * (p()->talents.soul_fire->effectN(2).time_value()*last_resource_cost)));
      }
    }

    struct drain_life_t : public warlock_spell_t
    {
      double inevitable_demise;

      drain_life_t( warlock_t* p, const std::string& options_str ) :
        warlock_spell_t( p, "Drain Life" )
      {
        parse_options( options_str );
        channeled = true;
        hasted_ticks = false;
        may_crit = false;
      }

      double bonus_ta(const action_state_t* s) const override
      {
        double ta = warlock_spell_t::bonus_ta(s);
        ta += inevitable_demise;
        return ta;
      }

      void execute() override
      {
        inevitable_demise = p()->buffs.inevitable_demise->check_stack_value();
        warlock_spell_t::execute();
        p()->buffs.inevitable_demise->expire();
      }
    };

    struct grimoire_of_sacrifice_t : public warlock_spell_t
    {
      grimoire_of_sacrifice_t( warlock_t* p, const std::string& options_str ) :
        warlock_spell_t( "grimoire_of_sacrifice", p, p -> talents.grimoire_of_sacrifice )
      {
        parse_options( options_str );
        harmful = false;
        ignore_false_positive = true;
      }

      bool ready() override
      {
        if ( !p()->warlock_pet_list.active ) return false;

        return warlock_spell_t::ready();
      }

      void execute() override
      {
        warlock_spell_t::execute();

        if ( p()->warlock_pet_list.active )
        {
          p()->warlock_pet_list.active->dismiss();
          p()->warlock_pet_list.active = nullptr;
          p()->buffs.grimoire_of_sacrifice->trigger();
        }
      }
    };

    struct grimoire_of_sacrifice_damage_t : public warlock_spell_t
    {
      grimoire_of_sacrifice_damage_t( warlock_t* p ) :
        warlock_spell_t( "grimoire_of_sacrifice", p, p -> find_spell( 196100 ) )
      {
        background = true;
        proc = true;
        destro_mastery = false;
      }
    };
  } // end actions namespace

  namespace buffs
  {
    struct debuff_havoc_t : public warlock_buff_t<buff_t>
    {
      debuff_havoc_t( warlock_td_t& p ) :
        base_t( p, "havoc", p.source -> find_spell( 80240 ) ) { }

      void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
      {
        base_t::expire_override( expiration_stacks, remaining_duration );

        p()->havoc_target = nullptr;
      }
    };
  }

  warlock_td_t::warlock_td_t( player_t* target, warlock_t& p ) :
    actor_target_data_t( target, &p ),
    agony_stack( 1 ),
    soc_threshold( 0.0 ),
    warlock( p )
  {
    dots_drain_life = target->get_dot( "drain_life", &p );
    
    //Aff
    dots_corruption = target->get_dot("corruption", &p);
    dots_agony = target->get_dot("agony", &p);
    for (unsigned i = 0; i < dots_unstable_affliction.size(); ++i)
    {
      dots_unstable_affliction[i] = target->get_dot("unstable_affliction_" + std::to_string(i + 1), &p);
    }
    dots_drain_soul = target->get_dot("drain_soul", &p);
    dots_phantom_singularity = target->get_dot("phantom_singularity", &p);
    dots_siphon_life = target->get_dot("siphon_life", &p);
    dots_seed_of_corruption = target->get_dot("seed_of_corruption", &p);
    dots_vile_taint = target->get_dot("vile_taint", &p);

    debuffs_haunt = make_buff( *this, "haunt", source->find_spell( 48181 ) )->set_refresh_behavior( buff_refresh_behavior::PANDEMIC );
    debuffs_shadow_embrace = make_buff( *this, "shadow_embrace", source->find_spell( 32390 ) )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_max_stack( 3 );
    debuffs_agony = make_buff( *this, "agony", source->find_spell( 980 ) )
      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
      ->set_max_stack( ( warlock.talents.writhe_in_agony->ok() ? warlock.talents.writhe_in_agony->effectN( 2 ).base_value() : 10 ) );
    debuffs_tormented_agony = make_buff(*this, "tormented_agony", source->find_spell(252938));

    //Destro
    dots_immolate = target->get_dot("immolate", &p);
    dots_roaring_blaze = target->get_dot("roaring_blaze", &p);
    dots_channel_demonfire = target->get_dot("channel_demonfire", &p);

    debuffs_eradication = make_buff( *this, "eradication", source->find_spell( 196414 ) )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION );
    debuffs_roaring_blaze = make_buff( *this, "roaring_blaze", source->find_spell( 205690 ) )
      ->set_max_stack( 100 );
    debuffs_shadowburn = make_buff(*this, "shadowburn", source->find_spell(17877));
    debuffs_havoc = new buffs::debuff_havoc_t(*this);
    debuffs_chaotic_flames = make_buff(*this, "chaotic_flames", source->find_spell(253092));

    //Demo
    dots_doom = target->get_dot("doom", &p);
    dots_umbral_blaze = target->get_dot("umbral_blaze", &p);

    debuffs_jaws_of_shadow = make_buff( *this, "jaws_of_shadow", source->find_spell( 242922 ) );
    debuffs_from_the_shadows = make_buff(*this, "from_the_shadows", source->find_spell(270569));

    target->callbacks_on_demise.push_back( [this]( player_t* ) { target_demise(); } );
  }

  void warlock_td_t::target_demise()
  {
    if ( !( target->is_enemy() ) )
    {
      return;
    }

    if ( warlock.specialization() == WARLOCK_AFFLICTION )
    {
      for ( auto& current_ua : dots_unstable_affliction )
      {
        if ( current_ua->is_ticking() )
        {
          warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from unstable affliction.", target->name(), warlock.name() );

          warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.unstable_affliction_refund );

          // you can only get one soul shard per death from UA refunds
          break;
        }
      }

      if (dots_drain_soul->is_ticking())
      {
        warlock.resource_gain(RESOURCE_SOUL_SHARD, 1, warlock.gains.drain_soul);
      }
    }

    if ( debuffs_haunt->check() )
    {
      warlock.sim->print_log( "Player {} demised. Warlock {} reset haunt's cooldown.", target->name(), warlock.name() );

      warlock.cooldowns.haunt->reset( true );
    }

    if (debuffs_shadowburn->check())
    {
      warlock.sim->print_log("Player {} demised. Warlock {} reset haunt's cooldown.", target->name(), warlock.name());

      warlock.cooldowns.shadowburn->reset(true);
    }
  }

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_e r ):
  player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    wracking_brilliance(false),
    agony_accumulator( 0.0 ),
    warlock_pet_list(),
    active(),
    talents(),
    legendary(),
    mastery_spells(),
    cooldowns(),
    spec(),
    buffs(),
    gains(),
    procs(),
    spells(),
    initial_soul_shards( 3 ),
    allow_sephuz( false ),
    deaths_embrace_fixed_time(),
    default_pet(),
    shard_react( timespan_t::zero() )
  {
    cooldowns.haunt = get_cooldown( "haunt" );
    cooldowns.shadowburn = get_cooldown("shadowburn");
    cooldowns.soul_fire = get_cooldown("soul_fire");
    cooldowns.sindorei_spite_icd = get_cooldown( "sindorei_spite_icd" );
    cooldowns.call_dreadstalkers = get_cooldown("call_dreadstalkers");
    cooldowns.darkglare = get_cooldown("summon_darkglare");

    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_SPELL_HASTE] = true;

    talent_points.register_validity_fn( [this]( const spell_data_t* spell )
    {
      if ( find_item( 151649 ) ) // Soul of the Netherlord
      {
        switch ( specialization() )
        {
          case WARLOCK_AFFLICTION:
            return spell -> id() == 234876; // Death's Embrace
          case WARLOCK_DEMONOLOGY:
            return spell -> id() == 171975; // Shadowy Inspiration
          case WARLOCK_DESTRUCTION:
            return spell -> id() == 196412; // Eradication
          default:
            return false;
        }
      }

      return false;
    } );
  }

void warlock_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery_spells.master_demonologist->ok() )
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    break;

  default: break;
  }
}

double warlock_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  const warlock_td_t* td = get_target_data( target );

  if (specialization() == WARLOCK_AFFLICTION) {
    if (td->debuffs_haunt->check())
      m *= 1.0 + td->debuffs_haunt->data().effectN(2).percent();
    if (td->debuffs_shadow_embrace->check())
      m *= 1.0 + (td->debuffs_shadow_embrace->data().effectN(1).percent() * td->debuffs_shadow_embrace->check());

    for (auto& current_ua : td->dots_unstable_affliction)
    {
      if (current_ua->is_ticking())
      {
        m *= 1.0 + spec.unstable_affliction->effectN(3).percent();
        break;
      }
    }
  }

  if (td->debuffs_from_the_shadows->check() && school == SCHOOL_SHADOWFLAME)
  {
    m *= 1.0 + td->debuffs_from_the_shadows->data().effectN(1).percent();
  }

  return m;
}

double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( legendary.stretens_insanity )
    m *= 1.0 + buffs.stretens_insanity->check() * buffs.stretens_insanity->data().effectN( 1 ).percent();

  if ( specialization() == WARLOCK_DESTRUCTION && dbc::is_school( school, SCHOOL_FIRE ) ) {
    m *= 1.0 + buffs.alythesss_pyrogenics->check_stack_value();
  }

  m *= 1.0 + buffs.sindorei_spite->check_stack_value();
  m *= 1.0 + buffs.lessons_of_spacetime->check_stack_value();

  return m;
}

double warlock_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  if (buffs.dark_soul_instability->check())
    sc += buffs.dark_soul_instability->check_value();

  return sc;
}

double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.sephuzs_secret->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.sephuzs_secret->check_value() );
  }

  if ( legendary.sephuzs_secret )
  {
    h *= 1.0 / ( 1.0 + legendary.sephuzs_passive );
  }

  if ( buffs.demonic_speed->check() )
    h *= 1.0 / ( 1.0 + buffs.demonic_speed->check_value() );

  if (buffs.dark_soul_misery->check())
    h *= 1.0 / (1.0 + buffs.dark_soul_misery->check_value());

  if (buffs.reverse_entropy->check())
    h *= 1.0 / (1.0 + buffs.reverse_entropy->check_value());

  if (specialization() == WARLOCK_DEMONOLOGY)
  {
    if (buffs.dreaded_haste->check())
      h *= 1.0 / (1.0 + buffs.dreaded_haste->check_value());
  }

  return h;
}

double warlock_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.sephuzs_secret->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.sephuzs_secret->check_value() );
  }

  if ( legendary.sephuzs_secret )
  {
    h *= 1.0 / ( 1.0 + legendary.sephuzs_passive );
  }

  if ( buffs.demonic_speed->check() )
    h *= 1.0 / ( 1.0 + buffs.demonic_speed->check_value() );

  if (buffs.reverse_entropy->check())
    h *= 1.0 / (1.0 + buffs.reverse_entropy->check_value());

  if (specialization() == WARLOCK_DEMONOLOGY)
  {
    if (buffs.dreaded_haste->check())
      h *= 1.0 / (1.0 + buffs.dreaded_haste->check_value());
  }

  return h;
}

double warlock_t::composite_melee_crit_chance() const
{
  double mc = player_t::composite_melee_crit_chance();

  if (buffs.dark_soul_instability->check())
    mc += buffs.dark_soul_instability->check_value();

  return mc;
}

double warlock_t::composite_mastery() const
{
  double m = player_t::composite_mastery();
  return m;
}

double warlock_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );
  return m;
}

double warlock_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  return player_t::resource_gain( resource_type, amount, source, action );
}

double warlock_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );
  return reg;
}

double warlock_t::composite_armor() const
{
  return player_t::composite_armor() + spec.fel_armor -> effectN( 2 ).base_value();
}

void warlock_t::halt()
{
  player_t::halt();
  if ( spells.melee )
    spells.melee->cancel();
}

double warlock_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return spec.nethermancy->effectN( 1 ).percent();

  return 0.0;
}

action_t* warlock_t::create_action( const std::string& action_name, const std::string& options_str )
{
  using namespace actions;

  if ( ( action_name == "summon_pet" ) && default_pet.empty() ) {
    sim->errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
    return nullptr;
  }
  // Pets
  if ( action_name == "summon_felhunter"      ) return new          summon_main_pet_t( "felhunter", this );
  if ( action_name == "summon_felguard"       ) return new           summon_main_pet_t( "felguard", this );
  if ( action_name == "summon_succubus"       ) return new           summon_main_pet_t( "succubus", this );
  if ( action_name == "summon_voidwalker"     ) return new         summon_main_pet_t( "voidwalker", this );
  if ( action_name == "summon_imp"            ) return new                summon_main_pet_t( "imp", this );
  if ( action_name == "summon_pet"            ) return new          summon_main_pet_t( default_pet, this );
  // Base Spells
  if ( action_name == "drain_life"            ) return new               drain_life_t( this, options_str );
  if ( action_name == "grimoire_of_sacrifice" ) return new    grimoire_of_sacrifice_t( this, options_str ); //aff and destro

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    action_t* aff_action = create_action_affliction( action_name, options_str );
    if ( aff_action )
      return aff_action;
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    action_t* demo_action = create_action_demonology( action_name, options_str );
    if ( demo_action )
      return demo_action;
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    action_t* destro_action = create_action_destruction(action_name, options_str);
    if (destro_action)
      return destro_action;
  }

  return player_t::create_action( action_name, options_str );
}

bool warlock_t::create_actions()
{
	return player_t::create_actions();
}

pet_t* warlock_t::create_pet( const std::string& pet_name, const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );
  if ( p ) return p;
  using namespace pets;

  if ( pet_name == "felhunter"          )   return new                felhunter::felhunter_pet_t( sim, this );
  if ( pet_name == "imp"                )   return new                            imp::imp_pet_t( sim, this );
  if ( pet_name == "succubus"           )   return new                  succubus::succubus_pet_t( sim, this );
  if ( pet_name == "voidwalker"         )   return new              voidwalker::voidwalker_pet_t( sim, this );
  if ( pet_name == "felguard"           )   return new                  felguard::felguard_pet_t( sim, this );

  if ( pet_name == "grimoire_felguard"   )  return new        felguard::felguard_pet_t( sim, this, pet_name );

  return nullptr;
}

void warlock_t::create_pets()
{
  if (specialization() == WARLOCK_DEMONOLOGY)
  {
    for (size_t i = 0; i < warlock_pet_list.wild_imps.size(); i++)
    {
      warlock_pet_list.wild_imps[i] = new pets::wild_imp::wild_imp_pet_t(sim, this);
    }
    for (size_t i = 0; i < warlock_pet_list.dreadstalkers.size(); i++)
    {
      warlock_pet_list.dreadstalkers[i] = new pets::dreadstalker::dreadstalker_t(sim, this);
    }
    for (size_t i = 0; i < warlock_pet_list.demonic_tyrants.size(); i++)
    {
      warlock_pet_list.demonic_tyrants[i] = new pets::demonic_tyrant::demonic_tyrant_t(sim, this);
    }
    if (talents.summon_vilefiend->ok())
    {
      for (size_t i = 0; i < warlock_pet_list.vilefiends.size(); i++)
      {
        warlock_pet_list.vilefiends[i] = new pets::vilefiend::vilefiend_t(sim, this);
      }
    }
    if (talents.inner_demons->ok() or talents.nether_portal->ok())
    {
      for (size_t i = 0; i < warlock_pet_list.shivarra.size(); i++)
      {
        warlock_pet_list.shivarra[i] = new pets::shivarra::shivarra_t(sim, this);
      }
      for (size_t i = 0; i < warlock_pet_list.darkhounds.size(); i++)
      {
        warlock_pet_list.darkhounds[i] = new pets::darkhound::darkhound_t(sim, this);
      }
      for (size_t i = 0; i < warlock_pet_list.bilescourges.size(); i++)
      {
        warlock_pet_list.bilescourges[i] = new pets::bilescourge::bilescourge_t(sim, this);
      }
      for (size_t i = 0; i < warlock_pet_list.urzuls.size(); i++)
      {
        warlock_pet_list.urzuls[i] = new pets::urzul::urzul_t(sim, this);
      }
      for (size_t i = 0; i < warlock_pet_list.void_terrors.size(); i++)
      {
        warlock_pet_list.void_terrors[i] = new pets::void_terror::void_terror_t(sim, this);
      }
      for (size_t i = 0; i < warlock_pet_list.wrathguards.size(); i++)
      {
        warlock_pet_list.wrathguards[i] = new pets::wrathguard::wrathguard_t(sim, this);
      }
      for (size_t i = 0; i < warlock_pet_list.vicious_hellhounds.size(); i++)
      {
        warlock_pet_list.vicious_hellhounds[i] = new pets::vicious_hellhound::vicious_hellhound_t(sim, this);
      }
      for (size_t i = 0; i < warlock_pet_list.illidari_satyrs.size(); i++)
      {
        warlock_pet_list.illidari_satyrs[i] = new pets::illidari_satyr::illidari_satyr_t(sim, this);
      }
      for (size_t i = 0; i < warlock_pet_list.eyes_of_guldan.size(); i++)
      {
        warlock_pet_list.eyes_of_guldan[i] = new pets::eyes_of_guldan::eyes_of_guldan_t(sim, this);
      }
      for (size_t i = 0; i < warlock_pet_list.prince_malchezaar.size(); i++)
      {
        warlock_pet_list.prince_malchezaar[i] = new pets::prince_malchezaar::prince_malchezaar_t(sim, this);
      }
    }
  }

  if (specialization() == WARLOCK_DESTRUCTION)
  {
    for (size_t i = 0; i < warlock_pet_list.infernals.size(); i++)
    {
      warlock_pet_list.infernals[i] = new pets::infernal::infernal_t(sim, this);
    }
  }

  if (specialization() == WARLOCK_AFFLICTION)
  {
    for (size_t i = 0; i < warlock_pet_list.darkglare.size(); i++)
    {
      warlock_pet_list.darkglare[i] = new pets::darkglare::darkglare_t(sim, this);
    }
  }

  for ( auto& pet : pet_name_list )
  {
    create_pet( pet );
  }
}

void warlock_t::create_buffs()
{
  player_t::create_buffs();

  create_buffs_affliction();
  create_buffs_demonology();
  create_buffs_destruction();

  buffs.grimoire_of_sacrifice = make_buff( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice->effectN( 2 ).trigger() );

  //legendary buffs
  buffs.stretens_insanity = make_buff( this, "stretens_insanity", find_spell( 208822 ) )
    ->set_default_value( find_spell( 208822 )->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_tick_behavior( buff_tick_behavior::NONE );
  buffs.lessons_of_spacetime = make_buff( this, "lessons_of_spacetime", find_spell( 236176 ) )
    ->set_default_value( find_spell( 236176 )->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION )
    ->set_tick_behavior( buff_tick_behavior::NONE );
  buffs.sephuzs_secret =
    make_buff<haste_buff_t>( this, "sephuzs_secret", find_spell( 208052 ) );
  buffs.sephuzs_secret->set_default_value( find_spell( 208052 )->effectN( 2 ).percent() )
    ->set_cooldown( find_spell( 226262 )->duration() );
  buffs.alythesss_pyrogenics = make_buff( this, "alythesss_pyrogenics", find_spell( 205675 ) )
    ->set_default_value( find_spell( 205675 )->effectN( 1 ).percent() )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  buffs.wakeners_loyalty = make_buff( this, "wakeners_loyalty", find_spell( 236200 ) )
    ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    ->set_default_value( find_spell( 236200 )->effectN( 1 ).percent() );
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  warlock_t::init_spells_affliction();
  warlock_t::init_spells_demonology();
  warlock_t::init_spells_destruction();

  // General
  spec.fel_armor = find_spell( 104938 );
  spec.nethermancy = find_spell( 86091 );

  // Specialization Spells
  spec.immolate                         = find_specialization_spell( "Immolate" );
  spec.nightfall                        = find_specialization_spell( "Nightfall" );
  spec.wild_imps                        = find_specialization_spell( "Wild Imps" );
  spec.demonic_core                     = find_specialization_spell( "Demonic Core" );
  spec.shadow_bite                      = find_specialization_spell( "Shadow Bite" );
  spec.shadow_bite_2                    = find_specialization_spell( 231799 );
  spec.unending_resolve                 = find_specialization_spell( "Unending Resolve" );
  spec.unending_resolve_2               = find_specialization_spell( 231794 );
  spec.firebolt                         = find_specialization_spell( "Firebolt" );
  spec.firebolt_2                       = find_specialization_spell( 231795 );

  // Talents
  talents.demon_skin                    = find_talent_spell( "Fire and Brimstone" );
  talents.burning_rush                  = find_talent_spell( "Burning Rush" );
  talents.dark_pact                     = find_talent_spell( "Dark Pact" );
  talents.darkfury                      = find_talent_spell( "Darkfury" );
  talents.mortal_coil                   = find_talent_spell( "Mortal Coil" );
  talents.demonic_circle                = find_talent_spell( "Demonic Circle" );
  talents.grimoire_of_sacrifice         = find_talent_spell( "Grimoire of Sacrifice" ); // aff and destro
  active.grimoire_of_sacrifice_proc     = new actions::grimoire_of_sacrifice_damage_t( this ); // grimoire of sacrifice
  talents.soul_conduit                  = find_talent_spell( "Soul Conduit" );
}

void warlock_t::init_rng()
{
  if ( specialization() == WARLOCK_AFFLICTION )
    init_rng_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_rng_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_rng_destruction();

  grimoire_of_sacrifice_rppm     = get_rppm( "grimoire_of_sacrifice", find_spell( 196099 ) );

  player_t::init_rng();
}

void warlock_t::init_gains()
{
  player_t::init_gains();

  if ( specialization() == WARLOCK_AFFLICTION )
    init_gains_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_gains_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_gains_destruction();

  gains.miss_refund                     = get_gain( "miss_refund" );
  gains.shadow_bolt                     = get_gain( "shadow_bolt" );
  gains.soul_conduit                    = get_gain( "soul_conduit" );
  
  gains.soulsnatcher                    = get_gain( "soulsnatcher" );
  gains.power_trip                      = get_gain( "power_trip" );
  gains.recurrent_ritual                = get_gain( "recurrent_ritual" );
  gains.feretory_of_souls               = get_gain( "feretory_of_souls" );
  gains.power_cord_of_lethtendris       = get_gain( "power_cord_of_lethtendris" );
}

void warlock_t::init_procs()
{
  player_t::init_procs();

  if ( specialization() == WARLOCK_AFFLICTION )
    init_procs_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_procs_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_procs_destruction();

  procs.one_shard_hog                   = get_proc( "one_shard_hog" );
  procs.two_shard_hog                   = get_proc( "two_shard_hog" );
  procs.three_shard_hog                 = get_proc( "three_shard_hog" );
  procs.portal_summon                   = get_proc( "portal_summon" );
  procs.demonic_calling                 = get_proc( "demonic_calling" );
  procs.power_trip                      = get_proc( "power_trip" );
  procs.soul_conduit                    = get_proc( "soul_conduit" );
  procs.t19_2pc_chaos_bolts             = get_proc( "t19_2pc_chaos_bolt" );
  procs.demonology_t20_2pc              = get_proc( "demonology_t20_2pc" );
}

void warlock_t::init_base_stats()
{
  if ( base.distance < 1.0 )
    base.distance = 40.0;

  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;
  base.spell_power_per_intellect = 1.0;

  base.attribute_multiplier[ATTR_STAMINA] *= 1.0 + spec.fel_armor->effectN( 1 ).percent();

  resources.base[RESOURCE_SOUL_SHARD] = 5;

  if ( default_pet.empty() )
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      default_pet = "felhunter";
    else if ( specialization() == WARLOCK_DEMONOLOGY )
      default_pet = "felguard";
    else if ( specialization() == WARLOCK_DESTRUCTION )
      default_pet = "imp";
  }
}

void warlock_t::init_scaling()
{
  player_t::init_scaling();
}

void warlock_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action("summon_pet");
  if (specialization() == WARLOCK_DEMONOLOGY)
    precombat->add_action("inner_demons,if=talent.inner_demons.enabled");

  precombat->add_action( "snapshot_stats" );

  if (specialization() != WARLOCK_DEMONOLOGY)
    precombat->add_action("grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled");
  if (specialization() == WARLOCK_DEMONOLOGY)
    precombat->add_action("demonbolt");

  if ( sim -> allow_potions )
  {
    precombat->add_action( "potion" );
  }
}

std::string warlock_t::default_potion() const
{
  std::string lvl110_potion = "prolonged_power";

  return ( true_level >= 100 ) ? lvl110_potion :
         ( true_level >=  90 ) ? "draenic_intellect" :
         ( true_level >=  85 ) ? "jade_serpent" :
         ( true_level >=  80 ) ? "volcanic" :
                                 "disabled";
}

std::string warlock_t::default_flask() const
{
  return ( true_level >= 100 ) ? "whispered_pact" :
         ( true_level >=  90 ) ? "greater_draenic_intellect_flask" :
         ( true_level >=  85 ) ? "warm_sun" :
         ( true_level >=  80 ) ? "draconic_mind" :
                                 "disabled";
}

std::string warlock_t::default_food() const
{
  std::string lvl100_food =
    (specialization() == WARLOCK_DESTRUCTION) ?   "frosty_stew" :
    (specialization() == WARLOCK_DEMONOLOGY) ?    "frosty_stew" :
    (specialization() == WARLOCK_AFFLICTION) ?    "felmouth_frenzy" :
                                                  "felmouth_frenzy";

  std::string lvl110_food =
    (specialization() == WARLOCK_AFFLICTION) ?    "nightborne_delicacy_platter" :
                                                  "azshari_salad";

  return ( true_level > 100 ) ? lvl110_food :
         ( true_level >  90 ) ? lvl100_food :
                                "disabled";
}

std::string warlock_t::default_rune() const
{
  return ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "focus" :
                                 "disabled";
}

void warlock_t::apl_global_filler() { }

void warlock_t::apl_default()
{
  if ( specialization() == WARLOCK_AFFLICTION )
    create_apl_affliction();
  else if ( specialization() == WARLOCK_DEMONOLOGY )
    create_apl_demonology();
  else if ( specialization() == WARLOCK_DESTRUCTION )
    create_apl_destruction();
}

void warlock_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    apl_precombat();

    apl_default();

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[RESOURCE_SOUL_SHARD] = initial_soul_shards;

  if ( warlock_pet_list.active )
    warlock_pet_list.active->init_resources( force );
}

void warlock_t::combat_begin()
{
  player_t::combat_begin();
}

void warlock_t::reset()
{
  player_t::reset();

  range::for_each( sim->target_list, [this]( const player_t* t )
  {
    if ( auto td = target_data[t] )
    {
      td->reset();
    }

    range::for_each( t->pet_list, [this]( const player_t* add )
    {
      if ( auto td = target_data[add] )
      {
        td->reset();
      }
    } );
  } );

  warlock_pet_list.active = nullptr;
  shard_react = timespan_t::zero();
  havoc_target = nullptr;
  agony_accumulator = rng().range( 0.0, 0.99 );
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "soul_shards", initial_soul_shards ) );
  add_option( opt_string( "default_pet", default_pet ) );
  add_option( opt_bool( "allow_sephuz", allow_sephuz ) );
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype & SAVE_PLAYER )
  {
    if ( initial_soul_shards != 3 )    profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( !default_pet.empty() )        profile_str += "default_pet=" + default_pet + "\n";
    if ( allow_sephuz != 0 )           profile_str += "allow_sephuz=" + util::to_string( allow_sephuz ) + "\n";
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast< warlock_t* >( source );

  initial_soul_shards = p->initial_soul_shards;
  allow_sephuz = p->allow_sephuz;
  deaths_embrace_fixed_time = p->deaths_embrace_fixed_time;
  default_pet = p->default_pet;
}

stat_e warlock_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    // This is all a guess at how the hybrid primaries will work, since they
    // don't actually appear on cloth gear yet. TODO: confirm behavior
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

expr_t* warlock_t::create_expression( const std::string& name_str )
{
  if ( name_str == "shard_react" )
  {
    return make_fn_expr( name_str, [this]()
        { return resources.current[RESOURCE_SOUL_SHARD] >= 1 && sim->current_time() >= shard_react; } );
  }
  else if ( name_str == "pet_count" )
  {
    return make_fn_expr(name_str,
        [this]()
        {
          double t = 0;
          for ( const auto& pet : pet_list )
          {
            if ( auto lock_pet = dynamic_cast<pets::warlock_pet_t*>( pet ) )
            {
              if ( !lock_pet->is_sleeping() )
              {
                t++;
              }
            }
          }
          return t;});
  }
  else if (name_str == "last_cast_imps")
  {
    struct wild_imp_last_cast_expression_t : public expr_t
    {
      warlock_t& player;

      wild_imp_last_cast_expression_t(warlock_t& p) :
        expr_t("last_cast_imps"), player(p) { }

      virtual double evaluate() override
      {
        int t = 0;
        for (auto& imp : player.warlock_pet_list.wild_imps)
        {
          if (!imp->is_sleeping())
          {
            if (imp->resources.current[RESOURCE_ENERGY] <= 20)
              t++;
          }
        }
        return t;
      }
    };

    return new wild_imp_last_cast_expression_t(*this);
  }
  else if (name_str == "two_cast_imps")
  {
    struct wild_imp_two_cast_expression_t : public expr_t
    {
      warlock_t& player;

      wild_imp_two_cast_expression_t(warlock_t& p) :
        expr_t("two_cast_imps"), player(p) { }

      virtual double evaluate() override
      {
        int t = 0;
        for (auto& imp : player.warlock_pet_list.wild_imps)
        {
          if (!imp->is_sleeping())
          {
            if (imp->resources.current[RESOURCE_ENERGY] <= 40)
              t++;
          }
        }
        return t;
      }
    };

    return new wild_imp_two_cast_expression_t(*this);
  }

  return player_t::create_expression( name_str );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warlock_report_t: public player_report_extension_t
{
public:
  warlock_report_t( warlock_t& player ):
    p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  warlock_t& p;
};

using namespace unique_gear;
using namespace actions;

struct power_cord_of_lethtendris_t : public scoped_actor_callback_t<warlock_t>
{
  power_cord_of_lethtendris_t() : super( WARLOCK ) {}

  void manipulate( warlock_t* p, const special_effect_t& e ) override
  {
    p->legendary.power_cord_of_lethtendris_chance = e.driver()->effectN( 1 ).percent();
  }
};
/*
struct reap_and_sow_t : public scoped_action_callback_t<reap_souls_t>
{
    reap_and_sow_t() : super ( WARLOCK, "reap_souls" )
    {}

    void manipulate (reap_souls_t* a, const special_effect_t& e) override
    {
        a->reap_and_sow_bonus = timespan_t::from_millis(e.driver()->effectN(1).base_value());
    }
};
*/

struct wakeners_loyalty_t : public scoped_actor_callback_t<warlock_t>
{
  wakeners_loyalty_t() : super( WARLOCK ) {}

  void manipulate( warlock_t* p, const special_effect_t& ) override
  {
    p->legendary.wakeners_loyalty_enabled = true;
  }
};
/*
struct hood_of_eternal_disdain_t : public scoped_action_callback_t<agony_t>
{
  hood_of_eternal_disdain_t() : super( WARLOCK, "agony" )
  {}

  void manipulate( agony_t* a, const special_effect_t& e ) override
  {
    a -> base_tick_time *= 1.0 + e.driver() -> effectN( 2 ).percent();
    a -> dot_duration *= 1.0 + e.driver() -> effectN( 1 ).percent();
  }
};

struct sacrolashs_dark_strike_t : public scoped_action_callback_t<corruption_t>
{
  sacrolashs_dark_strike_t() : super( WARLOCK, "corruption" )
  {}

  void manipulate( corruption_t* a, const special_effect_t& e ) override
  {
    a -> base_multiplier *= 1.0 + e.driver() -> effectN( 1 ).percent();
  }
};
*/
/*
struct kazzaks_final_curse_t : public scoped_action_callback_t<doom_t>
{
  kazzaks_final_curse_t() : super( WARLOCK, "doom" )
  {}

  
  void manipulate( doom_t* a, const special_effect_t& e ) override
  {
    a -> kazzaks_final_curse_multiplier = e.driver() -> effectN( 1 ).percent();
  }
  
};
*/

struct wilfreds_sigil_of_superior_summoning_t : public scoped_actor_callback_t<warlock_t>
{
  wilfreds_sigil_of_superior_summoning_t() : super( WARLOCK ) { }

  void manipulate( warlock_t*, const special_effect_t& ) override { }
};

struct sindorei_spite_t : public class_buff_cb_t<warlock_t>
{
  sindorei_spite_t() : super( WARLOCK, "sindorei_spite" ) { }

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e )->buffs.sindorei_spite;
  }

  buff_t* creator( const special_effect_t& e ) const override
  {
    return make_buff( e.player, buff_name, e.driver()->effectN( 1 ).trigger() )
      ->set_default_value( e.driver()->effectN( 1 ).trigger()->effectN( 1 ).percent() )
      ->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct lessons_of_spacetime_t : public scoped_actor_callback_t<warlock_t>
{
  lessons_of_spacetime_t() : super( WARLOCK ) { }

  void manipulate( warlock_t* p, const special_effect_t& ) override
  {
    //const spell_data_t * tmp = p -> find_spell( 236176 );
    p->legendary.lessons_of_spacetime = true;
    p->legendary.lessons_of_spacetime1 = timespan_t::from_seconds( 5 );
    p->legendary.lessons_of_spacetime2 = timespan_t::from_seconds( 9 );
    p->legendary.lessons_of_spacetime3 = timespan_t::from_seconds( 16 );
  }
};

struct odr_shawl_of_the_ymirjar_t : public scoped_actor_callback_t<warlock_t>
{
  odr_shawl_of_the_ymirjar_t() : super( WARLOCK_DESTRUCTION ) { }

  void manipulate( warlock_t* a, const special_effect_t&  ) override
  {
    a->legendary.odr_shawl_of_the_ymirjar = a -> find_spell( 212173 );
  }
};

struct stretens_insanity_t: public scoped_actor_callback_t<warlock_t>
{
  stretens_insanity_t(): super( WARLOCK_AFFLICTION ) { }

  void manipulate( warlock_t* a, const special_effect_t&  ) override
  {
    a->legendary.stretens_insanity = true;
  }
};

struct feretory_of_souls_t : public scoped_actor_callback_t<warlock_t>
{
  feretory_of_souls_t() : super( WARLOCK ) { }

  void manipulate( warlock_t* a, const special_effect_t& ) override
  {
    a->legendary.feretory_of_souls = true;
  }
};

struct sephuzs_secret_t : public scoped_actor_callback_t<warlock_t>
{
  sephuzs_secret_t() : super( WARLOCK ) { }

  void manipulate( warlock_t* a, const special_effect_t& e ) override
  {
    a->legendary.sephuzs_secret = true;
    a->legendary.sephuzs_passive = e.driver() -> effectN( 3 ).percent();
  }
};

struct magistrike_t : public scoped_actor_callback_t<warlock_t>
{
  magistrike_t() : super( WARLOCK ) { }

  void manipulate( warlock_t* a, const special_effect_t& ) override
  {
    a->legendary.magistrike = true;
  }
};

struct the_master_harvester_t : public scoped_actor_callback_t<warlock_t>
{
  the_master_harvester_t() : super( WARLOCK ) { }

  void manipulate( warlock_t* a, const special_effect_t& /* e */ ) override
  {
    a->legendary.the_master_harvester = true;
  }
};

struct alythesss_pyrogenics_t : public scoped_actor_callback_t<warlock_t>
{
  alythesss_pyrogenics_t() : super( WARLOCK ) { }

  void manipulate( warlock_t* a, const special_effect_t& /* e */ ) override
  {
    a->legendary.alythesss_pyrogenics = true;
  }
};

struct warlock_module_t : public module_t
{
  warlock_module_t() : module_t( WARLOCK ) { }

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new warlock_t( sim, name, r );
    p->report_extension = std::unique_ptr<player_report_extension_t>( new warlock_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override
  {
    // Legendaries

    //register_special_effect( 205797, hood_of_eternal_disdain_t() );
    //register_special_effect( 214225, kazzaks_final_curse_t() );
    //register_special_effect( 205721, recurrent_ritual_t() );
    register_special_effect( 214345, wilfreds_sigil_of_superior_summoning_t() );
    register_special_effect( 208868, sindorei_spite_t(), true );
    register_special_effect( 212172, odr_shawl_of_the_ymirjar_t() );
    register_special_effect( 205702, feretory_of_souls_t() );
    register_special_effect( 208821, stretens_insanity_t() );
    register_special_effect( 205753, power_cord_of_lethtendris_t() );
    //register_special_effect( 236114, reap_and_sow_t() );
    register_special_effect( 236199, wakeners_loyalty_t() );
    register_special_effect( 236174, lessons_of_spacetime_t() );
    register_special_effect( 208051, sephuzs_secret_t() );
    //register_special_effect( 207952, sacrolashs_dark_strike_t() );
    register_special_effect( 213014, magistrike_t() );
    register_special_effect( 248113, the_master_harvester_t() );
    register_special_effect( 205678, alythesss_pyrogenics_t() );
  }

  virtual void register_hotfixes() const override { }

  virtual bool valid() const override { return true; }
  virtual void init( player_t* ) const override { }
  virtual void combat_begin( sim_t* ) const override { }
  virtual void combat_end( sim_t* ) const override { }
};
}

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}
