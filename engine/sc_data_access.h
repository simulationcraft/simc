#ifndef SC_DATA_ACCESS_H
#define SC_DATA_ACCESS_H

#include "sc_data.h"

enum spell_attribute_t {
 SPELL_ATTR_UNK0 = 0x0000, // 0
 SPELL_ATTR_RANGED, // 1 All ranged abilites have this flag
 SPELL_ATTR_ON_NEXT_SWING_1, // 2 on next swing
 SPELL_ATTR_UNK3, // 3 not set in 3.0.3
 SPELL_ATTR_UNK4, // 4 isAbility
 SPELL_ATTR_TRADESPELL, // 5 trade spells, will be added by client to a sublist of profession spell
 SPELL_ATTR_PASSIVE, // 6 Passive spell
 SPELL_ATTR_UNK7, // 7 can't be linked in chat?
 SPELL_ATTR_UNK8, // 8 hide created item in tooltip (for effect=24)
 SPELL_ATTR_UNK9, // 9
 SPELL_ATTR_ON_NEXT_SWING_2, // 10 on next swing 2
 SPELL_ATTR_UNK11, // 11
 SPELL_ATTR_DAYTIME_ONLY, // 12 only useable at daytime, not set in 2.4.2
 SPELL_ATTR_NIGHT_ONLY, // 13 only useable at night, not set in 2.4.2
 SPELL_ATTR_INDOORS_ONLY, // 14 only useable indoors, not set in 2.4.2
 SPELL_ATTR_OUTDOORS_ONLY, // 15 Only useable outdoors.
 SPELL_ATTR_NOT_SHAPESHIFT, // 16 Not while shapeshifted
 SPELL_ATTR_ONLY_STEALTHED, // 17 Must be in stealth
 SPELL_ATTR_UNK18, // 18
 SPELL_ATTR_LEVEL_DAMAGE_CALCULATION, // 19 spelldamage depends on caster level
 SPELL_ATTR_STOP_ATTACK_TARGE, // 20 Stop attack after use this spell (and not begin attack if use)
 SPELL_ATTR_IMPOSSIBLE_DODGE_PARRY_BLOCK, // 21 Cannot be dodged/parried/blocked
 SPELL_ATTR_UNK22, // 22
 SPELL_ATTR_UNK23, // 23 castable while dead?
 SPELL_ATTR_CASTABLE_WHILE_MOUNTED, // 24 castable while mounted
 SPELL_ATTR_DISABLED_WHILE_ACTIVE, // 25 Activate and start cooldown after aura fade or remove summoned creature or go
 SPELL_ATTR_UNK26, // 26
 SPELL_ATTR_CASTABLE_WHILE_SITTING, // 27 castable while sitting
 SPELL_ATTR_CANT_USED_IN_COMBAT, // 28 Cannot be used in combat
 SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY, // 29 unaffected by invulnerability (hmm possible not...)
 SPELL_ATTR_UNK30, // 30 breakable by damage?
 SPELL_ATTR_CANT_CANCEL, // 31 positive aura can't be canceled

