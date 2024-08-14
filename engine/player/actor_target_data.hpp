// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "actor_pair.hpp"
#include "util/generic.hpp"

struct buff_t;
struct player_t;

struct actor_target_data_t : public actor_pair_t, private noncopyable
{
  struct atd_debuff_t
  {
    // NOTE: add debuffs that have player-scope implications (such as damage taken modifiers) or debuffs that will be
    // exposed to the APL to adjust actions based on their presence.
    //
    // These will require a targetdata_initializer in order to be created for all targets.
    //
    // Debuffs that are entirely self-contained within the proc or use action should be handled within the
    // generic_proc_t or dbc_proc_callback_t via target_specific_debuff.

    // BFA
    buff_t* blood_of_the_enemy;
    buff_t* condensed_lifeforce;
    buff_t* focused_resolve;
    // Shadowlands
    buff_t* adversary;                   // venthyr/nadjia/dauntless duelist debuff
    buff_t* plagueys_preemptive_strike;  // necro/marileth
    buff_t* sinful_revelation;           // enchant
    buff_t* putrid_burst;                // darkmoon deck: putrescence
    buff_t* dream_delver;                // nightfae/dreamweaver/dream delver debuff
    buff_t* soulglow_spectrometer;       // kyrian/mikanikos soulglow spectrometer debuff
    buff_t* scouring_touch;              // Shard of Dyz
    buff_t* exsanguinated;               // Shard of Bek
    buff_t* kevins_wrath;                // Marileth Kevin's Oozeling Kevin's Wrath debuff
    buff_t* wild_hunt_strategem;         // night_fae/korayn/ wild hunt strategem debuff
    buff_t* remnants_despair;            // Soulwarped Seal of Menethil DK ring
    // Dragonflight
    // The War Within
    buff_t* unwavering_focus;            // potion of unwavering focus
    buff_t* radiant_focus;               // Darkmoon Deck: Radiance
  } debuff;

  struct atd_dot_t
  {
  } dot;

  actor_target_data_t( player_t* target, player_t* source );
};
