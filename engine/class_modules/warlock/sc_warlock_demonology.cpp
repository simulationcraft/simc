#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock {
    namespace actions {

    } // end actions namespace

    namespace buffs {

    } // end buffs namespace

      // add actions
    action_t* warlock_t::create_action_demonology(const std::string& action_name, const std::string& options_str) {
        using namespace actions;

        //if (action_name == "corruption") return new                     corruption_t(this, options_str);

        return nullptr;
    }
    void warlock_t::create_buffs_demonology() {

    }
    void warlock_t::init_spells_demonology() {
        
    }
    void warlock_t::init_rng_demonology() {
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