#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock {
  namespace pets {
    warlock_pet_t::warlock_pet_t(sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian) :
      pet_t(sim, owner, pet_name, pt, guardian),
      special_action(nullptr),
      special_action_two(nullptr),
      melee_attack(nullptr),
      summon_stats(nullptr),
      ascendance(nullptr),
      buffs(),
      active()
    {
      owner_coeff.ap_from_sp = 0.5;
      owner_coeff.sp_from_sp = 1.0;
      owner_coeff.health = 0.5;
    }

    struct shadow_bite_t : public warlock_pet_melee_attack_t
    {
      shadow_bite_t(warlock_pet_t* p) :
        warlock_pet_melee_attack_t(p, "Shadow Bite")
      {

      }
    };
    struct felhunter_pet_t : public warlock_pet_t
    {
      felhunter_pet_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_FELHUNTER, name != "felhunter")
      {
        action_list_str = "shadow_bite";
      }

      void init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        melee_attack = new warlock_pet_melee_t(this);
      }

      action_t* create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "shadow_bite") return new shadow_bite_t(this);
        return warlock_pet_t::create_action(name, options_str);
      }
    };

    struct firebolt_t : public warlock_pet_spell_t
    {
      firebolt_t(warlock_pet_t* p) :
        warlock_pet_spell_t("Firebolt", p, p -> find_spell(3110)) { }
    };
    struct imp_pet_t : public warlock_pet_t
    {
      imp_pet_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_IMP, name != "imp")
      {
        action_list_str = "firebolt";
      }

      action_t* create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "firebolt") return new firebolt_t(this);
        return warlock_pet_t::create_action(name, options_str);
      }
    };

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
    struct succubus_pet_t : public warlock_pet_t
    {
      succubus_pet_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_SUCCUBUS, name != "succubus")
      {
        main_hand_weapon.swing_time = timespan_t::from_seconds(3.0);
        action_list_str = "lash_of_pain";
      }

      void init_base_stats()
      {
        warlock_pet_t::init_base_stats();

        main_hand_weapon.swing_time = timespan_t::from_seconds(3.0);
        melee_attack = new warlock_pet_melee_t(this);
      }

      action_t* create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "lash_of_pain") return new lash_of_pain_t(this);

        return warlock_pet_t::create_action(name, options_str);
      }
    };

    struct torment_t :
      public warlock_pet_spell_t
    {
      torment_t(warlock_pet_t* p) :
        warlock_pet_spell_t(p, "Torment") { }
    };
    struct voidwalker_pet_t : warlock_pet_t
    {
      voidwalker_pet_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_VOIDWALKER, name != "voidwalker")
      {
        action_list_str = "torment";
      }

      void init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        melee_attack = new warlock_pet_melee_t(this);
      }

      action_t* create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "torment") return new torment_t(this);
        return warlock_pet_t::create_action(name, options_str);
      }
    };

    namespace wild_imp {
      struct fel_firebolt_t : public warlock_pet_spell_t
      {
        fel_firebolt_t(warlock_pet_t* p) : warlock_pet_spell_t("fel_firebolt", p, p -> find_spell(104318))
        {
        }

        bool ready() override
        {
          if (!p()->resource_available(p()->primary_resource(), 20) & !p()->o()->buffs.demonic_power->check())
            p()->demise();

          return spell_t::ready();
        }

        double cost() const override
        {
          double c = warlock_pet_spell_t::cost();

          if (p()->o()->buffs.demonic_power->check())
          {
            c *= 1.0 + p()->o()->buffs.demonic_power->data().effectN(4).percent();
          }

          return c;
        }
      };

      wild_imp_pet_t::wild_imp_pet_t(sim_t* sim, warlock_t* owner) : warlock_pet_t(sim, owner, "wild_imp", PET_WILD_IMP),
        firebolt(),
        isnotdoge(),
        power_siphon(false)
      {
      }

      void wild_imp_pet_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();

        action_list_str = "fel_firebolt";

        resources.base[RESOURCE_ENERGY] = 100;
        resources.base_regen_per_second[RESOURCE_ENERGY] = 0;
      }

      action_t* wild_imp_pet_t::create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "fel_firebolt")
        {
          assert(firebolt == nullptr); // TODO: Check if we really want a non-background action stored in a pet-level
                                       // action?
          firebolt = new fel_firebolt_t(this);
          return firebolt;
        }

        return warlock_pet_t::create_action(name, options_str);
      }

      void wild_imp_pet_t::arise()
      {
        warlock_pet_t::arise();
        power_siphon = false;
        o()->buffs.wild_imps->increment();
        if (isnotdoge)
        {
          firebolt->cooldown->start(timespan_t::from_millis(rng().range(0, 500)));
        }
      }

      void wild_imp_pet_t::demise() {
        warlock_pet_t::demise();

        o()->buffs.wild_imps->decrement();
        if (!power_siphon)
          o()->buffs.demonic_core->trigger(1, buff_t::DEFAULT_VALUE(), o()->spec.demonic_core->effectN(1).percent());
      }
    }

    namespace dreadstalker {
      struct dreadbite_t : public warlock_pet_melee_attack_t
      {
        double t21_4pc_increase;

        dreadbite_t(warlock_pet_t* p) :
          warlock_pet_melee_attack_t("Dreadbite", p, p -> find_spell(205196))
        {
          weapon = &(p->main_hand_weapon);
          if (p->o()->talents.dreadlash->ok())
          {
            aoe = -1;
            radius = 8;
          }
          t21_4pc_increase = p->o()->sets->set(WARLOCK_DEMONOLOGY, T21, B4)->effectN(1).percent();
        }

        bool ready() override
        {
          if (p()->dreadbite_executes <= 0)
            return false;

          return warlock_pet_melee_attack_t::ready();
        }

        double action_multiplier() const override
        {
          double m = warlock_pet_melee_attack_t::action_multiplier();

          m *= 1.2; //until I can figure out wtf is going on with this spell

          if (p()->o()->sets->has_set_bonus(WARLOCK_DEMONOLOGY, T21, B4) && p()->bites_executed == 1)
            m *= 1.0 + t21_4pc_increase;

          if (p()->o()->talents.dreadlash->ok())
          {
            m *= 1.0 + p()->o()->talents.dreadlash->effectN(1).percent();
          }

          return m;
        }

        void execute() override
        {
          warlock_pet_melee_attack_t::execute();

          p()->dreadbite_executes--;
        }

        void impact(action_state_t* s) override
        {
          warlock_pet_melee_attack_t::impact(s);

          p()->bites_executed++;
        }
      };

      dreadstalker_t::dreadstalker_t(sim_t* sim, warlock_t* owner) : warlock_pet_t(sim, owner, "dreadstalker", PET_DREADSTALKER)
      {
        action_list_str = "travel/dreadbite";
        regen_type = REGEN_DISABLED;
        owner_coeff.ap_from_sp = 0.33;
      }

      void dreadstalker_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        resources.base[RESOURCE_ENERGY] = 0;
        resources.base_regen_per_second[RESOURCE_ENERGY] = 0;
        melee_attack = new warlock_pet_melee_t(this);
      }

      void dreadstalker_t::arise()
      {
        warlock_pet_t::arise();

        o()->buffs.dreadstalkers->set_duration(o()->find_spell(193332)->duration());
        o()->buffs.dreadstalkers->trigger();

        dreadbite_executes = 1;
        bites_executed = 0;

        if (o()->sets->has_set_bonus(WARLOCK_DEMONOLOGY, T21, B4))
          t21_4pc_reset = false;
      }

      void dreadstalker_t::demise() {
        warlock_pet_t::demise();

        o()->buffs.dreadstalkers->decrement();
        o()->buffs.demonic_core->trigger(1, buff_t::DEFAULT_VALUE(), o()->spec.demonic_core->effectN(2).percent());
        if (o()->azerite.shadows_bite.ok())
          o()->buffs.shadows_bite->trigger();
      }

      action_t* dreadstalker_t::create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "dreadbite") return new dreadbite_t(this);

        return warlock_pet_t::create_action(name, options_str);
      }
    }
    namespace vilefiend
    {
      struct bile_spit_t : public warlock_pet_spell_t
      {
        bile_spit_t(warlock_pet_t* p) : warlock_pet_spell_t("bile_spit", p, p -> find_spell(267997))
        {
          tick_may_crit = true;
        }
      };
      struct headbutt_t : public warlock_pet_melee_attack_t {
        headbutt_t(warlock_pet_t* p) : warlock_pet_melee_attack_t(p, "Headbutt") {
          cooldown->duration = timespan_t::from_seconds(5);
        }
      };

      vilefiend_t::vilefiend_t(sim_t* sim, warlock_t* owner) : warlock_pet_t(sim, owner, "vilefiend", PET_VILEFIEND)
      {
        action_list_str += "travel/headbutt";
        owner_coeff.ap_from_sp = 0.46;
      }

      void vilefiend_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        melee_attack = new warlock_pet_melee_t(this);
      }

      action_t* vilefiend_t::create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "bile_spit") return new bile_spit_t(this);
        if (name == "headbutt") return new headbutt_t(this);

        return warlock_pet_t::create_action(name, options_str);
      }
    }
    namespace demonic_tyrant {
      struct demonfire_blast_t : public warlock_pet_spell_t
      {
        demonfire_blast_t(warlock_pet_t* p, const std::string& options_str) : warlock_pet_spell_t("demonfire_blast", p, p -> find_spell(265279))
        {
          parse_options(options_str);
        }

        double action_multiplier() const override
        {
          double m = warlock_pet_spell_t::action_multiplier();

          if (p()->buffs.demonic_consumption->check())
          {
            m *= 1.0 + p()->buffs.demonic_consumption->check_stack_value();
          }

          return m;
        }
      };

      struct demonfire_t : public warlock_pet_spell_t
      {
        demonfire_t(warlock_pet_t* p, const std::string& options_str) : warlock_pet_spell_t("demonfire", p, p -> find_spell(270481))
        {
          parse_options(options_str);
        }

        double action_multiplier() const override
        {
          double m = warlock_pet_spell_t::action_multiplier();

          if (p()->buffs.demonic_consumption->check())
          {
            m *= 1.0 + p()->buffs.demonic_consumption->check_stack_value();
          }

          return m;
        }
      };

      demonic_tyrant_t::demonic_tyrant_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_DEMONIC_TYRANT, name != "demonic_tyrant") {
        action_list_str += "/sequence,name=rotation:demonfire_blast:demonfire:demonfire:demonfire";
        action_list_str += "/restart_sequence,name=rotation";
      }

      void demonic_tyrant_t::init_base_stats() {
        warlock_pet_t::init_base_stats();
      }

      void demonic_tyrant_t::demise() {
        warlock_pet_t::demise();

        if (o()->azerite.supreme_commander.ok())
        {
          o()->buffs.demonic_core->trigger(1);
          o()->buffs.supreme_commander->trigger();
        }
      }

      action_t* demonic_tyrant_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "demonfire") return new demonfire_t(this, options_str);
        if (name == "demonfire_blast") return new demonfire_blast_t(this, options_str);

        return warlock_pet_t::create_action(name, options_str);
      }
    }
    namespace shivarra {
      struct multi_slash_damage_t : public warlock_pet_melee_attack_t
      {
        multi_slash_damage_t(warlock_pet_t* p, int slash_num) : warlock_pet_melee_attack_t("multi-slash-" + std::to_string(slash_num), p, p -> find_spell(272172))
        {
          attack_power_mod.direct = data().effectN(slash_num).ap_coeff();
        }
      };

      struct multi_slash_t : public warlock_pet_melee_attack_t
      {
        std::array<multi_slash_damage_t*, 4> slashs;

        multi_slash_t(warlock_pet_t* p) : warlock_pet_melee_attack_t("multi-slash", p, p -> find_spell(272172))
        {
          for (unsigned i = 0; i < slashs.size(); ++i)
          {
            // Slash number is the spelldata effects number, so increase by 1.
            slashs[i] = new multi_slash_damage_t(p, i + 1);
            add_child(slashs[i]);
          }
        }

        void execute() override
        {
          cooldown->start(timespan_t::from_millis(rng().range(7000, 9000)));

          for (auto& slash : slashs)
          {
            slash->execute();
          }
        }
      };

      shivarra_t::shivarra_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "shivarra"),
        multi_slash()
      {
        action_list_str = "travel/multi_slash";
        owner_coeff.ap_from_sp = 0.065;
      }

      void shivarra_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        off_hand_weapon = main_hand_weapon;
        melee_attack = new warlock_pet_melee_t(this, 2.0);
      }

      void shivarra_t::arise()
      {
        warlock_pet_t::arise();
        multi_slash->cooldown->start(timespan_t::from_millis(rng().range(3500, 5100)));
      }

      action_t* shivarra_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "multi_slash")
        {
          assert(multi_slash == nullptr);
          multi_slash = new multi_slash_t(this);
          return multi_slash;
        }

        return warlock_pet_t::create_action(name, options_str);
      }
    }
    namespace darkhound {
      struct fel_bite_t : public warlock_pet_melee_attack_t {
        fel_bite_t(warlock_pet_t* p) : warlock_pet_melee_attack_t(p, "Fel Bite") {
        }

        void execute() override {
          warlock_pet_melee_attack_t::execute();
          cooldown->start(timespan_t::from_millis(rng().range(4500, 6500)));
        }
      };

      darkhound_t::darkhound_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "darkhound"),
        fel_bite()
      {
        action_list_str = "travel/fel_bite";
        owner_coeff.ap_from_sp = 0.065;
      }

      void darkhound_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();

        melee_attack = new warlock_pet_melee_t(this, 2.0);
      }

      void darkhound_t::arise()
      {
        warlock_pet_t::arise();
        fel_bite->cooldown->start(timespan_t::from_millis(rng().range(3000, 5000)));
      }

      action_t* darkhound_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "fel_bite")
        {
          assert(fel_bite == nullptr);
          fel_bite = new fel_bite_t(this);
          return fel_bite;
        }

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace bilescourge {
      struct toxic_bile_t : public warlock_pet_spell_t
      {
        toxic_bile_t(warlock_pet_t* p) : warlock_pet_spell_t("toxic_bile", p, p -> find_spell(272167))
        {
        }
      };

      bilescourge_t::bilescourge_t(sim_t* sim, warlock_t* owner, const std::string& name) : warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "bilescourge")
      {
        action_list_str = "toxic_bile";
        owner_coeff.ap_from_sp = 0.065;
      }

      void bilescourge_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
      }

      action_t* bilescourge_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "toxic_bile") return new toxic_bile_t(this);

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace urzul {
      struct many_faced_bite_t : public warlock_pet_melee_attack_t
      {
        many_faced_bite_t(warlock_pet_t* p) : warlock_pet_melee_attack_t("many_faced_bite", p, p -> find_spell(272439))
        {
          attack_power_mod.direct = data().effectN(1).ap_coeff();
        }

        void execute() override {
          warlock_pet_melee_attack_t::execute();
          cooldown->start(timespan_t::from_millis(rng().range(4500, 6000)));
        }
      };

      urzul_t::urzul_t(sim_t* sim, warlock_t* owner, const std::string& name) : warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "urzul")
        , many_faced_bite()
      {
        action_list_str = "travel";
        action_list_str += "/many_faced_bite";
        owner_coeff.ap_from_sp = 0.065;
      }

      void urzul_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        melee_attack = new warlock_pet_melee_t(this, 2.0);
      }

      void urzul_t::arise()
      {
        warlock_pet_t::arise();
        many_faced_bite->cooldown->start(timespan_t::from_millis(rng().range(3500, 4500)));
      }

      action_t* urzul_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "many_faced_bite")
        {
          assert(many_faced_bite == nullptr);
          many_faced_bite = new many_faced_bite_t(this);
          return many_faced_bite;
        }

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace void_terror {
      struct double_breath_damage_t : public warlock_pet_spell_t
      {
        double_breath_damage_t(warlock_pet_t* p, int breath_num) : warlock_pet_spell_t("double_breath-" + std::to_string(breath_num), p, p -> find_spell(272156))
        {
          attack_power_mod.direct = data().effectN(breath_num).ap_coeff();
        }
      };

      struct double_breath_t : public warlock_pet_spell_t
      {
        double_breath_damage_t* breath_1;
        double_breath_damage_t* breath_2;

        double_breath_t(warlock_pet_t* p) : warlock_pet_spell_t("double_breath", p, p -> find_spell(272156))
        {
          breath_1 = new double_breath_damage_t(p, 1);
          breath_2 = new double_breath_damage_t(p, 2);
          add_child(breath_1);
          add_child(breath_2);
        }

        void execute() override
        {
          cooldown->start(timespan_t::from_millis(rng().range(6000, 9000)));
          breath_1->execute();
          breath_2->execute();
        }
      };

      void_terror_t::void_terror_t(sim_t* sim, warlock_t* owner, const std::string& name) : warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "void_terror")
        , double_breath()
      {
        action_list_str = "travel";
        action_list_str += "/double_breath";
        owner_coeff.ap_from_sp = 0.065;
      }

      void void_terror_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        melee_attack = new warlock_pet_melee_t(this, 2.0);
      }

      void void_terror_t::arise()
      {
        warlock_pet_t::arise();
        double_breath->cooldown->start(timespan_t::from_millis(rng().range(1800, 5000)));
      }

      action_t* void_terror_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "double_breath")
        {
          assert(double_breath == nullptr);
          double_breath = new double_breath_t(this);
          return double_breath;
        }

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace wrathguard {
      struct overhead_assault_t : public warlock_pet_melee_attack_t
      {
        overhead_assault_t(warlock_pet_t* p) : warlock_pet_melee_attack_t("overhead_assault", p, p -> find_spell(272432))
        {
          attack_power_mod.direct = data().effectN(1).ap_coeff();
        }

        void execute() override {
          warlock_pet_melee_attack_t::execute();
          cooldown->start(timespan_t::from_millis(rng().range(4500, 6500)));
        }
      };

      wrathguard_t::wrathguard_t(sim_t* sim, warlock_t* owner, const std::string& name) : warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "wrathguard")
        , overhead_assault()
      {
        action_list_str = "travel/overhead_assault";
        owner_coeff.ap_from_sp = 0.065;
      }

      void wrathguard_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        off_hand_weapon = main_hand_weapon;
        melee_attack = new warlock_pet_melee_t(this, 2.0);
      }

      void wrathguard_t::arise()
      {
        warlock_pet_t::arise();
        overhead_assault->cooldown->start(timespan_t::from_millis(rng().range(3000, 5000)));
      }

      action_t* wrathguard_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "overhead_assault")
        {
          assert(overhead_assault == nullptr);
          overhead_assault = new overhead_assault_t(this);
          return overhead_assault;
        }

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace vicious_hellhound {
      struct demon_fangs_t : public warlock_pet_melee_attack_t
      {
        demon_fangs_t(warlock_pet_t* p) : warlock_pet_melee_attack_t("demon_fangs", p, p -> find_spell(272013))
        {
          attack_power_mod.direct = data().effectN(1).ap_coeff();
        }

        void execute() override {
          warlock_pet_melee_attack_t::execute();
          cooldown->start(timespan_t::from_millis(rng().range(4500, 6000)));
        }
      };

      vicious_hellhound_t::vicious_hellhound_t(sim_t* sim, warlock_t* owner, const std::string& name) : warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "vicious_hellhound"), demon_fang()
      {
        action_list_str = "travel";
        action_list_str += "/demon_fangs";
        owner_coeff.ap_from_sp = 0.065;
      }

      void vicious_hellhound_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();

        main_hand_weapon.swing_time = timespan_t::from_seconds(1.0);
        melee_attack = new warlock_pet_melee_t(this, 1.0);
      }

      void vicious_hellhound_t::arise()
      {
        warlock_pet_t::arise();
        demon_fang->cooldown->start(timespan_t::from_millis(rng().range(3200, 5100)));
      }

      action_t* vicious_hellhound_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "demon_fangs")
        {
          assert(demon_fang == nullptr);
          demon_fang = new demon_fangs_t(this);
          return demon_fang;
        }

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace illidari_satyr {
      struct shadow_slash_t : public warlock_pet_melee_attack_t
      {
        shadow_slash_t(warlock_pet_t* p) : warlock_pet_melee_attack_t("shadow_slash", p, p -> find_spell(272012))
        {
          attack_power_mod.direct = data().effectN(1).ap_coeff();
        }

        void execute() override {
          warlock_pet_melee_attack_t::execute();
          cooldown->start(timespan_t::from_millis(rng().range(4500, 6100)));
        }
      };

      illidari_satyr_t::illidari_satyr_t(sim_t* sim, warlock_t* owner, const std::string& name) : warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "illidari_satyr")
        , shadow_slash()
      {
        action_list_str = "travel/shadow_slash";
        owner_coeff.ap_from_sp = 0.065;
      }

      void illidari_satyr_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        off_hand_weapon = main_hand_weapon;
        melee_attack = new warlock_pet_melee_t(this, 1.0);
      }

      void illidari_satyr_t::arise()
      {
        warlock_pet_t::arise();
        shadow_slash->cooldown->start(timespan_t::from_millis(rng().range(3500, 5000)));
      }

      action_t* illidari_satyr_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "shadow_slash")
        {
          assert(shadow_slash == nullptr);
          shadow_slash = new shadow_slash_t(this);
          return shadow_slash;
        }

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace eyes_of_guldan {
      struct eye_of_guldan_t : public warlock_pet_spell_t
      {
        eye_of_guldan_t(warlock_pet_t* p) : warlock_pet_spell_t("eye_of_guldan", p, p -> find_spell(272131))
        {
          hasted_ticks = false;
        }
      };

      eyes_of_guldan_t::eyes_of_guldan_t(sim_t* sim, warlock_t* owner, const std::string& name) : warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "eyes_of_guldan") {
        action_list_str = "eye_of_guldan";
        owner_coeff.ap_from_sp = 0.065;
      }

      void eyes_of_guldan_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
      }

      void eyes_of_guldan_t::arise()
      {
        warlock_pet_t::arise();
        o()->buffs.eyes_of_guldan->trigger();
      }

      void eyes_of_guldan_t::demise()
      {
        warlock_pet_t::demise();
        o()->buffs.eyes_of_guldan->decrement();
      }

      action_t* eyes_of_guldan_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "eye_of_guldan") return new eye_of_guldan_t(this);

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace prince_malchezaar {
      prince_malchezaar_t::prince_malchezaar_t(sim_t* sim, warlock_t* owner, const std::string& name) : warlock_pet_t(sim, owner, name, PET_WARLOCK_RANDOM, name != "prince_malchezaar") {
        owner_coeff.ap_from_sp = 0.616;
        action_list_str = "travel";
      }

      void prince_malchezaar_t::init_base_stats()
      {
        warlock_pet_t::init_base_stats();
        off_hand_weapon = main_hand_weapon;
        melee_attack = new warlock_pet_melee_t(this);
      }

      void prince_malchezaar_t::arise()
      {
        warlock_pet_t::arise();
        o()->buffs.prince_malchezaar->trigger();
      }

      void prince_malchezaar_t::demise()
      {
        warlock_pet_t::demise();
        o()->buffs.prince_malchezaar->decrement();
      }
    }

    namespace infernal {
      struct immolation_tick_t : public warlock_pet_spell_t
      {
        immolation_tick_t(warlock_pet_t* p, const spell_data_t& s) :
          warlock_pet_spell_t("immolation_tick", p, s.effectN(1).trigger())
        {
          aoe = -1;
          background = true;
          may_crit = true;
        }
      };

      struct immolation_t : public warlock_pet_spell_t
      {
        immolation_t(warlock_pet_t* p, const std::string& options_str) :
          warlock_pet_spell_t("immolation", p, p -> find_spell(19483))
        {
          parse_options(options_str);

          dynamic_tick_action = hasted_ticks = true;
          tick_action = new immolation_tick_t(p, data());
        }

        void init() override
        {
          warlock_pet_spell_t::init();

          // Explicitly snapshot haste, as the spell actually has no duration in spell data
          snapshot_flags |= STATE_HASTE;
          update_flags |= STATE_HASTE;
        }

        timespan_t composite_dot_duration(const action_state_t*) const override
        {
          return player->sim->expected_iteration_time * 2;
        }

        virtual void cancel() override
        {
          dot_t* dot = find_dot(target);
          if (dot && dot->is_ticking())
          {
            dot->cancel();
          }

          action_t::cancel();
        }
      };

      infernal_t::infernal_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_INFERNAL, name != "infernal") {
      }

      void infernal_t::init_base_stats() {
        warlock_pet_t::init_base_stats();
        action_list_str = "immolation,if=!ticking";
        melee_attack = new warlock_pet_melee_t(this);
      }

      void infernal_t::arise()
      {
        warlock_pet_t::arise();

        buffs.embers->trigger();
      }

      void infernal_t::demise() {
        warlock_pet_t::demise();

        buffs.embers->expire();
        o()->buffs.grimoire_of_supremacy->expire();
      }

      action_t* infernal_t::create_action(const std::string& name, const std::string& options_str) {
        if (name == "immolation") return new immolation_t(this, options_str);

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    namespace darkglare
    {
      struct dark_glare_t : public warlock_pet_spell_t
      {
        dark_glare_t(warlock_pet_t* p) : warlock_pet_spell_t("dark_glare", p, p -> find_spell(205231))
        {

        }

        double action_multiplier() const override
        {
          double m = warlock_pet_spell_t::action_multiplier();

          double dots = 0.0;

          for (const auto target : sim->target_non_sleeping_list)
          {
            auto td = find_td(target);
            if (!td)
              continue;

            if (td->dots_agony->is_ticking())
              dots += 1.0;
            if (td->dots_corruption->is_ticking())
              dots += 1.0;
            if (td->dots_siphon_life->is_ticking())
              dots += 1.0;
            if (td->dots_phantom_singularity->is_ticking())
              dots += 1.0;
            if (td->dots_vile_taint->is_ticking())
              dots += 1.0;
            for (auto& current_ua : td->dots_unstable_affliction)
            {
              if (current_ua->is_ticking())
                dots += 1.0;
            }
          }

          m *= 1.0 + (dots * p()->o()->spec.summon_darkglare->effectN(3).percent());

          return m;
        }
      };

      darkglare_t::darkglare_t(sim_t* sim, warlock_t* owner, const std::string& name) :
        warlock_pet_t(sim, owner, name, PET_DARKGLARE, name != "darkglare")
      {
        action_list_str += "dark_glare";
      }

      double darkglare_t::composite_player_multiplier(school_e school) const
      {
        double m = warlock_pet_t::composite_player_multiplier(school);
        return m;
      }

      action_t* darkglare_t::create_action(const std::string& name, const std::string& options_str)
      {
        if (name == "dark_glare") return new dark_glare_t(this);

        return warlock_pet_t::create_action(name, options_str);
      }
    }

    void warlock_pet_t::create_buffs_pets() {
      buffs.demonic_consumption = make_buff(this, "demonic_consumption", find_spell(267972))
        ->set_default_value(find_spell(267972)->effectN(1).percent())
        ->set_max_stack(100);
      buffs.grimoire_of_service = make_buff(this, "grimoire_of_service", find_spell(216187));

      // destro
      buffs.embers = make_buff(this, "embers", find_spell(264364))
        ->set_period(timespan_t::from_seconds(0.5))
        ->set_tick_time_behavior(buff_tick_time_behavior::UNHASTED)
        ->set_tick_callback([this](buff_t*, int, const timespan_t&)
      {
        o()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, o()->gains.infernal);
      });
    }

    void warlock_pet_t::create_buffs_destruction() {
      buffs.embers = make_buff(this, "embers", find_spell(264364))
        ->set_period(timespan_t::from_seconds(0.5))
        ->set_tick_time_behavior(buff_tick_time_behavior::UNHASTED)
        ->set_tick_callback([this](buff_t*, int, const timespan_t&)
      {
        o()->resource_gain(RESOURCE_SOUL_SHARD, 0.1, o()->gains.infernal);
      });
    }

    void warlock_pet_t::init_spells_pets() {
      active.bile_spit = new vilefiend::bile_spit_t(this);
      init_spells_demonology();
    }
  }

  pet_t* warlock_t::create_main_pet(const std::string& pet_name, const std::string& pet_type)
  {
    pet_t* p = find_pet(pet_name);
    if (p) return p;
    using namespace pets;

    if (pet_name == "felhunter")          return new        felhunter_pet_t(sim, this, pet_name);
    if (pet_name == "imp")                return new        imp_pet_t(sim, this, pet_name);
    if (pet_name == "succubus")           return new        succubus_pet_t(sim, this, pet_name);
    if (pet_name == "voidwalker")         return new        voidwalker_pet_t(sim, this, pet_name);
    if (specialization() == WARLOCK_DEMONOLOGY)
    {
      return create_demo_pet(pet_name, pet_type);
    }

    return nullptr;
  }
}