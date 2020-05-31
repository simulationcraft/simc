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

CONSUMABLE_ITEM_WHITELIST = {
   # Food
   5: [
      62290,                            # Seafood Magnifique Feast
     133578,                            # Hearty Feast (7.0)
     133579,                            # Lavish Suramar Feast (7.0)
     156525,                            # Galley Banquet (8.0)
     156526,                            # Bountiful Captain's Feast (8.0)
     166804,                            # Boralus Blood Sausage (8.1)
     166240,                            # Sanguinated Feast (8.1)
     168315,                            # Famine Evaluator And Snack Table (8.2)
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
  re.compile("^Transcendence")
]

