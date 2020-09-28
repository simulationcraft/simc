import re, enum

class Class(enum.IntEnum):
  NONE         = 0
  WARRIOR      = 1
  PALADIN      = 2
  HUNTER       = 3
  ROGUE        = 4
  PRIEST       = 5
  DEATH_KNIGHT = 6
  SHAMAN       = 7
  MAGE         = 8
  WARLOCK      = 9
  MONK         = 10
  DRUID        = 11
  DEMON_HUNTER = 12

class HotfixType(enum.IntEnum):
  DISABLED = 0
  ENABLED  = 1
  REMOVED  = 2

CONSUMABLE_ITEM_WHITELIST = {
  # Food
  5: [
    62290,                             # Seafood Magnifique Feast
    133578,                            # Hearty Feast (7.0)
    133579,                            # Lavish Suramar Feast (7.0)
    156525,                            # Galley Banquet (8.0)
    156526,                            # Bountiful Captain's Feast (8.0)
    166804,                            # Boralus Blood Sausage (8.1)
    166240,                            # Sanguinated Feast (8.1)
    168315,                            # Famine Evaluator And Snack Table (8.2),
    172043,                            # Feast of Gluttonous Hedonism (9.0)
    ],
  # "Other"
  8: [
    # Battle for Azeroth
    168489, 168498, 168500, 168499,    # Superior Battle potions (8.2)
    168506, 168529, 169299,            # focused resolve, empowered proximity, unbridled fury (8.2)
    # 8.2 gems are class/subclass of JC, not gems
    168637, 168638, 168636,            # Epic main stat gems (8.2)
    168639, 168641, 168640, 168642,    # Epic secondary gems (8.2)

    163222, 163223, 163224, 163225,    # Battle potions
    152560, 152559, 152557,            # Potions of Bursting Blood, Rising Death, Steelskin
    160053,                            # Battle-Scarred Augment Rune
    168506,                            # Potion of Focused Resolve
    # Shadowlands
    171270, 171273, 171275,            # Spectral Stat potions (9.0)
    171352, 171351, 171349,            # Empowered Exorcisms, Deathly Fixation, Phantom Fire
    ]
}

# Auto generated from sources above
ITEM_WHITELIST = [ ]

for cat, spells in CONSUMABLE_ITEM_WHITELIST.items():
   ITEM_WHITELIST += spells

# Unique item whitelist
ITEM_WHITELIST = list(set(ITEM_WHITELIST))

CLASS_SKILL_CATEGORIES = [
   0,
   840,    # Warrior
   800,    # Paladin
   795,    # Hunter
   921,    # Rogue
   804,    # Priest
   796,    # Death Knight
   924,    # Shaman
   904,    # Mage
   849,    # Warlock
   829,    # Monk
   798,    # Druid
   1848,   # Demon Hunter
]

RACE_INFO = [
  { 'id':  1, 'bit':  0, 'name': 'Human',              'skill': 754  },
  { 'id':  2, 'bit':  1, 'name': 'Orc',                'skill': 125  },
  { 'id':  3, 'bit':  2, 'name': 'Dwarf',              'skill': 101  },
  { 'id':  4, 'bit':  3, 'name': 'NightElf',           'skill': 126  },
  { 'id':  5, 'bit':  4, 'name': 'Undead',             'skill': 220  },
  { 'id':  6, 'bit':  5, 'name': 'Tauren',             'skill': 124  },
  { 'id':  7, 'bit':  6, 'name': 'Gnome',              'skill': 753  },
  { 'id':  8, 'bit':  7, 'name': 'Troll',              'skill': 733  },
  { 'id':  9, 'bit':  8, 'name': 'Goblin',             'skill': 790  },
  { 'id': 10, 'bit':  9, 'name': 'BloodElf',           'skill': 756  },
  { 'id': 11, 'bit': 10, 'name': 'Draenei',            'skill': 760  },
  { 'id': 22, 'bit': 21, 'name': 'Worgen',             'skill': 789  },
  { 'id': 24, 'bit': 23, 'name': 'Pandaren',           'skill': 899  },
  { 'id': 25, 'bit': 24, 'name': 'Pandaren',           'skill': 899  },
  { 'id': 26, 'bit': 25, 'name': 'Pandaren',           'skill': 899  },
  { 'id': 27, 'bit': 26, 'name': 'Nightborne',         'skill': 2419 },
  { 'id': 28, 'bit': 27, 'name': 'HighmountainTauren', 'skill': 2420 },
  { 'id': 29, 'bit': 28, 'name': 'VoidElf',            'skill': 2423 },
  { 'id': 30, 'bit': 29, 'name': 'LightforgedDraenei', 'skill': 2421 },
  { 'id': 31, 'bit': 30, 'name': 'ZandalariTroll',     'skill': 2721 },
  { 'id': 32, 'bit': 31, 'name': 'KulTiran',           'skill': 2723 },
  { 'id': 34, 'bit': 11, 'name': 'DarkIronDwarf',      'skill': 2773 },
  { 'id': 35, 'bit': 12, 'name': 'Vulpera',            'skill': 2775 },
  { 'id': 36, 'bit': 13, 'name': 'MagharOrc',          'skill': 2598 },
  { 'id': 37, 'bit': 14, 'name': 'Mechagnome',         'skill': 2774 }
]

