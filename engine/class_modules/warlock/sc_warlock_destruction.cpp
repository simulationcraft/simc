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
        return can_havoc && target != p()->havoc_target && p()->havoc_target != nullptr && p()->buffs.active_havoc->check();
      }

      size_t available_targets(std::vector<player_t*>& tl) const override
      {
        if (can_havoc)
        {
          tl.clear();
          if ( !target->is_sleeping() )
          {
            tl.push_back( target );
            if ( !p()->havoc_target->is_sleeping() && use_havoc() )
              tl.push_back(p()->havoc_target);
          }
        }
        else
        {
          warlock_spell_t::available_targets( tl );
        }

        return tl.size();
      }

      void init() override
      {
        warlock_spell_t::init();

        if (can_havoc)
        {
          base_aoe_multiplier *= p()->spec.havoc->effectN( 1 ).percent();
          //available_targets() will handle Havoc target selection
          aoe = -1;
        }
      }

      //TODO: Implement triggering on Havoc buff trigger/expiration instead of invalidating on every possible call
      std::vector<player_t*>& target_list() const override
      {
        if(can_havoc)
          target_cache.is_valid = false;

        return warlock_spell_t::target_list();
      }

      void consume_resource() override
      {
        warlock_spell_t::consume_resource();

        if (resource_current == RESOURCE_SOUL_SHARD && p()->in_combat)
        {
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

          if (p()->talents.soul_fire->ok())
            p()->cooldowns.soul_fire->adjust((-1 * (p()->talents.soul_fire->effectN(2).time_value()*last_resource_cost)));
        }
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

      void execute() override
      {
        destruction_spell_t::execute();

        p()->buffs.backdraft->decrement();
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
      timespan_t havoc_duration;

      havoc_t(warlock_t* p, const std::string& options_str) : destruction_spell_t(p, "Havoc")
      {
        parse_options(options_str);
        may_crit = false;
        havoc_duration = p->find_spell(80240)->duration();
      }

      void execute() override
      {
        destruction_spell_t::execute();

        p()->havoc_target = execute_state->target;
        p()->buffs.active_havoc->trigger();
      }

      void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);

        td(s->target)->debuffs_havoc->trigger(1, buff_t::DEFAULT_VALUE(), -1.0, havoc_duration);
      }
    };

    //TODO: Check if initial damage of Immolate is reduced on Havoc'd target
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

        if (d->state->result_amount > 0.0 && p()->azerite.flashpoint.ok() && d->target->health_percentage() > 80 )
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

        // nor the havoced target if applicable
        if (use_havoc() )
        {
          it = range::find(tl, p()->havoc_target);
          if (it != tl.end())
          {
            tl.erase(it);
          }
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
          fnb_action->set_target( execute_state->target );
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


    buffs.active_havoc = make_buff( this, "active_havoc" )
      ->set_tick_behavior( buff_tick_behavior::NONE )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_duration( 10_s );

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
    buffs.accelerant = make_buff<stat_buff_t>( this, "accelerant", azerite.accelerant )
      ->add_stat( STAT_HASTE_RATING, azerite.accelerant.value() )
      ->set_duration( find_spell( 272957 )->duration() );
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
    azerite.accelerant                  = find_azerite_spell( "Accelerant" );
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
    gains.destruction_t20_2pc           = get_gain( "destruction_t20_2pc" );
    gains.chaos_shards                  = get_gain( "chaos_shards" );
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
    action_priority_list_t* fnb = get_action_priority_list( "fnb" );
    action_priority_list_t* cata = get_action_priority_list( "cata" );
    action_priority_list_t* inf = get_action_priority_list( "inf" );

    def->add_action( "run_action_list,name=cata,if=spell_targets.infernal_awakening>=3+raid_event.invulnerable.up&talent.cataclysm.enabled" );
    def->add_action( "run_action_list,name=fnb,if=spell_targets.infernal_awakening>=3+raid_event.invulnerable.up&talent.fire_and_brimstone.enabled" );
    def->add_action( "run_action_list,name=inf,if=spell_targets.infernal_awakening>=3+raid_event.invulnerable.up&talent.inferno.enabled" );
    def->add_talent( this, "Cataclysm");
    def->add_action( "immolate,cycle_targets=1,if=!debuff.havoc.remains&(refreshable|talent.internal_combustion.enabled&action.chaos_bolt.in_flight&remains-action.chaos_bolt.travel_time-5<duration*0.3)");
    def->add_action( "call_action_list,name=cds" );
    def->add_talent( this, "Channel Demonfire", "if=!buff.active_havoc.remains" );
    def->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&active_enemies>1+raid_event.invulnerable.up");
    def->add_action( "havoc,if=active_enemies>1+raid_event.invulnerable.up");
    def->add_talent( this, "Soul Fire", "cycle_targets=1,if=!debuff.havoc.remains" );
    def->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&execute_time+travel_time<target.time_to_die&(trinket.proc.intellect.react&trinket.proc.intellect.remains>cast_time|trinket.proc.mastery.react&trinket.proc.mastery.remains>cast_time|trinket.proc.versatility.react&trinket.proc.versatility.remains>cast_time|trinket.proc.crit.react&trinket.proc.crit.remains>cast_time|trinket.proc.spell_power.react&trinket.proc.spell_power.remains>cast_time)" );
    def->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&execute_time+travel_time<target.time_to_die&(trinket.stacking_proc.intellect.react&trinket.stacking_proc.intellect.remains>cast_time|trinket.stacking_proc.mastery.react&trinket.stacking_proc.mastery.remains>cast_time|trinket.stacking_proc.versatility.react&trinket.stacking_proc.versatility.remains>cast_time|trinket.stacking_proc.crit.react&trinket.stacking_proc.crit.remains>cast_time|trinket.stacking_proc.spell_power.react&trinket.stacking_proc.spell_power.remains>cast_time)" );
    def->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&execute_time+travel_time<target.time_to_die&(cooldown.summon_infernal.remains>=20|!talent.grimoire_of_supremacy.enabled)&(cooldown.dark_soul_instability.remains>=20|!talent.dark_soul_instability.enabled)&(talent.eradication.enabled&debuff.eradication.remains<=cast_time|buff.backdraft.remains|talent.internal_combustion.enabled)" );
    def->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&execute_time+travel_time<target.time_to_die&(soul_shard>=4|buff.dark_soul_instability.remains>cast_time|pet.infernal.active|buff.active_havoc.remains>cast_time)" );
    def->add_action("conflagrate,cycle_targets=1,if=!debuff.havoc.remains&((talent.flashover.enabled&buff.backdraft.stack<=2)|(!talent.flashover.enabled&buff.backdraft.stack<2))");
    def->add_talent( this, "Shadowburn", "cycle_targets=1,if=!debuff.havoc.remains&((charges=2|!buff.backdraft.remains|buff.backdraft.remains>buff.backdraft.stack*action.incinerate.execute_time))");
    def->add_action("incinerate,cycle_targets=1,if=!debuff.havoc.remains");

    cds->add_action( "summon_infernal,if=target.time_to_die>=210|!cooldown.dark_soul_instability.remains|target.time_to_die<=30+gcd|!talent.dark_soul_instability.enabled" );
    cds->add_talent( this, "Dark Soul: Instability", "if=target.time_to_die>=140|pet.infernal.active|target.time_to_die<=20+gcd" );
    cds->add_action( "potion,if=pet.infernal.active|target.time_to_die<65" );
    cds->add_action( "berserking" );
    cds->add_action( "blood_fury" );
    cds->add_action( "fireblood" );
    cds->add_action( "use_items" );

    fnb->add_action( "call_action_list,name=cds" );
    fnb->add_action( "rain_of_fire,if=soul_shard>=4.5" );
    fnb->add_action( "immolate,if=talent.channel_demonfire.enabled&!remains&cooldown.channel_demonfire.remains<=action.chaos_bolt.execute_time" );
    fnb->add_action( "channel_demonfire,if=!buff.active_havoc.remains" );
    fnb->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=4+raid_event.invulnerable.up&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    fnb->add_action( "havoc,if=spell_targets.rain_of_fire<=4+raid_event.invulnerable.up&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    fnb->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&talent.grimoire_of_supremacy.enabled&pet.infernal.remains>execute_time&active_enemies<=4+raid_event.invulnerable.up&((108*(spell_targets.rain_of_fire+raid_event.invulnerable.up)%3)<(240*(1+0.08*buff.grimoire_of_supremacy.stack)%2*(1+buff.active_havoc.remains>execute_time)))" );
    fnb->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=4+raid_event.invulnerable.up" );
    fnb->add_action( "havoc,if=spell_targets.rain_of_fire<=4+raid_event.invulnerable.up" );
    fnb->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&buff.active_havoc.remains>execute_time&spell_targets.rain_of_fire<=4+raid_event.invulnerable.up" );
    fnb->add_action( "immolate,cycle_targets=1,if=!debuff.havoc.remains&refreshable&spell_targets.incinerate<=8+raid_event.invulnerable.up" );
    fnb->add_action( "rain_of_fire" );
    fnb->add_action( "soul_fire,cycle_targets=1,if=!debuff.havoc.remains&spell_targets.incinerate<=3+raid_event.invulnerable.up" );
    fnb->add_action( "conflagrate,cycle_targets=1,if=!debuff.havoc.remains&(talent.flashover.enabled&buff.backdraft.stack<=2|spell_targets.incinerate<=7+raid_event.invulnerable.up|talent.roaring_blaze.enabled&spell_targets.incinerate<=9+raid_event.invulnerable.up)" );
    fnb->add_action( "incinerate,cycle_targets=1,if=!debuff.havoc.remains" );

    cata->add_action( "call_action_list,name=cds" );
    cata->add_action( "rain_of_fire,if=soul_shard>=4.5" );
    cata->add_action( "cataclysm" );
    cata->add_action( "immolate,if=talent.channel_demonfire.enabled&!remains&cooldown.channel_demonfire.remains<=action.chaos_bolt.execute_time" );
    cata->add_action( "channel_demonfire,if=!buff.active_havoc.remains" );
    cata->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=8+raid_event.invulnerable.up&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    cata->add_action( "havoc,if=spell_targets.rain_of_fire<=8+raid_event.invulnerable.up&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    cata->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&talent.grimoire_of_supremacy.enabled&pet.infernal.remains>execute_time&active_enemies<=8+raid_event.invulnerable.up&((108*(spell_targets.rain_of_fire+raid_event.invulnerable.up)%3)<(240*(1+0.08*buff.grimoire_of_supremacy.stack)%2*(1+buff.active_havoc.remains>execute_time)))" );
    cata->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=4+raid_event.invulnerable.up" );
    cata->add_action( "havoc,if=spell_targets.rain_of_fire<=4+raid_event.invulnerable.up" );
    cata->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&buff.active_havoc.remains>execute_time&spell_targets.rain_of_fire<=4+raid_event.invulnerable.up" );
    cata->add_action( "immolate,cycle_targets=1,if=!debuff.havoc.remains&refreshable&remains<=cooldown.cataclysm.remains" );
    cata->add_action( "rain_of_fire" );
    cata->add_action( "soul_fire,cycle_targets=1,if=!debuff.havoc.remains" );
    cata->add_action( "conflagrate,cycle_targets=1,if=!debuff.havoc.remains" );
    cata->add_action( "shadowburn,cycle_targets=1,if=!debuff.havoc.remains&((charges=2|!buff.backdraft.remains|buff.backdraft.remains>buff.backdraft.stack*action.incinerate.execute_time))" );
    cata->add_action( "incinerate,cycle_targets=1,if=!debuff.havoc.remains" );

    inf->add_action( "call_action_list,name=cds" );
    inf->add_action( "rain_of_fire,if=soul_shard>=4.5" );
    inf->add_action( "cataclysm" );
    inf->add_action( "immolate,if=talent.channel_demonfire.enabled&!remains&cooldown.channel_demonfire.remains<=action.chaos_bolt.execute_time" );
    inf->add_action( "channel_demonfire,if=!buff.active_havoc.remains" );
    inf->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=4+raid_event.invulnerable.up+talent.internal_combustion.enabled&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    inf->add_action( "havoc,if=spell_targets.rain_of_fire<=4+raid_event.invulnerable.up+talent.internal_combustion.enabled&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    inf->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&talent.grimoire_of_supremacy.enabled&pet.infernal.remains>execute_time&spell_targets.rain_of_fire<=4+raid_event.invulnerable.up+talent.internal_combustion.enabled&((108*(spell_targets.rain_of_fire+raid_event.invulnerable.up)%(3-0.16*(spell_targets.rain_of_fire+raid_event.invulnerable.up)))<(240*(1+0.08*buff.grimoire_of_supremacy.stack)%2*(1+buff.active_havoc.remains>execute_time)))" );
    inf->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=3+raid_event.invulnerable.up&(talent.eradication.enabled|talent.internal_combustion.enabled)" );
    inf->add_action( "havoc,if=spell_targets.rain_of_fire<=3+raid_event.invulnerable.up&(talent.eradication.enabled|talent.internal_combustion.enabled)" );
    inf->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&buff.active_havoc.remains>execute_time&spell_targets.rain_of_fire<=3+raid_event.invulnerable.up&(talent.eradication.enabled|talent.internal_combustion.enabled)" );
    inf->add_action( "immolate,cycle_targets=1,if=!debuff.havoc.remains&refreshable" );
    inf->add_action( "rain_of_fire" );
    inf->add_action( "soul_fire,cycle_targets=1,if=!debuff.havoc.remains" );
    inf->add_action( "conflagrate,cycle_targets=1,if=!debuff.havoc.remains" );
    inf->add_action( "shadowburn,cycle_targets=1,if=!debuff.havoc.remains&((charges=2|!buff.backdraft.remains|buff.backdraft.remains>buff.backdraft.stack*action.incinerate.execute_time))" );
    inf->add_action( "incinerate,cycle_targets=1,if=!debuff.havoc.remains" );
  }
}
