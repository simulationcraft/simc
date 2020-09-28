// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "sc_actor_pair.hpp"
#include "util/generic.hpp"

struct buff_t;
struct player_t;

struct actor_target_data_t : public actor_pair_t, private noncopyable
{
  struct atd_debuff_t
  {
    buff_t* mark_of_doom;
    buff_t* poisoned_dreams;
    buff_t* fel_burn;
    buff_t* flame_wreath;
    buff_t* thunder_ritual;
    buff_t* brutal_haymaker;
    buff_t* taint_of_the_sea;
    buff_t* solar_collapse;
    buff_t* volatile_magic;
    buff_t* maddening_whispers;
    buff_t* shadow_blades;
    // BFA - Azerite
    buff_t* azerite_globules;
    buff_t* dead_ahead;
    buff_t* battlefield_debuff;
    // BFA - Trinkets
    buff_t* wasting_infection;
    buff_t* everchill;
    buff_t* choking_brine;
    buff_t* razor_coral;
    buff_t* conductive_ink;
    buff_t* luminous_algae;
    buff_t* psyche_shredder;
    // BFA - Essences
    buff_t* blood_of_the_enemy;
    buff_t* condensed_lifeforce;
    buff_t* focused_resolve;
    buff_t* reaping_flames_tracker;
    // Shadowlands
    buff_t* adversary;                   // venthyr/nadjia/dauntless duelist debuff
    buff_t* plagueys_preemptive_strike;  // necro/marileth
    buff_t* sinful_revelation;           // enchant
    buff_t* putrid_burst;                // darkmoon deck: putrescence
    buff_t* echo_of_eonar;               // echo of eonar runecarve
  } debuff;

  struct atd_dot_t
  {
  } dot;

  actor_target_data_t( player_t* target, player_t* source );
};