CLASS_INFO = [
  { 'id':  1, 'bit':  0, 'name': 'Warrior',      'skill': 840  },
  { 'id':  2, 'bit':  1, 'name': 'Paladin',      'skill': 800  },
  { 'id':  3, 'bit':  2, 'name': 'Hunter',       'skill': 795  },
  { 'id':  4, 'bit':  3, 'name': 'Rogue',        'skill': 921  },
  { 'id':  5, 'bit':  4, 'name': 'Priest',       'skill': 804  },
  { 'id':  6, 'bit':  5, 'name': 'Death Knight', 'skill': 796  },
  { 'id':  7, 'bit':  6, 'name': 'Shaman',       'skill': 924  },
  { 'id':  8, 'bit':  7, 'name': 'Mage',         'skill': 904  },
  { 'id':  9, 'bit':  8, 'name': 'Warlock',      'skill': 849  },
  { 'id': 10, 'bit':  9, 'name': 'Monk',         'skill': 829  },
  { 'id': 11, 'bit': 10, 'name': 'Druid',        'skill': 798  },
  { 'id': 12, 'bit': 11, 'name': 'Demon Hunter', 'skill': 1848 }
]

PET_SKILL_CATEGORIES = [
  ( ),
  # Warrior
  ( ),
  # Paladin
  ( ),
  # Hunter
  ( 203, 208, 209, 210, 211, 212, 213, 214, 215, 217, 218, 236, 251, 270, 653, 654,
    655, 656, 763, 764, 765, 766, 767, 768, 775, 780, 781, 783, 784, 785, 786, 787,
    788, 808, 811, 815, 817, 818, 824, 983, 984, 985, 986, 988, 1305, 1819, 1993, 2189,
    2279, 2280, 2361, 2703, 2704, 2705, 2706, 2707, 2719),
  # Rogue
  ( ),
  # Priest
  ( ),
  # Death Knight
  ( 782, ),
  # Shaman
  ( 962, 963, 1748 ),
  # Mage
  ( 805, ),
  # Warlock
  ( 188, 189, 204, 205, 206, 207, 761, 927, 928, 929, 930, 931, 1981, 1982 ),
  # Monk
  ( ),
  # Druid
  ( ),
  # Demon Hunter
  ( )
]

SPELL_NAME_BLACKLIST = [
  re.compile("^Acquire Treasure Map"),
  re.compile("^Languages"),
  re.compile("^Teleport:"),
  re.compile("^Weapon Skills"),
  re.compile("^Armor Skills"),
  re.compile("^Tamed Pet Passive"),
  re.compile("^Empowering$"),
  re.compile("^Rush Order:"),
  re.compile("^Upgrade Weapon"),
  re.compile("^Upgrade Armor"),
  re.compile("^Wartorn Essence"),
  re.compile("^Battleborn Essence"),
  re.compile("^Portal:"),
  re.compile("^Polymorph"),
  re.compile("^Hex$"),
  re.compile("^Call Pet [2-9]"),
  re.compile("^Ancient (?:Portal|Teleport)"),
  re.compile("^Zen Pilgrimage"),
  re.compile("^Transcendence"),
  re.compile("^Contract:"),
  re.compile("^Apply Equipment$"),
]

SPELL_LABEL_BLACKLIST = [
  16,     # Agonizing Backlash
]
