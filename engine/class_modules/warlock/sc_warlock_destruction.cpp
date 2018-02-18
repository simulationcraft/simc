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
        spec.destruction = find_specialization_spell(137046);
        mastery_spells.chaotic_energies = find_mastery_spell(WARLOCK_DESTRUCTION);
        // Talents
        talents.eradication = find_talent_spell("Eradication");
        talents.flashover = find_talent_spell("Flashover");
        talents.soul_fire = find_talent_spell("Soul Fire");
        talents.reverse_entropy = find_talent_spell("Reverse Entropy");
        talents.internal_combustion = find_talent_spell("Internal Combustion");
        talents.shadowburn = find_talent_spell("Shadowburn");
        talents.fire_and_brimstone = find_talent_spell("Fire and Brimstone");
        talents.cataclysm = find_talent_spell("Cataclysm");
        talents.hellfire = find_talent_spell("Hellfire");
        talents.roaring_blaze = find_talent_spell("Roaring Blaze");
        talents.grimoire_of_supremacy = find_talent_spell("Grimoire of Supremacy");
        talents.channel_demonfire = find_talent_spell("Channel Demonfire");
        talents.dark_soul = find_talent_spell("Dark Soul");
    }
    void warlock_t::init_gains_destruction() {

    }
    void warlock_t::init_rng_destruction() {
    }
    void warlock_t::init_procs_destruction() {

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