#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock {
  namespace actions_destruction {
    using namespace actions;

    struct destruction_spell_t : public warlock_spell_t
    {
    public:
      gain_t * gain;

      bool can_havoc;
      bool destro_mastery;

      destruction_spell_t(warlock_t* p, const std::string& n) :
        destruction_spell_t(n, p, p -> find_class_spell(n))
      {
      }

      destruction_spell_t(warlock_t* p, const std::string& n, specialization_e s) :
        destruction_spell_t(n, p, p -> find_class_spell(n, s))
      {
      }

      destruction_spell_t(const std::string& token, warlock_t* p, const spell_data_t* s = spell_data_t::nil()) :
        warlock_spell_t(token, p, s)
      {
        may_crit = true;
        tick_may_crit = true;
        weapon_multiplier = 0.0;
        gain = player->get_gain(name_str);
        can_havoc = false;
        destro_mastery = true;
      }

      bool use_havoc() const
      {
        // Ensure we do not try to hit the same target twice.
        return can_havoc && p()->havoc_target && p()->havoc_target != target;
      }

      int n_targets() const override
      {
        if ( use_havoc() )
        {
          assert( warlock_spell_t::n_targets() == 0 );
          return 2;
        }
        else
          return warlock_spell_t::n_targets();
      }

      size_t available_targets(std::vector<player_t*>& tl) const override
      {
        warlock_spell_t::available_targets( tl );

        // Check target list size to prevent some silly scenarios where Havoc target
        // is the only target in the list.
        if ( tl.size() > 1 && use_havoc() )
        {
          // We need to make sure that the Havoc target ends up second in the target list,
          // so that Havoc spells can pick it up correctly.
          auto it = range::find( tl, p()->havoc_target );
          if ( it != tl.end() )
          {
            tl.erase( it );
            tl.insert( tl.begin() + 1, p()->havoc_target );
          }
        }

        return tl.size();
      }

      void init() override
      {
        warlock_spell_t::init();

        if ( can_havoc )
        {
          base_aoe_multiplier *= p()->spec.havoc->effectN( 1 ).percent();
          p()->havoc_spells.push_back( this );
        }
      }

      void consume_resource() override
      {
        warlock_spell_t::consume_resource();

        if (resource_current == RESOURCE_SOUL_SHARD && p()->in_combat)
        {
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

          if (p()->talents.soul_fire->ok())
            p()->cooldowns.soul_fire->adjust((-1 * (p()->talents.soul_fire->effectN(2).time_value()*last_resource_cost)));
        }
      }

      void execute() override
      {
        warlock_spell_t::execute();

        if ( can_havoc && num_targets_hit > 1 && p()->azerite.rolling_havoc.enabled() )
          p()->buffs.rolling_havoc->trigger();
      }

      void impact(action_state_t* s) override
      {
        warlock_spell_t::impact(s);

        if (p()->talents.reverse_entropy->ok())
        {
          auto success = p()->buffs.reverse_entropy->trigger();
          if (success)
          {
            p()->procs.reverse_entropy->occur();
          }
        }
      }

      double composite_target_multiplier(player_t* t) const override
      {
        double m = warlock_spell_t::composite_target_multiplier(t);

        auto td = this->td(t);
        if (td->debuffs_eradication->check())
          m *= 1.0 + td->debuffs_eradication->data().effectN(1).percent();

        return m;
      }

      //TODO: Check order of multipliers on Havoc'd spells
      double action_multiplier() const override
      {
        double pm = warlock_spell_t::action_multiplier();

        if (p()->mastery_spells.chaotic_energies->ok() && destro_mastery)
        {
          double destro_mastery_value = p()->cache.mastery_value() / 2.0;
          double chaotic_energies_rng = rng().range(0, destro_mastery_value);

          pm *= 1.0 + chaotic_energies_rng + (destro_mastery_value);
        }

        if (p()->buffs.grimoire_of_supremacy->check() && this->data().affected_by(p()->find_spell(266091)->effectN(1)))
        {
          pm *= 1.0 + p()->buffs.grimoire_of_supremacy->check_stack_value();
        }

        return pm;
      }
    };

    //Talents
    struct soul_fire_t : public destruction_spell_t
    {
      soul_fire_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("soul_fire", p, p -> talents.soul_fire)
      {
        parse_options(options_str);
        energize_type = ENERGIZE_PER_HIT;
        energize_resource = RESOURCE_SOUL_SHARD;
        energize_amount = (p->find_spell( 281490 )->effectN( 1 ).base_value()) / 10.0;

        can_havoc = true;
      }
    };

    struct internal_combustion_t : public destruction_spell_t
    {
      internal_combustion_t(warlock_t* p) :
        destruction_spell_t("Internal Combustion", p, p->talents.internal_combustion)
      {
        background = true;
        destro_mastery = false;
      }

      void init() override
      {
        destruction_spell_t::init();

        snapshot_flags |= STATE_MUL_DA | STATE_TGT_MUL_DA | STATE_MUL_PERSISTENT | STATE_VERSATILITY;
      }

      void execute() override
      {
        warlock_td_t* td = this->td(target);
        dot_t* dot = td->dots_immolate;

        assert(dot->current_action);
        action_state_t* state = dot->current_action->get_state(dot->state);
        dot->current_action->calculate_tick_amount(state, 1.0);
        double tick_base_damage = state->result_raw;
        timespan_t remaining = std::min(dot->remains(), timespan_t::from_seconds(data().effectN(1).base_value()));
        timespan_t dot_tick_time = dot->current_action->tick_time(state);
        double ticks_left = remaining / dot_tick_time;

        double total_damage = ticks_left * tick_base_damage;

        action_state_t::release(state);
        this->base_dd_min = this->base_dd_max = total_damage;

        destruction_spell_t::execute();
        td->dots_immolate->reduce_duration(remaining);
      }
    };

    struct shadowburn_t : public destruction_spell_t
    {
      shadowburn_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("shadowburn", p, p -> talents.shadowburn)
      {
        parse_options(options_str);
        energize_type = ENERGIZE_PER_HIT;
        energize_resource = RESOURCE_SOUL_SHARD;
        energize_amount = ( p->find_spell( 245731 )->effectN( 1 ).base_value() ) / 10.0;
        can_havoc = true;
      }

      virtual void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);

        if (result_is_hit(s->result))
        {
          td(s->target)->debuffs_shadowburn->trigger();
        }
      }
    };

    //TODO: Check the status of the comment below
    struct roaring_blaze_t : public destruction_spell_t {
      roaring_blaze_t(warlock_t* p) :
        destruction_spell_t("roaring_blaze", p, p -> find_spell(265931))
      {
        background = true;
        destro_mastery = false;
        hasted_ticks = true;
      }
    }; //damage is not correct

    struct dark_soul_instability_t : public destruction_spell_t
    {
      dark_soul_instability_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("dark_soul_instability", p, p -> talents.dark_soul_instability)
      {
        parse_options(options_str);
        harmful = may_crit = may_miss = false;
      }

      void execute() override
      {
        destruction_spell_t::execute();

        p()->buffs.dark_soul_instability->trigger();
      }
    };

    //Spells
    struct havoc_t : public destruction_spell_t
    {
      havoc_t(warlock_t* p, const std::string& options_str) : destruction_spell_t(p, "Havoc")
      {
        parse_options(options_str);
        may_crit = false;
      }

      void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);
        td(s->target)->debuffs_havoc->trigger();
      }
    };

    struct immolate_t : public destruction_spell_t
    {
      immolate_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("immolate", p, p -> find_spell(348))
      {
        parse_options(options_str);
        const spell_data_t* dmg_spell = player->find_spell(157736);

        can_havoc = true;

        //All of the DoT data for Immolate is in spell 157736
        base_tick_time = dmg_spell->effectN(1).period();
        dot_duration = dmg_spell->duration();
        spell_power_mod.tick = dmg_spell->effectN(1).sp_coeff();
        hasted_ticks = true;
        tick_may_crit = true;
      }

      virtual void tick(dot_t* d) override
      {
        destruction_spell_t::tick(d);

        if (d->state->result == RESULT_CRIT && rng().roll(data().effectN(2).percent()))
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate_crits);

        p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate);

        if (d->state->result_amount > 0.0 && p()->azerite.flashpoint.ok() && d->target->health_percentage() > p()->flashpoint_threshold * 100 )
          p()->buffs.flashpoint->trigger();

        // For some reason this triggers on every tick
        expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, p() );
      }
    };

    struct conflagrate_t : public destruction_spell_t
    {
      timespan_t total_duration;
      timespan_t base_duration;
      roaring_blaze_t* roaring_blaze;

      conflagrate_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("Conflagrate", p, p -> find_spell(17962)),
        total_duration(),
        base_duration(),
        roaring_blaze(new roaring_blaze_t(p))
      {
        parse_options(options_str);
        can_havoc = true;

        energize_type = ENERGIZE_PER_HIT;
        energize_resource = RESOURCE_SOUL_SHARD;
        energize_amount = ( p->find_spell( 245330 )->effectN( 1 ).base_value() ) / 10.0;

        cooldown->charges += p->spec.conflagrate_2->effectN(1).base_value();

        add_child(roaring_blaze);
      }

      void init() override
      {
        destruction_spell_t::init();

        cooldown->hasted = true;
      }

      void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);

        if (result_is_hit(s->result))
        {
          if ( p()->talents.roaring_blaze->ok() )
          {
            roaring_blaze->set_target( s->target );
            roaring_blaze->execute();
          }
        }
      }

      void execute() override
      {
        destruction_spell_t::execute();

        p()->buffs.backdraft->trigger( 1 + (p()->talents.flashover->ok() ? p()->talents.flashover->effectN( 1 ).base_value() : 0) );

        auto td = this->td(target);
        if (p()->azerite.bursting_flare.ok() && td->dots_immolate->is_ticking())
          p()->buffs.bursting_flare->trigger();

        sim->print_log("{}: Action {} {} charges remain", player->name(), name(), this->cooldown->current_charge);
      }

      double action_multiplier()const override
      {
        double m = destruction_spell_t::action_multiplier();

        if (p()->talents.flashover)
        {
          m *= 1.0 + p()->talents.flashover->effectN(3).percent();
        }

        return m;
      }
    };

    struct incinerate_fnb_t : public destruction_spell_t
    {
      incinerate_fnb_t(warlock_t* p) :
        destruction_spell_t("incinerate_fnb", p, p->find_class_spell("Incinerate"))
      {
        aoe = -1;
        background = true;

        if (p->talents.fire_and_brimstone->ok()) {
          base_multiplier *= p->talents.fire_and_brimstone->effectN(1).percent();
          energize_type = ENERGIZE_PER_HIT;
          energize_resource = RESOURCE_SOUL_SHARD;
          energize_amount = (p->talents.fire_and_brimstone->effectN(2).base_value()) / 10.0;
          gain = p->gains.incinerate_fnb;
        }
      }

      void init() override
      {
        destruction_spell_t::init();

        // F&B Incinerate's target list depends on the current Havoc target, so it
        // needs to invalidate its target list with the rest of the Havoc spells.
        p()->havoc_spells.push_back( this );
      }

      double bonus_da( const action_state_t* s ) const override
      {
        double da = destruction_spell_t::bonus_da( s );

        da += p()->azerite.chaos_shards.value( 2 );

        return da;
      }

      double cost() const override
      {
        return 0;
      }

      size_t available_targets(std::vector<player_t*>& tl) const override
      {
        destruction_spell_t::available_targets(tl);

        // Does not hit the main target
        auto it = range::find(tl, target);
        if (it != tl.end())
        {
          tl.erase(it);
        }

        // nor the havoced target
        it = range::find(tl, p()->havoc_target);
        if (it != tl.end())
        {
          tl.erase(it);
        }

        return tl.size();
      }

      void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);

        if (s->result == RESULT_CRIT)
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.incinerate_fnb_crits);
      }

      virtual double composite_target_crit_chance(player_t* target) const override
      {
        double m = destruction_spell_t::composite_target_crit_chance(target);

        auto td = this->td(target);
        if (td->debuffs_chaotic_flames->check())
          m += p()->find_spell(253092)->effectN(1).percent();

        return m;
      }
    };

    struct incinerate_t : public destruction_spell_t
    {
      double backdraft_gcd;
      double backdraft_cast_time;
      incinerate_fnb_t* fnb_action;

      incinerate_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t(p, "Incinerate"), fnb_action(new incinerate_fnb_t(p))
      {
        parse_options(options_str);

        add_child(fnb_action);

        can_havoc = true;

        backdraft_cast_time = 1.0 + p->buffs.backdraft->data().effectN(1).percent();
        backdraft_gcd = 1.0 + p->buffs.backdraft->data().effectN(2).percent();

        energize_type = ENERGIZE_PER_HIT;
        energize_resource = RESOURCE_SOUL_SHARD;
        energize_amount = (p->find_spell(244670)->effectN(1).base_value()) / 10.0;
      }

      double bonus_da( const action_state_t* s ) const override
      {
        double da = destruction_spell_t::bonus_da( s );

        da += p()->azerite.chaos_shards.value( 2 );

        return da;
      }

      virtual timespan_t execute_time() const override
      {
        timespan_t h = spell_t::execute_time();

        if (p()->buffs.backdraft->check() && !p()->buffs.chaotic_inferno->check() )
          h *= backdraft_cast_time;

        if (p()->buffs.chaotic_inferno->check())
          h *= 1.0 + p()->buffs.chaotic_inferno->check_value();

        return h;
      }

      timespan_t gcd() const override
      {
        timespan_t t = action_t::gcd();

        if (t == 0_ms)
          return t;

        if (p()->buffs.backdraft->check() && !p()->buffs.chaotic_inferno->check() )
          t *= backdraft_gcd;

        if (t < min_gcd)
          t = min_gcd;

        return t;
      }

      void execute() override
      {
        destruction_spell_t::execute();

        if ( !p()->buffs.chaotic_inferno->check() )
          p()->buffs.backdraft->decrement();

        p()->buffs.chaotic_inferno->decrement();

        if ( p()->talents.fire_and_brimstone->ok() )
        {
          fnb_action->set_target( target );
          fnb_action->execute();
        }
      }

      void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);

        if (s->result == RESULT_CRIT)
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.incinerate_crits);
      }

      virtual double composite_target_crit_chance(player_t* target) const override
      {
        double m = destruction_spell_t::composite_target_crit_chance(target);

        auto td = this->td(target);
        if (td->debuffs_chaotic_flames->check())
          m += p()->find_spell(253092)->effectN(1).percent();

        return m;
      }
    };

    struct chaos_bolt_t : public destruction_spell_t
    {
      double backdraft_gcd;
      double backdraft_cast_time;
      double refund;
      internal_combustion_t* internal_combustion;

      chaos_bolt_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t(p, "Chaos Bolt"),
        refund(0),
        internal_combustion(new internal_combustion_t(p))
      {
        parse_options(options_str);
        can_havoc = true;

        backdraft_cast_time = 1.0 + p->buffs.backdraft->data().effectN(1).percent();
        backdraft_gcd = 1.0 + p->buffs.backdraft->data().effectN(2).percent();

        add_child(internal_combustion);
      }

      virtual void schedule_execute(action_state_t* state = nullptr) override
      {
        destruction_spell_t::schedule_execute(state);
      }

      virtual timespan_t execute_time() const override
      {
        timespan_t h = warlock_spell_t::execute_time();

        if (p()->buffs.backdraft->check())
          h *= backdraft_cast_time;

        return h;
      }

      timespan_t gcd() const override
      {
        timespan_t t = warlock_spell_t::gcd();

        if (t == 0_ms)
          return t;

        if (p()->buffs.backdraft->check())
          t *= backdraft_gcd;

        if (t < min_gcd)
          t = min_gcd;

        return t;
      }

      void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);

        trigger_internal_combustion( s );

        if (p()->talents.eradication->ok() && result_is_hit(s->result))
          td(s->target)->debuffs_eradication->trigger();
      }

      void trigger_internal_combustion(action_state_t* s)
      {
        if (!p()->talents.internal_combustion->ok())
          return;

        if (!result_is_hit(s->result))
          return;

        auto td = this->td(s->target);
        if (!td->dots_immolate->is_ticking())
          return;

        internal_combustion->set_target(s->target);
        internal_combustion->execute();
      }

      void execute() override
      {
        destruction_spell_t::execute();

        if(p()->azerite.chaotic_inferno.ok())
          p()->buffs.chaotic_inferno->trigger();

        p()->buffs.crashing_chaos->decrement();
        p()->buffs.backdraft->decrement();
      }

      // Force spell to always crit
      double composite_crit_chance() const override
      {
        return 1.0;
      }

      double bonus_da(const action_state_t* s) const override
      {
        double da = destruction_spell_t::bonus_da(s);
        da += p()->azerite.chaotic_inferno.value(2);
        da += p()->buffs.crashing_chaos->check_value();
        return da;
      }

      double calculate_direct_amount(action_state_t* state) const override
      {
        destruction_spell_t::calculate_direct_amount(state);

        // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
        // player spell crit instead. The state target crit chance of the state object is correct.
        // Targeted Crit debuffs function as a separate multiplier.
        state->result_total *= 1.0 + player->cache.spell_crit_chance() + state->target_crit_chance;

        return state->result_total;
      }
    };

    struct channel_demonfire_tick_t : public destruction_spell_t
    {
      channel_demonfire_tick_t(warlock_t* p) :
        destruction_spell_t("channel_demonfire_tick", p, p -> find_spell(196448))
      {
        background = true;
        may_miss = false;
        dual = true;

        spell_power_mod.direct = data().effectN(1).sp_coeff();

        aoe = -1;
        base_aoe_multiplier = data().effectN(2).sp_coeff() / data().effectN(1).sp_coeff();
      }

      void impact(action_state_t* s) override
      {
        destruction_spell_t::impact( s );
        if (s->chain_target == 0)
        {
          expansion::bfa::trigger_leyshocks_grand_compilation( STAT_MASTERY_RATING, p() );
        }
        else
        {
          expansion::bfa::trigger_leyshocks_grand_compilation( STAT_CRIT_RATING, p() );
        }
      }
    };

    struct channel_demonfire_t : public destruction_spell_t
    {
      channel_demonfire_tick_t* channel_demonfire;
      int immolate_action_id;

      channel_demonfire_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("channel_demonfire", p, p -> talents.channel_demonfire),
        channel_demonfire(new channel_demonfire_tick_t(p)),
        immolate_action_id(0)
      {
        parse_options(options_str);
        channeled = true;
        hasted_ticks = true;
        may_crit = false;

        add_child(channel_demonfire);
      }

      void init() override
      {
        destruction_spell_t::init();

        cooldown->hasted = true;
        immolate_action_id = p()->find_action_id("immolate");
      }

      //TODO: This is suboptimal, can this be changed to available_targets() in some way?
      std::vector< player_t* >& target_list() const override
      {
        target_cache.list = destruction_spell_t::target_list();

        size_t i = target_cache.list.size();
        while (i > 0)
        {
          i--;
          player_t* current_target = target_cache.list[i];

          auto td = this->td(current_target);
          if (!td->dots_immolate->is_ticking())
            target_cache.list.erase(target_cache.list.begin() + i);
        }
        return target_cache.list;
      }

      void tick(dot_t* d) override
      {
        // Need to invalidate the target cache to figure out immolated targets.
        target_cache.is_valid = false;

        const auto& targets = target_list();

        if (!targets.empty())
        {
          channel_demonfire->set_target(targets[rng().range(size_t(), targets.size())]);
          channel_demonfire->execute();
        }

        destruction_spell_t::tick(d);
      }

      timespan_t composite_dot_duration(const action_state_t* s) const override
      {
        return s->action->tick_time(s) * 15.0;
      }

      virtual bool ready() override
      {
        double active_immolates = p()->get_active_dots(immolate_action_id);

        if (active_immolates == 0)
          return false;

        return destruction_spell_t::ready();
      }
    };

    struct infernal_awakening_t : public destruction_spell_t
    {
      infernal_awakening_t(warlock_t* p) :
        destruction_spell_t("infernal_awakening", p, p->find_spell(22703))
      {
        destro_mastery = false;
        aoe = -1;
        background = true;
        dual = true;
        trigger_gcd = 0_ms;
      }
    };

    struct summon_infernal_t : public destruction_spell_t
    {
      infernal_awakening_t* infernal_awakening;
      timespan_t infernal_duration;

      summon_infernal_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("Summon_Infernal", p, p -> find_spell(1122)),
        infernal_awakening(nullptr)
      {
        parse_options(options_str);

        harmful = may_crit = false;
        infernal_duration = p->find_spell(111685)->duration() + 1_ms;
        infernal_awakening = new infernal_awakening_t(p);
        infernal_awakening->stats = stats;
        radius = infernal_awakening->radius;
      }

      virtual void execute() override
      {
        destruction_spell_t::execute();

        if (infernal_awakening)
          infernal_awakening->execute();

        for (size_t i = 0; i < p()->warlock_pet_list.infernals.size(); i++)
        {
          if (p()->warlock_pet_list.infernals[i]->is_sleeping())
          {
            p()->warlock_pet_list.infernals[i]->summon(infernal_duration);
          }
        }

        if (p()->azerite.crashing_chaos.ok())
          p()->buffs.crashing_chaos->trigger(p()->buffs.crashing_chaos->max_stack());
      }
    };

    //AOE
    struct rain_of_fire_t : public destruction_spell_t
    {
      struct rain_of_fire_tick_t : public destruction_spell_t
      {
        rain_of_fire_tick_t(warlock_t* p) :
          destruction_spell_t("rain_of_fire_tick", p, p -> find_spell(42223))
        {
          aoe = -1;
          background = dual = direct_tick = true; // Legion TOCHECK
          callbacks = false;
          radius = p->find_spell(5740)->effectN(1).radius();
        }

        void impact(action_state_t* s) override
        {
          destruction_spell_t::impact(s);

          if (p()->talents.inferno && result_is_hit(s->result))
          {
            if (rng().roll(p()->talents.inferno->effectN(1).percent()))
            {
              p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.inferno);
            }
          }
        }

        virtual void execute() override
        {
          destruction_spell_t::execute();
        }
      };

      rain_of_fire_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("rain_of_fire", p, p -> find_spell(5740))
      {
        parse_options(options_str);
        aoe = -1;
        dot_duration = 0_ms;
        may_miss = may_crit = false;
        base_tick_time = data().duration() / 8.0; // ticks 8 times (missing from spell data)
        base_execute_time = 0_ms; // HOTFIX

        if (!p->active.rain_of_fire)
        {
          p->active.rain_of_fire = new rain_of_fire_tick_t(p);
          p->active.rain_of_fire->stats = stats;
        }
      }

      virtual void execute() override
      {
        destruction_spell_t::execute();

        make_event<ground_aoe_event_t>(*sim, p(), ground_aoe_params_t()
          .target(execute_state->target)
          .x(execute_state->target->x_position)
          .y(execute_state->target->y_position)
          .pulse_time(base_tick_time * player->cache.spell_haste())
          .duration(data().duration() * player->cache.spell_haste())
          .start_time(sim->current_time())
          .action(p()->active.rain_of_fire));
      }
    };

    //AOE talents
    struct cataclysm_t : public destruction_spell_t
    {
      immolate_t* immolate;

      cataclysm_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("cataclysm", p, p -> talents.cataclysm),
        immolate(new immolate_t(p, ""))
      {
        parse_options(options_str);
        aoe = -1;

        immolate->background = true;
        immolate->dual = true;
        immolate->base_costs[RESOURCE_MANA] = 0;
        immolate->base_dd_multiplier *= 0.0;
      }

      virtual void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);

        if (result_is_hit(s->result))
        {
          immolate->set_target( s->target );
          immolate->execute();
        }
      }
    };
  } // end actions namespace

  namespace buffs {

  } // end buffs namespace

  // add actions
  action_t* warlock_t::create_action_destruction(const std::string& action_name, const std::string& options_str) {
      using namespace actions_destruction;

      if (action_name == "conflagrate") return new                      conflagrate_t(this, options_str);
      if (action_name == "incinerate") return new                       incinerate_t(this, options_str);
      if (action_name == "immolate") return new                         immolate_t(this, options_str);
      if (action_name == "chaos_bolt") return new                       chaos_bolt_t(this, options_str);
      if (action_name == "rain_of_fire") return new                     rain_of_fire_t(this, options_str);
      if (action_name == "havoc") return new                            havoc_t(this, options_str);
      if (action_name == "summon_infernal") return new                  summon_infernal_t(this, options_str);

      //Talents
      if (action_name == "soul_fire") return new                        soul_fire_t(this, options_str);
      if (action_name == "shadowburn") return new                       shadowburn_t(this, options_str);
      if (action_name == "cataclysm") return new                        cataclysm_t(this, options_str);
      if (action_name == "channel_demonfire") return new                channel_demonfire_t(this, options_str);
      if (action_name == "dark_soul_instability") return new            dark_soul_instability_t(this, options_str);

      return nullptr;
  }
  void warlock_t::create_buffs_destruction() {
    //destruction buffs
    buffs.backdraft = make_buff( this, "backdraft", find_spell( 117828 ) )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_max_stack( find_spell( 117828 )->max_stacks() + ( talents.flashover ? talents.flashover->effectN( 2 ).base_value() : 0 ) );

    buffs.reverse_entropy = make_buff( this, "reverse_entropy", talents.reverse_entropy )
      ->set_default_value( find_spell( 266030 )->effectN( 1 ).percent() )
      ->set_duration( find_spell( 266030 )->duration() )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_trigger_spell( talents.reverse_entropy )
      ->add_invalidate( CACHE_HASTE );

    buffs.grimoire_of_supremacy = make_buff( this, "grimoire_of_supremacy", find_spell( 266091 ) )
      ->set_default_value( find_spell( 266091 )->effectN( 1 ).percent() );

    buffs.dark_soul_instability = make_buff( this, "dark_soul_instability", talents.dark_soul_instability )
      ->add_invalidate( CACHE_SPELL_CRIT_CHANCE )
      ->add_invalidate( CACHE_CRIT_CHANCE )
      ->set_default_value( talents.dark_soul_instability->effectN( 1 ).percent() );

    // Azerite
    buffs.bursting_flare = make_buff<stat_buff_t>( this, "bursting_flare", find_spell( 279913 ) )
      ->add_stat( STAT_MASTERY_RATING, azerite.bursting_flare.value() );
    buffs.chaotic_inferno = make_buff( this, "chaotic_inferno", find_spell( 279673 ) )
      ->set_default_value( find_spell( 279673 )->effectN( 1 ).percent() )
      ->set_chance( find_spell( 279672 )->proc_chance() );
    buffs.crashing_chaos = make_buff( this, "crashing_chaos", find_spell( 277706 ) )
      ->set_default_value( azerite.crashing_chaos.value() );
    buffs.rolling_havoc = make_buff<stat_buff_t>( this, "rolling_havoc", find_spell( 278931 ) )
      ->add_stat( STAT_INTELLECT, azerite.rolling_havoc.value() );
    buffs.flashpoint = make_buff<stat_buff_t>( this, "flashpoint", find_spell( 275429 ) )
      ->add_stat( STAT_HASTE_RATING, azerite.flashpoint.value() );
    //TOCHECK What happens when we get 2 procs within 2 seconds?
    buffs.chaos_shards = make_buff<stat_buff_t>( this, "chaos_shards", find_spell( 287660 ) )
      ->set_period( find_spell( 287660 )->effectN( 1 ).period() )
      ->set_tick_zero( true )
      ->set_tick_callback( [this]( buff_t* b, int, const timespan_t& ) {
      resource_gain( RESOURCE_SOUL_SHARD, b->data().effectN( 1 ).base_value() / 10.0, gains.chaos_shards );
    } );
  }

  void warlock_t::init_spells_destruction() {
    using namespace actions_destruction;

    spec.destruction                    = find_specialization_spell( 137046 );
    mastery_spells.chaotic_energies     = find_mastery_spell( WARLOCK_DESTRUCTION );

    spec.conflagrate                    = find_specialization_spell( "Conflagrate" );
    spec.conflagrate_2                  = find_specialization_spell( 231793 );
    spec.havoc                          = find_specialization_spell( "Havoc" );
    // Talents
    talents.flashover                   = find_talent_spell( "Flashover" );
    talents.eradication                 = find_talent_spell( "Eradication" );
    talents.soul_fire                   = find_talent_spell( "Soul Fire" );

    talents.reverse_entropy             = find_talent_spell( "Reverse Entropy" );
    talents.internal_combustion         = find_talent_spell( "Internal Combustion" );
    talents.shadowburn                  = find_talent_spell( "Shadowburn" );

    talents.inferno                     = find_talent_spell( "Inferno" );
    talents.fire_and_brimstone          = find_talent_spell( "Fire and Brimstone" );
    talents.cataclysm                   = find_talent_spell( "Cataclysm" );

    talents.roaring_blaze               = find_talent_spell( "Roaring Blaze" );
    talents.grimoire_of_supremacy       = find_talent_spell( "Grimoire of Supremacy" );

    talents.channel_demonfire           = find_talent_spell( "Channel Demonfire" );
    talents.dark_soul_instability       = find_talent_spell( "Dark Soul: Instability" );

    // Azerite
    azerite.bursting_flare              = find_azerite_spell( "Bursting Flare" );
    azerite.chaotic_inferno             = find_azerite_spell( "Chaotic Inferno" );
    azerite.crashing_chaos              = find_azerite_spell( "Crashing Chaos" );
    azerite.rolling_havoc               = find_azerite_spell( "Rolling Havoc" );
    azerite.flashpoint                  = find_azerite_spell( "Flashpoint" );
    azerite.chaos_shards                = find_azerite_spell( "Chaos Shards" );
  }

  void warlock_t::init_gains_destruction()
  {
    gains.conflagrate                   = get_gain( "conflagrate" );
    gains.shadowburn                    = get_gain( "shadowburn" );
    gains.immolate                      = get_gain( "immolate" );
    gains.immolate_crits                = get_gain( "immolate_crits" );
    gains.incinerate                    = get_gain( "incinerate" );
    gains.incinerate_crits              = get_gain( "incinerate_crits" );
    gains.incinerate_fnb                = get_gain( "incinerate_fnb" );
    gains.incinerate_fnb_crits          = get_gain( "incinerate_fnb_crits" );
    gains.soul_fire                     = get_gain( "soul_fire" );
    gains.infernal                      = get_gain( "infernal" );
    gains.shadowburn_shard              = get_gain( "shadowburn_shard" );
    gains.inferno                       = get_gain( "inferno" );
  }

  void warlock_t::init_rng_destruction() {
  }

  void warlock_t::init_procs_destruction() {
    procs.reverse_entropy = get_proc("reverse_entropy");
  }

  void warlock_t::create_options_destruction() {
  }

  void warlock_t::create_apl_destruction() {
    action_priority_list_t* def = get_action_priority_list("default");
    action_priority_list_t* cds = get_action_priority_list( "cds" );
    action_priority_list_t* havoc = get_action_priority_list( "havoc" );
    action_priority_list_t* aoe = get_action_priority_list( "aoe" );

    def->add_action("call_action_list,name=havoc,if=havoc_active&active_enemies<5-talent.inferno.enabled+(talent.inferno.enabled&talent.internal_combustion.enabled)", "Havoc uses a special priority list on most multitarget scenarios, but the target threshold can vary depending on talents");
    def->add_talent(this, "Cataclysm", "", "Cataclysm should be used on cooldown as soon as possible");
    def->add_action("call_action_list,name=aoe,if=active_enemies>2", "Two target scenarios are handled like single target with Havoc weaved in. Starting with three targets, a specialized AoE priority is required");
    def->add_action(this, "Immolate", "cycle_targets=1,if=refreshable&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>remains)", "Immolate should never fall off the primary target. If Cataclysm will refresh Immolate before it expires, there's no reason to waste time casting it");
    def->add_action(this, "Immolate", "if=talent.internal_combustion.enabled&action.chaos_bolt.in_flight&remains<duration*0.5", "#With Internal Combustion talented, it's possible Immolate will need to be refreshed sooner than the remaining duration says, if there's already a Chaos Bolt on the way to the target.");
    def->add_action("call_action_list,name=cds", "The general rule of thumb for talents is to maximize the number of uses of each");
    def->add_talent(this, "Channel Demonfire");
    def->add_action(this, "Havoc", "cycle_targets=1,if=!(target=self.target)&(dot.immolate.remains>dot.immolate.duration*0.5|!talent.internal_combustion.enabled)&(!cooldown.summon_infernal.ready|!talent.grimoire_of_supremacy.enabled|talent.grimoire_of_supremacy.enabled&pet.infernal.remains<=10)", "The if condition here always applies Havoc to something other than the primary target");
    def->add_talent(this, "Soul Fire", "", "Soul Fire should be used on cooldown, it does not appear worth saving for generating Soul Shards during cooldowns");
    def->add_action(this, "Conflagrate", "if=buff.backdraft.down&soul_shard>=1.5-0.3*talent.flashover.enabled&!variable.pool_soul_shards", "Conflagrate should only be used to set up Chaos Bolts. Flashover lets Conflagrate be used earlier to set up an Incinerate before CB. If a major cooldown is coming up, save charges for it");
    def->add_talent(this, "Shadowburn", "if=soul_shard<2&(!variable.pool_soul_shards|charges>1)", "Shadowburn is used as a discount Conflagrate to generate shards if you don't have enough for a Chaos Bolt. The same rules about saving it for major cooldowns applies");
    def->add_action("variable,name=pool_soul_shards,value=active_enemies>1&cooldown.havoc.remains<=10|cooldown.summon_infernal.remains<=20&(talent.grimoire_of_supremacy.enabled|talent.dark_soul_instability.enabled&cooldown.dark_soul_instability.remains<=20)|talent.dark_soul_instability.enabled&cooldown.dark_soul_instability.remains<=20&(cooldown.summon_infernal.remains>target.time_to_die|cooldown.summon_infernal.remains+cooldown.summon_infernal.duration>target.time_to_die)", "It's worth stocking up on Soul Shards before a major cooldown usage");
    def->add_action(this, "Chaos Bolt", "if=(talent.grimoire_of_supremacy.enabled|azerite.crashing_chaos.enabled)&pet.infernal.active|buff.dark_soul_instability.up", "Chaos Bolt has several possible use conditions. Crashing Chaos, Grimoire of Supremacy, and Dark Soul: Instability all favor casting as many CBs as possible when any of them are active");
    def->add_action(this, "Chaos Bolt", "if=!variable.pool_soul_shards&!talent.eradication.enabled", "If Soul Shards are not being pooled and Eradication is not talented, just spend CBs as they become available");
    def->add_action(this, "Chaos Bolt", "if=!variable.pool_soul_shards&talent.eradication.enabled&(debuff.eradication.remains<cast_time|buff.backdraft.up)", "With Eradication, it's beneficial to maximize the uptime on the debuff. However, it's still better to use Chaos Bolt immediately if Backdraft is up");
    def->add_action(this, "Chaos Bolt", "if=(soul_shard>=4.5-0.2*active_enemies)", "Even when saving, do not overcap on Soul Shards");
    def->add_action(this, "Conflagrate", "if=charges>1", "Don't overcap on charges of Conflagrate");
    def->add_action(this, "Incinerate");

    cds->add_action(this, "Summon Infernal", "if=cooldown.dark_soul_instability.ready|!talent.dark_soul_instability.enabled", "If both cooldowns are ready, summon the Infernal then activate DSI. If not using DSI, use this on CD");
    cds->add_talent(this, "Dark Soul: Instability", "if=pet.infernal.active");
    cds->add_action(this, "Summon Infernal", "if=target.time_to_die>cooldown.summon_infernal.duration", "If DSI is not ready but you can get more than one infernal in before the end of the fight, summon the Infernal now");
    cds->add_talent(this, "Dark Soul: Instability", "if=target.time_to_die>cooldown.dark_soul_instability.duration+20", "If you can get in more than one more DSI before the end of the fight, use that now");
    cds->add_action(this, "Summon Infernal", "if=talent.dark_soul_instability.enabled&cooldown.dark_soul_instability.remains>target.time_to_die", "If the fight will end before DSI is back up, summon the Infernal");
    cds->add_talent(this, "Dark Soul: Instability", "if=cooldown.summon_infernal.remains>target.time_to_die", "If the fight will end before infernal is back up, use DSI");
    cds->add_action(this, "Summon Infernal", "if=target.time_to_die<30", "If the fight is about to end, use CDs such that they get as much time up as possible");
    cds->add_talent(this, "Dark Soul: Instability", "if=target.time_to_die<20");
    cds->add_action( "potion,if=pet.infernal.active|target.time_to_die<30" );
    cds->add_action( "berserking" );
    cds->add_action( "blood_fury" );
    cds->add_action( "fireblood" );
    cds->add_action( "use_items" );

    havoc->add_action(this, "Conflagrate", "if=buff.backdraft.down&soul_shard>=1&soul_shard<=4");
    havoc->add_action(this, "Immolate", "if=talent.internal_combustion.enabled&remains<duration*0.5|!talent.internal_combustion.enabled&refreshable");
    havoc->add_action(this, "Chaos Bolt", "if=cast_time<havoc_remains");
    havoc->add_talent(this, "Soul Fire");
    havoc->add_talent(this, "Shadowburn", "if=active_enemies<3|!talent.fire_and_brimstone.enabled");
    havoc->add_action(this, "Incinerate", "if=cast_time<havoc_remains");

    aoe->add_action(this, "Rain of Fire", "if=pet.infernal.active&(buff.crashing_chaos.down|!talent.grimoire_of_supremacy.enabled)&(!cooldown.havoc.ready|active_enemies>3)", "Rain of Fire is typically the highest priority action, but certain situations favor using Chaos Bolt instead");
    aoe->add_talent(this, "Channel Demonfire", "if=dot.immolate.remains>cast_time", "Channel Demonfire only needs one Immolate active during its cast for AoE. Primary target is used here for simplicity");
    aoe->add_action(this, "Immolate", "cycle_targets=1,if=remains<5&(!talent.cataclysm.enabled|cooldown.cataclysm.remains>remains)", "Similar to single target, there is no need to refresh Immolates if Cataclysm can do it instead");
    aoe->add_action("call_action_list,name=cds", "Rules for cooldowns do not change for AoE, so call the same list as on single target");
    aoe->add_action(this, "Havoc", "cycle_targets=1,if=!(target=self.target)&active_enemies<4", "Three targets is an in-between case that gives a slight benefit to higher priority Havoc");
    aoe->add_action(this, "Chaos Bolt", "if=talent.grimoire_of_supremacy.enabled&pet.infernal.active&(havoc_active|talent.cataclysm.enabled|talent.inferno.enabled&active_enemies<4)", "Rain of Fire will start to dominate on heavy AoE, but some significant Chaos Bolt buffs will still give higher damage output on occasion");
    aoe->add_action(this, "Rain of Fire", "", "Barring any exceptions, Rain of Fire will be used as much as possible, since multiple copies of it can stack");
    aoe->add_action(this, "Havoc", "cycle_targets=1,if=!(target=self.target)&(!talent.grimoire_of_supremacy.enabled|!talent.inferno.enabled|talent.grimoire_of_supremacy.enabled&pet.infernal.remains<=10)", "Even if the Havoc priority list won't be used, Havoc is pretty much free damage and resources and should be used almost on cooldown");
    aoe->add_action(this, "Incinerate", "if=talent.fire_and_brimstone.enabled&buff.backdraft.up&soul_shard<5-0.2*active_enemies", "Use Fire and Brimstone if Backdraft is active, as long as it will not overcap on Soul Shards");
    aoe->add_talent(this, "Soul Fire", "", "Other Soul Shard generating abilities are good filler if not using Fire and Brimstone");
    aoe->add_action(this, "Conflagrate", "if=buff.backdraft.down");
    aoe->add_talent(this, "Shadowburn", "if=!talent.fire_and_brimstone.enabled");
    aoe->add_action(this, "Incinerate", "", "With Fire and Brimstone, Incinerate will be a strong filler. It's placed here for all talents to prevent accidentally using the single target rotation list");
  }
}