 SPELL_ATTR_EX_UNK0 = 0x0100, // 0
 SPELL_ATTR_EX_DRAIN_ALL_POWER, // 1 use all power (Only paladin Lay of Hands and Bunyanize)
 SPELL_ATTR_EX_CHANNELED_1, // 2 channeled 1
 SPELL_ATTR_EX_UNK3, // 3
 SPELL_ATTR_EX_UNK4, // 4
 SPELL_ATTR_EX_NOT_BREAK_STEALTH, // 5 Not break stealth
 SPELL_ATTR_EX_CHANNELED_2, // 6 channeled 2
 SPELL_ATTR_EX_NEGATIVE, // 7
 SPELL_ATTR_EX_NOT_IN_COMBAT_TARGET, // 8 Spell req target not to be in combat state
 SPELL_ATTR_EX_UNK9, // 9
 SPELL_ATTR_EX_NO_INITIAL_AGGRO, // 10 no generates threat on cast 100%
 SPELL_ATTR_EX_UNK11, // 11
 SPELL_ATTR_EX_UNK12, // 12
 SPELL_ATTR_EX_UNK13, // 13
 SPELL_ATTR_EX_UNK14, // 14
 SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY, // 15 remove auras on immunity
 SPELL_ATTR_EX_UNAFFECTED_BY_SCHOOL_IMMUNE, // 16 unaffected by school immunity
 SPELL_ATTR_EX_UNK17, // 17 for auras SPELL_AURA_TRACK_CREATURES, SPELL_AURA_TRACK_RESOURCES and SPELL_AURA_TRACK_STEALTHED select non-stacking tracking spells
 SPELL_ATTR_EX_UNK18, // 18
 SPELL_ATTR_EX_UNK19, // 19
 SPELL_ATTR_EX_REQ_COMBO_POINTS1, // 20 Req combo points on target
 SPELL_ATTR_EX_UNK21, // 21
 SPELL_ATTR_EX_REQ_COMBO_POINTS2, // 22 Req combo points on target
 SPELL_ATTR_EX_UNK23, // 23
 SPELL_ATTR_EX_UNK24, // 24 Req fishing pole??
 SPELL_ATTR_EX_UNK25, // 25
 SPELL_ATTR_EX_UNK26, // 26
 SPELL_ATTR_EX_UNK27, // 27
 SPELL_ATTR_EX_UNK28, // 28
 SPELL_ATTR_EX_UNK29, // 29
 SPELL_ATTR_EX_UNK30, // 30 overpower
 SPELL_ATTR_EX_UNK31, // 31

 SPELL_ATTR_EX2_UNK0 = 0x0200, // 0
 SPELL_ATTR_EX2_UNK1, // 1
 SPELL_ATTR_EX2_CANT_REFLECTED, // 2 ? used for detect can or not spell reflected // do not need LOS (e.g. 18220 since 3.3.3)
 SPELL_ATTR_EX2_UNK3, // 3 auto targeting? (e.g. fishing skill enhancement items since 3.3.3)
 SPELL_ATTR_EX2_UNK4, // 4
 SPELL_ATTR_EX2_AUTOREPEAT_FLAG, // 5
 SPELL_ATTR_EX2_UNK6, // 6 only usable on tabbed by yourself
 SPELL_ATTR_EX2_UNK7, // 7
 SPELL_ATTR_EX2_UNK8, // 8 not set in 3.0.3
 SPELL_ATTR_EX2_UNK9, // 9
 SPELL_ATTR_EX2_UNK10, // 10
 SPELL_ATTR_EX2_HEALTH_FUNNEL, // 11
 SPELL_ATTR_EX2_UNK12, // 12
 SPELL_ATTR_EX2_UNK13, // 13
 SPELL_ATTR_EX2_UNK14, // 14
 SPELL_ATTR_EX2_UNK15, // 15 not set in 3.0.3
 SPELL_ATTR_EX2_UNK16, // 16
 SPELL_ATTR_EX2_UNK17, // 17 suspend weapon timer instead of resetting it, (?Hunters Shot and Stings only have this flag?)
 SPELL_ATTR_EX2_UNK18, // 18 Only Revive pet - possible req dead pet
 SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT, // 19 does not necessarly need shapeshift
 SPELL_ATTR_EX2_UNK20, // 20
 SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD, // 21 for ice blocks, pala immunity buffs, priest absorb shields, but used also for other spells -> not sure!
 SPELL_ATTR_EX2_UNK22, // 22
 SPELL_ATTR_EX2_UNK23, // 23 Only mage Arcane Concentration have this flag
 SPELL_ATTR_EX2_UNK24, // 24
 SPELL_ATTR_EX2_UNK25, // 25
 SPELL_ATTR_EX2_UNK26, // 26 unaffected by school immunity
 SPELL_ATTR_EX2_UNK27, // 27
 SPELL_ATTR_EX2_UNK28, // 28 no breaks stealth if it fails??
 SPELL_ATTR_EX2_CANT_CRIT, // 29 Spell can't crit
 SPELL_ATTR_EX2_UNK30, // 30
 SPELL_ATTR_EX2_FOOD_BUFF, // 31 Food or Drink Buff (like Well Fed)

