#define PTR_CLASS_ABILITY_SIZE (51)
#define PTR_CLASS_ABILITY_TREE_SIZE (6)

#ifndef ptr_MAX_CLASS
#define ptr_MAX_CLASS (13)
#endif
// Class based active abilities, wow build 8.3.0.34220
static unsigned __ptr_class_ability_data[][PTR_CLASS_ABILITY_TREE_SIZE][PTR_CLASS_ABILITY_SIZE] = {
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
    // General tree, 22 abilities
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
      309328, // Hex (Living Honey)
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
    // General tree, 18 abilities
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
      169340, // Touch of Fatality
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
    // Windwalker tree, 28 abilities
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
      290461, // Reverse Harm
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
#define PTR_CLASS_MASTERY_ABILITY_SIZE (1)

// Class mastery abilities, wow build 8.3.0.34220
static unsigned __ptr_class_mastery_ability_data[MAX_CLASS][MAX_SPECS_PER_CLASS][PTR_CLASS_MASTERY_ABILITY_SIZE] = {
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
#define PTR_RACE_ABILITY_SIZE (7)

#ifndef ptr_MAX_RACE
#define ptr_MAX_RACE (33)
#endif

// Racial abilities, wow build 8.3.0.34220
static unsigned __ptr_race_ability_data[ptr_MAX_RACE][ptr_MAX_CLASS][PTR_RACE_ABILITY_SIZE] = {
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
  // Racial abilities for Vulpera
  {
    // Generic racial abilities
    {
      317795, // Alpaca Saddlebags
      312411, // Bag of Tricks
      312198, // Fire Resistance
      312370, // Make Camp
      312215, // Nose For Trouble
      312372, // Return to Camp
      312425, // Rummage Your Bag
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
  // Racial abilities for Mechagnome
  {
    // Generic racial abilities
    {
      312923, // Combat Analysis
      312916, // Emergency Failsafe
      312924, // Hyper Organic Light Originator
      312896, // Mastercraft
      312890, // Skeleton Pinkie
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
  // Racial abilities for Void Elf
  {
    // Generic racial abilities
    {
      255668, // Chill of Night
      255669, // Entropic Embrace
      255667, // Ethereal Connection
      255670, // Preternatural Calm
      256948, // Spatial Rift
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
