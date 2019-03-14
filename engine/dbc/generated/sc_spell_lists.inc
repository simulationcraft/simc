#define CLASS_ABILITY_SIZE (51)
#define CLASS_ABILITY_TREE_SIZE (6)

#ifndef MAX_CLASS
#define MAX_CLASS (13)
#endif
// Class based active abilities, wow build 8.1.5.29705
static unsigned __class_ability_data[][CLASS_ABILITY_TREE_SIZE][CLASS_ABILITY_SIZE] = {
  {
    {
           0,
    },
    {
           0,
    },
    {
           0,
    },
    {
           0,
    },
    {
           0,
    },
    {
           0,
    },
  },
  // Class active abilities for Warrior
  {
    // General tree, 14 abilities
    {
        6673, // Battle Shout
      117313, // Bloodthirst Heal
         100, // Charge
      163558, // Execute Off-Hand
        6544, // Heroic Leap
       57755, // Heroic Throw
      203524, // Neltharion's Fury (Artifact)
      205545, // Odyn's Fury (Artifact)
      203178, // Opportunity Strikes
        6552, // Pummel
      156287, // Ravager
         355, // Taunt
      215537, // Trauma
      209577, // Warbreaker (Artifact)
           0,
    },
    // Arms tree, 14 abilities
    {
       18499, // Berserker Rage
      227847, // Bladestorm
      167105, // Colossus Smash
      118038, // Die by the Sword
      163201, // Execute
        1715, // Hamstring
        5246, // Intimidating Shout
       12294, // Mortal Strike
        7384, // Overpower
       97462, // Rallying Cry
        1464, // Slam
      260708, // Sweeping Strikes
       34428, // Victory Rush
        1680, // Whirlwind
           0,
    },
    // Fury tree, 12 abilities
    {
       18499, // Berserker Rage
       23881, // Bloodthirst
      184364, // Enraged Regeneration
        5308, // Execute
        5246, // Intimidating Shout
       12323, // Piercing Howl
       85288, // Raging Blow
       97462, // Rallying Cry
      184367, // Rampage
        1719, // Recklessness
       34428, // Victory Rush
      190411, // Whirlwind
           0,
    },
    // Protection tree, 17 abilities
    {
      107574, // Avatar
       18499, // Berserker Rage
        1160, // Demoralizing Shout
       20243, // Devastate
      190456, // Ignore Pain
      198304, // Intercept
        5246, // Intimidating Shout
       12975, // Last Stand
       97462, // Rallying Cry
        6572, // Revenge
        2565, // Shield Block
       23922, // Shield Slam
         871, // Shield Wall
       46968, // Shockwave
       23920, // Spell Reflection
        6343, // Thunder Clap
       34428, // Victory Rush
           0,
    },
    {
           0,
    },
    {
           0,
    },
  },
  // Class active abilities for Paladin
  {
    // General tree, 16 abilities
    {
       86704, // Ancient Fury
       66235, // Ardent Defender
        1044, // Blessing of Freedom
      121183, // Contemplation
       35395, // Crusader Strike
         642, // Divine Shield
      190784, // Divine Steed
      209202, // Eye of Tyr (Artifact)
       62124, // Hand of Reckoning
      162056, // Jailer's Judgment
      228288, // Judgment
         633, // Lay on Hands
      114919, // Light's Hammer
      200652, // Tyr's Deliverance (Artifact)
      200654, // Tyr's Deliverance
      205273, // Wake of Ashes (Artifact)
           0,
    },
    // Holy tree, 16 abilities
    {
      212056, // Absolution
       31821, // Aura Mastery
       31884, // Avenging Wrath
       53563, // Beacon of Light
        1022, // Blessing of Protection
        6940, // Blessing of Sacrifice
        4987, // Cleanse
       26573, // Consecration
         498, // Divine Protection
       19750, // Flash of Light
         853, // Hammer of Justice
       82326, // Holy Light
       20473, // Holy Shock
      275773, // Judgment
       85222, // Light of Dawn
      183998, // Light of the Martyr
           0,
    },
    // Protection tree, 16 abilities
    {
       31850, // Ardent Defender
       31935, // Avenger's Shield
       31884, // Avenging Wrath
        1022, // Blessing of Protection
        6940, // Blessing of Sacrifice
      213644, // Cleanse Toxins
       26573, // Consecration
         498, // Divine Protection
       19750, // Flash of Light
       86659, // Guardian of Ancient Kings
         853, // Hammer of Justice
       53595, // Hammer of the Righteous
      275779, // Judgment
      184092, // Light of the Protector
       96231, // Rebuke
       53600, // Shield of the Righteous
           0,
    },
    // Retribution tree, 14 abilities
    {
       31884, // Avenging Wrath
      184575, // Blade of Justice
        1022, // Blessing of Protection
      213644, // Cleanse Toxins
       53385, // Divine Storm
       19750, // Flash of Light
      203538, // Greater Blessing of Kings
      203539, // Greater Blessing of Wisdom
         853, // Hammer of Justice
      183218, // Hand of Hindrance
       20271, // Judgment
       96231, // Rebuke
      184662, // Shield of Vengeance
       85256, // Templar's Verdict
           0,
    },
    {
           0,
    },
    {
           0,
    },
  },
  // Class active abilities for Hunter
  {
    // General tree, 23 abilities
    {
       61648, // Aspect of the Chameleon
      186257, // Aspect of the Cheetah
      186265, // Aspect of the Turtle
        1462, // Beast Lore
         883, // Call Pet 1
       83242, // Call Pet 2
       83243, // Call Pet 3
       83244, // Call Pet 4
       83245, // Call Pet 5
      272651, // Command Pet
        6991, // Feed Pet
        5384, // Feign Death
      125050, // Fetch
      127933, // Fireworks
        1543, // Flare
      203415, // Fury of the Eagle (Artifact)
         136, // Mend Pet
         982, // Revive Pet
      175686, // Stopping Power
        1515, // Tame Beast
      207068, // Titan's Thunder (Artifact)
      207097, // Titan's Thunder
      204147, // Windburst (Artifact)
           0,
    },
    // Beast Mastery tree, 14 abilities
    {
      193530, // Aspect of the Wild
      217200, // Barbed Shot
       19574, // Bestial Wrath
      171457, // Chimaera Shot
      193455, // Cobra Shot
        5116, // Concussive Shot
      147362, // Counter Shot
      109304, // Exhilaration
      187650, // Freezing Trap
       19577, // Intimidation
       34026, // Kill Command
       34477, // Misdirection
        2643, // Multi-Shot
      187698, // Tar Trap
           0,
    },
    // Marksmanship tree, 14 abilities
    {
       19434, // Aimed Shot
      185358, // Arcane Shot
      186387, // Bursting Shot
        5116, // Concussive Shot
      147362, // Counter Shot
      109304, // Exhilaration
      187650, // Freezing Trap
      260395, // Lethal Shots
       34477, // Misdirection
      257620, // Multi-Shot
      257044, // Rapid Fire
       56641, // Steady Shot
      187698, // Tar Trap
      288613, // Trueshot
           0,
    },
    // Survival tree, 18 abilities
    {
      186289, // Aspect of the Eagle
      187708, // Carve
      267666, // Chakrams
      266779, // Coordinated Assault
      109304, // Exhilaration
      259516, // Flanking Strike
      187650, // Freezing Trap
      190925, // Harpoon
       19577, // Intimidation
      259489, // Kill Command
       34477, // Misdirection
      265888, // Mongoose Bite
      187707, // Muzzle
      186270, // Raptor Strike
      259491, // Serpent Sting
      187698, // Tar Trap
      259495, // Wildfire Bomb
      195645, // Wing Clip
           0,
    },
    {
           0,
    },
    // Pet tree, 51 abilities
    {
      263446, // Acid Spit (Special Ability)
      160011, // Agile Reflexes (Special Ability)
       50433, // Ankle Crack (Special Ability)
      118459, // Beast Cleave
      186254, // Bestial Wrath
       17253, // Bite (Basic Attack)
       24423, // Bloody Screech (Special Ability)
      263869, // Bristle (Special Ability)
       93433, // Burrow Attack (Exotic Ability)
      263892, // Catlike Reflexes (Special Ability)
       16827, // Claw (Basic Attack)
       61684, // Dash (Basic Ability)
      160060, // Deadly Sting (Special Ability)
      263887, // Dragon's Guile (Special Ability)
       50285, // Dust Cloud (Special Ability)
      159953, // Feast (Exotic Ability)
       54644, // Frost Breath (Special Ability)
       92380, // Froststorm Breath (Exotic Ability)
      263840, // Furious Bite (Special Ability)
        2649, // Growl (Basic Ability)
      263853, // Infected Bite (Special Ability)
       19577, // Intimidation
       24394, // Intimidation
       83381, // Kill Command
      259277, // Kill Command
      263423, // Lock Jaw (Special Ability)
       54680, // Monstrous Bite (Special Ability)
      264264, // Nether Shock (Special Ability)
      263867, // Obsidian Skin (Special Ability)
       50245, // Pin (Special Ability)
       90347, // Play (Bonus Ability)
       24450, // Prowl (Bonus Ability)
      263857, // Ravage (Special Ability)
       94019, // Rest (Bonus Ability)
      263854, // Savage Rend (Special Ability)
      264055, // Serenity Dust (Special Ability)
      263904, // Serpent's Swiftness (Special Ability)
       26064, // Shell Shield (Special Ability)
       49966, // Smack (Basic Attack)
      264263, // Sonic Blast (Special Ability)
       90361, // Spirit Mend (Exotic Ability)
      264265, // Spirit Shock (Special Ability)
       90328, // Spirit Walk (Bonus Ability)
      264056, // Spore Cloud (Special Ability)
      263852, // Talon Rend (Special Ability)
      160065, // Tendon Rip (Special Ability)
      263858, // Toxic Sting (Special Ability)
       94022, // Trick (Bonus Ability)
       35346, // Warp Time (Special Ability)
      160067, // Web Spray (Special Ability)
      264360, // Winged Agility (Special Ability)
    },
  },
  // Class active abilities for Rogue
  {
    // General tree, 16 abilities
    {
       22482, // Blade Flurry
       31224, // Cloak of Shadows
      212205, // Create: Crimson Vial (PvP Talent)
      185311, // Crimson Vial
      202665, // Curse of the Dreadblades (Artifact)
        2818, // Deadly Poison
       56814, // Detection
        1966, // Feint
      209782, // Goremaw's Bite (Artifact)
        1766, // Kick
      192759, // Kingsbane (Artifact)
        1804, // Pick Lock
      114018, // Shroud of Concealment
        2983, // Sprint
       57934, // Tricks of the Trade
        8680, // Wound Poison
           0,
    },
    // Assassination tree, 21 abilities
    {
        2094, // Blind
        1833, // Cheap Shot
        3408, // Crippling Poison
        2823, // Deadly Poison
        1725, // Distract
       32645, // Envenom
        5277, // Evasion
      196819, // Eviscerate
       51723, // Fan of Knives
         703, // Garrote
         408, // Kidney Shot
        1329, // Mutilate
      185565, // Poisoned Knife
        1943, // Rupture
        6770, // Sap
       36554, // Shadowstep
        1752, // Sinister Strike
        1784, // Stealth
        1856, // Vanish
       79140, // Vendetta
        8679, // Wound Poison
           0,
    },
    // Outlaw tree, 17 abilities
    {
       13750, // Adrenaline Rush
        8676, // Ambush
      199804, // Between the Eyes
       13877, // Blade Flurry
        2094, // Blind
        1833, // Cheap Shot
        2098, // Dispatch
        1725, // Distract
        1776, // Gouge
      195457, // Grappling Hook
      185763, // Pistol Shot
      199754, // Riposte
      193316, // Roll the Bones
        6770, // Sap
      193315, // Sinister Strike
        1784, // Stealth
        1856, // Vanish
           0,
    },
    // Subtlety tree, 18 abilities
    {
          53, // Backstab
        2094, // Blind
        1833, // Cheap Shot
        1725, // Distract
        5277, // Evasion
      196819, // Eviscerate
         408, // Kidney Shot
      195452, // Nightblade
        6770, // Sap
      121471, // Shadow Blades
      185313, // Shadow Dance
       36554, // Shadowstep
      185438, // Shadowstrike
      197835, // Shuriken Storm
      114014, // Shuriken Toss
        1784, // Stealth
      212283, // Symbols of Death
        1856, // Vanish
           0,
    },
    {
           0,
    },
    {
           0,
    },
  },
  // Class active abilities for Priest
  {
    // General tree, 14 abilities
    {
      161862, // Ascension
         528, // Dispel Magic
       77489, // Echo of Light
         586, // Fade
      208065, // Light of T'uure (Artifact)
      207946, // Light's Wrath (Artifact)
      207948, // Light's Wrath
       32375, // Mass Dispel
         605, // Mind Control
       21562, // Power Word: Fortitude
        9484, // Shackle Undead
         585, // Smite
      215769, // Spirit of Redemption (PvP Talent)
      205065, // Void Torrent (Artifact)
           0,
    },
    // Discipline tree, 17 abilities
    {
       19236, // Desperate Prayer
        2061, // Flash Heal
      132157, // Holy Nova
       73325, // Leap of Faith
        1706, // Levitate
      212036, // Mass Resurrection
       33206, // Pain Suppression
       47540, // Penance
       62618, // Power Word: Barrier
      194509, // Power Word: Radiance
          17, // Power Word: Shield
        8122, // Psychic Scream
         527, // Purify
       47536, // Rapture
      186263, // Shadow Mend
         589, // Shadow Word: Pain
       34433, // Shadowfiend
           0,
    },
    // Holy tree, 19 abilities
    {
       19236, // Desperate Prayer
       64843, // Divine Hymn
        2061, // Flash Heal
       47788, // Guardian Spirit
        2060, // Heal
       14914, // Holy Fire
      132157, // Holy Nova
       88625, // Holy Word: Chastise
       34861, // Holy Word: Sanctify
        2050, // Holy Word: Serenity
       73325, // Leap of Faith
        1706, // Levitate
      212036, // Mass Resurrection
         596, // Prayer of Healing
       33076, // Prayer of Mending
        8122, // Psychic Scream
         527, // Purify
         139, // Renew
       64901, // Symbol of Hope
           0,
    },
    // Shadow tree, 19 abilities
    {
       47585, // Dispersion
        2061, // Flash Heal
       73325, // Leap of Faith
        1706, // Levitate
        8092, // Mind Blast
       15407, // Mind Flay
       48045, // Mind Sear
      217676, // Mind Spike Detonation
          17, // Power Word: Shield
        8122, // Psychic Scream
      213634, // Purify Disease
      186263, // Shadow Mend
         589, // Shadow Word: Pain
       34433, // Shadowfiend
      232698, // Shadowform
       15487, // Silence
       15286, // Vampiric Embrace
       34914, // Vampiric Touch
      228260, // Void Eruption
           0,
    },
    {
           0,
    },
    // Pet tree, 2 abilities
    {
      193473, // Mind Flay
       63619, // Shadowcrawl
           0,
    },
  },
  // Class active abilities for Death Knight
  {
    // General tree, 17 abilities
    {
       48707, // Anti-Magic Shell
      220143, // Apocalypse (Artifact)
      205223, // Consumption (Artifact)
      111673, // Control Undead
      127344, // Corpse Exploder
       56222, // Dark Command
       50977, // Death Gate
       49576, // Death Grip
       49998, // Death Strike
       48265, // Death's Advance
       47568, // Empower Rune Weapon
      190780, // Frost Breath
       57330, // Horn of Winter
       47528, // Mind Freeze
        3714, // Path of Frost
       61999, // Raise Ally
      190778, // Sindragosa's Fury (Artifact)
           0,
    },
    // Blood tree, 10 abilities
    {
      221562, // Asphyxiate
       50842, // Blood Boil
       49028, // Dancing Rune Weapon
       43265, // Death and Decay
      195292, // Death's Caress
      108199, // Gorefiend's Grasp
      206930, // Heart Strike
       48792, // Icebound Fortitude
      195182, // Marrowrend
       55233, // Vampiric Blood
           0,
    },
    // Frost tree, 8 abilities
    {
       45524, // Chains of Ice
       47568, // Empower Rune Weapon
       49143, // Frost Strike
       49184, // Howling Blast
       48792, // Icebound Fortitude
       49020, // Obliterate
       51271, // Pillar of Frost
      196770, // Remorseless Winter
           0,
    },
    // Unholy tree, 11 abilities
    {
      275699, // Apocalypse
       42650, // Army of the Dead
       45524, // Chains of Ice
       63560, // Dark Transformation
       47541, // Death Coil
       43265, // Death and Decay
       85948, // Festering Strike
       48792, // Icebound Fortitude
       77575, // Outbreak
       46584, // Raise Dead
       55090, // Scourge Strike
           0,
    },
    {
           0,
    },
    // Pet tree, 7 abilities
    {
       81280, // Blood Burst
       47468, // Claw
       51963, // Gargoyle Strike
       47481, // Gnaw
       47484, // Huddle
       47482, // Leap
      212423, // Skulker Shot
           0,
    },
  },
  // Class active abilities for Shaman
  {
    // General tree, 21 abilities
    {
      114911, // Ancestral Guidance
      108271, // Astral Shift
        2825, // Bloodlust
      192058, // Capacitor Totem
      204945, // Doom Winds (Artifact)
      207778, // Downpour
        2484, // Earthbind Totem
        2645, // Ghost Wolf
       32182, // Heroism
       51514, // Hex (Frog)
      210873, // Hex (Compy)
      211004, // Hex (Spider)
      211010, // Hex (Snake)
      211015, // Hex (Cockroach)
      269352, // Hex (Skeletal Hatchling)
      277778, // Hex (Zandalari Tendonripper)
      277784, // Hex (Wicker Mongrel)
         370, // Purge
      205495, // Stormkeeper (Artifact)
        8143, // Tremor Totem (Earth Totem)
      115356, // Windstrike
           0,
    },
    // Elemental tree, 14 abilities
    {
      188443, // Chain Lightning
       51886, // Cleanse Spirit
      198103, // Earth Elemental
        8042, // Earth Shock
       61882, // Earthquake
      198067, // Fire Elemental
      188389, // Flame Shock
      196840, // Frost Shock
        8004, // Healing Surge
      114074, // Lava Beam
       51505, // Lava Burst
      188196, // Lightning Bolt
       51490, // Thunderstorm
       57994, // Wind Shear
           0,
    },
    // Enhancement tree, 13 abilities
    {
       51886, // Cleanse Spirit
      187874, // Crash Lightning
      198103, // Earth Elemental
       51533, // Feral Spirit
      193796, // Flametongue
      196834, // Frostbrand
      188070, // Healing Surge
       60103, // Lava Lash
      187837, // Lightning Bolt
      193786, // Rockbiter
       58875, // Spirit Walk
       17364, // Stormstrike
       57994, // Wind Shear
           0,
    },
    // Restoration tree, 17 abilities
    {
      212048, // Ancestral Vision
        1064, // Chain Heal
         421, // Chain Lightning
      198103, // Earth Elemental
      188838, // Flame Shock
       73920, // Healing Rain
        5394, // Healing Stream Totem
        8004, // Healing Surge
      108280, // Healing Tide Totem
       77472, // Healing Wave
       51505, // Lava Burst
         403, // Lightning Bolt
       77130, // Purify Spirit
       61295, // Riptide
       98008, // Spirit Link Totem
       79206, // Spiritwalker's Grace
       57994, // Wind Shear
           0,
    },
    {
           0,
    },
    // Pet tree, 12 abilities
    {
       36213, // Angered Earth
      157348, // Call Lightning
      275382, // Ember Blast
      157375, // Eye of the Storm
       57984, // Fire Blast
      118337, // Harden Skin
      118297, // Immolate
      117588, // Meteor
      118345, // Pulverize
      275384, // Shocking Blast
      275386, // Spark Elemental
      157331, // Wind Gust
           0,
    },
  },
  // Class active abilities for Mage
  {
    // General tree, 30 abilities
    {
      120145, // Ancient Teleport: Dalaran
        1459, // Arcane Intellect
        1953, // Blink
      190336, // Conjure Refreshment
        2139, // Counterspell
      214634, // Ebonbolt (Artifact)
         122, // Frost Nova
       45438, // Ice Block
      131784, // Illusion
      210726, // Mark of Aluneth (Artifact)
      211076, // Mark of Aluneth
      211088, // Mark of Aluneth
      224968, // Mark of Aluneth (Artifact)
      194466, // Phoenix's Flames (Artifact)
         118, // Polymorph (Sheep)
       28271, // Polymorph (Turtle)
       28272, // Polymorph (Pig)
       61025, // Polymorph (Serpent)
       61305, // Polymorph (Black Cat)
       61721, // Polymorph (Rabbit)
       61780, // Polymorph (Turkey)
      126819, // Polymorph (Porcupine)
      161353, // Polymorph (Polar Bear Cub)
      161354, // Polymorph (Monkey)
      161355, // Polymorph (Penguin)
      161372, // Polymorph (Peacock)
      277787, // Polymorph (Direhorn)
      277792, // Polymorph (Bumblebee)
         475, // Remove Curse
       80353, // Time Warp
           0,
    },
    // Arcane tree, 12 abilities
    {
       44425, // Arcane Barrage
       30451, // Arcane Blast
        1449, // Arcane Explosion
        5143, // Arcane Missiles
       12042, // Arcane Power
      195676, // Displacement
       12051, // Evocation
      110959, // Greater Invisibility
          66, // Invisibility
      205025, // Presence of Mind
      235450, // Prismatic Barrier
       31589, // Slow
           0,
    },
    // Fire tree, 9 abilities
    {
      235313, // Blazing Barrier
      190319, // Combustion
       31661, // Dragon's Breath
      108853, // Fire Blast
         133, // Fireball
        2120, // Flamestrike
          66, // Invisibility
       11366, // Pyroblast
        2948, // Scorch
           0,
    },
    // Frost tree, 12 abilities
    {
      190356, // Blizzard
      235219, // Cold Snap
         120, // Cone of Cold
      108853, // Fire Blast
       44614, // Flurry
         116, // Frostbolt
       84714, // Frozen Orb
       11426, // Ice Barrier
       30455, // Ice Lance
       12472, // Icy Veins
          66, // Invisibility
       31687, // Summon Water Elemental
           0,
    },
    {
           0,
    },
    // Pet tree, 6 abilities
    {
      225119, // Arcane Assault
       88084, // Arcane Blast
       88082, // Fireball
       33395, // Freeze
       59638, // Frostbolt
       31707, // Waterbolt
           0,
    },
  },
  // Class active abilities for Warlock
  {
    // General tree, 27 abilities
    {
         710, // Banish
      187394, // Chaos Barrage
      215279, // Chaos Bolt
      119898, // Command Demon
        6201, // Create Healthstone
      111771, // Demonic Gateway
      196586, // Dimensional Rift (Artifact)
        1098, // Enslave Demon
        5782, // Fear
      111897, // Grimoire: Felhunter (Summon)
      111859, // Grimoire: Imp (Summon)
      111896, // Grimoire: Succubus (Summon)
      111895, // Grimoire: Voidwalker (Summon)
         755, // Health Funnel
      108396, // Mortal Coil (Talent)
      216698, // Reap Souls (Artifact)
         698, // Ritual of Summoning
      196657, // Shadow Bolt
       63106, // Siphon Life
       20707, // Soulstone
         691, // Summon Felhunter (Summon)
         688, // Summon Imp (Summon)
         712, // Summon Succubus (Summon)
         697, // Summon Voidwalker (Summon)
      211714, // Thal'kiel's Consumption (Artifact)
      211715, // Thal'kiel's Consumption
      104773, // Unending Resolve
           0,
    },
    // Affliction tree, 8 abilities
    {
         980, // Agony
         172, // Corruption
      234153, // Drain Life
       27243, // Seed of Corruption
      232670, // Shadow Bolt
       30283, // Shadowfury
      205180, // Summon Darkglare
       30108, // Unstable Affliction
           0,
    },
    // Demonology tree, 23 abilities
    {
      104316, // Call Dreadstalkers
      264178, // Demonbolt
      234153, // Drain Life
      270569, // From the Shadows
      111898, // Grimoire: Felguard (Summon)
      105174, // Hand of Gul'dan
      196277, // Implosion
         686, // Shadow Bolt
       30283, // Shadowfury
      196606, // Shadowy Inspiration
      267992, // Summon Bilescourge
      267996, // Summon Darkhound
      265187, // Summon Demonic Tyrant
      267989, // Summon Eyes of Gul'dan
       30146, // Summon Felguard (Summon)
      267987, // Summon Illidari Satyr
      267986, // Summon Prince Malchezaar
      267994, // Summon Shivarra
      268001, // Summon Ur'zul
      267988, // Summon Vicious Hellhound
      267991, // Summon Void Terror
      267995, // Summon Wrathguard
      279910, // Wild Imp
           0,
    },
    // Destruction tree, 13 abilities
    {
      116858, // Chaos Bolt
      215276, // Chaos Tear
       17962, // Conflagrate
      234153, // Drain Life
       80240, // Havoc
         348, // Immolate
       29722, // Incinerate
      226802, // Lord of Flames
        5740, // Rain of Fire
         686, // Shadow Bolt
       30283, // Shadowfury
      196639, // Shadowy Tear
        1122, // Summon Infernal (Guardian)
           0,
    },
    {
           0,
    },
    // Pet tree, 42 abilities
    {
       89766, // Axe Toss (Special Ability)
      267997, // Bile Spit
      171011, // Burning Presence (Special Ability)
        3716, // Consuming Shadows (Basic Attack)
      170995, // Cripple
      272013, // Demon Fangs
      270481, // Demonfire
      265279, // Demonfire Blast
      267971, // Demonic Consumption
      267972, // Demonic Consumption
       19505, // Devour Magic (Special Ability)
       85692, // Doom Bolt
      272156, // Double Breath
      205231, // Eye Beam
      272131, // Eye of Gul'dan
      272435, // Fel Bite
      104318, // Fel Firebolt (Basic Attack)
       89751, // Felstorm (Special Ability)
        3110, // Firebolt (Basic Attack)
       89792, // Flee (Special Ability)
      267999, // Headbutt
       19483, // Immolation
        7814, // Lash of Pain (Basic Attack)
       30213, // Legion Strike (Basic Attack)
        7870, // Lesser Invisibility (Special Ability)
      272439, // Many Faced Bite
      272172, // Multi-Slash
      272432, // Overhead Assault
       30151, // Pursuit (Special Ability)
        6358, // Seduction (Command Demon Ability)
      171014, // Seethe (Special Ability)
       54049, // Shadow Bite (Basic Attack)
      171138, // Shadow Lock (Special Ability)
      272012, // Shadow Slash
       89808, // Singe Magic (Command Demon Ability)
       19647, // Spell Lock (Command Demon Ability)
       17735, // Suffering (Special Ability)
      112042, // Threatening Presence (Special Ability)
      134477, // Threatening Presence (Special Ability)
      171021, // Torch Magic (Special Ability)
      272167, // Toxic Bile
        6360, // Whiplash (Special Ability)
           0,
    },
  },
  // Class active abilities for Monk
  {
    // General tree, 17 abilities
    {
      100784, // Blackout Kick
      117952, // Crackling Jade Lightning
      196061, // Crosswinds
      175697, // Disabling Technique
      214326, // Exploding Keg (Artifact)
      122281, // Healing Elixir
      115078, // Paralysis
      115546, // Provoke
      109132, // Roll
      205406, // Sheilun's Gift (Artifact)
      209525, // Soothing Mist (PvP Talent)
      205320, // Strike of the Windlord (Artifact)
      100780, // Tiger Palm
      101643, // Transcendence
      119996, // Transcendence: Transfer
      125883, // Zen Flight
      126892, // Zen Pilgrimage
           0,
    },
    // Brewmaster tree, 19 abilities
    {
      205523, // Blackout Strike
      115181, // Breath of Fire
      216521, // Celestial Fortune
      218164, // Detox
      195630, // Elusive Brawler
      115072, // Expel Harm
      115203, // Fortifying Brew
      124273, // Heavy Stagger
      115308, // Ironskin Brew
      121253, // Keg Smash
      119381, // Leg Sweep
      124275, // Light Stagger
      124274, // Moderate Stagger
      119582, // Purifying Brew
      116705, // Spear Hand Strike
      227291, // Stomp
      278767, // Training of Niuzao
      116670, // Vivify
      115176, // Zen Meditation
           0,
    },
    // Mistweaver tree, 15 abilities
    {
      228649, // Blackout Kick
      115450, // Detox
      124682, // Enveloping Mist
      191837, // Essence Font
      243435, // Fortifying Brew
      119381, // Leg Sweep
      116849, // Life Cocoon
      212051, // Reawaken
      115151, // Renewing Mist
      115310, // Revival
      107428, // Rising Sun Kick
      115175, // Soothing Mist
      101546, // Spinning Crane Kick
      116680, // Thunder Focus Tea
      116670, // Vivify
           0,
    },
    // Windwalker tree, 27 abilities
    {
      116768, // Blackout Kick!
      211432, // Combo Master
      196061, // Crosswinds
      286587, // Dance of Chi-Ji
      218164, // Detox
      116095, // Disable
      113656, // Fists of Fury
      101545, // Flying Serpent Kick
      115057, // Flying Serpent Kick
      252768, // Focus of Xuen
      287062, // Fury of Xuen
      287063, // Fury of Xuen
      288636, // Glory of the Dawn
      285594, // Good Karma
      213114, // Hidden Master's Forbidden Touch
      119381, // Leg Sweep
      228287, // Mark of the Crane
      240672, // Master of Combinations
      107428, // Rising Sun Kick
      116705, // Spear Hand Strike
      101546, // Spinning Crane Kick
      137639, // Storm, Earth, and Fire
      278710, // Swift Roundhouse
      242387, // Thunderfist
      115080, // Touch of Death
      122470, // Touch of Karma
      116670, // Vivify
           0,
    },
    {
           0,
    },
    {
           0,
    },
  },
  // Class active abilities for Druid
  {
    // General tree, 29 abilities
    {
      210722, // Ashamane's Frenzy (Artifact)
      210723, // Ashamane's Frenzy
        5487, // Bear Form (Shapeshift)
         768, // Cat Form (Shapeshift)
      127757, // Charm Woodland Creature
        1850, // Dash
      193753, // Dreamwalk
      236716, // Enraged Maul (PvP Talent)
         339, // Entangling Roots
      208253, // Essence of G'Hanir (Artifact)
      164862, // Flap
      165962, // Flight Form (Shapeshift)
      202771, // Full Moon (Artifact)
        6795, // Growl
      202768, // Half Moon (Artifact)
        2637, // Hibernate
       33917, // Mangle
        8921, // Moonfire (Lunar)
      164812, // Moonfire (Lunar)
      175682, // Nature's Control
      202767, // New Moon (Artifact)
      200851, // Rage of the Sleeper (Artifact)
       20484, // Rebirth
        8936, // Regrowth
        5221, // Shred
      210053, // Stag Form (Shapeshift)
      164815, // Sunfire (Solar)
         783, // Travel Form (Shapeshift)
      114282, // Treant Form (Shapeshift)
           0,
    },
    // Balance tree, 26 abilities
    {
       22812, // Barkskin
      194223, // Celestial Alignment
      122114, // Chosen of Elune
      202771, // Full Moon (Artifact)
      274283, // Full Moon
      211547, // Fury of Elune
      203001, // Goldrinn's Fang
      202768, // Half Moon (Artifact)
      274282, // Half Moon
       29166, // Innervate
      279641, // Lunar Shrapnel
      194153, // Lunar Strike (Lunar)
       24858, // Moonkin Form
      209406, // Oneth's Intuition
      209407, // Oneth's Overconfidence
      157228, // Owlkin Frenzy
        5215, // Prowl
        2782, // Remove Corruption
       78675, // Solar Beam
      190984, // Solar Wrath (Solar)
        2908, // Soothe
      191034, // Starfall
       78674, // Starsurge
      202461, // Stellar Drift
      272873, // Streaking Star
       93402, // Sunfire (Solar)
           0,
    },
    // Feral tree, 22 abilities
    {
      210705, // Ashamane's Rip
      106951, // Berserk
      145152, // Bloodtalons
      252750, // Bloody Gash
      210649, // Feral Instinct
      146874, // Feral Rage
       22568, // Ferocious Bite
      276026, // Iron Jaws
       22570, // Maim
      155625, // Moonfire
       69369, // Predatory Swiftness
        5215, // Prowl
        1822, // Rake
        2782, // Remove Corruption
        1079, // Rip
      106839, // Skull Bash
        2908, // Soothe
      106898, // Stampeding Roar
       61336, // Survival Instincts
      213764, // Swipe
      106832, // Thrash
        5217, // Tiger's Fury
           0,
    },
    // Guardian tree, 18 abilities
    {
       22812, // Barkskin
      203958, // Brambles
      289315, // Burst of Savagery
       22842, // Frenzied Regeneration
          99, // Incapacitating Roar
      192081, // Ironfur
        6807, // Maul
      227034, // Nature's Guardian
        5215, // Prowl
        2782, // Remove Corruption
      106839, // Skull Bash
        2908, // Soothe
      185321, // Stalwart Guardian
      106898, // Stampeding Roar
       61336, // Survival Instincts
      213764, // Swipe
      213771, // Swipe
      106832, // Thrash
           0,
    },
    // Restoration tree, 16 abilities
    {
       22812, // Barkskin
      145205, // Efflorescence
       29166, // Innervate
      102342, // Ironbark
       33763, // Lifebloom
       88423, // Nature's Cure
        5215, // Prowl
         774, // Rejuvenation
      212040, // Revitalize
        5176, // Solar Wrath (Solar)
        2908, // Soothe
       93402, // Sunfire (Solar)
       18562, // Swiftmend
         740, // Tranquility
      102793, // Ursol's Vortex
       48438, // Wild Growth
           0,
    },
    // Pet tree, 1 abilities
    {
      188046, // Fey Missile
           0,
    },
  },
  // Class active abilities for Demon Hunter
  {
    // General tree, 18 abilities
    {
      188499, // Blade Dance
      198589, // Blur
      179057, // Chaos Nova
      162794, // Chaos Strike
      278326, // Consume Magic
      183752, // Disrupt
      198013, // Eye Beam
      195072, // Fel Rush
      201467, // Fury of the Illidari (Artifact)
      201628, // Fury of the Illidari
      201789, // Fury of the Illidari
      131347, // Glide
      217832, // Imprison
      191427, // Metamorphosis
      207407, // Soul Carver (Artifact)
      188501, // Spectral Sight
      185123, // Throw Glaive
      198793, // Vengeful Retreat
           0,
    },
    // Havoc tree, 11 abilities
    {
      202446, // Anguish
      211796, // Chaos Blades
      211797, // Chaos Blades
      247938, // Chaos Blades
      196718, // Darkness
      203796, // Demon Blades
      162243, // Demon's Bite
      208605, // Nemesis
      217070, // Rage of the Illidari
      281854, // Torment
      275148, // Unbound Chaos
           0,
    },
    // Vengeance tree, 15 abilities
    {
      207760, // Burning Alive
      213011, // Charred Warblades
      203720, // Demon Spikes
      204021, // Fiery Brand
      209245, // Fiery Brand
      212818, // Fiery Demise
      178740, // Immolation Aura
      189110, // Infernal Strike
      203782, // Shear
      204596, // Sigil of Flame
      207684, // Sigil of Misery
      202137, // Sigil of Silence
      228477, // Soul Cleave
      204157, // Throw Glaive
      185245, // Torment
           0,
    },
    {
           0,
    },
    {
           0,
    },
    {
           0,
    },
  },
};
#define TREE_SPECIALIZATION_SIZE (34)