 SPELL_ATTR_EX3_UNK0 = 0x0300, // 0
 SPELL_ATTR_EX3_UNK1, // 1
 SPELL_ATTR_EX3_UNK2, // 2
 SPELL_ATTR_EX3_UNK3, // 3
 SPELL_ATTR_EX3_UNK4, // 4 Druid Rebirth only this spell have this flag
 SPELL_ATTR_EX3_UNK5, // 5
 SPELL_ATTR_EX3_UNK6, // 6
 SPELL_ATTR_EX3_UNK7, // 7 create a separate (de)buff stack for each caster
 SPELL_ATTR_EX3_UNK8, // 8
 SPELL_ATTR_EX3_UNK9, // 9
 SPELL_ATTR_EX3_MAIN_HAND, // 10 Main hand weapon required
 SPELL_ATTR_EX3_BATTLEGROUND, // 11 Can casted only on battleground
 SPELL_ATTR_EX3_CAST_ON_DEAD, // 12 target is a dead player (not every spell has this flag)
 SPELL_ATTR_EX3_UNK13, // 13
 SPELL_ATTR_EX3_UNK14, // 14 "Honorless Target" only this spells have this flag
 SPELL_ATTR_EX3_UNK15, // 15 Auto Shoot, Shoot, Throw, - this is autoshot flag
 SPELL_ATTR_EX3_UNK16, // 16 no triggers effects that trigger on casting a spell??
 SPELL_ATTR_EX3_UNK17, // 17 no triggers effects that trigger on casting a spell??
 SPELL_ATTR_EX3_UNK18, // 18
 SPELL_ATTR_EX3_UNK19, // 19
 SPELL_ATTR_EX3_DEATH_PERSISTENT, // 20 Death persistent spells
 SPELL_ATTR_EX3_UNK21, // 21
 SPELL_ATTR_EX3_REQ_WAND, // 22 Req wand
 SPELL_ATTR_EX3_UNK23, // 23
 SPELL_ATTR_EX3_REQ_OFFHAND, // 24 Req offhand weapon
 SPELL_ATTR_EX3_UNK25, // 25 no cause spell pushback ?
 SPELL_ATTR_EX3_UNK26, // 26
 SPELL_ATTR_EX3_UNK27, // 27
 SPELL_ATTR_EX3_UNK28, // 28
 SPELL_ATTR_EX3_UNK29, // 29
 SPELL_ATTR_EX3_UNK30, // 30
 SPELL_ATTR_EX3_UNK31, // 31

