// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SIMULATIONCRAFT_H
#define SIMULATIONCRAFT_H


// Platform, compiler and general configuration
#include "config.hpp"

// libraries
#include "lib/fmt/format.h"
#include "lib/fmt/ostream.h"
#include "lib/fmt/printf.h"
#include "lib/gsl-lite/gsl-lite.hpp"


#include "util/timespan.hpp" // Time class representing ingame time
#include "sc_enums.hpp"

#include "action/sc_action_state.hpp"
#include "action/sc_action.hpp"
#include "action/attack.hpp"
#include "action/spell_base.hpp"
#include "action/spell.hpp"
#include "action/heal.hpp"
#include "action/absorb.hpp"
#include "action/sequence.hpp"
#include "action/dot.hpp"
#include "action/action_callback.hpp"
#include "action/dbc_proc_callback.hpp"
#include "action/snapshot_stats.hpp"
#include "action/residual_action.hpp"

#include "buff/sc_buff.hpp"

#include "class_modules/class_module.hpp"

#include "dbc/dbc.hpp"
#include "dbc/item_database.hpp"
#include "dbc/data_enums.hh"
#include "dbc/data_definitions.hh"

#include "item/special_effect.hpp"
#include "item/item.hpp"
#include "item/enchants.hpp"
#include "item/item_targetdata_initializer.hpp"

#include "player/azerite_data.hpp"
#include "player/consumable.hpp"
#include "player/covenant.hpp"
#include "player/sample_data_helper.hpp"
#include "player/gear_stats.hpp"
#include "player/sc_actor_pair.hpp"
#include "player/actor_target_data.hpp"
#include "player/rating.hpp"
#include "player/weapon.hpp"
#include "player/set_bonus.hpp"
#include "player/player_stat_cache.hpp"
#include "player/player_processed_report_information.hpp"
#include "player/player_collected_data.hpp"
#include "player/player_talent_points.hpp"
#include "player/assessor.hpp"
#include "player/action_variable.hpp"
#include "player/player_scaling.hpp"
#include "player/player_resources.hpp"
#include "player/sc_player.hpp"
#include "player/target_specific.hpp"
#include "player/player_event.hpp"
#include "player/pet.hpp"
#include "player/stats.hpp"
#include "player/action_priority_list.hpp"
#include "player/expansion_effects.hpp"
#include "player/unique_gear.hpp"
#include "player/unique_gear_helper.hpp"
#include "player/ground_aoe.hpp"
#include "player/spawner_base.hpp"
#include "player/instant_absorb.hpp"
#include "player/runeforge_data.hpp"

#include "report/reports.hpp"
#include "report/decorators.hpp"
#include "report/color.hpp"
#include "report/report_helper.hpp"
#include "report/sc_highchart.hpp"

#include "sim/sc_option.hpp"
#include "sim/raid_event.hpp"
#include "sim/sc_expressions.hpp"
#include "sim/sc_sim.hpp"
#include "sim/scale_factor_control.hpp"
#include "sim/event.hpp"
#include "sim/benefit.hpp"
#include "sim/uptime.hpp"
#include "sim/proc.hpp"
#include "sim/real_ppm.hpp"
#include "sim/shuffled_rng.hpp"
#include "sim/sc_cooldown.hpp"
#include "sim/gain.hpp"
#include "sim/cooldown_waste_data.hpp"

#include "util/generic.hpp" // Generic programming tools
#include "util/sample_data.hpp"
#include "util/timeline.hpp"
#include "util/rng.hpp"
#include "util/util.hpp"
#include "util/io.hpp"
#include "util/scoped_callback.hpp"

#include <sstream>

#endif // SIMULATIONCRAFT_H