// Talent tree specialization abilities, wow build 8.1.5.29705
static unsigned __tree_specialization_data[][MAX_SPECS_PER_CLASS][TREE_SPECIALIZATION_SIZE] = {
  // Specialization abilities for Hunter pets
  {
    // Specialization abilities for Ferocity
    {
      264663, // Predator's Thirst (Ferocity Passive)
      264667, // Primal Rage (Ferocity Ability)
      264656, // Pathfinding (Cunning Passive)
      264662, // Endurance Training (Tenacity Passive)
           0,
    },
    // Specialization abilities for Tenacity
    {
      264662, // Endurance Training (Tenacity Passive)
      264735, // Survival of the Fittest (Tenacity Ability)
      264663, // Predator's Thirst (Ferocity Passive)
      264656, // Pathfinding (Cunning Passive)
           0,
    },
    // Specialization abilities for Cunning
    {
      264656, // Pathfinding (Cunning Passive)
       53271, // Master's Call (Cunning Ability)
      264662, // Endurance Training (Tenacity Passive)
      264663, // Predator's Thirst (Ferocity Passive)
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Warrior
  {
    // Specialization abilities for Arms
    {
       12294, // Mortal Strike
       86101, // Plate Specialization
      137049, // Arms Warrior
       97462, // Rallying Cry
      163201, // Execute
      167105, // Colossus Smash
        1680, // Whirlwind
      162698, // Stat Negation Aura - Strength DPS
      184783, // Tactician
        1464, // Slam
       34428, // Victory Rush
       18499, // Berserker Rage
        1715, // Hamstring
        5246, // Intimidating Shout
      118038, // Die by the Sword
      227847, // Bladestorm
      262111, // Mastery: Deep Wounds
      260708, // Sweeping Strikes
        7384, // Overpower
      262304, // Deep Wounds
      231830, // Execute (Rank 2)
      279423, // Seasoned Soldier
           0,
    },
    // Specialization abilities for Fury
    {
       23881, // Bloodthirst
       85288, // Raging Blow
       46917, // Titan's Grip
       86110, // Plate Specialization
       12950, // Whirlwind (Rank 2)
       76856, // Mastery: Unshackled Fury
      137050, // Fury Warrior
       97462, // Rallying Cry
        1719, // Recklessness
        5308, // Execute
       12323, // Piercing Howl
      162698, // Stat Negation Aura - Strength DPS
      184361, // Enrage
      184364, // Enraged Regeneration
      184367, // Rampage
       18499, // Berserker Rage
      190411, // Whirlwind
      209694, // Rampage!
        5246, // Intimidating Shout
      231842, // Dual Wield
      231827, // Execute (Rank 2)
       34428, // Victory Rush
           0,
    },
    // Specialization abilities for Protection
    {
       23922, // Shield Slam
        6572, // Revenge
       12975, // Last Stand
       86535, // Plate Specialization
       76857, // Mastery: Critical Block
      115768, // Deep Wounds
          71, // Vanguard
        1160, // Demoralizing Shout
      137048, // Protection Warrior
        6343, // Thunder Clap
         871, // Shield Wall
      162702, // Stat Negation Aura - Strength Tank
       20243, // Devastate
       23920, // Spell Reflection
        2565, // Shield Block
      198304, // Intercept
       34428, // Victory Rush
       18499, // Berserker Rage
        5301, // Revenge Trigger
      231834, // Shield Slam (Rank 2)
      231847, // Shield Block (Rank 2)
       97462, // Rallying Cry
      107574, // Avatar
       46968, // Shockwave
        5246, // Intimidating Shout
      190456, // Ignore Pain
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Paladin
  {
    // Specialization abilities for Holy
    {
       20473, // Holy Shock
       53563, // Beacon of Light
       53651, // Light's Beacon
       53576, // Infusion of Light
       86103, // Plate Specialization (Holy, Passive)
       82326, // Holy Light
       85222, // Light of Dawn
       31821, // Aura Mastery
      137029, // Holy Paladin
      148040, // Sacred Shield Overridde (Holy)
      162701, // Stat Negation Aura - Intellect Healer
       19750, // Flash of Light
        4987, // Cleanse
         498, // Divine Protection
        1022, // Blessing of Protection
        6940, // Blessing of Sacrifice
      183998, // Light of the Martyr
      183997, // Mastery: Lightbringer
       26573, // Consecration
         853, // Hammer of Justice
      212056, // Absolution
      219562, // Light of the Martyr
      231642, // Beacon of Light (Rank 2)
      231667, // Crusader Strike (Rank 2)
      272906, // Holy Shock (Rank 2)
       31884, // Avenging Wrath
      200327, // Blessing of Sacrifice (Rank 2)
      275773, // Judgment
      231644, // Judgment (Rank 2)
      289941, // Holy Shock
           0,
    },
    // Specialization abilities for Protection
    {
       31935, // Avenger's Shield
      105424, // Judgments of the Wise
       26573, // Consecration
       53600, // Shield of the Righteous
       85043, // Grand Crusader
       86102, // Plate Specialization (Protection, Passive)
      105805, // Sanctuary
       31850, // Ardent Defender
       86659, // Guardian of Ancient Kings
       76671, // Mastery: Divine Bulwark
      137028, // Protection Paladin
      161800, // Riposte
       53595, // Hammer of the Righteous
      162702, // Stat Negation Aura - Strength Tank
       25780, // Righteous Fury
         498, // Divine Protection
        1022, // Blessing of Protection
        6940, // Blessing of Sacrifice
       96231, // Rebuke
      184092, // Light of the Protector
         853, // Hammer of Justice
      213644, // Cleanse Toxins
       19750, // Flash of Light
       31884, // Avenging Wrath
      231665, // Avenger's Shield (Rank 2)
      229976, // Blessed Hammer
      275779, // Judgment
      231657, // Judgment (Rank 2)
           0,
    },
    // Specialization abilities for Retribution
    {
       85256, // Templar's Verdict
       53385, // Divine Storm
       86539, // Plate Specialization (Retribution, Passive)
       31884, // Avenging Wrath
      137027, // Retribution Paladin
      162698, // Stat Negation Aura - Strength DPS
       19750, // Flash of Light
      183218, // Hand of Hindrance
      183435, // Retribution
       96231, // Rebuke
        1022, // Blessing of Protection
      184575, // Blade of Justice
         853, // Hammer of Justice
      184662, // Shield of Vengeance
      203538, // Greater Blessing of Kings
      203539, // Greater Blessing of Wisdom
      213644, // Cleanse Toxins
      231667, // Crusader Strike (Rank 2)
      267316, // Mastery: Hand of Light
       20271, // Judgment
      231663, // Judgment (Rank 2)
      267344, // Art of War
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Hunter
  {
    // Specialization abilities for Beast Mastery
    {
       34026, // Kill Command
      115939, // Beast Cleave
       19574, // Bestial Wrath
       56315, // Kindred Spirits
       53270, // Exotic Beasts
       76657, // Mastery: Master of Beasts
      137015, // Beast Mastery Hunter
        5116, // Concussive Shot
      147362, // Counter Shot
        2643, // Multi-Shot
       34477, // Misdirection
      162697, // Stat Negation Aura - Agility DPS
      193455, // Cobra Shot
      185789, // Wild Call
      193530, // Aspect of the Wild
      109304, // Exhilaration
         781, // Disengage
      231548, // Bestial Wrath (Rank 2)
      231546, // Exhilaration (Rank 2)
      231549, // Disengage (Rank 2)
      187698, // Tar Trap
      187650, // Freezing Trap
      217200, // Barbed Shot
      262837, // Cobra Shot (Rank 2)
      262838, // Cobra Shot (Rank 3)
       19577, // Intimidation
           0,
    },
    // Specialization abilities for Marksmanship
    {
       19434, // Aimed Shot
      137016, // Marksmanship Hunter
        5116, // Concussive Shot
      147362, // Counter Shot
       34477, // Misdirection
      185358, // Arcane Shot
      186387, // Bursting Shot
      162697, // Stat Negation Aura - Agility DPS
      193468, // Mastery: Sniper Training (Mastery)
         781, // Disengage
      231549, // Disengage (Rank 2)
      109304, // Exhilaration
      231546, // Exhilaration (Rank 2)
      187650, // Freezing Trap
      187698, // Tar Trap
      257044, // Rapid Fire
      257620, // Multi-Shot
       56641, // Steady Shot
      257621, // Trick Shots
      155228, // Lone Wolf
      260240, // Precise Shots
      288613, // Trueshot
           0,
    },
    // Specialization abilities for Survival
    {
      137017, // Survival Hunter
      186270, // Raptor Strike
      187650, // Freezing Trap
      187698, // Tar Trap
      187707, // Muzzle
      190925, // Harpoon
      162697, // Stat Negation Aura - Agility DPS
      195645, // Wing Clip
      109304, // Exhilaration
      231550, // Harpoon (Rank 2)
      231546, // Exhilaration (Rank 2)
      259489, // Kill Command
      259495, // Wildfire Bomb
      259491, // Serpent Sting
         781, // Disengage
      262839, // Raptor Strike (Rank 2)
      263135, // Mastery: Spirit Bond
      263186, // Kill Command (Rank 2)
       34477, // Misdirection
      266779, // Coordinated Assault
      186289, // Aspect of the Eagle
       19577, // Intimidation
      231549, // Disengage (Rank 2)
      187708, // Carve
      294029, // Carve (Rank 2)
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Rogue
  {
    // Specialization abilities for Assassination
    {
       14117, // Improved Poisons
        1329, // Mutilate
       32645, // Envenom
       14190, // Seal Fate
       79134, // Venomous Wounds
       51667, // Cut to the Chase
       79140, // Vendetta
       76803, // Mastery: Potent Assassin
      137037, // Assassination Rogue
        1943, // Rupture
       51723, // Fan of Knives
         703, // Garrote
      185565, // Poisoned Knife
      162697, // Stat Negation Aura - Agility DPS
        3408, // Crippling Poison
        2823, // Deadly Poison
        8679, // Wound Poison
       36554, // Shadowstep
        1725, // Distract
        1833, // Cheap Shot
         921, // Pick Pocket
        6770, // Sap
        1784, // Stealth
        1856, // Vanish
        5277, // Evasion
         408, // Kidney Shot
      231691, // Sprint (Rank 2)
        1752, // Sinister Strike
      196819, // Eviscerate
      231719, // Garrote (Rank 2)
        2094, // Blind
      279877, // Sinister Strike (Rank 2)
           0,
    },
    // Specialization abilities for Outlaw
    {
       61329, // Combat Potency
       13877, // Blade Flurry
       35551, // Combat Potency
       13750, // Adrenaline Rush
       76806, // Mastery: Main Gauche
      137036, // Outlaw Rogue
       14161, // Ruthlessness
        8676, // Ambush
      185763, // Pistol Shot
      162697, // Stat Negation Aura - Agility DPS
      193315, // Sinister Strike
        1776, // Gouge
        2098, // Dispatch
        1784, // Stealth
         921, // Pick Pocket
        1725, // Distract
        1856, // Vanish
      199736, // Find Treasure (Racial)
      199754, // Riposte
      199804, // Between the Eyes
        2094, // Blind
        1833, // Cheap Shot
      193316, // Roll the Bones
        6770, // Sap
      231691, // Sprint (Rank 2)
      235484, // Between the Eyes (Rank 2)
       79096, // Restless Blades
      195457, // Grappling Hook
      279876, // Sinister Strike (Rank 2)
           0,
    },
    // Specialization abilities for Subtlety
    {
          53, // Backstab
       79152, // Energetic Recovery
       76808, // Mastery: Executioner
      137035, // Subtlety Rogue
      114014, // Shuriken Toss
      185314, // Deepening Shadows
      185438, // Shadowstrike
      162697, // Stat Negation Aura - Agility DPS
      195452, // Nightblade
        1725, // Distract
        1833, // Cheap Shot
         921, // Pick Pocket
        6770, // Sap
        1784, // Stealth
        1856, // Vanish
      196819, // Eviscerate
      196912, // Shadow Techniques
      121471, // Shadow Blades
      197835, // Shuriken Storm
       36554, // Shadowstep
      185313, // Shadow Dance
        2094, // Blind
      212283, // Symbols of Death
        5277, // Evasion
         408, // Kidney Shot
       58423, // Relentless Strikes (Passive)
      231691, // Sprint (Rank 2)
      231716, // Eviscerate (Rank 2)
      231718, // Shadowstrike (Rank 2)
      245751, // Sprint (Rank 2)
      277950, // Shadow's Grasp
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Priest
  {
    // Specialization abilities for Discipline
    {
       47540, // Penance
       33206, // Pain Suppression
       62618, // Power Word: Barrier
         527, // Purify
       81749, // Atonement
      137032, // Discipline Priest
       47536, // Rapture
      162701, // Stat Negation Aura - Intellect Healer
          17, // Power Word: Shield
       73325, // Leap of Faith
        1706, // Levitate
        2096, // Mind Vision
      186263, // Shadow Mend
         589, // Shadow Word: Pain
       34433, // Shadowfiend
      186440, // Shadow Mend
      194509, // Power Word: Radiance
        8122, // Psychic Scream
       45243, // Focused Will
      212036, // Mass Resurrection
        2061, // Flash Heal
      231682, // Smite (Rank 2)
      262861, // Smite (Rank 2)
       19236, // Desperate Prayer
      271534, // Mastery: Grace
      132157, // Holy Nova
      198068, // Power of the Dark Side
           0,
    },
    // Specialization abilities for Holy
    {
       88625, // Holy Word: Chastise
       20711, // Spirit of Redemption
        2060, // Heal
         596, // Prayer of Healing
       34861, // Holy Word: Sanctify
       47788, // Guardian Spirit
       64843, // Divine Hymn
         527, // Purify
       63733, // Holy Words (Rank 2)
       77485, // Mastery: Echo of Light
       14914, // Holy Fire
      137031, // Holy Priest
       45243, // Focused Will
      162701, // Stat Negation Aura - Intellect Healer
         139, // Renew
       73325, // Leap of Faith
       33076, // Prayer of Mending
        1706, // Levitate
        2050, // Holy Word: Serenity
      132157, // Holy Nova
        2061, // Flash Heal
      212036, // Mass Resurrection
      231687, // Holy Fire (Rank 2)
       19236, // Desperate Prayer
      262861, // Smite (Rank 2)
        8122, // Psychic Scream
       64901, // Symbol of Hope
        2096, // Mind Vision
      285485, // Focused Will (Rank 2)
           0,
    },
    // Specialization abilities for Shadow
    {
       15407, // Mind Flay
        8092, // Mind Blast
       15487, // Silence
       34914, // Vampiric Touch
       78203, // Shadowy Apparitions
       47585, // Dispersion
       15286, // Vampiric Embrace
       77486, // Mastery: Madness
      137033, // Shadow Priest
      185916, // Voidform
      186263, // Shadow Mend
      186440, // Shadow Mend
        2096, // Mind Vision
         589, // Shadow Word: Pain
       34433, // Shadowfiend
        8122, // Psychic Scream
          17, // Power Word: Shield
        1706, // Levitate
      213634, // Purify Disease
      228260, // Void Eruption
      228266, // Void Bolt
      228264, // Voidform
      231688, // Void Bolt (Rank 2)
        2061, // Flash Heal
      232698, // Shadowform
      162699, // Stat Negation Aura - Intellect DPS
       73325, // Leap of Faith
       48045, // Mind Sear
      190719, // Shadow Word: Death
       45243, // Focused Will
      280752, // Hallucinations
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Death Knight
  {
    // Specialization abilities for Blood
    {
       86537, // Plate Specialization (Passive)
       49028, // Dancing Rune Weapon
       55233, // Vampiric Blood
       77513, // Mastery: Blood Shield
       81136, // Crimson Scourge
      137008, // Blood Death Knight
      161797, // Riposte
      162702, // Stat Negation Aura - Strength Tank
       48263, // Veteran of the Third War
       43265, // Death and Decay
       50842, // Blood Boil
      195182, // Marrowrend
      195292, // Death's Caress
      108199, // Gorefiend's Grasp
      206930, // Heart Strike
      221562, // Asphyxiate
       48792, // Icebound Fortitude
           0,
    },
    // Specialization abilities for Frost
    {
       49143, // Frost Strike
       49184, // Howling Blast
       86113, // Plate Specialization (Passive)
       49020, // Obliterate
       51128, // Killing Machine
       59057, // Rime
       77514, // Mastery: Frozen Heart
      137006, // Frost Death Knight
         674, // Dual Wield (Passive)
      162698, // Stat Negation Aura - Strength DPS
       47568, // Empower Rune Weapon
       48792, // Icebound Fortitude
       45524, // Chains of Ice
      178819, // Dark Succor
      195621, // Frost Fever
      196770, // Remorseless Winter
       81229, // Runic Empowerment
      278223, // Death Strike (Rank 2)
       51271, // Pillar of Frost
       48263, // Veteran of the Third War
           0,
    },
    // Specialization abilities for Unholy
    {
       86536, // Plate Specialization (Passive)
       55090, // Scourge Strike
       85948, // Festering Strike
       49530, // Sudden Doom
       63560, // Dark Transformation
       77515, // Mastery: Dreadblade
      137007, // Unholy Death Knight
       46584, // Raise Dead
      162698, // Stat Negation Aura - Strength DPS
       42650, // Army of the Dead
       77575, // Outbreak
       43265, // Death and Decay
       45524, // Chains of Ice
      178819, // Dark Succor
       48792, // Icebound Fortitude
       47541, // Death Coil
      197147, // Festering Wound
       51462, // Runic Corruption
      275699, // Apocalypse
      278223, // Death Strike (Rank 2)
       48263, // Veteran of the Third War
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Shaman
  {
    // Specialization abilities for Elemental
    {
       60188, // Elemental Fury
       51490, // Thunderstorm
       51505, // Lava Burst
       77756, // Lava Surge
       86108, // Mail Specialization (Passive)
      137040, // Elemental Shaman
      162699, // Stat Negation Aura - Intellect DPS
      198103, // Earth Elemental
        8042, // Earth Shock
       61882, // Earthquake
      187828, // Maelstrom
      190488, // Maelstrom Controller
      188196, // Lightning Bolt
      190493, // Fulmination
      188443, // Chain Lightning
      188389, // Flame Shock
       57994, // Wind Shear
      168534, // Mastery: Elemental Overload
      196840, // Frost Shock
      198067, // Fire Elemental
       51886, // Cleanse Spirit
        8004, // Healing Surge
      231722, // Chain Lightning (Rank 2)
      280609, // Mastery: Elemental Overload (Rank 2)
      231721, // Lava Burst (Rank 2)
           0,
    },
    // Specialization abilities for Enhancement
    {
       86629, // Dual Wield
       86099, // Mail Specialization (Passive)
       51533, // Feral Spirit
       58875, // Spirit Walk
       77223, // Mastery: Enhanced Elements
      137041, // Enhancement Shaman
      157444, // Critical Strikes (Passive)
      162697, // Stat Negation Aura - Agility DPS
      187837, // Lightning Bolt
      188070, // Healing Surge
       57994, // Wind Shear
      187880, // Maelstrom Weapon
       17364, // Stormstrike
      193786, // Rockbiter
      193796, // Flametongue
       60103, // Lava Lash
      187874, // Crash Lightning
      190899, // Healing Surge (Rank 2)
      196834, // Frostbrand
      201845, // Stormbringer
       33757, // Windfury
       51886, // Cleanse Spirit
      231723, // Feral Spirit (Rank 2)
      198103, // Earth Elemental
           0,
    },
    // Specialization abilities for Restoration
    {
       61295, // Riptide
       77130, // Purify Spirit
       16196, // Resurgence
       86100, // Mail Specialization (Passive)
       51564, // Tidal Waves
       77472, // Healing Wave
       98008, // Spirit Link Totem
       77226, // Mastery: Deep Healing
      137039, // Restoration Shaman
      108280, // Healing Tide Totem
       79206, // Spiritwalker's Grace
      162701, // Stat Negation Aura - Intellect Healer
        1064, // Chain Heal
       73920, // Healing Rain
      188838, // Flame Shock
        5394, // Healing Stream Totem
         421, // Chain Lightning
        8004, // Healing Surge
       57994, // Wind Shear
         403, // Lightning Bolt
       51505, // Lava Burst
       77756, // Lava Surge
      212048, // Ancestral Vision
      231780, // Chain Heal (Rank 2)
      231785, // Tidal Waves (Rank 2)
      231725, // Riptide (Rank 2)
      231721, // Lava Burst (Rank 2)
      198103, // Earth Elemental
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Mage
  {
    // Specialization abilities for Arcane
    {
       30451, // Arcane Blast
        5143, // Arcane Missiles
       79684, // Clearcasting
       44425, // Arcane Barrage
       31589, // Slow
       12042, // Arcane Power
      137021, // Arcane Mage
        1449, // Arcane Explosion
       12051, // Evocation
      190427, // Arcane Charge
      190740, // Mastery: Savant
      162699, // Stat Negation Aura - Intellect DPS
      110959, // Greater Invisibility
      231564, // Arcane Barrage (Rank 2)
      231565, // Evocation (Rank 2)
          66, // Invisibility
      235450, // Prismatic Barrier
      205025, // Presence of Mind
      195676, // Displacement
           0,
    },
    // Specialization abilities for Fire
    {
       11366, // Pyroblast
         133, // Fireball
       44448, // Pyroblast Clearcasting Driver
      108853, // Fire Blast
      117216, // Critical Mass
       12846, // Mastery: Ignite
      137019, // Fire Mage
        2948, // Scorch
        2120, // Flamestrike
      157642, // Enhanced Pyrotechnics
      162699, // Stat Negation Aura - Intellect DPS
      190319, // Combustion
      195283, // Hot Streak
       31661, // Dragon's Breath
          66, // Invisibility
      231567, // Fire Blast (Rank 3)
      231568, // Fire Blast (Rank 2)
      231630, // Critical Mass (Rank 2)
       86949, // Cauterize
      235313, // Blazing Barrier
           0,
    },
    // Specialization abilities for Frost
    {
       31687, // Summon Water Elemental
         116, // Frostbolt
      112965, // Fingers of Frost
       12472, // Icy Veins
       84714, // Frozen Orb
       76613, // Mastery: Icicles
      137020, // Frost Mage
       12982, // Shatter
       30455, // Ice Lance
      190356, // Blizzard
      190447, // Brain Freeze
      162699, // Stat Negation Aura - Intellect DPS
         120, // Cone of Cold
          66, // Invisibility
       44614, // Flurry
      231584, // Brain Freeze (Rank 2)
      231582, // Shatter (Rank 2)
      231596, // Freeze (Pet)
      108853, // Fire Blast
      235219, // Cold Snap
       11426, // Ice Barrier
      236662, // Blizzard (Rank 2)
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Warlock
  {
    // Specialization abilities for Affliction
    {
       30108, // Unstable Affliction
         980, // Agony
       27243, // Seed of Corruption
       77215, // Mastery: Potent Afflictions
      137043, // Affliction Warlock
      162699, // Stat Negation Aura - Intellect DPS
         172, // Corruption
      231791, // Unstable Affliction (Rank 2)
      231792, // Agony (Rank 2)
      231811, // Soulstone (Rank 2)
      232670, // Shadow Bolt
      234153, // Drain Life
       30283, // Shadowfury
      205180, // Summon Darkglare
           0,
    },
    // Specialization abilities for Demonology
    {
      105174, // Hand of Gul'dan
       77219, // Mastery: Master Demonologist
       30146, // Summon Felguard (Summon)
      137044, // Demonology Warlock
      162699, // Stat Negation Aura - Intellect DPS
      104316, // Call Dreadstalkers
      108415, // Soul Link
         686, // Shadow Bolt
      231811, // Soulstone (Rank 2)
      234153, // Drain Life
       30283, // Shadowfury
      264178, // Demonbolt
      267102, // Demonic Core
      265187, // Summon Demonic Tyrant
      196277, // Implosion
           0,
    },
    // Specialization abilities for Destruction
    {
       17962, // Conflagrate
         348, // Immolate
       80240, // Havoc
       29722, // Incinerate
       77220, // Mastery: Chaotic Energies
      116858, // Chaos Bolt
        5740, // Rain of Fire
      137046, // Destruction Warlock
      162699, // Stat Negation Aura - Intellect DPS
      193541, // Immolate
         686, // Shadow Bolt
      231793, // Conflagrate (Rank 2)
      231811, // Soulstone (Rank 2)
      234153, // Drain Life
        1122, // Summon Infernal (Guardian)
       30283, // Shadowfury
      196406, // Backdraft
      157903, // Warlock
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Monk
  {
    // Specialization abilities for Brewmaster
    {
      115181, // Breath of Fire
      115308, // Ironskin Brew
      117906, // Mastery: Elusive Brawler
      115069, // Stagger
      119582, // Purifying Brew
      120225, // Leather Specialization (Passive)
      121253, // Keg Smash
      124502, // Gift of the Ox
      137023, // Brewmaster Monk
      157445, // Critical Strikes (Passive)
      115176, // Zen Meditation
      162700, // Stat Negation Aura - Agility Tank
      115203, // Fortifying Brew
      116705, // Spear Hand Strike
      205523, // Blackout Strike
      216519, // Celestial Fortune
      218164, // Detox
      115072, // Expel Harm
      231602, // Vivify (Rank 2)
      245013, // Brewmaster's Balance
      119381, // Leg Sweep
      116670, // Vivify
           0,
    },
    // Specialization abilities for Mistweaver
    {
      115151, // Renewing Mist
      115310, // Revival
      116645, // Teachings of the Monastery
      116680, // Thunder Focus Tea
      117907, // Mastery: Gust of Mists
      120224, // Leather Specialization (Passive)
      116849, // Life Cocoon
      124682, // Enveloping Mist
      137024, // Mistweaver Monk
      162701, // Stat Negation Aura - Intellect Healer
      191837, // Essence Font
      115450, // Detox
      107428, // Rising Sun Kick
      101546, // Spinning Crane Kick
      116670, // Vivify
      212051, // Reawaken
      231605, // Enveloping Mist (Rank 2)
      231633, // Essence Font (Rank 2)
      231876, // Thunder Focus Tea (Rank 2)
      262840, // Rising Sun Kick (Rank 2)
      119381, // Leg Sweep
      243435, // Fortifying Brew
      115175, // Soothing Mist
      274586, // Vivify (Rank 2)
      281231, // Renewing Mist (Rank 2)
           0,
    },
    // Specialization abilities for Windwalker
    {
      113656, // Fists of Fury
      101545, // Flying Serpent Kick
      107428, // Rising Sun Kick
      116092, // Afterlife
      120227, // Leather Specialization (Passive)
      115636, // Mastery: Combo Strikes
      124146, // Dual Wield
      122470, // Touch of Karma
      128595, // Combat Conditioning
      137025, // Windwalker Monk
      137384, // Combo Breaker
      137639, // Storm, Earth, and Fire
      116095, // Disable
      157445, // Critical Strikes (Passive)
      162697, // Stat Negation Aura - Agility DPS
      115080, // Touch of Death
      101546, // Spinning Crane Kick
      116705, // Spear Hand Strike
      157411, // Windwalking
      218164, // Detox
      220357, // Cyclone Strikes
      231602, // Vivify (Rank 2)
      231627, // Storm, Earth, and Fire (Rank 2)
      119381, // Leg Sweep
      261916, // Blackout Kick (Rank 2)
      261917, // Blackout Kick (Rank 3)
      262840, // Rising Sun Kick (Rank 2)
      271232, // Touch of Death Amplifier
      116670, // Vivify
           0,
    },
    {
           0,
    },
  },
  // Specialization abilities for Druid
  {
    // Specialization abilities for Balance
    {
       24858, // Moonkin Form
       78675, // Solar Beam
       86104, // Leather Specialization
       77492, // Mastery: Starlight
      137013, // Balance Druid
      162699, // Stat Negation Aura - Intellect DPS
       22812, // Barkskin
      190984, // Solar Wrath (Solar)
      191034, // Starfall
      194153, // Lunar Strike (Lunar)
      194223, // Celestial Alignment
       29166, // Innervate
       78674, // Starsurge
       93402, // Sunfire (Solar)
        5215, // Prowl
      197524, // Astral Influence
      197911, // Astral Power (Passive)
        2782, // Remove Corruption
      231021, // Starsurge (Rank 2)
      231042, // Moonkin Form (Rank 2)
      231050, // Sunfire (Rank 2)
      159456, // Travel Form (Rank 2)
      228545, // Shapeshift Form (Shapeshift)
        2908, // Soothe
      279619, // Eclipse
      279708, // Empowerments
           0,
    },
    // Specialization abilities for Feral
    {
      106733, // Feral Overrides Passive (Passive)
        5217, // Tiger's Fury
       16949, // Feral Instinct
        1079, // Rip
       16974, // Predatory Swiftness
       16864, // Omen of Clarity
       86097, // Leather Specialization
      106839, // Skull Bash
       77493, // Mastery: Razor Claws
       48484, // Infected Wounds
      137011, // Feral Druid
      157447, // Critical Strikes (Passive)
      162697, // Stat Negation Aura - Agility DPS
       22570, // Maim
        1822, // Rake
       61336, // Survival Instincts
       22568, // Ferocious Bite
        5215, // Prowl
      106898, // Stampeding Roar
      106832, // Thrash
      159286, // Primal Fury
      131768, // Feline Swiftness
      197692, // Feral Overrides Passive (Passive)
      106951, // Berserk
        2782, // Remove Corruption
      213764, // Swipe
      228545, // Shapeshift Form (Shapeshift)
      231052, // Rake (Rank 2)
      231055, // Tiger's Fury (Rank 2)
      231057, // Shred (Rank 3)
      231063, // Shred (Rank 2)
      159456, // Travel Form (Rank 2)
      231283, // Swipe (Rank 2)
        2908, // Soothe
    },
    // Specialization abilities for Guardian
    {
      106734, // Guardian Overrides Passive (Passive)
       16931, // Thick Hide
       86096, // Leather Specialization
      106839, // Skull Bash
      155783, // Mastery: Nature's Guardian
      159195, // Mastery: Nature's Guardian
      137010, // Guardian Druid
      157447, // Critical Strikes (Passive)
      162700, // Stat Negation Aura - Agility Tank
       22812, // Barkskin
        6807, // Maul
       61336, // Survival Instincts
       22842, // Frenzied Regeneration
      106898, // Stampeding Roar
      192081, // Ironfur
        5215, // Prowl
          99, // Incapacitating Roar
      210706, // Gore
        2782, // Remove Corruption
      213764, // Swipe
      106832, // Thrash
      231064, // Mangle (Rank 2)
      231065, // Lightning Reflexes
      231070, // Ironfur (Rank 2)
      159456, // Travel Form (Rank 2)
      228545, // Shapeshift Form (Shapeshift)
      270100, // Bear Form (Rank 2)
      273048, // Frenzied Regeneration (Rank 2)
        2908, // Soothe
      288826, // Stampeding Roar (Rank 2)
           0,
    },
    // Specialization abilities for Restoration
    {
       18562, // Swiftmend
       88423, // Nature's Cure
      102342, // Ironbark
      113043, // Omen of Clarity
       86093, // Leather Specialization
       48438, // Wild Growth
       33763, // Lifebloom
       77495, // Mastery: Harmony
      137012, // Restoration Druid
      145205, // Efflorescence
         740, // Tranquility
      162701, // Stat Negation Aura - Intellect Healer
         774, // Rejuvenation
       22812, // Barkskin
      145108, // Ysera's Gift
        5215, // Prowl
        5176, // Solar Wrath (Solar)
      102793, // Ursol's Vortex
       93402, // Sunfire (Solar)
       29166, // Innervate
      212040, // Revitalize
      231040, // Rejuvenation (Rank 2)
      231050, // Sunfire (Rank 2)
      159456, // Travel Form (Rank 2)
        2908, // Soothe
           0,
    },
  },
  // Specialization abilities for Demon Hunter
  {
    // Specialization abilities for Havoc
    {
      178940, // Shattered Souls
      212612, // Havoc Demon Hunter
      185164, // Mastery: Demonic Presence
      178976, // Leather Specialization (Passive)
      196718, // Darkness
      162243, // Demon's Bite
      162697, // Stat Negation Aura - Agility DPS
      197125, // Chaos Strike (Passive)
      278386, // Demonic Wards
      281854, // Torment
           0,
    },
    // Specialization abilities for Vengeance
    {
      189926, // Increased Threat
      185245, // Torment
      187827, // Metamorphosis
      204021, // Fiery Brand
      178740, // Immolation Aura
      189110, // Infernal Strike
      203513, // Demonic Wards
      203782, // Shear
      203720, // Demon Spikes
      202137, // Sigil of Silence
      204157, // Throw Glaive
      204596, // Sigil of Flame
      207197, // Riposte
      204254, // Shattered Souls
      212613, // Vengeance Demon Hunter
      203747, // Mastery: Fel Blood
      226359, // Leather Specialization (Passive)
      228477, // Soul Cleave
      207684, // Sigil of Misery
      162700, // Stat Negation Aura - Agility Tank
      203783, // Shear
      281242, // Chaos Brand
           0,
    },
    {
           0,
    },
    {
           0,
    },
  },
};
#define CLASS_MASTERY_ABILITY_SIZE (1)

// Class mastery abilities, wow build 8.1.5.29705
static unsigned __class_mastery_ability_data[MAX_CLASS][MAX_SPECS_PER_CLASS][CLASS_MASTERY_ABILITY_SIZE] = {
  {
    {
          0,
    },
    {
          0,
    },
    {
          0,
    },
    {
          0,
    },
  },
  // Class mastery abilities for Warrior
  {
    // Masteries for Arms specialization
    {
     262111, // Mastery: Deep Wounds
    },
    // Masteries for Fury specialization
    {
      76856, // Mastery: Unshackled Fury
    },
    // Masteries for Protection specialization
    {
      76857, // Mastery: Critical Block
    },
    {
          0,
    },
  },
  // Class mastery abilities for Paladin
  {
    // Masteries for Holy specialization
    {
     183997, // Mastery: Lightbringer
    },
    // Masteries for Protection specialization
    {
      76671, // Mastery: Divine Bulwark
    },
    // Masteries for Retribution specialization
    {
     267316, // Mastery: Hand of Light
    },
    {
          0,
    },
  },
  // Class mastery abilities for Hunter
  {
    // Masteries for Beast Mastery specialization
    {
      76657, // Mastery: Master of Beasts
    },
    // Masteries for Marksmanship specialization
    {
     193468, // Mastery: Sniper Training
    },
    // Masteries for Survival specialization
    {
     263135, // Mastery: Spirit Bond
    },
    {
          0,
    },
  },
  // Class mastery abilities for Rogue
  {
    // Masteries for Assassination specialization
    {
      76803, // Mastery: Potent Assassin
    },
    // Masteries for Outlaw specialization
    {
      76806, // Mastery: Main Gauche
    },
    // Masteries for Subtlety specialization
    {
      76808, // Mastery: Executioner
    },
    {
          0,
    },
  },
  // Class mastery abilities for Priest
  {
    // Masteries for Discipline specialization
    {
     271534, // Mastery: Grace
    },
    // Masteries for Holy specialization
    {
      77485, // Mastery: Echo of Light
    },
    // Masteries for Shadow specialization
    {
      77486, // Mastery: Madness
    },
    {
          0,
    },
  },
  // Class mastery abilities for Death Knight
  {
    // Masteries for Blood specialization
    {
      77513, // Mastery: Blood Shield
    },
    // Masteries for Frost specialization
    {
      77514, // Mastery: Frozen Heart
    },
    // Masteries for Unholy specialization
    {
      77515, // Mastery: Dreadblade
    },
    {
          0,
    },
  },
  // Class mastery abilities for Shaman
  {
    // Masteries for Elemental specialization
    {
     168534, // Mastery: Elemental Overload
    },
    // Masteries for Enhancement specialization
    {
      77223, // Mastery: Enhanced Elements
    },
    // Masteries for Restoration specialization
    {
      77226, // Mastery: Deep Healing
    },
    {
          0,
    },
  },
  // Class mastery abilities for Mage
  {
    // Masteries for Arcane specialization
    {
     190740, // Mastery: Savant
    },
    // Masteries for Fire specialization
    {
      12846, // Mastery: Ignite
    },
    // Masteries for Frost specialization
    {
      76613, // Mastery: Icicles
    },
    {
          0,
    },
  },
  // Class mastery abilities for Warlock
  {
    // Masteries for Affliction specialization
    {
      77215, // Mastery: Potent Afflictions
    },
    // Masteries for Demonology specialization
    {
      77219, // Mastery: Master Demonologist
    },
    // Masteries for Destruction specialization
    {
      77220, // Mastery: Chaotic Energies
    },
    {
          0,
    },
  },
  // Class mastery abilities for Monk
  {
    // Masteries for Brewmaster specialization
    {
     117906, // Mastery: Elusive Brawler
    },
    // Masteries for Mistweaver specialization
    {
     117907, // Mastery: Gust of Mists
    },
    // Masteries for Windwalker specialization
    {
     115636, // Mastery: Combo Strikes
    },
    {
          0,
    },
  },
  // Class mastery abilities for Druid
  {
    // Masteries for Balance specialization
    {
      77492, // Mastery: Starlight
    },
    // Masteries for Feral specialization
    {
      77493, // Mastery: Razor Claws
    },
    // Masteries for Guardian specialization
    {
     155783, // Mastery: Nature's Guardian
    },
    // Masteries for Restoration specialization
    {
      77495, // Mastery: Harmony
    },
  },
  // Class mastery abilities for Demon Hunter
  {
    // Masteries for Havoc specialization
    {
     185164, // Mastery: Demonic Presence
    },
    // Masteries for Vengeance specialization
    {
     203747, // Mastery: Fel Blood
    },
    {
          0,
    },
    {
          0,
    },
  },
};
#define RACE_ABILITY_SIZE (5)

#ifndef MAX_RACE
#define MAX_RACE (33)
#endif

// Racial abilities, wow build 8.1.5.29705
static unsigned __race_ability_data[MAX_RACE][MAX_CLASS][RACE_ABILITY_SIZE] = {
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Human
  {
    // Generic racial abilities
    {
      59752, // Every Man for Himself
      20598, // The Human Spirit
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Orc
  {
    // Generic racial abilities
    {
      20573, // Hardiness
          0,
    },
    // Racial abilities for Warrior class
    {
      20572, // Blood Fury
      21563, // Command
          0,
    },
    // Racial abilities for Paladin class
    {
      21563, // Command
          0,
    },
    // Racial abilities for Hunter class
    {
      20572, // Blood Fury
      21563, // Command
          0,
    },
    // Racial abilities for Rogue class
    {
      20572, // Blood Fury
      21563, // Command
          0,
    },
    // Racial abilities for Priest class
    {
      21563, // Command
          0,
    },
    // Racial abilities for Death Knight class
    {
      20572, // Blood Fury
      21563, // Command
          0,
    },
    // Racial abilities for Shaman class
    {
      33697, // Blood Fury
      21563, // Command
          0,
    },
    // Racial abilities for Mage class
    {
      33702, // Blood Fury
      21563, // Command
          0,
    },
    // Racial abilities for Warlock class
    {
      33702, // Blood Fury
      21563, // Command
          0,
    },
    // Racial abilities for Monk class
    {
      33697, // Blood Fury
      21563, // Command
          0,
    },
    // Racial abilities for Druid class
    {
      21563, // Command
          0,
    },
    // Racial abilities for Demon Hunter class
    {
      21563, // Command
          0,
    },
  },
  // Racial abilities for Dwarf
  {
    // Generic racial abilities
    {
      92682, // Explorer
      20596, // Frost Resistance
      59224, // Might of the Mountain
      20594, // Stoneform
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Night Elf
  {
    // Generic racial abilities
    {
      20583, // Nature Resistance
      20582, // Quickness
      58984, // Shadowmeld
      154748, // Touch of Elune
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    // Racial abilities for Rogue class
    {
      21009, // Elusiveness
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    // Racial abilities for Druid class
    {
      21009, // Elusiveness
          0,
    },
    {     0, },
  },
  // Racial abilities for Undead
  {
    // Generic racial abilities
    {
      20577, // Cannibalize
      20579, // Shadow Resistance
       5227, // Touch of the Grave
       7744, // Will of the Forsaken
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Tauren
  {
    // Generic racial abilities
    {
      154743, // Brawn
      20550, // Endurance
      20551, // Nature Resistance
      20549, // War Stomp
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Gnome
  {
    // Generic racial abilities
    {
      20592, // Arcane Resistance
      20589, // Escape Artist
      92680, // Nimble Fingers
          0,
    },
    // Racial abilities for Warrior class
    {
      154746, // Expansive Mind
          0,
    },
    // Racial abilities for Paladin class
    {
      20591, // Expansive Mind
          0,
    },
    // Racial abilities for Hunter class
    {
      227057, // Expansive Mind
          0,
    },
    // Racial abilities for Rogue class
    {
      154744, // Expansive Mind
          0,
    },
    // Racial abilities for Priest class
    {
      20591, // Expansive Mind
          0,
    },
    // Racial abilities for Death Knight class
    {
      154747, // Expansive Mind
          0,
    },
    // Racial abilities for Shaman class
    {
      20591, // Expansive Mind
          0,
    },
    // Racial abilities for Mage class
    {
      20591, // Expansive Mind
          0,
    },
    // Racial abilities for Warlock class
    {
      20591, // Expansive Mind
          0,
    },
    // Racial abilities for Monk class
    {
      20591, // Expansive Mind
      154744, // Expansive Mind
          0,
    },
    {     0, },
    {     0, },
  },
  // Racial abilities for Troll
  {
    // Generic racial abilities
    {
      20557, // Beast Slaying
      26297, // Berserking
      58943, // Da Voodoo Shuffle
      20555, // Regeneration
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Goblin
  {
    // Generic racial abilities
    {
      69041, // Rocket Barrage
      69070, // Rocket Jump
      69042, // Time is Money
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Blood Elf
  {
    // Generic racial abilities
    {
      154742, // Arcane Acuity
        822, // Arcane Resistance
          0,
    },
    // Racial abilities for Warrior class
    {
      69179, // Arcane Torrent
          0,
    },
    // Racial abilities for Paladin class
    {
      155145, // Arcane Torrent
          0,
    },
    // Racial abilities for Hunter class
    {
      80483, // Arcane Torrent
          0,
    },
    // Racial abilities for Rogue class
    {
      25046, // Arcane Torrent
          0,
    },
    // Racial abilities for Priest class
    {
      232633, // Arcane Torrent
          0,
    },
    // Racial abilities for Death Knight class
    {
      50613, // Arcane Torrent
          0,
    },
    {     0, },
    // Racial abilities for Mage class
    {
      28730, // Arcane Torrent
          0,
    },
    // Racial abilities for Warlock class
    {
      28730, // Arcane Torrent
          0,
    },
    // Racial abilities for Monk class
    {
      129597, // Arcane Torrent
          0,
    },
    {     0, },
    // Racial abilities for Demon Hunter class
    {
      202719, // Arcane Torrent
          0,
    },
  },
  // Racial abilities for Draenei
  {
    // Generic racial abilities
    {
       6562, // Heroic Presence
      59221, // Shadow Resistance
          0,
    },
    // Racial abilities for Warrior class
    {
      28880, // Gift of the Naaru
          0,
    },
    // Racial abilities for Paladin class
    {
      59542, // Gift of the Naaru
          0,
    },
    // Racial abilities for Hunter class
    {
      59543, // Gift of the Naaru
          0,
    },
    {     0, },
    // Racial abilities for Priest class
    {
      59544, // Gift of the Naaru
          0,
    },
    // Racial abilities for Death Knight class
    {
      59545, // Gift of the Naaru
          0,
    },
    // Racial abilities for Shaman class
    {
      59547, // Gift of the Naaru
          0,
    },
    // Racial abilities for Mage class
    {
      59548, // Gift of the Naaru
          0,
    },
    {     0, },
    // Racial abilities for Monk class
    {
      121093, // Gift of the Naaru
          0,
    },
    {     0, },
    {     0, },
  },
  // Racial abilities for Dark Iron Dwarf
  {
    // Generic racial abilities
    {
      265223, // Dungeon Delver
      265221, // Fireblood
      265224, // Forged in Flames
      265222, // Mass Production
      265225, // Mole Machine
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Mag'har Orc
  {
    // Generic racial abilities
    {
      274738, // Ancestral Call
      273216, // Open Skies
      273220, // Savage Blood
      273217, // Sympathetic Vigor
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Worgen
  {
    {     0, },
    // Racial abilities for Warrior class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    // Racial abilities for Paladin class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    // Racial abilities for Hunter class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    // Racial abilities for Rogue class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    // Racial abilities for Priest class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    // Racial abilities for Death Knight class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    // Racial abilities for Shaman class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    // Racial abilities for Mage class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    // Racial abilities for Warlock class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    {     0, },
    // Racial abilities for Druid class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
          0,
    },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Pandaren
  {
    // Generic racial abilities
    {
      107076, // Bouncy
      107072, // Epicurean
      107074, // Inner Peace
      107079, // Quaking Palm
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  {
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Nightborne
  {
    // Generic racial abilities
    {
      260364, // Arcane Pulse
      255664, // Arcane Resistance
      255661, // Cantrips
      255665, // Magical Affinity
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Highmountain Tauren
  {
    // Generic racial abilities
    {
      255654, // Bull Rush
      255658, // Mountaineer
      255655, // Pride of Ironhorn
      255659, // Rugged Tenacity
      255656, // Waste Not, Want Not
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Void Elf
  {
    // Generic racial abilities
    {
      255668, // Chill of Night
      255669, // Entropic Embrace
      255667, // Ethereal Connection
      255670, // Preternatural Calm
      256948, // Spatial Rift
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Lightforged Draenei
  {
    {     0, },
    // Racial abilities for Warrior class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Paladin class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Hunter class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Rogue class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Priest class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Death Knight class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Shaman class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Mage class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Warlock class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Monk class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Druid class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
    // Racial abilities for Demon Hunter class
    {
      255653, // Demonbane
      255651, // Holy Resistance
      255647, // Light's Judgment
      255652, // Light's Reckoning
          0,
    },
  },
  // Racial abilities for Zandalari Troll
  {
    // Generic racial abilities
    {
      291619, // City of Gold
      292752, // Embrace of the Loa
      292751, // Embrace of the Loa
      281954, // Pterrordax Swoop
      291944, // Regeneratin'
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
  // Racial abilities for Kul Tiran
  {
    // Generic racial abilities
    {
      291628, // Brush It Off
      291622, // Child of the Sea
      287712, // Haymaker
      291417, // Rime of the Ancient Mariner
          0,
    },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
    {     0, },
  },
};
#define SET_BONUS_DATA_SIZE (724)

// Set bonus data, wow build 8.1.5.29705
static item_set_bonus_t __set_bonus_data[SET_BONUS_DATA_SIZE] = {
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   2,   1,   71, 165636, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   2,   1,   72, 165639, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   2,   1,   73, 165641, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   4,   1,   73, 165670, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   4,   1,   72, 165670, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   4,   1,   71, 165670, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   2,   2,   70, 165886, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   2,   2,   66, 165905, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   2,   2,   65, 170860, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   4,   2,   70, 165895, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   4,   2,   66, 171427, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   4,   2,   65, 185571, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   2,   3,  255, 166005, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   2,   3,  253, 166005, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   2,   3,  254, 166005, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   4,   3,  253, 171478, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   4,   3,  254, 170884, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   4,   3,  255, 166012, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   2,   4,  259, 165995, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   2,   4,  261, 165995, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   2,   4,  260, 165995, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   4,   4,  259, 170883, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   4,   4,  261, 170877, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   4,   4,  260, 182303, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   2,   5,  256, 171124, { 127065, 127077, 127088, 127099, 127110,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   2,   5,  257, 171158, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   2,   5,  258, 171146, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   4,   5,  256, 171141, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   4,   5,  257, 171142, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   4,   5,  258, 171151, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   2,   6,  252, 166020, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   2,   6,  250, 166020, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   2,   6,  251, 166020, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   4,   6,  250, 171456, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   4,   6,  251, 166056, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   4,   6,  252, 166061, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   2,   7,  262, 171109, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   2,   7,  263, 166103, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   2,   7,  264, 171381, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   4,   7,  262, 166096, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   4,   7,  264, 166063, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   4,   7,  263, 171121, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   2,   8,   62, 171349, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   2,   8,   63, 165977, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   2,   8,   64, 180721, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   4,   8,   62, 171351, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   4,   8,   63, 171169, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   4,   8,   64, 180741, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   2,   9,  265, 171377, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   2,   9,  266, 171393, { 127066, 127080, 127091, 127102, 127113,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   2,   9,  267, 171383, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   4,   9,  265, 171379, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   4,   9,  266, 165967, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   4,   9,  267, 189209, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   2,  10,  268, 165691, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   2,  10,  270, 170799, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   2,  10,  269, 180743, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   4,  10,  270, 165697, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   4,  10,  268, 171445, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   4,  10,  269, 181742, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   2,  11,  102, 165701, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   2,  11,  103, 170848, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   2,  11,  104, 171408, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   2,  11,  105, 170853, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   4,  11,  103, 165698, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   4,  11,  104, 171489, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   4,  11,  102, 180717, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   4,  11,  105, 165707, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   1,   71, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   1,   72, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   1,   73, 179142, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   1,   71, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   1,   72, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   1,   73, 179143, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   2,   65, 179145, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   2,   66, 179142, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   2,   70, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   2,   65, 179146, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   2,   66, 179143, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   2,   70, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   3,  255, 179130, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   3,  253, 179130, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   3,  254, 179130, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   3,  255, 179131, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   3,  253, 179131, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   3,  254, 179131, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,   4,  259, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,   4,  261, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,   4,  260, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,   4,  259, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,   4,  261, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,   4,  260, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   5,  257, 179110, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   5,  258, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   5,  256, 179110, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   5,  257, 179111, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   5,  258, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   5,  256, 179111, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   6,  250, 179142, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   6,  251, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   6,  252, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   6,  250, 179143, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   6,  251, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   6,  252, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   7,  264, 179137, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   7,  262, 179133, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   7,  263, 179130, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   7,  264, 179138, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   7,  262, 179134, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   7,  263, 179131, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   8,   62, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   8,   63, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   8,   64, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   8,   62, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   8,   63, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   8,   64, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   9,  265, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   9,  266, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   9,  267, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   9,  265, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   9,  266, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   9,  267, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  10,  269, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  10,  270, 179121, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  10,  268, 179127, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  10,  269, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  10,  270, 179122, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  10,  268, 179128, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  11,  104, 179127, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  11,  105, 179121, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  11,  102, 179117, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  11,  103, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  11,  104, 179128, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  11,  105, 179122, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  11,  102, 179118, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  11,  103, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   2,   1,   72, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   2,   1,   73, 187142, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   2,   1,   71, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   4,   1,   72, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   4,   1,   73, 187143, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   4,   1,   71, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   2,   2,   65, 187140, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   2,   2,   70, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   2,   2,   66, 187142, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   4,   2,   65, 187155, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   4,   2,   70, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   4,   2,   66, 187143, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   2,   3,  255, 187137, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   2,   3,  254, 187137, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   2,   3,  253, 187137, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   4,   3,  255, 187688, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   4,   3,  254, 187688, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   4,   3,  253, 187688, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,   4,  259, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,   4,  260, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,   4,  261, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,   4,  259, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,   4,  260, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,   4,  261, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   2,   5,  258, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   2,   5,  257, 187132, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   2,   5,  256, 187132, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   4,   5,  258, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   4,   5,  257, 187430, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   4,   5,  256, 187430, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   2,   6,  252, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   2,   6,  250, 187142, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   2,   6,  251, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   4,   6,  252, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   4,   6,  250, 187143, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      2,  1263,   18,   4,   6,  251, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   2,   7,  263, 187137, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   2,   7,  262, 187138, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   2,   7,  264, 187139, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   4,   7,  263, 187688, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   4,   7,  262, 187778, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      2,  1262,   18,   4,   7,  264, 187158, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   2,   8,   62, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   2,   8,   63, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   2,   8,   64, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   4,   8,   62, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   4,   8,   63, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   4,   8,   64, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   2,   9,  265, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   2,   9,  266, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   2,   9,  267, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   4,   9,  265, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   4,   9,  266, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      2,  1260,   18,   4,   9,  267, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,  10,  270, 187134, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,  10,  269, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,  10,  268, 187136, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,  10,  270, 187604, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,  10,  269, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,  10,  268, 187862, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,  11,  102, 187133, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,  11,  105, 187134, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,  11,  103, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   2,  11,  104, 187136, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,  11,  102, 187435, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,  11,  105, 187604, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,  11,  103, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      2,  1261,   18,   4,  11,  104, 187862, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Battlelord's Plate"                         ,   "tier19oh",      3,  1280,   19,   2,   1,   -1, 221615, { 139681, 139682, 139683, 139684, 139685, 139686, 139687, 139688,  0 } },
  { "Battlelord's Plate"                         ,   "tier19oh",      3,  1280,   19,   4,   1,   -1, 221617, { 139681, 139682, 139683, 139684, 139685, 139686, 139687, 139688,  0 } },
  { "Battlelord's Plate"                         ,   "tier19oh",      3,  1280,   19,   6,   1,   -1, 221533, { 139681, 139682, 139683, 139684, 139685, 139686, 139687, 139688,  0 } },
  { "Battlelord's Plate"                         ,   "tier19oh",      3,  1280,   19,   8,   1,   -1, 221669, { 139681, 139682, 139683, 139684, 139685, 139686, 139687, 139688,  0 } },
  { "Battleplate of the Silver Hand"             ,   "tier19oh",      3,  1275,   19,   2,   2,   -1, 221615, { 139690, 139691, 139692, 139693, 139694, 139695, 139696, 139697,  0 } },
  { "Battleplate of the Silver Hand"             ,   "tier19oh",      3,  1275,   19,   4,   2,   -1, 221617, { 139690, 139691, 139692, 139693, 139694, 139695, 139696, 139697,  0 } },
  { "Battleplate of the Silver Hand"             ,   "tier19oh",      3,  1275,   19,   6,   2,   65, 221534, { 139690, 139691, 139692, 139693, 139694, 139695, 139696, 139697,  0 } },
  { "Battleplate of the Silver Hand"             ,   "tier19oh",      3,  1275,   19,   6,   2,   70, 221533, { 139690, 139691, 139692, 139693, 139694, 139695, 139696, 139697,  0 } },
  { "Battleplate of the Silver Hand"             ,   "tier19oh",      3,  1275,   19,   6,   2,   66, 221533, { 139690, 139691, 139692, 139693, 139694, 139695, 139696, 139697,  0 } },
  { "Battleplate of the Silver Hand"             ,   "tier19oh",      3,  1275,   19,   8,   2,   -1, 221653, { 139690, 139691, 139692, 139693, 139694, 139695, 139696, 139697,  0 } },
  { "Guise of the Unseen Path"                   ,   "tier19oh",      3,  1272,   19,   2,   3,   -1, 221615, { 139707, 139708, 139709, 139710, 139711, 139712, 139713, 139714,  0 } },
  { "Guise of the Unseen Path"                   ,   "tier19oh",      3,  1272,   19,   4,   3,   -1, 221617, { 139707, 139708, 139709, 139710, 139711, 139712, 139713, 139714,  0 } },
  { "Guise of the Unseen Path"                   ,   "tier19oh",      3,  1272,   19,   6,   3,   -1, 221535, { 139707, 139708, 139709, 139710, 139711, 139712, 139713, 139714,  0 } },
  { "Guise of the Unseen Path"                   ,   "tier19oh",      3,  1272,   19,   8,   3,   -1, 221642, { 139707, 139708, 139709, 139710, 139711, 139712, 139713, 139714,  0 } },
  { "Battlegear of the Uncrowned"                ,   "tier19oh",      3,  1277,   19,   2,   4,   -1, 221615, { 139739, 139740, 139741, 139742, 139743, 139744, 139745, 139746,  0 } },
  { "Battlegear of the Uncrowned"                ,   "tier19oh",      3,  1277,   19,   4,   4,   -1, 221617, { 139739, 139740, 139741, 139742, 139743, 139744, 139745, 139746,  0 } },
  { "Battlegear of the Uncrowned"                ,   "tier19oh",      3,  1277,   19,   6,   4,   -1, 221535, { 139739, 139740, 139741, 139742, 139743, 139744, 139745, 139746,  0 } },
  { "Battlegear of the Uncrowned"                ,   "tier19oh",      3,  1277,   19,   8,   4,   -1, 221662, { 139739, 139740, 139741, 139742, 139743, 139744, 139745, 139746,  0 } },
  { "Regalia of the High Priest"                 ,   "tier19oh",      3,  1276,   19,   2,   5,   -1, 221615, { 139759, 139755, 139756, 139757, 139758, 139760, 139761, 139762,  0 } },
  { "Regalia of the High Priest"                 ,   "tier19oh",      3,  1276,   19,   4,   5,   -1, 221617, { 139759, 139755, 139756, 139757, 139758, 139760, 139761, 139762,  0 } },
  { "Regalia of the High Priest"                 ,   "tier19oh",      3,  1276,   19,   6,   5,   -1, 221534, { 139759, 139755, 139756, 139757, 139758, 139760, 139761, 139762,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Regalia of the High Priest"                 ,   "tier19oh",      3,  1276,   19,   8,   5,   -1, 221655, { 139759, 139755, 139756, 139757, 139758, 139760, 139761, 139762,  0 } },
  { "Deathlord's Battleplate"                    ,   "tier19oh",      3,  1269,   19,   2,   6,   -1, 221615, { 139673, 139674, 139675, 139676, 139677, 139678, 139679, 139680,  0 } },
  { "Deathlord's Battleplate"                    ,   "tier19oh",      3,  1269,   19,   4,   6,   -1, 221617, { 139673, 139674, 139675, 139676, 139677, 139678, 139679, 139680,  0 } },
  { "Deathlord's Battleplate"                    ,   "tier19oh",      3,  1269,   19,   6,   6,   -1, 221533, { 139673, 139674, 139675, 139676, 139677, 139678, 139679, 139680,  0 } },
  { "Deathlord's Battleplate"                    ,   "tier19oh",      3,  1269,   19,   8,   6,   -1, 221620, { 139673, 139674, 139675, 139676, 139677, 139678, 139679, 139680,  0 } },
  { "Raiment of the Farseer"                     ,   "tier19oh",      3,  1278,   19,   2,   7,   -1, 221615, { 139698, 139699, 139700, 139701, 139702, 139703, 139704, 139705,  0 } },
  { "Raiment of the Farseer"                     ,   "tier19oh",      3,  1278,   19,   4,   7,   -1, 221617, { 139698, 139699, 139700, 139701, 139702, 139703, 139704, 139705,  0 } },
  { "Raiment of the Farseer"                     ,   "tier19oh",      3,  1278,   19,   6,   7,  262, 221534, { 139698, 139699, 139700, 139701, 139702, 139703, 139704, 139705,  0 } },
  { "Raiment of the Farseer"                     ,   "tier19oh",      3,  1278,   19,   6,   7,  264, 221534, { 139698, 139699, 139700, 139701, 139702, 139703, 139704, 139705,  0 } },
  { "Raiment of the Farseer"                     ,   "tier19oh",      3,  1278,   19,   6,   7,  263, 221535, { 139698, 139699, 139700, 139701, 139702, 139703, 139704, 139705,  0 } },
  { "Raiment of the Farseer"                     ,   "tier19oh",      3,  1278,   19,   8,   7,   -1, 221664, { 139698, 139699, 139700, 139701, 139702, 139703, 139704, 139705,  0 } },
  { "Vesture of Tirisgarde"                      ,   "tier19oh",      3,  1273,   19,   2,   8,   -1, 221615, { 139751, 139747, 139748, 139749, 139750, 139752, 139753, 139754,  0 } },
  { "Vesture of Tirisgarde"                      ,   "tier19oh",      3,  1273,   19,   4,   8,   -1, 221617, { 139751, 139747, 139748, 139749, 139750, 139752, 139753, 139754,  0 } },
  { "Vesture of Tirisgarde"                      ,   "tier19oh",      3,  1273,   19,   6,   8,   -1, 221534, { 139751, 139747, 139748, 139749, 139750, 139752, 139753, 139754,  0 } },
  { "Vesture of Tirisgarde"                      ,   "tier19oh",      3,  1273,   19,   8,   8,   -1, 221646, { 139751, 139747, 139748, 139749, 139750, 139752, 139753, 139754,  0 } },
  { "Vestments of the Black Harvest"             ,   "tier19oh",      3,  1279,   19,   2,   9,   -1, 221615, { 139767, 139763, 139764, 139765, 139766, 139768, 139769, 139770,  0 } },
  { "Vestments of the Black Harvest"             ,   "tier19oh",      3,  1279,   19,   4,   9,   -1, 221617, { 139767, 139763, 139764, 139765, 139766, 139768, 139769, 139770,  0 } },
  { "Vestments of the Black Harvest"             ,   "tier19oh",      3,  1279,   19,   6,   9,   -1, 221534, { 139767, 139763, 139764, 139765, 139766, 139768, 139769, 139770,  0 } },
  { "Vestments of the Black Harvest"             ,   "tier19oh",      3,  1279,   19,   8,   9,   -1, 221667, { 139767, 139763, 139764, 139765, 139766, 139768, 139769, 139770,  0 } },
  { "Grandmaster's Finery"                       ,   "tier19oh",      3,  1274,   19,   2,  10,   -1, 221615, { 139731, 139732, 139733, 139734, 139735, 139736, 139737, 139738,  0 } },
  { "Grandmaster's Finery"                       ,   "tier19oh",      3,  1274,   19,   4,  10,   -1, 221617, { 139731, 139732, 139733, 139734, 139735, 139736, 139737, 139738,  0 } },
  { "Grandmaster's Finery"                       ,   "tier19oh",      3,  1274,   19,   6,  10,  268, 221535, { 139731, 139732, 139733, 139734, 139735, 139736, 139737, 139738,  0 } },
  { "Grandmaster's Finery"                       ,   "tier19oh",      3,  1274,   19,   6,  10,  270, 221534, { 139731, 139732, 139733, 139734, 139735, 139736, 139737, 139738,  0 } },
  { "Grandmaster's Finery"                       ,   "tier19oh",      3,  1274,   19,   6,  10,  269, 221535, { 139731, 139732, 139733, 139734, 139735, 139736, 139737, 139738,  0 } },
  { "Grandmaster's Finery"                       ,   "tier19oh",      3,  1274,   19,   8,  10,   -1, 199380, { 139731, 139732, 139733, 139734, 139735, 139736, 139737, 139738,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   2,  11,   -1, 221615, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   4,  11,   -1, 221617, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   6,  11,  103, 221535, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   6,  11,  104, 221535, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   6,  11,  105, 221534, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   6,  11,  102, 221534, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   8,  11,  104, 221636, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   8,  11,  105, 221636, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   8,  11,  102, 221636, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Dreamgrove Raiment"                         ,   "tier19oh",      3,  1271,   19,   8,  11,  103, 221638, { 139723, 139724, 139725, 139726, 139727, 139728, 139729, 139730,  0 } },
  { "Battlegear of the Shattered Abyss"          ,   "tier19oh",      3,  1270,   19,   2,  12,   -1, 221615, { 139715, 139716, 139717, 139718, 139719, 139720, 139721, 139722,  0 } },
  { "Battlegear of the Shattered Abyss"          ,   "tier19oh",      3,  1270,   19,   4,  12,   -1, 221617, { 139715, 139716, 139717, 139718, 139719, 139720, 139721, 139722,  0 } },
  { "Battlegear of the Shattered Abyss"          ,   "tier19oh",      3,  1270,   19,   6,  12,   -1, 221535, { 139715, 139716, 139717, 139718, 139719, 139720, 139721, 139722,  0 } },
  { "Battlegear of the Shattered Abyss"          ,   "tier19oh",      3,  1270,   19,   8,  12,   -1, 221632, { 139715, 139716, 139717, 139718, 139719, 139720, 139721, 139722,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,   1,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,   2,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,   3,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,   4,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,   5,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,   6,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,   7,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,   8,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,   9,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,  10,   -1, 228445, { 134529, 134533,  0 } },
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,  11,   -1, 228445, { 134529, 134533,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "March of the Legion"                        , "march_of_the_legion",      4,  1293,   19,   2,  12,   -1, 228445, { 134529, 134533,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,   1,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,   2,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,   3,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,   4,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,   5,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,   6,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,   7,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,   8,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,   9,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,  10,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,  11,   -1, 228447, { 137419, 137487,  0 } },
  { "Journey Through Time"                       , "journey_through_time",      5,  1294,   19,   2,  12,   -1, 228447, { 137419, 137487,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,   1,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,   2,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,   3,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,   4,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,   5,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,   6,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,   7,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,   8,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,   9,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,  10,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,  11,   -1, 224146, { 134432, 134424,  0 } },
  { "Nightwell Arcanum"                          , "tier19p_cloth",      6,  1295,   19,   2,  12,   -1, 224146, { 134432, 134424,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,   1,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,   2,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,   3,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,   4,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,   5,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,   6,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,   7,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,   8,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,   9,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,  10,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,  11,   -1, 224148, { 137480, 137397,  0 } },
  { "Jacin's Ruse"                               , "tier19p_leather",      7,  1296,   19,   2,  12,   -1, 224148, { 137480, 137397,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,   1,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,   2,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,   3,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,   4,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,   5,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,   6,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,   7,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,   8,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,   9,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,  10,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,  11,   -1, 224150, { 134480, 137405,  0 } },
  { "Traitor's Oath"                             , "tier19p_mail",      8,  1297,   19,   2,  12,   -1, 224150, { 134480, 137405,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,   1,   -1, 228448, { 134518, 134503,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,   2,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,   3,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,   4,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,   5,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,   6,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,   7,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,   8,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,   9,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,  10,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,  11,   -1, 228448, { 134518, 134503,  0 } },
  { "Fortitude of the Nightborne"                , "tier19p_plate",      9,  1298,   19,   2,  12,   -1, 228448, { 134518, 134503,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",     10,  1242,   17,   2,   1,   71, 165345, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",     10,  1242,   17,   2,   1,   72, 165337, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",     10,  1242,   17,   2,   1,   73, 165338, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",     10,  1242,   17,   4,   1,   71, 165336, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",     10,  1242,   17,   4,   1,   72, 165349, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",     10,  1242,   17,   4,   1,   73, 165351, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",     10,  1238,   17,   2,   2,   65, 165438, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",     10,  1238,   17,   2,   2,   66, 165446, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",     10,  1238,   17,   2,   2,   70, 165440, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",     10,  1238,   17,   4,   2,   65, 167697, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",     10,  1238,   17,   4,   2,   66, 167740, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",     10,  1238,   17,   4,   2,   70, 165439, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",     10,  1236,   17,   2,   3,  253, 165517, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",     10,  1236,   17,   2,   3,  254, 165519, { 115545, 115546, 115547, 115548, 115549,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Rylakstalker's Battlegear"                  ,     "tier17",     10,  1236,   17,   2,   3,  255, 165544, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",     10,  1236,   17,   4,   3,  253, 165518, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",     10,  1236,   17,   4,   3,  254, 165525, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",     10,  1236,   17,   4,   3,  255, 165545, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",     10,  1240,   17,   2,   4,  259, 165516, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",     10,  1240,   17,   2,   4,  260, 165480, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",     10,  1240,   17,   2,   4,  261, 165482, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",     10,  1240,   17,   4,   4,  259, 166886, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",     10,  1240,   17,   4,   4,  260, 165478, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",     10,  1240,   17,   4,   4,  261, 165514, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",     10,  1239,   17,   2,   5,  256, 165614, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",     10,  1239,   17,   2,   5,  257, 165621, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",     10,  1239,   17,   2,   5,  258, 165628, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",     10,  1239,   17,   4,   5,  256, 167694, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",     10,  1239,   17,   4,   5,  257, 167684, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",     10,  1239,   17,   4,   5,  258, 165629, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",     10,  1234,   17,   2,   6,  250, 167192, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",     10,  1234,   17,   2,   6,  251, 165547, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",     10,  1234,   17,   2,   6,  252, 165575, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",     10,  1234,   17,   4,   6,  250, 165571, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",     10,  1234,   17,   4,   6,  251, 165568, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",     10,  1234,   17,   4,   6,  252, 165574, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",     10,  1241,   17,   2,   7,  262, 165577, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",     10,  1241,   17,   2,   7,  263, 165605, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",     10,  1241,   17,   2,   7,  264, 165576, { 115575, 115576, 115577, 115578, 115579,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Windspeaker's Regalia"                      ,     "tier17",     10,  1241,   17,   4,   7,  262, 165580, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",     10,  1241,   17,   4,   7,  263, 165610, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",     10,  1241,   17,   4,   7,  264, 167702, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",     10,  1235,   17,   2,   8,   62, 165475, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",     10,  1235,   17,   2,   8,   63, 165458, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",     10,  1235,   17,   2,   8,   64, 165469, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",     10,  1235,   17,   4,   8,   62, 165476, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",     10,  1235,   17,   4,   8,   63, 165459, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",     10,  1235,   17,   4,   8,   64, 165470, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",     10,  1243,   17,   2,   9,  265, 165448, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",     10,  1243,   17,   2,   9,  266, 165450, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",     10,  1243,   17,   2,   9,  267, 165455, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",     10,  1243,   17,   4,   9,  265, 165449, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",     10,  1243,   17,   4,   9,  266, 165451, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",     10,  1243,   17,   4,   9,  267, 165452, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",     10,  1237,   17,   2,  10,  268, 165353, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",     10,  1237,   17,   2,  10,  270, 165404, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",     10,  1237,   17,   2,  10,  269, 165403, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",     10,  1237,   17,   4,  10,  268, 165352, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",     10,  1237,   17,   4,  10,  270, 167718, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",     10,  1237,   17,   4,  10,  269, 165402, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",     10,  1233,   17,   2,  11,  103, 165431, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",     10,  1233,   17,   2,  11,  102, 165433, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",     10,  1233,   17,   2,  11,  104, 165410, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",     10,  1233,   17,   2,  11,  105, 165437, { 115540, 115541, 115542, 115543, 115544,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Living Wood Battlegear"                     ,     "tier17",     10,  1233,   17,   4,  11,  103, 165432, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",     10,  1233,   17,   4,  11,  102, 166677, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",     10,  1233,   17,   4,  11,  104, 165423, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",     10,  1233,   17,   4,  11,  105, 167714, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",     11,  1258,   18,   2,   1,   71, 185800, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",     11,  1258,   18,   2,   1,   72, 185798, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",     11,  1258,   18,   2,   1,   73, 185796, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",     11,  1258,   18,   4,   1,   71, 185804, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",     11,  1258,   18,   4,   1,   72, 185799, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",     11,  1258,   18,   4,   1,   73, 185797, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",     11,  1254,   18,   2,   2,   65, 185545, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",     11,  1254,   18,   2,   2,   66, 185675, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",     11,  1254,   18,   2,   2,   70,  67188, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",     11,  1254,   18,   4,   2,   65, 185571, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",     11,  1254,   18,   4,   2,   66, 185677, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",     11,  1254,   18,   4,   2,   70, 185648, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",     11,  1253,   18,   2,   3,  253, 188200, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",     11,  1253,   18,   2,   3,  254, 188190, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",     11,  1253,   18,   2,   3,  255, 188198, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",     11,  1253,   18,   4,   3,  253, 188201, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",     11,  1253,   18,   4,   3,  254, 188191, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",     11,  1253,   18,   4,   3,  255, 188199, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Felblade Armor"                             ,     "tier18",     11,  1256,   18,   2,   4,  259, 186182, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Felblade Armor"                             ,     "tier18",     11,  1256,   18,   2,   4,  260, 186285, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Felblade Armor"                             ,     "tier18",     11,  1256,   18,   2,   4,  261, 186278, { 124248, 124257, 124263, 124269, 124274,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Felblade Armor"                             ,     "tier18",     11,  1256,   18,   4,   4,  259, 186277, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Felblade Armor"                             ,     "tier18",     11,  1256,   18,   4,   4,  260, 186288, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Felblade Armor"                             ,     "tier18",     11,  1256,   18,   4,   4,  261, 186279, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Attire of Piety"                            ,     "tier18",     11,  1255,   18,   2,   5,  256, 186477, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",     11,  1255,   18,   2,   5,  257, 186298, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",     11,  1255,   18,   2,   5,  258, 186980, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",     11,  1255,   18,   4,   5,  256, 186492, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",     11,  1255,   18,   4,   5,  257, 186378, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",     11,  1255,   18,   4,   5,  258, 186981, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Demongaze Armor"                            ,     "tier18",     11,  1249,   18,   2,   6,  250, 187872, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",     11,  1249,   18,   2,   6,  251, 187868, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",     11,  1249,   18,   2,   6,  252, 187865, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",     11,  1249,   18,   4,   6,  250, 187873, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",     11,  1249,   18,   4,   6,  251, 187870, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",     11,  1249,   18,   4,   6,  252, 187866, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",     11,  1257,   18,   2,   7,  262, 185880, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",     11,  1257,   18,   2,   7,  263, 185871, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",     11,  1257,   18,   2,   7,  264, 185873, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",     11,  1257,   18,   4,   7,  262, 185881, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",     11,  1257,   18,   4,   7,  263, 185872, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",     11,  1257,   18,   4,   7,  264, 185874, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",     11,  1251,   18,   2,   8,   62, 186166, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",     11,  1251,   18,   2,   8,   63, 186167, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",     11,  1251,   18,   2,   8,   64, 185969, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",     11,  1251,   18,   4,   8,   62, 186165, { 124154, 124160, 124165, 124171, 124177,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Raiment of the Arcanic Conclave"            ,     "tier18",     11,  1251,   18,   4,   8,   63, 186168, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",     11,  1251,   18,   4,   8,   64, 185971, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",     11,  1259,   18,   2,   9,  265, 185882, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",     11,  1259,   18,   2,   9,  266, 185884, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",     11,  1259,   18,   2,   9,  267, 185965, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",     11,  1259,   18,   4,   9,  265, 185883, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",     11,  1259,   18,   4,   9,  266, 185964, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",     11,  1259,   18,   4,   9,  267, 185967, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",     11,  1252,   18,   2,  10,  268, 185398, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",     11,  1252,   18,   2,  10,  270, 185126, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",     11,  1252,   18,   2,  10,  269, 185542, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",     11,  1252,   18,   4,  10,  268, 185399, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",     11,  1252,   18,   4,  10,  270, 185258, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",     11,  1252,   18,   4,  10,  269, 185543, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",     11,  1250,   18,   2,  11,  103, 185805, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",     11,  1250,   18,   2,  11,  104, 185813, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",     11,  1250,   18,   2,  11,  105, 185294, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",     11,  1250,   18,   2,  11,  102, 187875, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",     11,  1250,   18,   4,  11,  103, 185811, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",     11,  1250,   18,   4,  11,  104, 185814, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",     11,  1250,   18,   4,  11,  105, 185397, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",     11,  1250,   18,   4,  11,  102, 187877, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   2,   1,   71, 212156, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   2,   1,   73, 212237, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   2,   1,   72,  70847, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   4,   1,   72, 212158, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   4,   1,   73, 212244, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   4,   1,   71,  70847, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   2,   2,   65, 211436, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   2,   2,   70, 211444, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   2,   2,   66, 211553, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   4,   2,   65, 211438, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   4,   2,   70, 211448, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   4,   2,   66, 211554, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   2,   3,  254, 211331, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   2,   3,  253, 197140, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   2,   3,  255,  89923, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   4,   3,  254, 211322, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   4,   3,  253, 197139, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   4,   3,  255, 225092, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Doomblade Battlegear"                       ,     "tier19",     12,  1289,   19,   2,   4,  261, 211661, { 138326, 138329, 138332, 138335, 138338, 138371,  0 } },
  { "Doomblade Battlegear"                       ,     "tier19",     12,  1289,   19,   2,   4,  260, 211667, { 138326, 138329, 138332, 138335, 138338, 138371,  0 } },
  { "Doomblade Battlegear"                       ,     "tier19",     12,  1289,   19,   2,   4,  259, 211671, { 138326, 138329, 138332, 138335, 138338, 138371,  0 } },
  { "Doomblade Battlegear"                       ,     "tier19",     12,  1289,   19,   4,   4,  261, 211663, { 138326, 138329, 138332, 138335, 138338, 138371,  0 } },
  { "Doomblade Battlegear"                       ,     "tier19",     12,  1289,   19,   4,   4,  260, 211668, { 138326, 138329, 138332, 138335, 138338, 138371,  0 } },
  { "Doomblade Battlegear"                       ,     "tier19",     12,  1289,   19,   4,   4,  259, 211673, { 138326, 138329, 138332, 138335, 138338, 138371,  0 } },
  { "Vestments of the Purifier"                  ,     "tier19",     12,  1288,   19,   2,   5,  256, 211556, { 138310, 138313, 138316, 138319, 138322, 138370,  0 } },
  { "Vestments of the Purifier"                  ,     "tier19",     12,  1288,   19,   2,   5,  257, 211638, { 138310, 138313, 138316, 138319, 138322, 138370,  0 } },
  { "Vestments of the Purifier"                  ,     "tier19",     12,  1288,   19,   2,   5,  258, 211646, { 138310, 138313, 138316, 138319, 138322, 138370,  0 } },
  { "Vestments of the Purifier"                  ,     "tier19",     12,  1288,   19,   4,   5,  256, 211563, { 138310, 138313, 138316, 138319, 138322, 138370,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Vestments of the Purifier"                  ,     "tier19",     12,  1288,   19,   4,   5,  257, 211640, { 138310, 138313, 138316, 138319, 138322, 138370,  0 } },
  { "Vestments of the Purifier"                  ,     "tier19",     12,  1288,   19,   4,   5,  258, 211654, { 138310, 138313, 138316, 138319, 138322, 138370,  0 } },
  { "Dreadwyrm Battleplate"                      ,     "tier19",     12,  1281,   19,   2,   6,  250, 211039, { 138349, 138352, 138355, 138358, 138361, 138364,  0 } },
  { "Dreadwyrm Battleplate"                      ,     "tier19",     12,  1281,   19,   2,   6,  251, 211042, { 138349, 138352, 138355, 138358, 138361, 138364,  0 } },
  { "Dreadwyrm Battleplate"                      ,     "tier19",     12,  1281,   19,   2,   6,  252, 211047, { 138349, 138352, 138355, 138358, 138361, 138364,  0 } },
  { "Dreadwyrm Battleplate"                      ,     "tier19",     12,  1281,   19,   4,   6,  250, 211041, { 138349, 138352, 138355, 138358, 138361, 138364,  0 } },
  { "Dreadwyrm Battleplate"                      ,     "tier19",     12,  1281,   19,   4,   6,  251, 211045, { 138349, 138352, 138355, 138358, 138361, 138364,  0 } },
  { "Dreadwyrm Battleplate"                      ,     "tier19",     12,  1281,   19,   4,   6,  252, 211050, { 138349, 138352, 138355, 138358, 138361, 138364,  0 } },
  { "Regalia of Shackled Elements"               ,     "tier19",     12,  1290,   19,   2,   7,  262, 211951, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Shackled Elements"               ,     "tier19",     12,  1290,   19,   2,   7,  263, 211987, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Shackled Elements"               ,     "tier19",     12,  1290,   19,   2,   7,  264, 211992, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Shackled Elements"               ,     "tier19",     12,  1290,   19,   4,   7,  263, 211988, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Shackled Elements"               ,     "tier19",     12,  1290,   19,   4,   7,  264, 211993, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Shackled Elements"               ,     "tier19",     12,  1290,   19,   4,   7,  262, 191493, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   2,   8,   63, 211394, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   2,   8,   64, 211411, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   2,   8,   62, 187287, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   4,   8,   62, 211386, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   4,   8,   63, 211395, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   4,   8,   64, 211404, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Legacy of Azj'Aqir"                         ,     "tier19",     12,  1291,   19,   2,   9,  265, 212002, { 138311, 138314, 138317, 138320, 138323, 138373,  0 } },
  { "Legacy of Azj'Aqir"                         ,     "tier19",     12,  1291,   19,   2,   9,  267, 212018, { 138311, 138314, 138317, 138320, 138323, 138373,  0 } },
  { "Legacy of Azj'Aqir"                         ,     "tier19",     12,  1291,   19,   2,   9,  266, 138305, { 138311, 138314, 138317, 138320, 138323, 138373,  0 } },
  { "Legacy of Azj'Aqir"                         ,     "tier19",     12,  1291,   19,   4,   9,  265, 212003, { 138311, 138314, 138317, 138320, 138323, 138373,  0 } },
  { "Legacy of Azj'Aqir"                         ,     "tier19",     12,  1291,   19,   4,   9,  266, 212007, { 138311, 138314, 138317, 138320, 138323, 138373,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Legacy of Azj'Aqir"                         ,     "tier19",     12,  1291,   19,   4,   9,  267, 212017, { 138311, 138314, 138317, 138320, 138323, 138373,  0 } },
  { "Vestments of Enveloped Dissonance"          ,     "tier19",     12,  1286,   19,   2,  10,  268, 211415, { 138325, 138328, 138331, 138334, 138337, 138367,  0 } },
  { "Vestments of Enveloped Dissonance"          ,     "tier19",     12,  1286,   19,   2,  10,  270, 211418, { 138325, 138328, 138331, 138334, 138337, 138367,  0 } },
  { "Vestments of Enveloped Dissonance"          ,     "tier19",     12,  1286,   19,   2,  10,  269, 211427, { 138325, 138328, 138331, 138334, 138337, 138367,  0 } },
  { "Vestments of Enveloped Dissonance"          ,     "tier19",     12,  1286,   19,   4,  10,  268, 211417, { 138325, 138328, 138331, 138334, 138337, 138367,  0 } },
  { "Vestments of Enveloped Dissonance"          ,     "tier19",     12,  1286,   19,   4,  10,  270, 211423, { 138325, 138328, 138331, 138334, 138337, 138367,  0 } },
  { "Vestments of Enveloped Dissonance"          ,     "tier19",     12,  1286,   19,   4,  10,  269, 211430, { 138325, 138328, 138331, 138334, 138337, 138367,  0 } },
  { "Garb of the Astral Warden"                  ,     "tier19",     12,  1283,   19,   2,  11,  102, 211092, { 138324, 138327, 138330, 138333, 138336, 138366,  0 } },
  { "Garb of the Astral Warden"                  ,     "tier19",     12,  1283,   19,   2,  11,  103, 211140, { 138324, 138327, 138330, 138333, 138336, 138366,  0 } },
  { "Garb of the Astral Warden"                  ,     "tier19",     12,  1283,   19,   2,  11,  104, 211149, { 138324, 138327, 138330, 138333, 138336, 138366,  0 } },
  { "Garb of the Astral Warden"                  ,     "tier19",     12,  1283,   19,   2,  11,  105, 211165, { 138324, 138327, 138330, 138333, 138336, 138366,  0 } },
  { "Garb of the Astral Warden"                  ,     "tier19",     12,  1283,   19,   4,  11,  102, 211104, { 138324, 138327, 138330, 138333, 138336, 138366,  0 } },
  { "Garb of the Astral Warden"                  ,     "tier19",     12,  1283,   19,   4,  11,  103, 211142, { 138324, 138327, 138330, 138333, 138336, 138366,  0 } },
  { "Garb of the Astral Warden"                  ,     "tier19",     12,  1283,   19,   4,  11,  104, 211150, { 138324, 138327, 138330, 138333, 138336, 138366,  0 } },
  { "Garb of the Astral Warden"                  ,     "tier19",     12,  1283,   19,   4,  11,  105, 211170, { 138324, 138327, 138330, 138333, 138336, 138366,  0 } },
  { "Vestment of Second Sight"                   ,     "tier19",     12,  1282,   19,   2,  12,  577, 211054, { 138375, 138376, 138377, 138378, 138379, 138380,  0 } },
  { "Vestment of Second Sight"                   ,     "tier19",     12,  1282,   19,   2,  12,  581, 211075, { 138375, 138376, 138377, 138378, 138379, 138380,  0 } },
  { "Vestment of Second Sight"                   ,     "tier19",     12,  1282,   19,   4,  12,  577, 211074, { 138375, 138376, 138377, 138378, 138379, 138380,  0 } },
  { "Vestment of Second Sight"                   ,     "tier19",     12,  1282,   19,   4,  12,  581, 211077, { 138375, 138376, 138377, 138378, 138379, 138380,  0 } },
  { "Titanic Onslaught Armor"                    ,     "tier20",     13,  1312,   20,   2,   1,   71, 242298, { 147187, 147188, 147189, 147190, 147191, 147192,  0 } },
  { "Titanic Onslaught Armor"                    ,     "tier20",     13,  1312,   20,   2,   1,   72, 242299, { 147187, 147188, 147189, 147190, 147191, 147192,  0 } },
  { "Titanic Onslaught Armor"                    ,     "tier20",     13,  1312,   20,   2,   1,   73, 242302, { 147187, 147188, 147189, 147190, 147191, 147192,  0 } },
  { "Titanic Onslaught Armor"                    ,     "tier20",     13,  1312,   20,   4,   1,   71, 242297, { 147187, 147188, 147189, 147190, 147191, 147192,  0 } },
  { "Titanic Onslaught Armor"                    ,     "tier20",     13,  1312,   20,   4,   1,   72, 242301, { 147187, 147188, 147189, 147190, 147191, 147192,  0 } },
  { "Titanic Onslaught Armor"                    ,     "tier20",     13,  1312,   20,   4,   1,   73, 242373, { 147187, 147188, 147189, 147190, 147191, 147192,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Radiant Lightbringer Armor"                 ,     "tier20",     13,  1307,   20,   2,   2,   65, 242261, { 147157, 147158, 147159, 147160, 147161, 147162,  0 } },
  { "Radiant Lightbringer Armor"                 ,     "tier20",     13,  1307,   20,   2,   2,   66, 242263, { 147157, 147158, 147159, 147160, 147161, 147162,  0 } },
  { "Radiant Lightbringer Armor"                 ,     "tier20",     13,  1307,   20,   2,   2,   70, 242267, { 147157, 147158, 147159, 147160, 147161, 147162,  0 } },
  { "Radiant Lightbringer Armor"                 ,     "tier20",     13,  1307,   20,   4,   2,   65, 242262, { 147157, 147158, 147159, 147160, 147161, 147162,  0 } },
  { "Radiant Lightbringer Armor"                 ,     "tier20",     13,  1307,   20,   4,   2,   66, 242264, { 147157, 147158, 147159, 147160, 147161, 147162,  0 } },
  { "Radiant Lightbringer Armor"                 ,     "tier20",     13,  1307,   20,   4,   2,   70, 242266, { 147157, 147158, 147159, 147160, 147161, 147162,  0 } },
  { "Wildstalker Armor"                          ,     "tier20",     13,  1304,   20,   2,   3,  253, 242239, { 147139, 147140, 147141, 147142, 147143, 147144,  0 } },
  { "Wildstalker Armor"                          ,     "tier20",     13,  1304,   20,   2,   3,  254, 242242, { 147139, 147140, 147141, 147142, 147143, 147144,  0 } },
  { "Wildstalker Armor"                          ,     "tier20",     13,  1304,   20,   2,   3,  255, 138305, { 147139, 147140, 147141, 147142, 147143, 147144,  0 } },
  { "Wildstalker Armor"                          ,     "tier20",     13,  1304,   20,   4,   3,  253, 242240, { 147139, 147140, 147141, 147142, 147143, 147144,  0 } },
  { "Wildstalker Armor"                          ,     "tier20",     13,  1304,   20,   4,   3,  254, 242241, { 147139, 147140, 147141, 147142, 147143, 147144,  0 } },
  { "Wildstalker Armor"                          ,     "tier20",     13,  1304,   20,   4,   3,  255,  70854, { 147139, 147140, 147141, 147142, 147143, 147144,  0 } },
  { "Fanged Slayer's Armor"                      ,     "tier20",     13,  1309,   20,   2,   4,  259, 242274, { 147169, 147170, 147171, 147172, 147173, 147174,  0 } },
  { "Fanged Slayer's Armor"                      ,     "tier20",     13,  1309,   20,   2,   4,  260, 242276, { 147169, 147170, 147171, 147172, 147173, 147174,  0 } },
  { "Fanged Slayer's Armor"                      ,     "tier20",     13,  1309,   20,   2,   4,  261, 242280, { 147169, 147170, 147171, 147172, 147173, 147174,  0 } },
  { "Fanged Slayer's Armor"                      ,     "tier20",     13,  1309,   20,   4,   4,  259, 242275, { 147169, 147170, 147171, 147172, 147173, 147174,  0 } },
  { "Fanged Slayer's Armor"                      ,     "tier20",     13,  1309,   20,   4,   4,  260, 242278, { 147169, 147170, 147171, 147172, 147173, 147174,  0 } },
  { "Fanged Slayer's Armor"                      ,     "tier20",     13,  1309,   20,   4,   4,  261, 242279, { 147169, 147170, 147171, 147172, 147173, 147174,  0 } },
  { "Vestments of Blind Absolution"              ,     "tier20",     13,  1308,   20,   2,   5,  256, 242268, { 147163, 147164, 147165, 147166, 147167, 147168,  0 } },
  { "Vestments of Blind Absolution"              ,     "tier20",     13,  1308,   20,   2,   5,  257, 242270, { 147163, 147164, 147165, 147166, 147167, 147168,  0 } },
  { "Vestments of Blind Absolution"              ,     "tier20",     13,  1308,   20,   2,   5,  258, 242272, { 147163, 147164, 147165, 147166, 147167, 147168,  0 } },
  { "Vestments of Blind Absolution"              ,     "tier20",     13,  1308,   20,   4,   5,  256, 242269, { 147163, 147164, 147165, 147166, 147167, 147168,  0 } },
  { "Vestments of Blind Absolution"              ,     "tier20",     13,  1308,   20,   4,   5,  257, 242271, { 147163, 147164, 147165, 147166, 147167, 147168,  0 } },
  { "Vestments of Blind Absolution"              ,     "tier20",     13,  1308,   20,   4,   5,  258, 242273, { 147163, 147164, 147165, 147166, 147167, 147168,  0 } },
  { "Gravewarden Armaments"                      ,     "tier20",     13,  1301,   20,   2,   6,  250, 242001, { 147121, 147122, 147123, 147124, 147125, 147126,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Gravewarden Armaments"                      ,     "tier20",     13,  1301,   20,   2,   6,  251, 242058, { 147121, 147122, 147123, 147124, 147125, 147126,  0 } },
  { "Gravewarden Armaments"                      ,     "tier20",     13,  1301,   20,   2,   6,  252, 242064, { 147121, 147122, 147123, 147124, 147125, 147126,  0 } },
  { "Gravewarden Armaments"                      ,     "tier20",     13,  1301,   20,   4,   6,  250, 242009, { 147121, 147122, 147123, 147124, 147125, 147126,  0 } },
  { "Gravewarden Armaments"                      ,     "tier20",     13,  1301,   20,   4,   6,  251, 242063, { 147121, 147122, 147123, 147124, 147125, 147126,  0 } },
  { "Gravewarden Armaments"                      ,     "tier20",     13,  1301,   20,   4,   6,  252, 242225, { 147121, 147122, 147123, 147124, 147125, 147126,  0 } },
  { "Regalia of the Skybreaker"                  ,     "tier20",     13,  1310,   20,   2,   7,  262, 242282, { 147175, 147176, 147177, 147178, 147179, 147180,  0 } },
  { "Regalia of the Skybreaker"                  ,     "tier20",     13,  1310,   20,   2,   7,  263, 242283, { 147175, 147176, 147177, 147178, 147179, 147180,  0 } },
  { "Regalia of the Skybreaker"                  ,     "tier20",     13,  1310,   20,   2,   7,  264, 242287, { 147175, 147176, 147177, 147178, 147179, 147180,  0 } },
  { "Regalia of the Skybreaker"                  ,     "tier20",     13,  1310,   20,   4,   7,  262, 242281, { 147175, 147176, 147177, 147178, 147179, 147180,  0 } },
  { "Regalia of the Skybreaker"                  ,     "tier20",     13,  1310,   20,   4,   7,  263, 242285, { 147175, 147176, 147177, 147178, 147179, 147180,  0 } },
  { "Regalia of the Skybreaker"                  ,     "tier20",     13,  1310,   20,   4,   7,  264, 242288, { 147175, 147176, 147177, 147178, 147179, 147180,  0 } },
  { "Regalia of the Arcane Tempest"              ,     "tier20",     13,  1305,   20,   2,   8,   62, 242248, { 147145, 147146, 147147, 147148, 147149, 147150,  0 } },
  { "Regalia of the Arcane Tempest"              ,     "tier20",     13,  1305,   20,   2,   8,   63, 242249, { 147145, 147146, 147147, 147148, 147149, 147150,  0 } },
  { "Regalia of the Arcane Tempest"              ,     "tier20",     13,  1305,   20,   2,   8,   64, 242252, { 147145, 147146, 147147, 147148, 147149, 147150,  0 } },
  { "Regalia of the Arcane Tempest"              ,     "tier20",     13,  1305,   20,   4,   8,   64, 242254, { 147145, 147146, 147147, 147148, 147149, 147150,  0 } },
  { "Regalia of the Arcane Tempest"              ,     "tier20",     13,  1305,   20,   4,   8,   63, 242250, { 147145, 147146, 147147, 147148, 147149, 147150,  0 } },
  { "Regalia of the Arcane Tempest"              ,     "tier20",     13,  1305,   20,   4,   8,   62, 246225, { 147145, 147146, 147147, 147148, 147149, 147150,  0 } },
  { "Diabolic Raiment"                           ,     "tier20",     13,  1311,   20,   2,   9,  265, 242290, { 147181, 147182, 147183, 147184, 147185, 147186,  0 } },
  { "Diabolic Raiment"                           ,     "tier20",     13,  1311,   20,   2,   9,  266, 242293, { 147181, 147182, 147183, 147184, 147185, 147186,  0 } },
  { "Diabolic Raiment"                           ,     "tier20",     13,  1311,   20,   2,   9,  267, 242295, { 147181, 147182, 147183, 147184, 147185, 147186,  0 } },
  { "Diabolic Raiment"                           ,     "tier20",     13,  1311,   20,   4,   9,  265, 242291, { 147181, 147182, 147183, 147184, 147185, 147186,  0 } },
  { "Diabolic Raiment"                           ,     "tier20",     13,  1311,   20,   4,   9,  266, 242294, { 147181, 147182, 147183, 147184, 147185, 147186,  0 } },
  { "Diabolic Raiment"                           ,     "tier20",     13,  1311,   20,   4,   9,  267, 242296, { 147181, 147182, 147183, 147184, 147185, 147186,  0 } },
  { "Xuen's Battlegear"                          ,     "tier20",     13,  1306,   20,   2,  10,  268, 242255, { 147151, 147152, 147153, 147154, 147155, 147156,  0 } },
  { "Xuen's Battlegear"                          ,     "tier20",     13,  1306,   20,   2,  10,  270, 242257, { 147151, 147152, 147153, 147154, 147155, 147156,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Xuen's Battlegear"                          ,     "tier20",     13,  1306,   20,   2,  10,  269, 242260, { 147151, 147152, 147153, 147154, 147155, 147156,  0 } },
  { "Xuen's Battlegear"                          ,     "tier20",     13,  1306,   20,   4,  10,  268, 242256, { 147151, 147152, 147153, 147154, 147155, 147156,  0 } },
  { "Xuen's Battlegear"                          ,     "tier20",     13,  1306,   20,   4,  10,  270, 242258, { 147151, 147152, 147153, 147154, 147155, 147156,  0 } },
  { "Xuen's Battlegear"                          ,     "tier20",     13,  1306,   20,   4,  10,  269, 242259, { 147151, 147152, 147153, 147154, 147155, 147156,  0 } },
  { "Stormheart Raiment"                         ,     "tier20",     13,  1303,   20,   2,  11,  102, 242231, { 147133, 147134, 147135, 147136, 147137, 147138,  0 } },
  { "Stormheart Raiment"                         ,     "tier20",     13,  1303,   20,   2,  11,  103, 242234, { 147133, 147134, 147135, 147136, 147137, 147138,  0 } },
  { "Stormheart Raiment"                         ,     "tier20",     13,  1303,   20,   2,  11,  104, 242236, { 147133, 147134, 147135, 147136, 147137, 147138,  0 } },
  { "Stormheart Raiment"                         ,     "tier20",     13,  1303,   20,   2,  11,  105, 242238, { 147133, 147134, 147135, 147136, 147137, 147138,  0 } },
  { "Stormheart Raiment"                         ,     "tier20",     13,  1303,   20,   4,  11,  102, 242233, { 147133, 147134, 147135, 147136, 147137, 147138,  0 } },
  { "Stormheart Raiment"                         ,     "tier20",     13,  1303,   20,   4,  11,  103, 242235, { 147133, 147134, 147135, 147136, 147137, 147138,  0 } },
  { "Stormheart Raiment"                         ,     "tier20",     13,  1303,   20,   4,  11,  104, 242237, { 147133, 147134, 147135, 147136, 147137, 147138,  0 } },
  { "Stormheart Raiment"                         ,     "tier20",     13,  1303,   20,   4,  11,  105, 242313, { 147133, 147134, 147135, 147136, 147137, 147138,  0 } },
  { "Demonbane Armor"                            ,     "tier20",     13,  1302,   20,   2,  12,  577, 242227, { 147127, 147128, 147129, 147130, 147131, 147132,  0 } },
  { "Demonbane Armor"                            ,     "tier20",     13,  1302,   20,   2,  12,  581, 242228, { 147127, 147128, 147129, 147130, 147131, 147132,  0 } },
  { "Demonbane Armor"                            ,     "tier20",     13,  1302,   20,   4,  12,  577, 242226, { 147127, 147128, 147129, 147130, 147131, 147132,  0 } },
  { "Demonbane Armor"                            ,     "tier20",     13,  1302,   20,   4,  12,  581, 242229, { 147127, 147128, 147129, 147130, 147131, 147132,  0 } },
  { "Juggernaut Battlegear"                      ,     "tier21",     14,  1319,   21,   2,   1,   71, 251878, { 152178, 152179, 152180, 152181, 152182, 152183,  0 } },
  { "Juggernaut Battlegear"                      ,     "tier21",     14,  1319,   21,   2,   1,   72, 251880, { 152178, 152179, 152180, 152181, 152182, 152183,  0 } },
  { "Juggernaut Battlegear"                      ,     "tier21",     14,  1319,   21,   2,   1,   73, 188635, { 152178, 152179, 152180, 152181, 152182, 152183,  0 } },
  { "Juggernaut Battlegear"                      ,     "tier21",     14,  1319,   21,   4,   1,   71, 251879, { 152178, 152179, 152180, 152181, 152182, 152183,  0 } },
  { "Juggernaut Battlegear"                      ,     "tier21",     14,  1319,   21,   4,   1,   73, 251883, { 152178, 152179, 152180, 152181, 152182, 152183,  0 } },
  { "Juggernaut Battlegear"                      ,     "tier21",     14,  1319,   21,   4,   1,   72, 200853, { 152178, 152179, 152180, 152181, 152182, 152183,  0 } },
  { "Light's Vanguard Battleplate"               ,     "tier21",     14,  1324,   21,   2,   2,   65, 251863, { 152148, 152149, 152150, 152151, 152152, 152153,  0 } },
  { "Light's Vanguard Battleplate"               ,     "tier21",     14,  1324,   21,   2,   2,   66, 251869, { 152148, 152149, 152150, 152151, 152152, 152153,  0 } },
  { "Light's Vanguard Battleplate"               ,     "tier21",     14,  1324,   21,   2,   2,   70, 251867, { 152148, 152149, 152150, 152151, 152152, 152153,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Light's Vanguard Battleplate"               ,     "tier21",     14,  1324,   21,   4,   2,   65, 251865, { 152148, 152149, 152150, 152151, 152152, 152153,  0 } },
  { "Light's Vanguard Battleplate"               ,     "tier21",     14,  1324,   21,   4,   2,   66, 251870, { 152148, 152149, 152150, 152151, 152152, 152153,  0 } },
  { "Light's Vanguard Battleplate"               ,     "tier21",     14,  1324,   21,   4,   2,   70, 251868, { 152148, 152149, 152150, 152151, 152152, 152153,  0 } },
  { "Serpentstalker Guise"                       ,     "tier21",     14,  1327,   21,   2,   3,  253, 251755, { 152130, 152131, 152132, 152133, 152134, 152135,  0 } },
  { "Serpentstalker Guise"                       ,     "tier21",     14,  1327,   21,   2,   3,  255,  70847, { 152130, 152131, 152132, 152133, 152134, 152135,  0 } },
  { "Serpentstalker Guise"                       ,     "tier21",     14,  1327,   21,   2,   3,  254,  70854, { 152130, 152131, 152132, 152133, 152134, 152135,  0 } },
  { "Serpentstalker Guise"                       ,     "tier21",     14,  1327,   21,   4,   3,  253, 251756, { 152130, 152131, 152132, 152133, 152134, 152135,  0 } },
  { "Serpentstalker Guise"                       ,     "tier21",     14,  1327,   21,   4,   3,  255, 203638, { 152130, 152131, 152132, 152133, 152134, 152135,  0 } },
  { "Serpentstalker Guise"                       ,     "tier21",     14,  1327,   21,   4,   3,  254, 214826, { 152130, 152131, 152132, 152133, 152134, 152135,  0 } },
  { "Regalia of the Dashing Scoundrel"           ,     "tier21",     14,  1322,   21,   2,   4,  259, 251776, { 152160, 152161, 152162, 152163, 152164, 152165,  0 } },
  { "Regalia of the Dashing Scoundrel"           ,     "tier21",     14,  1322,   21,   2,   4,  260, 251778, { 152160, 152161, 152162, 152163, 152164, 152165,  0 } },
  { "Regalia of the Dashing Scoundrel"           ,     "tier21",     14,  1322,   21,   2,   4,  261, 251785, { 152160, 152161, 152162, 152163, 152164, 152165,  0 } },
  { "Regalia of the Dashing Scoundrel"           ,     "tier21",     14,  1322,   21,   4,   4,  259, 251777, { 152160, 152161, 152162, 152163, 152164, 152165,  0 } },
  { "Regalia of the Dashing Scoundrel"           ,     "tier21",     14,  1322,   21,   4,   4,  260, 251783, { 152160, 152161, 152162, 152163, 152164, 152165,  0 } },
  { "Regalia of the Dashing Scoundrel"           ,     "tier21",     14,  1322,   21,   4,   4,  261, 251788, { 152160, 152161, 152162, 152163, 152164, 152165,  0 } },
  { "Gilded Seraph's Raiment"                    ,     "tier21",     14,  1323,   21,   2,   5,  256, 251843, { 152154, 152155, 152156, 152157, 152158, 152159,  0 } },
  { "Gilded Seraph's Raiment"                    ,     "tier21",     14,  1323,   21,   2,   5,  257, 251831, { 152154, 152155, 152156, 152157, 152158, 152159,  0 } },
  { "Gilded Seraph's Raiment"                    ,     "tier21",     14,  1323,   21,   2,   5,  258, 251845, { 152154, 152155, 152156, 152157, 152158, 152159,  0 } },
  { "Gilded Seraph's Raiment"                    ,     "tier21",     14,  1323,   21,   4,   5,  256, 251844, { 152154, 152155, 152156, 152157, 152158, 152159,  0 } },
  { "Gilded Seraph's Raiment"                    ,     "tier21",     14,  1323,   21,   4,   5,  257, 251842, { 152154, 152155, 152156, 152157, 152158, 152159,  0 } },
  { "Gilded Seraph's Raiment"                    ,     "tier21",     14,  1323,   21,   4,   5,  258, 251846, { 152154, 152155, 152156, 152157, 152158, 152159,  0 } },
  { "Dreadwake Armor"                            ,     "tier21",     14,  1330,   21,   2,   6,  250, 251876, { 152112, 152113, 152114, 152115, 152116, 152117,  0 } },
  { "Dreadwake Armor"                            ,     "tier21",     14,  1330,   21,   2,   6,  251, 251873, { 152112, 152113, 152114, 152115, 152116, 152117,  0 } },
  { "Dreadwake Armor"                            ,     "tier21",     14,  1330,   21,   2,   6,  252, 251871, { 152112, 152113, 152114, 152115, 152116, 152117,  0 } },
  { "Dreadwake Armor"                            ,     "tier21",     14,  1330,   21,   4,   6,  250, 251877, { 152112, 152113, 152114, 152115, 152116, 152117,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Dreadwake Armor"                            ,     "tier21",     14,  1330,   21,   4,   6,  251, 251875, { 152112, 152113, 152114, 152115, 152116, 152117,  0 } },
  { "Dreadwake Armor"                            ,     "tier21",     14,  1330,   21,   4,   6,  252, 251872, { 152112, 152113, 152114, 152115, 152116, 152117,  0 } },
  { "Garb of Venerated Spirits"                  ,     "tier21",     14,  1321,   21,   2,   7,  262, 251757, { 152166, 152167, 152168, 152169, 152170, 152171,  0 } },
  { "Garb of Venerated Spirits"                  ,     "tier21",     14,  1321,   21,   2,   7,  263, 251762, { 152166, 152167, 152168, 152169, 152170, 152171,  0 } },
  { "Garb of Venerated Spirits"                  ,     "tier21",     14,  1321,   21,   2,   7,  264, 251764, { 152166, 152167, 152168, 152169, 152170, 152171,  0 } },
  { "Garb of Venerated Spirits"                  ,     "tier21",     14,  1321,   21,   4,   7,  262, 251758, { 152166, 152167, 152168, 152169, 152170, 152171,  0 } },
  { "Garb of Venerated Spirits"                  ,     "tier21",     14,  1321,   21,   4,   7,  263, 251761, { 152166, 152167, 152168, 152169, 152170, 152171,  0 } },
  { "Garb of Venerated Spirits"                  ,     "tier21",     14,  1321,   21,   4,   7,  264, 251765, { 152166, 152167, 152168, 152169, 152170, 152171,  0 } },
  { "Runebound Regalia"                          ,     "tier21",     14,  1326,   21,   2,   8,   62, 251861, { 152136, 152137, 152138, 152139, 152140, 152141,  0 } },
  { "Runebound Regalia"                          ,     "tier21",     14,  1326,   21,   2,   8,   63, 251857, { 152136, 152137, 152138, 152139, 152140, 152141,  0 } },
  { "Runebound Regalia"                          ,     "tier21",     14,  1326,   21,   2,   8,   64, 251859, { 152136, 152137, 152138, 152139, 152140, 152141,  0 } },
  { "Runebound Regalia"                          ,     "tier21",     14,  1326,   21,   4,   8,   62, 251862, { 152136, 152137, 152138, 152139, 152140, 152141,  0 } },
  { "Runebound Regalia"                          ,     "tier21",     14,  1326,   21,   4,   8,   63, 251858, { 152136, 152137, 152138, 152139, 152140, 152141,  0 } },
  { "Runebound Regalia"                          ,     "tier21",     14,  1326,   21,   4,   8,   64, 251860, { 152136, 152137, 152138, 152139, 152140, 152141,  0 } },
  { "Grim Inquisitor's Regalia"                  ,     "tier21",     14,  1320,   21,   2,   9,  266, 251851, { 152172, 152173, 152174, 152175, 152176, 152177,  0 } },
  { "Grim Inquisitor's Regalia"                  ,     "tier21",     14,  1320,   21,   2,   9,  267, 251854, { 152172, 152173, 152174, 152175, 152176, 152177,  0 } },
  { "Grim Inquisitor's Regalia"                  ,     "tier21",     14,  1320,   21,   2,   9,  265, 251847, { 152172, 152173, 152174, 152175, 152176, 152177,  0 } },
  { "Grim Inquisitor's Regalia"                  ,     "tier21",     14,  1320,   21,   4,   9,  265, 251849, { 152172, 152173, 152174, 152175, 152176, 152177,  0 } },
  { "Grim Inquisitor's Regalia"                  ,     "tier21",     14,  1320,   21,   4,   9,  267, 251855, { 152172, 152173, 152174, 152175, 152176, 152177,  0 } },
  { "Grim Inquisitor's Regalia"                  ,     "tier21",     14,  1320,   21,   4,   9,  266, 211105, { 152172, 152173, 152174, 152175, 152176, 152177,  0 } },
  { "Chi'Ji's Battlegear"                        ,     "tier21",     14,  1325,   21,   2,  10,  268, 251829, { 152142, 152143, 152144, 152145, 152146, 152147,  0 } },
  { "Chi'Ji's Battlegear"                        ,     "tier21",     14,  1325,   21,   2,  10,  270, 251825, { 152142, 152143, 152144, 152145, 152146, 152147,  0 } },
  { "Chi'Ji's Battlegear"                        ,     "tier21",     14,  1325,   21,   2,  10,  269, 251823, { 152142, 152143, 152144, 152145, 152146, 152147,  0 } },
  { "Chi'Ji's Battlegear"                        ,     "tier21",     14,  1325,   21,   4,  10,  268, 251830, { 152142, 152143, 152144, 152145, 152146, 152147,  0 } },
  { "Chi'Ji's Battlegear"                        ,     "tier21",     14,  1325,   21,   4,  10,  270, 251826, { 152142, 152143, 152144, 152145, 152146, 152147,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Chi'Ji's Battlegear"                        ,     "tier21",     14,  1325,   21,   4,  10,  269, 251821, { 152142, 152143, 152144, 152145, 152146, 152147,  0 } },
  { "Bearmantle Battlegear"                      ,     "tier21",     14,  1328,   21,   2,  11,  102, 251803, { 152124, 152125, 152126, 152127, 152128, 152129,  0 } },
  { "Bearmantle Battlegear"                      ,     "tier21",     14,  1328,   21,   2,  11,  103, 251789, { 152124, 152125, 152126, 152127, 152128, 152129,  0 } },
  { "Bearmantle Battlegear"                      ,     "tier21",     14,  1328,   21,   2,  11,  104, 251791, { 152124, 152125, 152126, 152127, 152128, 152129,  0 } },
  { "Bearmantle Battlegear"                      ,     "tier21",     14,  1328,   21,   2,  11,  105, 251813, { 152124, 152125, 152126, 152127, 152128, 152129,  0 } },
  { "Bearmantle Battlegear"                      ,     "tier21",     14,  1328,   21,   4,  11,  102, 251809, { 152124, 152125, 152126, 152127, 152128, 152129,  0 } },
  { "Bearmantle Battlegear"                      ,     "tier21",     14,  1328,   21,   4,  11,  103, 251790, { 152124, 152125, 152126, 152127, 152128, 152129,  0 } },
  { "Bearmantle Battlegear"                      ,     "tier21",     14,  1328,   21,   4,  11,  104, 251792, { 152124, 152125, 152126, 152127, 152128, 152129,  0 } },
  { "Bearmantle Battlegear"                      ,     "tier21",     14,  1328,   21,   4,  11,  105, 251814, { 152124, 152125, 152126, 152127, 152128, 152129,  0 } },
  { "Felreaper Vestments"                        ,     "tier21",     14,  1329,   21,   2,  12,  577, 251767, { 152118, 152119, 152120, 152121, 152122, 152123,  0 } },
  { "Felreaper Vestments"                        ,     "tier21",     14,  1329,   21,   2,  12,  581, 251770, { 152118, 152119, 152120, 152121, 152122, 152123,  0 } },
  { "Felreaper Vestments"                        ,     "tier21",     14,  1329,   21,   4,  12,  577, 251769, { 152118, 152119, 152120, 152121, 152122, 152123,  0 } },
  { "Felreaper Vestments"                        ,     "tier21",     14,  1329,   21,   4,  12,  581, 251774, { 152118, 152119, 152120, 152121, 152122, 152123,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,   1,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,   2,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,   3,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,   4,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,   5,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,   6,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,   7,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,   8,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,   9,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,  10,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,  11,   -1, 277522, { 158362, 159631,  0 } },
  { "Waycrest Legacy"                            , "waycrests_legacy",     15,  1439,   21,   2,  12,   -1, 277522, { 158362, 159631,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,   1,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,   2,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,   3,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,   4,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,   5,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,   6,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,   7,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,   8,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,   9,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,  10,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,  11,   -1, 290263, { 165567, 165512,  0 } },
  { "Gift of the Loa"                            , "gift_of_the_loa",     16,  1442,   23,   2,  12,   -1, 290263, { 165567, 165512,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,   1,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,   2,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,   3,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,   4,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,   5,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,   6,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,   7,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,   8,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,   9,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,  10,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,  11,   -1, 290362, { 165604, 165566,  0 } },
  { "Keepsakes of the Resolute Commandant"       ,  "keepsakes",     17,  1443,   23,   2,  12,   -1, 290362, { 165604, 165566,  0 } },
};
#define RPPMMOD_SIZE (947)

// 947 RPPM Modifiers, wow build level 8.1.5.29705
static struct rppm_modifier_t __rppmmodifier_data[] = {
  {  17495,    4,  1,  1.0000 },
  {  27656,    4,  1,  1.0000 },
  {  29633,    4,  1,  1.0000 },
  {  29725,    4,  1,  1.0000 },
  {  45054,    4,  1,  1.0000 },
  {  49530,    1,  1,  1.0000 },
  {  51128,    4,  1,  1.0000 },
  {  51128,    4,  2,  1.0000 },
  {  60066,    4,  2,  1.0000 },
  {  60306,    4,  1,  1.0000 },
  {  64764,    4,  1,  1.0000 },
  { 104428,    4,  1,  1.0000 },
  { 104441,    4,  1,  1.0000 },
  { 118314,    4,  1,  1.0000 },
  { 119938,    4,  1,  1.0000 },
  { 126468,    4,  1,  1.0000 },
  { 137248,    0,  7, -1.0000 },
  { 137592,    0,  7, -1.0000 },
  { 137592,   62,  4, -0.2390 },
  { 137592,   63,  4, -0.2950 },
  { 137592,   64,  4,  0.3870 },
  { 137592,  265,  4, -0.3750 },
  { 137592,  266,  4, -0.4020 },
  { 137592,  267,  4, -0.4910 },
  { 137592,  262,  4,  0.8910 },
  { 137592,  102,  4,  0.8720 },
  { 137592,  258,  4, -0.0670 },
  { 137594,    0,  7, -1.0000 },
  { 137595,    4,  1,  1.0000 },
  { 137595,    0,  7, -1.0000 },
  { 137595,  260,  4,  0.1360 },
  { 137595,  259,  4,  0.7890 },
  { 137595,  261,  4,  0.1140 },
  { 137595,   71,  4,  0.3390 },
  { 137595,   72,  4,  0.2570 },
  { 137595,  103,  4,  0.7210 },
  { 137595,  251,  4,  0.5320 },
  { 137595,  252,  4, -0.1620 },
  { 137595,   70,  4,  0.2950 },
  { 137595,  263,  4, -0.1910 },
  { 137595,  269,  4,  0.0870 },
  { 137595,  253,  4, -0.0500 },
  { 137595,  254,  4,  0.1070 },
  { 137595,  255,  4, -0.0500 },
  { 137595,  250,  4, -0.4000 },
  { 137595,  104,  4, -0.4000 },
  { 137595,  268,  4, -0.4000 },
  { 137595,   66,  4, -0.4000 },
  { 137595,   73,  4, -0.4000 },
  { 138365,    4,  1,  1.0000 },
  { 138367,    4,  1,  1.0000 },
  { 138849,    4,  1,  1.0000 },
  { 138924,    4,  1,  1.0000 },
  { 138964,  528,  6,  1.0000 },
  { 138964,  102,  4, -0.3500 },
  { 138964,    0,  7, -0.7500 },
  { 138964,  256,  3, -0.4000 },
  { 138968,    4,  1,  1.0000 },
  { 139116,  528,  6,  1.0000 },
  { 139134,    4,  2,  1.0000 },
  { 139171,    4,  2,  1.0000 },
  { 139190,    4,  1,  1.0000 },
  { 146195,    4,  1,  1.0000 },
  { 146195,  250,  4, -0.4000 },
  { 146195,  104,  4, -0.4000 },
  { 146195,  268,  4, -0.4000 },
  { 146195,   66,  4, -0.4000 },
  { 146195,   73,  4, -0.4000 },
  { 146195,   71,  4,  0.3500 },
  { 146195,  253,  4,  0.0000 },
  { 146195,  260,  4,  0.1500 },
  { 146195,  259,  4,  0.5500 },
  { 146195,  263,  4,  0.5500 },
  { 146195,  103,  4,  0.3000 },
  { 146195,  251,  4,  0.5000 },
  { 146195,   72,  4,  0.0500 },
  { 146195,  254,  4,  0.2000 },
  { 146195,   70,  4,  0.4500 },
  { 146195,  261,  4,  0.0000 },
  { 146195,  255,  4,  0.1500 },
  { 146195,  252,  4,  0.0500 },
  { 146195,  269,  4,  0.2000 },
  { 146197,    4,  1,  1.0000 },
  { 146197,   62,  4,  0.2500 },
  { 146197,  266,  4,  0.2500 },
  { 146197,  265,  4,  0.1000 },
  { 146197,  267,  4,  0.1500 },
  { 146197,  262,  4,  0.0500 },
  { 146197,   63,  4,  0.2000 },
  { 146197,   64,  4,  0.2000 },
  { 146197,  102,  4,  0.1000 },
  { 146197,  258,  4,  0.0000 },
  { 146197,  250,  4, -0.7500 },
  { 146197,  104,  4, -0.7500 },
  { 146197,  268,  4, -0.7500 },
  { 146197,   66,  4, -0.7500 },
  { 146197,   73,  4, -0.7500 },
  { 146199,  105,  4, -0.2000 },
  { 146199,  270,  4, -0.2000 },
  { 146199,   65,  4,  0.1000 },
  { 146199,  256,  4,  0.4000 },
  { 146199,  257,  4,  0.0000 },
  { 146199,  264,  4, -0.3000 },
  { 146199,  250,  4, -0.7500 },
  { 146199,  104,  4, -0.7500 },
  { 146199,  268,  4, -0.7500 },
  { 146199,   66,  4, -0.7500 },
  { 146199,   73,  4, -0.7500 },
  { 146316,    4,  1,  1.0000 },
  { 159239,    4,  1,  1.0000 },
  { 160223,    0,  7, -1.0000 },
  { 162452,    4,  1,  1.0000 },
  { 165476,    4,  1,  1.0000 },
  { 166441,    4,  1,  1.0000 },
  { 179115,    8,  3,  0.1000 },
  { 179115,  512,  3, -0.1000 },
  { 179128,    4,  1,  1.0000 },
  { 179143,    4,  1,  1.0000 },
  { 183951,    4,  1,  1.0000 },
  { 183951,   62,  4, -0.4000 },
  { 183951,  258,  4, -0.1670 },
  { 184066,   62,  4, -0.4000 },
  { 184066,  258,  4, -0.1670 },
  { 184249,    4,  1,  1.0000 },
  { 187143,    4,  1,  1.0000 },
  { 187435,    4,  1,  1.0000 },
  { 188027,    4,  1,  1.0000 },
  { 188028,    4,  1,  1.0000 },
  { 188534,    4,  1,  1.0000 },
  { 190889,    4,  1,  1.0000 },
  { 190890,    4,  1,  1.0000 },
  { 191048,    4,  1,  1.0000 },
  { 192623,    4,  1,  1.0000 },
  { 193371,    4,  1,  1.0000 },
  { 196099,    5,  1,  1.0000 },
  { 196777,    4,  1,  1.0000 },
  { 197846,    4,  1,  1.0000 },
  { 198068,    4,  1,  1.0000 },
  { 198735,    4,  1,  1.0000 },
  { 199806,    4,  1,  1.0000 },
  { 201336,    4,  1,  1.0000 },
  { 201407,    4,  1,  1.0000 },
  { 201471,    4,  1,  1.0000 },
  { 201900,    4,  1,  1.0000 },
  { 202762,    4,  1,  1.0000 },
  { 202840,    4,  1,  1.0000 },
  { 203557,    4,  1,  1.0000 },
  { 206478,    4,  1,  1.0000 },
  { 207104,    4,  1,  1.0000 },
  { 207320,    4,  1,  1.0000 },
  { 207767,   71,  4, -0.4000 },
  { 208213,    4,  1,  1.0000 },
  { 208653,    0,  4,  0.0000 },
  { 209579,    4,  1,  1.0000 },
  { 211984,    4,  1,  1.0000 },
  { 213782,    4,  1,  1.0000 },
  { 213782,  252,  4, -0.3200 },
  { 214054,    4,  1,  1.0000 },
  { 214054,  263,  4, -0.2400 },
  { 214349,    4,  1,  1.0000 },
  { 214349,  252,  4, -0.3200 },
  { 214396,    4,  1,  1.0000 },
  { 214492,    4,  1,  1.0000 },
  { 214571,    4,  1,  1.0000 },
  { 214962,    4,  1,  1.0000 },
  { 215089,  252,  4, -0.3200 },
  { 215089,    4,  1,  1.0000 },
  { 215120,    4,  1,  1.0000 },
  { 215120,  263,  4, -0.2400 },
  { 215247,    4,  1,  1.0000 },
  { 215264,    4,  1,  1.0000 },
  { 215293,    4,  1,  1.0000 },
  { 215404,    4,  1,  1.0000 },
  { 215444,    4,  1,  1.0000 },
  { 215444,  252,  4, -0.3200 },
  { 215569,    4,  1,  1.0000 },
  { 215658,    4,  1,  1.0000 },
  { 215745,    4,  1,  1.0000 },
  { 215745,  253,  4, -0.2700 },
  { 215857,    4,  2,  1.0000 },
  { 216085,    4,  1,  1.0000 },
  { 216085,  263,  4, -0.2400 },
  { 216085,  253,  4, -0.2700 },
  { 221767,    4,  2,  1.0000 },
  { 221811,    4,  1,  1.0000 },
  { 221811,  263,  4, -0.2400 },
  { 221845,    3,  1,  1.0000 },
  { 221845,  252,  4, -0.3200 },
  { 221873,    4,  1,  1.0000 },
  { 222015,    4,  1,  1.0000 },
  { 222167,    4,  1,  1.0000 },
  { 222187,    4,  1,  1.0000 },
  { 222187,  263,  4, -0.2400 },
  { 222207,    4,  1,  1.0000 },
  { 222703,    4,  1,  1.0000 },
  { 222705,  252,  4, -0.3200 },
  { 224059,    4,  1,  1.0000 },
  { 224073,    4,  1,  1.0000 },
  { 224076,    4,  1,  1.0000 },
  { 224158,    4,  1,  1.0000 },
  { 225123, 4060,  3, -0.5000 },
  { 225123,  250,  4, -0.5000 },
  { 225123,   65,  4, -0.5000 },
  { 225123,   66,  4, -0.5000 },
  { 225123,   73,  4, -0.5000 },
  { 225123,    4,  1,  1.0000 },
  { 225125, 2607,  3, -0.5000 },
  { 225125,  103,  4, -0.5000 },
  { 225125,  104,  4, -0.5000 },
  { 225125,  105,  4, -0.5000 },
  { 225125,  257,  4, -0.5000 },
  { 225125,  263,  4, -0.5000 },
  { 225125,  264,  4, -0.5000 },
  { 225127,  435,  3, -0.5000 },
  { 225127,  581,  4, -0.5000 },
  { 225127,  102,  4, -0.5000 },
  { 225127,  104,  4, -0.5000 },
  { 225127,  105,  4, -0.5000 },
  { 225127,  253,  4, -0.5000 },
  { 225127,  254,  4, -0.5000 },
  { 225127,  268,  4, -0.5000 },
  { 225127,  270,  4, -0.5000 },
  { 225127,  262,  4, -0.5000 },
  { 225127,  264,  4, -0.5000 },
  { 225127,    4,  1,  1.0000 },
  { 225128, 2477,  3, -0.5000 },
  { 225128,  102,  4, -0.5000 },
  { 225128,  103,  4, -0.5000 },
  { 225128,  104,  4, -0.5000 },
  { 225128,  268,  4, -0.5000 },
  { 225128,  269,  4, -0.5000 },
  { 225128,   66,  4, -0.5000 },
  { 225128,   70,  4, -0.5000 },
  { 225128,  258,  4, -0.5000 },
  { 225128,  262,  4, -0.5000 },
  { 225128,  263,  4, -0.5000 },
  { 225128,    4,  1,  1.0000 },
  { 225129,  400,  3, -0.5000 },
  { 225129,  250,  4, -0.5000 },
  { 225129,  581,  4, -0.5000 },
  { 225129,  102,  4, -0.5000 },
  { 225129,  104,  4, -0.5000 },
  { 225129,  105,  4, -0.5000 },
  { 225129,  268,  4, -0.5000 },
  { 225129,  270,  4, -0.5000 },
  { 225129,   65,  4, -0.5000 },
  { 225129,   66,  4, -0.5000 },
  { 225129,  262,  4, -0.5000 },
  { 225129,  264,  4, -0.5000 },
  { 225129,   73,  4, -0.5000 },
  { 225131, 2603,  3, -0.5000 },
  { 225131,  103,  4, -0.5000 },
  { 225131,  104,  4, -0.5000 },
  { 225131,  105,  4, -0.5000 },
  { 225131,  255,  4, -0.5000 },
  { 225131,  257,  4, -0.5000 },
  { 225131,  263,  4, -0.5000 },
  { 225131,  264,  4, -0.5000 },
  { 225131,    4,  1,  1.0000 },
  { 225134, 2603,  3, -0.5000 },
  { 225134,  103,  4, -0.5000 },
  { 225134,  104,  4, -0.5000 },
  { 225134,  105,  4, -0.5000 },
  { 225134,  255,  4, -0.5000 },
  { 225134,  257,  4, -0.5000 },
  { 225134,  263,  4, -0.5000 },
  { 225134,  264,  4, -0.5000 },
  { 225134,    4,  1,  1.0000 },
  { 225135,  435,  3, -0.5000 },
  { 225135,  581,  4, -0.5000 },
  { 225135,  102,  4, -0.5000 },
  { 225135,  104,  4, -0.5000 },
  { 225135,  105,  4, -0.5000 },
  { 225135,  268,  4, -0.5000 },
  { 225135,  270,  4, -0.5000 },
  { 225135,  262,  4, -0.5000 },
  { 225135,  264,  4, -0.5000 },
  { 225135,  577,  4,  0.0000 },
  { 225135,  103,  4, -0.3000 },
  { 225135,  253,  4,  1.0000 },
  { 225135,  254,  4,  1.0000 },
  { 225135,  255,  4,  1.5000 },
  { 225135,  269,  4,  0.0000 },
  { 225135,    8,  3, -0.3000 },
  { 225135,  263,  4,  0.0000 },
  { 225136, 2477,  3, -0.5000 },
  { 225136,  102,  4, -0.5000 },
  { 225136,  103,  4, -0.5000 },
  { 225136,  104,  4, -0.5000 },
  { 225136,  268,  4, -0.5000 },
  { 225136,  269,  4, -0.5000 },
  { 225136,   66,  4, -0.5000 },
  { 225136,   70,  4, -0.5000 },
  { 225136,  258,  4, -0.5000 },
  { 225136,  262,  4, -0.5000 },
  { 225136,  263,  4, -0.5000 },
  { 225137, 2607,  3, -0.5000 },
  { 225137,  103,  4, -0.5000 },
  { 225137,  104,  4, -0.5000 },
  { 225137,  105,  4, -0.5000 },
  { 225137,  257,  4, -0.5000 },
  { 225137,  263,  4, -0.5000 },
  { 225137,  264,  4, -0.5000 },
  { 225137,    4,  1,  1.0000 },
  { 225138,    4,  1,  1.0000 },
  { 225138,  105,  4,  0.1200 },
  { 225138,  270,  4,  0.0300 },
  { 225138,  257,  4,  0.1800 },
  { 225138,  256,  4, -0.2400 },
  { 225138,   65,  4, -0.1300 },
  { 225138,  264,  4,  0.1200 },
  { 225139,  251,  4,  1.2000 },
  { 225139,  252,  4,  1.8000 },
  { 225139,  577,  4,  0.1200 },
  { 225139,  103,  4,  1.2400 },
  { 225139,  253,  4,  1.8000 },
  { 225139,  254,  4,  1.2400 },
  { 225139,  255,  4,  0.5000 },
  { 225139,  269,  4, -0.2000 },
  { 225139,   70,  4,  0.0000 },
  { 225139,  259,  4,  0.1700 },
  { 225139,  260,  4,  1.8000 },
  { 225139,  261,  4,  2.0000 },
  { 225139,  263,  4,  2.0000 },
  { 225139,   71,  4,  0.1000 },
  { 225139,   72,  4,  0.7200 },
  { 225142, 2607,  3, -0.5000 },
  { 225142,  103,  4, -0.5000 },
  { 225142,  104,  4, -0.5000 },
  { 225142,  105,  4, -0.5000 },
  { 225142,  257,  4, -0.5000 },
  { 225142,  263,  4, -0.5000 },
  { 225142,  264,  4, -0.5000 },
  { 225601,    4,  1,  1.0000 },
  { 225606,    4,  1,  1.0000 },
  { 227171,    4,  1,  1.0000 },
  { 227868,    4,  1,  1.0000 },
  { 228141,    4,  1,  1.0000 },
  { 228400,    4,  1,  1.0000 },
  { 228445,    4,  1,  1.0000 },
  { 230080,    4,  1,  1.0000 },
  { 230140,    4,  1,  1.0000 },
  { 230236,    4,  1,  1.0000 },
  { 230236,  263,  4, -0.5000 },
  { 230241,    4,  1,  1.0000 },
  { 230257,    4,  1,  1.0000 },
  { 230257,  263,  4, -0.5000 },
  { 230257, 2603,  3, -0.5000 },
  { 230257,  103,  4, -0.5000 },
  { 230257,  104,  4, -0.5000 },
  { 230257,  105,  4, -0.5000 },
  { 230257,  255,  4, -0.5000 },
  { 230257,  257,  4, -0.5000 },
  { 230257,  264,  4, -0.5000 },
  { 235003,    4,  1,  1.0000 },
  { 235011,    4,  1,  1.0000 },
  { 236167,    4,  1,  1.0000 },
  { 236545,    4,  1,  1.0000 },
  { 238498, 2093,  3, -0.5000 },
  { 238498,  103,  4, -0.5000 },
  { 238498,  104,  4, -0.5000 },
  { 238498,  263,  4, -0.5000 },
  { 238498,  268,  4, -0.5000 },
  { 238498,  269,  4, -0.5000 },
  { 238498,   70,  4, -0.5000 },
  { 238498,   66,  4, -0.5000 },
  { 238524,  435,  3, -0.5000 },
  { 238524,    4,  1,  1.0000 },
  { 242207,  476,  3, -0.5000 },
  { 242207,  251,  4, -0.5000 },
  { 242207,  252,  4, -0.5000 },
  { 242207,  577,  4, -0.5000 },
  { 242207,  102,  4, -0.5000 },
  { 242207,  103,  4, -0.5000 },
  { 242207,  105,  4, -0.5000 },
  { 242207,  270,  4, -0.5000 },
  { 242207,  269,  4, -0.5000 },
  { 242207,   65,  4, -0.5000 },
  { 242207,   70,  4, -0.5000 },
  { 242207,   71,  4, -0.5000 },
  { 242207,   72,  4, -0.5000 },
  { 242215,  400,  3, -0.5000 },
  { 242215,  250,  4, -0.5000 },
  { 242215,  581,  4, -0.5000 },
  { 242215,  102,  4, -0.5000 },
  { 242215,  104,  4, -0.5000 },
  { 242215,  105,  4, -0.5000 },
  { 242215,  253,  4, -0.5000 },
  { 242215,  254,  4, -0.5000 },
  { 242215,  268,  4, -0.5000 },
  { 242215,  270,  4, -0.5000 },
  { 242215,   65,  4, -0.5000 },
  { 242215,   66,  4, -0.5000 },
  { 242215,  262,  4, -0.5000 },
  { 242215,  264,  4, -0.5000 },
  { 242215,   73,  4, -0.5000 },
  { 242215,    4,  1,  1.0000 },
  { 242217,  400,  3, -0.5000 },
  { 242217,  250,  4, -0.5000 },
  { 242217,  581,  4, -0.5000 },
  { 242217,  102,  4, -0.5000 },
  { 242217,  104,  4, -0.5000 },
  { 242217,  105,  4, -0.5000 },
  { 242217,  253,  4, -0.5000 },
  { 242217,  254,  4, -0.5000 },
  { 242217,  268,  4, -0.5000 },
  { 242217,  270,  4, -0.5000 },
  { 242217,   65,  4, -0.5000 },
  { 242217,   66,  4, -0.5000 },
  { 242217,  262,  4, -0.5000 },
  { 242217,  264,  4, -0.5000 },
  { 242217,   73,  4, -0.5000 },
  { 242287,    4,  1,  1.0000 },
  { 242290,    4,  1,  1.0000 },
  { 242325, 2477,  3, -0.5000 },
  { 242325,  102,  4, -0.5000 },
  { 242325,  103,  4, -0.5000 },
  { 242325,  104,  4, -0.5000 },
  { 242325,  268,  4, -0.5000 },
  { 242325,  269,  4, -0.5000 },
  { 242325,   66,  4, -0.5000 },
  { 242325,   70,  4, -0.5000 },
  { 242325,  258,  4, -0.5000 },
  { 242325,  262,  4, -0.5000 },
  { 242325,  263,  4, -0.5000 },
  { 242325,    4,  1,  1.0000 },
  { 242524, 2603,  3, -0.5000 },
  { 242524,  103,  4, -0.5000 },
  { 242524,  104,  4, -0.5000 },
  { 242524,  105,  4, -0.5000 },
  { 242524,  255,  4, -0.5000 },
  { 242524,  257,  4, -0.5000 },
  { 242524,  263,  4, -0.5000 },
  { 242524,  264,  4, -0.5000 },
  { 242524,    4,  1,  1.0000 },
  { 242541, 2477,  3, -0.5000 },
  { 242541,  102,  4, -0.5000 },
  { 242541,  103,  4, -0.5000 },
  { 242541,  104,  4, -0.5000 },
  { 242541,  268,  4, -0.5000 },
  { 242541,  269,  4, -0.5000 },
  { 242541,   66,  4, -0.5000 },
  { 242541,   70,  4, -0.5000 },
  { 242541,  258,  4, -0.5000 },
  { 242541,  262,  4, -0.5000 },
  { 242541,  263,  4, -0.5000 },
  { 242570, 2603,  3, -0.5000 },
  { 242570,  103,  4, -0.5000 },
  { 242570,  104,  4, -0.5000 },
  { 242570,  105,  4, -0.5000 },
  { 242570,  255,  4, -0.5000 },
  { 242570,  257,  4, -0.5000 },
  { 242570,  263,  4, -0.5000 },
  { 242570,  264,  4, -0.5000 },
  { 242570,    4,  1,  1.0000 },
  { 242605, 2603,  3, -0.5000 },
  { 242605,  103,  4, -0.5000 },
  { 242605,  104,  4, -0.5000 },
  { 242605,  105,  4, -0.5000 },
  { 242605,  255,  4, -0.5000 },
  { 242605,  257,  4, -0.5000 },
  { 242605,  263,  4, -0.5000 },
  { 242605,  264,  4, -0.5000 },
  { 242605,    4,  1,  1.0000 },
  { 242611,  400,  3, -0.5000 },
  { 242611,  250,  4, -0.5000 },
  { 242611,  581,  4, -0.5000 },
  { 242611,  102,  4, -0.5000 },
  { 242611,  104,  4, -0.5000 },
  { 242611,  105,  4, -0.5000 },
  { 242611,  268,  4, -0.5000 },
  { 242611,  270,  4, -0.5000 },
  { 242611,   65,  4, -0.5000 },
  { 242611,   66,  4, -0.5000 },
  { 242611,  262,  4, -0.5000 },
  { 242611,  264,  4, -0.5000 },
  { 242611,   73,  4, -0.5000 },
  { 242616,  476,  3, -0.5000 },
  { 242616,  251,  4, -0.5000 },
  { 242616,  252,  4, -0.5000 },
  { 242616,  577,  4, -0.5000 },
  { 242616,  102,  4, -0.5000 },
  { 242616,  103,  4, -0.5000 },
  { 242616,  105,  4, -0.5000 },
  { 242616,  270,  4, -0.5000 },
  { 242616,  269,  4, -0.5000 },
  { 242616,   65,  4, -0.5000 },
  { 242616,   70,  4, -0.5000 },
  { 242616,   71,  4, -0.5000 },
  { 242616,   72,  4, -0.5000 },
  { 242616,    4,  1,  1.0000 },
  { 242628,  476,  3, -0.5000 },
  { 242628,  251,  4, -0.5000 },
  { 242628,  252,  4, -0.5000 },
  { 242628,  577,  4, -0.5000 },
  { 242628,  102,  4, -0.5000 },
  { 242628,  103,  4, -0.5000 },
  { 242628,  105,  4, -0.5000 },
  { 242628,  270,  4, -0.5000 },
  { 242628,  269,  4, -0.5000 },
  { 242628,   65,  4, -0.5000 },
  { 242628,   70,  4, -0.5000 },
  { 242628,   71,  4, -0.5000 },
  { 242628,   72,  4, -0.5000 },
  { 242628,    4,  1,  1.0000 },
  { 244034,    4,  1,  1.0000 },
  { 244041,    4,  1,  1.0000 },
  { 244134,    4,  1,  1.0000 },
  { 244193,    4,  1,  1.0000 },
  { 244193,  253,  4, -0.2700 },
  { 244330,    4,  1,  1.0000 },
  { 244381,    4,  1,  1.0000 },
  { 247625,    4,  1,  1.0000 },
  { 248168,    4,  1,  1.0000 },
  { 248171,    4,  1,  1.0000 },
  { 248173,    4,  1,  1.0000 },
  { 248175,    4,  1,  1.0000 },
  { 248198,    4,  1,  1.0000 },
  { 248199,    4,  1,  1.0000 },
  { 248200,    4,  1,  1.0000 },
  { 248218,    4,  1,  1.0000 },
  { 248256,    4,  1,  1.0000 },
  { 248257,    4,  1,  1.0000 },
  { 248260,    4,  1,  1.0000 },
  { 248262,    4,  1,  1.0000 },
  { 248264,    4,  1,  1.0000 },
  { 248265,    4,  1,  1.0000 },
  { 248266,    4,  1,  1.0000 },
  { 248267,    4,  1,  1.0000 },
  { 248268,    4,  1,  1.0000 },
  { 248269,    4,  1,  1.0000 },
  { 248274,    4,  1,  1.0000 },
  { 248276,    4,  1,  1.0000 },
  { 248277,    4,  1,  1.0000 },
  { 248278,    4,  1,  1.0000 },
  { 250075,    4,  1,  1.0000 },
  { 250091,    4,  1,  1.0000 },
  { 250091,  253,  4, -0.2700 },
  { 250093,    4,  1,  1.0000 },
  { 250768, 2477,  3, -0.5000 },
  { 250768,  102,  4, -0.5000 },
  { 250768,  103,  4, -0.5000 },
  { 250768,  104,  4, -0.5000 },
  { 250768,  268,  4, -0.5000 },
  { 250768,  269,  4, -0.5000 },
  { 250768,   66,  4, -0.5000 },
  { 250768,   70,  4, -0.5000 },
  { 250768,  258,  4, -0.5000 },
  { 250768,  262,  4, -0.5000 },
  { 250768,  263,  4, -0.5000 },
  { 250768,    4,  1,  1.0000 },
  { 250846, 2607,  3, -0.5000 },
  { 250846,  103,  4, -0.5000 },
  { 250846,  104,  4, -0.5000 },
  { 250846,  105,  4, -0.5000 },
  { 250846,  257,  4, -0.5000 },
  { 250846,  263,  4, -0.5000 },
  { 250846,  264,  4, -0.5000 },
  { 250846,    4,  1,  1.0000 },
  { 251316,    4,  1,  1.0000 },
  { 251875,    4,  1,  1.0000 },
  { 251925,  476,  3, -0.5000 },
  { 251925,  251,  4, -0.5000 },
  { 251925,  252,  4, -0.5000 },
  { 251925,  577,  4, -0.5000 },
  { 251925,  102,  4, -0.5000 },
  { 251925,  103,  4, -0.5000 },
  { 251925,  105,  4, -0.5000 },
  { 251925,  270,  4, -0.5000 },
  { 251925,  269,  4, -0.5000 },
  { 251925,   65,  4, -0.5000 },
  { 251925,   70,  4, -0.5000 },
  { 251925,   71,  4, -0.5000 },
  { 251925,   72,  4, -0.5000 },
  { 251925,    4,  1,  1.0000 },
  { 252207,    4,  1,  1.0000 },
  { 252875,    4,  1,  1.0000 },
  { 252888,    4,  1,  1.0000 },
  { 252922,    4,  1,  1.0000 },
  { 253070,    4,  1,  1.0000 },
  { 253093,    4,  1,  1.0000 },
  { 253111,    4,  1,  1.0000 },
  { 253242, 2603,  3, -0.5000 },
  { 253242,  103,  4, -0.5000 },
  { 253242,  104,  4, -0.5000 },
  { 253242,  105,  4, -0.5000 },
  { 253242,  255,  4, -0.5000 },
  { 253242,  257,  4, -0.5000 },
  { 253242,  263,  4, -0.5000 },
  { 253242,  264,  4, -0.5000 },
  { 253242,    4,  1,  1.0000 },
  { 253259, 2089,  3, -0.5000 },
  { 253259,  103,  4, -0.5000 },
  { 253259,  104,  4, -0.5000 },
  { 253259,  255,  4, -0.5000 },
  { 253259,  263,  4, -0.5000 },
  { 253259,  268,  4, -0.5000 },
  { 253259,  269,  4, -0.5000 },
  { 253259,   66,  4, -0.5000 },
  { 253259,   70,  4, -0.5000 },
  { 253259,    3,  2,  1.0000 },
  { 253263, 2603,  3, -0.5000 },
  { 253263,  103,  4, -0.5000 },
  { 253263,  104,  4, -0.5000 },
  { 253263,  105,  4, -0.5000 },
  { 253263,  255,  4, -0.5000 },
  { 253263,  257,  4, -0.5000 },
  { 253263,  263,  4, -0.5000 },
  { 253263,  264,  4, -0.5000 },
  { 253263,    4,  1,  1.0000 },
  { 253268, 2477,  3, -0.5000 },
  { 253268,  102,  4, -0.5000 },
  { 253268,  103,  4, -0.5000 },
  { 253268,  104,  4, -0.5000 },
  { 253268,  268,  4, -0.5000 },
  { 253268,  269,  4, -0.5000 },
  { 253268,   66,  4, -0.5000 },
  { 253268,   70,  4, -0.5000 },
  { 253268,  258,  4, -0.5000 },
  { 253268,  262,  4, -0.5000 },
  { 253268,  263,  4, -0.5000 },
  { 253285, 2477,  3, -0.5000 },
  { 253285,  102,  4, -0.5000 },
  { 253285,  103,  4, -0.5000 },
  { 253285,  104,  4, -0.5000 },
  { 253285,  268,  4, -0.5000 },
  { 253285,  269,  4, -0.5000 },
  { 253285,   66,  4, -0.5000 },
  { 253285,   70,  4, -0.5000 },
  { 253285,  258,  4, -0.5000 },
  { 253285,  262,  4, -0.5000 },
  { 253285,  263,  4, -0.5000 },
  { 253285,    4,  1,  1.0000 },
  { 253304,  400,  3, -0.5000 },
  { 253304,  250,  4, -0.5000 },
  { 253304,  581,  4, -0.5000 },
  { 253304,  102,  4, -0.5000 },
  { 253304,  104,  4, -0.5000 },
  { 253304,  105,  4, -0.5000 },
  { 253304,  268,  4, -0.5000 },
  { 253304,  270,  4, -0.5000 },
  { 253304,   65,  4, -0.5000 },
  { 253304,   66,  4, -0.5000 },
  { 253304,  262,  4, -0.5000 },
  { 253304,  264,  4, -0.5000 },
  { 253304,   73,  4, -0.5000 },
  { 253305,  400,  3, -0.5000 },
  { 253305,  250,  4, -0.5000 },
  { 253305,  581,  4, -0.5000 },
  { 253305,  102,  4, -0.5000 },
  { 253305,  104,  4, -0.5000 },
  { 253305,  105,  4, -0.5000 },
  { 253305,  268,  4, -0.5000 },
  { 253305,  270,  4, -0.5000 },
  { 253305,   65,  4, -0.5000 },
  { 253305,   66,  4, -0.5000 },
  { 253305,  262,  4, -0.5000 },
  { 253305,  264,  4, -0.5000 },
  { 253305,   73,  4, -0.5000 },
  { 253310,  400,  3, -0.5000 },
  { 253310,  250,  4, -0.5000 },
  { 253310,  581,  4, -0.5000 },
  { 253310,  102,  4, -0.5000 },
  { 253310,  104,  4, -0.5000 },
  { 253310,  105,  4, -0.5000 },
  { 253310,  268,  4, -0.5000 },
  { 253310,  270,  4, -0.5000 },
  { 253310,   65,  4, -0.5000 },
  { 253310,   66,  4, -0.5000 },
  { 253310,  262,  4, -0.5000 },
  { 253310,  264,  4, -0.5000 },
  { 253310,   73,  4, -0.5000 },
  { 253310,    4,  1,  1.0000 },
  { 253323,  400,  3, -0.5000 },
  { 253323,  250,  4, -0.5000 },
  { 253323,  581,  4, -0.5000 },
  { 253323,  102,  4, -0.5000 },
  { 253323,  104,  4, -0.5000 },
  { 253323,  105,  4, -0.5000 },
  { 253323,  253,  4, -0.5000 },
  { 253323,  254,  4, -0.5000 },
  { 253323,  268,  4, -0.5000 },
  { 253323,  270,  4, -0.5000 },
  { 253323,   65,  4, -0.5000 },
  { 253323,   66,  4, -0.5000 },
  { 253323,  262,  4, -0.5000 },
  { 253323,  264,  4, -0.5000 },
  { 253323,   73,  4, -0.5000 },
  { 253323,    4,  1,  1.0000 },
  { 253326,  400,  3, -0.5000 },
  { 253326,  250,  4, -0.5000 },
  { 253326,  581,  4, -0.5000 },
  { 253326,  102,  4, -0.5000 },
  { 253326,  104,  4, -0.5000 },
  { 253326,  105,  4, -0.5000 },
  { 253326,  253,  4, -0.5000 },
  { 253326,  254,  4, -0.5000 },
  { 253326,  268,  4, -0.5000 },
  { 253326,  270,  4, -0.5000 },
  { 253326,   65,  4, -0.5000 },
  { 253326,   66,  4, -0.5000 },
  { 253326,  262,  4, -0.5000 },
  { 253326,  264,  4, -0.5000 },
  { 253326,   73,  4, -0.5000 },
  { 253326,    4,  1,  1.0000 },
  { 253793,    4,  1,  1.0000 },
  { 255818, 2603,  3, -0.5000 },
  { 255818,  103,  4, -0.5000 },
  { 255818,  104,  4, -0.5000 },
  { 255818,  105,  4, -0.5000 },
  { 255818,  255,  4, -0.5000 },
  { 255818,  257,  4, -0.5000 },
  { 255818,  263,  4, -0.5000 },
  { 255818,  264,  4, -0.5000 },
  { 255818,    4,  1,  1.0000 },
  { 256067,    4,  1,  1.0000 },
  { 256815,  476,  3, -0.5000 },
  { 256815,  251,  4, -0.5000 },
  { 256815,  252,  4, -0.5000 },
  { 256815,  577,  4, -0.5000 },
  { 256815,  102,  4, -0.5000 },
  { 256815,  103,  4, -0.5000 },
  { 256815,  105,  4, -0.5000 },
  { 256815,  270,  4, -0.5000 },
  { 256815,  269,  4, -0.5000 },
  { 256815,   65,  4, -0.5000 },
  { 256815,   70,  4, -0.5000 },
  { 256815,   71,  4, -0.5000 },
  { 256815,   72,  4, -0.5000 },
  { 256819,  435,  3, -0.5000 },
  { 256819,  581,  4, -0.5000 },
  { 256819,  102,  4, -0.5000 },
  { 256819,  104,  4, -0.5000 },
  { 256819,  105,  4, -0.5000 },
  { 256819,  268,  4, -0.5000 },
  { 256819,  270,  4, -0.5000 },
  { 256819,  262,  4, -0.5000 },
  { 256819,  264,  4, -0.5000 },
  { 256819,    4,  1,  1.0000 },
  { 256822, 2477,  3, -0.5000 },
  { 256822,  102,  4, -0.5000 },
  { 256822,  103,  4, -0.5000 },
  { 256822,  104,  4, -0.5000 },
  { 256822,  268,  4, -0.5000 },
  { 256822,  269,  4, -0.5000 },
  { 256822,   66,  4, -0.5000 },
  { 256822,   70,  4, -0.5000 },
  { 256822,  258,  4, -0.5000 },
  { 256822,  262,  4, -0.5000 },
  { 256822,  263,  4, -0.5000 },
  { 256822,    4,  1,  1.0000 },
  { 256825, 4060,  3, -0.5000 },
  { 256825,  250,  4, -0.5000 },
  { 256825,   65,  4, -0.5000 },
  { 256825,   66,  4, -0.5000 },
  { 256825,   73,  4, -0.5000 },
  { 256827, 2607,  3, -0.5000 },
  { 256827,  103,  4, -0.5000 },
  { 256827,  104,  4, -0.5000 },
  { 256827,  105,  4, -0.5000 },
  { 256827,  257,  4, -0.5000 },
  { 256827,  263,  4, -0.5000 },
  { 256827,  264,  4, -0.5000 },
  { 258880,    4,  1,  1.0000 },
  { 258885,    4,  1,  1.0000 },
  { 258896,    4,  1,  1.0000 },
  { 258907,    4,  1,  1.0000 },
  { 258933,    4,  1,  1.0000 },
  { 258942,    4,  1,  1.0000 },
  { 258946,    4,  1,  1.0000 },
  { 258954,    4,  1,  1.0000 },
  { 258969,    4,  1,  1.0000 },
  { 258972,    4,  1,  1.0000 },
  { 258976,    4,  1,  1.0000 },
  { 258981,    4,  1,  1.0000 },
  { 258984,    4,  1,  1.0000 },
  { 258989,    4,  1,  1.0000 },
  { 258993,    4,  1,  1.0000 },
  { 259003,    4,  1,  1.0000 },
  { 259004,    4,  1,  1.0000 },
  { 259005,    4,  1,  1.0000 },
  { 259006,    4,  1,  1.0000 },
  { 259007,    4,  1,  1.0000 },
  { 259008,    4,  1,  1.0000 },
  { 259009,    4,  1,  1.0000 },
  { 262505,  400,  3, -0.5000 },
  { 262505,  250,  4, -0.5000 },
  { 262505,  581,  4, -0.5000 },
  { 262505,  102,  4, -0.5000 },
  { 262505,  104,  4, -0.5000 },
  { 262505,  105,  4, -0.5000 },
  { 262505,  268,  4, -0.5000 },
  { 262505,  270,  4, -0.5000 },
  { 262505,   65,  4, -0.5000 },
  { 262505,   66,  4, -0.5000 },
  { 262505,  262,  4, -0.5000 },
  { 262505,  264,  4, -0.5000 },
  { 262505,   73,  4, -0.5000 },
  { 262507, 4060,  3, -0.5000 },
  { 262507,  250,  4, -0.5000 },
  { 262507,   65,  4, -0.5000 },
  { 262507,   66,  4, -0.5000 },
  { 262507,   73,  4, -0.5000 },
  { 262507,    4,  1,  1.0000 },
  { 265000,    4,  1,  1.0000 },
  { 265003,    4,  1,  1.0000 },
  { 265025,    4,  1,  1.0000 },
  { 265027,    4,  1,  1.0000 },
  { 265034,    4,  1,  1.0000 },
  { 265036,    4,  1,  1.0000 },
  { 265057,    4,  1,  1.0000 },
  { 265066,    4,  1,  1.0000 },
  { 265071,    4,  1,  1.0000 },
  { 265072,    4,  1,  1.0000 },
  { 265073,    4,  1,  1.0000 },
  { 265078,    4,  1,  1.0000 },
  { 265079,    4,  1,  1.0000 },
  { 265082,    4,  1,  1.0000 },
  { 265083,    4,  1,  1.0000 },
  { 265162,    4,  1,  1.0000 },
  { 265167,    4,  1,  1.0000 },
  { 265170,    4,  1,  1.0000 },
  { 265174,    4,  1,  1.0000 },
  { 265176,    4,  1,  1.0000 },
  { 265230,    4,  1,  1.0000 },
  { 265233,    4,  1,  1.0000 },
  { 265234,    4,  1,  1.0000 },
  { 265236,    4,  1,  1.0000 },
  { 265238,    4,  1,  1.0000 },
  { 265240,    4,  1,  1.0000 },
  { 265255,    4,  1,  1.0000 },
  { 265266,    4,  1,  1.0000 },
  { 265270,    4,  1,  1.0000 },
  { 265274,    4,  1,  1.0000 },
  { 265276,    4,  1,  1.0000 },
  { 265278,    4,  1,  1.0000 },
  { 265282,    4,  1,  1.0000 },
  { 265310,    4,  1,  1.0000 },
  { 265317,    4,  1,  1.0000 },
  { 265327,    4,  1,  1.0000 },
  { 265331,    4,  1,  1.0000 },
  { 265353,    4,  1,  1.0000 },
  { 265408,    4,  1,  1.0000 },
  { 265413,    4,  1,  1.0000 },
  { 265414,    4,  1,  1.0000 },
  { 265416,    4,  1,  1.0000 },
  { 265419,    4,  1,  1.0000 },
  { 265421,    4,  1,  1.0000 },
  { 265950,    4,  1,  1.0000 },
  { 267177,    4,  1,  1.0000 },
  { 267344,    1,  1,  1.0000 },
  { 268035,    4,  1,  1.0000 },
  { 268501,    4,  1,  1.0000 },
  { 268512,    4,  1,  1.0000 },
  { 268516,    4,  1,  1.0000 },
  { 268517,    4,  1,  1.0000 },
  { 268531,    4,  1,  1.0000 },
  { 268553,    4,  1,  1.0000 },
  { 269009,    4,  1,  1.0000 },
  { 269066,    4,  1,  1.0000 },
  { 269853,    4,  1,  1.0000 },
  { 269891,    4,  1,  1.0000 },
  { 270668,    4,  1,  1.0000 },
  { 270809,    4,  1,  1.0000 },
  { 270921,    4,  1,  1.0000 },
  { 271190,    4,  1,  1.0000 },
  { 271547,    4,  1,  1.0000 },
  { 271631,    4,  1,  1.0000 },
  { 271681,    4,  1,  1.0000 },
  { 271683,    4,  1,  1.0000 },
  { 271705,    4,  1,  1.0000 },
  { 271818,    4,  1,  1.0000 },
  { 272258,    4,  1,  1.0000 },
  { 272275,    4,  1,  1.0000 },
  { 272797,    4,  1,  1.0000 },
  { 272817,    4,  1,  1.0000 },
  { 273153,    4,  1,  1.0000 },
  { 273389,    4,  1,  1.0000 },
  { 273835,    4,  1,  1.0000 },
  { 274071,    4,  1,  1.0000 },
  { 274072,    4,  1,  1.0000 },
  { 274145,    4,  1,  1.0000 },
  { 274219,    4,  1,  1.0000 },
  { 274225,    4,  1,  1.0000 },
  { 274233,    4,  1,  1.0000 },
  { 274239,    4,  1,  1.0000 },
  { 274458,    4,  1,  1.0000 },
  { 274477,    4,  1,  1.0000 },
  { 274484,    4,  1,  1.0000 },
  { 274790,    4,  1,  1.0000 },
  { 275460,    4,  1,  1.0000 },
  { 276123,    4,  1,  1.0000 },
  { 276135,    4,  1,  1.0000 },
  { 276176,    4,  1,  1.0000 },
  { 277522,    4,  1,  1.0000 },
  { 277905,    4,  1,  1.0000 },
  { 277926,    4,  1,  1.0000 },
  { 278053,    4,  1,  1.0000 },
  { 278112,    4,  1,  1.0000 },
  { 278140,    4,  1,  1.0000 },
  { 278152,    4,  1,  1.0000 },
  { 278161,    4,  1,  1.0000 },
  { 278824,    4,  1,  1.0000 },
  { 278874,    4,  1,  1.0000 },
  { 278876,    4,  1,  1.0000 },
  { 278880,    4,  1,  1.0000 },
  { 278897,    4,  1,  1.0000 },
  { 278908,    4,  1,  1.0000 },
  { 278911,    4,  1,  1.0000 },
  { 279426,    4,  1,  1.0000 },
  { 279449,    4,  1,  1.0000 },
  { 279786,    4,  1,  1.0000 },
  { 279955,    4,  1,  1.0000 },
  { 280163,    4,  1,  1.0000 },
  { 280168,    4,  1,  1.0000 },
  { 280383,    4,  1,  1.0000 },
  { 280702,    4,  1,  1.0000 },
  { 280721,    4,  1,  1.0000 },
  { 280816,    4,  1,  1.0000 },
  { 280837,    4,  1,  1.0000 },
  { 280854,    4,  1,  1.0000 },
  { 281712,    4,  1,  1.0000 },
  { 284971,    4,  1,  1.0000 },
  { 285023,    4,  1,  1.0000 },
  { 286392,    4,  1,  1.0000 },
  { 286586,    4,  1,  1.0000 },
  { 288109,    4,  1,  1.0000 },
  { 288328,    4,  1,  1.0000 },
  { 288757,    4,  1,  1.0000 },
  { 288804,    4,  1,  1.0000 },
  { 289287,    4,  1,  1.0000 },
  { 289289,    4,  1,  1.0000 },
  { 289391,    4,  1,  1.0000 },
  { 292359,    4,  1,  1.0000 },
  { 292360,    4,  1,  1.0000 },
  { 292361,    4,  1,  1.0000 },
  { 292363,    4,  1,  1.0000 },
  { 292364,    4,  1,  1.0000 },
  { 292824,    4,  1,  1.0000 },
  { 292975,    4,  1,  1.0000 },
  { 292991,    4,  1,  1.0000 },
  { 295157,    4,  1,  1.0000 },
  { 295254,    4,  1,  1.0000 },
  { 295280,    4,  1,  1.0000 },
  { 295391,    4,  1,  1.0000 },
  { 295427,    4,  1,  1.0000 },
  { 295812,    4,  1,  1.0000 },
  {      0,    0,  0,  0.0000 },
};