 SPELL_ATTR_EX4_UNK0 = 0x0400, // 0
 SPELL_ATTR_EX4_UNK1, // 1 proc on finishing move?
 SPELL_ATTR_EX4_UNK2, // 2
 SPELL_ATTR_EX4_UNK3, // 3
 SPELL_ATTR_EX4_UNK4, // 4 This will no longer cause guards to attack on use??
 SPELL_ATTR_EX4_UNK5, // 5
 SPELL_ATTR_EX4_NOT_STEALABLE, // 6 although such auras might be dispellable, they cannot be stolen
 SPELL_ATTR_EX4_UNK7, // 7
 SPELL_ATTR_EX4_STACK_DOT_MODIFIER, // 8 no effect on non DoTs?
 SPELL_ATTR_EX4_UNK9, // 9
 SPELL_ATTR_EX4_SPELL_VS_EXTEND_COST, // 10 Rogue Shiv have this flag
 SPELL_ATTR_EX4_UNK11, // 11
 SPELL_ATTR_EX4_UNK12, // 12
 SPELL_ATTR_EX4_UNK13, // 13
 SPELL_ATTR_EX4_UNK14, // 14
 SPELL_ATTR_EX4_UNK15, // 15
 SPELL_ATTR_EX4_NOT_USABLE_IN_ARENA, // 16 not usable in arena
 SPELL_ATTR_EX4_USABLE_IN_ARENA, // 17 usable in arena
 SPELL_ATTR_EX4_UNK18, // 18
 SPELL_ATTR_EX4_UNK19, // 19
 SPELL_ATTR_EX4_UNK20, // 20 do not give "more powerful spell" error message
 SPELL_ATTR_EX4_UNK21, // 21
 SPELL_ATTR_EX4_UNK22, // 22
 SPELL_ATTR_EX4_UNK23, // 23
 SPELL_ATTR_EX4_UNK24, // 24
 SPELL_ATTR_EX4_UNK25, // 25 pet scaling auras
 SPELL_ATTR_EX4_CAST_ONLY_IN_OUTLAND, // 26 Can only be used in Outland.
 SPELL_ATTR_EX4_UNK27, // 27
 SPELL_ATTR_EX4_UNK28, // 28
 SPELL_ATTR_EX4_UNK29, // 29
 SPELL_ATTR_EX4_UNK30, // 30
 SPELL_ATTR_EX4_UNK31, // 31

 SPELL_ATTR_EX5_UNK0 = 0x0500, // 0
 SPELL_ATTR_EX5_NO_REAGENT_WHILE_PREP, // 1 not need reagents if UNIT_FLAG_PREPARATION
 SPELL_ATTR_EX5_UNK2, // 2 removed at enter arena (e.g. 31850 since 3.3.3)
 SPELL_ATTR_EX5_USABLE_WHILE_STUNNED, // 3 usable while stunned
 SPELL_ATTR_EX5_UNK4, // 4
 SPELL_ATTR_EX5_SINGLE_TARGET_SPELL, // 5 Only one target can be apply at a time
 SPELL_ATTR_EX5_UNK6, // 6
 SPELL_ATTR_EX5_UNK7, // 7
 SPELL_ATTR_EX5_UNK8, // 8
 SPELL_ATTR_EX5_START_PERIODIC_AT_APPLY, // 9 begin periodic tick at aura apply
 SPELL_ATTR_EX5_UNK10, // 10
 SPELL_ATTR_EX5_UNK11, // 11
 SPELL_ATTR_EX5_UNK12, // 12
 SPELL_ATTR_EX5_UNK13, // 13 haste affects duration (e.g. 8050 since 3.3.3)
 SPELL_ATTR_EX5_UNK14, // 14
 SPELL_ATTR_EX5_UNK15, // 15
 SPELL_ATTR_EX5_UNK16, // 16
 SPELL_ATTR_EX5_USABLE_WHILE_FEARED, // 17 usable while feared
 SPELL_ATTR_EX5_USABLE_WHILE_CONFUSED, // 18 usable while confused
 SPELL_ATTR_EX5_UNK19, // 19
 SPELL_ATTR_EX5_UNK20, // 20
 SPELL_ATTR_EX5_UNK21, // 21
 SPELL_ATTR_EX5_UNK22, // 22
 SPELL_ATTR_EX5_UNK23, // 23
 SPELL_ATTR_EX5_UNK24, // 24
 SPELL_ATTR_EX5_UNK25, // 25
 SPELL_ATTR_EX5_UNK26, // 26
 SPELL_ATTR_EX5_UNK27, // 27
 SPELL_ATTR_EX5_UNK28, // 28
 SPELL_ATTR_EX5_UNK29, // 29
 SPELL_ATTR_EX5_UNK30, // 30
 SPELL_ATTR_EX5_UNK31, // 31 Forces all nearby enemies to focus attacks caster

