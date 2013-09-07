// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SPECIALIZATION_HPP
#define SPECIALIZATION_HPP

enum specialization_e {
  SPEC_NONE            = 0,
  SPEC_PET             = 1,
  PET_FEROCITY         = 74,
  PET_TENACITY         = 81,
  PET_CUNNING          = 79,
  WARRIOR_ARMS         = 71,
  WARRIOR_FURY         = 72,
  WARRIOR_PROTECTION   = 73,
  PALADIN_HOLY         = 65,
  PALADIN_PROTECTION   = 66,
  PALADIN_RETRIBUTION  = 70,
  HUNTER_BEAST_MASTERY = 253,
  HUNTER_MARKSMANSHIP  = 254,
  HUNTER_SURVIVAL      = 255,
  ROGUE_ASSASSINATION  = 259,
  ROGUE_COMBAT         = 260,
  ROGUE_SUBTLETY       = 261,
  PRIEST_DISCIPLINE    = 256,
  PRIEST_HOLY          = 257,
  PRIEST_SHADOW        = 258,
  DEATH_KNIGHT_BLOOD   = 250,
  DEATH_KNIGHT_FROST   = 251,
  DEATH_KNIGHT_UNHOLY  = 252,
  SHAMAN_ELEMENTAL     = 262,
  SHAMAN_ENHANCEMENT   = 263,
  SHAMAN_RESTORATION   = 264,
  MAGE_ARCANE          = 62,
  MAGE_FIRE            = 63,
  MAGE_FROST           = 64,
  WARLOCK_AFFLICTION   = 265,
  WARLOCK_DEMONOLOGY   = 266,
  WARLOCK_DESTRUCTION  = 267,
  MONK_BREWMASTER      = 268,
  MONK_MISTWEAVER      = 270,
  MONK_WINDWALKER      = 269,
  DRUID_BALANCE        = 102,
  DRUID_FERAL          = 103,
  DRUID_GUARDIAN       = 104,
  DRUID_RESTORATION    = 105,
};

namespace specdata {
static const unsigned n_specs = 37;
static const specialization_e __specs[37] = {
  PET_FEROCITY, 
  PET_TENACITY, 
  PET_CUNNING, 
  WARRIOR_ARMS, 
  WARRIOR_FURY, 
  WARRIOR_PROTECTION, 
  PALADIN_HOLY, 
  PALADIN_PROTECTION, 
  PALADIN_RETRIBUTION, 
  HUNTER_BEAST_MASTERY, 
  HUNTER_MARKSMANSHIP, 
  HUNTER_SURVIVAL, 
  ROGUE_ASSASSINATION, 
  ROGUE_COMBAT, 
  ROGUE_SUBTLETY, 
  PRIEST_DISCIPLINE, 
  PRIEST_HOLY, 
  PRIEST_SHADOW, 
  DEATH_KNIGHT_BLOOD, 
  DEATH_KNIGHT_FROST, 
  DEATH_KNIGHT_UNHOLY, 
  SHAMAN_ELEMENTAL, 
  SHAMAN_ENHANCEMENT, 
  SHAMAN_RESTORATION, 
  MAGE_ARCANE, 
  MAGE_FIRE, 
  MAGE_FROST, 
  WARLOCK_AFFLICTION, 
  WARLOCK_DEMONOLOGY, 
  WARLOCK_DESTRUCTION, 
  MONK_BREWMASTER, 
  MONK_MISTWEAVER, 
  MONK_WINDWALKER, 
  DRUID_BALANCE, 
  DRUID_FERAL, 
  DRUID_GUARDIAN, 
  DRUID_RESTORATION
};

inline unsigned spec_count()
{ return n_specs; }

inline specialization_e spec_id( unsigned idx )
{ assert( idx < n_specs ); return __specs[ idx ]; }

}

#endif

