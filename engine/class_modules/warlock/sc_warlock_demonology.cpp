#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock {
    namespace actions {
        struct shadow_bolt_t : public warlock_spell_t {
            cooldown_t* icd;

            shadow_bolt_t(warlock_t* p, const std::string& options_str) : warlock_spell_t(p, "Shadow Bolt") {
                parse_options(options_str);
                energize_type = ENERGIZE_ON_CAST;
                energize_resource = RESOURCE_SOUL_SHARD;
                energize_amount = 1;
            }

            void execute() override {
                warlock_spell_t::execute();
            }
        };

    } // end actions namespace

    namespace buffs {

    } // end buffs namespace

      // add actions
    action_t* warlock_t::create_action_demonology(const std::string& action_name, const std::string& options_str) {
        using namespace actions;

        if (action_name == "shadow_bolt") return new        shadow_bolt_t(this, options_str);

        return nullptr;
    }
    void warlock_t::create_buffs_demonology() {

    }
    void warlock_t::init_spells_demonology() {
        
    }
    void warlock_t::init_gains_demonology() {

    }
    void warlock_t::init_rng_demonology() {
    }
    void warlock_t::init_procs_demonology() {

    }
    void warlock_t::create_options_demonology() {
    }

    void warlock_t::create_apl_demonology() {
        action_priority_list_t* default = get_action_priority_list("default");

        default->add_action("shadow_bolt");
    }

    using namespace unique_gear;
    using namespace actions;

    void warlock_t::legendaries_demonology() {
        
    }
}