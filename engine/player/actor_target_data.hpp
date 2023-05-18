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
    buff_t* shattered_psyche;            // memory of past sins
    buff_t* dream_delver;                // nightfae/dreamweaver/dream delver debuff
    buff_t* carvers_eye_debuff;          // necrolord/bonesmith/carver's eye debuff for cd tracking
    buff_t* soulglow_spectrometer;       // kyrian/mikanikos soulglow spectrometer debuff
    buff_t* scouring_touch;              // Shard of Dyz
    buff_t* exsanguinated;               // Shard of Bek
    buff_t* kevins_wrath;                // Marileth Kevin's Oozeling Kevin's Wrath debuff
    buff_t* frozen_heart;                // Relic of the Frozen Wastes debuff
    buff_t* volatile_satchel;            // Ticking Sack of Terror debuff
    buff_t* wild_hunt_strategem;         // night_fae/korayn/ wild hunt strategem debuff
    buff_t* remnants_despair;            // Soulwarped Seal of Menethil DK ring
    buff_t* scent_of_souls;              // Bells of the Endless Feast debuff
    buff_t* chains_of_domination;        // Chains of Domination debuff
    // Dragonflight
    buff_t* dragonfire_bomb; // Dragonfire Bomb Dispenser
    buff_t* awakening_rime;  // darkmoon deck: rime
    buff_t* grudge;          // spiteful storm
    buff_t* skewering_cold;  // globe of jagged ice
    buff_t* heavens_nemesis; // Neltharax, Enemy of the Sky
    buff_t* crystalline_web; // Iceblood Deathsnare
    buff_t* ever_decaying_spores; // Ever Decaying Spores Embellishment
  } debuff;

  struct atd_dot_t
  {
  } dot;

  actor_target_data_t( player_t* target, player_t* source );
};
