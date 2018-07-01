#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock {
  namespace actions_destruction {
    using namespace actions;

    //Tier
    struct flames_of_argus_t : public residual_action_t
    {
      flames_of_argus_t(warlock_t* player) :
        residual_action_t("flames_of_argus", player, player -> find_spell(253097))
      {
        background = true;
        may_miss = may_crit = false;
        school = SCHOOL_CHROMATIC;
      }
    };
    //Talents
    struct soul_fire_t : public warlock_spell_t
    {
      soul_fire_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t("soul_fire", p, p -> talents.soul_fire)
      {
        parse_options(options_str);
        energize_type = ENERGIZE_ON_CAST;
        energize_resource = RESOURCE_SOUL_SHARD;
        energize_amount = (std::double_t(p->find_spell(281490)->effectN(1).base_value()) / 10);

        can_havoc = true;
      }

      void execute() override
      {
        warlock_spell_t::execute();

        p()->buffs.backdraft->decrement();
      }
    };

    struct internal_combustion_t : public warlock_spell_t
    {
      internal_combustion_t(warlock_t* p) :
        warlock_spell_t("Internal Combustion", p, p->talents.internal_combustion)
      {
        background = true;
        destro_mastery = false;
      }

      void init() override
      {
        warlock_spell_t::init();

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

        warlock_spell_t::execute();
        td->dots_immolate->reduce_duration(remaining);
      }
    };

    struct shadowburn_t : public warlock_spell_t
    {
      shadowburn_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t("shadowburn", p, p -> talents.shadowburn)
      {
        parse_options(options_str);

        can_havoc = true;
      }

      virtual void impact(action_state_t* s) override
      {
        warlock_spell_t::impact(s);

        if (result_is_hit(s->result))
        {
          td(s->target)->debuffs_shadowburn->trigger();
          p()->resource_gain(RESOURCE_SOUL_SHARD, (std::double_t(p()->find_spell(245731)->effectN(1).base_value()) / 10), p()->gains.shadowburn);
        }
      }
    };

    struct roaring_blaze_t : public warlock_spell_t {
      roaring_blaze_t(warlock_t* p) :
        warlock_spell_t("roaring_blaze", p, p -> find_spell(265931))
      {
        background = true;
        base_tick_time = timespan_t::from_seconds(2.0);
        dot_duration = data().duration();
        spell_power_mod.tick = data().effectN(1).sp_coeff();
        destro_mastery = false;
        hasted_ticks = true;
      }
    }; //damage is not correct

    struct dark_soul_instability_t : public warlock_spell_t
    {
      dark_soul_instability_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t("dark_soul_instability", p, p -> talents.dark_soul_instability)
      {
        parse_options(options_str);
        harmful = may_crit = may_miss = false;
      }

      void execute() override
      {
        warlock_spell_t::execute();

        p()->buffs.dark_soul_instability->trigger();
      }
    };

    //Spells
    struct havoc_t : public warlock_spell_t
    {
      timespan_t havoc_duration;

      havoc_t(warlock_t* p, const std::string& options_str) : warlock_spell_t(p, "Havoc")
      {
        parse_options(options_str);
        may_crit = false;
        havoc_duration = p->find_spell(80240)->duration();
      }

      void execute() override
      {
        warlock_spell_t::execute();

        p()->havoc_target = execute_state->target;
        p()->buffs.active_havoc->trigger();
      }

      void impact(action_state_t* s) override
      {
        warlock_spell_t::impact(s);

        td(s->target)->debuffs_havoc->trigger(1, buff_t::DEFAULT_VALUE(), -1.0, havoc_duration);
      }
    };

    struct immolate_t : public warlock_spell_t
    {
      immolate_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t("immolate", p, p -> find_spell(348))
      {
        parse_options(options_str);
        const spell_data_t* dmg_spell = player->find_spell(157736);

        can_havoc = true;

        base_tick_time = dmg_spell->effectN(1).period();
        dot_duration = dmg_spell->duration();
        spell_power_mod.tick = dmg_spell->effectN(1).sp_coeff();
        spell_power_mod.direct = data().effectN(1).sp_coeff();
        hasted_ticks = true;
        tick_may_crit = true;
      }

      virtual void tick(dot_t* d) override
      {
        warlock_spell_t::tick(d);

        if (d->state->result == RESULT_CRIT && rng().roll(data().effectN(2).percent()))
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate_crits);

        p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate);

        auto td = find_td(this->target);
        if (d->state->result_amount > 0.0 && p()->azerite.flashpoint.ok() && target->health_percentage() > 0.80)
          p()->buffs.flashpoint->trigger();
      }
    };

    struct conflagrate_t : public warlock_spell_t
    {
      timespan_t total_duration;
      timespan_t base_duration;
      roaring_blaze_t* roaring_blaze;
      conflagrate_t* havoc_cast;

      conflagrate_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t("Conflagrate", p, p -> find_spell(17962)),
        total_duration(),
        base_duration(),
        roaring_blaze(new roaring_blaze_t(p)),
        havoc_cast()
      {
        //special case to stop havocd conflags using charges
        if (options_str == "havoc")
        {
          can_havoc = false;
          background = true;
          cooldown = p->get_cooldown("conflag_havoc");
        }
        else
        {
          parse_options(options_str);
          can_havoc = true;
        }

        energize_type = ENERGIZE_NONE;

        cooldown->charges += p->spec.conflagrate_2->effectN(1).base_value();

        cooldown->charges += p->sets->set(WARLOCK_DESTRUCTION, T19, B4)->effectN(1).base_value();
        cooldown->duration += p->sets->set(WARLOCK_DESTRUCTION, T19, B4)->effectN(2).time_value();

        add_child(roaring_blaze);
      }

      void init() override
      {
        warlock_spell_t::init();

        cooldown->hasted = true;

        //special case to stop havocd conflags using charges
        if (can_havoc)
        {
          havoc_cast = new conflagrate_t(p(), "havoc");
          add_child(havoc_cast);
        }
      }

      void impact(action_state_t* s) override
      {
        warlock_spell_t::impact(s);

        p()->buffs.backdraft->trigger( 1 + ( p()->talents.flashover->ok() ? p()->talents.flashover->effectN(1).base_value() : 0 ) );

        if (result_is_hit(s->result))
        {
          if (p()->talents.roaring_blaze->ok() && !havocd)
            roaring_blaze->execute();

          p()->resource_gain(RESOURCE_SOUL_SHARD, (std::double_t(p()->find_spell(245330)->effectN(1).base_value()) / 10), p()->gains.conflagrate);
        }
      }

      void execute() override
      {
        //special case to stop havocd conflags using charges
        if (can_havoc && p()->havoc_target && havocd)
        {
          assert(havoc_cast);
          havoc_cast->set_target(p()->havoc_target);
          havoc_cast->havocd = true;
          havoc_cast->execute();
          return;
        }

        warlock_spell_t::execute();

        auto td = find_td(this->target);
        if (p()->azerite.bursting_flare.ok() && td->dots_immolate->is_ticking())
          p()->buffs.bursting_flare->trigger();

        sim->print_log("{}: Action {} {} charges remain", player->name(), name(), this->cooldown->current_charge);
      }

      double action_multiplier()const override
      {
        double m = warlock_spell_t::action_multiplier();

        if (p()->talents.flashover)
        {
          m *= 1.0 + p()->talents.flashover->effectN(3).percent();
        }

        return m;
      }
    };

    struct incinerate_t : public warlock_spell_t
    {
      double backdraft_gcd;
      double backdraft_cast_time;

      incinerate_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t(p, "Incinerate")
      {
        parse_options(options_str);
        if (p->talents.fire_and_brimstone->ok() && !havocd)
          aoe = -1;

        can_havoc = true;

        backdraft_cast_time = 1.0 + p->buffs.backdraft->data().effectN(1).percent();
        backdraft_gcd = 1.0 + p->buffs.backdraft->data().effectN(2).percent();

        energize_type = ENERGIZE_ON_CAST;
        energize_resource = RESOURCE_SOUL_SHARD;
        energize_amount = std::double_t(p->find_spell(244670)->effectN(1).base_value()) / 10;
      }

      virtual timespan_t execute_time() const override
      {
        timespan_t h = spell_t::execute_time();

        if (p()->buffs.backdraft->check())
          h *= backdraft_cast_time;

        if (p()->buffs.chaotic_inferno->check())
          h *= 1.0 + p()->buffs.chaotic_inferno->check_value();

        return h;
      }

      timespan_t gcd() const override
      {
        timespan_t t = action_t::gcd();

        if (t == timespan_t::zero())
          return t;

        if (p()->buffs.backdraft->check())
          t *= backdraft_gcd;

        if (t < min_gcd)
          t = min_gcd;

        return t;
      }

      void execute() override
      {
        warlock_spell_t::execute();

        p()->buffs.chaotic_inferno->decrement();

        if (execute_state->target == p()->havoc_target)
          havocd = true;

        if(!havocd)
          p()->buffs.backdraft->decrement();

        if (!(execute_state->target == this->target))
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.fnb_bits);
        if (execute_state->result == RESULT_CRIT)
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.incinerate_crits);
        if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T20, B2))
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.destruction_t20_2pc);
      }

      virtual double composite_target_crit_chance(player_t* target) const override
      {
        double m = warlock_spell_t::composite_target_crit_chance(target);

        if (auto td = this->find_td( target ))
        {
          if (td->debuffs_chaotic_flames->check())
            m += p()->find_spell(253092)->effectN(1).percent();
        }

        return m;
      }
    };

    struct duplicate_chaos_bolt_t : public warlock_spell_t
    {
      player_t* original_target;
      flames_of_argus_t* flames_of_argus;

      duplicate_chaos_bolt_t(warlock_t* p) :
        warlock_spell_t("chaos_bolt_magistrike", p, p -> find_spell(213229)),
        original_target(nullptr),
        flames_of_argus(nullptr)
      {
        background = dual = true;
        base_multiplier *= 1.0 + (p->sets->set(WARLOCK_DESTRUCTION, T18, B2)->effectN(2).percent());
        base_multiplier *= 1.0 + (p->sets->set(WARLOCK_DESTRUCTION, T17, B4)->effectN(1).percent());

        if (p->sets->has_set_bonus(WARLOCK_DESTRUCTION, T21, B4))
        {
          flames_of_argus = new flames_of_argus_t(p);
        }
      }

      timespan_t travel_time() const override
      {
        double distance;
        distance = original_target->get_player_distance(*target);

        if (execute_state && execute_state->target)
          distance += execute_state->target->height;

        if (distance == 0) return timespan_t::zero();

        double t = distance / travel_speed;

        double v = sim->travel_variance;

        if (v)
          t = rng().gauss(t, v);

        return timespan_t::from_seconds(t);
      }

      std::vector< player_t* >& target_list() const override
      {
        target_cache.list.clear();
        for (size_t j = 0; j < sim->target_non_sleeping_list.size(); ++j)
        {
          player_t* duplicate_target = sim->target_non_sleeping_list[j];
          if (target == duplicate_target)
            continue;
          if (target->get_player_distance(*duplicate_target) <= 30)
            target_cache.list.push_back(duplicate_target);
        }
        return target_cache.list;
      }

      // Force spell to always crit
      double composite_crit_chance() const override
      {
        return 1.0;
      }

      double bonus_da(const action_state_t* s) const override
      {
        double da = warlock_spell_t::bonus_da(s);
        da += p()->azerite.chaotic_inferno.value(2);
        return da;
      }

      double calculate_direct_amount(action_state_t* state) const override
      {
        warlock_spell_t::calculate_direct_amount(state);

        // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
        // player spell crit instead. The state target crit chance of the state object is correct.
        // Targeted Crit debuffs function as a separate multiplier.
        state->result_total *= 1.0 + player->cache.spell_crit_chance() + state->target_crit_chance;

        return state->result_total;
      }

      void impact(action_state_t* s) override
      {
        warlock_spell_t::impact(s);

        if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T21, B2))
          td(s->target)->debuffs_chaotic_flames->trigger();

        if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T21, B4))
        {
          residual_action::trigger(flames_of_argus, s->target, s->result_amount * p()->sets->set(WARLOCK_DESTRUCTION, T21, B4)->effectN(1).percent());
        }
      }
    };

    struct chaos_bolt_t : public warlock_spell_t
    {
      double backdraft_gcd;
      double backdraft_cast_time;
      double refund;
      duplicate_chaos_bolt_t* duplicate;
      double duplicate_chance;
      flames_of_argus_t* flames_of_argus;
      internal_combustion_t* internal_combustion;

      chaos_bolt_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t(p, "Chaos Bolt"),
        refund(0),
        duplicate(nullptr),
        duplicate_chance(0),
        flames_of_argus(nullptr),
        internal_combustion(new internal_combustion_t(p))
      {
        parse_options(options_str);
        can_havoc = true;
        affected_by_destruction_t20_4pc = true;

        backdraft_cast_time = 1.0 + p->buffs.backdraft->data().effectN(1).percent();
        backdraft_gcd = 1.0 + p->buffs.backdraft->data().effectN(2).percent();

        duplicate = new duplicate_chaos_bolt_t(p);
        duplicate_chance = p->find_spell(213014)->proc_chance();
        duplicate->travel_speed = travel_speed;
        add_child(duplicate);
        add_child(internal_combustion);

        if (p->sets->has_set_bonus(WARLOCK_DESTRUCTION, T21, B4))
        {
          flames_of_argus = new flames_of_argus_t(p);
          add_child(flames_of_argus);
        }
      }

      virtual void schedule_execute(action_state_t* state = nullptr) override
      {
        warlock_spell_t::schedule_execute(state);

        if (p()->buffs.embrace_chaos->check())
        {
          p()->procs.t19_2pc_chaos_bolts->occur();
        }
      }

      virtual timespan_t execute_time() const override
      {
        timespan_t h = spell_t::execute_time();

        if (p()->buffs.backdraft->check())
          h *= backdraft_cast_time;

        if (p()->buffs.embrace_chaos->check())
          h *= 1.0 + p()->buffs.embrace_chaos->data().effectN(1).percent();

        return h;
      }

      timespan_t gcd() const override
      {
        timespan_t t = action_t::gcd();

        if (t == timespan_t::zero())
          return t;

        if (p()->buffs.backdraft->check())
          t *= backdraft_gcd;

        if (t < min_gcd)
          t = min_gcd;

        return t;
      }

      void impact(action_state_t* s) override
      {
        warlock_spell_t::impact(s);
        if (p()->talents.eradication->ok() && result_is_hit(s->result))
          td(s->target)->debuffs_eradication->trigger();
        trigger_internal_combustion(s);
        if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T21, B2))
          td(s->target)->debuffs_chaotic_flames->trigger();
        if (!havocd && p()->legendary.magistrike && rng().roll(duplicate_chance))
        {
          duplicate->original_target = s->target;
          duplicate->target = s->target;
          duplicate->target_cache.is_valid = false;
          duplicate->target_list();
          duplicate->target_cache.is_valid = true;
          if (duplicate->target_cache.list.size() > 0)
          {
            size_t target_to_strike = rng().range(size_t(), duplicate->target_cache.list.size());
            duplicate->target = duplicate->target_cache.list[target_to_strike];
            duplicate->execute();
          }
        }
        if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T21, B4))
        {
          residual_action::trigger(flames_of_argus, s->target, s->result_amount * p()->sets->set(WARLOCK_DESTRUCTION, T21, B4)->effectN(1).percent());
        }
      }

      void trigger_internal_combustion(action_state_t* s)
      {
        if (!p()->talents.internal_combustion->ok())
          return;

        if (!result_is_hit(s->result))
          return;

        auto td = this->find_td(s->target);
        if (!td || !td->dots_immolate->is_ticking())
          return;

        internal_combustion->execute();
      }

      void execute() override
      {
        warlock_spell_t::execute();

        p()->buffs.embrace_chaos->trigger();
        if(p()->azerite.chaotic_inferno.ok())
          p()->buffs.chaotic_inferno->trigger();
        p()->buffs.crashing_chaos->decrement();

        if(!havocd)
          p()->buffs.backdraft->decrement();
      }

      // Force spell to always crit
      double composite_crit_chance() const override
      {
        return 1.0;
      }

      double bonus_da(const action_state_t* s) const override
      {
        double da = warlock_spell_t::bonus_da(s);
        da += p()->azerite.chaotic_inferno.value(2);
        da += p()->buffs.crashing_chaos->check_value();
        return da;
      }

      double calculate_direct_amount(action_state_t* state) const override
      {
        warlock_spell_t::calculate_direct_amount(state);

        // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
        // player spell crit instead. The state target crit chance of the state object is correct.
        // Targeted Crit debuffs function as a separate multiplier.
        state->result_total *= 1.0 + player->cache.spell_crit_chance() + state->target_crit_chance;

        return state->result_total;
      }
    };

    struct channel_demonfire_tick_t : public warlock_spell_t
    {
      channel_demonfire_tick_t(warlock_t* p) :
        warlock_spell_t("channel_demonfire_tick", p, p -> find_spell(196448))
      {
        background = true;
        may_miss = false;
        dual = true;

        can_feretory = false;

        spell_power_mod.direct = data().effectN(1).sp_coeff();

        aoe = -1;
        base_aoe_multiplier = data().effectN(2).sp_coeff() / data().effectN(1).sp_coeff();
      }
    };

    struct channel_demonfire_t : public warlock_spell_t
    {
      channel_demonfire_tick_t* channel_demonfire;
      int immolate_action_id;

      channel_demonfire_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t("channel_demonfire", p, p -> talents.channel_demonfire),
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
        warlock_spell_t::init();

        cooldown->hasted = true;
        immolate_action_id = p()->find_action_id("immolate");
      }

      std::vector< player_t* >& target_list() const override
      {
        target_cache.list = warlock_spell_t::target_list();

        size_t i = target_cache.list.size();
        while (i > 0)
        {
          i--;
          player_t* current_target = target_cache.list[i];

          auto td = find_td(current_target);
          if (!td || !td->dots_immolate->is_ticking())
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

        warlock_spell_t::tick(d);
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

        return warlock_spell_t::ready();
      }
    };

    struct infernal_awakening_t : public warlock_spell_t
    {
      infernal_awakening_t(warlock_t* p) :
        warlock_spell_t("infernal_awakening", p, p->find_spell(22703))
      {
        destro_mastery = false;
        aoe = -1;
        background = true;
        dual = true;
        trigger_gcd = timespan_t::zero();
      }
    };

    struct summon_infernal_t : public warlock_spell_t
    {
      infernal_awakening_t* infernal_awakening;
      timespan_t infernal_duration;

      summon_infernal_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t("Summon_Infernal", p, p -> find_spell(1122)),
        infernal_awakening(nullptr)
      {
        parse_options(options_str);

        harmful = may_crit = false;
        infernal_duration = p->find_spell(111685)->duration() + timespan_t::from_millis(1);
        infernal_awakening = new infernal_awakening_t(p);
        infernal_awakening->stats = stats;
        radius = infernal_awakening->radius;
      }

      virtual void execute() override
      {
        warlock_spell_t::execute();

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
    struct rain_of_fire_t : public warlock_spell_t
    {
      struct rain_of_fire_tick_t : public warlock_spell_t
      {
        rain_of_fire_tick_t(warlock_t* p) :
          warlock_spell_t("rain_of_fire_tick", p, p -> find_spell(42223))
        {
          aoe = -1;
          background = dual = direct_tick = true; // Legion TOCHECK
          callbacks = false;
          radius = p->find_spell(5740)->effectN(1).radius();
        }

        void impact(action_state_t* s) override
        {
          warlock_spell_t::impact(s);

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
          warlock_spell_t::execute();
          if (this->num_targets_hit >= 3 && p()->azerite.accelerant.ok())
            p()->buffs.accelerant->trigger();
        }
      };

      rain_of_fire_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t("rain_of_fire", p, p -> find_spell(5740))
      {
        parse_options(options_str);
        aoe = -1;
        dot_duration = timespan_t::zero();
        may_miss = may_crit = false;
        base_tick_time = data().duration() / 8.0; // ticks 8 times (missing from spell data)
        base_execute_time = timespan_t::zero(); // HOTFIX

        if (!p->active.rain_of_fire)
        {
          p->active.rain_of_fire = new rain_of_fire_tick_t(p);
          p->active.rain_of_fire->stats = stats;
        }
      }

      virtual void execute() override
      {
        warlock_spell_t::execute();

        make_event<ground_aoe_event_t>(*sim, p(), ground_aoe_params_t()
          .target(execute_state->target)
          .x(execute_state->target->x_position)
          .y(execute_state->target->y_position)
          .pulse_time(base_tick_time * player->cache.spell_haste())
          .duration(data().duration() * player->cache.spell_haste())
          .start_time(sim->current_time())
          .action(p()->active.rain_of_fire));

        if (p()->legendary.alythesss_pyrogenics)
          p()->buffs.alythesss_pyrogenics->trigger(1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() * player->cache.spell_haste());
      }
    };

    //AOE talents
    struct cataclysm_t : public warlock_spell_t
    {
      immolate_t* immolate;

      cataclysm_t(warlock_t* p, const std::string& options_str) :
        warlock_spell_t("cataclysm", p, p -> talents.cataclysm),
        immolate(new immolate_t(p, ""))
      {
        parse_options(options_str);
        aoe = -1;

        immolate->background = true;
        immolate->dual = true;
        immolate->base_costs[RESOURCE_MANA] = 0;
      }

      virtual void impact(action_state_t* s) override
      {
        warlock_spell_t::impact(s);

        if (result_is_hit(s->result))
        {
          immolate->target = s->target;
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
    buffs.backdraft = make_buff(this, "backdraft", find_spell(117828))
      ->set_refresh_behavior(buff_refresh_behavior::DURATION)
      ->set_max_stack( find_spell(117828)->max_stacks() + ( talents.flashover ? talents.flashover->effectN(2).base_value() : 0 ) );

    buffs.embrace_chaos = make_buff(this, "embrace_chaos", sets->set(WARLOCK_DESTRUCTION, T19, B2)->effectN(1).trigger())
      ->set_chance(sets->set(WARLOCK_DESTRUCTION, T19, B2)->proc_chance());

    buffs.active_havoc = make_buff(this, "active_havoc")
      ->set_tick_behavior(buff_tick_behavior::NONE)
      ->set_refresh_behavior(buff_refresh_behavior::DURATION)
      ->set_duration(timespan_t::from_seconds(10));

    buffs.reverse_entropy = make_buff<haste_buff_t>(this, "reverse_entropy", talents.reverse_entropy)
      ->set_default_value(find_spell(266030)->effectN(1).percent())
      ->set_duration(find_spell(266030)->duration())
      ->set_refresh_behavior(buff_refresh_behavior::DURATION)
      ->set_trigger_spell(talents.reverse_entropy);

    buffs.grimoire_of_supremacy = make_buff(this, "grimoire_of_supremacy", find_spell(266091))
      ->set_default_value(find_spell(266091)->effectN(1).percent());

    buffs.dark_soul_instability = make_buff(this, "dark_soul_instability", talents.dark_soul_instability)
      ->add_invalidate(CACHE_SPELL_CRIT_CHANCE)
      ->add_invalidate(CACHE_CRIT_CHANCE)
      ->set_default_value(talents.dark_soul_instability->effectN(1).percent());

    // Azerite
    buffs.accelerant = make_buff<stat_buff_t>(this, "accelerant", azerite.accelerant)
      ->add_stat(STAT_HASTE_RATING, azerite.accelerant.value())
      ->set_duration(find_spell(272957)->duration());
    buffs.bursting_flare = make_buff<stat_buff_t>(this, "bursting_flare", find_spell(279913))
      ->add_stat(STAT_MASTERY_RATING, azerite.bursting_flare.value());
    buffs.chaotic_inferno = make_buff(this, "chaotic_inferno", find_spell(279673))
      ->set_default_value(find_spell(279673)->effectN(1).percent())
      ->set_chance(find_spell(279672)->proc_chance());
    buffs.crashing_chaos = make_buff(this, "crashing_chaos", azerite.crashing_chaos)
      ->set_max_stack(azerite.crashing_chaos.spell_ref().effectN(2).base_value())
      ->set_default_value(azerite.crashing_chaos.value());
    buffs.rolling_havoc = make_buff<stat_buff_t>(this, "rolling_havoc", find_spell(278931))
      ->add_stat(STAT_INTELLECT, azerite.rolling_havoc.value());
    buffs.flashpoint = make_buff<stat_buff_t>(this, "flashpoint", find_spell(275429))
      ->add_stat(STAT_HASTE_RATING, azerite.flashpoint.value());
  }

  void warlock_t::init_spells_destruction() {
    using namespace actions_destruction;

    spec.destruction                    = find_specialization_spell(137046);
    mastery_spells.chaotic_energies     = find_mastery_spell(WARLOCK_DESTRUCTION);

    spec.conflagrate                    = find_specialization_spell("Conflagrate");
    spec.conflagrate_2                  = find_specialization_spell(231793);
    spec.havoc                          = find_specialization_spell("Havoc");
    // Talents
    talents.flashover                   = find_talent_spell("Flashover");
    talents.eradication                 = find_talent_spell("Eradication");
    talents.soul_fire                   = find_talent_spell("Soul Fire");

    talents.reverse_entropy             = find_talent_spell("Reverse Entropy");
    talents.internal_combustion         = find_talent_spell("Internal Combustion");
    talents.shadowburn                  = find_talent_spell("Shadowburn");

    talents.inferno                     = find_talent_spell("Inferno");
    talents.fire_and_brimstone          = find_talent_spell("Fire and Brimstone");
    talents.cataclysm                   = find_talent_spell("Cataclysm");
        
    talents.roaring_blaze               = find_talent_spell("Roaring Blaze");
    talents.grimoire_of_supremacy       = find_talent_spell("Grimoire of Supremacy");

    talents.channel_demonfire           = find_talent_spell("Channel Demonfire");
    talents.dark_soul_instability       = find_talent_spell("Dark Soul: Instability");

    // Azerite
    azerite.accelerant                  = find_azerite_spell("Accelerant");
    azerite.bursting_flare              = find_azerite_spell("Bursting Flare");
    azerite.chaotic_inferno             = find_azerite_spell("Chaotic Inferno");
    azerite.crashing_chaos              = find_azerite_spell("Crashing Chaos");
    azerite.rolling_havoc               = find_azerite_spell("Rolling Havoc");
    azerite.flashpoint                  = find_azerite_spell("Flashpoint");
  }

  void warlock_t::init_gains_destruction() {
    gains.conflagrate                   = get_gain("conflagrate");
    gains.shadowburn                    = get_gain("shadowburn");
    gains.immolate                      = get_gain("immolate");
    gains.immolate_crits                = get_gain("immolate_crits");
    gains.reverse_entropy               = get_gain("reverse_entropy");
    gains.incinerate                    = get_gain("incinerate");
    gains.incinerate_crits              = get_gain("incinerate_crits");
    gains.fnb_bits                      = get_gain("fnb_bits");
    gains.soul_fire                     = get_gain("soul_fire");
    gains.infernal                      = get_gain("infernal");
    gains.shadowburn_shard              = get_gain("shadowburn_shard");
    gains.inferno                       = get_gain("inferno");
    gains.destruction_t20_2pc           = get_gain("destruction_t20_2pc");
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
    action_priority_list_t* aoe = get_action_priority_list("aoe");

    def->add_action("run_action_list,name=aoe,if=spell_targets.infernal_awakening>=3");
    def->add_action("immolate,cycle_targets=1,if=refreshable");
    def->add_action("havoc,cycle_targets=1,if=!(target=sim.target)");
    def->add_action("summon_infernal");
    def->add_talent(this, "Dark Soul: Instability");
    def->add_talent(this, "Soul Fire", "cycle_targets=1,if=!debuff.havoc.remains");
    def->add_talent(this, "Channel Demonfire", "cycle_targets=1,if=target=sim.target");
    def->add_talent(this, "Cataclysm", "cycle_targets=1,if=target=sim.target");
    def->add_action("chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&!talent.internal_combustion.enabled&soul_shard>=4|(talent.eradication.enabled&debuff.eradication.remains<=cast_time)|buff.dark_soul_instability.remains>cast_time|pet.infernal.active&talent.grimoire_of_supremacy.enabled");
    def->add_action("chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&talent.internal_combustion.enabled&dot.immolate.remains>8|soul_shard=5");
    def->add_action("conflagrate,cycle_targets=1,if=target=sim.target&(talent.flashover.enabled&buff.backdraft.stack<=2)|(!talent.flashover.enabled&buff.backdraft.stack<2)");
    def->add_talent(this, "Sahdowburn", ",cycle_targets=1,if=target=sim.target&charges=2|!buff.backdraft.remains|buff.backdraft.remains>buff.backdraft.stack*action.incinerate.execute_time");
    def->add_action("incinerate,cycle_targets=1,if=target=sim.target");

    aoe->add_action("summon_infernal");
    aoe->add_talent(this, "Dark Soul: Instability");
    aoe->add_talent(this, "Cataclysm");
    aoe->add_action("rain_of_fire,if=soul_shard>=4.5");
    aoe->add_action("immolate,if=!remains");
    aoe->add_talent(this, "Channel Demonfire");
    aoe->add_action("immolate,cycle_targets=1,if=refreshable&(!talent.fire_and_brimstone.enabled|talent.cataclysm.enabled&cooldown.cataclysm.remains>=12|talent.inferno.enabled)");
    aoe->add_action("rain_of_fire");
    aoe->add_action("conflagrate,if=(talent.flashover.enabled&buff.backdraft.stack<=2)|(!talent.flashover.enabled&buff.backdraft.stack<2)");
    aoe->add_talent(this, "Shadowburn", "if=!talent.fire_and_brimstone.enabled&(charges=2|!buff.backdraft.remains|buff.backdraft.remains>buff.backdraft.stack*action.incinerate.execute_time)");
    aoe->add_action("incinerate");
  }

  using namespace unique_gear;
  using namespace actions;

  void warlock_t::legendaries_destruction() {

  }
}
