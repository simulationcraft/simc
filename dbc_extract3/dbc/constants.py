# Constants
CLASS_NAMES = {
    'Warrior'     : 1,
    'Paladin'     : 2,
    'Hunter'      : 3,
    'Rogue'       : 4,
    'Priest'      : 5,
    'Death Knight': 6,
    'Shaman'      : 7,
    'Mage'        : 8,
    'Warlock'     : 9,
    'Monk'        : 10,
    'Druid'       : 11,
    'Demon Hunter': 12
}

RACE_NAMES = {
    'Human'              : 0,
    'Orc'                : 1,
    'Dwarf'              : 2,
    'Night Elf'          : 3,
    'Undead'             : 4,
    'Tauren'             : 5,
    'Gnome'              : 6,
    'Troll'              : 7,
    'Goblin'             : 8,
    'Blood Elf'          : 9,
    'Draenei'            : 10,
    'Dark Iron Dwarf'    : 11,
    'Mag\'har Orc'       : 13,
    'Worgen'             : 21,
    'Pandaren'           : 23,
    'Nightborne'         : 26,
    'Highmountain Tauren': 27,
    'Void Elf'           : 28,
    'Lightforged Draenei': 29
}

SKILL_CATEGORY_BLACKLIST = [
    148,                # Horse Riding
    762,                # Riding
    129,                # First aid
    183,                # Generic (DND)
]

# Any spell with this effect type, will be automatically
# blacklisted
# http://github.com/mangos/mangos/blob/400/src/game/SharedDefines.h
EFFECT_TYPE_BLACKLIST = [
    5,      # SPELL_EFFECT_TELEPORT_UNITS
    #10,     # SPELL_EFFECT_HEAL
    16,     # SPELL_EFFECT_QUEST_COMPLETE
    18,     # SPELL_EFFECT_RESURRECT
    25,     # SPELL_EFFECT_WEAPONS
    39,     # SPELL_EFFECT_LANGUAGE
    47,     # SPELL_EFFECT_TRADESKILL
    50,     # SPELL_EFFECT_TRANS_DOOR
    60,     # SPELL_EFFECT_PROFICIENCY
    71,     # SPELL_EFFECT_PICKPOCKET
    94,     # SPELL_EFFECT_SELF_RESURRECT
    97,     # SPELL_EFFECT_SUMMON_ALL_TOTEMS
    103,    # Grant reputation to faction
    109,    # SPELL_EFFECT_SUMMON_DEAD_PET
    110,    # SPELL_EFFECT_DESTROY_ALL_TOTEMS
    118,    # SPELL_EFFECT_SKILL
    126,    # SPELL_STEAL_BENEFICIAL_BUFF
    252,    # Some kind of teleport
    255,    # Some kind of transmog thing
]

# http://github.com/mangos/mangos/blob/400/src/game/SpellAuraDefines.h
AURA_TYPE_BLACKLIST = [
    1,      # SPELL_AURA_BIND_SIGHT
    2,      # SPELL_AURA_MOD_POSSESS
    5,      # SPELL_AURA_MOD_CONFUSE
    6,      # SPELL_AURA_MOD_CHARM
    7,      # SPELL_AURA_MOD_FEAR
    #8,      # SPELL_AURA_PERIODIC_HEAL
    17,     # SPELL_AURA_MOD_STEALTH_DETECT
    25,     # SPELL_AURA_MOD_PACIFY
    30,     # SPELL_AURA_MOD_SKILL (various skills?)
    #31,     # SPELL_AURA_MOD_INCREASE_SPEED
    44,     # SPELL_AURA_TRACK_CREATURES
    45,     # SPELL_AURA_TRACK_RESOURCES
    56,     # SPELL_AURA_TRANSFORM
    58,     # SPELL_AURA_MOD_INCREASE_SWIM_SPEED
    75,     # SPELL_AURA_MOD_LANGUAGE
    78,     # SPELL_AURA_MOUNTED
    82,     # SPELL_AURA_WATER_BREATHING
    91,     # SPELL_AURA_MOD_DETECT_RANGE
    98,     # SPELL_AURA_MOD_SKILL (trade skills?)
    104,    # SPELL_AURA_WATER_WALK,
    105,    # SPELL_AURA_FEATHER_FALL
    151,    # SPELL_AURA_TRACK_STEALTHED
    154,    # SPELL_AURA_MOD_STEALTH_LEVEL
    156,    # SPELL_AURA_MOD_REPUTATION_GAIN
    206,    # SPELL_AURA_MOD_FLIGHT_SPEED_xx begin
    207,
    208,
    209,
    210,
    211,
    212     # SPELL_AURA_MOD_FLIGHT_SPEED_xx ends
]