 SPELL_ATTR_EX6_UNK0 = 0x0600, // 0 Only Move spell have this flag
 SPELL_ATTR_EX6_ONLY_IN_ARENA, // 1 only usable in arena, not used in 3.2.0a and early
 SPELL_ATTR_EX6_UNK2, // 2
 SPELL_ATTR_EX6_UNK3, // 3
 SPELL_ATTR_EX6_UNK4, // 4
 SPELL_ATTR_EX6_UNK5, // 5
 SPELL_ATTR_EX6_UNK6, // 6
 SPELL_ATTR_EX6_UNK7, // 7
 SPELL_ATTR_EX6_UNK8, // 8
 SPELL_ATTR_EX6_UNK9, // 9
 SPELL_ATTR_EX6_UNK10, // 10
 SPELL_ATTR_EX6_NOT_IN_RAID_INSTANCE, // 11 not usable in raid instance
 SPELL_ATTR_EX6_UNK12, // 12 for auras SPELL_AURA_TRACK_CREATURES, SPELL_AURA_TRACK_RESOURCES and SPELL_AURA_TRACK_STEALTHED select non-stacking tracking spells
 SPELL_ATTR_EX6_UNK13, // 13
 SPELL_ATTR_EX6_UNK14, // 14
 SPELL_ATTR_EX6_UNK15, // 15 not set in 3.0.3
 SPELL_ATTR_EX6_UNK16, // 16
 SPELL_ATTR_EX6_UNK17, // 17
 SPELL_ATTR_EX6_UNK18, // 18
 SPELL_ATTR_EX6_UNK19, // 19
 SPELL_ATTR_EX6_UNK20, // 20
 SPELL_ATTR_EX6_UNK21, // 21
 SPELL_ATTR_EX6_UNK22, // 22
 SPELL_ATTR_EX6_UNK23, // 23 not set in 3.0.3
 SPELL_ATTR_EX6_UNK24, // 24 not set in 3.0.3
 SPELL_ATTR_EX6_UNK25, // 25 not set in 3.0.3
 SPELL_ATTR_EX6_UNK26, // 26 not set in 3.0.3
 SPELL_ATTR_EX6_UNK27, // 27 not set in 3.0.3
 SPELL_ATTR_EX6_UNK28, // 28 not set in 3.0.3
 SPELL_ATTR_EX6_NO_DMG_PERCENT_MODS, // 29 do not apply damage percent mods (usually in cases where it has already been applied)
 SPELL_ATTR_EX6_UNK30, // 30 not set in 3.0.3
 SPELL_ATTR_EX6_UNK31, // 31 not set in 3.0.3

 SPELL_ATTR_EX7_UNK0 = 0x0700, // 0
 SPELL_ATTR_EX7_UNK1, // 1
 SPELL_ATTR_EX7_UNK2, // 2
 SPELL_ATTR_EX7_UNK3, // 3
 SPELL_ATTR_EX7_UNK4, // 4
 SPELL_ATTR_EX7_UNK5, // 5
 SPELL_ATTR_EX7_UNK6, // 6
 SPELL_ATTR_EX7_UNK7, // 7
 SPELL_ATTR_EX7_UNK8, // 8
 SPELL_ATTR_EX7_UNK9, // 9
 SPELL_ATTR_EX7_UNK10, // 10
 SPELL_ATTR_EX7_UNK11, // 11
 SPELL_ATTR_EX7_UNK12, // 12
 SPELL_ATTR_EX7_UNK13, // 13
 SPELL_ATTR_EX7_UNK14, // 14
 SPELL_ATTR_EX7_UNK15, // 15
 SPELL_ATTR_EX7_UNK16, // 16
 SPELL_ATTR_EX7_UNK17, // 17
 SPELL_ATTR_EX7_UNK18, // 18
 SPELL_ATTR_EX7_UNK19, // 19
 SPELL_ATTR_EX7_UNK20, // 20
 SPELL_ATTR_EX7_UNK21, // 21
 SPELL_ATTR_EX7_UNK22, // 22
 SPELL_ATTR_EX7_UNK23, // 23
 SPELL_ATTR_EX7_UNK24, // 24
 SPELL_ATTR_EX7_UNK25, // 25
 SPELL_ATTR_EX7_UNK26, // 26
 SPELL_ATTR_EX7_UNK27, // 27
 SPELL_ATTR_EX7_UNK28, // 28
 SPELL_ATTR_EX7_UNK29, // 29
 SPELL_ATTR_EX7_UNK30, // 30
 SPELL_ATTR_EX7_UNK31, // 31

