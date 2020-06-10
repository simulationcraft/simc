// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include "sc_enums.hpp"
#include "util/string_view.hpp"

#include <cassert>
#include <functional>
#include <vector>

struct action_t;
struct expr_t;
struct item_data_t;
struct player_t;
struct special_effect_t;
struct scoped_callback_t;
struct special_effect_db_item_t;
struct sim_t;

namespace unique_gear
{
using special_effect_set_t = std::vector<const special_effect_db_item_t*>;

void register_hotfixes();
void register_hotfixes_legion();
void register_hotfixes_bfa();

void register_special_effects();
void register_special_effects_legion();  // Legion special effects
void register_special_effects_bfa();     // Battle for Azeroth special effects

void sort_special_effects();
void unregister_special_effects();

void add_effect( const special_effect_db_item_t& );
special_effect_set_t find_special_effect_db_item( unsigned spell_id );

void register_target_data_initializers( sim_t* );
void register_target_data_initializers_legion( sim_t* );  // Legion targetdata initializers
void register_target_data_initializers_bfa( sim_t* );     // Battle for Azeroth targetdata initializers

void init( player_t* );

special_effect_t* find_special_effect( player_t* actor, unsigned spell_id, special_effect_e = SPECIAL_EFFECT_NONE );

// First-phase special effect initializers
void initialize_special_effect( special_effect_t& effect, unsigned spell_id );
void initialize_special_effect_fallbacks( player_t* actor );
// Second-phase special effect initializer
void initialize_special_effect_2( special_effect_t* effect );

// Initialize special effects related to various race spells
void initialize_racial_effects( player_t* );

std::unique_ptr<expr_t> create_expression( player_t& player, util::string_view name_str );

}  // namespace unique_gear
