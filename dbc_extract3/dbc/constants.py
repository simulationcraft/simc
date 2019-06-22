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
