#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock {
    namespace actions {

    } // end actions namespace

    namespace buffs {

    } // end buffs namespace

      // add actions
    action_t* warlock_t::create_action_destruction(const std::string& action_name, const std::string& options_str) {
        using namespace actions;

        //if (action_name == "corruption") return new                     corruption_t(this, options_str);

        return nullptr;
    }
    void warlock_t::create_buffs_destruction() {

    }
    void warlock_t::init_spells_destruction() {

    }
    void warlock_t::init_rng_destruction() {
    }
    void warlock_t::create_options_destruction() {
    }

    void warlock_t::create_apl_destruction() {
        action_priority_list_t* default = get_action_priority_list("default");

        default->add_action("incinerate");
    }

    using namespace unique_gear;
    using namespace actions;

    void warlock_t::legendaries_destruction() {

    }
}