 SPELL_ATTR_EX8_UNK0 = 0x0800, // 0
 SPELL_ATTR_EX8_UNK1, // 1
 SPELL_ATTR_EX8_UNK2, // 2
 SPELL_ATTR_EX8_UNK3, // 3
 SPELL_ATTR_EX8_UNK4, // 4
 SPELL_ATTR_EX8_UNK5, // 5
 SPELL_ATTR_EX8_UNK6, // 6
 SPELL_ATTR_EX8_UNK7, // 7
 SPELL_ATTR_EX8_UNK8, // 8
 SPELL_ATTR_EX8_UNK9, // 9
 SPELL_ATTR_EX8_UNK10, // 10
 SPELL_ATTR_EX8_UNK11, // 11
 SPELL_ATTR_EX8_UNK12, // 12
 SPELL_ATTR_EX8_UNK13, // 13
 SPELL_ATTR_EX8_UNK14, // 14
 SPELL_ATTR_EX8_UNK15, // 15
 SPELL_ATTR_EX8_UNK16, // 16
 SPELL_ATTR_EX8_UNK17, // 17
 SPELL_ATTR_EX8_UNK18, // 18
 SPELL_ATTR_EX8_UNK19, // 19
 SPELL_ATTR_EX8_UNK20, // 20
 SPELL_ATTR_EX8_UNK21, // 21
 SPELL_ATTR_EX8_UNK22, // 22
 SPELL_ATTR_EX8_UNK23, // 23
 SPELL_ATTR_EX8_UNK24, // 24
 SPELL_ATTR_EX8_UNK25, // 25
 SPELL_ATTR_EX8_UNK26, // 26
 SPELL_ATTR_EX8_UNK27, // 27
 SPELL_ATTR_EX8_UNK28, // 28
 SPELL_ATTR_EX8_UNK29, // 29
 SPELL_ATTR_EX8_UNK30, // 30
 SPELL_ATTR_EX8_UNK31, // 31

 SPELL_ATTR_EX9_UNK0 = 0x0900, // 0
 SPELL_ATTR_EX9_UNK1, // 1
 SPELL_ATTR_EX9_UNK2, // 2
 SPELL_ATTR_EX9_UNK3, // 3
 SPELL_ATTR_EX9_UNK4, // 4
 SPELL_ATTR_EX9_UNK5, // 5
 SPELL_ATTR_EX9_UNK6, // 6
 SPELL_ATTR_EX9_UNK7, // 7
 SPELL_ATTR_EX9_UNK8, // 8
 SPELL_ATTR_EX9_UNK9, // 9
 SPELL_ATTR_EX9_UNK10, // 10
 SPELL_ATTR_EX9_UNK11, // 11
 SPELL_ATTR_EX9_UNK12, // 12
 SPELL_ATTR_EX9_UNK13, // 13
 SPELL_ATTR_EX9_UNK14, // 14
 SPELL_ATTR_EX9_UNK15, // 15
 SPELL_ATTR_EX9_UNK16, // 16
 SPELL_ATTR_EX9_UNK17, // 17
 SPELL_ATTR_EX9_UNK18, // 18
 SPELL_ATTR_EX9_UNK19, // 19
 SPELL_ATTR_EX9_UNK20, // 20
 SPELL_ATTR_EX9_UNK21, // 21
 SPELL_ATTR_EX9_UNK22, // 22
 SPELL_ATTR_EX9_UNK23, // 23
 SPELL_ATTR_EX9_UNK24, // 24
 SPELL_ATTR_EX9_UNK25, // 25
 SPELL_ATTR_EX9_UNK26, // 26
 SPELL_ATTR_EX9_UNK27, // 27
 SPELL_ATTR_EX9_UNK28, // 28
 SPELL_ATTR_EX9_UNK29, // 29
 SPELL_ATTR_EX9_UNK30, // 30
 SPELL_ATTR_EX9_UNK31, // 31
};


