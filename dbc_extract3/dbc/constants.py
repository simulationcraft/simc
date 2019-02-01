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
   ],
   # "Other"
   8: [
     # Battle for Azeroth
     163222, 163223, 163224, 163225,    # Battle potions
     152560, 152559, 152557,            # Potions of Bursting Blood, Rising Death, Steelskin
     160053,                            # Battle-Scarred Augment Rune
   ]
}

# Auto generated from sources above
ITEM_WHITELIST = [ ]

for cat, spells in CONSUMABLE_ITEM_WHITELIST.items():
   ITEM_WHITELIST += spells

# Unique item whitelist
ITEM_WHITELIST = list(set(ITEM_WHITELIST))
