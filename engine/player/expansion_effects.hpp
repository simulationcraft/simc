// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

// Expansion specific methods and helpers
namespace expansion
{
// Legion (WoW 7.0)
namespace legion
{
} // namespace legion

namespace bfa
{
// All Leyshocks-related functionality defined in sc_unique_gear_x7.cpp

// Register a simple spell id -> stat buff type mapping
void register_leyshocks_trigger( unsigned spell_id, stat_e stat_buff );

// Trigger based on a spell id on a predetermined mappings on an actor
void trigger_leyshocks_grand_compilation( unsigned spell_id, player_t* actor );
// Bypass spell mapping to trigger any of the buffs required on an actor
void trigger_leyshocks_grand_compilation( stat_e stat, player_t* actor );
}
} // namespace expansion