MECHANIC_BLACKLIST = [
    21,     # MECHANIC_MOUNT
]

# Pattern based whitelist, these will always be added
SPELL_NAME_BLACKLIST = [
    "^Acquire Treasure Map",
    "^Languages",
    "^Teleport:",
    "^Weapon Skills",
    "^Armor Skills",
    "^Tamed Pet Passive",
    "^Empowering$",
]

SPELL_ID_BLACKLIST = [
    3561,   # Teleports --
    3562,
    3563,
    3565,
    3566,
    3567,   # -- Teleports
    20585,  # Wisp spirit (night elf racial)
    42955,  # Conjure Refreshment
    43987,  # Ritual of Refreshment
    48018,  # Demonic Circle: Summon
    48020,  # Demonic Circle: Teleport
    69044,  # Best deals anywhere (goblin racial)
    69046,  # Pack hobgoblin (woe hogger)
    68978,  # Flayer (worgen racial)
    68996,  # Two forms (worgen racial)
]

# Explicitly included list of spells per class, that cannot be
# found from talents, or based on a SkillLine category
# The list contains a tuple ( spell_id, category[, activated ] ),
# where category is an entry in the _class_categories class-specific
# tuples, e.g. general = 0, specialization0..3 = 1..4 and pets as a whole
# are 5. The optional activated parameter forces the spell to appear (if set
# to True) or disappear (if set to False) from the class activated spell list,
# regardless of the automated activated check.
# Manually entered general spells ("tree" 0) do not appear in class activated lists, even if
# they pass the "activated" check, but will appear there if the optional activated parameter
# exists, and is set to True.
# The first tuple in the list is for non-class related, generic spells that are whitelisted,
# without a category
SPELL_ID_LIST= [
    (
     109871, 109869,            # No'Kaled the Elements of Death - LFR
     107785, 107789,            # No'Kaled the Elements of Death - Normal
     109872, 109870,            # No'Kaled the Elements of Death - Heroic
     52586,  68043,  68044,     # Gurthalak, Voice of the Deeps - LFR, N, H
     109959, 109955, 109939,    # Rogue Legendary buffs for P1, 2, 3
     84745,  84746,             # Shallow Insight, Moderate Insight
     138537,                    # Death Knight Tier15 2PC melee pet special attack
     137597,                    # Legendary meta gem Lightning Strike
     137323, 137247,            # Healer legendary meta
     137331, 137326,
     146137,                    # Cleave
     146071,                    # Multistrike
     120032, 142530,            # Dancing Steel
     104993, 142535,            # Jade Spirit
     116631,                    # Colossus
     105617,                    # Alchemist's Flask
     137596,                    # Capacitance
     104510, 104423,            # Windsong Mastery / Haste buffs
     177172, 177175, 177176,    # WoD Legendary ring, phase 1(?)
     177161, 177159, 177160,    # WoD Legendary ring, phase 1(?)
     187619, 187616, 187624, 187625, # phase 2?
     143924,                    # Leech
     54861, 133022,             # Nitro boosts
     175457, 175456, 175439,    # Focus Augmentation / Hyper Augmentation / Stout Augmentation
     179154, 179155, 179156, 179157, # T17 LFR cloth dps set bonus nukes
     183950,                    # Darklight Ray (WoD 6.2 Int DPS Trinket 3 damage spell)
     184559,                    # Spirit Eruption (WoD 6.2 Agi DPS Trinket 3 damage spell)
     184279,                    # Felstorm (WoD 6.2 Agi DPS Trinket 2 damage spell)
     60235,                     # Darkmoon Card: Greatness proc
     71556, 71558, 71559, 71560,# Deathbringer's Will procs
     71484, 71485, 71492,
     45428, 45429, 45430,       # Shattered Sun Pendant procs
     45431, 45432, 45478,
     45479, 45480,
     184968,                    # Unholy coil heal (Blood dk class trinket)
     191091, 191118, 191119,    # Mark of the Loyal Druid spells
     191121, 191112, 191123,
     191124, 191146,
     214802, 214803,            # Momento of Angerboda trinket
     215476,                    # Obelisk of the Void trinket
     215695,                    # Figurehead of the Naglfar trinket
     215407,                    # Caged Horror trinket
     191545, 191548, 191549, 191550, 191551, 191552, 191553, 191554, # Darkmoon Deck: Dominion
     191603, 191604, 191605, 191606, 191607, 191608, 191609, 191610, # Darkmoon Deck: Hellfire
     191624, 191625, 191626, 191627, 191628, 191629, 191630, 191631, # Darkmoon Deck: Immortality
     216099,                    # Elementium Bomb Squirrel Generator trinket
     222517, 222518, 222519, 222520, # Nature's Call trinket
     222050,                   # Wriggling Sinew trinket
     221865,                   # Twisting Wind trinket
     221804, 221805,           # Ravaged Seed Pod trinket
     207694,                   # Symbiote Strike for Cinidaria, the Symbiote
     201639, 201640, 201641,   # 7.0 Lavish Suramar Feast food buffs
     201635, 201636, 201637,   # 7.0 Hearty Feast food buffs
     215047,                   # Terrorbound Nexus trinket
     # Eyasu's Mulligan stuff
     227394, 227397,
     227392, 227395,
     227396, 227393,
     215754, 215750,           # Magma Spit / summon spell for Spawn of Serpentrix
     # 7.1.5 de-links Mark of the Hidden Satyr damage spell fully from the driver
     191259,
     # 7.1.5 Entwined Elemental Foci buffs
     225729, 225730,
     # 7.1.5 Archimonde's Hatred Reborn damage spell 
     235188,
     # 7.2.0 Dreadstone of Endless Shadows stat buffs
     238499, 238500, 238501,
     # 7.3.0 Netherlight stuff
     252879, 252896, 253098, 253073,
     # 7.3.0 Acrid Catalyst Injector stat buffs
     255742, 255744,
     # 7.3.0 Sheath of Asara
     255870, 257702,
     # 7.3.0 Terminus Signaling Beacon
     257376,
     # 7.3.0 Gorshalach's Legacy
     255672,
     # 7.3.0 Forgefiend's Fabricator
     253322, 256025, 
     # 7.3.0 Riftworld Codex buffs and damage component
     252545, 251938, 256415, 252550,
     # 7.3.2 Norgannon pantheon "random school" nukes
     257243, 257532, 257241, 257242, 257534, 257533,
     # 7.3.5 Auxilary spells for the new races
     259756, 259760,
     # 8.0.1 Azerite power Elemental Whirl buffs
     268954, 268955, 268953,
    ),

    # Warrior:
    (
        ( 58385,  0 ),          # Glyph of Hamstring
        ( 118779, 0, False ),   # Victory Rush heal is not directly activatable
        ( 144442, 0 ),          # T16 Melee 4 pc buff
        ( 119938, 0 ),          # Overpower
        ( 209700, 0 ),           # Void Cleave (arms artifact gold medal)
        ( 218877, 0 ),
        ( 218850, 0 ),
        ( 218836, 0 ),
        ( 218835, 0 ),
        ( 218834, 0 ),
        ( 218822, 0 ),
        ( 209493, 0 ),
        ( 242952, 0 ),
        ( 242953, 0 ),
    ),

    # Paladin:
    (
        ( 86700, 5 ),           # Ancient Power
        ( 144581, 0 ),          # Blessing of the Guardians (prot T16 2-piece bonus)
        ( 122287, 0, True ),    # Symbiosis Wrath
        ( 42463, 0, False ),    # Seal of Truth damage id not directly activatable
        ( 114852, 0, False ),   # Holy Prism false positives for activated
        ( 114871, 0, False ),
        ( 65148, 0, False ),    # Sacred Shield absorb tick
        ( 136494, 0, False ),   # World of Glory false positive for activated
        ( 113075, 0, False ),   # Barkskin (from Symbiosis)
        ( 144569, 0, False ),   # Bastion of Power (prot T16 4-piece bonus)
        ( 130552, 0, True ),    # Harsh Word
        ( 186876, 0 ),          # echoed Divine Storm (speculative)
        ( 186805, 0 ),          # echoed Templar's Verdict (speculative)
        ( 193115, 0 ),          # Blade of Light (speculative)
        ( 180290, 0 ),          # ashen strike (speculative)
        ( 221883, 0 ),          # Divine Steed
        ( 228288, 0, True ),
        ( 205729, 0 ),
        ( 238996, 0 ),          # Righteous Verdict
        ( 242981, 0 ),          # Blessing of the Ashbringer
        ( 211561, 0 ),          # Justice Gaze
        ( 246973, 0 ),          # Sacred Judgment (Ret T20 4p)
    ),

    # Hunter:
    (
      ( 131900, 0 ), # Murder of Crows damage spell
      ( 171457, 1 ), # Chimaera Shot - Nature
      ( 201594, 1 ), # Stampede
      ( 118459, 5 ), # Beast Cleave
      ( 257622, 2 ), # Trick Shots buff
      ( 269502, 2 ), ( 260395, 2 ), # Lethal Shots buffs
      ( 259516, 3 ), # Flanking Strike
      ( 267666, 3 ), # Chakrams
      # Wildfire Infusion (Volatile Bomb spells)
      ( 271048, 3 ), ( 271049, 3 ), ( 260231, 3 ),
    ),

    # Rogue:
    (
        ( 121474, 0 ),          # Shadow Blades off hand
        ( 113780, 0, False ),   # Deadly Poison damage is not directly activatable
        ( 89775, 0, False ),    # Hemorrhage damage is not directy activatable
        ( 86392, 0, False ),    # Main Gauche false positive for activatable
        ( 145211, 0 ),          # Subtlety Tier16 4PC proc
        ( 168908, 0 ),          # Sinister Calling: Hemorrhage
        ( 168952, 0 ),          # Sinister Calling: Crimson Tempest
        ( 168971, 0 ),          # Sinister Calling: Garrote
        ( 168963, 0 ),          # Sinister Calling: Rupture
        ( 115189, 0 ),          # Anticipation buff
        ( 157562, 0 ),          # Crimson Poison (Enhanced Crimson Tempest perk)
        ( 186183, 0 ),          # Assassination T18 2PC Nature Damage component
        ( 157957, 0 ),          # Shadow Reflection Dispatch
        ( 173458, 0 ),          # Shadow Reflection Backstab
        ( 195627, 0 ),          # Free Pistol Shot proc for Saber Slash
        # Roll the Bones buffs
        ( 199603, 0 ), ( 193358, 0 ), ( 193357, 0 ), ( 193359, 0 ), ( 199600, 0 ), ( 193356, 0 ),
        ( 193536, 0 ),          # Weaponmaster damage spell for ticks
        ( 197393, 0 ), ( 197395, 0 ), # Finality spells
        ( 192380, 0 ),          # Poisoned Knives damage component
        ( 202848, 0 ),          # Blunderbuss driver
        ( 197496, 0 ), ( 197498, 0 ), # New nightblade buffs
        ( 246558, 0 ),          # Outlaw T20 4pc Lesser Adrenaline Rush
        ( 279043, 0 ),          # Shadow Blades damaging spell
    ),

    # Priest:
    (   (  63619, 5 ), 			# Shadowfiend "Shadowcrawl"
        (  94472, 0 ), 			# Atonement Crit
        ( 114908, 0, False ), 	# Spirit Shell absorb
        ( 190714, 3, False ), 	# Shadow Word: Death - Insanity gain
        ( 193473, 5 ),			# Void Tendril "Mind Flay"
        ( 217676, 3 ),			# Mind Spike Detonation
        ( 194249, 3, False ),   # Void Form extra data
        ( 212570, 3, False ),   # Surrendered Soul (Surrender To Madness Death)
        ( 269555, 3 ),          # Azerite Trait Torment of Torments 
    ),

    # Death Knight:
    (
      ( 51963, 5 ), # gargoyle strike
      ( 66198, 0 ), # Obliterate off-hand
      ( 66196, 0 ), # Frost Strike off-hand
      ( 66216, 0 ), # Plague Strike off-hand
      ( 66188, 0 ), # Death Strike off-hand
      ( 113516, 0, True ), # Symbiosis Wild Mushroom: Plague
      ( 52212, 0, False ), # Death and Decay false positive for activatable
      ( 81277, 5 ), ( 81280, 5 ), ( 50453, 5 ), # Bloodworms heal / burst
      ( 77535, 0 ), # Blood Shield
      ( 116783, 0 ), # Death Siphon heal
      ( 96171, 0 ), # Will of the Necropolish Rune Tap buff
      ( 144948, 0 ), # T16 tank 4PC Bone Shield charge proc
      ( 144953, 0 ), # T16 tank 2PC Death Strike proc
      ( 144909, 0 ), # T16 dps 4PC frost driver spell
      ( 57330, 0, True ), # Horn of Winter needs to be explicitly put in the general tree, as our (over)zealous filtering thinks it's not an active ability
      ( 47568, 0, True ), # Same goes for Empower Rune Weapon
      ( 170205, 0 ), # Frost T17 4pc driver continued ...
      ( 187981, 0 ), ( 187970, 0 ), # T18 4pc unholy relevant spells
      ( 184982, 0 ),    # Frozen Obliteration
      ( 212333, 5 ),    # Cleaver for Sludge Belcher
      ( 212332, 5 ),    # Smash for Sludge Belcher
      ( 212338, 5 ),    # Vile Gas for Sludge Belcher
      ( 212337, 5 ),	# Powerful Smash for Sludge Belcher
      ( 198715, 5 ),    # Val'kyr Strike for Dark Arbiter
      ( 211947, 0 ),    # Shadow Empowerment for Dark Arbiter
      ( 81141, 0 ),     # Crimson Scourge buff
      ( 212423, 5 ),    # Skulker Shot for All Will Serve
      ( 207260, 5 ),    # Arrow Spray for All Will Serve
      ( 45470, 0 ),     # Death Strike heal
      ( 196545, 0 ),    # Bonestorm heal
      ( 221847, 0 ),    # Blood Mirror damage
      ( 205164, 0 ),    # Crystalline Swords
      ( 205165, 0 ),    # More crystalline swords stuff
      ( 191730, 0 ), ( 191727, 0 ), ( 191728, 0 ), ( 191729, 0 ), # Armies of the Damned debuffs
      ( 253590, 0 ),    # T21 4P frost damage component
    ),

    # Shaman:
    ( (  77451, 0 ), (  45284, 0 ), (  45297, 0 ),  #  Overloads
      ( 114074, 1 ), ( 114738, 0 ),                 # Ascendance: Lava Beam, Lava Beam overload
      ( 120687, 0 ), ( 120588, 0 ),                 # Stormlash, Elemental Blast overload
      ( 121617, 0 ),                                # Ancestral Swiftness 5% haste passive
      ( 25504, 0, False ), ( 33750, 0, False ),     # Windfury passives are not directly activatable
      ( 8034, 0, False ),                           # Frostbrand false positive for activatable
      ( 145002, 0, False ),                         # Lightning Elemental nuke
      ( 157348, 5 ), ( 157331, 5 ),                 # Storm elemental spells
      ( 159101, 0 ), ( 159105, 0 ), ( 159103, 0 ),  # Echo of the Elements spec buffs
      ( 173184, 0 ), ( 173185, 0 ), ( 173186, 0 ),  # Elemental Blast buffs
      ( 173183, 0 ),                                # Elemental Blast buffs
      ( 170512, 0 ), ( 170523, 0 ),                 # Feral Spirit windfury (t17 enhance 4pc set bonus)
      ( 189078, 0 ),                                # Gathering Vortex (Elemental t18 4pc charge)
      ( 201846, 0 ),                                # Stormfury buff
      ( 195256, 0 ), ( 195222, 0 ),                 # Stormlash stuff for legion
      ( 199054, 0 ), ( 199053, 0 ),                 # Unleash Doom, DOOM I SAY
      ( 198300, 0 ),                                # Gathering Storms buff
      ( 198830, 0 ), ( 199019, 0 ), ( 198933, 0 ),  # Doomhammer procs
      ( 197576, 0 ),                                # Magnitude ground? effect
      ( 191635, 0 ), ( 191634, 0 ),                 # Static Overload spells
      ( 202045, 0 ), ( 202044, 0 ),                 # Feral Swipe, Stomp
      ( 33750, 0 ),                                 # Offhand Windfury Attack
      ( 210801, 0 ), ( 210797, 0 ),                 # Crashing Storm
      ( 207835, 0 ), ( 213307, 0 ),                 # Stormlash auxilary spell(s)
      # Various feral spirit variation special attacks
      ( 198455, 0 ), ( 198483, 0 ), ( 198485, 0 ), ( 198480, 0 ),
      ( 191726, 0 ), ( 191732, 0 ),                 # Greater Lightning Elemental spells
      ( 214816, 0 ), ( 218559, 0 ), ( 218558, 0 ),  # Overload energize spells
      ( 219271, 0 ),                                # Icefury overload
      ( 77762, 0 ),                                 # Lava Surge cast time modifier
      ( 214134, 0 ),                                # The Deceiver's Blood Pact energize
      ( 224126, 0 ), ( 224125, 0 ), ( 224127, 0 ),  # Wolves of Doom Doom Effects, DOOM!
      ( 198506, 0 ),                                # Wolves of Doom, summon spell
      ( 197568, 0 ),                                # Lightning Rod damage spell
      ( 207998, 0 ), ( 207999, 0 ),                 # 7.0 legendary ring Eye of the Twisting Nether
      ( 252143, 0 ),                                # Earth Shock Overload (Elemental T21 2PC)
    ),

    # Mage:
    (
      ( 48107, 0, False ), ( 48108, 0, False ), # Heating Up and Pyroblast! buffs
      ( 88084, 5 ), ( 88082, 5 ), ( 59638, 5 ), # Mirror Image spells.
      ( 80354, 0 ),                             # Temporal Displacement
      ( 131581, 0 ),                            # Waterbolt
      ( 7268, 0, False ),                       # Arcane missiles trigger
      ( 115757, 0, False ),                     # Frost Nova false positive for activatable
      ( 145264, 0 ),                            # T16 Frigid Blast
      ( 148022, 0 ),                            # Icicle
      ( 155152, 5 ),                            # Prismatic Crystal nuke
      ( 157978, 0 ), ( 157979, 0 ),             # Unstable magic aoe
      ( 187677, 0 ),                            # Aegwynn's Ascendance AOE
      ( 191764, 0 ), ( 191799, 0 ),             # Arcane T18 2P Pet
      ( 194432, 0 ),                            # Felo'melorn - Aftershocks
      ( 225119, 5 ),                            # Arcane Familiar attack, Arcane Assault
      ( 210833, 0 ),                            # Touch of the Magi
      ( 228358, 0 ),                            # Winter's Chill
      ( 242253, 0 ),                            # Frost T20 2P Frozen Mass
      ( 240689, 0 ),                            # Aluneth - Time and Space
      ( 248147, 0 ),                            # Erupting Infernal Core
      ( 248177, 0 ),                            # Rage of the Frost Wyrm
      ( 222305, 0 ),                            # Sorcerous Fireball
      ( 222320, 0 ),                            # Sorcerous Frostbolt
      ( 222321, 0 ),                            # Sorcerous Arcane Blast
      ( 205473, 0 ),                            # Icicles buff
      ( 253257, 0 ),                            # Frost T21 4P Arctic Blast
      ( 187292, 0 ),                            # Ro3 buff (?)
      ( 264352, 0 ),                            # Mana Adept
      ( 263725, 0 ),                            # Clearcasting buff
      ( 264774, 0 ),                            # Ro3 buff (talent)
      ( 277726, 0 ),                            # Amplification (?)
    ),

    # Warlock:
    (
      ( 157900, 0, True ), # grimoire: doomguard
      ( 157901, 0, True ), # grimoire: infernal
      ( 104025, 2, True ), # immolation aura
      ( 104232, 3, True ), # destruction rain of fire
      ( 111859, 0, True ), ( 111895, 0, True ), ( 111897, 0, True ), ( 111896, 0, True ), ( 111898, 2, True ), # Grimoire of Service summons
      ( 114654, 0 ),        # Fire and Brimstone nukes
      ( 108686, 0 ),
      ( 109468, 0 ),
      ( 104225, 0 ),
      ( 129476, 0 ),        # Immolation Aura
      ( 189297, 0 ),        # Demonology Warlock T18 4P Pet
      ( 189298, 0 ),        # Demonology Warlock T18 4P Pet
      ( 189296, 0 ),        # Demonology Warlock T18 4P Pet
      ( 17941, 0 ),         # Agony energize
      ( 104318, 5 ),        # Wild Imp Firebolt
      ( 205196, 5 ),        # Dreadstalker Dreadbite
      ( 226802, 3 ),        # Lord of Flames buff
      ( 196659, 3 ),        # Dimensional rift spells
      ( 196606, 2 ),        # Shadowy Inspiration buff
      ( 205231, 5 ),        # Darkglare spell
      ( 196639, 3 ),
      ( 215409, 3 ),
      ( 215276, 3 ),
      ( 187385, 3 ),
      ( 205260, 0 ),        # Soul effigy damage
      ( 233496, 0 ),
      ( 233497, 0 ),
      ( 233498, 0 ),
      ( 233499, 0 ),
      ( 213229, 0 ),
      ( 243050, 0 ),        # Fire Rift
      ( 242922, 0 ),        # Jaws of Shadow
              ( 270569, 2 ),		# From the Shadows debuff
              ( 265279, 5 ),		# Demonic Tyrant - Demonfire Blast
              ( 270481, 5 ),		# Demonic Tyrant - Demonfire
              ( 267971, 5 ),		# Demonic Tyrant - Demonic Consumption
              ( 267972, 5 ),		# Demonic Tyrant - Demonic Consumption (not sure which is needed)
              ( 267997, 5 ),		# Vilefiend - Bile Spit
              ( 267999, 5 ),		# Vilefiend - Headbutt
              ( 267994, 2 ),		# Summon Shivarra
              ( 267996, 2 ),		# Summon Darkhound
              ( 267992, 2 ),		# Summon Bilescourge
              ( 268001, 2 ),		# Summon Ur'zul
              ( 267991, 2 ),		# Summon Void Terror
              ( 267995, 2 ),		# Summon Wrathguard
              ( 267988, 2 ),		# Summon Vicious Hellhound
              ( 267987, 2 ),		# Summon Illidari Satyr
              ( 267989, 2 ),		# Summon Eyes of Gul'dan
              ( 267986, 2 ),		# Summon Prince Malchezaar
              ( 272172, 5 ),		# Shivarra - Multi-Slash
              ( 272435, 5 ),		# Darkhound - Fel Bite
              ( 272167, 5 ),		# Bilescourge - Toxic Bile
              ( 272439, 5 ),		# Ur'zul - Many Faced Bite
              ( 272156, 5 ),		# Void Terror - Double Breath
              ( 272432, 5 ),		# Wrathguard - Overhead Assault
              ( 272013, 5 ),		# Vicious Hellhound - Demon Fangs
              ( 272012, 5 ),		# Illidari Satyr - Shadow Slash
              ( 272131, 5 ),		# Eye of Gul'dan - Eye of Gul'dan
              ( 267964, 0 ),		# new soul strike?
              
    ),

    # Monk:
    (
      # General
      # Brewmaster
      ( 195630, 1 ), # Brewmaster Mastery Buff
      ( 124503, 0 ), # Gift of the Ox Orb Left
      ( 124506, 0 ), # Gift of the Ox Orb Right
      ( 178173, 0 ), # Gift of the Ox Explosion
      ( 124275, 1 ), # Light Stagger
      ( 124274, 1 ), # Medium Stagger
      ( 124273, 1 ), # Heavy Stagger
      ( 216521, 1 ), # Celestial Fortune Heal
      ( 227679, 1 ), # Face Palm
      ( 227291, 1 ), # Niuzao pet Stomp
      # Mistweaver
      ( 228649, 2 ), # Teachings of the Monastery - Blackout Proc
      # Windwalker
      ( 115057, 3 ), # Flying Serpent Kick Movement spell
      ( 116768, 3 ), # Combo Breaker: Blackout Kick
      ( 121283, 0 ), # Chi Sphere from Power Strikes
      ( 125174, 3 ), # Touch of Karma redirect buff
      ( 195651, 3 ), # Crosswinds Artifact trait trigger spell
      ( 196061, 3 ), # Crosswinds Artifact trait damage spell
      ( 211432, 3 ), # Tier 19 4-piece DPS Buff
      ( 220358, 3 ), # Cyclone Strikes info
      ( 228287, 3 ), # Spinning Crane Kick's Mark of the Crane debuff
      ( 240672, 3 ), # Master of Combinations Artifact trait buff
      ( 242387, 3 ), # Thunderfist Artifact trait buff
      ( 252768, 3 ), # Tier 21 2-piece DPS effect
      # Legendary
      ( 213114, 3 ), # Hidden Master's Forbidden Touch buff
    ),

    # Druid:
    ( (  93402, 1, True ), # Sunfire
      ( 106996, 1, True ), # Astral Storm
      ( 112071, 1, True ), # Celestial Alignment
      ( 122114, 1, True ), # Chosen of Elune
      ( 122283, 0, True ),
      ( 110807, 0, True ),
      ( 112997, 0, True ),
      ( 144770, 1, False ), ( 144772, 1, False ), # Balance Tier 16 2pc spells
      ( 150017, 5 ),       # Rake for Treants
      ( 146874, 2 ),       # Feral Rage (T16 4pc feral bonus)
      ( 124991, 0 ), ( 124988, 0 ), # Nature's Vigil
      ( 155627, 2 ),       # Lunar Inspiration
      ( 155625, 2 ),       # Lunar Inspiration Moonfire
      ( 145152, 2 ),       # Bloodtalons buff
      ( 135597, 3 ),       # Tooth and Claw absorb buff
      ( 155784, 3 ),       # Primal Tenacity buff
      ( 137542, 0 ),       # Displacer Beast buff
      ( 185321, 3 ),       # Stalwart Guardian buff (T18 trinket)
      ( 188046, 5 ),       # T18 2P Faerie casts this spell
      ( 202771, 1 ),       # Half Moon artifact power
      ( 202768, 1 ),       # Full Moon artifact power
      ( 203001, 1 ),       # Goldrinn's Fang, Spirit of Goldrinn artifact power
      ( 203958, 3 ),       # Brambles (talent) damage spell
      ( 210721, 2 ),       # Shredded Wounds (Fangs of Ashamane artifact passive)
      ( 210713, 2 ),       # Ashamane's Rake (Fangs of Ashamane artifact trait spell)
      ( 210705, 2 ),       # Ashamane's Rip (Fangs of Ashamane artifact trait spell)
      ( 210649, 2 ),       # Feral Instinct (Fangs of Ashamane artifact trait)
      ( 211140, 2 ),       # Feral tier19_2pc
      ( 211142, 2 ),       # Feral tier19_4pc
      ( 213557, 2 ),       # Protection of Ashamane ICD (Fangs of Ashamane artifact trait)
      ( 211547, 1 ),       # Fury of Elune move spell
      ( 213771, 3 ),       # Swipe (Bear)
      ( 209406, 1 ),       # Oneth's Intuition buff
      ( 209407, 1 ),       # Oneth's Overconfidence buff
      ( 213666, 1 ),       # Echoing Stars artifact spell
      ( 69369,  2 ),       # Predatory Swiftness buff
      ( 227034, 3 ),       # Nature's Guardian heal
      ( 252750, 2 ),       # Feral tier21_2pc
      ( 274282, 1 ),       # Half Moon
      ( 274283, 1 ),       # Full Moon
    ),
    # Demon Hunter:
    (
      # General
      ( 225102, 0 ), # Fel Eruption damage

      # Havoc
      ( 236167, 1 ), # Felblade proc rate
      ( 208605, 1 ), # Nemesis player buff
      ( 203796, 1 ), # Demon Blade proc
      ( 217070, 1 ), # Rage of the Illidari explosion
      ( 217060, 1 ), # Rage of the Illidari buff
      ( 202446, 1 ), # Anguish damage spell
      ( 222703, 1 ), # Fel Barrage proc rate
      ( 211796, 1 ), # Chaos Blades damage spell

      # Vengeance
      ( 203557, 2 ), # Felblade proc rate
      ( 209245, 2 ), # Fiery Brand damage reduction
      ( 213011, 2 ), # Charred Warblades heal
      ( 212818, 2 ), # Fiery Demise debuff
   ),
]

CONSUMABLE_ITEM_WHITELIST = {
   # Food
   5: [
      62290,    # Seafood Magnifique Feast
   ]
}