class sc_data_access_t : public sc_data_t
{
public:
  sc_data_access_t( sc_data_t* p = NULL );
  sc_data_access_t( const sc_data_t& copy );
  virtual ~sc_data_access_t() { };

  // Spell methods
  virtual bool          spell_exists( const uint32_t spell_id ) SC_CONST;
  virtual const char*   spell_name_str( const uint32_t spell_id ) SC_CONST;
  virtual bool          spell_is_used( const uint32_t spell_id ) SC_CONST;
  virtual void          spell_set_used( const uint32_t spell_id, const bool value );
  virtual bool          spell_is_enabled( const uint32_t spell_id ) SC_CONST;
  virtual void          spell_set_enabled( const uint32_t spell_id, const bool value );

  virtual double        spell_missile_speed( const uint32_t spell_id ) SC_CONST;
  virtual uint32_t      spell_school_mask( const uint32_t spell_id ) SC_CONST;
  virtual resource_type spell_power_type( const uint32_t spell_id ) SC_CONST;
  virtual bool          spell_is_class( const uint32_t spell_id, const player_type c ) SC_CONST;
  virtual bool          spell_is_race( const uint32_t spell_id, const race_type r ) SC_CONST;
  virtual bool          spell_is_level( const uint32_t spell_id, const uint32_t level ) SC_CONST;
  virtual double        spell_min_range( const uint32_t spell_id ) SC_CONST;
  virtual double        spell_max_range( const uint32_t spell_id ) SC_CONST;
  virtual bool          spell_in_range( const uint32_t spell_id, const double range ) SC_CONST;
  virtual double        spell_cooldown( const uint32_t spell_id ) SC_CONST;
  virtual double        spell_gcd( const uint32_t spell_id ) SC_CONST;
  virtual uint32_t      spell_category( const uint32_t spell_id ) SC_CONST;
  virtual double        spell_duration( const uint32_t spell_id ) SC_CONST;
  virtual double        spell_cost( const uint32_t spell_id ) SC_CONST;
  virtual double        spell_rune_cost( const uint32_t spell_id ) SC_CONST;
  virtual double        spell_runic_power_gain( const uint32_t spell_id ) SC_CONST;
  virtual uint32_t      spell_max_stacks( const uint32_t spell_id ) SC_CONST;
  virtual uint32_t      spell_initial_stacks( const uint32_t spell_id ) SC_CONST;
  virtual double        spell_proc_chance( const uint32_t spell_id ) SC_CONST;
  virtual double        spell_cast_time( const uint32_t spell_id, const uint32_t level ) SC_CONST;
  virtual uint32_t      spell_effect_id( const uint32_t spell_id, const uint32_t effect_num ) SC_CONST;
  virtual bool          spell_flags( const uint32_t spell_id, const spell_attribute_t f ) SC_CONST;

  // Effect methods
  virtual bool          effect_exists( const uint32_t effect_id ) SC_CONST;

  virtual bool          effect_is_used( const uint32_t effect_id ) SC_CONST;
  virtual void          effect_set_used( const uint32_t effect_id, const bool value );
  virtual bool          effect_is_enabled( const uint32_t effect_id ) SC_CONST;
  virtual void          effect_set_enabled( const uint32_t effect_id, const bool value );

  virtual uint32_t      effect_spell_id( const uint32_t effect_id ) SC_CONST;
  virtual uint32_t      effect_spell_effect_num( const uint32_t effect_id ) SC_CONST;
  virtual uint32_t      effect_type( const uint32_t effect_id ) SC_CONST;
  virtual uint32_t      effect_subtype( const uint32_t effect_id ) SC_CONST;
  virtual int32_t       effect_base_value( const uint32_t effect_id ) SC_CONST;
  virtual int32_t       effect_misc_value1( const uint32_t effect_id ) SC_CONST;
  virtual int32_t       effect_misc_value2( const uint32_t effect_id ) SC_CONST;
  virtual uint32_t      effect_trigger_spell_id( const uint32_t effect_id ) SC_CONST;

