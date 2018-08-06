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
      bool havocd;
      bool affected_by_destruction_t20_4pc;
      bool affected_by_flamelicked;
      bool affected_by_odr_shawl_of_the_ymirjar;
      bool destro_mastery;
      bool can_feretory;

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
        havocd = false;
        affected_by_destruction_t20_4pc = false;
        destro_mastery = true;
        can_feretory = true;
      }

      bool use_havoc() const
      {
        if (!p()->havoc_target || target == p()->havoc_target || !can_havoc)
          return false;

        return true;
      }

      void reset() override
      {
        havocd = false;
        warlock_spell_t::reset();
      }

      void init() override
      {
        warlock_spell_t::init();

        affected_by_flamelicked = false;
        havocd = false;

        affected_by_odr_shawl_of_the_ymirjar = data().affected_by(p()->find_spell(212173)->effectN(1));

        if (data().affected_by(p()->spec.destruction->effectN(1)))
          base_dd_multiplier *= 1.0 + p()->spec.destruction->effectN(1).percent();

        if (data().affected_by(p()->spec.destruction->effectN(2)))
          base_td_multiplier *= 1.0 + p()->spec.destruction->effectN(2).percent();
      }

      double cost() const override
      {
        double c = warlock_spell_t::cost();
        return c;
      }

      void execute() override
      {
        warlock_spell_t::execute();
        if (use_havoc() && execute_state->target == this->target && !havocd)
        {
          this->set_target(p()->havoc_target);
          this->havocd = true;
          spell_t::execute();
          if (p()->azerite.rolling_havoc.ok())
            p()->buffs.rolling_havoc->trigger();
          this->havocd = false;
        }

        if (can_feretory && p()->legendary.feretory_of_souls && rng().roll(p()->find_spell(205702)->proc_chance()) && dbc::is_school(school, SCHOOL_FIRE))
          p()->resource_gain(RESOURCE_SOUL_SHARD, 1.0, p()->gains.feretory_of_souls);
      }

      void consume_resource() override
      {
        warlock_spell_t::consume_resource();

        if (resource_current == RESOURCE_SOUL_SHARD && p()->in_combat)
        {
          if (p()->legendary.the_master_harvester)
          {
            double sh_proc_chance = p()->find_spell(p()->legendary.the_master_harvester->spell_id)->effectN(3).percent();

            for (int i = 0; i < last_resource_cost; i++)
            {
              if (p()->rng().roll(sh_proc_chance))
              {
                p()->buffs.soul_harvest->trigger();
              }
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

      virtual void update_ready(timespan_t cd_duration) override
      {
        if (havocd)
          return;

        warlock_spell_t::update_ready(cd_duration);
      }

      double composite_target_multiplier(player_t* t) const override
      {
        double m = warlock_spell_t::composite_target_multiplier(t);

        auto td = this->td(t);
        if (td->debuffs_eradication->check())
          m *= 1.0 + td->debuffs_eradication->data().effectN(1).percent();

        if (p()->legendary.odr_shawl_of_the_ymirjar && target == p()->havoc_target && affected_by_odr_shawl_of_the_ymirjar)
          m *= 1.0 + p()->find_spell(212173)->effectN(1).percent();

        return m;
      }

      double action_multiplier() const override
      {
        double pm = warlock_spell_t::action_multiplier();

        if (p()->mastery_spells.chaotic_energies->ok() && destro_mastery)
        {
          double destro_mastery_value = p()->cache.mastery_value() / 2.0;
          double chaotic_energies_rng;

          if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T20, B4) && affected_by_destruction_t20_4pc)
            chaotic_energies_rng = destro_mastery_value;
          else
            chaotic_energies_rng = rng().range(0, destro_mastery_value);

          pm *= 1.0 + chaotic_energies_rng + (destro_mastery_value);
        }

        if (havocd)
          pm *= p()->spec.havoc->effectN(1).percent();

        if (p()->buffs.grimoire_of_supremacy->check() && this->data().affected_by(p()->find_spell(266091)->effectN(1)))
        {
          pm *= 1.0 + p()->buffs.grimoire_of_supremacy->check_stack_value();
        }

        return pm;
      }
    };

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
    struct soul_fire_t : public destruction_spell_t
    {
      soul_fire_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("soul_fire", p, p -> talents.soul_fire)
      {
        parse_options(options_str);
        energize_type = ENERGIZE_ON_CAST;
        energize_resource = RESOURCE_SOUL_SHARD;
        energize_amount = (std::double_t(p->find_spell(281490)->effectN(1).base_value()) / 10);

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

        can_havoc = true;
      }

      virtual void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);

        if (result_is_hit(s->result))
        {
          td(s->target)->debuffs_shadowburn->trigger();
          p()->resource_gain(RESOURCE_SOUL_SHARD, (std::double_t(p()->find_spell(245731)->effectN(1).base_value()) / 10), p()->gains.shadowburn);
        }
      }
    };

    struct roaring_blaze_t : public destruction_spell_t {
      roaring_blaze_t(warlock_t* p) :
        destruction_spell_t("roaring_blaze", p, p -> find_spell(265931))
      {
        background = true;
        base_tick_time = timespan_t::from_seconds(2.0);
        dot_duration = data().duration();
        spell_power_mod.tick = data().effectN(1).sp_coeff();
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

    struct immolate_t : public destruction_spell_t
    {
      immolate_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("immolate", p, p -> find_spell(348))
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
        destruction_spell_t::tick(d);

        if (d->state->result == RESULT_CRIT && rng().roll(data().effectN(2).percent()))
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate_crits);

        p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate);

        if (d->state->result_amount > 0.0 && p()->azerite.flashpoint.ok() && target->health_percentage() > 80 )
          p()->buffs.flashpoint->trigger();
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

        energize_type = ENERGIZE_NONE;

        cooldown->charges += p->spec.conflagrate_2->effectN(1).base_value();

        cooldown->charges += p->sets->set(WARLOCK_DESTRUCTION, T19, B4)->effectN(1).base_value();
        cooldown->duration += p->sets->set(WARLOCK_DESTRUCTION, T19, B4)->effectN(2).time_value();

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

        p()->buffs.backdraft->trigger( 1 + ( p()->talents.flashover->ok() ? p()->talents.flashover->effectN(1).base_value() : 0 ) );

        if (result_is_hit(s->result))
        {
          if ( p()->talents.roaring_blaze->ok() )
            roaring_blaze->execute();

          p()->resource_gain(RESOURCE_SOUL_SHARD, (std::double_t(p()->find_spell(245330)->effectN(1).base_value()) / 10), p()->gains.conflagrate);
        }
      }

      void execute() override
      {
        destruction_spell_t::execute();

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
          energize_amount = std::double_t(p->talents.fire_and_brimstone->effectN(2).base_value()) / 10;
          gain = p->gains.fnb_bits;
        }
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

      void execute() override
      {
        destruction_spell_t::execute();

        if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T20, B2))
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.destruction_t20_2pc);
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
        destruction_spell_t::execute();

        p()->buffs.chaotic_inferno->decrement();

        if (execute_state->target == p()->havoc_target)
          havocd = true;

        if(!havocd)
          p()->buffs.backdraft->decrement();

        if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T20, B2))
          p()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, p()->gains.destruction_t20_2pc);

        if (!havocd && p()->talents.fire_and_brimstone->ok())
        {
          fnb_action->set_target(execute_state->target);
          fnb_action->execute();
        }
        havocd = false;
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

    struct duplicate_chaos_bolt_t : public destruction_spell_t
    {
      player_t* original_target;
      flames_of_argus_t* flames_of_argus;

      duplicate_chaos_bolt_t(warlock_t* p) :
        destruction_spell_t("chaos_bolt_magistrike", p, p -> find_spell(213229)),
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
        double da = destruction_spell_t::bonus_da(s);
        da += p()->azerite.chaotic_inferno.value(2);
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

      void impact(action_state_t* s) override
      {
        destruction_spell_t::impact(s);

        if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T21, B2))
          td(s->target)->debuffs_chaotic_flames->trigger();

        if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T21, B4))
        {
          residual_action::trigger(flames_of_argus, s->target, s->result_amount * p()->sets->set(WARLOCK_DESTRUCTION, T21, B4)->effectN(1).percent());
        }
      }
    };

    struct chaos_bolt_t : public destruction_spell_t
    {
      double backdraft_gcd;
      double backdraft_cast_time;
      double refund;
      duplicate_chaos_bolt_t* duplicate;
      double duplicate_chance;
      flames_of_argus_t* flames_of_argus;
      internal_combustion_t* internal_combustion;

      chaos_bolt_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t(p, "Chaos Bolt"),
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
        destruction_spell_t::schedule_execute(state);

        if (p()->buffs.embrace_chaos->check())
        {
          p()->procs.t19_2pc_chaos_bolts->occur();
        }
      }

      virtual timespan_t execute_time() const override
      {
        timespan_t h = warlock_spell_t::execute_time();

        if (p()->buffs.backdraft->check())
          h *= backdraft_cast_time;

        if (p()->buffs.embrace_chaos->check())
          h *= 1.0 + p()->buffs.embrace_chaos->data().effectN(1).percent();

        return h;
      }

      timespan_t gcd() const override
      {
        timespan_t t = warlock_spell_t::gcd();

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
        destruction_spell_t::impact(s);

        trigger_internal_combustion( s );
        if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T21, B2 ) )
          td( s->target )->debuffs_chaotic_flames->trigger();
        if (p()->talents.eradication->ok() && result_is_hit(s->result))
          td(s->target)->debuffs_eradication->trigger();
        if (!havocd && p()->legendary.magistrike_restraints && rng().roll(duplicate_chance))
        {
          duplicate->original_target = s->target;
          duplicate->set_target( s->target );
          duplicate->target_cache.is_valid = false;
          duplicate->target_list();
          duplicate->target_cache.is_valid = true;
          if (duplicate->target_cache.list.size() > 0)
          {
            size_t target_to_strike = rng().range(size_t(), duplicate->target_cache.list.size());
            duplicate->set_target( duplicate->target_cache.list[target_to_strike] );
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

        auto td = this->td(s->target);
        if (!td->dots_immolate->is_ticking())
          return;

        internal_combustion->execute();
      }

      void execute() override
      {
        destruction_spell_t::execute();

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

        can_feretory = false;

        spell_power_mod.direct = data().effectN(1).sp_coeff();

        aoe = -1;
        base_aoe_multiplier = data().effectN(2).sp_coeff() / data().effectN(1).sp_coeff();
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
        trigger_gcd = timespan_t::zero();
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
        infernal_duration = p->find_spell(111685)->duration() + timespan_t::from_millis(1);
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

        if ( p() ->legendary.sindorei_spite && p()->cooldowns.sindorei_spite_icd->up() )
        {
          p()->buffs.sindorei_spite->up();
          p()->buffs.sindorei_spite->trigger();
          p()->cooldowns.sindorei_spite_icd->start( timespan_t::from_seconds( 180.0 ) );
        }
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
          if (this->num_targets_hit >= 3 && p()->azerite.accelerant.ok())
            p()->buffs.accelerant->trigger();
        }
      };

      rain_of_fire_t(warlock_t* p, const std::string& options_str) :
        destruction_spell_t("rain_of_fire", p, p -> find_spell(5740))
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
        destruction_spell_t::execute();

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

    buffs.embrace_chaos = make_buff( this, "embrace_chaos", sets->set( WARLOCK_DESTRUCTION, T19, B2 )->effectN( 1 ).trigger() )
      ->set_chance( sets->set( WARLOCK_DESTRUCTION, T19, B2 )->proc_chance() );

    buffs.active_havoc = make_buff( this, "active_havoc" )
      ->set_tick_behavior( buff_tick_behavior::NONE )
      ->set_refresh_behavior( buff_refresh_behavior::DURATION )
      ->set_duration( timespan_t::from_seconds( 10 ) );

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
    action_priority_list_t* cds = get_action_priority_list( "cds" );
    action_priority_list_t* fnb = get_action_priority_list( "fnb" );
    action_priority_list_t* cata = get_action_priority_list( "cata" );
    action_priority_list_t* inf = get_action_priority_list( "inf" );

    def->add_action( "run_action_list,name=cata,if=spell_targets.infernal_awakening>=3&talent.cataclysm.enabled" );
    def->add_action( "run_action_list,name=fnb,if=spell_targets.infernal_awakening>=3&talent.fire_and_brimstone.enabled" );
    def->add_action( "run_action_list,name=inf,if=spell_targets.infernal_awakening>=3&talent.inferno.enabled" );
    def->add_action("immolate,cycle_targets=1,if=!debuff.havoc.remains&(refreshable|talent.internal_combustion.enabled&action.chaos_bolt.in_flight&remains-action.chaos_bolt.travel_time-5<duration*0.3)");
    def->add_action( "call_action_list,name=cds" );
    def->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10" );
    def->add_action( "havoc,if=active_enemies>1" );
    def->add_talent(this, "Channel Demonfire" );
    def->add_talent(this, "Cataclysm");
    def->add_talent( this, "Soul Fire", "cycle_targets=1,if=!debuff.havoc.remains" );
    def->add_action("chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&execute_time+travel_time<target.time_to_die&(talent.internal_combustion.enabled|!talent.internal_combustion.enabled&soul_shard>=4|(talent.eradication.enabled&debuff.eradication.remains<=cast_time)|buff.dark_soul_instability.remains>cast_time|pet.infernal.active&talent.grimoire_of_supremacy.enabled)");
    def->add_action("conflagrate,cycle_targets=1,if=!debuff.havoc.remains&((talent.flashover.enabled&buff.backdraft.stack<=2)|(!talent.flashover.enabled&buff.backdraft.stack<2))");
    def->add_talent(this, "Shadowburn", "cycle_targets=1,if=!debuff.havoc.remains&((charges=2|!buff.backdraft.remains|buff.backdraft.remains>buff.backdraft.stack*action.incinerate.execute_time))");
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
    fnb->add_action( "channel_demonfire" );
    fnb->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=4&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    fnb->add_action( "havoc,if=spell_targets.rain_of_fire<=4&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    fnb->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&talent.grimoire_of_supremacy.enabled&pet.infernal.remains>execute_time&active_enemies<=4&((108*spell_targets.rain_of_fire%3)<(240*(1+0.08*buff.grimoire_of_supremacy.stack)%2*(1+buff.active_havoc.remains>execute_time)))" );
    fnb->add_action( "immolate,cycle_targets=1,if=!debuff.havoc.remains&refreshable&spell_targets.incinerate<=8" );
    fnb->add_action( "rain_of_fire" );
    fnb->add_action( "soul_fire,cycle_targets=1,if=!debuff.havoc.remains&spell_targets.incinerate=3" );
    fnb->add_action( "conflagrate,cycle_targets=1,if=!debuff.havoc.remains&(talent.flashover.enabled&buff.backdraft.stack<=2|spell_targets.incinerate<=7|talent.roaring_blaze.enabled&spell_targets.incinerate<=9)" );
    fnb->add_action( "incinerate,cycle_targets=1,if=!debuff.havoc.remains" );

    cata->add_action( "call_action_list,name=cds" );
    cata->add_action( "rain_of_fire,if=soul_shard>=4.5" );
    cata->add_action( "cataclysm" );
    cata->add_action( "immolate,if=talent.channel_demonfire.enabled&!remains&cooldown.channel_demonfire.remains<=action.chaos_bolt.execute_time" );
    cata->add_action( "channel_demonfire" );
    cata->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=8&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    cata->add_action( "havoc,if=spell_targets.rain_of_fire<=8&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    cata->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&talent.grimoire_of_supremacy.enabled&pet.infernal.remains>execute_time&active_enemies<=8&((108*spell_targets.rain_of_fire%3)<(240*(1+0.08*buff.grimoire_of_supremacy.stack)%2*(1+buff.active_havoc.remains>execute_time)))" );
    cata->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=4" );
    cata->add_action( "havoc,if=spell_targets.rain_of_fire<=4" );
    cata->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&buff.active_havoc.remains>execute_time&spell_targets.rain_of_fire<=4" );
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
    inf->add_action( "channel_demonfire" );
    inf->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=4+talent.internal_combustion.enabled&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    inf->add_action( "havoc,if=spell_targets.rain_of_fire<=4+talent.internal_combustion.enabled&talent.grimoire_of_supremacy.enabled&pet.infernal.active&pet.infernal.remains<=10" );
    inf->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&talent.grimoire_of_supremacy.enabled&pet.infernal.remains>execute_time&spell_targets.rain_of_fire<=4+talent.internal_combustion.enabled&((108*spell_targets.rain_of_fire%(3-0.16*spell_targets.rain_of_fire))<(240*(1+0.08*buff.grimoire_of_supremacy.stack)%2*(1+buff.active_havoc.remains>execute_time)))" );
    inf->add_action( "havoc,cycle_targets=1,if=!(target=sim.target)&target.time_to_die>10&spell_targets.rain_of_fire<=3&(talent.eradication.enabled|talent.internal_combustion.enabled)" );
    inf->add_action( "havoc,if=spell_targets.rain_of_fire<=3&(talent.eradication.enabled|talent.internal_combustion.enabled)" );
    inf->add_action( "chaos_bolt,cycle_targets=1,if=!debuff.havoc.remains&buff.active_havoc.remains>execute_time&spell_targets.rain_of_fire<=3&(talent.eradication.enabled|talent.internal_combustion.enabled)" );
    inf->add_action( "immolate,cycle_targets=1,if=!debuff.havoc.remains&refreshable" );
    inf->add_action( "rain_of_fire" );
    inf->add_action( "soul_fire,cycle_targets=1,if=!debuff.havoc.remains" );
    inf->add_action( "conflagrate,cycle_targets=1,if=!debuff.havoc.remains" );
    inf->add_action( "shadowburn,cycle_targets=1,if=!debuff.havoc.remains&((charges=2|!buff.backdraft.remains|buff.backdraft.remains>buff.backdraft.stack*action.incinerate.execute_time))" );
    inf->add_action( "incinerate,cycle_targets=1,if=!debuff.havoc.remains" );


  }
}