  virtual double        effect_m_average( const uint32_t effect_id ) SC_CONST;
  virtual double        effect_m_delta( const uint32_t effect_id ) SC_CONST;
  virtual double        effect_m_unk( const uint32_t effect_id ) SC_CONST;
  virtual double        effect_coeff( const uint32_t effect_id ) SC_CONST;
  virtual double        effect_period( const uint32_t effect_id ) SC_CONST;
  virtual double        effect_radius( const uint32_t effect_id ) SC_CONST;
  virtual double        effect_radius_max( const uint32_t effect_id ) SC_CONST;
  virtual double        effect_pp_combo_points( const uint32_t effect_id ) SC_CONST;

  virtual double        effect_average( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST;
  virtual double        effect_delta( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST;
  virtual double        effect_unk( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST;

  virtual double        effect_min( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST; 
  virtual double        effect_max( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST; 


// Talent methods
  virtual bool          talent_exists( const uint32_t talent_id ) SC_CONST;
  virtual const char*   talent_name_str( const uint32_t talent_id ) SC_CONST;
  virtual bool          talent_is_used( const uint32_t talent_id ) SC_CONST;
  virtual void          talent_set_used( const uint32_t talent_id, const bool value );
  virtual bool          talent_is_enabled( const uint32_t talent_id ) SC_CONST;
  virtual void          talent_set_enabled( const uint32_t talent_id, const bool value );
  virtual uint32_t      talent_tab_page( const uint32_t talent_id );
  virtual bool          talent_is_class( const uint32_t talent_id, const player_type c ) SC_CONST;
  virtual bool          talent_is_pet( const uint32_t talent_id, const pet_type_t p ) SC_CONST;
  virtual uint32_t      talent_depends_id( const uint32_t talent_id ) SC_CONST;
  virtual uint32_t      talent_depends_rank( const uint32_t talent_id ) SC_CONST;
  virtual uint32_t      talent_col( const uint32_t talent_id ) SC_CONST;
  virtual uint32_t      talent_row( const uint32_t talent_id ) SC_CONST;
  virtual uint32_t      talent_rank_spell_id( const uint32_t talent_id, const uint32_t rank ) SC_CONST;

  virtual uint32_t      talent_max_rank( const uint32_t talent_id ) SC_CONST;

  virtual uint32_t      talent_player_get_id_by_num( const player_type c, const uint32_t tab, const uint32_t num ) SC_CONST;
  virtual uint32_t      talent_pet_get_id_by_num( const pet_type_t p, const uint32_t num ) SC_CONST;

// Scaling methods
  virtual double        melee_crit_base( const player_type c ) SC_CONST;
  virtual double        spell_crit_base( const player_type c ) SC_CONST;
  virtual double        melee_crit_scale( const player_type c, const uint32_t level ) SC_CONST;
  virtual double        spell_crit_scale( const player_type c, const uint32_t level ) SC_CONST;
  virtual double        spi_regen( const player_type c, const uint32_t level ) SC_CONST;
  virtual double        oct_regen( const player_type c, const uint32_t level ) SC_CONST;
  virtual double        combat_ratings( const player_type c, const rating_type r, const uint32_t level ) SC_CONST;
  virtual double        dodge_base( const player_type c ) SC_CONST;
  virtual double        dodge_scale( const player_type c, const uint32_t level ) SC_CONST;
  virtual double        base_mp5( const player_type c, const uint32_t level ) SC_CONST;
  virtual double        class_stats( const player_type c, const uint32_t level, const stat_type s ) SC_CONST;
  virtual double        race_stats( const race_type r, const stat_type s ) SC_CONST;

private:
};


#endif
