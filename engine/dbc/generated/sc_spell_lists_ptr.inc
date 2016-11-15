#define PTR_CLASS_ABILITY_SIZE (30)
#define PTR_CLASS_ABILITY_TREE_SIZE (6)

#ifndef ptr_MAX_CLASS
#define ptr_MAX_CLASS (13)
#endif
// Class based active abilities, wow build 23038
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
    // General tree, 13 abilities
    {
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
        1719, // Battle Cry
       18499, // Berserker Rage
      227847, // Bladestorm
         845, // Cleave
      167105, // Colossus Smash
       97462, // Commanding Shout
      118038, // Die by the Sword
      163201, // Execute
        1715, // Hamstring
        5246, // Intimidating Shout
       12294, // Mortal Strike
        1464, // Slam
       34428, // Victory Rush
        1680, // Whirlwind
           0,
    },
    // Fury tree, 12 abilities
    {
        1719, // Battle Cry
       18499, // Berserker Rage
       23881, // Bloodthirst
       97462, // Commanding Shout
      184364, // Enraged Regeneration
        5308, // Execute
      100130, // Furious Slash
        5246, // Intimidating Shout
       12323, // Piercing Howl
       85288, // Raging Blow
      184367, // Rampage
      190411, // Whirlwind
           0,
    },
    // Protection tree, 14 abilities
    {
        1719, // Battle Cry
       18499, // Berserker Rage
        1160, // Demoralizing Shout
       20243, // Devastate
      190456, // Ignore Pain
      198304, // Intercept
       12975, // Last Stand
        6572, // Revenge
        2565, // Shield Block
       23922, // Shield Slam
         871, // Shield Wall
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
    // General tree, 17 abilities
    {
       86704, // Ancient Fury
      114919, // Arcing Light
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
       20271, // Judgment
         633, // Lay on Hands
      200652, // Tyr's Deliverance (Artifact)
      200654, // Tyr's Deliverance
      205273, // Wake of Ashes (Artifact)
           0,
    },
    // Holy tree, 15 abilities
    {
      212056, // Absolution
       31821, // Aura Mastery
       31842, // Avenging Wrath
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
       85222, // Light of Dawn
      183998, // Light of the Martyr
           0,
    },
    // Protection tree, 15 abilities
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
      203528, // Greater Blessing of Might
      203539, // Greater Blessing of Wisdom
         853, // Hammer of Justice
      183218, // Hand of Hindrance
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
    // General tree, 24 abilities
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
        6991, // Feed Pet
        5384, // Feign Death
      125050, // Fetch
      127933, // Fireworks
        1543, // Flare
      203415, // Fury of the Eagle (Artifact)
      212621, // Marked Shot
         136, // Mend Pet
         982, // Revive Pet
      175686, // Stopping Power
        1515, // Tame Beast
      207097, // Titan's Thunder
      207068, // Titan's Thunder (Artifact)
      210000, // Wake Up
      204147, // Windburst (Artifact)
           0,
    },
    // Beast Mastery tree, 12 abilities
    {
      193530, // Aspect of the Wild
       19574, // Bestial Wrath
      193455, // Cobra Shot
        5116, // Concussive Shot
      147362, // Counter Shot
      120679, // Dire Beast
      109304, // Exhilaration
      187650, // Freezing Trap
       34026, // Kill Command
       34477, // Misdirection
        2643, // Multi-Shot
      187698, // Tar Trap
           0,
    },
    // Marksmanship tree, 13 abilities
    {
       19434, // Aimed Shot
      185358, // Arcane Shot
      186387, // Bursting Shot
      193455, // Cobra Shot
        5116, // Concussive Shot
      147362, // Counter Shot
      109304, // Exhilaration
      187650, // Freezing Trap
      185901, // Marked Shot
       34477, // Misdirection
        2643, // Multi-Shot
      187698, // Tar Trap
      193526, // Trueshot
           0,
    },
    // Survival tree, 14 abilities
    {
      186289, // Aspect of the Eagle
      187708, // Carve
      109304, // Exhilaration
      191433, // Explosive Trap
      202800, // Flanking Strike
      187650, // Freezing Trap
      190925, // Harpoon
      193265, // Hatchet Toss
      185855, // Lacerate
      190928, // Mongoose Bite
      187707, // Muzzle
      186270, // Raptor Strike
      187698, // Tar Trap
      195645, // Wing Clip
           0,
    },
    {
           0,
    },
    // Pet tree, 30 abilities
    {
      160011, // Agile Reflexes (Special Ability)
       90355, // Ancient Hysteria (Exotic Ability)
       50433, // Ankle Crack (Special Ability)
       17253, // Bite (Basic Attack)
       24423, // Bloody Screech (Special Ability)
       93433, // Burrow Attack (Exotic Ability)
       16827, // Claw (Basic Attack)
      160060, // Deadly Sting (Special Ability)
      159956, // Dust of Life (Exotic Ability)
      159953, // Feast (Special Ability)
       54644, // Frost Breath (Special Ability)
       92380, // Froststorm Breath (Exotic Ability)
        2649, // Growl (Basic Ability)
      159926, // Harden Shell (Special Ability)
       19577, // Intimidation
       24394, // Intimidation
      138374, // Lightning Blast
       54680, // Monstrous Bite (Exotic Ability)
      160452, // Netherwinds
       90347, // Play (Bonus Ability)
       24450, // Prowl (Bonus Ability)
       94019, // Rest (Bonus Ability)
       26064, // Shell Shield (Special Ability)
       49966, // Smack (Basic Attack)
       90361, // Spirit Mend (Exotic Ability)
       90328, // Spirit Walk (Bonus Ability)
      160065, // Tendon Rip (Special Ability)
       94022, // Trick (Bonus Ability)
       35346, // Warp Time (Special Ability)
      160067, // Web Spray (Special Ability)
    },
  },
  // Class active abilities for Rogue
  {
    // General tree, 17 abilities
    {
       22482, // Blade Flurry
       31224, // Cloak of Shadows
      212205, // Create: Crimson Vial (Honor Talent)
      185311, // Crimson Vial
      202665, // Curse of the Dreadblades (Artifact)
        2818, // Deadly Poison
       56814, // Detection
        1966, // Feint
      209782, // Goremaw's Bite (Artifact)
        1766, // Kick
      192759, // Kingsbane (Artifact)
      112974, // Leeching Poison (Talent)
        1804, // Pick Lock
      114018, // Shroud of Concealment
        2983, // Sprint
       57934, // Tricks of the Trade
        8680, // Wound Poison
           0,
    },
    // Assassination tree, 20 abilities
    {
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
      199740, // Bribe
        1833, // Cheap Shot
        1725, // Distract
        1776, // Gouge
      185763, // Pistol Shot
      199754, // Riposte
      193316, // Roll the Bones
        2098, // Run Through
      193315, // Saber Slash
        6770, // Sap
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
    // General tree, 13 abilities
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
        9484, // Shackle Undead
         585, // Smite
      215769, // Spirit of Redemption (Honor Talent)
      205065, // Void Torrent (Artifact)
           0,
    },
    // Discipline tree, 16 abilities
    {
        2061, // Flash Heal
       73325, // Leap of Faith
        1706, // Levitate
      212036, // Mass Resurrection
       33206, // Pain Suppression
       47540, // Penance
      200829, // Plea
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
    // Holy tree, 17 abilities
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
         527, // Purify
         139, // Renew
           0,
    },
    // Shadow tree, 18 abilities
    {
       47585, // Dispersion
        2061, // Flash Heal
        1706, // Levitate
        8092, // Mind Blast
       15407, // Mind Flay
      217676, // Mind Spike Detonation
          17, // Power Word: Shield
        8122, // Psychic Scream
      213634, // Purify Disease
      186263, // Shadow Mend
       32379, // Shadow Word: Death
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
    // General tree, 16 abilities
    {
       48707, // Anti-Magic Shell
      220143, // Apocalypse (Artifact)
      205223, // Consumption (Artifact)
      111673, // Control Undead
      127344, // Corpse Explosion
       56222, // Dark Command
       50977, // Death Gate
       49576, // Death Grip
       49998, // Death Strike
       47568, // Empower Rune Weapon
      190780, // Frost Breath
       57330, // Horn of Winter
       47528, // Mind Freeze
        3714, // Path of Frost
       61999, // Raise Ally
      190778, // Sindragosa's Fury (Artifact)
           0,
    },
    // Blood tree, 11 abilities
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
      212552, // Wraith Walk
           0,
    },
    // Frost tree, 9 abilities
    {
       45524, // Chains of Ice
       47568, // Empower Rune Weapon
       49143, // Frost Strike
       49184, // Howling Blast
       48792, // Icebound Fortitude
       49020, // Obliterate
       51271, // Pillar of Frost
      196770, // Remorseless Winter
      212552, // Wraith Walk
           0,
    },
    // Unholy tree, 12 abilities
    {
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
       49206, // Summon Gargoyle
      212552, // Wraith Walk
           0,
    },
    {
           0,
    },
    // Pet tree, 10 abilities
    {
      207260, // Arrow Spray
       81280, // Blood Burst
       47468, // Claw
      212333, // Cleaver
       51963, // Gargoyle Strike
       47481, // Gnaw
       47484, // Huddle
       47482, // Leap
      212423, // Skulker Shot
      198715, // Val'kyr Strike
           0,
    },
  },
  // Class active abilities for Shaman
  {
    // General tree, 18 abilities
    {
      114911, // Ancestral Guidance
        2008, // Ancestral Spirit
      108271, // Astral Shift
        2825, // Bloodlust
      216965, // Disabling Totem
      204945, // Doom Winds (Artifact)
        2484, // Earthbind Totem (Earth Totem)
        2645, // Ghost Wolf
      207778, // Gift of the Queen (Artifact)
       32182, // Heroism
      211004, // Hex
      211010, // Hex
      211015, // Hex
       51514, // Hex
      210873, // Hex
         370, // Purge
      205495, // Stormkeeper (Artifact)
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
    // Enhancement tree, 12 abilities
    {
       51886, // Cleanse Spirit
      187874, // Crash Lightning
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
    // Restoration tree, 16 abilities
    {
      212048, // Ancestral Vision
        1064, // Chain Heal
         421, // Chain Lightning
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
    // Pet tree, 8 abilities
    {
       36213, // Angered Earth
      157348, // Call Lightning
       57984, // Fire Blast
      117588, // Fire Nova
      118337, // Harden Skin
      118297, // Immolate
      118345, // Pulverize
      157331, // Wind Gust
           0,
    },
  },
  // Class active abilities for Mage
  {
    // General tree, 26 abilities
    {
      120145, // Ancient Teleport: Dalaran
        1953, // Blink
      190336, // Conjure Refreshment
        2139, // Counterspell
      214634, // Ebonbolt (Artifact)
         122, // Frost Nova
       45438, // Ice Block
      131784, // Illusion
      211076, // Mark of Aluneth
      211088, // Mark of Aluneth
      224968, // Mark of Aluneth (Artifact)
      210726, // Mark of Aluneth (Artifact)
      194466, // Phoenix's Flames (Artifact)
         118, // Polymorph (Sheep)
       61721, // Polymorph (Rabbit)
       61780, // Polymorph (Turkey)
      161353, // Polymorph (Polar Bear Cub)
      161354, // Polymorph (Monkey)
      161355, // Polymorph (Penguin)
      161372, // Polymorph (Turtle)
       61025, // Polymorph (Serpent)
       28271, // Polymorph (Turtle)
       28272, // Polymorph (Pig)
      126819, // Polymorph (Pig)
       61305, // Polymorph (Black Cat)
       80353, // Time Warp
           0,
    },
    // Arcane tree, 11 abilities
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
    // Pet tree, 8 abilities
    {
      205235, // Arcane Assault
       88084, // Arcane Blast
       88082, // Fireball
       33395, // Freeze
       59638, // Frostbolt
      155152, // Prismatic Crystal
      135029, // Water Jet
       31707, // Waterbolt
           0,
    },
  },
  // Class active abilities for Warlock
  {
    // General tree, 29 abilities
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
       18540, // Summon Doomguard (Guardian)
         691, // Summon Felhunter (Summon)
         688, // Summon Imp (Summon)
        1122, // Summon Infernal (Guardian)
         712, // Summon Succubus (Summon)
         697, // Summon Voidwalker (Summon)
      211714, // Thal'kiel's Consumption (Artifact)
      211715, // Thal'kiel's Consumption
      104773, // Unending Resolve
           0,
    },
    // Affliction tree, 7 abilities
    {
         980, // Agony
         172, // Corruption
      198590, // Drain Soul
        1454, // Life Tap
       27243, // Seed of Corruption
      232670, // Shadow Bolt
       30108, // Unstable Affliction
           0,
    },
    // Demonology tree, 11 abilities
    {
      104316, // Call Dreadstalkers
      193396, // Demonic Empowerment
      193440, // Demonwrath
         603, // Doom
      234153, // Drain Life
      111898, // Grimoire: Felguard (Summon)
      105174, // Hand of Gul'dan
        1454, // Life Tap
         686, // Shadow Bolt
      196606, // Shadowy Inspiration
       30146, // Summon Felguard (Summon)
           0,
    },
    // Destruction tree, 12 abilities
    {
      116858, // Chaos Bolt
      215276, // Chaos Tear
       17962, // Conflagrate
      234153, // Drain Life
       80240, // Havoc
         348, // Immolate
       29722, // Incinerate
        1454, // Life Tap
      226802, // Lord of Flames
        5740, // Rain of Fire
         686, // Shadow Bolt
      196639, // Shadowy Tear
           0,
    },
    {
           0,
    },
    // Pet tree, 28 abilities
    {
       89766, // Axe Toss (Special Ability)
      171011, // Burning Presence (Special Ability)
      119899, // Cauterize Master (Special Ability)
        3716, // Consuming Shadows (Basic Attack)
      170995, // Cripple
       19505, // Devour Magic (Special Ability)
       85692, // Doom Bolt
      205231, // Eye Laser
      104318, // Fel Firebolt (Basic Attack)
       89751, // Felstorm (Special Ability)
        3110, // Firebolt (Basic Attack)
       89792, // Flee (Special Ability)
        7814, // Lash of Pain (Basic Attack)
       30213, // Legion Strike (Basic Attack)
        7870, // Lesser Invisibility (Special Ability)
       30151, // Pursuit (Special Ability)
        6358, // Seduction (Special Ability)
      171014, // Seethe (Special Ability)
       54049, // Shadow Bite (Basic Attack)
      171138, // Shadow Lock (Special Ability)
      115232, // Shadow Shield (Special Ability)
       89808, // Singe Magic (Special Ability)
       19647, // Spell Lock (Special Ability)
       17735, // Suffering (Special Ability)
      134477, // Threatening Presence (Special Ability)
      112042, // Threatening Presence (Special Ability)
      171021, // Torch Magic (Special Ability)
        6360, // Whiplash (Special Ability)
           0,
    },
  },
  // Class active abilities for Monk
  {
    // General tree, 16 abilities
    {
      100784, // Blackout Kick
      117952, // Crackling Jade Lightning
      175697, // Disabling Technique
      214326, // Exploding Keg (Artifact)
      122281, // Healing Elixir
      115078, // Paralysis
      115546, // Provoke
      109132, // Roll
      205406, // Sheilun's Gift (Artifact)
      209525, // Soothing Mist (Honor Talent)
      205320, // Strike of the Windlord (Artifact)
      100780, // Tiger Palm
      101643, // Transcendence
      119996, // Transcendence: Transfer
      125883, // Zen Flight
      126892, // Zen Pilgrimage
           0,
    },
    // Brewmaster tree, 16 abilities
    {
      205523, // Blackout Strike
      115181, // Breath of Fire
      216521, // Celestial Fortune
      218164, // Detox
      116694, // Effuse
      195630, // Elusive Brawler
      115072, // Expel Harm
      115203, // Fortifying Brew
      124273, // Heavy Stagger
      115308, // Ironskin Brew
      121253, // Keg Smash
      124275, // Light Stagger
      124274, // Moderate Stagger
      119582, // Purifying Brew
      116705, // Spear Hand Strike
      115176, // Zen Meditation
           0,
    },
    // Mistweaver tree, 13 abilities
    {
      228649, // Blackout Kick
      115450, // Detox
      116694, // Effuse
      124682, // Enveloping Mist
      191837, // Essence Font
      116849, // Life Cocoon
      212051, // Reawaken
      115151, // Renewing Mist
      115310, // Revival
      107428, // Rising Sun Kick
      101546, // Spinning Crane Kick
      116680, // Thunder Focus Tea
      116670, // Vivify
           0,
    },
    // Windwalker tree, 15 abilities
    {
      116768, // Blackout Kick!
      211432, // Combo Master
      218164, // Detox
      116095, // Disable
      116694, // Effuse
      113656, // Fists of Fury
      101545, // Flying Serpent Kick
      213114, // Hidden Master's Forbidden Touch
      228287, // Mark of the Crane
      107428, // Rising Sun Kick
      116705, // Spear Hand Strike
      101546, // Spinning Crane Kick
      137639, // Storm, Earth, and Fire
      115080, // Touch of Death
      122470, // Touch of Karma
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
    // General tree, 25 abilities
    {
      210722, // Ashamane's Frenzy (Artifact)
      210723, // Ashamane's Frenzy
        5487, // Bear Form (Shapeshift)
         768, // Cat Form (Shapeshift)
      127757, // Charm Woodland Creature
        1850, // Dash
      193753, // Dreamwalk
         339, // Entangling Roots
      208253, // Essence of G'Hanir (Artifact)
      164862, // Flap
      165962, // Flight Form (Shapeshift)
      202771, // Full Moon (Artifact)
        6795, // Growl
      202768, // Half Moon (Artifact)
        8921, // Moonfire (Lunar)
      164812, // Moonfire (Lunar)
      175682, // Nature's Control
      202767, // New Moon (Artifact)
      200851, // Rage of the Sleeper (Artifact)
       20484, // Rebirth
        8936, // Regrowth
      210053, // Stag Form (Shapeshift)
      164815, // Sunfire (Solar)
         783, // Travel Form (Shapeshift)
      114282, // Treant Form (Shapeshift)
           0,
    },
    // Balance tree, 19 abilities
    {
       22812, // Barkskin
      194223, // Celestial Alignment
      122114, // Chosen of Elune
      202771, // Full Moon (Artifact)
      211547, // Fury of Elune
      203001, // Goldrinn's Fang
      202768, // Half Moon (Artifact)
       29166, // Innervate
      194153, // Lunar Strike (Lunar)
       24858, // Moonkin Form
      209406, // Oneth's Intuition
      209407, // Oneth's Overconfidence
        5215, // Prowl
        2782, // Remove Corruption
       78675, // Solar Beam
      190984, // Solar Wrath (Solar)
      191034, // Starfall
       78674, // Starsurge
       93402, // Sunfire (Solar)
           0,
    },
    // Feral tree, 20 abilities
    {
      210705, // Ashamane's Rip
      106951, // Berserk
      145152, // Bloodtalons
      210649, // Feral Instinct
      146874, // Feral Rage
       22568, // Ferocious Bite
       22570, // Maim
      155625, // Moonfire
       69369, // Predatory Swiftness
        5215, // Prowl
        1822, // Rake
        2782, // Remove Corruption
        1079, // Rip
        5221, // Shred
      106839, // Skull Bash
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
       22842, // Frenzied Regeneration
          99, // Incapacitating Roar
      192081, // Ironfur
       33917, // Mangle
      192083, // Mark of Ursol
        6807, // Maul
      227034, // Nature's Guardian
        5215, // Prowl
        2782, // Remove Corruption
      106839, // Skull Bash
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
        5185, // Healing Touch
       29166, // Innervate
      102342, // Ironbark
       33763, // Lifebloom
       88423, // Nature's Cure
        5215, // Prowl
         774, // Rejuvenation
      212040, // Revitalize
        5176, // Solar Wrath (Solar)
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
      183752, // Consume Magic
      162243, // Demon's Bite
      198013, // Eye Beam
      195072, // Fel Rush
      201789, // Fury of the Illidari
      201467, // Fury of the Illidari (Artifact)
      201628, // Fury of the Illidari
      131347, // Glide
      217832, // Imprison
      191427, // Metamorphosis
      207407, // Soul Carver (Artifact)
      188501, // Spectral Sight
      185123, // Throw Glaive
      198793, // Vengeful Retreat
           0,
    },
    // Havoc tree, 6 abilities
    {
      202446, // Anguish
      211796, // Chaos Blades
      196718, // Darkness
      203796, // Demon Blades
      208605, // Nemesis
      217070, // Rage of the Illidari
           0,
    },
    // Vengeance tree, 15 abilities
    {
      213011, // Charred Warblades
      203720, // Demon Spikes
      218256, // Empower Wards
      209245, // Fiery Brand
      204021, // Fiery Brand
      178740, // Immolation Aura
      189110, // Infernal Strike
      187827, // Metamorphosis
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
#define PTR_TREE_SPECIALIZATION_SIZE (36)

// Talent tree specialization abilities, wow build 23038
static unsigned __ptr_tree_specialization_data[][MAX_SPECS_PER_CLASS][PTR_TREE_SPECIALIZATION_SIZE] = {
  // Specialization abilities for Hunter pets
  {
    // Specialization abilities for Ferocity
    {
       61684, // Dash (Ferocity, Cunning Ability)
       55709, // Heart of the Phoenix (Ferocity Ability)
       20782, // Combat Experience
       53184, // Spiked Collar
       53482, // Blood of the Rhino
       19596, // Boar's Speed
       53497, // Cornered
       61688, // Great Stamina
           0,
    },
    // Specialization abilities for Tenacity
    {
       61685, // Charge (Tenacity Ability)
       53478, // Last Stand (Tenacity Ability)
       63900, // Thunderstomp (Tenacity Ability)
       53482, // Blood of the Rhino
       20782, // Combat Experience
       61688, // Great Stamina
       19596, // Boar's Speed
       53497, // Cornered
       53184, // Spiked Collar
           0,
    },
    // Specialization abilities for Cunning
    {
       53490, // Bullheaded (Cunning Ability)
       61684, // Dash (Ferocity, Cunning Ability)
       53480, // Roar of Sacrifice (Cunning Ability)
       19596, // Boar's Speed
       20782, // Combat Experience
       53497, // Cornered
       53482, // Blood of the Rhino
       61688, // Great Stamina
       53184, // Spiked Collar
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
       76838, // Mastery: Colossal Might
      137049, // Arms Warrior
       97462, // Commanding Shout
        1719, // Battle Cry
      163201, // Execute
      167105, // Colossus Smash
        1680, // Whirlwind
      162698, // Stat Negation Aura - Strength DPS
      184783, // Tactician
        1464, // Slam
       34428, // Victory Rush
         845, // Cleave
       18499, // Berserker Rage
        1715, // Hamstring
        5246, // Intimidating Shout
      118038, // Die by the Sword
      227847, // Bladestorm
      231833, // Cleave (Rank 2)
      231830, // Execute (Rank 2)
           0,
    },
    // Specialization abilities for Fury
    {
       23881, // Bloodthirst
       85288, // Raging Blow
       46917, // Titan's Grip
       81099, // Single-Minded Fury
       86110, // Plate Specialization
       12950, // Whirlwind (Rank 2)
       76856, // Mastery: Unshackled Fury
      137050, // Fury Warrior
       97462, // Commanding Shout
        1719, // Battle Cry
        5308, // Execute
       12323, // Piercing Howl
      162698, // Stat Negation Aura - Strength DPS
      184361, // Enrage
      184364, // Enraged Regeneration
      184367, // Rampage
       18499, // Berserker Rage
      190411, // Whirlwind
      100130, // Furious Slash
      209694, // Rampage!
        5246, // Intimidating Shout
      231842, // Dual Wield
      231824, // Furious Slash (Rank 2)
      231827, // Execute (Rank 2)
           0,
    },
    // Specialization abilities for Protection
    {
       23922, // Shield Slam
       29144, // Unwavering Sentinel
        6572, // Revenge
       12975, // Last Stand
       46953, // Sword and Board
       86535, // Plate Specialization
       84608, // Bastion of Defense
       76857, // Mastery: Critical Block
        1160, // Demoralizing Shout
      131086, // Protection
      137048, // Protection Warrior
        6343, // Thunder Clap
         871, // Shield Wall
      161798, // Riposte
      162702, // Stat Negation Aura - Strength Tank
      115768, // Deep Wounds
       20243, // Devastate
          71, // Defensive Stance
       23920, // Spell Reflection
        2565, // Shield Block
      190456, // Ignore Pain
      198304, // Intercept
       34428, // Victory Rush
        1719, // Battle Cry
       18499, // Berserker Rage
        5301, // Revenge Trigger
      231834, // Shield Slam (Rank 2)
      231847, // Shield Block (Rank 2)
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
       31842, // Avenging Wrath
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
      227068, // Righteousness
      231644, // Judgment (Rank 2)
      231642, // Beacon of Light (Rank 2)
      231667, // Crusader Strike (Rank 2)
           0,
    },
    // Specialization abilities for Protection
    {
       31935, // Avenger's Shield
       53592, // Guarded by the Light
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
      157488, // Improved Block
         853, // Hammer of Justice
      213644, // Cleanse Toxins
       19750, // Flash of Light
       31884, // Avenging Wrath
      231657, // Judgment (Rank 2)
      231665, // Avenger's Shield (Rank 2)
      229976, // Blessed Hammer
           0,
    },
    // Specialization abilities for Retribution
    {
       85256, // Templar's Verdict
       53385, // Divine Storm
       86539, // Plate Specialization (Retribution, Passive)
       31884, // Avenging Wrath
       76672, // Mastery: Divine Judgment
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
      203528, // Greater Blessing of Might
      203538, // Greater Blessing of Kings
      203539, // Greater Blessing of Wisdom
      213644, // Cleanse Toxins
      231667, // Crusader Strike (Rank 2)
      231663, // Judgment (Rank 3)
      231661, // Judgment (Rank 2)
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
      120679, // Dire Beast
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
           0,
    },
    // Specialization abilities for Marksmanship
    {
       19434, // Aimed Shot
       35110, // Bombardment
      137016, // Marksmanship Hunter
        5116, // Concussive Shot
      147362, // Counter Shot
        2643, // Multi-Shot
       34477, // Misdirection
      185358, // Arcane Shot
      185987, // Hunter's Mark
      185901, // Marked Shot
      186387, // Bursting Shot
      162697, // Stat Negation Aura - Agility DPS
      193468, // Mastery: Sniper Training (Mastery)
      193526, // Trueshot
         781, // Disengage
      212658, // Hunting Party
      193455, // Cobra Shot
      231549, // Disengage (Rank 2)
      231554, // Marksman's Focus
      109304, // Exhilaration
      231546, // Exhilaration (Rank 2)
      187650, // Freezing Trap
      187698, // Tar Trap
           0,
    },
    // Specialization abilities for Survival
    {
      137017, // Survival Hunter
      164856, // Survivalist
      185855, // Lacerate
      186270, // Raptor Strike
      187650, // Freezing Trap
      187698, // Tar Trap
      187707, // Muzzle
      187708, // Carve
      190925, // Harpoon
      190928, // Mongoose Bite
      191334, // Mastery: Hunting Companion
      191433, // Explosive Trap
      162697, // Stat Negation Aura - Agility DPS
      193265, // Hatchet Toss
      186289, // Aspect of the Eagle
      195645, // Wing Clip
      109304, // Exhilaration
      202800, // Flanking Strike
      231555, // Aspect of the Eagle (Rank 2)
      231550, // Harpoon (Rank 2)
      231546, // Exhilaration (Rank 2)
      234955, // Waylay
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
       84601, // Assassin's Resolve
       14117, // Improved Poisons
        1329, // Mutilate
       32645, // Envenom
       14190, // Seal Fate
       79134, // Venomous Wounds
       51667, // Cut to the Chase
       79140, // Vendetta
       76803, // Mastery: Potent Poisons
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
           0,
    },
    // Specialization abilities for Outlaw
    {
       61329, // Vitality
       13877, // Blade Flurry
       35551, // Combat Potency
       13750, // Adrenaline Rush
       76806, // Mastery: Main Gauche
      137036, // Outlaw Rogue
       14161, // Ruthlessness
        8676, // Ambush
      185763, // Pistol Shot
      162697, // Stat Negation Aura - Agility DPS
      193315, // Saber Slash
        1776, // Gouge
        2098, // Run Through
        1784, // Stealth
         921, // Pick Pocket
        1725, // Distract
        1856, // Vanish
      199736, // Find Treasure (Racial)
      199740, // Bribe
      199754, // Riposte
      199804, // Between the Eyes
        2094, // Blind
        1833, // Cheap Shot
      193316, // Roll the Bones
        6770, // Sap
      231691, // Sprint (Rank 2)
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
       77484, // Mastery: Absolution
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
      200829, // Plea
       45243, // Focused Will
      212036, // Mass Resurrection
      231682, // Smite (Rank 2)
        2061, // Flash Heal
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
       63733, // Serendipity
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
       32379, // Shadow Word: Death
      185916, // Voidform
      186263, // Shadow Mend
      186440, // Shadow Mend
        2096, // Mind Vision
         589, // Shadow Word: Pain
       34433, // Shadowfiend
      190719, // Shadow Word: Death
        8122, // Psychic Scream
          17, // Power Word: Shield
        1706, // Levitate
      213634, // Purify Disease
      228260, // Void Eruption
      228266, // Void Bolt
      228264, // Voidform
      231688, // Void Bolt (Rank 2)
      231689, // Shadow Word: Death (Rank 2)
        2061, // Flash Heal
      232698, // Shadowform
      234702, // Mind Flay (Rank 2)
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
      212552, // Wraith Walk
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
       51271, // Pillar of Frost
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
      212552, // Wraith Walk
           0,
    },
    // Specialization abilities for Unholy
    {
       86536, // Plate Specialization (Passive)
       55090, // Scourge Strike
       85948, // Festering Strike
       49530, // Sudden Doom
       63560, // Dark Transformation
       49206, // Summon Gargoyle
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
      212552, // Wraith Walk
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
      123099, // Spiritual Insight
      137040, // Elemental Shaman
      162699, // Stat Negation Aura - Intellect DPS
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
      198103, // Earth Elemental
       16164, // Elemental Focus
       51886, // Cleanse Spirit
        8004, // Healing Surge
      231721, // Lava Burst (Rank 2)
      231722, // Chain Lightning (Rank 2)
      232643, // Flame Shock (Rank 2)
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
      195255, // Stormlash
      187874, // Crash Lightning
      190899, // Healing Surge (Rank 2)
      196834, // Frostbrand
      201845, // Stormbringer
       33757, // Windfury
       51886, // Cleanse Spirit
      231723, // Feral Spirit (Rank 2)
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
       79684, // Arcane Missiles
       44425, // Arcane Barrage
       31589, // Slow
       12042, // Arcane Power
      137021, // Arcane Mage
        1449, // Arcane Explosion
       12051, // Evocation
      190427, // Arcane Charge
      190740, // Mastery: Savant
      162699, // Stat Negation Aura - Intellect DPS
      195676, // Displacement
      110959, // Greater Invisibility
      231564, // Arcane Barrage (Rank 2)
      231565, // Evocation (Rank 2)
          66, // Invisibility
      235450, // Prismatic Barrier
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
      231598, // Water Jet (Pet)
      108853, // Fire Blast
      235219, // Cold Snap
       11426, // Ice Barrier
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
        1454, // Life Tap
      162699, // Stat Negation Aura - Intellect DPS
         172, // Corruption
      231791, // Unstable Affliction (Rank 2)
      231799, // Shadow Bite (Rank 2)
      231792, // Agony (Rank 2)
      231811, // Soulstone (Rank 2)
      232670, // Shadow Bolt
      198590, // Drain Soul
           0,
    },
    // Specialization abilities for Demonology
    {
      105174, // Hand of Gul'dan
       77219, // Mastery: Master Demonologist
       30146, // Summon Felguard (Summon)
      137044, // Demonology Warlock
        1454, // Life Tap
      162699, // Stat Negation Aura - Intellect DPS
         603, // Doom
      104316, // Call Dreadstalkers
      193396, // Demonic Empowerment
      193440, // Demonwrath
      108415, // Soul Link
         686, // Shadow Bolt
      231811, // Soulstone (Rank 2)
      234153, // Drain Life
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
        1454, // Life Tap
      193541, // Immolate
         686, // Shadow Bolt
      231795, // Firebolt (Rank 2)
      231793, // Conflagrate (Rank 2)
      231794, // Unending Resolve (Rank 2)
      231811, // Soulstone (Rank 2)
      234153, // Drain Life
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
      116694, // Effuse
      216519, // Celestial Fortune
      218164, // Detox
      115072, // Expel Harm
      231602, // Effuse (Rank 2)
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
      193884, // Soothing Mist
      107428, // Rising Sun Kick
      101546, // Spinning Crane Kick
      116694, // Effuse
      116670, // Vivify
      212051, // Reawaken
      231605, // Enveloping Mist (Rank 2)
      231606, // Renewing Mist (Rank 2)
      231633, // Essence Font (Rank 2)
      231876, // Thunder Focus Tea (Rank 2)
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
      103985, // Stance of the Fierce Tiger (Stance)
      115080, // Touch of Death
      101546, // Spinning Crane Kick
      116705, // Spear Hand Strike
      157411, // Windwalking
      116694, // Effuse
      218164, // Detox
      220357, // Cyclone Strikes
      231602, // Effuse (Rank 2)
      231627, // Storm, Earth, and Fire (Rank 2)
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
      106732, // Balance Overrides Passive (Passive)
       24858, // Moonkin Form
       78675, // Solar Beam
      108299, // Killer Instinct
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
      231049, // Starfall (Rank 2)
      231050, // Sunfire (Rank 2)
      159456, // Travel Form (Rank 2)
           0,
    },
    // Specialization abilities for Feral
    {
      106733, // Feral Overrides Passive (Passive)
        5217, // Tiger's Fury
       16949, // Feral Instinct
        1079, // Rip
       16974, // Predatory Swiftness
       33873, // Nurturing Instinct
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
        5221, // Shred
      106898, // Stampeding Roar
      106832, // Thrash
      159286, // Primal Fury
      131768, // Feline Swiftness
      197692, // Feral Overrides Passive (Passive)
      106951, // Berserk
        2782, // Remove Corruption
      213764, // Swipe
      228545, // Cat Form (Shapeshift)
      231052, // Rake (Rank 2)
      231055, // Tiger's Fury (Rank 2)
      231056, // Ferocious Bite (Rank 2)
      231057, // Shred (Rank 3)
      231063, // Shred (Rank 2)
      159456, // Travel Form (Rank 2)
      231283, // Swipe (Rank 2)
    },
    // Specialization abilities for Guardian
    {
      106734, // Guardian Overrides Passive (Passive)
       33873, // Nurturing Instinct
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
       33917, // Mangle
      106898, // Stampeding Roar
      192081, // Ironfur
      192083, // Mark of Ursol
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
           0,
    },
    // Specialization abilities for Restoration
    {
       18562, // Swiftmend
       88423, // Nature's Cure
       48500, // Living Seed
      108299, // Killer Instinct
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
      231032, // Regrowth (Rank 2)
      231040, // Rejuvenation (Rank 2)
      231050, // Sunfire (Rank 2)
        5185, // Healing Touch
      159456, // Travel Form (Rank 2)
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
      185244, // Pain
      207197, // Riposte
      204254, // Shattered Souls
      203783, // Shear
      212613, // Vengeance Demon Hunter
      218256, // Empower Wards
      203747, // Mastery: Fel Blood
      226359, // Leather Specialization (Passive)
      228477, // Soul Cleave
      207684, // Sigil of Misery
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

// Class mastery abilities, wow build 23038
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
      76838, // Mastery: Colossal Might
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
      76672, // Mastery: Divine Judgment
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
     191334, // Mastery: Hunting Companion
    },
    {
          0,
    },
  },
  // Class mastery abilities for Rogue
  {
    // Masteries for Assassination specialization
    {
      76803, // Mastery: Potent Poisons
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
      77484, // Mastery: Absolution
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
#define PTR_RACE_ABILITY_SIZE (4)

#ifndef ptr_MAX_RACE
#define ptr_MAX_RACE (28)
#endif

// Racial abilities, wow build 23038
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
    },
    // Racial abilities for Paladin class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
    },
    // Racial abilities for Hunter class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
    },
    // Racial abilities for Rogue class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
    },
    // Racial abilities for Priest class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
    },
    // Racial abilities for Death Knight class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
    },
    // Racial abilities for Shaman class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
    },
    // Racial abilities for Mage class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
    },
    // Racial abilities for Warlock class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
    },
    {     0, },
    // Racial abilities for Druid class
    {
      68976, // Aberration
      94293, // Altered Form
      68992, // Darkflight
      68975, // Viciousness
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
  // Racial abilities for Pandaren
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
};
#define PTR_SET_BONUS_DATA_SIZE (544)

// Set bonus data, wow build 23038
static item_set_bonus_t __ptr_set_bonus_data[PTR_SET_BONUS_DATA_SIZE] = {
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
  { "Grandmaster's Finery"                       ,   "tier19oh",      3,  1274,   19,   8,  10,   -1, 221651, { 139731, 139732, 139733, 139734, 139735, 139736, 139737, 139738,  0 } },
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
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   2,   1,   71, 212149, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   2,   1,   72, 212157, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   2,   1,   73, 212237, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls, Spec,  Spell, Items
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   4,   1,   71, 212156, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   4,   1,   72, 212158, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Warplate of the Obsidian Aspect"            ,     "tier19",     12,  1292,   19,   4,   1,   73, 212244, { 138351, 138354, 138357, 138360, 138363, 138374,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   2,   2,   65, 211436, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   2,   2,   70, 211444, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   2,   2,   66, 211553, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   4,   2,   65, 211438, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   4,   2,   70, 211448, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Battleplate of the Highlord"                ,     "tier19",     12,  1287,   19,   4,   2,   66, 211554, { 138350, 138353, 138356, 138359, 138362, 138369,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   2,   3,  253, 211172, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   2,   3,  254, 211331, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   2,   3,  255, 211340, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   4,   3,  253, 211181, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   4,   3,  254, 211322, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
  { "Eagletalon Battlegear"                      ,     "tier19",     12,  1284,   19,   4,   3,  255, 211357, { 138339, 138340, 138342, 138344, 138347, 138368,  0 } },
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
  { "Regalia of Bound Elements"                  ,     "tier19",     12,  1290,   19,   2,   7,  262, 211951, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Bound Elements"                  ,     "tier19",     12,  1290,   19,   2,   7,  263, 211987, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Bound Elements"                  ,     "tier19",     12,  1290,   19,   2,   7,  264, 211992, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Bound Elements"                  ,     "tier19",     12,  1290,   19,   4,   7,  262, 211963, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Bound Elements"                  ,     "tier19",     12,  1290,   19,   4,   7,  263, 211988, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Bound Elements"                  ,     "tier19",     12,  1290,   19,   4,   7,  264, 211993, { 138341, 138343, 138345, 138346, 138348, 138372,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   2,   8,   62, 211379, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   2,   8,   63, 211394, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   2,   8,   64, 211411, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   4,   8,   62, 211386, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   4,   8,   63, 211395, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Regalia of Everburning Knowledge"           ,     "tier19",     12,  1285,   19,   4,   8,   64, 211404, { 138309, 138312, 138315, 138318, 138321, 138365,  0 } },
  { "Legacy of Azj'Aqir"                         ,     "tier19",     12,  1291,   19,   2,   9,  265, 212002, { 138311, 138314, 138317, 138320, 138323, 138373,  0 } },
  { "Legacy of Azj'Aqir"                         ,     "tier19",     12,  1291,   19,   2,   9,  266, 212005, { 138311, 138314, 138317, 138320, 138323, 138373,  0 } },
  { "Legacy of Azj'Aqir"                         ,     "tier19",     12,  1291,   19,   2,   9,  267, 212018, { 138311, 138314, 138317, 138320, 138323, 138373,  0 } },
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
};
#define PTR_RPPMMOD_SIZE (251)

// 251 RPPM Modifiers, wow build level 23038
static struct rppm_modifier_t __ptr_rppmmodifier_data[] = {
  {  45054,    4,  1,  1.0000 },
  {  49530,    1,  1,  1.0000 },
  {  51128,    4,  1,  1.0000 },
  {  51128,    4,  2,  1.0000 },
  {  60066,    4,  2,  1.0000 },
  {  60306,    4,  1,  1.0000 },
  { 104428,    4,  1,  1.0000 },
  { 104441,    4,  1,  1.0000 },
  { 118314,    4,  1,  1.0000 },
  { 119938,    4,  1,  1.0000 },
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
  { 138120,   72,  4, -0.6250 },
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
  { 185987,    4,  1,  1.0000 },
  { 187143,    4,  1,  1.0000 },
  { 187435,    4,  1,  1.0000 },
  { 188027,    4,  1,  1.0000 },
  { 188028,    4,  1,  1.0000 },
  { 188534,    4,  1,  1.0000 },
  { 188783,    4,  1,  1.0000 },
  { 190889,    4,  1,  1.0000 },
  { 190890,    4,  1,  1.0000 },
  { 191048,    4,  1,  1.0000 },
  { 192623,    4,  1,  1.0000 },
  { 192657,    4,  1,  1.0000 },
  { 193371,    4,  1,  1.0000 },
  { 195255,    4,  1,  1.0000 },
  { 195679,    4,  2,  1.0000 },
  { 196099,    5,  1,  1.0000 },
  { 196777,    4,  1,  1.0000 },
  { 197846,    4,  1,  1.0000 },
  { 198068,    4,  1,  1.0000 },
  { 198735,    4,  1,  1.0000 },
  { 199806,    4,  1,  1.0000 },
  { 201336,    4,  1,  1.0000 },
  { 201407,    4,  1,  1.0000 },
  { 201466,    4,  1,  1.0000 },
  { 201471,    4,  1,  1.0000 },
  { 201900,    4,  1,  1.0000 },
  { 202387,    4,  1,  1.0000 },
  { 202762,    4,  1,  1.0000 },
  { 202840,    4,  1,  1.0000 },
  { 203557,    4,  1,  1.0000 },
  { 207320,    4,  1,  1.0000 },
  { 207767,    4,  1,  1.0000 },
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
  { 215857,    0,  2,  1.0000 },
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
  { 222167,    0,  1,  1.0000 },
  { 222187,    4,  1,  1.0000 },
  { 222187,  263,  4, -0.2400 },
  { 222207,    4,  1,  1.0000 },
  { 222703,    4,  1,  1.0000 },
  { 222705,  252,  4, -0.3200 },
  { 224059,    4,  1,  1.0000 },
  { 224073,    4,  1,  1.0000 },
  { 224076,    4,  1,  1.0000 },
  { 224158,    4,  1,  1.0000 },
  { 225123,    4,  1,  1.0000 },
  { 225127,    4,  1,  1.0000 },
  { 225128,    0,  1,  1.0000 },
  { 225131,    0,  1,  1.0000 },
  { 225134,    4,  1,  1.0000 },
  { 225135,  103,  4, -0.3000 },
  { 225135,  259,  4, -0.3000 },
  { 225135,  261,  4, -0.3000 },
  { 225135,  255,  4,  0.6000 },
  { 225135,  254,  4,  0.5000 },
  { 225135,  253,  4,  0.5000 },
  { 225137,    4,  1,  1.0000 },
  { 225138,    4,  1,  1.0000 },
  { 225138,  105,  4,  0.1200 },
  { 225138,  270,  4,  0.0300 },
  { 225138,  257,  4,  0.1800 },
  { 225138,  256,  4, -0.2400 },
  { 225138,   65,  4, -0.1300 },
  { 225138,  264,  4,  0.1200 },
  { 225139,  251,  4,  1.0000 },
  { 225139,  252,  4,  0.6600 },
  { 225139,  577,  4,  0.4500 },
  { 225139,  103,  4,  1.2400 },
  { 225139,  253,  4,  1.8000 },
  { 225139,  254,  4,  1.2400 },
  { 225139,  255,  4,  3.0000 },
  { 225139,  269,  4,  1.8000 },
  { 225139,   70,  4,  0.4000 },
  { 225139,  259,  4,  0.1700 },
  { 225139,  260,  4,  1.8000 },
  { 225139,  261,  4,  2.0000 },
  { 225139,  263,  4,  2.0000 },
  { 225139,   71,  4,  0.1000 },
  { 225139,   72,  4,  0.7200 },
  { 225601,    4,  1,  1.0000 },
  { 225606,    4,  1,  1.0000 },
  { 227171,    4,  1,  1.0000 },
  { 227868,    4,  1,  1.0000 },
  { 228141,    4,  1,  1.0000 },
  { 228400,    3,  1,  1.0000 },
  { 228445,    4,  1,  1.0000 },
  { 230080,    4,  1,  1.0000 },
  { 230140,    4,  1,  1.0000 },
  { 230236,    4,  1,  1.0000 },
  { 230236,  263,  4, -0.5000 },
  { 230241,    4,  1,  1.0000 },
  { 230257,    4,  1,  1.0000 },
  { 230257,  263,  4, -0.5000 },
  { 231832,    1,  1,  1.0000 },
  { 235003,    4,  1,  1.0000 },
  { 235011,    4,  1,  1.0000 },
  { 236167,    4,  1,  1.0000 },
  {      0,    0,  0,  0.0000 },
};
#define PTR_ARTIFACT_SIZE (41)

// Artifact base data, wow build 23038
static struct artifact_t __ptr_artifact_data[PTR_ARTIFACT_SIZE] = {
  {  2,  70 },
  {  3, 577 },
  {  4,  62 },
  { 11,  73 },
  { 12, 251 },
  { 13, 105 },
  { 15, 250 },
  { 16, 252 },
  { 17, 261 },
  { 34, 255 },
  { 35,  72 },
  { 36,  71 },
  { 37, 266 },
  { 38, 267 },
  { 39, 265 },
  { 40, 262 },
  { 41, 263 },
  { 42, 264 },
  { 43, 259 },
  { 44, 260 },
  { 45, 257 },
  { 46, 256 },
  { 47, 258 },
  { 48,  65 },
  { 49,  66 },
  { 50, 269 },
  { 51, 270 },
  { 52, 268 },
  { 53,  64 },
  { 54,  63 },
  { 55, 254 },
  { 56, 253 },
  { 57, 104 },
  { 58, 103 },
  { 59, 102 },
  { 60, 581 },
  { 74,  70 },
  { 80,  70 },
  { 81,  65 },
  { 82,  66 },
  {  0,   0 },
};
#define PTR_ARTIFACT_POWER_SIZE (715)

// Artifact power data, wow build 23038
static struct artifact_power_data_t __ptr_artifact_power_data[PTR_ARTIFACT_POWER_SIZE] = {
  {   40,   2,  18,  0,   0, 205273, "Wake of Ashes"                      }, // Wake of Ashes (id=205273, n_ranks=1)
  {   41,   2,   0,  4,   3, 186941, "Highlord's Judgment"                }, // Highlord's Judgment (id=186941, n_ranks=6)
  {   42,   2,   0,  5,   3, 184759, "Sharpened Edge"                     }, // Sharpened Edge (id=184759, n_ranks=6)
  {   43,   2,   0,  6,   3, 184843, "Righteous Blade"                    }, // Righteous Blade (id=184843, n_ranks=6)
  {   44,   2,   0, 11,   3, 186934, "Embrace the Light"                  }, // Embrace the Light (id=186934, n_ranks=6)
  {   47,   2,   0, 12,   3, 184778, "Deflection"                         }, // Deflection (id=184778, n_ranks=6)
  {   49,   2,   1,  1,   1, 186773, "Divine Tempest"                     }, // Divine Tempest (id=186773, n_ranks=1)
  {   50,   2,   0,  8,   3, 186927, "Deliver the Justice"                }, // Deliver the Justice (id=186927, n_ranks=6)
  {   51,   2,   0,  9,   3, 185368, "Might of the Templar"               }, // Might of the Templar (id=185368, n_ranks=6)
  {   52,   2,   0, 10,   3, 186944, "Protector of the Ashen Blade"       }, // Protector of the Ashen Blade (id=186944, n_ranks=6)
  {   53,   2,   0,  7,   3, 186945, "Wrath of the Ashbringer"            }, // Wrath of the Ashbringer (id=186945, n_ranks=6)
  {   54,   2,   1,  2,   1, 186788, "Echo of the Highlord"               }, // Echo of the Highlord (id=186788, n_ranks=1)
  {  350,   2,   0,  0,   1, 185086, "Endless Resolve"                    }, // Endless Resolve (id=185086, n_ranks=1)
  {  351,   2,   0,  0,   1, 182234, "Unbreakable Will"                   }, // Unbreakable Will (id=182234, n_ranks=1)
  {  352,   2,   0,  0,   1, 193058, "Healing Storm"                      }, // Healing Storm (id=193058, n_ranks=1)
  {  353,   2,   1,  3,   1, 179546, "Ashes to Ashes"                     }, // Ashes to Ashes (id=179546, n_ranks=1)
  { 1118,   2,   5,  0,  20, 207604, "Ashbringer's Light"                 }, // Ashbringer's Light (id=207604, n_ranks=20)
  { 1207,   2,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1275,   2,   0,  0,   1, 214081, "Blade of Light"                     }, // Blade of Light (id=214081, n_ranks=1)
  { 1000,   3,   0,  4,   3, 201454, "Contained Fury"                     }, // Contained Fury (id=201454, n_ranks=6)
  { 1001,   3,   0,  5,   3, 201455, "Critical Chaos"                     }, // Critical Chaos (id=201455, n_ranks=6)
  { 1002,   3,   0,  6,   3, 201456, "Chaos Vision"                       }, // Chaos Vision (id=201456, n_ranks=6)
  { 1003,   3,   0,  7,   3, 201457, "Sharpened Glaives"                  }, // Sharpened Glaives (id=201457, n_ranks=6)
  { 1004,   3,   0,  8,   3, 201458, "Demon Rage"                         }, // Demon Rage (id=201458, n_ranks=6)
  { 1005,   3,   0, 11,   3, 201459, "Illidari Knowledge"                 }, // Illidari Knowledge (id=201459, n_ranks=6)
  { 1006,   3,   0,  9,   3, 201460, "Unleashed Demons"                   }, // Unleashed Demons (id=201460, n_ranks=6)
  { 1007,   3,   0, 12,   3, 201463, "Deceiver's Fury"                    }, // Deceiver's Fury (id=201463, n_ranks=6)
  { 1008,   3,   0, 10,   3, 201464, "Overwhelming Power"                 }, // Overwhelming Power (id=201464, n_ranks=6)
  { 1010,   3,  18,  0,   0, 201467, "Fury of the Illidari"               }, // Fury of the Illidari (id=201467, n_ranks=1)
  { 1011,   3,   0,  0,   1, 201468, "Feast on the Souls"                 }, // Feast on the Souls (id=201468, n_ranks=1)
  { 1012,   3,   0,  0,   1, 201469, "Demon Speed"                        }, // Demon Speed (id=201469, n_ranks=1)
  { 1013,   3,   0,  0,   1, 201470, "Balanced Blades"                    }, // Balanced Blades (id=201470, n_ranks=1)
  { 1014,   3,   1,  1,   1, 201471, "Inner Demons"                       }, // Inner Demons (id=201471, n_ranks=1)
  { 1015,   3,   1,  2,   1, 201472, "Rage of the Illidari"               }, // Rage of the Illidari (id=201472, n_ranks=1)
  { 1016,   3,   1,  3,   1, 201473, "Anguish of the Deceiver"            }, // Anguish of the Deceiver (id=201473, n_ranks=1)
  { 1190,   3,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1330,   3,   0,  0,   1, 214795, "Warglaives of Chaos"                }, // Warglaives of Chaos (id=214795, n_ranks=1)
  { 1362,   3,   5,  0,  20, 214907, "Chaos Burn"                         }, // Chaos Burn (id=214907, n_ranks=20)
  {   72,   4,   0,  4,   3, 187304, "Torrential Barrage"                 }, // Torrential Barrage (id=187304, n_ranks=6)
  {   73,   4,   1,  1,   1, 188006, "Arcane Rebound"                     }, // Arcane Rebound (id=188006, n_ranks=1)
  {   74,   4,   0,  5,   3, 187258, "Blasting Rod"                       }, // Blasting Rod (id=187258, n_ranks=6)
  {   75,   4,   0,  6,   3, 187321, "Aegwynn's Wrath"                    }, // Aegwynn's Wrath (id=187321, n_ranks=6)
  {   77,   4,   0,  8,   3, 187313, "Arcane Purification"                }, // Arcane Purification (id=187313, n_ranks=6)
  {   78,   4,   0,  0,   1, 187310, "Crackling Energy"                   }, // Crackling Energy (id=187310, n_ranks=1)
  {   79,   4,   0, 10,   3, 187287, "Aegwynn's Fury"                     }, // Aegwynn's Fury (id=187287, n_ranks=6)
  {   80,   4,   1,  3,   1, 215463, "Rule of Threes"                     }, // Rule of Threes (id=215463, n_ranks=1)
  {   81,   4,   0,  9,   3, 187276, "Ethereal Sensitivity"               }, // Ethereal Sensitivity (id=187276, n_ranks=6)
  {   82,   4,   0,  7,   3, 187264, "Aegwynn's Imperative"               }, // Aegwynn's Imperative (id=187264, n_ranks=6)
  {   83,   4,   0, 11,   3, 187301, "Everywhere At Once"                 }, // Everywhere At Once (id=187301, n_ranks=6)
  {   84,   4,   0, 12,   3, 210716, "Force Barrier"                      }, // Force Barrier (id=210716, n_ranks=6)
  {   86,   4,   1,  2,   1, 210725, "Touch of the Magi"                  }, // Touch of the Magi (id=210725, n_ranks=1)
  {   87,   4,   0,  0,   1, 187680, "Aegwynn's Ascendance"               }, // Aegwynn's Ascendance (id=187680, n_ranks=1)
  {  290,   4,  18,  0,   0, 224968, "Mark of Aluneth"                    }, // Mark of Aluneth (id=224968, n_ranks=1)
  { 1169,   4,   0,  0,   1, 210730, "Sloooow Down"                       }, // Sloooow Down (id=210730, n_ranks=1)
  { 1199,   4,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1339,   4,   0,  0,   1, 187318, "Might of the Guardians"             }, // Might of the Guardians (id=187318, n_ranks=1)
  { 1371,   4,   5,  0,  20, 214917, "Ancient Power"                      }, // Ancient Power (id=214917, n_ranks=20)
  {   91,  11,  18,  0,   0, 203524, "Neltharion's Fury"                  }, // Neltharion's Fury (id=203524, n_ranks=1)
  {   92,  11,   1,  1,   1, 189059, "Scales of Earth"                    }, // Scales of Earth (id=189059, n_ranks=1)
  {   93,  11,   1,  2,   1, 203576, "Dragon Scales"                      }, // Dragon Scales (id=203576, n_ranks=1)
  {   94,  11,   1,  3,   1, 188778, "Might of the Vrykul"                }, // Might of the Vrykul (id=188778, n_ranks=1)
  {   95,  11,   0,  4,   3, 188635, "Vrykul Shield Training"             }, // Vrykul Shield Training (id=188635, n_ranks=6)
  {   96,  11,   0,  5,   3, 216272, "Rage of the Fallen"                 }, // Rage of the Fallen (id=216272, n_ranks=6)
  {   97,  11,   0,  0,   1, 188651, "Rumbling Voice"                     }, // Rumbling Voice (id=188651, n_ranks=1)
  {   98,  11,   0,  6,   3, 188683, "Will to Survive"                    }, // Will to Survive (id=188683, n_ranks=6)
  {   99,  11,   0,  7,   3, 203230, "Leaping Giants"                     }, // Leaping Giants (id=203230, n_ranks=6)
  {  100,  11,   0,  8,   3, 203225, "Dragon Skin"                        }, // Dragon Skin (id=203225, n_ranks=6)
  {  101,  11,   0,  9,   3, 203227, "Intolerance"                        }, // Intolerance (id=203227, n_ranks=6)
  {  102,  11,   0, 10,   3, 188639, "Shatter the Bones"                  }, // Shatter the Bones (id=188639, n_ranks=6)
  {  103,  11,   0,  0,   1, 203261, "Wall of Steel"                      }, // Wall of Steel (id=203261, n_ranks=1)
  {  104,  11,   0,  0,   1, 188672, "Reflective Plating"                 }, // Reflective Plating (id=188672, n_ranks=1)
  {  105,  11,   0, 11,   3, 188632, "Toughness"                          }, // Toughness (id=188632, n_ranks=6)
  {  106,  11,   0, 12,   3, 188644, "Thunder Crash"                      }, // Thunder Crash (id=188644, n_ranks=6)
  { 1222,  11,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1358,  11,   0,  0,   1, 188647, "Strength of the Earth Aspect"       }, // Strength of the Earth Aspect (id=188647, n_ranks=1)
  { 1395,  11,   5,  0,  20, 214939, "Unbreakable Bulwark"                }, // Unbreakable Bulwark (id=214939, n_ranks=20)
  { 1456,  11,   8,  0,   0, 226829, "Artificial Damage"                  }, // Artificial Damage (id=226829, n_ranks=1)
  {  108,  12,   0,  4,   3, 189080, "Cold as Ice"                        }, // Cold as Ice (id=189080, n_ranks=6)
  {  109,  12,   0,  6,   3, 189086, "Blast Radius"                       }, // Blast Radius (id=189086, n_ranks=6)
  {  110,  12,   0,  5,   3, 189092, "Ambidexterity"                      }, // Ambidexterity (id=189092, n_ranks=6)
  {  111,  12,   0, 10,   3, 189097, "Over-Powered"                       }, // Over-Powered (id=189097, n_ranks=6)
  {  113,  12,   0,  8,   3, 189144, "Nothing but the Boots"              }, // Nothing but the Boots (id=189144, n_ranks=6)
  {  114,  12,   0,  9,   3, 189147, "Bad to the Bone"                    }, // Bad to the Bone (id=189147, n_ranks=6)
  {  115,  12,   0, 11,   3, 189154, "Ice in Your Veins"                  }, // Ice in Your Veins (id=189154, n_ranks=6)
  {  117,  12,   0,  7,   3, 189164, "Dead of Winter"                     }, // Dead of Winter (id=189164, n_ranks=6)
  {  119,  12,   0,  0,   1, 189179, "Frozen Core"                        }, // Frozen Core (id=189179, n_ranks=1)
  {  120,  12,   0,  0,   1, 189180, "Mirror Ball"                        }, // Mirror Ball (id=189180, n_ranks=1)
  {  122,  12,  18,  1,   0, 189186, "Crystalline Swords"                 }, // Crystalline Swords (id=189186, n_ranks=1)
  {  123,  12,   1,  2,   1, 189185, "Hypothermia"                        }, // Hypothermia (id=189185, n_ranks=1)
  {  124,  12,   1,  3,   1, 190778, "Sindragosa's Fury"                  }, // Sindragosa's Fury (id=190778, n_ranks=1)
  { 1090,  12,   0, 12,   3, 204875, "Frozen Skin"                        }, // Frozen Skin (id=204875, n_ranks=6)
  { 1091,  12,   0,  0,   1, 205209, "Chill of the Grave"                 }, // Chill of the Grave (id=205209, n_ranks=1)
  { 1092,  12,   1,  3,   1, 189184, "Frozen Soul"                        }, // Frozen Soul (id=189184, n_ranks=1)
  { 1188,  12,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1332,  12,   0,  0,   1, 218931, "Blades of Frost"                    }, // Blades of Frost (id=218931, n_ranks=1)
  { 1360,  12,   5,  0,  20, 214904, "Soulbiter"                          }, // Soulbiter (id=214904, n_ranks=20)
  {  125,  13,  18,  0,   0, 208253, "Essence of G'Hanir"                 }, // Essence of G'Hanir (id=208253, n_ranks=1)
  {  126,  13,   1,  1,   1, 189849, "Dreamwalker"                        }, // Dreamwalker (id=189849, n_ranks=1)
  {  127,  13,   1,  2,   1, 189857, "Tranquil Mind"                      }, // Tranquil Mind (id=189857, n_ranks=1)
  {  128,  13,   1,  3,   1, 189870, "Power of the Archdruid"             }, // Power of the Archdruid (id=189870, n_ranks=1)
  {  129,  13,   0,  0,   1, 189787, "Nature's Essence"                   }, // Nature's Essence (id=189787, n_ranks=1)
  {  130,  13,   0,  4,   3, 189772, "Knowledge of the Ancients"          }, // Knowledge of the Ancients (id=189772, n_ranks=6)
  {  131,  13,   0,  5,   3, 189768, "Seeds of the World Tree"            }, // Seeds of the World Tree (id=189768, n_ranks=6)
  {  132,  13,   0,  6,   3, 189760, "Essence of Nordrassil"              }, // Essence of Nordrassil (id=189760, n_ranks=6)
  {  133,  13,   0,  7,   3, 189757, "Infusion of Nature"                 }, // Infusion of Nature (id=189757, n_ranks=6)
  {  134,  13,   0,  8,   3, 189754, "Armor of the Ancients"              }, // Armor of the Ancients (id=189754, n_ranks=6)
  {  135,  13,   0,  9,   3, 189749, "Natural Mending"                    }, // Natural Mending (id=189749, n_ranks=6)
  {  136,  13,   0, 10,   3, 189744, "Blessing of the World Tree"         }, // Blessing of the World Tree (id=189744, n_ranks=6)
  {  137,  13,   0, 11,   3, 186396, "Persistence"                        }, // Persistence (id=186396, n_ranks=6)
  {  138,  13,   0,  0,   1, 186393, "Blooms of G'Hanir"                  }, // Blooms of G'Hanir (id=186393, n_ranks=1)
  {  139,  13,   0,  0,   1, 186372, "Mark of Shifting"                   }, // Mark of Shifting (id=186372, n_ranks=1)
  {  140,  13,   0, 12,   3, 186320, "Grovewalker"                        }, // Grovewalker (id=186320, n_ranks=6)
  { 1195,  13,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1254,  13,   8,  0,   0, 213428, "Artificial Damage"                  }, // Artificial Damage (id=213428, n_ranks=1)
  { 1335,  13,   0,  0,   1, 222644, "Nature's Vigor"                     }, // Nature's Vigor (id=222644, n_ranks=1)
  { 1367,  13,   5,  0,  20, 214913, "G'Hanir's Bloom"                    }, // G'Hanir's Bloom (id=214913, n_ranks=20)
  {  272,  15,   0,  4,   3, 192450, "Iron Heart"                         }, // Iron Heart (id=192450, n_ranks=6)
  {  273,  15,   0,  5,   3, 192453, "Meat Shield"                        }, // Meat Shield (id=192453, n_ranks=6)
  {  274,  15,   0,  6,   3, 192447, "Grim Perseverance"                  }, // Grim Perseverance (id=192447, n_ranks=6)
  {  275,  15,   0,  7,   3, 192457, "Veinrender"                         }, // Veinrender (id=192457, n_ranks=6)
  {  276,  15,   0,  8,   3, 192460, "Coagulopathy"                       }, // Coagulopathy (id=192460, n_ranks=6)
  {  277,  15,   0,  9,   3, 192542, "Vampiric Fangs"                     }, // Vampiric Fangs (id=192542, n_ranks=6)
  {  278,  15,   0, 10,   3, 192464, "All-Consuming Rot"                  }, // All-Consuming Rot (id=192464, n_ranks=6)
  {  279,  15,   0, 11,   3, 192514, "Dance of Darkness"                  }, // Dance of Darkness (id=192514, n_ranks=6)
  {  280,  15,   0, 12,   3, 192538, "Bonebreaker"                        }, // Bonebreaker (id=192538, n_ranks=6)
  {  281,  15,   0,  0,   1, 192557, "Rattling Bones"                     }, // Rattling Bones (id=192557, n_ranks=1)
  {  282,  15,   0,  0,   1, 192548, "Blood Feast"                        }, // Blood Feast (id=192548, n_ranks=1)
  {  283,  15,   0,  0,   1, 192570, "Mouth of Hell"                      }, // Mouth of Hell (id=192570, n_ranks=1)
  {  284,  15,   1,  1,   1, 192558, "Skeletal Shattering"                }, // Skeletal Shattering (id=192558, n_ranks=1)
  {  285,  15,   1,  2,   1, 192567, "Unending Thirst"                    }, // Unending Thirst (id=192567, n_ranks=1)
  {  286,  15,   1,  3,   1, 193213, "Umbilicus Eternus"                  }, // Umbilicus Eternus (id=193213, n_ranks=1)
  {  289,  15,  18,  0,   0, 205223, "Consumption"                        }, // Consumption (id=205223, n_ranks=1)
  { 1187,  15,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1331,  15,   0,  0,   1, 221775, "Sanguinary Affinity"                }, // Sanguinary Affinity (id=221775, n_ranks=1)
  { 1359,  15,   5,  0,  20, 214903, "The Hungering Maw"                  }, // The Hungering Maw (id=214903, n_ranks=20)
  { 1451,  15,   8,  0,   0, 226829, "Artificial Damage"                  }, // Artificial Damage (id=226829, n_ranks=1)
  {  149,  16,  18,  0,   0, 220143, "Apocalypse"                         }, // Apocalypse (id=220143, n_ranks=1)
  {  150,  16,   1,  1,   1, 191760, "The Shambler"                       }, // The Shambler (id=191760, n_ranks=1)
  {  151,  16,   1,  2,   1, 191741, "Double Doom"                        }, // Double Doom (id=191741, n_ranks=1)
  {  152,  16,   1,  3,   1, 191747, "Scourge of Worlds"                  }, // Scourge of Worlds (id=191747, n_ranks=1)
  {  153,  16,   0,  0,   1, 191637, "Portal to the Underworld"           }, // Portal to the Underworld (id=191637, n_ranks=1)
  {  154,  16,   0,  0,   1, 191731, "Armies of the Damned"               }, // Armies of the Damned (id=191731, n_ranks=1)
  {  156,  16,   0,  4,   3, 191419, "Deadliest Coil"                     }, // Deadliest Coil (id=191419, n_ranks=6)
  {  157,  16,   0,  5,   3, 191442, "Rotten Touch"                       }, // Rotten Touch (id=191442, n_ranks=6)
  {  158,  16,   0,  6,   3, 191485, "Plaguebearer"                       }, // Plaguebearer (id=191485, n_ranks=6)
  {  262,  16,   0, 11,   3, 191565, "Deadly Durability"                  }, // Deadly Durability (id=191565, n_ranks=6)
  {  263,  16,   0,  0,   1, 191721, "Gravitational Pull"                 }, // Gravitational Pull (id=191721, n_ranks=1)
  {  264,  16,   0,  7,   3, 191592, "Runic Tattoos"                      }, // Runic Tattoos (id=191592, n_ranks=6)
  {  265,  16,   0,  8,   3, 191494, "Scourge the Unbeliever"             }, // Scourge the Unbeliever (id=191494, n_ranks=6)
  {  266,  16,   0,  9,   3, 191488, "The Darkest Crusade"                }, // The Darkest Crusade (id=191488, n_ranks=6)
  {  267,  16,   0, 12,   3, 191584, "Unholy Endurance"                   }, // Unholy Endurance (id=191584, n_ranks=6)
  { 1119,  16,   0, 10,   3, 208598, "Eternal Agony"                      }, // Eternal Agony (id=208598, n_ranks=6)
  { 1189,  16,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1333,  16,   0,  0,   1, 218280, "Feast of Souls"                     }, // Feast of Souls (id=218280, n_ranks=1)
  { 1361,  16,   5,  0,  20, 214906, "Fleshsearer"                        }, // Fleshsearer (id=214906, n_ranks=20)
  {  851,  17,  18,  0,   0, 209782, "Goremaw's Bite"                     }, // Goremaw's Bite (id=209782, n_ranks=1)
  {  852,  17,   0,  4,   3, 197231, "The Quiet Knife"                    }, // The Quiet Knife (id=197231, n_ranks=6)
  {  853,  17,   0,  5,   3, 197233, "Demon's Kiss"                       }, // Demon's Kiss (id=197233, n_ranks=6)
  {  854,  17,   0,  6,   3, 197234, "Gutripper"                          }, // Gutripper (id=197234, n_ranks=6)
  {  855,  17,   0,  7,   3, 197235, "Precision Strike"                   }, // Precision Strike (id=197235, n_ranks=6)
  {  856,  17,   0,  8,   3, 197369, "Fortune's Bite"                     }, // Fortune's Bite (id=197369, n_ranks=6)
  {  857,  17,   0,  9,   3, 197386, "Soul Shadows"                       }, // Soul Shadows (id=197386, n_ranks=6)
  {  858,  17,   0, 10,   3, 197239, "Energetic Stabbing"                 }, // Energetic Stabbing (id=197239, n_ranks=6)
  {  859,  17,   0, 11,   3, 197241, "Catlike Reflexes"                   }, // Catlike Reflexes (id=197241, n_ranks=6)
  {  860,  17,   0, 12,   3, 197244, "Ghost Armor"                        }, // Ghost Armor (id=197244, n_ranks=6)
  {  861,  17,   0,  0,   1, 197610, "Second Shuriken"                    }, // Second Shuriken (id=197610, n_ranks=1)
  {  862,  17,   0,  0,   1, 197256, "Flickering Shadows"                 }, // Flickering Shadows (id=197256, n_ranks=1)
  {  863,  17,   0,  0,   1, 197604, "Embrace of Darkness"                }, // Embrace of Darkness (id=197604, n_ranks=1)
  {  864,  17,   1,  1,   1, 209835, "Akaari's Soul"                      }, // Akaari's Soul (id=209835, n_ranks=1)
  {  865,  17,   1,  2,   1, 209781, "Shadow Nova"                        }, // Shadow Nova (id=209781, n_ranks=1)
  {  866,  17,   1,  3,   1, 197406, "Finality"                           }, // Finality (id=197406, n_ranks=1)
  { 1213,  17,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1349,  17,   0,  0,   1, 221856, "Shadow Fangs"                       }, // Shadow Fangs (id=221856, n_ranks=1)
  { 1386,  17,   5,  0,  20, 214930, "Legionblade"                        }, // Legionblade (id=214930, n_ranks=20)
  { 1068,  34,  18,  0,   0, 203415, "Fury of the Eagle"                  }, // Fury of the Eagle (id=203415, n_ranks=1)
  { 1070,  34,   0,  4,   3, 203566, "Sharpened Fang"                     }, // Sharpened Fang (id=203566, n_ranks=6)
  { 1071,  34,   0,  5,   3, 203577, "My Beloved Monster"                 }, // My Beloved Monster (id=203577, n_ranks=6)
  { 1072,  34,   0,  6,   3, 203578, "Lacerating Talons"                  }, // Lacerating Talons (id=203578, n_ranks=6)
  { 1073,  34,   0,  7,   3, 203638, "Raptor's Cry"                       }, // Raptor's Cry (id=203638, n_ranks=6)
  { 1074,  34,   0,  8,   3, 203669, "Fluffy, Go"                         }, // Fluffy, Go (id=203669, n_ranks=6)
  { 1075,  34,   0,  9,   3, 203670, "Explosive Force"                    }, // Explosive Force (id=203670, n_ranks=6)
  { 1076,  34,   0, 10,   3, 203673, "Hellcarver"                         }, // Hellcarver (id=203673, n_ranks=6)
  { 1077,  34,   0, 11,   3, 224764, "Bird of Prey"                       }, // Bird of Prey (id=224764, n_ranks=6)
  { 1078,  34,   0, 12,   3, 203749, "Hunter's Bounty"                    }, // Hunter's Bounty (id=203749, n_ranks=6)
  { 1079,  34,   0,  0,   1, 225092, "Embrace of the Aspects"             }, // Embrace of the Aspects (id=225092, n_ranks=1)
  { 1080,  34,   0,  0,   1, 203752, "Hunter's Guile"                     }, // Hunter's Guile (id=203752, n_ranks=1)
  { 1081,  34,   0,  0,   1, 203754, "Terms of Engagement"                }, // Terms of Engagement (id=203754, n_ranks=1)
  { 1082,  34,   1,  1,   1, 203755, "Aspect of the Skylord"              }, // Aspect of the Skylord (id=203755, n_ranks=1)
  { 1083,  34,   1,  2,   1, 203563, "Talon Strike"                       }, // Talon Strike (id=203563, n_ranks=1)
  { 1084,  34,   1,  3,   1, 203757, "Eagle's Bite"                       }, // Eagle's Bite (id=203757, n_ranks=1)
  { 1198,  34,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1338,  34,   0,  0,   1, 221773, "Iron Talons"                        }, // Iron Talons (id=221773, n_ranks=1)
  { 1370,  34,   5,  0,  20, 214916, "Voice of the Wild Gods"             }, // Voice of the Wild Gods (id=214916, n_ranks=20)
  {  980,  35,   1,  1,   1, 200875, "Juggernaut"                         }, // Juggernaut (id=200875, n_ranks=1)
  {  981,  35,   1,  2,   1, 200845, "Rage of the Valarjar"               }, // Rage of the Valarjar (id=200845, n_ranks=1)
  {  982,  35,   1,  3,   1, 200872, "Odyn's Champion"                    }, // Odyn's Champion (id=200872, n_ranks=1)
  {  984,  35,  18,  0,   0, 205545, "Odyn's Fury"                        }, // Odyn's Fury (id=205545, n_ranks=1)
  {  985,  35,   0,  0,   1, 200871, "Focus In Chaos"                     }, // Focus In Chaos (id=200871, n_ranks=1)
  {  986,  35,   0,  0,   1, 200870, "Helya's Wrath"                      }, // Helya's Wrath (id=200870, n_ranks=1)
  {  987,  35,   0,  0,   1, 200863, "Sense Death"                        }, // Sense Death (id=200863, n_ranks=1)
  {  988,  35,   0,  4,   3, 200846, "Deathdealer"                        }, // Deathdealer (id=200846, n_ranks=6)
  {  989,  35,   0,  5,   3, 216273, "Wild Slashes"                       }, // Wild Slashes (id=216273, n_ranks=6)
  {  990,  35,   0,  6,   3, 200849, "Wrath and Fury"                     }, // Wrath and Fury (id=200849, n_ranks=6)
  {  991,  35,   0,  7,   3, 200853, "Unstoppable"                        }, // Unstoppable (id=200853, n_ranks=6)
  {  992,  35,   0,  8,   3, 200856, "Uncontrolled Rage"                  }, // Uncontrolled Rage (id=200856, n_ranks=6)
  {  993,  35,   0, 11,   3, 200857, "Battle Scars"                       }, // Battle Scars (id=200857, n_ranks=6)
  {  994,  35,   0, 12,   3, 200859, "Bloodcraze"                         }, // Bloodcraze (id=200859, n_ranks=6)
  {  995,  35,   0,  9,   3, 200860, "Unrivaled Strength"                 }, // Unrivaled Strength (id=200860, n_ranks=6)
  {  996,  35,   0, 10,   3, 200861, "Raging Berserker"                   }, // Raging Berserker (id=200861, n_ranks=6)
  { 1221,  35,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1357,  35,   0,  0,   1, 200847, "Thirst for Battle"                  }, // Thirst for Battle (id=200847, n_ranks=1)
  { 1394,  35,   5,  0,  20, 214938, "Titanic Power"                      }, // Titanic Power (id=214938, n_ranks=20)
  { 1136,  36,  18,  0,   0, 209577, "Warbreaker"                         }, // Warbreaker (id=209577, n_ranks=1)
  { 1137,  36,   1,  1,   1, 209574, "Shattered Defenses"                 }, // Shattered Defenses (id=209574, n_ranks=1)
  { 1138,  36,   1,  2,   1, 209573, "Void Cleave"                        }, // Void Cleave (id=209573, n_ranks=1)
  { 1139,  36,   1,  3,   1, 209566, "Corrupted Blood of Zakajz"          }, // Corrupted Blood of Zakajz (id=209566, n_ranks=1)
  { 1140,  36,   0,  0,   1, 209559, "Defensive Measures"                 }, // Defensive Measures (id=209559, n_ranks=1)
  { 1141,  36,   0,  0,   1, 209554, "Focus in Battle"                    }, // Focus in Battle (id=209554, n_ranks=1)
  { 1142,  36,   0,  0,   1, 209548, "Will of the First King"             }, // Will of the First King (id=209548, n_ranks=1)
  { 1143,  36,   0,  4,   3, 209459, "Unending Rage"                      }, // Unending Rage (id=209459, n_ranks=6)
  { 1144,  36,   0,  5,   3, 209462, "One Against Many"                   }, // One Against Many (id=209462, n_ranks=6)
  { 1145,  36,   0,  6,   3, 209472, "Crushing Blows"                     }, // Crushing Blows (id=209472, n_ranks=6)
  { 1146,  36,   0,  7,   3, 216274, "Many Will Fall"                     }, // Many Will Fall (id=216274, n_ranks=6)
  { 1147,  36,   0,  8,   3, 209481, "Deathblow"                          }, // Deathblow (id=209481, n_ranks=6)
  { 1148,  36,   0, 11,   3, 209483, "Tactical Advance"                   }, // Tactical Advance (id=209483, n_ranks=6)
  { 1149,  36,   0,  9,   3, 209492, "Precise Strikes"                    }, // Precise Strikes (id=209492, n_ranks=6)
  { 1150,  36,   0, 10,   3, 209494, "Exploit the Weakness"               }, // Exploit the Weakness (id=209494, n_ranks=6)
  { 1151,  36,   0, 12,   3, 209541, "Touch of Zakajz"                    }, // Touch of Zakajz (id=209541, n_ranks=6)
  { 1220,  36,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1356,  36,   0,  0,   1, 209480, "Thoradin's Might"                   }, // Thoradin's Might (id=209480, n_ranks=1)
  { 1393,  36,   5,  0,  20, 214937, "Unbreakable Steel"                  }, // Unbreakable Steel (id=214937, n_ranks=20)
  { 1170,  37,  18,  0,   0, 211714, "Thal'kiel's Consumption"            }, // Thal'kiel's Consumption (id=211714, n_ranks=1)
  { 1171,  37,   0,  4,   3, 211108, "Summoner's Prowess"                 }, // Summoner's Prowess (id=211108, n_ranks=6)
  { 1172,  37,   0,  5,   3, 211126, "Legionwrath"                        }, // Legionwrath (id=211126, n_ranks=6)
  { 1173,  37,   0,  6,   3, 211106, "The Doom of Azeroth"                }, // The Doom of Azeroth (id=211106, n_ranks=6)
  { 1174,  37,   0,  7,   3, 211123, "Sharpened Dreadfangs"               }, // Sharpened Dreadfangs (id=211123, n_ranks=6)
  { 1175,  37,   0,  8,   3, 211105, "Dirty Hands"                        }, // Dirty Hands (id=211105, n_ranks=6)
  { 1176,  37,   0,  9,   3, 211119, "Infernal Furnace"                   }, // Infernal Furnace (id=211119, n_ranks=6)
  { 1177,  37,   0, 10,   3, 211099, "Maw of Shadows"                     }, // Maw of Shadows (id=211099, n_ranks=6)
  { 1178,  37,   0, 11,   3, 211144, "Open Link"                          }, // Open Link (id=211144, n_ranks=6)
  { 1179,  37,   0, 12,   3, 211131, "Firm Resolve"                       }, // Firm Resolve (id=211131, n_ranks=6)
  { 1180,  37,   0,  0,   1, 211158, "Imp-erator"                         }, // Imp-erator (id=211158, n_ranks=1)
  { 1181,  37,   0,  0,   1, 218567, "Soul Skin"                          }, // Soul Skin (id=218567, n_ranks=1)
  { 1182,  37,   0,  0,   1, 218572, "Doom, Doubled"                      }, // Doom, Doubled (id=218572, n_ranks=1)
  { 1183,  37,   1,  1,   1, 211219, "The Expendables"                    }, // The Expendables (id=211219, n_ranks=1)
  { 1184,  37,   1,  2,   1, 211720, "Thal'kiel's Discord"                }, // Thal'kiel's Discord (id=211720, n_ranks=1)
  { 1185,  37,   1,  3,   1, 211530, "Stolen Power"                       }, // Stolen Power (id=211530, n_ranks=1)
  { 1218,  37,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1354,  37,   0,  0,   1, 221882, "Breath of Thal'kiel"                }, // Breath of Thal'kiel (id=221882, n_ranks=1)
  { 1391,  37,   5,  0,  20, 214935, "Thal'kiel's Lingering Power"        }, // Thal'kiel's Lingering Power (id=214935, n_ranks=20)
  {  803,  38,  18,  0,   0, 196586, "Dimensional Rift"                   }, // Dimensional Rift (id=196586, n_ranks=1)
  {  804,  38,   0,  4,   3, 196211, "Master of Disaster"                 }, // Master of Disaster (id=196211, n_ranks=6)
  {  805,  38,   0,  5,   3, 196217, "Chaotic Instability"                }, // Chaotic Instability (id=196217, n_ranks=6)
  {  806,  38,   0,  6,   3, 196222, "Fire and the Flames"                }, // Fire and the Flames (id=196222, n_ranks=6)
  {  807,  38,   0,  7,   3, 196227, "Residual Flames"                    }, // Residual Flames (id=196227, n_ranks=6)
  {  808,  38,   0,  8,   3, 196236, "Soulsnatcher"                       }, // Soulsnatcher (id=196236, n_ranks=6)
  {  809,  38,   0,  9,   3, 196432, "Burning Hunger"                     }, // Burning Hunger (id=196432, n_ranks=6)
  {  810,  38,   0, 10,   3, 196258, "Fire From the Sky"                  }, // Fire From the Sky (id=196258, n_ranks=6)
  {  811,  38,   0, 11,   3, 196301, "Devourer of Life"                   }, // Devourer of Life (id=196301, n_ranks=6)
  {  812,  38,   0, 12,   3, 196305, "Eternal Struggle"                   }, // Eternal Struggle (id=196305, n_ranks=6)
  {  813,  38,   0,  0,   1, 196675, "Planeswalker"                       }, // Planeswalker (id=196675, n_ranks=1)
  {  814,  38,   0,  0,   1, 215223, "Demonic Durability"                 }, // Demonic Durability (id=215223, n_ranks=1)
  {  815,  38,   0,  0,   1, 215273, "Impish Incineration"                }, // Impish Incineration (id=215273, n_ranks=1)
  {  816,  38,   1,  1,   1, 219415, "Dimension Ripper"                   }, // Dimension Ripper (id=219415, n_ranks=1)
  {  817,  38,   1,  2,   1, 219195, "Conflagration of Chaos"             }, // Conflagration of Chaos (id=219195, n_ranks=1)
  {  818,  38,   1,  3,   1, 224103, "Lord of Flames"                     }, // Lord of Flames (id=224103, n_ranks=1)
  { 1219,  38,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1355,  38,   0,  0,   1, 215183, "Flames of the Pit"                  }, // Flames of the Pit (id=215183, n_ranks=1)
  { 1392,  38,   5,  0,  20, 214936, "Stolen Power"                       }, // Stolen Power (id=214936, n_ranks=20)
  {  915,  39,   0,  4,   3, 199111, "Inimitable Agony"                   }, // Inimitable Agony (id=199111, n_ranks=6)
  {  916,  39,   0,  5,   3, 199112, "Hideous Corruption"                 }, // Hideous Corruption (id=199112, n_ranks=6)
  {  917,  39,   0,  6,   3, 199120, "Drained to a Husk"                  }, // Drained to a Husk (id=199120, n_ranks=6)
  {  918,  39,   0,  7,   3, 199152, "Inherently Unstable"                }, // Inherently Unstable (id=199152, n_ranks=6)
  {  919,  39,   0,  8,   3, 199153, "Seeds of Doom"                      }, // Seeds of Doom (id=199153, n_ranks=6)
  {  920,  39,   0,  9,   3, 199158, "Perdition"                          }, // Perdition (id=199158, n_ranks=6)
  {  921,  39,   0, 10,   3, 199163, "Shadowy Incantations"               }, // Shadowy Incantations (id=199163, n_ranks=6)
  {  922,  39,   0, 11,   3, 199212, "Shadows of the Flesh"               }, // Shadows of the Flesh (id=199212, n_ranks=6)
  {  923,  39,   0, 12,   3, 199214, "Long Dark Night of the Soul"        }, // Long Dark Night of the Soul (id=199214, n_ranks=6)
  {  924,  39,   0,  0,   1, 199220, "Sweet Souls"                        }, // Sweet Souls (id=199220, n_ranks=1)
  {  925,  39,   0,  0,   1, 199257, "Fatal Echoes"                       }, // Fatal Echoes (id=199257, n_ranks=1)
  {  926,  39,   0,  0,   1, 199282, "Compounding Horror"                 }, // Compounding Horror (id=199282, n_ranks=1)
  {  927,  39,   1,  0,   1, 199471, "Soul Flame"                         }, // Soul Flame (id=199471, n_ranks=1)
  {  928,  39,   1,  0,   1, 199472, "Wrath of Consumption"               }, // Wrath of Consumption (id=199472, n_ranks=1)
  {  929,  39,   1,  0,   1, 201424, "Harvester of Souls"                 }, // Harvester of Souls (id=201424, n_ranks=1)
  {  999,  39,  18,  0,   0, 216698, "Reap Souls"                         }, // Reap Souls (id=216698, n_ranks=1)
  { 1217,  39,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1353,  39,   0,  0,   1, 221862, "Crystalline Shadows"                }, // Crystalline Shadows (id=221862, n_ranks=1)
  { 1390,  39,   5,  0,  20, 214934, "Soulstealer"                        }, // Soulstealer (id=214934, n_ranks=20)
  {  291,  40,  18,  0,   0, 205495, "Stormkeeper"                        }, // Stormkeeper (id=205495, n_ranks=1)
  {  292,  40,   0,  0,   1, 191602, "Static Overload"                    }, // Static Overload (id=191602, n_ranks=1)
  {  293,  40,   0,  0,   1, 191647, "Master of the Elements"             }, // Master of the Elements (id=191647, n_ranks=1)
  {  294,  40,   0,  0,   1, 191512, "Elementalist"                       }, // Elementalist (id=191512, n_ranks=1)
  {  295,  40,   1,  1,   1, 191717, "Fury of the Storms"                 }, // Fury of the Storms (id=191717, n_ranks=1)
  {  296,  40,   1,  2,   1, 191861, "Power of the Maelstrom"             }, // Power of the Maelstrom (id=191861, n_ranks=1)
  {  297,  40,   1,  3,   1, 192630, "Volcanic Inferno"                   }, // Volcanic Inferno (id=192630, n_ranks=1)
  {  298,  40,   0,  4,   3, 191493, "Call the Thunder"                   }, // Call the Thunder (id=191493, n_ranks=6)
  {  299,  40,   0,  5,   3, 191499, "The Ground Trembles"                }, // The Ground Trembles (id=191499, n_ranks=6)
  {  300,  40,   0,  6,   3, 191504, "Lava Imbued"                        }, // Lava Imbued (id=191504, n_ranks=6)
  {  301,  40,   0,  7,   3, 191740, "Firestorm"                          }, // Firestorm (id=191740, n_ranks=6)
  {  302,  40,   0, 11,   3, 191569, "Protection of the Elements"         }, // Protection of the Elements (id=191569, n_ranks=6)
  {  303,  40,   0,  8,   3, 191572, "Molten Blast"                       }, // Molten Blast (id=191572, n_ranks=6)
  {  304,  40,   0,  9,   3, 191577, "Electric Discharge"                 }, // Electric Discharge (id=191577, n_ranks=6)
  {  305,  40,   0, 12,   3, 191582, "Shamanistic Healing"                }, // Shamanistic Healing (id=191582, n_ranks=6)
  {  306,  40,   0, 10,   3, 191598, "Earthen Attunement"                 }, // Earthen Attunement (id=191598, n_ranks=6)
  { 1214,  40,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1350,  40,   0,  0,   1, 215414, "Surge of Power"                     }, // Surge of Power (id=215414, n_ranks=1)
  { 1387,  40,   5,  0,  20, 214931, "Stormkeeper's Power"                }, // Stormkeeper's Power (id=214931, n_ranks=20)
  {  899,  41,  18,  0,   0, 204945, "Doom Winds"                         }, // Doom Winds (id=204945, n_ranks=1)
  {  900,  41,   1,  1,   1, 198736, "Unleash Doom"                       }, // Unleash Doom (id=198736, n_ranks=1)
  {  901,  41,   1,  2,   1, 198505, "Doom Wolves"                        }, // Doom Wolves (id=198505, n_ranks=1)
  {  902,  41,   0,  0,   1, 198361, "Raging Storms"                      }, // Raging Storms (id=198361, n_ranks=1)
  {  903,  41,   0,  0,   1, 198367, "Stormflurry"                        }, // Stormflurry (id=198367, n_ranks=1)
  {  904,  41,   0,  0,   1, 198434, "Alpha Wolf"                         }, // Alpha Wolf (id=198434, n_ranks=1)
  {  905,  41,   0,  4,   3, 215381, "Weapons of the Elements"            }, // Weapons of the Elements (id=215381, n_ranks=6)
  {  906,  41,   0,  5,   3, 198236, "Forged in Lava"                     }, // Forged in Lava (id=198236, n_ranks=6)
  {  907,  41,   0, 10,   3, 198238, "Spirit of the Maelstrom"            }, // Spirit of the Maelstrom (id=198238, n_ranks=6)
  {  908,  41,   0,  6,   3, 198247, "Wind Surge"                         }, // Wind Surge (id=198247, n_ranks=6)
  {  909,  41,   0, 12,   3, 198248, "Elemental Healing"                  }, // Elemental Healing (id=198248, n_ranks=6)
  {  910,  41,   0,  7,   3, 198292, "Wind Strikes"                       }, // Wind Strikes (id=198292, n_ranks=6)
  {  911,  41,   0, 11,   3, 198296, "Spiritual Healing"                  }, // Spiritual Healing (id=198296, n_ranks=6)
  {  912,  41,   0,  8,   3, 198299, "Gathering Storms"                   }, // Gathering Storms (id=198299, n_ranks=6)
  {  913,  41,   0,  9,   3, 198349, "Gathering of the Maelstrom"         }, // Gathering of the Maelstrom (id=198349, n_ranks=6)
  {  930,  41,   1,  3,   1, 199107, "Doom Vortex"                        }, // Doom Vortex (id=199107, n_ranks=1)
  { 1215,  41,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1351,  41,   0,  0,   1, 198228, "Hammer of Storms"                   }, // Hammer of Storms (id=198228, n_ranks=1)
  { 1388,  41,   5,  0,  20, 214932, "Earthshattering Blows"              }, // Earthshattering Blows (id=214932, n_ranks=20)
  { 1102,  42,  18,  0,   0, 207778, "Gift of the Queen"                  }, // Gift of the Queen (id=207778, n_ranks=1)
  { 1103,  42,   0,  4,   3, 207088, "Tidal Chains"                       }, // Tidal Chains (id=207088, n_ranks=6)
  { 1104,  42,   0,  5,   3, 207092, "Buffeting Waves"                    }, // Buffeting Waves (id=207092, n_ranks=6)
  { 1105,  42,   0,  6,   3, 207118, "Pull of the Sea"                    }, // Pull of the Sea (id=207118, n_ranks=6)
  { 1106,  42,   0,  7,   3, 207206, "Wavecrash"                          }, // Wavecrash (id=207206, n_ranks=6)
  { 1107,  42,   0,  8,   3, 207255, "Empowered Droplets"                 }, // Empowered Droplets (id=207255, n_ranks=6)
  { 1108,  42,   0,  9,   3, 207285, "Queen Ascendant"                    }, // Queen Ascendant (id=207285, n_ranks=6)
  { 1109,  42,   0, 10,   3, 207348, "Floodwaters"                        }, // Floodwaters (id=207348, n_ranks=6)
  { 1110,  42,   0, 11,   3, 207351, "Ghost in the Mist"                  }, // Ghost in the Mist (id=207351, n_ranks=6)
  { 1111,  42,   0, 12,   3, 207354, "Caress of the Tidemother"           }, // Caress of the Tidemother (id=207354, n_ranks=6)
  { 1112,  42,   0,  0,   1, 207355, "Sense of Urgency"                   }, // Sense of Urgency (id=207355, n_ranks=1)
  { 1113,  42,   0,  0,   1, 207356, "Refreshing Currents"                }, // Refreshing Currents (id=207356, n_ranks=1)
  { 1114,  42,   0,  0,   1, 207357, "Servant of the Queen"               }, // Servant of the Queen (id=207357, n_ranks=1)
  { 1115,  42,   1,  1,   1, 207358, "Tidal Pools"                        }, // Tidal Pools (id=207358, n_ranks=1)
  { 1116,  42,   1,  2,   1, 207360, "Queen's Decree"                     }, // Queen's Decree (id=207360, n_ranks=1)
  { 1117,  42,   1,  3,   1, 207362, "Cumulative Upkeep"                  }, // Cumulative Upkeep (id=207362, n_ranks=1)
  { 1216,  42,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1258,  42,   8,  0,   0, 213428, "Artificial Damage"                  }, // Artificial Damage (id=213428, n_ranks=1)
  { 1352,  42,   0,  0,   1, 224841, "Grace of the Sea"                   }, // Grace of the Sea (id=224841, n_ranks=1)
  { 1389,  42,   5,  0,  20, 214933, "Flow of the Tides"                  }, // Flow of the Tides (id=214933, n_ranks=20)
  {  323,  43,   0,  4,   3, 192310, "Toxic Blades"                       }, // Toxic Blades (id=192310, n_ranks=6)
  {  324,  43,   0,  5,   3, 192315, "Serrated Edge"                      }, // Serrated Edge (id=192315, n_ranks=6)
  {  325,  43,   0,  6,   3, 192318, "Master Alchemist"                   }, // Master Alchemist (id=192318, n_ranks=6)
  {  326,  43,   0, 11,   3, 192323, "Fade into Shadows"                  }, // Fade into Shadows (id=192323, n_ranks=6)
  {  327,  43,   0,  7,   3, 192326, "Balanced Blades"                    }, // Balanced Blades (id=192326, n_ranks=6)
  {  328,  43,   0,  8,   3, 192329, "Gushing Wound"                      }, // Gushing Wound (id=192329, n_ranks=6)
  {  329,  43,   0, 12,   3, 192345, "Shadow Walker"                      }, // Shadow Walker (id=192345, n_ranks=6)
  {  330,  43,   0,  9,   3, 192349, "Master Assassin"                    }, // Master Assassin (id=192349, n_ranks=6)
  {  331,  43,   0, 10,   3, 192376, "Poison Knives"                      }, // Poison Knives (id=192376, n_ranks=6)
  {  332,  43,   0,  0,   1, 192384, "Urge to Kill"                       }, // Urge to Kill (id=192384, n_ranks=1)
  {  333,  43,   0,  0,   1, 192422, "Shadow Swiftness"                   }, // Shadow Swiftness (id=192422, n_ranks=1)
  {  334,  43,   0,  0,   1, 192424, "Surge of Toxins"                    }, // Surge of Toxins (id=192424, n_ranks=1)
  {  335,  43,   1,  0,   1, 192428, "From the Shadows"                   }, // From the Shadows (id=192428, n_ranks=1)
  {  337,  43,   1,  0,   1, 192657, "Bag of Tricks"                      }, // Bag of Tricks (id=192657, n_ranks=1)
  {  346,  43,  18,  0,   0, 192759, "Kingsbane"                          }, // Kingsbane (id=192759, n_ranks=1)
  {  347,  43,   1,  0,   1, 192923, "Blood of the Assassinated"          }, // Blood of the Assassinated (id=192923, n_ranks=1)
  { 1211,  43,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1276,  43,   0,  0,   1, 214368, "Assassin's Blades"                  }, // Assassin's Blades (id=214368, n_ranks=1)
  { 1384,  43,   5,  0,  20, 214928, "Slayer's Precision"                 }, // Slayer's Precision (id=214928, n_ranks=20)
  { 1052,  44,  18,  0,   0, 202665, "Curse of the Dreadblades"           }, // Curse of the Dreadblades (id=202665, n_ranks=1)
  { 1053,  44,   1,  1,   1, 202897, "Blunderbuss"                        }, // Blunderbuss (id=202897, n_ranks=1)
  { 1054,  44,   1,  2,   1, 202820, "Greed"                              }, // Greed (id=202820, n_ranks=1)
  { 1055,  44,   1,  3,   1, 202769, "Blurred Time"                       }, // Blurred Time (id=202769, n_ranks=1)
  { 1056,  44,   0,  0,   1, 202755, "Deception"                          }, // Deception (id=202755, n_ranks=1)
  { 1057,  44,   0,  0,   1, 202753, "Hidden Blade"                       }, // Hidden Blade (id=202753, n_ranks=1)
  { 1058,  44,   0,  0,   1, 202628, "Blademaster"                        }, // Blademaster (id=202628, n_ranks=1)
  { 1059,  44,   0,  4,   3, 216230, "Black Powder"                       }, // Black Powder (id=216230, n_ranks=6)
  { 1060,  44,   0,  5,   3, 202507, "Blade Dancer"                       }, // Blade Dancer (id=202507, n_ranks=6)
  { 1061,  44,   0,  6,   3, 202514, "Fate's Thirst"                      }, // Fate's Thirst (id=202514, n_ranks=6)
  { 1062,  44,   0, 11,   3, 202521, "Fortune's Strike"                   }, // Fortune's Strike (id=202521, n_ranks=6)
  { 1063,  44,   0,  7,   3, 202522, "Gunslinger"                         }, // Gunslinger (id=202522, n_ranks=6)
  { 1064,  44,   0,  8,   3, 202524, "Fatebringer"                        }, // Fatebringer (id=202524, n_ranks=6)
  { 1065,  44,   0,  9,   3, 202530, "Fortune Strikes"                    }, // Fortune Strikes (id=202530, n_ranks=6)
  { 1066,  44,   0, 12,   3, 202533, "Ghostly Shell"                      }, // Ghostly Shell (id=202533, n_ranks=6)
  { 1067,  44,   0, 10,   3, 202907, "Fortune's Boon"                     }, // Fortune's Boon (id=202907, n_ranks=6)
  { 1212,  44,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1348,  44,   0,  0,   1, 202463, "Cursed Edges"                       }, // Cursed Edges (id=202463, n_ranks=1)
  { 1385,  44,   5,  0,  20, 214929, "Cursed Steel"                       }, // Cursed Steel (id=214929, n_ranks=20)
  {  834,  45,  18,  0,   0, 196684, "Invoke the Naaru"                   }, // Invoke the Naaru (id=196684, n_ranks=1)
  {  835,  45,   1,  1,   1, 196492, "Renew the Faith"                    }, // Renew the Faith (id=196492, n_ranks=1)
  {  836,  45,   1,  2,   1, 208065, "Light of T'uure"                    }, // Light of T'uure (id=208065, n_ranks=1)
  {  837,  45,   1,  3,   1, 196578, "Blessing of T'uure"                 }, // Blessing of T'uure (id=196578, n_ranks=1)
  {  838,  45,   0, 11,   3, 196489, "Power of the Naaru"                 }, // Power of the Naaru (id=196489, n_ranks=6)
  {  839,  45,   0,  0,   1, 196437, "Guardians of the Light"             }, // Guardians of the Light (id=196437, n_ranks=1)
  {  840,  45,   0,  0,   1, 196779, "Holy Mending"                       }, // Holy Mending (id=196779, n_ranks=1)
  {  841,  45,   0, 12,   3, 196355, "Trust in the Light"                 }, // Trust in the Light (id=196355, n_ranks=6)
  {  842,  45,   0,  5,   3, 196358, "Say Your Prayers"                   }, // Say Your Prayers (id=196358, n_ranks=6)
  {  843,  45,   0,  6,   3, 196416, "Serenity Now"                       }, // Serenity Now (id=196416, n_ranks=6)
  {  844,  45,   0,  7,   3, 196418, "Reverence"                          }, // Reverence (id=196418, n_ranks=6)
  {  845,  45,   0,  0,   1, 196419, "Focus in the Light"                 }, // Focus in the Light (id=196419, n_ranks=1)
  {  846,  45,   0,  8,   3, 196422, "Holy Hands"                         }, // Holy Hands (id=196422, n_ranks=6)
  {  847,  45,   0,  9,   3, 196429, "Hallowed Ground"                    }, // Hallowed Ground (id=196429, n_ranks=6)
  {  848,  45,   0, 10,   3, 196430, "Words of Healing"                   }, // Words of Healing (id=196430, n_ranks=6)
  {  849,  45,   0,  4,   3, 196434, "Holy Guidance"                      }, // Holy Guidance (id=196434, n_ranks=6)
  { 1209,  45,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1257,  45,   8,  0,   0, 213428, "Artificial Damage"                  }, // Artificial Damage (id=213428, n_ranks=1)
  { 1346,  45,   0,  0,   1, 222646, "Follower of the Light"              }, // Follower of the Light (id=222646, n_ranks=1)
  { 1380,  45,   5,  0,  20, 214926, "Beacon of Light"                    }, // Beacon of Light (id=214926, n_ranks=20)
  {  883,  46,  18,  0,   0, 207946, "Light's Wrath"                      }, // Light's Wrath (id=207946, n_ranks=1)
  {  884,  46,   0,  0,   1, 197779, "Taming the Shadows"                 }, // Taming the Shadows (id=197779, n_ranks=1)
  {  885,  46,   0,  0,   1, 197781, "Share in the Light"                 }, // Share in the Light (id=197781, n_ranks=1)
  {  886,  46,   1,  1,   1, 197815, "Barrier for the Devoted"            }, // Barrier for the Devoted (id=197815, n_ranks=1)
  {  887,  46,   1,  2,   1, 198074, "Sins of the Many"                   }, // Sins of the Many (id=198074, n_ranks=1)
  {  888,  46,   0,  4,   3, 197708, "Confession"                         }, // Confession (id=197708, n_ranks=6)
  {  889,  46,   0, 11,   3, 197711, "Vestments of Discipline"            }, // Vestments of Discipline (id=197711, n_ranks=6)
  {  890,  46,   0,  5,   3, 197713, "Pain is in Your Mind"               }, // Pain is in Your Mind (id=197713, n_ranks=6)
  {  891,  46,   0,  6,   3, 197715, "The Edge of Dark and Light"         }, // The Edge of Dark and Light (id=197715, n_ranks=6)
  {  892,  46,   0,  7,   3, 197716, "Burst of Light"                     }, // Burst of Light (id=197716, n_ranks=6)
  {  893,  46,   0,  8,   3, 197727, "Doomsayer"                          }, // Doomsayer (id=197727, n_ranks=6)
  {  894,  46,   0,  9,   3, 197729, "Shield of Faith"                    }, // Shield of Faith (id=197729, n_ranks=6)
  {  895,  46,   0, 10,   3, 197762, "Borrowed Time"                      }, // Borrowed Time (id=197762, n_ranks=6)
  {  896,  46,   0, 12,   3, 216212, "Darkest Shadows"                    }, // Darkest Shadows (id=216212, n_ranks=6)
  {  897,  46,   1,  3,   1, 198068, "Power of the Dark Side"             }, // Power of the Dark Side (id=198068, n_ranks=1)
  {  898,  46,   0,  0,   1, 197766, "Speed of the Pious"                 }, // Speed of the Pious (id=197766, n_ranks=1)
  { 1208,  46,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1345,  46,   0,  0,   1, 216122, "Invoke the Light"                   }, // Invoke the Light (id=216122, n_ranks=1)
  { 1379,  46,   5,  0,  20, 214925, "Forbidden Flame"                    }, // Forbidden Flame (id=214925, n_ranks=20)
  { 1436,  46,   8,  0,   0, 219655, "Artificial Damage"                  }, // Artificial Damage (id=219655, n_ranks=1)
  {  764,  47,  18,  0,   0, 205065, "Void Torrent"                       }, // Void Torrent (id=205065, n_ranks=1)
  {  765,  47,   0,  0,   1, 194024, "Thrive in the Shadows"              }, // Thrive in the Shadows (id=194024, n_ranks=1)
  {  766,  47,   0,  0,   1, 194026, "Sinister Thoughts"                  }, // Sinister Thoughts (id=194026, n_ranks=1)
  {  767,  47,   0,  4,   3, 194093, "Unleash the Shadows"                }, // Unleash the Shadows (id=194093, n_ranks=6)
  {  768,  47,   1,  1,   1, 193371, "Call to the Void"                   }, // Call to the Void (id=193371, n_ranks=1)
  {  769,  47,   1,  2,   1, 194179, "Sphere of Insanity"                 }, // Sphere of Insanity (id=194179, n_ranks=1)
  {  770,  47,   1,  3,   1, 194378, "Mass Hysteria"                      }, // Mass Hysteria (id=194378, n_ranks=1)
  {  771,  47,   0, 11,   3, 193642, "From the Shadows"                   }, // From the Shadows (id=193642, n_ranks=6)
  {  772,  47,   0,  5,   3, 193643, "Mind Shattering"                    }, // Mind Shattering (id=193643, n_ranks=6)
  {  773,  47,   0,  6,   3, 193644, "To the Pain"                        }, // To the Pain (id=193644, n_ranks=6)
  {  774,  47,   0,  7,   3, 193645, "Death's Embrace"                    }, // Death's Embrace (id=193645, n_ranks=6)
  {  775,  47,   0, 12,   3, 193647, "Thoughts of Insanity"               }, // Thoughts of Insanity (id=193647, n_ranks=6)
  {  776,  47,   0, 10,   3, 194002, "Creeping Shadows"                   }, // Creeping Shadows (id=194002, n_ranks=6)
  {  777,  47,   0,  8,   3, 194007, "Touch of Darkness"                  }, // Touch of Darkness (id=194007, n_ranks=6)
  {  778,  47,   0,  9,   3, 194016, "Void Corruption"                    }, // Void Corruption (id=194016, n_ranks=6)
  {  779,  47,   0,  0,   1, 194018, "Mental Fortitude"                   }, // Mental Fortitude (id=194018, n_ranks=1)
  { 1210,  47,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1347,  47,   0,  0,   1, 215322, "Void Siphon"                        }, // Void Siphon (id=215322, n_ranks=1)
  { 1381,  47,   5,  0,  20, 214927, "Darkening Whispers"                 }, // Darkening Whispers (id=214927, n_ranks=20)
  {  963,  48,   1,  2,   1, 200474, "Power of the Silver Hand"           }, // Power of the Silver Hand (id=200474, n_ranks=1)
  {  964,  48,   1,  3,   1, 200430, "Protection of Tyr"                  }, // Protection of Tyr (id=200430, n_ranks=1)
  {  965,  48,  18,  0,   0, 200652, "Tyr's Deliverance"                  }, // Tyr's Deliverance (id=200652, n_ranks=1)
  {  966,  48,   1,  1,   1, 200421, "The Light Saves"                    }, // The Light Saves (id=200421, n_ranks=1)
  {  967,  48,   0,  0,   1, 200407, "Protection of the Light"            }, // Protection of the Light (id=200407, n_ranks=1)
  {  968,  48,   0,  0,   1, 200373, "Vindicator"                         }, // Vindicator (id=200373, n_ranks=1)
  {  969,  48,   0, 11,   3, 200327, "Share the Burden"                   }, // Share the Burden (id=200327, n_ranks=6)
  {  970,  48,   0,  4,   3, 200326, "Focused Healing"                    }, // Focused Healing (id=200326, n_ranks=6)
  {  971,  48,   0,  5,   3, 200316, "Justice through Sacrifice"          }, // Justice through Sacrifice (id=200316, n_ranks=6)
  {  972,  48,   0,  6,   3, 200315, "Shock Treatment"                    }, // Shock Treatment (id=200315, n_ranks=6)
  {  973,  48,   0,  0,   1, 200311, "Templar of the Light"               }, // Templar of the Light (id=200311, n_ranks=1)
  {  974,  48,   0, 12,   3, 200302, "Knight of the Silver Hand"          }, // Knight of the Silver Hand (id=200302, n_ranks=6)
  {  975,  48,   0,  9,   3, 200298, "Blessings of the Silver Hand"       }, // Blessings of the Silver Hand (id=200298, n_ranks=6)
  {  976,  48,   0,  7,   3, 200296, "Expel the Darkness"                 }, // Expel the Darkness (id=200296, n_ranks=6)
  {  977,  48,   0,  8,   3, 200294, "Deliver the Light"                  }, // Deliver the Light (id=200294, n_ranks=6)
  { 1186,  48,   0, 10,   3, 200482, "Second Sunrise"                     }, // Second Sunrise (id=200482, n_ranks=6)
  { 1205,  48,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1256,  48,   8,  0,   0, 213428, "Artificial Damage"                  }, // Artificial Damage (id=213428, n_ranks=1)
  { 1342,  48,   0,  0,   1, 222648, "Virtues of the Light"               }, // Virtues of the Light (id=222648, n_ranks=1)
  { 1377,  48,   5,  0,  20, 214923, "Silver Touch"                       }, // Silver Touch (id=214923, n_ranks=20)
  { 1120,  49,  18,  0,   0, 209202, "Eye of Tyr"                         }, // Eye of Tyr (id=209202, n_ranks=1)
  { 1121,  49,   0,  4,   3, 209229, "Hammer Time"                        }, // Hammer Time (id=209229, n_ranks=6)
  { 1122,  49,   0,  5,   3, 209217, "Stern Judgment"                     }, // Stern Judgment (id=209217, n_ranks=6)
  { 1123,  49,   0,  6,   3, 209220, "Unflinching Defense"                }, // Unflinching Defense (id=209220, n_ranks=6)
  { 1124,  49,   0,  7,   3, 209226, "Righteous Crusader"                 }, // Righteous Crusader (id=209226, n_ranks=6)
  { 1125,  49,   0,  8,   3, 209223, "Scatter the Shadows"                }, // Scatter the Shadows (id=209223, n_ranks=6)
  { 1126,  49,   0,  9,   3, 209218, "Consecration in Flame"              }, // Consecration in Flame (id=209218, n_ranks=6)
  { 1127,  49,   0, 10,   3, 209216, "Bastion of Truth"                   }, // Bastion of Truth (id=209216, n_ranks=6)
  { 1128,  49,   0, 11,   3, 209224, "Resolve of Truth"                   }, // Resolve of Truth (id=209224, n_ranks=6)
  { 1129,  49,   0, 12,   3, 209225, "Faith's Armor"                      }, // Faith's Armor (id=209225, n_ranks=6)
  { 1130,  49,   0,  0,   1, 209341, "Painful Truths"                     }, // Painful Truths (id=209341, n_ranks=1)
  { 1131,  49,   0,  0,   1, 209285, "Sacrifice of the Just"              }, // Sacrifice of the Just (id=209285, n_ranks=1)
  { 1132,  49,   0,  0,   1, 209376, "Forbearant Faithful"                }, // Forbearant Faithful (id=209376, n_ranks=1)
  { 1133,  49,   1,  1,   1, 209389, "Bulwark of Order"                   }, // Bulwark of Order (id=209389, n_ranks=1)
  { 1134,  49,   1,  2,   1, 209539, "Light of the Titans"                }, // Light of the Titans (id=209539, n_ranks=1)
  { 1135,  49,   1,  3,   1, 209474, "Tyr's Enforcer"                     }, // Tyr's Enforcer (id=209474, n_ranks=1)
  { 1206,  49,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1343,  49,   0,  0,   1, 221841, "Truthguard's Light"                 }, // Truthguard's Light (id=221841, n_ranks=1)
  { 1378,  49,   5,  0,  20, 214924, "Unrelenting Light"                  }, // Unrelenting Light (id=214924, n_ranks=20)
  { 1455,  49,   8,  0,   0, 226829, "Artificial Damage"                  }, // Artificial Damage (id=226829, n_ranks=1)
  {  800,  50,   0,  4,   3, 195243, "Inner Peace"                        }, // Inner Peace (id=195243, n_ranks=6)
  {  801,  50,   0, 11,   3, 195244, "Light on Your Feet"                 }, // Light on Your Feet (id=195244, n_ranks=6)
  {  820,  50,   0,  5,   3, 195263, "Rising Winds"                       }, // Rising Winds (id=195263, n_ranks=6)
  {  821,  50,   0,  6,   3, 218607, "Tiger Claws"                        }, // Tiger Claws (id=218607, n_ranks=6)
  {  822,  50,   0,  7,   3, 195266, "Death Art"                          }, // Death Art (id=195266, n_ranks=6)
  {  824,  50,   0,  8,   3, 195269, "Power of a Thousand Cranes"         }, // Power of a Thousand Cranes (id=195269, n_ranks=6)
  {  825,  50,   0,  9,   3, 195291, "Fists of the Wind"                  }, // Fists of the Wind (id=195291, n_ranks=6)
  {  826,  50,   0,  0,   1, 195295, "Good Karma"                         }, // Good Karma (id=195295, n_ranks=1)
  {  827,  50,   0,  0,   1, 195298, "Spiritual Focus"                    }, // Spiritual Focus (id=195298, n_ranks=1)
  {  828,  50,   0,  0,   1, 195300, "Transfer the Power"                 }, // Transfer the Power (id=195300, n_ranks=1)
  {  829,  50,   0, 12,   3, 195380, "Healing Winds"                      }, // Healing Winds (id=195380, n_ranks=6)
  {  830,  50,   1,  1,   1, 195399, "Gale Burst"                         }, // Gale Burst (id=195399, n_ranks=1)
  {  831,  50,  18,  0,   0, 205320, "Strike of the Windlord"             }, // Strike of the Windlord (id=205320, n_ranks=1)
  {  832,  50,   1,  2,   1, 195650, "Crosswinds"                         }, // Crosswinds (id=195650, n_ranks=1)
  {  833,  50,   1,  3,   1, 196082, "Tornado Kicks"                      }, // Tornado Kicks (id=196082, n_ranks=1)
  { 1094,  50,   0, 10,   3, 195267, "Strength of Xuen"                   }, // Strength of Xuen (id=195267, n_ranks=6)
  { 1204,  50,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1341,  50,   0,  0,   1, 195265, "Dark Skies"                         }, // Dark Skies (id=195265, n_ranks=1)
  { 1376,  50,   5,  0,  20, 214922, "Windborne Blows"                    }, // Windborne Blows (id=214922, n_ranks=20)
  {  931,  51,  18,  0,   0, 205406, "Sheilun's Gift"                     }, // Sheilun's Gift (id=205406, n_ranks=1)
  {  932,  51,   1,  1,   1, 199887, "The Mists of Sheilun"               }, // The Mists of Sheilun (id=199887, n_ranks=1)
  {  933,  51,   1,  2,   1, 199665, "Blessings of Yu'lon"                }, // Blessings of Yu'lon (id=199665, n_ranks=1)
  {  934,  51,   1,  3,   1, 199640, "Celestial Breath"                   }, // Celestial Breath (id=199640, n_ranks=1)
  {  935,  51,   0,  0,   1, 199401, "Light on Your Feet"                 }, // Light on Your Feet (id=199401, n_ranks=1)
  {  936,  51,   0,  0,   1, 199563, "Mists of Life"                      }, // Mists of Life (id=199563, n_ranks=1)
  {  937,  51,   0,  0,   1, 199573, "Dancing Mists"                      }, // Dancing Mists (id=199573, n_ranks=1)
  {  938,  51,   0,  4,   3, 199364, "Coalescing Mists"                   }, // Coalescing Mists (id=199364, n_ranks=6)
  {  939,  51,   0, 11,   3, 199365, "Shroud of Mist"                     }, // Shroud of Mist (id=199365, n_ranks=6)
  {  940,  51,   0,  5,   3, 199366, "Way of the Mistweaver"              }, // Way of the Mistweaver (id=199366, n_ranks=6)
  {  941,  51,   0,  6,   3, 199367, "Protection of Shaohao"              }, // Protection of Shaohao (id=199367, n_ranks=6)
  {  942,  51,   0,  7,   3, 199372, "Extended Healing"                   }, // Extended Healing (id=199372, n_ranks=6)
  {  943,  51,   0,  8,   3, 199377, "Soothing Remedies"                  }, // Soothing Remedies (id=199377, n_ranks=6)
  {  944,  51,   0,  9,   3, 199380, "Infusion of Life"                   }, // Infusion of Life (id=199380, n_ranks=6)
  {  945,  51,   0, 12,   3, 199384, "Spirit Tether"                      }, // Spirit Tether (id=199384, n_ranks=6)
  {  946,  51,   0, 10,   3, 199485, "Essence of the Mists"               }, // Essence of the Mists (id=199485, n_ranks=6)
  { 1203,  51,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1255,  51,   8,  0,   0, 213428, "Artificial Damage"                  }, // Artificial Damage (id=213428, n_ranks=1)
  { 1295,  51,   0,  0,   1, 214516, "Mists of Wisdom"                    }, // Mists of Wisdom (id=214516, n_ranks=1)
  { 1375,  51,   5,  0,  20, 214921, "Mistweaving"                        }, // Mistweaving (id=214921, n_ranks=20)
  { 1202,  52,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1277,  52,  18,  0,   0, 214326, "Exploding Keg"                      }, // Exploding Keg (id=214326, n_ranks=1)
  { 1278,  52,   0,  4,   3, 213051, "Obsidian Fists"                     }, // Obsidian Fists (id=213051, n_ranks=6)
  { 1279,  52,   0,  5,   3, 213049, "Hot Blooded"                        }, // Hot Blooded (id=213049, n_ranks=6)
  { 1280,  52,   0,  6,   3, 213180, "Overflow"                           }, // Overflow (id=213180, n_ranks=6)
  { 1281,  52,   0,  7,   3, 213062, "Dark Side of the Moon"              }, // Dark Side of the Moon (id=213062, n_ranks=6)
  { 1282,  52,   0,  8,   3, 213055, "Staggering Around"                  }, // Staggering Around (id=213055, n_ranks=6)
  { 1283,  52,   0,  9,   3, 213133, "Healthy Appetite"                   }, // Healthy Appetite (id=213133, n_ranks=6)
  { 1284,  52,   0, 10,   3, 213136, "Gifted Student"                     }, // Gifted Student (id=213136, n_ranks=6)
  { 1285,  52,   0, 11,   3, 213047, "Potent Kick"                        }, // Potent Kick (id=213047, n_ranks=6)
  { 1286,  52,   0, 12,   3, 213116, "Face Palm"                          }, // Face Palm (id=213116, n_ranks=6)
  { 1287,  52,   0,  0,   1, 213161, "Swift as a Coursing River"          }, // Swift as a Coursing River (id=213161, n_ranks=1)
  { 1288,  52,   0,  0,   1, 216424, "Obstinate Determination"            }, // Obstinate Determination (id=216424, n_ranks=1)
  { 1289,  52,   0,  0,   1, 213050, "Smashed"                            }, // Smashed (id=213050, n_ranks=1)
  { 1290,  52,   1,  1,   1, 213183, "Dragonfire Brew"                    }, // Dragonfire Brew (id=213183, n_ranks=1)
  { 1291,  52,   1,  2,   1, 214372, "Brew-Stache"                        }, // Brew-Stache (id=214372, n_ranks=1)
  { 1292,  52,   1,  3,   1, 213340, "Fortification"                      }, // Fortification (id=213340, n_ranks=1)
  { 1293,  52,   0,  0,   1, 214428, "Full Keg"                           }, // Full Keg (id=214428, n_ranks=1)
  { 1374,  52,   5,  0,  20, 214920, "Wanderer's Hardiness"               }, // Wanderer's Hardiness (id=214920, n_ranks=20)
  { 1454,  52,   8,  0,   0, 226829, "Artificial Damage"                  }, // Artificial Damage (id=226829, n_ranks=1)
  {  783,  53,  18,  0,   0, 214634, "Ebonbolt"                           }, // Ebonbolt (id=214634, n_ranks=1)
  {  784,  53,   0,  4,   3, 195315, "Icy Caress"                         }, // Icy Caress (id=195315, n_ranks=6)
  {  785,  53,   0,  5,   3, 195317, "Ice Age"                            }, // Ice Age (id=195317, n_ranks=6)
  {  786,  53,   0,  6,   3, 195322, "Let It Go"                          }, // Let It Go (id=195322, n_ranks=6)
  {  787,  53,   0,  7,   3, 195323, "Orbital Strike"                     }, // Orbital Strike (id=195323, n_ranks=6)
  {  788,  53,   0,  8,   3, 195345, "Frozen Veins"                       }, // Frozen Veins (id=195345, n_ranks=6)
  {  789,  53,   0,  9,   3, 195351, "Clarity of Thought"                 }, // Clarity of Thought (id=195351, n_ranks=6)
  {  790,  53,   0, 10,   3, 195352, "The Storm Rages"                    }, // The Storm Rages (id=195352, n_ranks=6)
  {  791,  53,   0, 11,   3, 195354, "Shield of Alodi"                    }, // Shield of Alodi (id=195354, n_ranks=6)
  {  792,  53,   0, 12,   3, 214626, "Jouster"                            }, // Jouster (id=214626, n_ranks=6)
  {  793,  53,   0,  0,   1, 195419, "Chain Reaction"                     }, // Chain Reaction (id=195419, n_ranks=1)
  {  794,  53,   0,  0,   1, 214664, "Ice Nine"                           }, // Ice Nine (id=214664, n_ranks=1)
  {  795,  53,   0,  0,   1, 220817, "Icy Hand"                           }, // Icy Hand (id=220817, n_ranks=1)
  {  796,  53,   1,  1,   1, 214776, "It's Cold Outside"                  }, // It's Cold Outside (id=214776, n_ranks=1)
  {  797,  53,   1,  2,   1, 195615, "Black Ice"                          }, // Black Ice (id=195615, n_ranks=1)
  {  798,  53,   1,  3,   1, 195448, "Chilled to the Core"                }, // Chilled to the Core (id=195448, n_ranks=1)
  { 1200,  53,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1296,  53,   0,  0,   1, 214629, "Shattering Bolts"                   }, // Shattering Bolts (id=214629, n_ranks=1)
  { 1373,  53,   5,  0,  20, 214919, "Spellborne"                         }, // Spellborne (id=214919, n_ranks=20)
  {  748,  54,  18,  0,   0, 194466, "Phoenix's Flames"                   }, // Phoenix's Flames (id=194466, n_ranks=1)
  {  749,  54,   0,  4,   3, 194224, "Fire at Will"                       }, // Fire at Will (id=194224, n_ranks=6)
  {  750,  54,   0,  5,   3, 194234, "Reignition Overdrive"               }, // Reignition Overdrive (id=194234, n_ranks=6)
  {  751,  54,   0, 10,   3, 194239, "Pyroclasmic Paranoia"               }, // Pyroclasmic Paranoia (id=194239, n_ranks=6)
  {  752,  54,   0,  6,   3, 194312, "Burning Gaze"                       }, // Burning Gaze (id=194312, n_ranks=6)
  {  753,  54,   0,  7,   3, 210182, "Blue Flame Special"                 }, // Blue Flame Special (id=210182, n_ranks=6)
  {  754,  54,   0,  8,   3, 194313, "Great Balls of Fire"                }, // Great Balls of Fire (id=194313, n_ranks=6)
  {  755,  54,   0,  9,   3, 194314, "Everburning Consumption"            }, // Everburning Consumption (id=194314, n_ranks=6)
  {  756,  54,   0, 11,   3, 194318, "Cauterizing Blink"                  }, // Cauterizing Blink (id=194318, n_ranks=6)
  {  757,  54,   0, 12,   3, 194315, "Molten Skin"                        }, // Molten Skin (id=194315, n_ranks=6)
  {  758,  54,   0,  0,   1, 194487, "Blast Furnace"                      }, // Blast Furnace (id=194487, n_ranks=1)
  {  759,  54,   0,  0,   1, 215796, "Big Mouth"                          }, // Big Mouth (id=215796, n_ranks=1)
  {  760,  54,   0,  0,   1, 227481, "Scorched Earth"                     }, // Scorched Earth (id=227481, n_ranks=1)
  {  761,  54,   1,  1,   1, 194431, "Aftershocks"                        }, // Aftershocks (id=194431, n_ranks=1)
  {  762,  54,   1,  2,   1, 215773, "Phoenix Reborn"                     }, // Phoenix Reborn (id=215773, n_ranks=1)
  {  763,  54,   1,  3,   1, 194331, "Pyretic Incantation"                }, // Pyretic Incantation (id=194331, n_ranks=1)
  { 1201,  54,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1340,  54,   0,  0,   1, 221844, "Wings of Flame"                     }, // Wings of Flame (id=221844, n_ranks=1)
  { 1372,  54,   5,  0,  20, 214918, "Empowered Spellblade"               }, // Empowered Spellblade (id=214918, n_ranks=20)
  {  307,  55,  18,  0,   0, 204147, "Windburst"                          }, // Windburst (id=204147, n_ranks=1)
  {  308,  55,   0,  0,   1, 204219, "Mark of the Windrunner"             }, // Mark of the Windrunner (id=204219, n_ranks=1)
  {  309,  55,   0,  0,   1, 191328, "Critical Focus"                     }, // Critical Focus (id=191328, n_ranks=1)
  {  310,  55,   0,  0,   1, 191339, "Rapid Killing"                      }, // Rapid Killing (id=191339, n_ranks=1)
  {  311,  55,   1,  1,   1, 191048, "Call of the Hunter"                 }, // Call of the Hunter (id=191048, n_ranks=1)
  {  312,  55,   0,  4,   3, 190449, "Deadly Aim"                         }, // Deadly Aim (id=190449, n_ranks=6)
  {  313,  55,   0,  5,   3, 190457, "Windrunner's Guidance"              }, // Windrunner's Guidance (id=190457, n_ranks=6)
  {  314,  55,   0,  6,   3, 190462, "Quick Shot"                         }, // Quick Shot (id=190462, n_ranks=6)
  {  315,  55,   0,  7,   3, 190467, "Called Shot"                        }, // Called Shot (id=190467, n_ranks=6)
  {  316,  55,   0, 11,   3, 190503, "Healing Shell"                      }, // Healing Shell (id=190503, n_ranks=6)
  {  317,  55,   0, 12,   3, 190514, "Survival of the Fittest"            }, // Survival of the Fittest (id=190514, n_ranks=6)
  {  318,  55,   0,  8,   3, 190520, "Precision"                          }, // Precision (id=190520, n_ranks=6)
  {  319,  55,   0,  9,   3, 190529, "Marked for Death"                   }, // Marked for Death (id=190529, n_ranks=6)
  {  320,  55,   0, 10,   3, 190567, "Gust of Wind"                       }, // Gust of Wind (id=190567, n_ranks=6)
  {  321,  55,   1,  2,   1, 190852, "Legacy of the Windrunners"          }, // Legacy of the Windrunners (id=190852, n_ranks=1)
  {  322,  55,   1,  3,   1, 204089, "Bullseye"                           }, // Bullseye (id=204089, n_ranks=1)
  { 1197,  55,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1337,  55,   0,  0,   1, 214826, "Wind Arrows"                        }, // Wind Arrows (id=214826, n_ranks=1)
  { 1369,  55,   5,  0,  20, 214915, "Windflight Arrows"                  }, // Windflight Arrows (id=214915, n_ranks=20)
  {  868,  56,   0,  4,   3, 197038, "Wilderness Expert"                  }, // Wilderness Expert (id=197038, n_ranks=6)
  {  869,  56,   0,  5,   3, 197047, "Furious Swipes"                     }, // Furious Swipes (id=197047, n_ranks=6)
  {  870,  56,   0,  6,   3, 197080, "Pack Leader"                        }, // Pack Leader (id=197080, n_ranks=6)
  {  871,  56,   0, 12,   3, 197138, "Natural Reflexes"                   }, // Natural Reflexes (id=197138, n_ranks=6)
  {  872,  56,   0,  7,   3, 197139, "Focus of the Titans"                }, // Focus of the Titans (id=197139, n_ranks=6)
  {  873,  56,   0,  8,   3, 197140, "Spitting Cobras"                    }, // Spitting Cobras (id=197140, n_ranks=6)
  {  874,  56,   0, 11,   3, 197160, "Mimiron's Shell"                    }, // Mimiron's Shell (id=197160, n_ranks=6)
  {  875,  56,   0,  9,   3, 197162, "Jaws of Thunder"                    }, // Jaws of Thunder (id=197162, n_ranks=6)
  {  876,  56,   0,  0,   1, 197178, "Hunter's Advantage"                 }, // Hunter's Advantage (id=197178, n_ranks=1)
  {  877,  56,   0,  0,   1, 197199, "Spirit Bond"                        }, // Spirit Bond (id=197199, n_ranks=1)
  {  878,  56,   1,  1,   1, 197248, "Master of Beasts"                   }, // Master of Beasts (id=197248, n_ranks=1)
  {  879,  56,   1,  2,   1, 207068, "Titan's Thunder"                    }, // Titan's Thunder (id=207068, n_ranks=1)
  {  880,  56,   0,  0,   1, 197343, "Pathfinder"                         }, // Pathfinder (id=197343, n_ranks=1)
  {  881,  56,  18,  0,   0, 197344, "Hati's Bond"                        }, // Hati's Bond (id=197344, n_ranks=1)
  {  882,  56,   1,  3,   1, 197354, "Surge of the Stormgod"              }, // Surge of the Stormgod (id=197354, n_ranks=1)
  { 1095,  56,   0, 10,   3, 206910, "Unleash the Beast"                  }, // Unleash the Beast (id=206910, n_ranks=6)
  { 1196,  56,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1336,  56,   0,  0,   1, 215779, "Beast Master"                       }, // Beast Master (id=215779, n_ranks=1)
  { 1368,  56,   5,  0,  20, 214914, "Spiritbound"                        }, // Spiritbound (id=214914, n_ranks=20)
  {  948,  57,   0,  4,   3, 200395, "Reinforced Fur"                     }, // Reinforced Fur (id=200395, n_ranks=6)
  {  949,  57,   0,  5,   3, 200399, "Ursoc's Endurance"                  }, // Ursoc's Endurance (id=200399, n_ranks=6)
  {  950,  57,   0, 11,   3, 200400, "Wildflesh"                          }, // Wildflesh (id=200400, n_ranks=6)
  {  951,  57,   0,  7,   3, 200402, "Perpetual Spring"                   }, // Perpetual Spring (id=200402, n_ranks=6)
  {  952,  57,   0,  8,   3, 200409, "Jagged Claws"                       }, // Jagged Claws (id=200409, n_ranks=6)
  {  953,  57,   0,  9,   3, 200414, "Bestial Fortitude"                  }, // Bestial Fortitude (id=200414, n_ranks=6)
  {  954,  57,   0, 10,   3, 200415, "Sharpened Instincts"                }, // Sharpened Instincts (id=200415, n_ranks=6)
  {  955,  57,   0,  6,   3, 208762, "Mauler"                             }, // Mauler (id=208762, n_ranks=6)
  {  956,  57,   0, 12,   3, 200440, "Vicious Bites"                      }, // Vicious Bites (id=200440, n_ranks=6)
  {  957,  57,   0,  0,   1, 215799, "Bear Hug"                           }, // Bear Hug (id=215799, n_ranks=1)
  {  958,  57,   0,  0,   1, 200515, "Bloody Paws"                        }, // Bloody Paws (id=200515, n_ranks=1)
  {  959,  57,   0,  0,   1, 214996, "Roar of the Crowd"                  }, // Roar of the Crowd (id=214996, n_ranks=1)
  {  960,  57,  18,  0,   0, 200851, "Rage of the Sleeper"                }, // Rage of the Sleeper (id=200851, n_ranks=1)
  {  961,  57,   1,  1,   1, 200855, "Embrace of the Nightmare"           }, // Embrace of the Nightmare (id=200855, n_ranks=1)
  {  962,  57,   1,  2,   1, 200854, "Gory Fur"                           }, // Gory Fur (id=200854, n_ranks=1)
  {  979,  57,   1,  3,   1, 200850, "Adaptive Fur"                       }, // Adaptive Fur (id=200850, n_ranks=1)
  { 1194,  57,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1334,  57,   0,  0,   1, 215061, "Iron Claws"                         }, // Iron Claws (id=215061, n_ranks=1)
  { 1366,  57,   5,  0,  20, 214912, "Ursoc's Bond"                       }, // Ursoc's Bond (id=214912, n_ranks=20)
  { 1453,  57,   8,  0,   0, 226829, "Artificial Damage"                  }, // Artificial Damage (id=226829, n_ranks=1)
  { 1153,  58,  18,  0,   0, 210722, "Ashamane's Frenzy"                  }, // Ashamane's Frenzy (id=210722, n_ranks=1)
  { 1154,  58,   1,  1,   1, 210702, "Ashamane's Bite"                    }, // Ashamane's Bite (id=210702, n_ranks=1)
  { 1155,  58,   1,  2,   1, 210676, "Shadow Thrash"                      }, // Shadow Thrash (id=210676, n_ranks=1)
  { 1156,  58,   1,  3,   1, 210666, "Open Wounds"                        }, // Open Wounds (id=210666, n_ranks=1)
  { 1157,  58,   0,  0,   1, 210663, "Scent of Blood"                     }, // Scent of Blood (id=210663, n_ranks=1)
  { 1158,  58,   0,  0,   1, 210650, "Protection of Ashamane"             }, // Protection of Ashamane (id=210650, n_ranks=1)
  { 1159,  58,   0,  0,   1, 210638, "Hardened Roots"                     }, // Hardened Roots (id=210638, n_ranks=1)
  { 1160,  58,   0, 11,   3, 210557, "Honed Instincts"                    }, // Honed Instincts (id=210557, n_ranks=6)
  { 1161,  58,   0,  4,   3, 210570, "Razor Fangs"                        }, // Razor Fangs (id=210570, n_ranks=6)
  { 1162,  58,   0,  5,   3, 210571, "Feral Power"                        }, // Feral Power (id=210571, n_ranks=6)
  { 1163,  58,   0,  6,   3, 210575, "Powerful Bite"                      }, // Powerful Bite (id=210575, n_ranks=6)
  { 1164,  58,   0,  7,   3, 210579, "Ashamane's Energy"                  }, // Ashamane's Energy (id=210579, n_ranks=6)
  { 1165,  58,   0, 12,   3, 210590, "Attuned to Nature"                  }, // Attuned to Nature (id=210590, n_ranks=6)
  { 1166,  58,   0,  8,   3, 210593, "Tear the Flesh"                     }, // Tear the Flesh (id=210593, n_ranks=6)
  { 1167,  58,   0, 10,   3, 210631, "Feral Instinct"                     }, // Feral Instinct (id=210631, n_ranks=6)
  { 1168,  58,   0,  9,   3, 210637, "Sharpened Claws"                    }, // Sharpened Claws (id=210637, n_ranks=6)
  { 1193,  58,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1327,  58,   0,  0,   1, 214736, "Shredder Fangs"                     }, // Shredder Fangs (id=214736, n_ranks=1)
  { 1365,  58,   5,  0,  20, 214911, "Fangs of the First"                 }, // Fangs of the First (id=214911, n_ranks=20)
  { 1034,  59,   0, 11,   3, 202302, "Bladed Feathers"                    }, // Bladed Feathers (id=202302, n_ranks=6)
  { 1035,  59,   0, 12,   3, 203018, "Touch of the Moon"                  }, // Touch of the Moon (id=203018, n_ranks=6)
  { 1036,  59,   0,  4,   3, 202384, "Dark Side of the Moon"              }, // Dark Side of the Moon (id=202384, n_ranks=6)
  { 1037,  59,   0,  5,   3, 202386, "Twilight Glow"                      }, // Twilight Glow (id=202386, n_ranks=6)
  { 1038,  59,   0,  6,   3, 202426, "Solar Stabbing"                     }, // Solar Stabbing (id=202426, n_ranks=6)
  { 1039,  59,   0,  7,   3, 202433, "Scythe of the Stars"                }, // Scythe of the Stars (id=202433, n_ranks=6)
  { 1040,  59,   0,  8,   3, 202445, "Falling Star"                       }, // Falling Star (id=202445, n_ranks=6)
  { 1041,  59,   0,  9,   3, 202466, "Sunfire Burns"                      }, // Sunfire Burns (id=202466, n_ranks=6)
  { 1042,  59,   0, 10,   3, 202464, "Empowerment"                        }, // Empowerment (id=202464, n_ranks=6)
  { 1043,  59,   0,  0,   1, 202890, "Rapid Innervation"                  }, // Rapid Innervation (id=202890, n_ranks=1)
  { 1044,  59,   0,  0,   1, 202918, "Light of the Sun"                   }, // Light of the Sun (id=202918, n_ranks=1)
  { 1045,  59,   0,  0,   1, 213682, "Sunblind"                           }, // Sunblind (id=213682, n_ranks=1)
  { 1046,  59,   1,  1,   1, 202940, "Moon and Stars"                     }, // Moon and Stars (id=202940, n_ranks=1)
  { 1047,  59,   1,  2,   1, 202996, "Power of Goldrinn"                  }, // Power of Goldrinn (id=202996, n_ranks=1)
  { 1048,  59,   1,  3,   1, 214508, "Echoing Stars"                      }, // Echoing Stars (id=214508, n_ranks=1)
  { 1049,  59,  18,  0,   0, 202767, "New Moon"                           }, // New Moon (id=202767, n_ranks=1)
  { 1192,  59,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1294,  59,   0,  0,   1, 214514, "Skywrath"                           }, // Skywrath (id=214514, n_ranks=1)
  { 1364,  59,   5,  0,  20, 214910, "Goldrinn's Fury"                    }, // Goldrinn's Fury (id=214910, n_ranks=20)
  { 1096,  60,  18,  0,   0, 207407, "Soul Carver"                        }, // Soul Carver (id=207407, n_ranks=1)
  { 1097,  60,   0,  0,   1, 212894, "Demonic Flames"                     }, // Demonic Flames (id=212894, n_ranks=1)
  { 1098,  60,   1,  1,   1, 207387, "Painbringer"                        }, // Painbringer (id=207387, n_ranks=1)
  { 1099,  60,   0,  4,   3, 207343, "Aldrachi Design"                    }, // Aldrachi Design (id=207343, n_ranks=6)
  { 1100,  60,   0,  5,   3, 207347, "Aura of Pain"                       }, // Aura of Pain (id=207347, n_ranks=6)
  { 1101,  60,   0,  6,   3, 207352, "Honed Warblades"                    }, // Honed Warblades (id=207352, n_ranks=6)
  { 1191,  60,   8,  0,   0, 211309, "Artificial Stamina"                 }, // Artificial Stamina (id=211309, n_ranks=1)
  { 1228,  60,   0,  0,   1, 212829, "Defensive Spikes"                   }, // Defensive Spikes (id=212829, n_ranks=1)
  { 1229,  60,   0,  7,   3, 207375, "Infernal Force"                     }, // Infernal Force (id=207375, n_ranks=6)
  { 1230,  60,   0,  8,   3, 212816, "Embrace the Pain"                   }, // Embrace the Pain (id=212816, n_ranks=6)
  { 1231,  60,   0,  9,   3, 212817, "Fiery Demise"                       }, // Fiery Demise (id=212817, n_ranks=6)
  { 1232,  60,   0, 10,   3, 212819, "Will of the Illidari"               }, // Will of the Illidari (id=212819, n_ranks=6)
  { 1233,  60,   0, 11,   3, 212821, "Devour Souls"                       }, // Devour Souls (id=212821, n_ranks=6)
  { 1234,  60,   0, 12,   3, 212827, "Shatter the Souls"                  }, // Shatter the Souls (id=212827, n_ranks=6)
  { 1235,  60,   1,  3,   1, 213010, "Charred Warblades"                  }, // Charred Warblades (id=213010, n_ranks=1)
  { 1236,  60,   1,  2,   1, 213017, "Fueled by Pain"                     }, // Fueled by Pain (id=213017, n_ranks=1)
  { 1328,  60,   0,  0,   1, 214744, "Tormented Souls"                    }, // Tormented Souls (id=214744, n_ranks=1)
  { 1363,  60,   5,  0,  20, 214909, "Soulgorger"                         }, // Soulgorger (id=214909, n_ranks=20)
  { 1434,  60,   0,  0,   1, 218910, "Siphon Power"                       }, // Siphon Power (id=218910, n_ranks=1)
  { 1452,  60,   8,  0,   0, 226829, "Artificial Damage"                  }, // Artificial Damage (id=226829, n_ranks=1)
  { 1089,  74,   0,  0,   0, 226829, 0                                    },
  { 1237,  80,   1,  3,   1, 193108, "Blades of Light"                    }, // Blades of Light (id=193108, n_ranks=1)
  { 1238,  80,   0,  0,   1, 193058, "Healing Storm"                      }, // Healing Storm (id=193058, n_ranks=1)
  { 1239,  80,   0,  0,   1, 182234, "Unbreakable Will"                   }, // Unbreakable Will (id=182234, n_ranks=1)
  { 1240,  80,   0,  0,   1, 185086, "Endless Resolve"                    }, // Endless Resolve (id=185086, n_ranks=1)
  { 1241,  80,   1,  2,   1, 186788, "Echo of the Highlord"               }, // Echo of the Highlord (id=186788, n_ranks=1)
  { 1242,  80,   0,  7,   3, 186945, "Wrath of the Ashbringer"            }, // Wrath of the Ashbringer (id=186945, n_ranks=6)
  { 1243,  80,   0, 10,   3, 186944, "Protector of the Ashen Blade"       }, // Protector of the Ashen Blade (id=186944, n_ranks=6)
  { 1244,  80,   0,  9,   3, 185368, "Might of the Templar"               }, // Might of the Templar (id=185368, n_ranks=6)
  { 1245,  80,   0,  8,   3, 186927, "Deliver the Justice"                }, // Deliver the Justice (id=186927, n_ranks=6)
  { 1246,  80,   1,  1,   1, 186773, "Divine Tempest"                     }, // Divine Tempest (id=186773, n_ranks=1)
  { 1247,  80,   0, 12,   3, 184778, "Deflection"                         }, // Deflection (id=184778, n_ranks=6)
  { 1248,  80,   0, 11,   3, 186934, "Embrace the Light"                  }, // Embrace the Light (id=186934, n_ranks=6)
  { 1249,  80,   0,  6,   3, 184843, "Righteous Blade"                    }, // Righteous Blade (id=184843, n_ranks=6)
  { 1250,  80,   0,  5,   3, 184759, "Sharpened Edge"                     }, // Sharpened Edge (id=184759, n_ranks=6)
  { 1251,  80,   0,  4,   3, 186941, "Highlord's Judgment"                }, // Highlord's Judgment (id=186941, n_ranks=6)
  { 1252,  80,   1,  0,   1, 205273, "Wake of Ashes"                      }, // Wake of Ashes (id=205273, n_ranks=1)
  { 1253,  80,   2,  0,   1, 179546, "Ashes to Ashes"                     }, // Ashes to Ashes (id=179546, n_ranks=1)
  {    0,   0,   0,  0,   0, 179546, 0                                    },
};
#define PTR_ARTIFACT_POWER_RANK_SIZE (3063)

// Artifact power rank data, wow build 23038
static struct artifact_power_rank_t __ptr_artifact_power_rank_data[PTR_ARTIFACT_POWER_RANK_SIZE] = {
  { 2316,    40,  0, 205273,      0.000, 0x00000000, 0 },
  { 2317,    41,  0, 186941,      8.000, 0x00000000, 0 },
  { 3199,    41,  1, 186941,     16.000, 0x00000000, 0 },
  { 3200,    41,  2, 186941,     24.000, 0x00000000, 0 },
  { 3201,    41,  3, 186941,     32.000, 0x00000000, 0 },
  { 3202,    41,  4, 186941,     40.000, 0x00000000, 0 },
  { 3203,    41,  5, 186941,     48.000, 0x00000000, 0 },
  { 2320,    42,  0, 184759,      0.000, 0x00000000, 0 },
  { 3204,    42,  1, 184759,     12.000, 0x00000000, 0 },
  { 3205,    42,  2, 184759,     18.000, 0x00000000, 0 },
  { 3206,    42,  3, 184759,     24.000, 0x00000000, 0 },
  { 3207,    42,  4, 184759,     30.000, 0x00000000, 0 },
  { 3208,    42,  5, 184759,     36.000, 0x00000000, 0 },
  { 2323,    43,  0, 184843,      0.000, 0x00000000, 0 },
  { 3209,    43,  1, 184843,     10.000, 0x00000000, 0 },
  { 3210,    43,  2, 184843,     15.000, 0x00000000, 0 },
  { 3211,    43,  3, 184843,     20.000, 0x00000000, 0 },
  { 3212,    43,  4, 184843,     25.000, 0x00000000, 0 },
  { 3213,    43,  5, 184843,     30.000, 0x00000000, 0 },
  { 2326,    44,  0, 186934,      0.000, 0x00000000, 0 },
  { 3214,    44,  1, 186934,     30.000, 0x00000000, 0 },
  { 3215,    44,  2, 186934,     45.000, 0x00000000, 0 },
  { 3216,    44,  3, 186934,     60.000, 0x00000000, 0 },
  { 3217,    44,  4, 186934,     75.000, 0x00000000, 0 },
  { 3218,    44,  5, 186934,     90.000, 0x00000000, 0 },
  { 2329,    47,  0, 184778,      0.000, 0x00000000, 0 },
  { 3219,    47,  1, 184778, -20000.000, 0x00000000, 0 },
  { 3220,    47,  2, 184778, -30000.000, 0x00000000, 0 },
  { 3221,    47,  3, 184778, -40000.000, 0x00000000, 0 },
  { 3222,    47,  4, 184778, -50000.000, 0x00000000, 0 },
  { 3223,    47,  5, 184778, -60000.000, 0x00000000, 0 },
  { 2332,    49,  0, 186773,      0.000, 0x00000000, 0 },
  { 2333,    50,  0, 186927,      0.000, 0x00000000, 0 },
  { 3224,    50,  1, 186927,     16.000, 0x00000000, 0 },
  { 3225,    50,  2, 186927,     24.000, 0x00000000, 0 },
  { 3226,    50,  3, 186927,     32.000, 0x00000000, 0 },
  { 3227,    50,  4, 186927,     40.000, 0x00000000, 0 },
  { 3228,    50,  5, 186927,     48.000, 0x00000000, 0 },
  { 2336,    51,  0, 185368,      0.000, 0x00000000, 0 },
  { 3229,    51,  1, 185368,      4.000, 0x00000000, 0 },
  { 3230,    51,  2, 185368,      6.000, 0x00000000, 0 },
  { 3231,    51,  3, 185368,      8.000, 0x00000000, 0 },
  { 3232,    51,  4, 185368,     10.000, 0x00000000, 0 },
  { 3233,    51,  5, 185368,     12.000, 0x00000000, 0 },
  { 2339,    52,  0, 186944,      0.000, 0x00000000, 0 },
  { 3234,    52,  1, 186944,    -20.000, 0x00000000, 0 },
  { 3235,    52,  2, 186944,    -30.000, 0x00000000, 0 },
  { 3236,    52,  3, 186944,    -40.000, 0x00000000, 0 },
  { 3237,    52,  4, 186944,    -50.000, 0x00000000, 0 },
  { 3238,    52,  5, 186944,    -60.000, 0x00000000, 0 },
  { 2342,    53,  0, 186945,      0.000, 0x00000000, 0 },
  { 3239,    53,  1, 186945,   5000.000, 0x00000000, 0 },
  { 3240,    53,  2, 186945,   7500.000, 0x00000000, 0 },
  { 3241,    53,  3, 186945,  10000.000, 0x00000000, 0 },
  { 3242,    53,  4, 186945,  12500.000, 0x00000000, 0 },
  { 3243,    53,  5, 186945,  15000.000, 0x00000000, 0 },
  { 2345,    54,  0, 186788,      0.000, 0x00000000, 0 },
  { 2380,    72,  0, 187304,      3.000, 0x00000000, 0 },
  { 3514,    72,  1, 187304,      6.000, 0x00000000, 0 },
  { 3515,    72,  2, 187304,     10.000, 0x00000000, 0 },
  { 3516,    72,  3, 187304,     13.000, 0x00000000, 0 },
  { 3517,    72,  4, 187304,     16.000, 0x00000000, 0 },
  { 3518,    72,  5, 187304,     20.000, 0x00000000, 0 },
  { 2383,    73,  0, 188006,      0.000, 0x00000000, 0 },
  { 2384,    74,  0, 187258,      0.000, 0x00000000, 0 },
  { 3519,    74,  1, 187258,      4.000, 0x00000000, 0 },
  { 3520,    74,  2, 187258,      6.000, 0x00000000, 0 },
  { 3521,    74,  3, 187258,      8.000, 0x00000000, 0 },
  { 3522,    74,  4, 187258,     10.000, 0x00000000, 0 },
  { 3523,    74,  5, 187258,     12.000, 0x00000000, 0 },
  { 2387,    75,  0, 187321,      1.000, 0x00000000, 0 },
  { 3524,    75,  1, 187321,      2.000, 0x00000000, 0 },
  { 3525,    75,  2, 187321,      3.000, 0x00000000, 0 },
  { 3526,    75,  3, 187321,      4.000, 0x00000000, 0 },
  { 3527,    75,  4, 187321,      5.000, 0x00000000, 0 },
  { 3528,    75,  5, 187321,      6.000, 0x00000000, 0 },
  { 2393,    77,  0, 187313,      0.000, 0x00000000, 0 },
  { 3534,    77,  1, 187313,      6.000, 0x00000000, 0 },
  { 3535,    77,  2, 187313,     10.000, 0x00000000, 0 },
  { 3536,    77,  3, 187313,     13.000, 0x00000000, 0 },
  { 3537,    77,  4, 187313,     16.000, 0x00000000, 0 },
  { 3538,    77,  5, 187313,     20.000, 0x00000000, 0 },
  { 2396,    78,  0, 187310,      0.000, 0x00000000, 0 },
  { 2399,    79,  0, 187287,      2.000, 0x00000000, 0 },
  { 3544,    79,  1, 187287,      4.000, 0x00000000, 0 },
  { 3545,    79,  2, 187287,      6.000, 0x00000000, 0 },
  { 3546,    79,  3, 187287,      8.000, 0x00000000, 0 },
  { 3547,    79,  4, 187287,     10.000, 0x00000000, 0 },
  { 3548,    79,  5, 187287,     12.000, 0x00000000, 0 },
  { 2402,    80,  0, 215463,      0.000, 0x00000000, 0 },
  { 2405,    81,  0, 187276,      1.000, 0x00000000, 0 },
  { 3554,    81,  1, 187276,      2.000, 0x00000000, 0 },
  { 3555,    81,  2, 187276,      3.000, 0x00000000, 0 },
  { 3556,    81,  3, 187276,      4.000, 0x00000000, 0 },
  { 3557,    81,  4, 187276,      5.000, 0x00000000, 0 },
  { 3558,    81,  5, 187276,      6.000, 0x00000000, 0 },
  { 2408,    82,  0, 187264,   1000.000, 0x00000000, 0 },
  { 3559,    82,  1, 187264,   2000.000, 0x00000000, 0 },
  { 3560,    82,  2, 187264,   3000.000, 0x00000000, 0 },
  { 3561,    82,  3, 187264,   4000.000, 0x00000000, 0 },
  { 3562,    82,  4, 187264,   5000.000, 0x00000000, 0 },
  { 3563,    82,  5, 187264,   6000.000, 0x00000000, 0 },
  { 2411,    83,  0, 187301,      1.000, 0x00000000, 0 },
  { 3564,    83,  1, 187301,      2.000, 0x00000000, 0 },
  { 3565,    83,  2, 187301,      3.000, 0x00000000, 0 },
  { 3566,    83,  3, 187301,      4.000, 0x00000000, 0 },
  { 3567,    83,  4, 187301,      5.000, 0x00000000, 0 },
  { 3568,    83,  5, 187301,      6.000, 0x00000000, 0 },
  { 2414,    84,  0, 210716,     20.000, 0x00000000, 0 },
  { 5142,    84,  1, 210716,     40.000, 0x00000000, 0 },
  { 5143,    84,  2, 210716,     60.000, 0x00000000, 0 },
  { 5144,    84,  3, 210716,     80.000, 0x00000000, 0 },
  { 5145,    84,  4, 210716,    100.000, 0x00000000, 0 },
  { 5146,    84,  5, 210716,    120.000, 0x00000000, 0 },
  { 2420,    86,  0, 210725,      0.000, 0x00000000, 0 },
  { 2421,    87,  0, 187680,      0.000, 0x00000000, 0 },
  { 2422,    91,  0, 203524,      0.000, 0x00000000, 0 },
  { 2423,    92,  0, 189059,      0.000, 0x00000000, 0 },
  { 2424,    93,  0, 203576,      0.000, 0x00000000, 0 },
  { 2425,    94,  0, 188778,      0.000, 0x00000000, 0 },
  { 2426,    95,  0, 188635,      2.000, 0x00000000, 0 },
  { 3469,    95,  1, 188635,      4.000, 0x00000000, 0 },
  { 3470,    95,  2, 188635,      6.000, 0x00000000, 0 },
  { 3471,    95,  3, 188635,      8.000, 0x00000000, 0 },
  { 3472,    95,  4, 188635,     10.000, 0x00000000, 0 },
  { 3473,    95,  5, 188635,     12.000, 0x00000000, 0 },
  { 2429,    96,  0, 216272,      6.000, 0x00000000, 0 },
  { 6463,    96,  1, 216272,     12.000, 0x00000000, 0 },
  { 6464,    96,  2, 216272,     18.000, 0x00000000, 0 },
  { 6465,    96,  3, 216272,     24.000, 0x00000000, 0 },
  { 6466,    96,  4, 216272,     30.000, 0x00000000, 0 },
  { 6467,    96,  5, 216272,     36.000, 0x00000000, 0 },
  { 2432,    97,  0, 188651,      0.000, 0x00000000, 0 },
  { 2433,    98,  0, 188683,      0.000, 0x00000000, 0 },
  { 3479,    98,  1, 188683,     20.000, 0x00000000, 0 },
  { 3480,    98,  2, 188683,     30.000, 0x00000000, 0 },
  { 3481,    98,  3, 188683,     40.000, 0x00000000, 0 },
  { 3482,    98,  4, 188683,     50.000, 0x00000000, 0 },
  { 3483,    98,  5, 188683,     60.000, 0x00000000, 0 },
  { 2436,    99,  0, 203230,      0.000, 0x00000000, 0 },
  { 4801,    99,  1, 203230,  -4000.000, 0x00000000, 0 },
  { 4802,    99,  2, 203230,  -6000.000, 0x00000000, 0 },
  { 4803,    99,  3, 203230,  -8000.000, 0x00000000, 0 },
  { 4804,    99,  4, 203230, -10000.000, 0x00000000, 0 },
  { 4805,    99,  5, 203230, -12000.000, 0x00000000, 0 },
  { 2439,   100,  0, 203225,      0.000, 0x00000000, 0 },
  { 4806,   100,  1, 203225,      4.000, 0x00000000, 0 },
  { 4807,   100,  2, 203225,      6.000, 0x00000000, 0 },
  { 4808,   100,  3, 203225,      8.000, 0x00000000, 0 },
  { 4809,   100,  4, 203225,     10.000, 0x00000000, 0 },
  { 4810,   100,  5, 203225,     12.000, 0x00000000, 0 },
  { 2442,   101,  0, 203227,      0.000, 0x00000000, 0 },
  { 4795,   101,  1, 203227,    200.000, 0x00000000, 0 },
  { 4796,   101,  2, 203227,    300.000, 0x00000000, 0 },
  { 4797,   101,  3, 203227,    400.000, 0x00000000, 0 },
  { 4798,   101,  4, 203227,    500.000, 0x00000000, 0 },
  { 4799,   101,  5, 203227,    600.000, 0x00000000, 0 },
  { 2445,   102,  0, 188639,      0.000, 0x00000000, 0 },
  { 3499,   102,  1, 188639,     10.000, 0x00000000, 0 },
  { 3500,   102,  2, 188639,     15.000, 0x00000000, 0 },
  { 3501,   102,  3, 188639,     20.000, 0x00000000, 0 },
  { 3502,   102,  4, 188639,     25.000, 0x00000000, 0 },
  { 3503,   102,  5, 188639,     30.000, 0x00000000, 0 },
  { 2448,   103,  0, 203261,      0.000, 0x00000000, 0 },
  { 2449,   104,  0, 188672,      0.000, 0x00000000, 0 },
  { 2450,   105,  0, 188632,      1.000, 0x00000000, 0 },
  { 3504,   105,  1, 188632,      2.000, 0x00000000, 0 },
  { 3505,   105,  2, 188632,      3.000, 0x00000000, 0 },
  { 3506,   105,  3, 188632,      4.000, 0x00000000, 0 },
  { 3507,   105,  4, 188632,      5.000, 0x00000000, 0 },
  { 3508,   105,  5, 188632,      6.000, 0x00000000, 0 },
  { 2453,   106,  0, 188644,      0.000, 0x00000000, 0 },
  { 3509,   106,  1, 188644,     20.000, 0x00000000, 0 },
  { 3510,   106,  2, 188644,     30.000, 0x00000000, 0 },
  { 3511,   106,  3, 188644,     40.000, 0x00000000, 0 },
  { 3512,   106,  4, 188644,     50.000, 0x00000000, 0 },
  { 3513,   106,  5, 188644,     60.000, 0x00000000, 0 },
  { 2457,   108,  0, 189080,      1.000, 0x00000000, 0 },
  { 3595,   108,  1, 189080,      2.000, 0x00000000, 0 },
  { 3596,   108,  2, 189080,      3.000, 0x00000000, 0 },
  { 3597,   108,  3, 189080,      4.000, 0x00000000, 0 },
  { 3598,   108,  4, 189080,      5.000, 0x00000000, 0 },
  { 3599,   108,  5, 189080,      6.000, 0x00000000, 0 },
  { 2460,   109,  0, 189086,      0.000, 0x00000000, 0 },
  { 3600,   109,  1, 189086,     20.000, 0x00000000, 0 },
  { 3601,   109,  2, 189086,     30.000, 0x00000000, 0 },
  { 3602,   109,  3, 189086,     40.000, 0x00000000, 0 },
  { 3603,   109,  4, 189086,     50.000, 0x00000000, 0 },
  { 3604,   109,  5, 189086,     60.000, 0x00000000, 0 },
  { 2463,   110,  0, 189092,      0.000, 0x00000000, 0 },
  { 3605,   110,  1, 189092,     20.000, 0x00000000, 0 },
  { 3606,   110,  2, 189092,     30.000, 0x00000000, 0 },
  { 3607,   110,  3, 189092,     40.000, 0x00000000, 0 },
  { 3608,   110,  4, 189092,     50.000, 0x00000000, 0 },
  { 3609,   110,  5, 189092,     60.000, 0x00000000, 0 },
  { 2466,   111,  0, 189097,      0.000, 0x00000000, 0 },
  { 3610,   111,  1, 189097,     60.000, 0x00000000, 0 },
  { 3611,   111,  2, 189097,    100.000, 0x00000000, 0 },
  { 3612,   111,  3, 189097,    130.000, 0x00000000, 0 },
  { 3613,   111,  4, 189097,    160.000, 0x00000000, 0 },
  { 3614,   111,  5, 189097,    200.000, 0x00000000, 0 },
  { 2472,   113,  0, 189144,     12.000, 0x00000000, 0 },
  { 3620,   113,  1, 189144,     24.000, 0x00000000, 0 },
  { 3621,   113,  2, 189144,     36.000, 0x00000000, 0 },
  { 3622,   113,  3, 189144,     48.000, 0x00000000, 0 },
  { 3623,   113,  4, 189144,     60.000, 0x00000000, 0 },
  { 3624,   113,  5, 189144,     72.000, 0x00000000, 0 },
  { 2475,   114,  0, 189147,     15.000, 0x00000000, 0 },
  { 3625,   114,  1, 189147,     30.000, 0x00000000, 0 },
  { 3626,   114,  2, 189147,     45.000, 0x00000000, 0 },
  { 3627,   114,  3, 189147,     60.000, 0x00000000, 0 },
  { 3628,   114,  4, 189147,     75.000, 0x00000000, 0 },
  { 3629,   114,  5, 189147,     90.000, 0x00000000, 0 },
  { 2478,   115,  0, 189154,      0.000, 0x00000000, 0 },
  { 3630,   115,  1, 189154,    200.000, 0x00000000, 0 },
  { 3631,   115,  2, 189154,    300.000, 0x00000000, 0 },
  { 3632,   115,  3, 189154,    400.000, 0x00000000, 0 },
  { 3633,   115,  4, 189154,    500.000, 0x00000000, 0 },
  { 3634,   115,  5, 189154,    600.000, 0x00000000, 0 },
  { 2484,   117,  0, 189164,      0.000, 0x00000000, 0 },
  { 3635,   117,  1, 189164,     10.000, 0x00000000, 0 },
  { 3636,   117,  2, 189164,     15.000, 0x00000000, 0 },
  { 3637,   117,  3, 189164,     20.000, 0x00000000, 0 },
  { 3638,   117,  4, 189164,     25.000, 0x00000000, 0 },
  { 3639,   117,  5, 189164,     30.000, 0x00000000, 0 },
  { 2490,   119,  0, 189179,      0.000, 0x00000000, 0 },
  { 2491,   120,  0, 189180,      0.000, 0x00000000, 0 },
  { 2493,   122,  0, 189186,      0.000, 0x00000000, 0 },
  { 2494,   123,  0, 189185,      0.000, 0x00000000, 0 },
  { 2495,   124,  0, 190778,      0.000, 0x00000000, 0 },
  { 2496,   125,  0, 208253,      0.000, 0x00000000, 0 },
  { 2497,   126,  0, 189849,      0.000, 0x00000000, 0 },
  { 2498,   127,  0, 189857,      0.000, 0x00000000, 0 },
  { 2499,   128,  0, 189870,      0.000, 0x00000000, 0 },
  { 2500,   129,  0, 189787,      0.000, 0x00000000, 0 },
  { 2501,   130,  0, 189772,      0.000, 0x00000000, 0 },
  { 3424,   130,  1, 189772,      4.000, 0x00000000, 0 },
  { 3425,   130,  2, 189772,      6.000, 0x00000000, 0 },
  { 3426,   130,  3, 189772,      8.000, 0x00000000, 0 },
  { 3427,   130,  4, 189772,     10.000, 0x00000000, 0 },
  { 3428,   130,  5, 189772,     12.000, 0x00000000, 0 },
  { 2504,   131,  0, 189768,      8.000, 0x00000000, 0 },
  { 3429,   131,  1, 189768,     16.000, 0x00000000, 0 },
  { 3430,   131,  2, 189768,     24.000, 0x00000000, 0 },
  { 3431,   131,  3, 189768,     32.000, 0x00000000, 0 },
  { 3432,   131,  4, 189768,     40.000, 0x00000000, 0 },
  { 3433,   131,  5, 189768,     48.000, 0x00000000, 0 },
  { 2507,   132,  0, 189760,      0.000, 0x00000000, 0 },
  { 3434,   132,  1, 189760,     10.000, 0x00000000, 0 },
  { 3435,   132,  2, 189760,     15.000, 0x00000000, 0 },
  { 3436,   132,  3, 189760,     20.000, 0x00000000, 0 },
  { 3437,   132,  4, 189760,     25.000, 0x00000000, 0 },
  { 3438,   132,  5, 189760,     30.000, 0x00000000, 0 },
  { 2510,   133,  0, 189757,     -2.000, 0x00000000, 0 },
  { 3439,   133,  1, 189757,     -4.000, 0x00000000, 0 },
  { 3440,   133,  2, 189757,     -6.000, 0x00000000, 0 },
  { 3441,   133,  3, 189757,     -8.000, 0x00000000, 0 },
  { 3442,   133,  4, 189757,    -10.000, 0x00000000, 0 },
  { 3443,   133,  5, 189757,    -12.000, 0x00000000, 0 },
  { 2513,   134,  0, 189754,      0.000, 0x00000000, 0 },
  { 3444,   134,  1, 189754,     -4.000, 0x00000000, 0 },
  { 3445,   134,  2, 189754,     -6.000, 0x00000000, 0 },
  { 3446,   134,  3, 189754,     -8.000, 0x00000000, 0 },
  { 3447,   134,  4, 189754,    -10.000, 0x00000000, 0 },
  { 3448,   134,  5, 189754,    -12.000, 0x00000000, 0 },
  { 2516,   135,  0, 189749,     10.000, 0x00000000, 0 },
  { 3449,   135,  1, 189749,     20.000, 0x00000000, 0 },
  { 3450,   135,  2, 189749,     30.000, 0x00000000, 0 },
  { 3451,   135,  3, 189749,     40.000, 0x00000000, 0 },
  { 3452,   135,  4, 189749,     50.000, 0x00000000, 0 },
  { 3453,   135,  5, 189749,     60.000, 0x00000000, 0 },
  { 2519,   136,  0, 189744,      0.000, 0x00000000, 0 },
  { 3454,   136,  1, 189744,      6.000, 0x00000000, 0 },
  { 3455,   136,  2, 189744,     10.000, 0x00000000, 0 },
  { 3456,   136,  3, 189744,     13.000, 0x00000000, 0 },
  { 3457,   136,  4, 189744,     16.000, 0x00000000, 0 },
  { 3458,   136,  5, 189744,     20.000, 0x00000000, 0 },
  { 2522,   137,  0, 186396,      0.000, 0x00000000, 0 },
  { 3459,   137,  1, 186396,   2000.000, 0x00000000, 0 },
  { 3460,   137,  2, 186396,   3000.000, 0x00000000, 0 },
  { 3461,   137,  3, 186396,   4000.000, 0x00000000, 0 },
  { 3462,   137,  4, 186396,   5000.000, 0x00000000, 0 },
  { 3463,   137,  5, 186396,   6000.000, 0x00000000, 0 },
  { 2525,   138,  0, 186393,      0.000, 0x00000000, 0 },
  { 2526,   139,  0, 186372,      0.000, 0x00000000, 0 },
  { 2527,   140,  0, 186320,      1.000, 0x00000000, 0 },
  { 3464,   140,  1, 186320,      2.000, 0x00000000, 0 },
  { 3465,   140,  2, 186320,      3.000, 0x00000000, 0 },
  { 3466,   140,  3, 186320,      4.000, 0x00000000, 0 },
  { 3467,   140,  4, 186320,      5.000, 0x00000000, 0 },
  { 3468,   140,  5, 186320,      6.000, 0x00000000, 0 },
  { 2530,   149,  0, 220143,      0.000, 0x00000000, 0 },
  { 2531,   150,  0, 191760,      0.000, 0x00000000, 0 },
  { 2532,   151,  0, 191741,      0.000, 0x00000000, 0 },
  { 2533,   152,  0, 191747,      0.000, 0x00000000, 0 },
  { 2534,   153,  0, 191637,      0.000, 0x00000000, 0 },
  { 2535,   154,  0, 191731,      0.000, 0x00000000, 0 },
  { 2536,   156,  0, 191419,      7.000, 0x00000000, 0 },
  { 3645,   156,  1, 191419,     14.000, 0x00000000, 0 },
  { 3646,   156,  2, 191419,     21.000, 0x00000000, 0 },
  { 3647,   156,  3, 191419,     28.000, 0x00000000, 0 },
  { 3648,   156,  4, 191419,     35.000, 0x00000000, 0 },
  { 3649,   156,  5, 191419,     42.000, 0x00000000, 0 },
  { 2539,   157,  0, 191442,      7.000, 0x00000000, 0 },
  { 3650,   157,  1, 191442,     14.000, 0x00000000, 0 },
  { 3651,   157,  2, 191442,     21.000, 0x00000000, 0 },
  { 3652,   157,  3, 191442,     28.000, 0x00000000, 0 },
  { 3653,   157,  4, 191442,     35.000, 0x00000000, 0 },
  { 3654,   157,  5, 191442,     42.000, 0x00000000, 0 },
  { 2542,   158,  0, 191485,      7.000, 0x00000000, 0 },
  { 3655,   158,  1, 191485,     14.000, 0x00000000, 0 },
  { 3656,   158,  2, 191485,     21.000, 0x00000000, 0 },
  { 3657,   158,  3, 191485,     28.000, 0x00000000, 0 },
  { 3658,   158,  4, 191485,     35.000, 0x00000000, 0 },
  { 3659,   158,  5, 191485,     42.000, 0x00000000, 0 },
  { 2545,   262,  0, 191565,      0.000, 0x00000000, 0 },
  { 3670,   262,  1, 191565,      6.000, 0x00000000, 0 },
  { 3671,   262,  2, 191565,     10.000, 0x00000000, 0 },
  { 3672,   262,  3, 191565,     13.000, 0x00000000, 0 },
  { 3673,   262,  4, 191565,     16.000, 0x00000000, 0 },
  { 3674,   262,  5, 191565,     20.000, 0x00000000, 0 },
  { 2548,   263,  0, 191721,      0.000, 0x00000000, 0 },
  { 2549,   264,  0, 191592,      0.000, 0x00000000, 0 },
  { 2550,   264,  1, 191593,      0.000, 0x00000000, 0 },
  { 2551,   264,  2, 191594,      0.000, 0x00000000, 0 },
  { 6505,   264,  3, 224466,      0.000, 0x00000000, 0 },
  { 6506,   264,  4, 224467,      0.000, 0x00000000, 0 },
  { 6507,   264,  5, 224468,      0.000, 0x00000000, 0 },
  { 2552,   265,  0, 191494,      3.000, 0x00000000, 0 },
  { 3665,   265,  1, 191494,      6.000, 0x00000000, 0 },
  { 3666,   265,  2, 191494,     10.000, 0x00000000, 0 },
  { 3667,   265,  3, 191494,     13.000, 0x00000000, 0 },
  { 3668,   265,  4, 191494,     16.000, 0x00000000, 0 },
  { 3669,   265,  5, 191494,     20.000, 0x00000000, 0 },
  { 2555,   266,  0, 191488,     10.000, 0x00000000, 0 },
  { 3660,   266,  1, 191488,     20.000, 0x00000000, 0 },
  { 3661,   266,  2, 191488,     30.000, 0x00000000, 0 },
  { 3662,   266,  3, 191488,     40.000, 0x00000000, 0 },
  { 3663,   266,  4, 191488,     50.000, 0x00000000, 0 },
  { 3664,   266,  5, 191488,     60.000, 0x00000000, 0 },
  { 2558,   267,  0, 191584,      0.000, 0x00000000, 0 },
  { 3675,   267,  1, 191584,    -10.000, 0x00000000, 0 },
  { 3676,   267,  2, 191584,    -15.000, 0x00000000, 0 },
  { 3677,   267,  3, 191584,    -20.000, 0x00000000, 0 },
  { 3678,   267,  4, 191584,    -25.000, 0x00000000, 0 },
  { 3679,   267,  5, 191584,    -30.000, 0x00000000, 0 },
  { 2561,   272,  0, 192450,      0.000, 0x00000000, 0 },
  { 3685,   272,  1, 192450,      4.000, 0x00000000, 0 },
  { 3686,   272,  2, 192450,      6.000, 0x00000000, 0 },
  { 3687,   272,  3, 192450,      8.000, 0x00000000, 0 },
  { 3688,   272,  4, 192450,     10.000, 0x00000000, 0 },
  { 3689,   272,  5, 192450,     12.000, 0x00000000, 0 },
  { 2564,   273,  0, 192453,      0.000, 0x00000000, 0 },
  { 3690,   273,  1, 192453,      2.000, 0x00000000, 0 },
  { 3691,   273,  2, 192453,      3.000, 0x00000000, 0 },
  { 3692,   273,  3, 192453,      4.000, 0x00000000, 0 },
  { 3693,   273,  4, 192453,      5.000, 0x00000000, 0 },
  { 3694,   273,  5, 192453,      6.000, 0x00000000, 0 },
  { 2567,   274,  0, 192447,      1.000, 0x00000000, 0 },
  { 3680,   274,  1, 192447,      2.000, 0x00000000, 0 },
  { 3681,   274,  2, 192447,      3.000, 0x00000000, 0 },
  { 3682,   274,  3, 192447,      4.000, 0x00000000, 0 },
  { 3683,   274,  4, 192447,      5.000, 0x00000000, 0 },
  { 3684,   274,  5, 192447,      6.000, 0x00000000, 0 },
  { 2570,   275,  0, 192457,      3.000, 0x00000000, 0 },
  { 3695,   275,  1, 192457,      6.000, 0x00000000, 0 },
  { 3696,   275,  2, 192457,      9.000, 0x00000000, 0 },
  { 3697,   275,  3, 192457,     12.000, 0x00000000, 0 },
  { 3698,   275,  4, 192457,     15.000, 0x00000000, 0 },
  { 3699,   275,  5, 192457,     18.000, 0x00000000, 0 },
  { 2573,   276,  0, 192460,      4.000, 0x00000000, 0 },
  { 3700,   276,  1, 192460,      8.000, 0x00000000, 0 },
  { 3701,   276,  2, 192460,     12.000, 0x00000000, 0 },
  { 3702,   276,  3, 192460,     16.000, 0x00000000, 0 },
  { 3703,   276,  4, 192460,     20.000, 0x00000000, 0 },
  { 3704,   276,  5, 192460,     24.000, 0x00000000, 0 },
  { 2576,   277,  0, 192542,      0.000, 0x00000000, 0 },
  { 2577,   277,  1, 192543,      0.000, 0x00000000, 0 },
  { 2578,   277,  2, 192544,      0.000, 0x00000000, 0 },
  { 4974,   277,  3, 208793,      0.000, 0x00000000, 0 },
  { 4975,   277,  4, 208794,      0.000, 0x00000000, 0 },
  { 4976,   277,  5, 208795,      0.000, 0x00000000, 0 },
  { 2579,   278,  0, 192464,      4.000, 0x00000000, 0 },
  { 3705,   278,  1, 192464,      8.000, 0x00000000, 0 },
  { 3706,   278,  2, 192464,     12.000, 0x00000000, 0 },
  { 3707,   278,  3, 192464,     16.000, 0x00000000, 0 },
  { 3708,   278,  4, 192464,     20.000, 0x00000000, 0 },
  { 3709,   278,  5, 192464,     24.000, 0x00000000, 0 },
  { 2582,   279,  0, 192514,   2000.000, 0x00000000, 0 },
  { 3710,   279,  1, 192514,   4000.000, 0x00000000, 0 },
  { 3711,   279,  2, 192514,   6000.000, 0x00000000, 0 },
  { 3712,   279,  3, 192514,   8000.000, 0x00000000, 0 },
  { 3713,   279,  4, 192514,  10000.000, 0x00000000, 0 },
  { 3714,   279,  5, 192514,  12000.000, 0x00000000, 0 },
  { 2585,   280,  0, 192538,      0.000, 0x00000000, 0 },
  { 3715,   280,  1, 192538,     16.000, 0x00000000, 0 },
  { 3716,   280,  2, 192538,     24.000, 0x00000000, 0 },
  { 3717,   280,  3, 192538,     32.000, 0x00000000, 0 },
  { 3718,   280,  4, 192538,     40.000, 0x00000000, 0 },
  { 3719,   280,  5, 192538,     48.000, 0x00000000, 0 },
  { 2588,   281,  0, 192557,      0.000, 0x00000000, 0 },
  { 2589,   282,  0, 192548,      0.000, 0x00000000, 0 },
  { 2590,   283,  0, 192570,      0.000, 0x00000000, 0 },
  { 2591,   284,  0, 192558,      0.000, 0x00000000, 0 },
  { 2592,   285,  0, 192567,      0.000, 0x00000000, 0 },
  { 2593,   286,  0, 193213,      0.000, 0x00000000, 0 },
  { 4029,   289,  0, 205223,      0.000, 0x00000000, 0 },
  { 2594,   290,  0, 224968,      0.000, 0x00000000, 0 },
  { 2595,   291,  0, 205495,      0.000, 0x00000000, 0 },
  { 2596,   292,  0, 191602,      0.000, 0x00000000, 0 },
  { 2597,   293,  0, 191647,      0.000, 0x00000000, 0 },
  { 5316,   294,  0, 191512,      0.000, 0x00000000, 0 },
  { 2599,   295,  0, 191717,      0.000, 0x00000000, 0 },
  { 2600,   296,  0, 191861,      0.000, 0x00000000, 0 },
  { 2601,   297,  0, 192630,      0.000, 0x00000000, 0 },
  { 2602,   298,  0, 191493,      0.000, 0x00000000, 0 },
  { 3381,   298,  1, 191493,      2.000, 0x00000000, 0 },
  { 3382,   298,  2, 191493,      3.000, 0x00000000, 0 },
  { 3383,   298,  3, 191493,      4.000, 0x00000000, 0 },
  { 3384,   298,  4, 191493,      5.000, 0x00000000, 0 },
  { 3385,   298,  5, 191493,      6.000, 0x00000000, 0 },
  { 2605,   299,  0, 191499,      0.000, 0x00000000, 0 },
  { 3386,   299,  1, 191499,     10.000, 0x00000000, 0 },
  { 3387,   299,  2, 191499,     15.000, 0x00000000, 0 },
  { 3388,   299,  3, 191499,     20.000, 0x00000000, 0 },
  { 3389,   299,  4, 191499,     25.000, 0x00000000, 0 },
  { 3390,   299,  5, 191499,     30.000, 0x00000000, 0 },
  { 2608,   300,  0, 191504,      3.000, 0x00000000, 0 },
  { 3391,   300,  1, 191504,      6.000, 0x00000000, 0 },
  { 3392,   300,  2, 191504,      9.000, 0x00000000, 0 },
  { 3393,   300,  3, 191504,     12.000, 0x00000000, 0 },
  { 3394,   300,  4, 191504,     15.000, 0x00000000, 0 },
  { 3395,   300,  5, 191504,     18.000, 0x00000000, 0 },
  { 2611,   301,  0, 191740,      8.000, 0x00000000, 0 },
  { 5317,   301,  1, 191740,     16.000, 0x00000000, 0 },
  { 5318,   301,  2, 191740,     24.000, 0x00000000, 0 },
  { 5319,   301,  3, 191740,     32.000, 0x00000000, 0 },
  { 5320,   301,  4, 191740,     40.000, 0x00000000, 0 },
  { 5321,   301,  5, 191740,     48.000, 0x00000000, 0 },
  { 2614,   302,  0, 191569,      0.000, 0x00000000, 0 },
  { 3401,   302,  1, 191569,     30.000, 0x00000000, 0 },
  { 3402,   302,  2, 191569,     45.000, 0x00000000, 0 },
  { 3403,   302,  3, 191569,     60.000, 0x00000000, 0 },
  { 3404,   302,  4, 191569,     75.000, 0x00000000, 0 },
  { 3405,   302,  5, 191569,     90.000, 0x00000000, 0 },
  { 2617,   303,  0, 191572,      4.000, 0x00000000, 0 },
  { 3406,   303,  1, 191572,      8.000, 0x00000000, 0 },
  { 3407,   303,  2, 191572,     12.000, 0x00000000, 0 },
  { 3408,   303,  3, 191572,     16.000, 0x00000000, 0 },
  { 3409,   303,  4, 191572,     20.000, 0x00000000, 0 },
  { 3410,   303,  5, 191572,     24.000, 0x00000000, 0 },
  { 2620,   304,  0, 191577,      0.000, 0x00000000, 0 },
  { 3411,   304,  1, 191577,     10.000, 0x00000000, 0 },
  { 3412,   304,  2, 191577,     15.000, 0x00000000, 0 },
  { 3413,   304,  3, 191577,     20.000, 0x00000000, 0 },
  { 3414,   304,  4, 191577,     25.000, 0x00000000, 0 },
  { 3415,   304,  5, 191577,     30.000, 0x00000000, 0 },
  { 2623,   305,  0, 191582,      0.000, 0x00000000, 0 },
  { 5501,   305,  1, 191582,     28.000, 0x00000000, 0 },
  { 5502,   305,  2, 191582,     26.000, 0x00000000, 0 },
  { 5503,   305,  3, 191582,     24.000, 0x00000000, 0 },
  { 5504,   305,  4, 191582,     22.000, 0x00000000, 0 },
  { 5505,   305,  5, 191582,     20.000, 0x00000000, 0 },
  { 2626,   306,  0, 191598,      4.000, 0x00000000, 0 },
  { 3419,   306,  1, 191598,      8.000, 0x00000000, 0 },
  { 3420,   306,  2, 191598,     12.000, 0x00000000, 0 },
  { 3421,   306,  3, 191598,     16.000, 0x00000000, 0 },
  { 3422,   306,  4, 191598,     20.000, 0x00000000, 0 },
  { 3423,   306,  5, 191598,     24.000, 0x00000000, 0 },
  { 2629,   307,  0, 204147,      0.000, 0x00000000, 0 },
  { 2630,   308,  0, 204219,      0.000, 0x00000000, 0 },
  { 2631,   309,  0, 191328,      0.000, 0x00000000, 0 },
  { 2632,   310,  0, 191339,      0.000, 0x00000000, 0 },
  { 2633,   311,  0, 191048,      0.000, 0x00000000, 0 },
  { 2634,   312,  0, 190449,      0.000, 0x00000000, 0 },
  { 3336,   312,  1, 190449,      6.000, 0x00000000, 0 },
  { 3337,   312,  2, 190449,     10.000, 0x00000000, 0 },
  { 3338,   312,  3, 190449,     13.000, 0x00000000, 0 },
  { 3339,   312,  4, 190449,     16.000, 0x00000000, 0 },
  { 3340,   312,  5, 190449,     20.000, 0x00000000, 0 },
  { 2637,   313,  0, 190457,      0.000, 0x00000000, 0 },
  { 3341,   313,  1, 190457,      6.000, 0x00000000, 0 },
  { 3342,   313,  2, 190457,     10.000, 0x00000000, 0 },
  { 3343,   313,  3, 190457,     13.000, 0x00000000, 0 },
  { 3344,   313,  4, 190457,     16.000, 0x00000000, 0 },
  { 3345,   313,  5, 190457,     20.000, 0x00000000, 0 },
  { 2640,   314,  0, 190462,      0.000, 0x00000000, 0 },
  { 3346,   314,  1, 190462, -20000.000, 0x00000000, 0 },
  { 3347,   314,  2, 190462, -30000.000, 0x00000000, 0 },
  { 3348,   314,  3, 190462, -40000.000, 0x00000000, 0 },
  { 3349,   314,  4, 190462, -50000.000, 0x00000000, 0 },
  { 3350,   314,  5, 190462, -60000.000, 0x00000000, 0 },
  { 2643,   315,  0, 190467,      0.000, 0x00000000, 0 },
  { 3351,   315,  1, 190467,      6.000, 0x00000000, 0 },
  { 3352,   315,  2, 190467,     10.000, 0x00000000, 0 },
  { 3353,   315,  3, 190467,     13.000, 0x00000000, 0 },
  { 3354,   315,  4, 190467,     16.000, 0x00000000, 0 },
  { 3355,   315,  5, 190467,     20.000, 0x00000000, 0 },
  { 2646,   316,  0, 190503,      0.000, 0x00000000, 0 },
  { 3356,   316,  1, 190503,      6.000, 0x00000000, 0 },
  { 3357,   316,  2, 190503,     10.000, 0x00000000, 0 },
  { 3358,   316,  3, 190503,     13.000, 0x00000000, 0 },
  { 3359,   316,  4, 190503,     16.000, 0x00000000, 0 },
  { 3360,   316,  5, 190503,     20.000, 0x00000000, 0 },
  { 2649,   317,  0, 190514,      0.000, 0x00000000, 0 },
  { 3361,   317,  1, 190514,     -6.000, 0x00000000, 0 },
  { 3362,   317,  2, 190514,    -10.000, 0x00000000, 0 },
  { 3363,   317,  3, 190514,    -13.000, 0x00000000, 0 },
  { 3364,   317,  4, 190514,    -16.000, 0x00000000, 0 },
  { 3365,   317,  5, 190514,    -20.000, 0x00000000, 0 },
  { 2652,   318,  0, 190520,      0.000, 0x00000000, 0 },
  { 3366,   318,  1, 190520,      6.000, 0x00000000, 0 },
  { 3367,   318,  2, 190520,     10.000, 0x00000000, 0 },
  { 3368,   318,  3, 190520,     13.000, 0x00000000, 0 },
  { 3369,   318,  4, 190520,     16.000, 0x00000000, 0 },
  { 3370,   318,  5, 190520,     20.000, 0x00000000, 0 },
  { 2655,   319,  0, 190529,      2.000, 0x00000000, 0 },
  { 3371,   319,  1, 190529,      4.000, 0x00000000, 0 },
  { 3372,   319,  2, 190529,      6.000, 0x00000000, 0 },
  { 3373,   319,  3, 190529,      8.000, 0x00000000, 0 },
  { 3374,   319,  4, 190529,     10.000, 0x00000000, 0 },
  { 3375,   319,  5, 190529,     12.000, 0x00000000, 0 },
  { 2658,   320,  0, 190567,      0.000, 0x00000000, 0 },
  { 3376,   320,  1, 190567,    -10.000, 0x00000000, 0 },
  { 3377,   320,  2, 190567,    -15.000, 0x00000000, 0 },
  { 3378,   320,  3, 190567,    -20.000, 0x00000000, 0 },
  { 3379,   320,  4, 190567,    -25.000, 0x00000000, 0 },
  { 3380,   320,  5, 190567,    -30.000, 0x00000000, 0 },
  { 2661,   321,  0, 190852,      0.000, 0x00000000, 0 },
  { 2662,   322,  0, 204089,      0.000, 0x00000000, 0 },
  { 2663,   323,  0, 192310,      0.000, 0x00000000, 0 },
  { 3291,   323,  1, 192310,      6.000, 0x00000000, 0 },
  { 3292,   323,  2, 192310,     10.000, 0x00000000, 0 },
  { 3293,   323,  3, 192310,     13.000, 0x00000000, 0 },
  { 3294,   323,  4, 192310,     16.000, 0x00000000, 0 },
  { 3295,   323,  5, 192310,     20.000, 0x00000000, 0 },
  { 2666,   324,  0, 192315,      0.000, 0x00000000, 0 },
  { 3296,   324,  1, 192315,      6.000, 0x00000000, 0 },
  { 3297,   324,  2, 192315,     10.000, 0x00000000, 0 },
  { 3298,   324,  3, 192315,     13.000, 0x00000000, 0 },
  { 3299,   324,  4, 192315,     16.000, 0x00000000, 0 },
  { 3300,   324,  5, 192315,     20.000, 0x00000000, 0 },
  { 2669,   325,  0, 192318,      0.000, 0x00000000, 0 },
  { 3301,   325,  1, 192318,     10.000, 0x00000000, 0 },
  { 3302,   325,  2, 192318,     15.000, 0x00000000, 0 },
  { 3303,   325,  3, 192318,     20.000, 0x00000000, 0 },
  { 3304,   325,  4, 192318,     25.000, 0x00000000, 0 },
  { 3305,   325,  5, 192318,     30.000, 0x00000000, 0 },
  { 2672,   326,  0, 192323,      0.000, 0x00000000, 0 },
  { 3306,   326,  1, 192323,  -6000.000, 0x00000000, 0 },
  { 3307,   326,  2, 192323, -10000.000, 0x00000000, 0 },
  { 3308,   326,  3, 192323, -13000.000, 0x00000000, 0 },
  { 3309,   326,  4, 192323, -16000.000, 0x00000000, 0 },
  { 3310,   326,  5, 192323, -20000.000, 0x00000000, 0 },
  { 2675,   327,  0, 192326,      0.000, 0x00000000, 0 },
  { 3311,   327,  1, 192326,      4.000, 0x00000000, 0 },
  { 3312,   327,  2, 192326,      6.000, 0x00000000, 0 },
  { 3313,   327,  3, 192326,      8.000, 0x00000000, 0 },
  { 3314,   327,  4, 192326,     10.000, 0x00000000, 0 },
  { 3315,   327,  5, 192326,     12.000, 0x00000000, 0 },
  { 2678,   328,  0, 192329,      0.000, 0x00000000, 0 },
  { 3316,   328,  1, 192329,      6.000, 0x00000000, 0 },
  { 3317,   328,  2, 192329,     10.000, 0x00000000, 0 },
  { 3318,   328,  3, 192329,     13.000, 0x00000000, 0 },
  { 3319,   328,  4, 192329,     16.000, 0x00000000, 0 },
  { 3320,   328,  5, 192329,     20.000, 0x00000000, 0 },
  { 2681,   329,  0, 192345,      0.000, 0x00000000, 0 },
  { 3321,   329,  1, 192345,  -6000.000, 0x00000000, 0 },
  { 3322,   329,  2, 192345, -10000.000, 0x00000000, 0 },
  { 3323,   329,  3, 192345, -13000.000, 0x00000000, 0 },
  { 3324,   329,  4, 192345, -16000.000, 0x00000000, 0 },
  { 3325,   329,  5, 192345, -20000.000, 0x00000000, 0 },
  { 2684,   330,  0, 192349,      0.000, 0x00000000, 0 },
  { 3326,   330,  1, 192349, -20000.000, 0x00000000, 0 },
  { 3327,   330,  2, 192349, -30000.000, 0x00000000, 0 },
  { 3328,   330,  3, 192349, -40000.000, 0x00000000, 0 },
  { 3329,   330,  4, 192349, -50000.000, 0x00000000, 0 },
  { 3330,   330,  5, 192349, -60000.000, 0x00000000, 0 },
  { 2687,   331,  0, 192376,      0.000, 0x00000000, 0 },
  { 3331,   331,  1, 192376,      8.000, 0x00000000, 0 },
  { 3332,   331,  2, 192376,     12.000, 0x00000000, 0 },
  { 3333,   331,  3, 192376,     16.000, 0x00000000, 0 },
  { 3334,   331,  4, 192376,     20.000, 0x00000000, 0 },
  { 3335,   331,  5, 192376,     24.000, 0x00000000, 0 },
  { 2690,   332,  0, 192384,      0.000, 0x00000000, 0 },
  { 2691,   333,  0, 192422,      0.000, 0x00000000, 0 },
  { 2692,   334,  0, 192424,      0.000, 0x00000000, 0 },
  { 2693,   335,  0, 192428,      0.000, 0x00000000, 0 },
  { 2694,   337,  0, 192657,      0.000, 0x00000000, 0 },
  { 2695,   346,  0, 192759,      0.000, 0x00000000, 0 },
  { 2696,   347,  0, 192923,      0.000, 0x00000000, 0 },
  { 2697,   350,  0, 185086,      0.000, 0x00000000, 0 },
  { 2698,   351,  0, 182234,      0.000, 0x00000000, 0 },
  { 2699,   352,  0, 193058,      0.000, 0x00000000, 0 },
  { 2700,   353,  0, 179546,      0.000, 0x00000000, 0 },
  { 3160,   748,  0, 194466,      0.000, 0x00000000, 0 },
  { 2727,   749,  0, 194224,     -3.000, 0x00000000, 0 },
  { 3720,   749,  1, 194224,     -6.000, 0x00000000, 0 },
  { 3721,   749,  2, 194224,     -9.000, 0x00000000, 0 },
  { 3722,   749,  3, 194224,    -12.000, 0x00000000, 0 },
  { 3723,   749,  4, 194224,    -15.000, 0x00000000, 0 },
  { 3724,   749,  5, 194224,    -18.000, 0x00000000, 0 },
  { 3161,   750,  0, 194234,      8.000, 0x00000000, 0 },
  { 3725,   750,  1, 194234,     16.000, 0x00000000, 0 },
  { 3726,   750,  2, 194234,     24.000, 0x00000000, 0 },
  { 3727,   750,  3, 194234,     32.000, 0x00000000, 0 },
  { 3728,   750,  4, 194234,     40.000, 0x00000000, 0 },
  { 3729,   750,  5, 194234,     48.000, 0x00000000, 0 },
  { 3162,   751,  0, 194239,      2.000, 0x00000000, 0 },
  { 3730,   751,  1, 194239,      4.000, 0x00000000, 0 },
  { 3731,   751,  2, 194239,      6.000, 0x00000000, 0 },
  { 3732,   751,  3, 194239,      8.000, 0x00000000, 0 },
  { 3733,   751,  4, 194239,     10.000, 0x00000000, 0 },
  { 3734,   751,  5, 194239,     12.000, 0x00000000, 0 },
  { 3163,   752,  0, 194312,      1.000, 0x00000000, 0 },
  { 3735,   752,  1, 194312,      2.000, 0x00000000, 0 },
  { 3736,   752,  2, 194312,      3.000, 0x00000000, 0 },
  { 3737,   752,  3, 194312,      4.000, 0x00000000, 0 },
  { 3738,   752,  4, 194312,      5.000, 0x00000000, 0 },
  { 3739,   752,  5, 194312,      6.000, 0x00000000, 0 },
  { 5135,   753,  0, 210182,      5.000, 0x00000000, 0 },
  { 5136,   753,  1, 210182,     10.000, 0x00000000, 0 },
  { 5137,   753,  2, 210182,     15.000, 0x00000000, 0 },
  { 5138,   753,  3, 210182,     20.000, 0x00000000, 0 },
  { 5139,   753,  4, 210182,     25.000, 0x00000000, 0 },
  { 5140,   753,  5, 210182,     30.000, 0x00000000, 0 },
  { 3165,   754,  0, 194313,      3.000, 0x00000000, 0 },
  { 3745,   754,  1, 194313,      6.000, 0x00000000, 0 },
  { 3746,   754,  2, 194313,      9.000, 0x00000000, 0 },
  { 3747,   754,  3, 194313,     12.000, 0x00000000, 0 },
  { 3748,   754,  4, 194313,     15.000, 0x00000000, 0 },
  { 3749,   754,  5, 194313,     18.000, 0x00000000, 0 },
  { 3166,   755,  0, 194314,      4.000, 0x00000000, 0 },
  { 3750,   755,  1, 194314,      8.000, 0x00000000, 0 },
  { 3751,   755,  2, 194314,     12.000, 0x00000000, 0 },
  { 3752,   755,  3, 194314,     16.000, 0x00000000, 0 },
  { 3753,   755,  4, 194314,     20.000, 0x00000000, 0 },
  { 3754,   755,  5, 194314,     24.000, 0x00000000, 0 },
  { 3167,   756,  0, 194318,      0.833, 0x00000000, 0 },
  { 3760,   756,  1, 194318,      1.666, 0x00000000, 0 },
  { 3761,   756,  2, 194318,      2.500, 0x00000000, 0 },
  { 3762,   756,  3, 194318,      3.333, 0x00000000, 0 },
  { 3763,   756,  4, 194318,      4.167, 0x00000000, 0 },
  { 3764,   756,  5, 194318,      5.000, 0x00000000, 0 },
  { 3168,   757,  0, 194315,     -5.000, 0x00000000, 0 },
  { 3755,   757,  1, 194315,    -10.000, 0x00000000, 0 },
  { 3756,   757,  2, 194315,    -15.000, 0x00000000, 0 },
  { 3757,   757,  3, 194315,    -20.000, 0x00000000, 0 },
  { 3758,   757,  4, 194315,    -25.000, 0x00000000, 0 },
  { 3759,   757,  5, 194315,    -30.000, 0x00000000, 0 },
  { 5500,   758,  0, 194487,      0.000, 0x00000000, 0 },
  { 3169,   759,  0, 215796,      0.000, 0x00000000, 0 },
  { 3170,   760,  0, 227481,      0.000, 0x00000000, 0 },
  { 3171,   761,  0, 194431,      0.000, 0x00000000, 0 },
  { 5134,   762,  0, 215773,      0.000, 0x00000000, 0 },
  { 3172,   763,  0, 194331,      0.000, 0x00000000, 0 },
  { 3173,   764,  0, 205065,      0.000, 0x00000000, 0 },
  { 3174,   765,  0, 194024,      0.000, 0x00000000, 0 },
  { 3175,   766,  0, 194026,      0.000, 0x00000000, 0 },
  { 3176,   767,  0, 194093,      0.000, 0x00000000, 0 },
  { 3286,   767,  1, 194093,     30.000, 0x00000000, 0 },
  { 3287,   767,  2, 194093,     45.000, 0x00000000, 0 },
  { 3288,   767,  3, 194093,     60.000, 0x00000000, 0 },
  { 3289,   767,  4, 194093,     75.000, 0x00000000, 0 },
  { 3290,   767,  5, 194093,     90.000, 0x00000000, 0 },
  { 3177,   768,  0, 193371,      0.000, 0x00000000, 0 },
  { 3178,   769,  0, 194179,      0.000, 0x00000000, 0 },
  { 3179,   770,  0, 194378,      0.000, 0x00000000, 0 },
  { 3180,   771,  0, 193642,      0.000, 0x00000000, 0 },
  { 3194,   771,  1, 193642, -20000.000, 0x00000000, 0 },
  { 3195,   771,  2, 193642, -30000.000, 0x00000000, 0 },
  { 3196,   771,  3, 193642, -40000.000, 0x00000000, 0 },
  { 3197,   771,  4, 193642, -50000.000, 0x00000000, 0 },
  { 3198,   771,  5, 193642, -60000.000, 0x00000000, 0 },
  { 3181,   772,  0, 193643,      0.000, 0x00000000, 0 },
  { 3251,   772,  1, 193643,     10.000, 0x00000000, 0 },
  { 3252,   772,  2, 193643,     15.000, 0x00000000, 0 },
  { 3253,   772,  3, 193643,     20.000, 0x00000000, 0 },
  { 3254,   772,  4, 193643,     25.000, 0x00000000, 0 },
  { 3255,   772,  5, 193643,     30.000, 0x00000000, 0 },
  { 3182,   773,  0, 193644,      0.000, 0x00000000, 0 },
  { 3256,   773,  1, 193644,     12.000, 0x00000000, 0 },
  { 3257,   773,  2, 193644,     18.000, 0x00000000, 0 },
  { 3258,   773,  3, 193644,     24.000, 0x00000000, 0 },
  { 3259,   773,  4, 193644,     30.000, 0x00000000, 0 },
  { 3260,   773,  5, 193644,     36.000, 0x00000000, 0 },
  { 3183,   774,  0, 193645,      0.000, 0x00000000, 0 },
  { 3261,   774,  1, 193645,      6.000, 0x00000000, 0 },
  { 3262,   774,  2, 193645,     10.000, 0x00000000, 0 },
  { 3263,   774,  3, 193645,     13.000, 0x00000000, 0 },
  { 3264,   774,  4, 193645,     16.000, 0x00000000, 0 },
  { 3265,   774,  5, 193645,     20.000, 0x00000000, 0 },
  { 3184,   775,  0, 193647,      0.000, 0x00000000, 0 },
  { 6513,   775,  1, 193647,     20.000, 0x00000000, 0 },
  { 6514,   775,  2, 193647,     30.000, 0x00000000, 0 },
  { 6515,   775,  3, 193647,     40.000, 0x00000000, 0 },
  { 6516,   775,  4, 193647,     50.000, 0x00000000, 0 },
  { 6517,   775,  5, 193647,     60.000, 0x00000000, 0 },
  { 3185,   776,  0, 194002,      0.000, 0x00000000, 0 },
  { 3271,   776,  1, 194002,      2.000, 0x00000000, 0 },
  { 3272,   776,  2, 194002,      3.000, 0x00000000, 0 },
  { 3273,   776,  3, 194002,      4.000, 0x00000000, 0 },
  { 3274,   776,  4, 194002,      5.000, 0x00000000, 0 },
  { 3275,   776,  5, 194002,      6.000, 0x00000000, 0 },
  { 3186,   777,  0, 194007,      0.000, 0x00000000, 0 },
  { 3276,   777,  1, 194007,     10.000, 0x00000000, 0 },
  { 3277,   777,  2, 194007,     15.000, 0x00000000, 0 },
  { 3278,   777,  3, 194007,     20.000, 0x00000000, 0 },
  { 3279,   777,  4, 194007,     25.000, 0x00000000, 0 },
  { 3280,   777,  5, 194007,     30.000, 0x00000000, 0 },
  { 3187,   778,  0, 194016,      0.000, 0x00000000, 0 },
  { 3281,   778,  1, 194016,     10.000, 0x00000000, 0 },
  { 3282,   778,  2, 194016,     15.000, 0x00000000, 0 },
  { 3283,   778,  3, 194016,     20.000, 0x00000000, 0 },
  { 3284,   778,  4, 194016,     25.000, 0x00000000, 0 },
  { 3285,   778,  5, 194016,     30.000, 0x00000000, 0 },
  { 3188,   779,  0, 194018,      0.000, 0x00000000, 0 },
  { 3777,   783,  0, 214634,      0.000, 0x00000000, 0 },
  { 3782,   784,  0, 195315,      0.000, 0x00000000, 0 },
  { 3783,   784,  1, 195315,      6.000, 0x00000000, 0 },
  { 3784,   784,  2, 195315,     10.000, 0x00000000, 0 },
  { 3785,   784,  3, 195315,     13.000, 0x00000000, 0 },
  { 3786,   784,  4, 195315,     16.000, 0x00000000, 0 },
  { 3787,   784,  5, 195315,     20.000, 0x00000000, 0 },
  { 3788,   785,  0, 195317,      5.000, 0x00000000, 0 },
  { 3789,   785,  1, 195317,     10.000, 0x00000000, 0 },
  { 3790,   785,  2, 195317,     15.000, 0x00000000, 0 },
  { 3791,   785,  3, 195317,     20.000, 0x00000000, 0 },
  { 3792,   785,  4, 195317,     25.000, 0x00000000, 0 },
  { 3793,   785,  5, 195317,     30.000, 0x00000000, 0 },
  { 3794,   786,  0, 195322,      8.000, 0x00000000, 0 },
  { 3795,   786,  1, 195322,     16.000, 0x00000000, 0 },
  { 3796,   786,  2, 195322,     24.000, 0x00000000, 0 },
  { 3797,   786,  3, 195322,     32.000, 0x00000000, 0 },
  { 3798,   786,  4, 195322,     40.000, 0x00000000, 0 },
  { 3799,   786,  5, 195322,     48.000, 0x00000000, 0 },
  { 3800,   787,  0, 195323,     30.000, 0x00000000, 0 },
  { 3801,   787,  1, 195323,     60.000, 0x00000000, 0 },
  { 3802,   787,  2, 195323,     90.000, 0x00000000, 0 },
  { 3803,   787,  3, 195323,    120.000, 0x00000000, 0 },
  { 3804,   787,  4, 195323,    150.000, 0x00000000, 0 },
  { 3805,   787,  5, 195323,    180.000, 0x00000000, 0 },
  { 3806,   788,  0, 195345,   -500.000, 0x00000000, 0 },
  { 3807,   788,  1, 195345,  -1000.000, 0x00000000, 0 },
  { 3808,   788,  2, 195345,  -1500.000, 0x00000000, 0 },
  { 3809,   788,  3, 195345,  -2000.000, 0x00000000, 0 },
  { 3810,   788,  4, 195345,  -2500.000, 0x00000000, 0 },
  { 3811,   788,  5, 195345,  -3000.000, 0x00000000, 0 },
  { 3812,   789,  0, 195351,      2.000, 0x00000000, 0 },
  { 3813,   789,  1, 195351,      4.000, 0x00000000, 0 },
  { 3814,   789,  2, 195351,      6.000, 0x00000000, 0 },
  { 3815,   789,  3, 195351,      8.000, 0x00000000, 0 },
  { 3816,   789,  4, 195351,     10.000, 0x00000000, 0 },
  { 3817,   789,  5, 195351,     12.000, 0x00000000, 0 },
  { 3818,   790,  0, 195352,      5.000, 0x00000000, 0 },
  { 3819,   790,  1, 195352,     10.000, 0x00000000, 0 },
  { 3820,   790,  2, 195352,     15.000, 0x00000000, 0 },
  { 3821,   790,  3, 195352,     20.000, 0x00000000, 0 },
  { 3822,   790,  4, 195352,     25.000, 0x00000000, 0 },
  { 3823,   790,  5, 195352,     30.000, 0x00000000, 0 },
  { 3824,   791,  0, 195354,     20.000, 0x00000000, 0 },
  { 3825,   791,  1, 195354,     40.000, 0x00000000, 0 },
  { 3826,   791,  2, 195354,     60.000, 0x00000000, 0 },
  { 3827,   791,  3, 195354,     80.000, 0x00000000, 0 },
  { 3828,   791,  4, 195354,    100.000, 0x00000000, 0 },
  { 3829,   791,  5, 195354,    120.000, 0x00000000, 0 },
  { 3830,   792,  0, 214626,     -2.000, 0x00000000, 0 },
  { 3831,   792,  1, 214626,     -4.000, 0x00000000, 0 },
  { 3832,   792,  2, 214626,     -6.000, 0x00000000, 0 },
  { 3833,   792,  3, 214626,     -8.000, 0x00000000, 0 },
  { 3834,   792,  4, 214626,    -10.000, 0x00000000, 0 },
  { 3835,   792,  5, 214626,    -12.000, 0x00000000, 0 },
  { 3836,   793,  0, 195419,      0.000, 0x00000000, 0 },
  { 5584,   794,  0, 214664,      0.000, 0x00000000, 0 },
  { 3837,   795,  0, 220817,      0.000, 0x00000000, 0 },
  { 5618,   796,  0, 214776,      0.000, 0x00000000, 0 },
  { 5615,   797,  0, 195615,      0.000, 0x00000000, 0 },
  { 3838,   798,  0, 195448,      0.000, 0x00000000, 0 },
  { 3765,   800,  0, 195243,      0.000, 0x00000000, 0 },
  { 3766,   800,  1, 195243,     20.000, 0x00000000, 0 },
  { 3767,   800,  2, 195243,     30.000, 0x00000000, 0 },
  { 3768,   800,  3, 195243,     40.000, 0x00000000, 0 },
  { 3769,   800,  4, 195243,     50.000, 0x00000000, 0 },
  { 3770,   800,  5, 195243,     60.000, 0x00000000, 0 },
  { 3771,   801,  0, 195244,      3.000, 0x00000000, 0 },
  { 3772,   801,  1, 195244,      6.000, 0x00000000, 0 },
  { 3773,   801,  2, 195244,     10.000, 0x00000000, 0 },
  { 3774,   801,  3, 195244,     13.000, 0x00000000, 0 },
  { 3775,   801,  4, 195244,     16.000, 0x00000000, 0 },
  { 3776,   801,  5, 195244,     20.000, 0x00000000, 0 },
  { 3903,   803,  0, 196586,      0.000, 0x00000000, 0 },
  { 3904,   804,  0, 196211,      3.000, 0x00000000, 0 },
  { 3905,   804,  1, 196211,      6.000, 0x00000000, 0 },
  { 3906,   804,  2, 196211,      9.000, 0x00000000, 0 },
  { 3907,   804,  3, 196211,     12.000, 0x00000000, 0 },
  { 3908,   804,  4, 196211,     15.000, 0x00000000, 0 },
  { 3909,   804,  5, 196211,     18.000, 0x00000000, 0 },
  { 3910,   805,  0, 196217,      5.000, 0x00000000, 0 },
  { 3911,   805,  1, 196217,     10.000, 0x00000000, 0 },
  { 3912,   805,  2, 196217,     15.000, 0x00000000, 0 },
  { 3913,   805,  3, 196217,     20.000, 0x00000000, 0 },
  { 3914,   805,  4, 196217,     25.000, 0x00000000, 0 },
  { 3915,   805,  5, 196217,     30.000, 0x00000000, 0 },
  { 3916,   806,  0, 196222,     -2.000, 0x00000000, 0 },
  { 3917,   806,  1, 196222,     -4.000, 0x00000000, 0 },
  { 3918,   806,  2, 196222,     -6.000, 0x00000000, 0 },
  { 3919,   806,  3, 196222,     -8.000, 0x00000000, 0 },
  { 3920,   806,  4, 196222,    -10.000, 0x00000000, 0 },
  { 3921,   806,  5, 196222,    -12.000, 0x00000000, 0 },
  { 3922,   807,  0, 196227,      6.000, 0x00000000, 0 },
  { 3923,   807,  1, 196227,     12.000, 0x00000000, 0 },
  { 3924,   807,  2, 196227,     18.000, 0x00000000, 0 },
  { 3925,   807,  3, 196227,     24.000, 0x00000000, 0 },
  { 3926,   807,  4, 196227,     30.000, 0x00000000, 0 },
  { 3927,   807,  5, 196227,     36.000, 0x00000000, 0 },
  { 3928,   808,  0, 196236,      0.000, 0x00000000, 0 },
  { 3929,   808,  1, 196236,     10.000, 0x00000000, 0 },
  { 3930,   808,  2, 196236,     15.000, 0x00000000, 0 },
  { 3931,   808,  3, 196236,     20.000, 0x00000000, 0 },
  { 3932,   808,  4, 196236,     25.000, 0x00000000, 0 },
  { 3933,   808,  5, 196236,     30.000, 0x00000000, 0 },
  { 3955,   809,  0, 196432,      8.000, 0x00000000, 0 },
  { 3956,   809,  1, 196432,     16.000, 0x00000000, 0 },
  { 3957,   809,  2, 196432,     24.000, 0x00000000, 0 },
  { 3958,   809,  3, 196432,     32.000, 0x00000000, 0 },
  { 3959,   809,  4, 196432,     40.000, 0x00000000, 0 },
  { 3960,   809,  5, 196432,     48.000, 0x00000000, 0 },
  { 3934,   810,  0, 196258,      0.000, 0x00000000, 0 },
  { 3935,   810,  1, 196258,     10.000, 0x00000000, 0 },
  { 3936,   810,  2, 196258,     15.000, 0x00000000, 0 },
  { 3937,   810,  3, 196258,     20.000, 0x00000000, 0 },
  { 3938,   810,  4, 196258,     25.000, 0x00000000, 0 },
  { 3939,   810,  5, 196258,     30.000, 0x00000000, 0 },
  { 3942,   811,  0, 196301,     15.000, 0x00000000, 0 },
  { 3943,   811,  1, 196301,     30.000, 0x00000000, 0 },
  { 3944,   811,  2, 196301,     45.000, 0x00000000, 0 },
  { 3945,   811,  3, 196301,     60.000, 0x00000000, 0 },
  { 3946,   811,  4, 196301,     75.000, 0x00000000, 0 },
  { 3947,   811,  5, 196301,     90.000, 0x00000000, 0 },
  { 3948,   812,  0, 196305,     -3.000, 0x00000000, 0 },
  { 3949,   812,  1, 196305,     -6.000, 0x00000000, 0 },
  { 3950,   812,  2, 196305,     -9.000, 0x00000000, 0 },
  { 3951,   812,  3, 196305,    -12.000, 0x00000000, 0 },
  { 3952,   812,  4, 196305,    -15.000, 0x00000000, 0 },
  { 3953,   812,  5, 196305,    -18.000, 0x00000000, 0 },
  { 3966,   813,  0, 196675,      0.000, 0x00000000, 0 },
  { 6445,   814,  0, 215223,      0.000, 0x00000000, 0 },
  { 6446,   815,  0, 215273,      0.000, 0x00000000, 0 },
  { 3961,   816,  0, 219415,      0.000, 0x00000000, 0 },
  { 3964,   817,  0, 219195,      0.000, 0x00000000, 0 },
  { 3965,   818,  0, 224103,      0.000, 0x00000000, 0 },
  { 3858,   820,  0, 195263,      0.000, 0x00000000, 0 },
  { 3859,   820,  1, 195263,     10.000, 0x00000000, 0 },
  { 3860,   820,  2, 195263,     15.000, 0x00000000, 0 },
  { 3861,   820,  3, 195263,     20.000, 0x00000000, 0 },
  { 3862,   820,  4, 195263,     25.000, 0x00000000, 0 },
  { 3863,   820,  5, 195263,     30.000, 0x00000000, 0 },
  { 3864,   821,  0, 218607,      0.000, 0x00000000, 0 },
  { 6483,   821,  1, 218607,     20.000, 0x00000000, 0 },
  { 6484,   821,  2, 218607,     30.000, 0x00000000, 0 },
  { 6485,   821,  3, 218607,     40.000, 0x00000000, 0 },
  { 6486,   821,  4, 218607,     50.000, 0x00000000, 0 },
  { 6487,   821,  5, 218607,     60.000, 0x00000000, 0 },
  { 3870,   822,  0, 195266,      0.000, 0x00000000, 0 },
  { 3871,   822,  1, 195266,     20.000, 0x00000000, 0 },
  { 3872,   822,  2, 195266,     30.000, 0x00000000, 0 },
  { 3873,   822,  3, 195266,     40.000, 0x00000000, 0 },
  { 4881,   822,  4, 195266,     50.000, 0x00000000, 0 },
  { 6526,   822,  5, 195266,     60.000, 0x00000000, 0 },
  { 3880,   824,  0, 195269,      0.000, 0x00000000, 0 },
  { 3881,   824,  1, 195269,      6.000, 0x00000000, 0 },
  { 3882,   824,  2, 195269,     10.000, 0x00000000, 0 },
  { 3883,   824,  3, 195269,     13.000, 0x00000000, 0 },
  { 3884,   824,  4, 195269,     16.000, 0x00000000, 0 },
  { 3885,   824,  5, 195269,     20.000, 0x00000000, 0 },
  { 3886,   825,  0, 195291,      0.000, 0x00000000, 0 },
  { 3887,   825,  1, 195291,     10.000, 0x00000000, 0 },
  { 3888,   825,  2, 195291,     15.000, 0x00000000, 0 },
  { 4878,   825,  3, 195291,     20.000, 0x00000000, 0 },
  { 4879,   825,  4, 195291,     25.000, 0x00000000, 0 },
  { 4880,   825,  5, 195291,     30.000, 0x00000000, 0 },
  { 3889,   826,  0, 195295,      0.000, 0x00000000, 0 },
  { 3890,   827,  0, 195298,      0.000, 0x00000000, 0 },
  { 3891,   828,  0, 195300,      0.000, 0x00000000, 0 },
  { 3892,   829,  0, 195380,      0.000, 0x00000000, 0 },
  { 3893,   829,  1, 195380,     15.000, 0x00000000, 0 },
  { 3894,   829,  2, 195380,     20.000, 0x00000000, 0 },
  { 3895,   829,  3, 195380,     25.000, 0x00000000, 0 },
  { 3896,   829,  4, 195380,     30.000, 0x00000000, 0 },
  { 3897,   829,  5, 195380,     35.000, 0x00000000, 0 },
  { 3898,   830,  0, 195399,      0.000, 0x00000000, 0 },
  { 3899,   831,  0, 205320,      0.000, 0x00000000, 0 },
  { 3900,   832,  0, 195650,      0.000, 0x00000000, 0 },
  { 3901,   833,  0, 196082,      0.000, 0x00000000, 0 },
  { 3967,   834,  0, 196684,      0.000, 0x00000000, 0 },
  { 3968,   835,  0, 196492,      0.000, 0x00000000, 0 },
  { 3969,   836,  0, 208065,      0.000, 0x00000000, 0 },
  { 3970,   837,  0, 196578,      0.000, 0x00000000, 0 },
  { 3971,   838,  0, 196489,      6.000, 0x00000000, 0 },
  { 5506,   838,  1, 196489,     12.000, 0x00000000, 0 },
  { 5507,   838,  2, 196489,     18.000, 0x00000000, 0 },
  { 5508,   838,  3, 196489,     24.000, 0x00000000, 0 },
  { 5509,   838,  4, 196489,     30.000, 0x00000000, 0 },
  { 5510,   838,  5, 196489,     36.000, 0x00000000, 0 },
  { 3972,   839,  0, 196437,      0.000, 0x00000000, 0 },
  { 3973,   840,  0, 196779,      0.000, 0x00000000, 0 },
  { 3974,   841,  0, 196355,      0.000, 0x00000000, 0 },
  { 3975,   841,  1, 196355,     10.000, 0x00000000, 0 },
  { 3976,   841,  2, 196355,     15.000, 0x00000000, 0 },
  { 3977,   841,  3, 196355,     20.000, 0x00000000, 0 },
  { 3978,   841,  4, 196355,     25.000, 0x00000000, 0 },
  { 3979,   841,  5, 196355,     30.000, 0x00000000, 0 },
  { 3980,   842,  0, 196358,      6.000, 0x00000000, 0 },
  { 3981,   842,  1, 196358,     12.000, 0x00000000, 0 },
  { 3982,   842,  2, 196358,     18.000, 0x00000000, 0 },
  { 3983,   842,  3, 196358,     24.000, 0x00000000, 0 },
  { 3984,   842,  4, 196358,     30.000, 0x00000000, 0 },
  { 3985,   842,  5, 196358,     36.000, 0x00000000, 0 },
  { 3986,   843,  0, 196416,      0.000, 0x00000000, 0 },
  { 3987,   843,  1, 196416,     10.000, 0x00000000, 0 },
  { 3988,   843,  2, 196416,     15.000, 0x00000000, 0 },
  { 3989,   843,  3, 196416,     20.000, 0x00000000, 0 },
  { 3990,   843,  4, 196416,     25.000, 0x00000000, 0 },
  { 3991,   843,  5, 196416,     30.000, 0x00000000, 0 },
  { 3992,   844,  0, 196418,      3.000, 0x00000000, 0 },
  { 3993,   844,  1, 196418,      6.000, 0x00000000, 0 },
  { 3994,   844,  2, 196418,      9.000, 0x00000000, 0 },
  { 3995,   844,  3, 196418,     12.000, 0x00000000, 0 },
  { 3996,   844,  4, 196418,     15.000, 0x00000000, 0 },
  { 3997,   844,  5, 196418,     18.000, 0x00000000, 0 },
  { 3998,   845,  0, 196419,      0.000, 0x00000000, 0 },
  { 4004,   846,  0, 196422,      5.000, 0x00000000, 0 },
  { 4005,   846,  1, 196422,     10.000, 0x00000000, 0 },
  { 4006,   846,  2, 196422,     15.000, 0x00000000, 0 },
  { 4007,   846,  3, 196422,     20.000, 0x00000000, 0 },
  { 4008,   846,  4, 196422,     25.000, 0x00000000, 0 },
  { 4009,   846,  5, 196422,     30.000, 0x00000000, 0 },
  { 4010,   847,  0, 196429,      0.000, 0x00000000, 0 },
  { 4011,   847,  1, 196429,  -6000.000, 0x00000000, 0 },
  { 4012,   847,  2, 196429, -10000.000, 0x00000000, 0 },
  { 4013,   847,  3, 196429, -13000.000, 0x00000000, 0 },
  { 4014,   847,  4, 196429, -16000.000, 0x00000000, 0 },
  { 4015,   847,  5, 196429, -20000.000, 0x00000000, 0 },
  { 4016,   848,  0, 196430,      0.000, 0x00000000, 0 },
  { 4017,   848,  1, 196430,      6.000, 0x00000000, 0 },
  { 4018,   848,  2, 196430,     10.000, 0x00000000, 0 },
  { 4019,   848,  3, 196430,     13.000, 0x00000000, 0 },
  { 4020,   848,  4, 196430,     16.000, 0x00000000, 0 },
  { 4021,   848,  5, 196430,     20.000, 0x00000000, 0 },
  { 4022,   849,  0, 196434,      4.000, 0x00000000, 0 },
  { 4023,   849,  1, 196434,      8.000, 0x00000000, 0 },
  { 4024,   849,  2, 196434,     12.000, 0x00000000, 0 },
  { 4025,   849,  3, 196434,     16.000, 0x00000000, 0 },
  { 4026,   849,  4, 196434,     20.000, 0x00000000, 0 },
  { 4027,   849,  5, 196434,     24.000, 0x00000000, 0 },
  { 4030,   851,  0, 209782,      0.000, 0x00000000, 0 },
  { 4031,   852,  0, 197231,      3.000, 0x00000000, 0 },
  { 4046,   852,  1, 197231,      6.000, 0x00000000, 0 },
  { 4047,   852,  2, 197231,     10.000, 0x00000000, 0 },
  { 4048,   852,  3, 197231,     13.000, 0x00000000, 0 },
  { 4049,   852,  4, 197231,     16.000, 0x00000000, 0 },
  { 4050,   852,  5, 197231,     20.000, 0x00000000, 0 },
  { 4032,   853,  0, 197233,      0.000, 0x00000000, 0 },
  { 4051,   853,  1, 197233,     10.000, 0x00000000, 0 },
  { 4052,   853,  2, 197233,     15.000, 0x00000000, 0 },
  { 4053,   853,  3, 197233,     20.000, 0x00000000, 0 },
  { 4054,   853,  4, 197233,     25.000, 0x00000000, 0 },
  { 4055,   853,  5, 197233,     30.000, 0x00000000, 0 },
  { 4033,   854,  0, 197234,      0.000, 0x00000000, 0 },
  { 4056,   854,  1, 197234,     10.000, 0x00000000, 0 },
  { 4057,   854,  2, 197234,     15.000, 0x00000000, 0 },
  { 4058,   854,  3, 197234,     20.000, 0x00000000, 0 },
  { 4059,   854,  4, 197234,     25.000, 0x00000000, 0 },
  { 4060,   854,  5, 197234,     30.000, 0x00000000, 0 },
  { 4034,   855,  0, 197235,      5.000, 0x00000000, 0 },
  { 4061,   855,  1, 197235,     10.000, 0x00000000, 0 },
  { 4062,   855,  2, 197235,     15.000, 0x00000000, 0 },
  { 4063,   855,  3, 197235,     20.000, 0x00000000, 0 },
  { 4064,   855,  4, 197235,     25.000, 0x00000000, 0 },
  { 4065,   855,  5, 197235,     30.000, 0x00000000, 0 },
  { 4035,   856,  0, 197369,      5.000, 0x00000000, 0 },
  { 4142,   856,  1, 197369,     10.000, 0x00000000, 0 },
  { 4143,   856,  2, 197369,     15.000, 0x00000000, 0 },
  { 4144,   856,  3, 197369,     20.000, 0x00000000, 0 },
  { 4145,   856,  4, 197369,     25.000, 0x00000000, 0 },
  { 4146,   856,  5, 197369,     30.000, 0x00000000, 0 },
  { 4036,   857,  0, 197386,   3000.000, 0x00000000, 0 },
  { 4147,   857,  1, 197386,   6000.000, 0x00000000, 0 },
  { 4148,   857,  2, 197386,  10000.000, 0x00000000, 0 },
  { 4149,   857,  3, 197386,  13000.000, 0x00000000, 0 },
  { 4150,   857,  4, 197386,  16000.000, 0x00000000, 0 },
  { 4151,   857,  5, 197386,  20000.000, 0x00000000, 0 },
  { 4037,   858,  0, 197239,      0.000, 0x00000000, 0 },
  { 4066,   858,  1, 197239,     10.000, 0x00000000, 0 },
  { 4067,   858,  2, 197239,     15.000, 0x00000000, 0 },
  { 4068,   858,  3, 197239,     20.000, 0x00000000, 0 },
  { 4069,   858,  4, 197239,     25.000, 0x00000000, 0 },
  { 4070,   858,  5, 197239,     30.000, 0x00000000, 0 },
  { 4038,   859,  0, 197241,      0.000, 0x00000000, 0 },
  { 4071,   859,  1, 210144,      0.000, 0x00000000, 0 },
  { 4072,   859,  2, 210145,      0.000, 0x00000000, 0 },
  { 4073,   859,  3, 210146,      0.000, 0x00000000, 0 },
  { 4074,   859,  4, 210147,      0.000, 0x00000000, 0 },
  { 4075,   859,  5, 210148,      0.000, 0x00000000, 0 },
  { 4039,   860,  0, 197244,      0.000, 0x00000000, 0 },
  { 4076,   860,  1, 197244,      6.000, 0x00000000, 0 },
  { 4077,   860,  2, 197244,     10.000, 0x00000000, 0 },
  { 4078,   860,  3, 197244,     13.000, 0x00000000, 0 },
  { 4079,   860,  4, 197244,     16.000, 0x00000000, 0 },
  { 4080,   860,  5, 197244,     20.000, 0x00000000, 0 },
  { 4040,   861,  0, 197610,      0.000, 0x00000000, 0 },
  { 4041,   862,  0, 197256,      0.000, 0x00000000, 0 },
  { 4042,   863,  0, 197604,      0.000, 0x00000000, 0 },
  { 4043,   864,  0, 209835,      0.000, 0x00000000, 0 },
  { 4044,   865,  0, 209781,      0.000, 0x00000000, 0 },
  { 4045,   866,  0, 197406,      0.000, 0x00000000, 0 },
  { 4087,   868,  0, 197038,      0.000, 0x00000000, 0 },
  { 4088,   868,  1, 197038,   2000.000, 0x00000000, 0 },
  { 4089,   868,  2, 197038,   3000.000, 0x00000000, 0 },
  { 4090,   868,  3, 197038,   4000.000, 0x00000000, 0 },
  { 4091,   868,  4, 197038,   5000.000, 0x00000000, 0 },
  { 4092,   868,  5, 197038,   6000.000, 0x00000000, 0 },
  { 4093,   869,  0, 197047,      0.000, 0x00000000, 0 },
  { 4094,   869,  1, 197047,     20.000, 0x00000000, 0 },
  { 4095,   869,  2, 197047,     30.000, 0x00000000, 0 },
  { 4096,   869,  3, 197047,     40.000, 0x00000000, 0 },
  { 4097,   869,  4, 197047,     50.000, 0x00000000, 0 },
  { 4098,   869,  5, 197047,     60.000, 0x00000000, 0 },
  { 4099,   870,  0, 197080,      0.000, 0x00000000, 0 },
  { 4100,   870,  1, 197080,     10.000, 0x00000000, 0 },
  { 4101,   870,  2, 197080,     15.000, 0x00000000, 0 },
  { 4102,   870,  3, 197080,     20.000, 0x00000000, 0 },
  { 4103,   870,  4, 197080,     25.000, 0x00000000, 0 },
  { 4104,   870,  5, 197080,     30.000, 0x00000000, 0 },
  { 4105,   871,  0, 197138,      0.000, 0x00000000, 0 },
  { 4106,   871,  1, 197138,      4.000, 0x00000000, 0 },
  { 4107,   871,  2, 197138,      6.000, 0x00000000, 0 },
  { 4108,   871,  3, 197138,      8.000, 0x00000000, 0 },
  { 4109,   871,  4, 197138,     10.000, 0x00000000, 0 },
  { 4110,   871,  5, 197138,     12.000, 0x00000000, 0 },
  { 4111,   872,  0, 197139,      0.000, 0x00000000, 0 },
  { 4112,   872,  1, 197139,     10.000, 0x00000000, 0 },
  { 4113,   872,  2, 197139,     15.000, 0x00000000, 0 },
  { 4114,   872,  3, 197139,     20.000, 0x00000000, 0 },
  { 4115,   872,  4, 197139,     25.000, 0x00000000, 0 },
  { 4116,   872,  5, 197139,     30.000, 0x00000000, 0 },
  { 4117,   873,  0, 197140,      0.000, 0x00000000, 0 },
  { 4118,   873,  1, 197140,      6.000, 0x00000000, 0 },
  { 4119,   873,  2, 197140,     10.000, 0x00000000, 0 },
  { 4120,   873,  3, 197140,     13.000, 0x00000000, 0 },
  { 4121,   873,  4, 197140,     16.000, 0x00000000, 0 },
  { 4122,   873,  5, 197140,     20.000, 0x00000000, 0 },
  { 4123,   874,  0, 197160,      0.000, 0x00000000, 0 },
  { 4124,   874,  1, 197160,     20.000, 0x00000000, 0 },
  { 4125,   874,  2, 197160,     30.000, 0x00000000, 0 },
  { 4126,   874,  3, 197160,     40.000, 0x00000000, 0 },
  { 4127,   874,  4, 197160,     50.000, 0x00000000, 0 },
  { 4128,   874,  5, 197160,     60.000, 0x00000000, 0 },
  { 4129,   875,  0, 197162,     10.000, 0x00000000, 0 },
  { 4130,   875,  1, 197162,     20.000, 0x00000000, 0 },
  { 4131,   875,  2, 197162,     30.000, 0x00000000, 0 },
  { 4132,   875,  3, 197162,     40.000, 0x00000000, 0 },
  { 4133,   875,  4, 197162,     50.000, 0x00000000, 0 },
  { 4134,   875,  5, 197162,     60.000, 0x00000000, 0 },
  { 4135,   876,  0, 197178,      0.000, 0x00000000, 0 },
  { 4136,   877,  0, 197199,      0.000, 0x00000000, 0 },
  { 4137,   878,  0, 197248,      0.000, 0x00000000, 0 },
  { 4138,   879,  0, 207068,      0.000, 0x00000000, 0 },
  { 4139,   880,  0, 197343,      0.000, 0x00000000, 0 },
  { 4140,   881,  0, 197344,      0.000, 0x00000000, 0 },
  { 4141,   882,  0, 197354,      0.000, 0x00000000, 0 },
  { 4152,   883,  0, 207946,      0.000, 0x00000000, 0 },
  { 4153,   884,  0, 197779,      0.000, 0x00000000, 0 },
  { 4154,   885,  0, 197781,      0.000, 0x00000000, 0 },
  { 4155,   886,  0, 197815,      0.000, 0x00000000, 0 },
  { 4156,   887,  0, 198074,      0.000, 0x00000000, 0 },
  { 4157,   888,  0, 197708,      4.000, 0x00000000, 0 },
  { 4158,   888,  1, 197708,      8.000, 0x00000000, 0 },
  { 4159,   888,  2, 197708,     12.000, 0x00000000, 0 },
  { 4160,   888,  3, 197708,     16.000, 0x00000000, 0 },
  { 4161,   888,  4, 197708,     20.000, 0x00000000, 0 },
  { 4162,   888,  5, 197708,     24.000, 0x00000000, 0 },
  { 4163,   889,  0, 197711,      0.000, 0x00000000, 0 },
  { 4164,   889,  1, 197711,     -6.000, 0x00000000, 0 },
  { 4165,   889,  2, 197711,    -10.000, 0x00000000, 0 },
  { 4166,   889,  3, 197711,    -13.000, 0x00000000, 0 },
  { 4167,   889,  4, 197711,    -16.000, 0x00000000, 0 },
  { 4168,   889,  5, 197711,    -20.000, 0x00000000, 0 },
  { 4169,   890,  0, 197713,      0.000, 0x00000000, 0 },
  { 4170,   890,  1, 197713, -20000.000, 0x00000000, 0 },
  { 4171,   890,  2, 197713, -30000.000, 0x00000000, 0 },
  { 4172,   890,  3, 197713, -40000.000, 0x00000000, 0 },
  { 4173,   890,  4, 197713, -50000.000, 0x00000000, 0 },
  { 4174,   890,  5, 197713, -60000.000, 0x00000000, 0 },
  { 4175,   891,  0, 197715,      5.000, 0x00000000, 0 },
  { 4176,   891,  1, 197715,     10.000, 0x00000000, 0 },
  { 4177,   891,  2, 197715,     15.000, 0x00000000, 0 },
  { 4178,   891,  3, 197715,     20.000, 0x00000000, 0 },
  { 4179,   891,  4, 197715,     25.000, 0x00000000, 0 },
  { 4180,   891,  5, 197715,     30.000, 0x00000000, 0 },
  { 4181,   892,  0, 197716,      5.000, 0x00000000, 0 },
  { 4182,   892,  1, 197716,     10.000, 0x00000000, 0 },
  { 4183,   892,  2, 197716,     15.000, 0x00000000, 0 },
  { 4184,   892,  3, 197716,     20.000, 0x00000000, 0 },
  { 4185,   892,  4, 197716,     25.000, 0x00000000, 0 },
  { 4186,   892,  5, 197716,     30.000, 0x00000000, 0 },
  { 4187,   893,  0, 197727,      0.000, 0x00000000, 0 },
  { 4188,   893,  1, 197727,   2000.000, 0x00000000, 0 },
  { 4189,   893,  2, 197727,   3000.000, 0x00000000, 0 },
  { 4190,   893,  3, 197727,   4000.000, 0x00000000, 0 },
  { 4191,   893,  4, 197727,   5000.000, 0x00000000, 0 },
  { 4192,   893,  5, 197727,   6000.000, 0x00000000, 0 },
  { 4193,   894,  0, 197729,      5.000, 0x00000000, 0 },
  { 4194,   894,  1, 197729,     10.000, 0x00000000, 0 },
  { 4195,   894,  2, 197729,     15.000, 0x00000000, 0 },
  { 4196,   894,  3, 197729,     20.000, 0x00000000, 0 },
  { 4197,   894,  4, 197729,     25.000, 0x00000000, 0 },
  { 4198,   894,  5, 197729,     30.000, 0x00000000, 0 },
  { 4199,   895,  0, 197762,     -5.000, 0x00000000, 0 },
  { 4200,   895,  1, 197762,    -10.000, 0x00000000, 0 },
  { 4201,   895,  2, 197762,    -15.000, 0x00000000, 0 },
  { 4202,   895,  3, 197762,    -20.000, 0x00000000, 0 },
  { 4203,   895,  4, 197762,    -25.000, 0x00000000, 0 },
  { 4204,   895,  5, 197762,    -30.000, 0x00000000, 0 },
  { 4205,   896,  0, 216212,      0.000, 0x00000000, 0 },
  { 6453,   896,  1, 216212,      6.000, 0x00000000, 0 },
  { 6454,   896,  2, 216212,     10.000, 0x00000000, 0 },
  { 6455,   896,  3, 216212,     13.000, 0x00000000, 0 },
  { 6456,   896,  4, 216212,     16.000, 0x00000000, 0 },
  { 6457,   896,  5, 216212,     20.000, 0x00000000, 0 },
  { 4211,   897,  0, 198068,      0.000, 0x00000000, 0 },
  { 4212,   898,  0, 197766,      0.000, 0x00000000, 0 },
  { 4213,   899,  0, 204945,      0.000, 0x00000000, 0 },
  { 4214,   900,  0, 198736,      0.000, 0x00000000, 0 },
  { 4215,   901,  0, 198505,      0.000, 0x00000000, 0 },
  { 4216,   902,  0, 198361,      0.000, 0x00000000, 0 },
  { 4217,   903,  0, 198367,      0.000, 0x00000000, 0 },
  { 4218,   904,  0, 198434,      0.000, 0x00000000, 0 },
  { 4219,   905,  0, 215381,      0.000, 0x00000000, 0 },
  { 6447,   905,  1, 215381,      6.000, 0x00000000, 0 },
  { 6448,   905,  2, 215381,     10.000, 0x00000000, 0 },
  { 6449,   905,  3, 215381,     13.000, 0x00000000, 0 },
  { 6450,   905,  4, 215381,     16.000, 0x00000000, 0 },
  { 6451,   905,  5, 215381,     20.000, 0x00000000, 0 },
  { 4225,   906,  0, 198236,      0.000, 0x00000000, 0 },
  { 4226,   906,  1, 198236,      6.000, 0x00000000, 0 },
  { 4227,   906,  2, 198236,     10.000, 0x00000000, 0 },
  { 4228,   906,  3, 198236,     13.000, 0x00000000, 0 },
  { 4229,   906,  4, 198236,     16.000, 0x00000000, 0 },
  { 4230,   906,  5, 198236,     20.000, 0x00000000, 0 },
  { 4231,   907,  0, 198238,      0.000, 0x00000000, 0 },
  { 4232,   907,  1, 198238,     10.000, 0x00000000, 0 },
  { 4233,   907,  2, 198238,     15.000, 0x00000000, 0 },
  { 4234,   907,  3, 198238,     20.000, 0x00000000, 0 },
  { 4235,   907,  4, 198238,     25.000, 0x00000000, 0 },
  { 4236,   907,  5, 198238,     30.000, 0x00000000, 0 },
  { 4237,   908,  0, 198247,      0.000, 0x00000000, 0 },
  { 4238,   908,  1, 198247,     10.000, 0x00000000, 0 },
  { 4239,   908,  2, 198247,     15.000, 0x00000000, 0 },
  { 4240,   908,  3, 198247,     20.000, 0x00000000, 0 },
  { 4241,   908,  4, 198247,     25.000, 0x00000000, 0 },
  { 4242,   908,  5, 198247,     30.000, 0x00000000, 0 },
  { 4243,   909,  0, 198248,      0.000, 0x00000000, 0 },
  { 4244,   909,  1, 198248,      4.000, 0x00000000, 0 },
  { 4245,   909,  2, 198248,      6.000, 0x00000000, 0 },
  { 4246,   909,  3, 198248,      8.000, 0x00000000, 0 },
  { 4247,   909,  4, 198248,     10.000, 0x00000000, 0 },
  { 4248,   909,  5, 198248,     12.000, 0x00000000, 0 },
  { 4249,   910,  0, 198292,     10.000, 0x00000000, 0 },
  { 4250,   910,  1, 198292,     20.000, 0x00000000, 0 },
  { 4251,   910,  2, 198292,     30.000, 0x00000000, 0 },
  { 4252,   910,  3, 198292,     40.000, 0x00000000, 0 },
  { 4253,   910,  4, 198292,     50.000, 0x00000000, 0 },
  { 4254,   910,  5, 198292,     60.000, 0x00000000, 0 },
  { 4255,   911,  0, 198296,      0.000, 0x00000000, 0 },
  { 4256,   911,  1, 198296,     20.000, 0x00000000, 0 },
  { 4257,   911,  2, 198296,     30.000, 0x00000000, 0 },
  { 4258,   911,  3, 198296,     40.000, 0x00000000, 0 },
  { 4259,   911,  4, 198296,     50.000, 0x00000000, 0 },
  { 4260,   911,  5, 198296,     60.000, 0x00000000, 0 },
  { 4261,   912,  0, 198299,      0.000, 0x00000000, 0 },
  { 4262,   912,  1, 198299,      4.000, 0x00000000, 0 },
  { 4263,   912,  2, 198299,      6.000, 0x00000000, 0 },
  { 4264,   912,  3, 198299,      8.000, 0x00000000, 0 },
  { 4265,   912,  4, 198299,     10.000, 0x00000000, 0 },
  { 4266,   912,  5, 198299,     12.000, 0x00000000, 0 },
  { 4267,   913,  0, 198349,      0.000, 0x00000000, 0 },
  { 4268,   913,  1, 198349,      2.000, 0x00000000, 0 },
  { 4269,   913,  2, 198349,      3.000, 0x00000000, 0 },
  { 4270,   913,  3, 198349,      4.000, 0x00000000, 0 },
  { 4271,   913,  4, 198349,      5.000, 0x00000000, 0 },
  { 4272,   913,  5, 198349,      6.000, 0x00000000, 0 },
  { 4273,   915,  0, 199111,      4.000, 0x00000000, 0 },
  { 4274,   915,  1, 199111,      8.000, 0x00000000, 0 },
  { 4275,   915,  2, 199111,     12.000, 0x00000000, 0 },
  { 4276,   915,  3, 199111,     16.000, 0x00000000, 0 },
  { 4277,   915,  4, 199111,     20.000, 0x00000000, 0 },
  { 4278,   915,  5, 199111,     24.000, 0x00000000, 0 },
  { 4279,   916,  0, 199112,      4.000, 0x00000000, 0 },
  { 4280,   916,  1, 199112,      8.000, 0x00000000, 0 },
  { 4281,   916,  2, 199112,     12.000, 0x00000000, 0 },
  { 4282,   916,  3, 199112,     16.000, 0x00000000, 0 },
  { 4283,   916,  4, 199112,     20.000, 0x00000000, 0 },
  { 4284,   916,  5, 199112,     24.000, 0x00000000, 0 },
  { 4285,   917,  0, 199120,      2.000, 0x00000000, 0 },
  { 4286,   917,  1, 199120,      4.000, 0x00000000, 0 },
  { 4287,   917,  2, 199120,      6.000, 0x00000000, 0 },
  { 4288,   917,  3, 199120,      8.000, 0x00000000, 0 },
  { 4289,   917,  4, 199120,     10.000, 0x00000000, 0 },
  { 4290,   917,  5, 199120,     12.000, 0x00000000, 0 },
  { 4292,   918,  0, 199152,      2.000, 0x00000000, 0 },
  { 4293,   918,  1, 199152,      4.000, 0x00000000, 0 },
  { 4294,   918,  2, 199152,      6.000, 0x00000000, 0 },
  { 4295,   918,  3, 199152,      8.000, 0x00000000, 0 },
  { 4296,   918,  4, 199152,     10.000, 0x00000000, 0 },
  { 4297,   918,  5, 199152,     12.000, 0x00000000, 0 },
  { 4298,   919,  0, 199153,      5.000, 0x00000000, 0 },
  { 4299,   919,  1, 199153,     10.000, 0x00000000, 0 },
  { 4300,   919,  2, 199153,     15.000, 0x00000000, 0 },
  { 4301,   919,  3, 199153,     20.000, 0x00000000, 0 },
  { 4302,   919,  4, 199153,     25.000, 0x00000000, 0 },
  { 4303,   919,  5, 199153,     30.000, 0x00000000, 0 },
  { 4304,   920,  0, 199158,      6.000, 0x00000000, 0 },
  { 4305,   920,  1, 199158,     12.000, 0x00000000, 0 },
  { 4306,   920,  2, 199158,     20.000, 0x00000000, 0 },
  { 4307,   920,  3, 199158,     26.000, 0x00000000, 0 },
  { 4308,   920,  4, 199158,     32.000, 0x00000000, 0 },
  { 4309,   920,  5, 199158,     40.000, 0x00000000, 0 },
  { 4310,   921,  0, 199163,      1.000, 0x00000000, 0 },
  { 4311,   921,  1, 199163,      2.000, 0x00000000, 0 },
  { 4312,   921,  2, 199163,      3.000, 0x00000000, 0 },
  { 4313,   921,  3, 199163,      4.000, 0x00000000, 0 },
  { 4314,   921,  4, 199163,      5.000, 0x00000000, 0 },
  { 4315,   921,  5, 199163,      6.000, 0x00000000, 0 },
  { 4316,   922,  0, 199212,      1.000, 0x00000000, 0 },
  { 4317,   922,  1, 199212,      2.000, 0x00000000, 0 },
  { 4318,   922,  2, 199212,      3.000, 0x00000000, 0 },
  { 4319,   922,  3, 199212,      4.000, 0x00000000, 0 },
  { 4320,   922,  4, 199212,      5.000, 0x00000000, 0 },
  { 4321,   922,  5, 199212,      6.000, 0x00000000, 0 },
  { 4322,   923,  0, 199214,     10.000, 0x00000000, 0 },
  { 4323,   923,  1, 199214,     20.000, 0x00000000, 0 },
  { 4324,   923,  2, 199214,     30.000, 0x00000000, 0 },
  { 4325,   923,  3, 199214,     40.000, 0x00000000, 0 },
  { 4326,   923,  4, 199214,     50.000, 0x00000000, 0 },
  { 4327,   923,  5, 199214,     60.000, 0x00000000, 0 },
  { 4328,   924,  0, 199220,      0.000, 0x00000000, 0 },
  { 4329,   925,  0, 199257,      0.000, 0x00000000, 0 },
  { 4330,   926,  0, 199282,      0.000, 0x00000000, 0 },
  { 4332,   927,  0, 199471,      0.000, 0x00000000, 0 },
  { 4333,   928,  0, 199472,      0.000, 0x00000000, 0 },
  { 4334,   929,  0, 201424,      0.000, 0x00000000, 0 },
  { 4291,   930,  0, 199107,      0.000, 0x00000000, 0 },
  { 4335,   931,  0, 205406,      0.000, 0x00000000, 0 },
  { 4336,   932,  0, 199887,      0.000, 0x00000000, 0 },
  { 4337,   933,  0, 199665,      0.000, 0x00000000, 0 },
  { 4338,   934,  0, 199640,      0.000, 0x00000000, 0 },
  { 4339,   935,  0, 199401,      0.000, 0x00000000, 0 },
  { 4340,   936,  0, 199563,      0.000, 0x00000000, 0 },
  { 4341,   937,  0, 199573,      0.000, 0x00000000, 0 },
  { 4342,   938,  0, 199364,      0.000, 0x00000000, 0 },
  { 4343,   938,  1, 199364,      6.000, 0x00000000, 0 },
  { 4344,   938,  2, 199364,     10.000, 0x00000000, 0 },
  { 4345,   938,  3, 199364,     13.000, 0x00000000, 0 },
  { 4346,   938,  4, 199364,     16.000, 0x00000000, 0 },
  { 4347,   938,  5, 199364,     20.000, 0x00000000, 0 },
  { 4348,   939,  0, 199365,      0.000, 0x00000000, 0 },
  { 4349,   939,  1, 199365,     -6.000, 0x00000000, 0 },
  { 4350,   939,  2, 199365,    -10.000, 0x00000000, 0 },
  { 4351,   939,  3, 199365,    -13.000, 0x00000000, 0 },
  { 4352,   939,  4, 199365,    -16.000, 0x00000000, 0 },
  { 4353,   939,  5, 199365,    -20.000, 0x00000000, 0 },
  { 4354,   940,  0, 199366,      0.000, 0x00000000, 0 },
  { 4355,   940,  1, 199366,      6.000, 0x00000000, 0 },
  { 4356,   940,  2, 199366,     10.000, 0x00000000, 0 },
  { 4357,   940,  3, 199366,     13.000, 0x00000000, 0 },
  { 4358,   940,  4, 199366,     16.000, 0x00000000, 0 },
  { 4359,   940,  5, 199366,     20.000, 0x00000000, 0 },
  { 4360,   941,  0, 199367,      0.000, 0x00000000, 0 },
  { 4361,   941,  1, 199367,     10.000, 0x00000000, 0 },
  { 4362,   941,  2, 199367,     15.000, 0x00000000, 0 },
  { 4363,   941,  3, 199367,     20.000, 0x00000000, 0 },
  { 4364,   941,  4, 199367,     25.000, 0x00000000, 0 },
  { 4365,   941,  5, 199367,     30.000, 0x00000000, 0 },
  { 4366,   942,  0, 199372,   1000.000, 0x00000000, 0 },
  { 4367,   942,  1, 199372,   2000.000, 0x00000000, 0 },
  { 4368,   942,  2, 199372,   3000.000, 0x00000000, 0 },
  { 4369,   942,  3, 199372,   4000.000, 0x00000000, 0 },
  { 4370,   942,  4, 199372,   5000.000, 0x00000000, 0 },
  { 4371,   942,  5, 199372,   6000.000, 0x00000000, 0 },
  { 4372,   943,  0, 199377,      0.000, 0x00000000, 0 },
  { 4373,   943,  1, 199377,      6.000, 0x00000000, 0 },
  { 4374,   943,  2, 199377,     10.000, 0x00000000, 0 },
  { 4375,   943,  3, 199377,     13.000, 0x00000000, 0 },
  { 4376,   943,  4, 199377,     16.000, 0x00000000, 0 },
  { 4377,   943,  5, 199377,     20.000, 0x00000000, 0 },
  { 4378,   944,  0, 199380,      5.000, 0x00000000, 0 },
  { 4379,   944,  1, 199380,     10.000, 0x00000000, 0 },
  { 4380,   944,  2, 199380,     15.000, 0x00000000, 0 },
  { 4381,   944,  3, 199380,     20.000, 0x00000000, 0 },
  { 4382,   944,  4, 199380,     25.000, 0x00000000, 0 },
  { 4383,   944,  5, 199380,     30.000, 0x00000000, 0 },
  { 4384,   945,  0, 199384,      0.000, 0x00000000, 0 },
  { 4385,   945,  1, 199384,     30.000, 0x00000000, 0 },
  { 4386,   945,  2, 199384,     40.000, 0x00000000, 0 },
  { 4387,   945,  3, 199384,     50.000, 0x00000000, 0 },
  { 4388,   945,  4, 199384,     60.000, 0x00000000, 0 },
  { 4389,   945,  5, 199384,     70.000, 0x00000000, 0 },
  { 4390,   946,  0, 199485,      5.000, 0x00000000, 0 },
  { 5573,   946,  1, 199485,     10.000, 0x00000000, 0 },
  { 5574,   946,  2, 199485,     15.000, 0x00000000, 0 },
  { 5575,   946,  3, 199485,     20.000, 0x00000000, 0 },
  { 5576,   946,  4, 199485,     25.000, 0x00000000, 0 },
  { 5577,   946,  5, 199485,     30.000, 0x00000000, 0 },
  { 4396,   948,  0, 200395,      4.000, 0x00000000, 0 },
  { 4397,   948,  1, 200395,      8.000, 0x00000000, 0 },
  { 4398,   948,  2, 200395,     12.000, 0x00000000, 0 },
  { 4399,   948,  3, 200395,     16.000, 0x00000000, 0 },
  { 4400,   948,  4, 200395,     20.000, 0x00000000, 0 },
  { 4401,   948,  5, 200395,     24.000, 0x00000000, 0 },
  { 4402,   949,  0, 200399,    500.000, 0x00000000, 0 },
  { 4403,   949,  1, 200399,   1000.000, 0x00000000, 0 },
  { 4404,   949,  2, 200399,   1500.000, 0x00000000, 0 },
  { 4405,   949,  3, 200399,   2000.000, 0x00000000, 0 },
  { 4406,   949,  4, 200399,   2500.000, 0x00000000, 0 },
  { 4407,   949,  5, 200399,   3000.000, 0x00000000, 0 },
  { 4408,   950,  0, 200400,      5.000, 0x00000000, 0 },
  { 4409,   950,  1, 200400,     10.000, 0x00000000, 0 },
  { 4410,   950,  2, 200400,     15.000, 0x00000000, 0 },
  { 4411,   950,  3, 200400,     20.000, 0x00000000, 0 },
  { 4412,   950,  4, 200400,     25.000, 0x00000000, 0 },
  { 4413,   950,  5, 200400,     30.000, 0x00000000, 0 },
  { 4414,   951,  0, 200402,     -3.000, 0x00000000, 0 },
  { 4415,   951,  1, 200402,     -6.000, 0x00000000, 0 },
  { 4416,   951,  2, 200402,     -9.000, 0x00000000, 0 },
  { 4417,   951,  3, 200402,    -12.000, 0x00000000, 0 },
  { 4418,   951,  4, 200402,    -15.000, 0x00000000, 0 },
  { 4419,   951,  5, 200402,    -18.000, 0x00000000, 0 },
  { 4420,   952,  0, 200409,     10.000, 0x00000000, 0 },
  { 4421,   952,  1, 200409,     20.000, 0x00000000, 0 },
  { 4422,   952,  2, 200409,     30.000, 0x00000000, 0 },
  { 4423,   952,  3, 200409,     40.000, 0x00000000, 0 },
  { 4424,   952,  4, 200409,     50.000, 0x00000000, 0 },
  { 4425,   952,  5, 200409,     60.000, 0x00000000, 0 },
  { 4426,   953,  0, 200414,      1.000, 0x00000000, 0 },
  { 4427,   953,  1, 200414,      2.000, 0x00000000, 0 },
  { 4428,   953,  2, 200414,      3.000, 0x00000000, 0 },
  { 4429,   953,  3, 200414,      4.000, 0x00000000, 0 },
  { 4430,   953,  4, 200414,      5.000, 0x00000000, 0 },
  { 4431,   953,  5, 200414,      6.000, 0x00000000, 0 },
  { 4432,   954,  0, 200415,     -3.000, 0x00000000, 0 },
  { 4433,   954,  1, 200415,     -6.000, 0x00000000, 0 },
  { 4434,   954,  2, 200415,    -10.000, 0x00000000, 0 },
  { 4435,   954,  3, 200415,    -13.000, 0x00000000, 0 },
  { 4436,   954,  4, 200415,    -16.000, 0x00000000, 0 },
  { 4437,   954,  5, 200415,    -20.000, 0x00000000, 0 },
  { 4641,   955,  0, 208762,      3.000, 0x00000000, 0 },
  { 4969,   955,  1, 208762,      6.000, 0x00000000, 0 },
  { 4970,   955,  2, 208762,      9.000, 0x00000000, 0 },
  { 4971,   955,  3, 208762,     12.000, 0x00000000, 0 },
  { 4972,   955,  4, 208762,     15.000, 0x00000000, 0 },
  { 4973,   955,  5, 208762,     18.000, 0x00000000, 0 },
  { 4438,   956,  0, 200440,      7.000, 0x00000000, 0 },
  { 4439,   956,  1, 200440,     14.000, 0x00000000, 0 },
  { 4440,   956,  2, 200440,     21.000, 0x00000000, 0 },
  { 4441,   956,  3, 200440,     28.000, 0x00000000, 0 },
  { 4442,   956,  4, 200440,     35.000, 0x00000000, 0 },
  { 4443,   956,  5, 200440,     42.000, 0x00000000, 0 },
  { 4444,   957,  0, 215799,      0.000, 0x00000000, 0 },
  { 4506,   958,  0, 200515,      0.000, 0x00000000, 0 },
  { 4576,   959,  0, 214996,      0.000, 0x00000000, 0 },
  { 4509,   960,  0, 200851,      0.000, 0x00000000, 0 },
  { 4510,   961,  0, 200855,      0.000, 0x00000000, 0 },
  { 4511,   962,  0, 200854,      0.000, 0x00000000, 0 },
  { 4445,   963,  0, 200474,      0.000, 0x00000000, 0 },
  { 4446,   964,  0, 200430,      0.000, 0x00000000, 0 },
  { 4447,   965,  0, 200652,      0.000, 0x00000000, 0 },
  { 4448,   966,  0, 200421,      0.000, 0x00000000, 0 },
  { 4449,   967,  0, 200407,      0.000, 0x00000000, 0 },
  { 4450,   968,  0, 200373,      0.000, 0x00000000, 0 },
  { 4451,   969,  0, 200327,    -10.000, 0x00000000, 0 },
  { 4452,   969,  1, 200327,    -20.000, 0x00000000, 0 },
  { 4453,   969,  2, 200327,    -30.000, 0x00000000, 0 },
  { 4454,   969,  3, 200327,    -40.000, 0x00000000, 0 },
  { 4455,   969,  4, 200327,    -50.000, 0x00000000, 0 },
  { 4456,   969,  5, 200327,    -60.000, 0x00000000, 0 },
  { 4457,   970,  0, 200326,     -9.090, 0x00000000, 0 },
  { 4458,   970,  1, 200326,    -16.667, 0x00000000, 0 },
  { 4459,   970,  2, 200326,    -23.070, 0x00000000, 0 },
  { 4460,   970,  3, 200326,    -28.570, 0x00000000, 0 },
  { 4461,   970,  4, 200326,    -33.333, 0x00000000, 0 },
  { 4462,   970,  5, 200326,    -37.500, 0x00000000, 0 },
  { 4463,   971,  0, 200316,      5.000, 0x00000000, 0 },
  { 4464,   971,  1, 200316,     10.000, 0x00000000, 0 },
  { 4465,   971,  2, 200316,     15.000, 0x00000000, 0 },
  { 4466,   971,  3, 200316,     20.000, 0x00000000, 0 },
  { 4467,   971,  4, 200316,     25.000, 0x00000000, 0 },
  { 4468,   971,  5, 200316,     30.000, 0x00000000, 0 },
  { 4469,   972,  0, 200315,     16.000, 0x00000000, 0 },
  { 4470,   972,  1, 200315,     32.000, 0x00000000, 0 },
  { 4471,   972,  2, 200315,     48.000, 0x00000000, 0 },
  { 4472,   972,  3, 200315,     64.000, 0x00000000, 0 },
  { 4473,   972,  4, 200315,     80.000, 0x00000000, 0 },
  { 4474,   972,  5, 200315,     96.000, 0x00000000, 0 },
  { 4475,   973,  0, 200311,      0.000, 0x00000000, 0 },
  { 4481,   974,  0, 200302,      0.000, 0x00000000, 0 },
  { 4482,   974,  1, 200302,     -6.000, 0x00000000, 0 },
  { 4483,   974,  2, 200302,    -10.000, 0x00000000, 0 },
  { 4484,   974,  3, 200302,    -13.000, 0x00000000, 0 },
  { 4485,   974,  4, 200302,    -16.000, 0x00000000, 0 },
  { 4486,   974,  5, 200302,    -20.000, 0x00000000, 0 },
  { 4487,   975,  0, 200298,      0.000, 0x00000000, 0 },
  { 4488,   975,  1, 200298,    -10.000, 0x00000000, 0 },
  { 4489,   975,  2, 200298,    -15.000, 0x00000000, 0 },
  { 4490,   975,  3, 200298,    -20.000, 0x00000000, 0 },
  { 4491,   975,  4, 200298,    -25.000, 0x00000000, 0 },
  { 4492,   975,  5, 200298,    -30.000, 0x00000000, 0 },
  { 4493,   976,  0, 200296,      0.000, 0x00000000, 0 },
  { 4494,   976,  1, 200296,      6.000, 0x00000000, 0 },
  { 4495,   976,  2, 200296,     10.000, 0x00000000, 0 },
  { 4496,   976,  3, 200296,     13.000, 0x00000000, 0 },
  { 4497,   976,  4, 200296,     16.000, 0x00000000, 0 },
  { 4498,   976,  5, 200296,     20.000, 0x00000000, 0 },
  { 4499,   977,  0, 200294,      0.000, 0x00000000, 0 },
  { 4500,   977,  1, 200294,      6.000, 0x00000000, 0 },
  { 4501,   977,  2, 200294,     10.000, 0x00000000, 0 },
  { 4502,   977,  3, 200294,     13.000, 0x00000000, 0 },
  { 4503,   977,  4, 200294,     16.000, 0x00000000, 0 },
  { 4504,   977,  5, 200294,     20.000, 0x00000000, 0 },
  { 4512,   979,  0, 200850,      0.000, 0x00000000, 0 },
  { 4513,   980,  0, 200875,      0.000, 0x00000000, 0 },
  { 4514,   981,  0, 200845,      0.000, 0x00000000, 0 },
  { 4515,   982,  0, 200872,      0.000, 0x00000000, 0 },
  { 4517,   984,  0, 205545,      0.000, 0x00000000, 0 },
  { 4518,   985,  0, 200871,      0.000, 0x00000000, 0 },
  { 4519,   986,  0, 200870,      0.000, 0x00000000, 0 },
  { 4520,   987,  0, 200863,      0.000, 0x00000000, 0 },
  { 4521,   988,  0, 200846,      0.000, 0x00000000, 0 },
  { 4522,   988,  1, 200846,      6.000, 0x00000000, 0 },
  { 4523,   988,  2, 200846,     10.000, 0x00000000, 0 },
  { 4524,   988,  3, 200846,     13.000, 0x00000000, 0 },
  { 4525,   988,  4, 200846,     16.000, 0x00000000, 0 },
  { 4526,   988,  5, 200846,     20.000, 0x00000000, 0 },
  { 4527,   989,  0, 216273,      7.000, 0x00000000, 0 },
  { 6508,   989,  1, 216273,     14.000, 0x00000000, 0 },
  { 6509,   989,  2, 216273,     21.000, 0x00000000, 0 },
  { 6510,   989,  3, 216273,     28.000, 0x00000000, 0 },
  { 6511,   989,  4, 216273,     35.000, 0x00000000, 0 },
  { 6512,   989,  5, 216273,     42.000, 0x00000000, 0 },
  { 4533,   990,  0, 200849,      5.000, 0x00000000, 0 },
  { 4534,   990,  1, 200849,     10.000, 0x00000000, 0 },
  { 4535,   990,  2, 200849,     15.000, 0x00000000, 0 },
  { 4536,   990,  3, 200849,     20.000, 0x00000000, 0 },
  { 4537,   990,  4, 200849,     25.000, 0x00000000, 0 },
  { 4538,   990,  5, 200849,     30.000, 0x00000000, 0 },
  { 4539,   991,  0, 200853,      0.000, 0x00000000, 0 },
  { 4540,   991,  1, 200853,      6.000, 0x00000000, 0 },
  { 4541,   991,  2, 200853,      9.000, 0x00000000, 0 },
  { 4542,   991,  3, 200853,     12.000, 0x00000000, 0 },
  { 4543,   991,  4, 200853,     15.000, 0x00000000, 0 },
  { 4544,   991,  5, 200853,     18.000, 0x00000000, 0 },
  { 4545,   992,  0, 200856,      0.000, 0x00000000, 0 },
  { 4546,   992,  1, 200856,    100.000, 0x00000000, 0 },
  { 4547,   992,  2, 200856,    150.000, 0x00000000, 0 },
  { 4548,   992,  3, 200856,    200.000, 0x00000000, 0 },
  { 4549,   992,  4, 200856,    250.000, 0x00000000, 0 },
  { 4550,   992,  5, 200856,    300.000, 0x00000000, 0 },
  { 4551,   993,  0, 200857,      2.000, 0x00000000, 0 },
  { 4552,   993,  1, 200857,      4.000, 0x00000000, 0 },
  { 4553,   993,  2, 200857,      6.000, 0x00000000, 0 },
  { 4554,   993,  3, 200857,      8.000, 0x00000000, 0 },
  { 4555,   993,  4, 200857,     10.000, 0x00000000, 0 },
  { 4556,   993,  5, 200857,     12.000, 0x00000000, 0 },
  { 4557,   994,  0, 200859,      0.000, 0x00000000, 0 },
  { 4558,   994,  1, 200859,      2.000, 0x00000000, 0 },
  { 4559,   994,  2, 200859,      3.000, 0x00000000, 0 },
  { 4560,   994,  3, 200859,      4.000, 0x00000000, 0 },
  { 4561,   994,  4, 200859,      5.000, 0x00000000, 0 },
  { 4562,   994,  5, 200859,      6.000, 0x00000000, 0 },
  { 4563,   995,  0, 200860,      5.000, 0x00000000, 0 },
  { 4564,   995,  1, 200860,     10.000, 0x00000000, 0 },
  { 4565,   995,  2, 200860,     15.000, 0x00000000, 0 },
  { 4566,   995,  3, 200860,     20.000, 0x00000000, 0 },
  { 4567,   995,  4, 200860,     25.000, 0x00000000, 0 },
  { 4568,   995,  5, 200860,     30.000, 0x00000000, 0 },
  { 4569,   996,  0, 200861,      1.000, 0x00000000, 0 },
  { 4570,   996,  1, 200861,      2.000, 0x00000000, 0 },
  { 4571,   996,  2, 200861,      3.000, 0x00000000, 0 },
  { 4572,   996,  3, 200861,      4.000, 0x00000000, 0 },
  { 4573,   996,  4, 200861,      5.000, 0x00000000, 0 },
  { 4574,   996,  5, 200861,      6.000, 0x00000000, 0 },
  { 4640,   999,  0, 216698,      0.000, 0x00000000, 0 },
  { 4578,  1000,  0, 201454,      0.000, 0x00000000, 0 },
  { 4579,  1000,  1, 201454,     20.000, 0x00000000, 0 },
  { 4580,  1000,  2, 201454,     30.000, 0x00000000, 0 },
  { 4581,  1000,  3, 201454,     40.000, 0x00000000, 0 },
  { 4582,  1000,  4, 201454,     50.000, 0x00000000, 0 },
  { 4583,  1000,  5, 201454,     60.000, 0x00000000, 0 },
  { 4584,  1001,  0, 201455,      6.000, 0x00000000, 0 },
  { 4585,  1001,  1, 201455,     12.000, 0x00000000, 0 },
  { 4586,  1001,  2, 201455,     18.000, 0x00000000, 0 },
  { 4587,  1001,  3, 201455,     24.000, 0x00000000, 0 },
  { 4588,  1001,  4, 201455,     30.000, 0x00000000, 0 },
  { 4589,  1001,  5, 201455,     36.000, 0x00000000, 0 },
  { 4590,  1002,  0, 201456,      6.000, 0x00000000, 0 },
  { 4591,  1002,  1, 201456,     12.000, 0x00000000, 0 },
  { 4592,  1002,  2, 201456,     18.000, 0x00000000, 0 },
  { 4593,  1002,  3, 201456,     24.000, 0x00000000, 0 },
  { 4594,  1002,  4, 201456,     30.000, 0x00000000, 0 },
  { 4595,  1002,  5, 201456,     36.000, 0x00000000, 0 },
  { 4596,  1003,  0, 201457,      5.000, 0x00000000, 0 },
  { 4597,  1003,  1, 201457,     10.000, 0x00000000, 0 },
  { 4598,  1003,  2, 201457,     15.000, 0x00000000, 0 },
  { 4599,  1003,  3, 201457,     20.000, 0x00000000, 0 },
  { 4600,  1003,  4, 201457,     25.000, 0x00000000, 0 },
  { 4601,  1003,  5, 201457,     30.000, 0x00000000, 0 },
  { 4602,  1004,  0, 201458,      6.000, 0x00000000, 0 },
  { 4603,  1004,  1, 201458,     12.000, 0x00000000, 0 },
  { 4604,  1004,  2, 201458,     18.000, 0x00000000, 0 },
  { 4605,  1004,  3, 201458,     24.000, 0x00000000, 0 },
  { 4606,  1004,  4, 201458,     30.000, 0x00000000, 0 },
  { 4607,  1004,  5, 201458,     36.000, 0x00000000, 0 },
  { 4608,  1005,  0, 201459,      0.000, 0x00000000, 0 },
  { 4609,  1005,  1, 201459,     -8.000, 0x00000000, 0 },
  { 4610,  1005,  2, 201459,    -12.000, 0x00000000, 0 },
  { 4611,  1005,  3, 201459,    -16.000, 0x00000000, 0 },
  { 4612,  1005,  4, 201459,    -20.000, 0x00000000, 0 },
  { 4613,  1005,  5, 201459,    -24.000, 0x00000000, 0 },
  { 4614,  1006,  0, 201460,      0.000, 0x00000000, 0 },
  { 4615,  1006,  1, 201460, -40000.000, 0x00000000, 0 },
  { 4616,  1006,  2, 201460, -60000.000, 0x00000000, 0 },
  { 4617,  1006,  3, 201460, -80000.000, 0x00000000, 0 },
  { 4618,  1006,  4, 201460, -100000.000, 0x00000000, 0 },
  { 4619,  1006,  5, 201460, -120000.000, 0x00000000, 0 },
  { 4620,  1007,  0, 201463,      0.000, 0x00000000, 0 },
  { 4621,  1007,  1, 201463,     10.000, 0x00000000, 0 },
  { 4622,  1007,  2, 201463,     15.000, 0x00000000, 0 },
  { 4623,  1007,  3, 201463,     20.000, 0x00000000, 0 },
  { 4624,  1007,  4, 201463,     25.000, 0x00000000, 0 },
  { 4625,  1007,  5, 201463,     30.000, 0x00000000, 0 },
  { 4626,  1008,  0, 201464,      0.000, 0x00000000, 0 },
  { 4627,  1008,  1, 201464,     20.000, 0x00000000, 0 },
  { 4628,  1008,  2, 201464,     30.000, 0x00000000, 0 },
  { 4629,  1008,  3, 201464,     40.000, 0x00000000, 0 },
  { 4630,  1008,  4, 201464,     50.000, 0x00000000, 0 },
  { 4631,  1008,  5, 201464,     60.000, 0x00000000, 0 },
  { 4633,  1010,  0, 201467,      0.000, 0x00000000, 0 },
  { 4634,  1011,  0, 201468,      0.000, 0x00000000, 0 },
  { 4635,  1012,  0, 201469,      0.000, 0x00000000, 0 },
  { 4636,  1013,  0, 201470,      0.000, 0x00000000, 0 },
  { 4637,  1014,  0, 201471,      0.000, 0x00000000, 0 },
  { 4638,  1015,  0, 201472,      0.000, 0x00000000, 0 },
  { 4639,  1016,  0, 201473,      0.000, 0x00000000, 0 },
  { 4671,  1034,  0, 202302,     25.000, 0x00000000, 0 },
  { 4672,  1034,  1, 202302,     50.000, 0x00000000, 0 },
  { 4673,  1034,  2, 202302,     75.000, 0x00000000, 0 },
  { 4674,  1034,  3, 202302,    100.000, 0x00000000, 0 },
  { 4675,  1034,  4, 202302,    125.000, 0x00000000, 0 },
  { 4676,  1034,  5, 202302,    150.000, 0x00000000, 0 },
  { 4786,  1035,  0, 203018,      5.000, 0x00000000, 0 },
  { 4959,  1035,  1, 203018,     10.000, 0x00000000, 0 },
  { 4960,  1035,  2, 203018,     15.000, 0x00000000, 0 },
  { 4961,  1035,  3, 203018,     20.000, 0x00000000, 0 },
  { 4962,  1035,  4, 203018,     25.000, 0x00000000, 0 },
  { 4963,  1035,  5, 203018,     30.000, 0x00000000, 0 },
  { 4677,  1036,  0, 202384,      5.000, 0x00000000, 0 },
  { 4678,  1036,  1, 202384,     10.000, 0x00000000, 0 },
  { 4679,  1036,  2, 202384,     15.000, 0x00000000, 0 },
  { 4680,  1036,  3, 202384,     20.000, 0x00000000, 0 },
  { 4681,  1036,  4, 202384,     25.000, 0x00000000, 0 },
  { 4682,  1036,  5, 202384,     30.000, 0x00000000, 0 },
  { 4683,  1037,  0, 202386,      7.000, 0x00000000, 0 },
  { 4684,  1037,  1, 202386,     14.000, 0x00000000, 0 },
  { 4685,  1037,  2, 202386,     21.000, 0x00000000, 0 },
  { 4686,  1037,  3, 202386,     28.000, 0x00000000, 0 },
  { 4687,  1037,  4, 202386,     35.000, 0x00000000, 0 },
  { 4688,  1037,  5, 202386,     42.000, 0x00000000, 0 },
  { 4689,  1038,  0, 202426,      5.000, 0x00000000, 0 },
  { 4690,  1038,  1, 202426,     10.000, 0x00000000, 0 },
  { 4691,  1038,  2, 202426,     15.000, 0x00000000, 0 },
  { 4692,  1038,  3, 202426,     20.000, 0x00000000, 0 },
  { 4693,  1038,  4, 202426,     25.000, 0x00000000, 0 },
  { 4694,  1038,  5, 202426,     30.000, 0x00000000, 0 },
  { 4695,  1039,  0, 202433,      6.000, 0x00000000, 0 },
  { 4696,  1039,  1, 202433,     12.000, 0x00000000, 0 },
  { 4697,  1039,  2, 202433,     18.000, 0x00000000, 0 },
  { 4698,  1039,  3, 202433,     24.000, 0x00000000, 0 },
  { 4699,  1039,  4, 202433,     30.000, 0x00000000, 0 },
  { 4700,  1039,  5, 202433,     36.000, 0x00000000, 0 },
  { 4701,  1040,  0, 202445,     10.000, 0x00000000, 0 },
  { 4702,  1040,  1, 202445,     20.000, 0x00000000, 0 },
  { 4703,  1040,  2, 202445,     30.000, 0x00000000, 0 },
  { 4704,  1040,  3, 202445,     40.000, 0x00000000, 0 },
  { 4705,  1040,  4, 202445,     50.000, 0x00000000, 0 },
  { 4706,  1040,  5, 202445,     60.000, 0x00000000, 0 },
  { 4713,  1041,  0, 202466,      7.000, 0x00000000, 0 },
  { 4714,  1041,  1, 202466,     14.000, 0x00000000, 0 },
  { 4715,  1041,  2, 202466,     21.000, 0x00000000, 0 },
  { 4716,  1041,  3, 202466,     28.000, 0x00000000, 0 },
  { 4717,  1041,  4, 202466,     35.000, 0x00000000, 0 },
  { 4718,  1041,  5, 202466,     42.000, 0x00000000, 0 },
  { 4707,  1042,  0, 202464,      3.000, 0x00000000, 0 },
  { 4708,  1042,  1, 202464,      6.000, 0x00000000, 0 },
  { 4709,  1042,  2, 202464,     10.000, 0x00000000, 0 },
  { 4710,  1042,  3, 202464,     13.000, 0x00000000, 0 },
  { 4711,  1042,  4, 202464,     16.000, 0x00000000, 0 },
  { 4712,  1042,  5, 202464,     20.000, 0x00000000, 0 },
  { 4784,  1043,  0, 202890,      0.000, 0x00000000, 0 },
  { 4783,  1044,  0, 202918,      0.000, 0x00000000, 0 },
  { 4785,  1045,  0, 213682,      0.000, 0x00000000, 0 },
  { 4788,  1046,  0, 202940,      0.000, 0x00000000, 0 },
  { 4787,  1047,  0, 202996,      0.000, 0x00000000, 0 },
  { 4789,  1048,  0, 214508,      0.000, 0x00000000, 0 },
  { 4719,  1049,  0, 202767,      0.000, 0x00000000, 0 },
  { 4722,  1052,  0, 202665,      0.000, 0x00000000, 0 },
  { 4723,  1053,  0, 202897,      0.000, 0x00000000, 0 },
  { 4724,  1054,  0, 202820,      0.000, 0x00000000, 0 },
  { 4725,  1055,  0, 202769,      0.000, 0x00000000, 0 },
  { 4726,  1056,  0, 202755,      0.000, 0x00000000, 0 },
  { 4727,  1057,  0, 202753,      0.000, 0x00000000, 0 },
  { 4728,  1058,  0, 202628,      0.000, 0x00000000, 0 },
  { 4729,  1059,  0, 216230,      6.000, 0x00000000, 0 },
  { 6458,  1059,  1, 216230,     12.000, 0x00000000, 0 },
  { 6459,  1059,  2, 216230,     18.000, 0x00000000, 0 },
  { 6460,  1059,  3, 216230,     24.000, 0x00000000, 0 },
  { 6461,  1059,  4, 216230,     30.000, 0x00000000, 0 },
  { 6462,  1059,  5, 216230,     36.000, 0x00000000, 0 },
  { 4735,  1060,  0, 202507,      0.000, 0x00000000, 0 },
  { 4736,  1060,  1, 202507,      6.000, 0x00000000, 0 },
  { 4737,  1060,  2, 202507,     10.000, 0x00000000, 0 },
  { 4738,  1060,  3, 202507,     13.000, 0x00000000, 0 },
  { 4739,  1060,  4, 202507,     16.000, 0x00000000, 0 },
  { 4740,  1060,  5, 202507,     20.000, 0x00000000, 0 },
  { 4741,  1061,  0, 202514,      6.000, 0x00000000, 0 },
  { 4742,  1061,  1, 202514,     12.000, 0x00000000, 0 },
  { 4743,  1061,  2, 202514,     18.000, 0x00000000, 0 },
  { 4744,  1061,  3, 202514,     24.000, 0x00000000, 0 },
  { 4745,  1061,  4, 202514,     30.000, 0x00000000, 0 },
  { 4746,  1061,  5, 202514,     36.000, 0x00000000, 0 },
  { 4747,  1062,  0, 202521,     10.000, 0x00000000, 0 },
  { 4748,  1062,  1, 202521,     20.000, 0x00000000, 0 },
  { 4749,  1062,  2, 202521,     30.000, 0x00000000, 0 },
  { 4750,  1062,  3, 202521,     40.000, 0x00000000, 0 },
  { 4751,  1062,  4, 202521,     50.000, 0x00000000, 0 },
  { 4752,  1062,  5, 202521,     60.000, 0x00000000, 0 },
  { 4753,  1063,  0, 202522,      6.000, 0x00000000, 0 },
  { 4754,  1063,  1, 202522,     12.000, 0x00000000, 0 },
  { 4755,  1063,  2, 202522,     18.000, 0x00000000, 0 },
  { 4756,  1063,  3, 202522,     24.000, 0x00000000, 0 },
  { 4757,  1063,  4, 202522,     30.000, 0x00000000, 0 },
  { 4758,  1063,  5, 202522,     36.000, 0x00000000, 0 },
  { 4759,  1064,  0, 202524,     -3.000, 0x00000000, 0 },
  { 4760,  1064,  1, 202524,     -6.000, 0x00000000, 0 },
  { 4761,  1064,  2, 202524,     -9.000, 0x00000000, 0 },
  { 4762,  1064,  3, 202524,    -12.000, 0x00000000, 0 },
  { 4763,  1064,  4, 202524,    -15.000, 0x00000000, 0 },
  { 4764,  1064,  5, 202524,    -18.000, 0x00000000, 0 },
  { 4765,  1065,  0, 202530,      0.000, 0x00000000, 0 },
  { 4766,  1065,  1, 202530,      2.000, 0x00000000, 0 },
  { 4767,  1065,  2, 202530,      3.000, 0x00000000, 0 },
  { 4768,  1065,  3, 202530,      4.000, 0x00000000, 0 },
  { 4769,  1065,  4, 202530,      5.000, 0x00000000, 0 },
  { 4770,  1065,  5, 202530,      6.000, 0x00000000, 0 },
  { 4771,  1066,  0, 202533,      0.000, 0x00000000, 0 },
  { 4772,  1066,  1, 202533,      4.000, 0x00000000, 0 },
  { 4773,  1066,  2, 202533,      6.000, 0x00000000, 0 },
  { 4774,  1066,  3, 202533,      8.000, 0x00000000, 0 },
  { 4775,  1066,  4, 202533,     10.000, 0x00000000, 0 },
  { 4776,  1066,  5, 202533,     12.000, 0x00000000, 0 },
  { 4777,  1067,  0, 202907, -10000.000, 0x00000000, 0 },
  { 4778,  1067,  1, 202907, -18000.000, 0x00000000, 0 },
  { 4779,  1067,  2, 202907, -25000.000, 0x00000000, 0 },
  { 4780,  1067,  3, 202907, -31000.000, 0x00000000, 0 },
  { 4781,  1067,  4, 202907, -37000.000, 0x00000000, 0 },
  { 4782,  1067,  5, 202907, -42000.000, 0x00000000, 0 },
  { 4790,  1068,  0, 203415,      0.000, 0x00000000, 0 },
  { 4792,  1070,  0, 203566,      3.000, 0x00000000, 0 },
  { 4840,  1070,  1, 203566,      6.000, 0x00000000, 0 },
  { 4841,  1070,  2, 203566,     10.000, 0x00000000, 0 },
  { 4842,  1070,  3, 203566,     13.000, 0x00000000, 0 },
  { 4843,  1070,  4, 203566,     16.000, 0x00000000, 0 },
  { 4844,  1070,  5, 203566,     20.000, 0x00000000, 0 },
  { 4793,  1071,  0, 203577,      3.000, 0x00000000, 0 },
  { 4845,  1071,  1, 203577,      6.000, 0x00000000, 0 },
  { 4846,  1071,  2, 203577,     10.000, 0x00000000, 0 },
  { 4847,  1071,  3, 203577,     13.000, 0x00000000, 0 },
  { 4848,  1071,  4, 203577,     16.000, 0x00000000, 0 },
  { 4849,  1071,  5, 203577,     20.000, 0x00000000, 0 },
  { 4794,  1072,  0, 203578,      3.000, 0x00000000, 0 },
  { 4850,  1072,  1, 203578,      6.000, 0x00000000, 0 },
  { 4851,  1072,  2, 203578,     10.000, 0x00000000, 0 },
  { 4852,  1072,  3, 203578,     13.000, 0x00000000, 0 },
  { 4853,  1072,  4, 203578,     16.000, 0x00000000, 0 },
  { 4854,  1072,  5, 203578,     20.000, 0x00000000, 0 },
  { 4800,  1073,  0, 203638,      5.000, 0x00000000, 0 },
  { 4855,  1073,  1, 203638,     10.000, 0x00000000, 0 },
  { 4856,  1073,  2, 203638,     15.000, 0x00000000, 0 },
  { 4857,  1073,  3, 203638,     20.000, 0x00000000, 0 },
  { 4858,  1073,  4, 203638,     25.000, 0x00000000, 0 },
  { 4859,  1073,  5, 203638,     30.000, 0x00000000, 0 },
  { 4811,  1074,  0, 203669,      5.000, 0x00000000, 0 },
  { 4964,  1074,  1, 203669,     10.000, 0x00000000, 0 },
  { 4965,  1074,  2, 203669,     15.000, 0x00000000, 0 },
  { 4966,  1074,  3, 203669,     20.000, 0x00000000, 0 },
  { 4967,  1074,  4, 203669,     25.000, 0x00000000, 0 },
  { 4968,  1074,  5, 203669,     30.000, 0x00000000, 0 },
  { 4812,  1075,  0, 203670,      3.000, 0x00000000, 0 },
  { 4860,  1075,  1, 203670,      6.000, 0x00000000, 0 },
  { 4861,  1075,  2, 203670,     10.000, 0x00000000, 0 },
  { 4862,  1075,  3, 203670,     13.000, 0x00000000, 0 },
  { 4863,  1075,  4, 203670,     16.000, 0x00000000, 0 },
  { 4864,  1075,  5, 203670,     20.000, 0x00000000, 0 },
  { 4813,  1076,  0, 203673,      3.000, 0x00000000, 0 },
  { 4838,  1076,  1, 203673,      6.000, 0x00000000, 0 },
  { 4839,  1076,  2, 203673,     10.000, 0x00000000, 0 },
  { 4865,  1076,  3, 203673,     13.000, 0x00000000, 0 },
  { 4866,  1076,  4, 203673,     16.000, 0x00000000, 0 },
  { 4867,  1076,  5, 203673,     20.000, 0x00000000, 0 },
  { 4814,  1077,  0, 224764,     10.000, 0x00000000, 0 },
  { 4868,  1077,  1, 224764,     20.000, 0x00000000, 0 },
  { 4869,  1077,  2, 224764,     30.000, 0x00000000, 0 },
  { 4870,  1077,  3, 224764,     40.000, 0x00000000, 0 },
  { 4871,  1077,  4, 224764,     50.000, 0x00000000, 0 },
  { 4872,  1077,  5, 224764,     60.000, 0x00000000, 0 },
  { 4815,  1078,  0, 203749,      5.000, 0x00000000, 0 },
  { 4873,  1078,  1, 203749,     10.000, 0x00000000, 0 },
  { 4874,  1078,  2, 203749,     15.000, 0x00000000, 0 },
  { 4875,  1078,  3, 203749,     20.000, 0x00000000, 0 },
  { 4876,  1078,  4, 203749,     25.000, 0x00000000, 0 },
  { 4877,  1078,  5, 203749,     30.000, 0x00000000, 0 },
  { 4816,  1079,  0, 225092,      0.000, 0x00000000, 0 },
  { 4817,  1080,  0, 203752,      0.000, 0x00000000, 0 },
  { 4818,  1081,  0, 203754,      0.000, 0x00000000, 0 },
  { 4819,  1082,  0, 203755,      0.000, 0x00000000, 0 },
  { 4820,  1083,  0, 203563,      0.000, 0x00000000, 0 },
  { 4821,  1084,  0, 203757,      0.000, 0x00000000, 0 },
  { 4823,  1090,  0, 204875,      3.000, 0x00000000, 0 },
  { 4824,  1090,  1, 204875,      6.000, 0x00000000, 0 },
  { 4825,  1090,  2, 204875,     10.000, 0x00000000, 0 },
  { 4826,  1090,  3, 204875,     13.000, 0x00000000, 0 },
  { 4827,  1090,  4, 204875,     16.000, 0x00000000, 0 },
  { 4828,  1090,  5, 204875,     20.000, 0x00000000, 0 },
  { 4830,  1091,  0, 205209,      0.000, 0x00000000, 0 },
  { 4829,  1092,  0, 189184,      0.000, 0x00000000, 0 },
  { 4832,  1094,  0, 195267,      0.000, 0x00000000, 0 },
  { 4833,  1094,  1, 195267,      4.000, 0x00000000, 0 },
  { 4834,  1094,  2, 195267,      6.000, 0x00000000, 0 },
  { 4835,  1094,  3, 195267,      8.000, 0x00000000, 0 },
  { 4836,  1094,  4, 195267,     10.000, 0x00000000, 0 },
  { 4837,  1094,  5, 195267,     12.000, 0x00000000, 0 },
  { 4882,  1095,  0, 206910,      0.000, 0x00000000, 0 },
  { 4883,  1095,  1, 206910,      4.000, 0x00000000, 0 },
  { 4884,  1095,  2, 206910,      6.000, 0x00000000, 0 },
  { 4885,  1095,  3, 206910,      8.000, 0x00000000, 0 },
  { 4886,  1095,  4, 206910,     10.000, 0x00000000, 0 },
  { 4887,  1095,  5, 206910,     12.000, 0x00000000, 0 },
  { 4888,  1096,  0, 207407,      0.000, 0x00000000, 0 },
  { 4889,  1097,  0, 212894,      0.000, 0x00000000, 0 },
  { 4890,  1098,  0, 207387,      0.000, 0x00000000, 0 },
  { 4891,  1099,  0, 207343,      0.000, 0x00000000, 0 },
  { 4892,  1099,  1, 207343,      2.000, 0x00000000, 0 },
  { 4893,  1099,  2, 207343,      3.000, 0x00000000, 0 },
  { 4894,  1099,  3, 207343,      4.000, 0x00000000, 0 },
  { 4895,  1099,  4, 207343,      5.000, 0x00000000, 0 },
  { 4896,  1099,  5, 207343,      6.000, 0x00000000, 0 },
  { 4897,  1100,  0, 207347,      3.000, 0x00000000, 0 },
  { 4898,  1100,  1, 207347,      6.000, 0x00000000, 0 },
  { 4899,  1100,  2, 207347,      9.000, 0x00000000, 0 },
  { 4900,  1100,  3, 207347,     12.000, 0x00000000, 0 },
  { 4901,  1100,  4, 207347,     15.000, 0x00000000, 0 },
  { 4902,  1100,  5, 207347,     18.000, 0x00000000, 0 },
  { 4903,  1101,  0, 207352,      3.000, 0x00000000, 0 },
  { 4904,  1101,  1, 207352,      6.000, 0x00000000, 0 },
  { 4905,  1101,  2, 207352,      9.000, 0x00000000, 0 },
  { 4906,  1101,  3, 207352,     12.000, 0x00000000, 0 },
  { 4907,  1101,  4, 207352,     15.000, 0x00000000, 0 },
  { 4908,  1101,  5, 207352,     18.000, 0x00000000, 0 },
  { 4909,  1102,  0, 207778,      0.000, 0x00000000, 0 },
  { 4910,  1103,  0, 207088,     10.000, 0x00000000, 0 },
  { 4925,  1103,  1, 207088,     20.000, 0x00000000, 0 },
  { 4926,  1103,  2, 207088,     30.000, 0x00000000, 0 },
  { 5104,  1103,  3, 207088,     40.000, 0x00000000, 0 },
  { 5105,  1103,  4, 207088,     50.000, 0x00000000, 0 },
  { 5106,  1103,  5, 207088,     60.000, 0x00000000, 0 },
  { 4911,  1104,  0, 207092,      5.000, 0x00000000, 0 },
  { 4927,  1104,  1, 207092,     10.000, 0x00000000, 0 },
  { 4928,  1104,  2, 207092,     15.000, 0x00000000, 0 },
  { 5107,  1104,  3, 207092,     20.000, 0x00000000, 0 },
  { 5108,  1104,  4, 207092,     25.000, 0x00000000, 0 },
  { 5109,  1104,  5, 207092,     30.000, 0x00000000, 0 },
  { 4912,  1105,  0, 207118,      5.000, 0x00000000, 0 },
  { 5110,  1105,  1, 210029,     10.000, 0x00000000, 0 },
  { 5111,  1105,  2, 210030,     15.000, 0x00000000, 0 },
  { 5112,  1105,  3, 210031,     20.000, 0x00000000, 0 },
  { 5113,  1105,  4, 210032,     25.000, 0x00000000, 0 },
  { 5114,  1105,  5, 210033,     30.000, 0x00000000, 0 },
  { 4913,  1106,  0, 207206,     -9.090, 0x00000000, 0 },
  { 4931,  1106,  1, 207206,    -16.667, 0x00000000, 0 },
  { 4932,  1106,  2, 207206,    -23.070, 0x00000000, 0 },
  { 5115,  1106,  3, 207206,    -28.571, 0x00000000, 0 },
  { 5116,  1106,  4, 207206,    -33.333, 0x00000000, 0 },
  { 5117,  1106,  5, 207206,    -37.500, 0x00000000, 0 },
  { 4914,  1107,  0, 207255,      2.000, 0x00000000, 0 },
  { 4933,  1107,  1, 207255,      4.000, 0x00000000, 0 },
  { 4934,  1107,  2, 207255,      6.000, 0x00000000, 0 },
  { 5118,  1107,  3, 207255,      8.000, 0x00000000, 0 },
  { 5119,  1107,  4, 207255,     10.000, 0x00000000, 0 },
  { 5120,  1107,  5, 207255,     12.000, 0x00000000, 0 },
  { 4915,  1108,  0, 207285,     -5.000, 0x00000000, 0 },
  { 4935,  1108,  1, 207285,    -10.000, 0x00000000, 0 },
  { 4936,  1108,  2, 207285,    -15.000, 0x00000000, 0 },
  { 5121,  1108,  3, 207285,    -20.000, 0x00000000, 0 },
  { 5122,  1108,  4, 207285,    -25.000, 0x00000000, 0 },
  { 5123,  1108,  5, 207285,    -30.000, 0x00000000, 0 },
  { 4916,  1109,  0, 207348,      3.000, 0x00000000, 0 },
  { 4937,  1109,  1, 207348,      6.000, 0x00000000, 0 },
  { 4938,  1109,  2, 207348,     10.000, 0x00000000, 0 },
  { 5124,  1109,  3, 207348,     13.000, 0x00000000, 0 },
  { 5125,  1109,  4, 207348,     16.000, 0x00000000, 0 },
  { 5126,  1109,  5, 207348,     20.000, 0x00000000, 0 },
  { 4917,  1110,  0, 207351,      1.000, 0x00000000, 0 },
  { 4939,  1110,  1, 207351,      2.000, 0x00000000, 0 },
  { 4940,  1110,  2, 207351,      3.000, 0x00000000, 0 },
  { 5127,  1110,  3, 207351,      4.000, 0x00000000, 0 },
  { 5128,  1110,  4, 207351,      5.000, 0x00000000, 0 },
  { 5129,  1110,  5, 207351,      6.000, 0x00000000, 0 },
  { 4918,  1111,  0, 207354,     -3.000, 0x00000000, 0 },
  { 4941,  1111,  1, 207354,     -6.000, 0x00000000, 0 },
  { 4942,  1111,  2, 207354,    -10.000, 0x00000000, 0 },
  { 5130,  1111,  3, 207354,    -13.000, 0x00000000, 0 },
  { 5131,  1111,  4, 207354,    -16.000, 0x00000000, 0 },
  { 5132,  1111,  5, 207354,    -20.000, 0x00000000, 0 },
  { 4922,  1112,  0, 207355,      0.000, 0x00000000, 0 },
  { 4923,  1113,  0, 207356,      0.000, 0x00000000, 0 },
  { 4924,  1114,  0, 207357,      0.000, 0x00000000, 0 },
  { 4919,  1115,  0, 207358,      0.000, 0x00000000, 0 },
  { 4920,  1116,  0, 207360,      0.000, 0x00000000, 0 },
  { 4921,  1117,  0, 207362,      0.000, 0x00000000, 0 },
  { 4943,  1118,  0, 207604,      5.000, 0x00000000, 0 },
  { 4944,  1118,  1, 207604,      5.500, 0x00000000, 0 },
  { 4945,  1118,  2, 207604,      6.000, 0x00000000, 0 },
  { 4946,  1118,  3, 207604,      6.500, 0x00000000, 0 },
  { 4947,  1118,  4, 207604,      7.000, 0x00000000, 0 },
  { 4948,  1118,  5, 207604,      7.500, 0x00000000, 0 },
  { 4949,  1118,  6, 207604,      8.000, 0x00000000, 0 },
  { 4950,  1118,  7, 207604,      8.500, 0x00000000, 0 },
  { 4951,  1118,  8, 207604,      9.000, 0x00000000, 0 },
  { 4952,  1118,  9, 207604,      9.500, 0x00000000, 0 },
  { 5647,  1118, 10, 207604,     10.000, 0x00000000, 0 },
  { 5648,  1118, 11, 207604,     10.500, 0x00000000, 0 },
  { 5649,  1118, 12, 207604,     11.000, 0x00000000, 0 },
  { 5650,  1118, 13, 207604,     11.500, 0x00000000, 0 },
  { 5651,  1118, 14, 207604,     12.000, 0x00000000, 0 },
  { 5652,  1118, 15, 207604,     12.500, 0x00000000, 0 },
  { 5653,  1118, 16, 207604,     13.000, 0x00000000, 0 },
  { 5654,  1118, 17, 207604,     13.500, 0x00000000, 0 },
  { 5655,  1118, 18, 207604,     14.000, 0x00000000, 0 },
  { 5656,  1118, 19, 207604,     14.500, 0x00000000, 0 },
  { 4953,  1119,  0, 208598,   2000.000, 0x00000000, 0 },
  { 4954,  1119,  1, 208598,   4000.000, 0x00000000, 0 },
  { 4955,  1119,  2, 208598,   6000.000, 0x00000000, 0 },
  { 4956,  1119,  3, 208598,   8000.000, 0x00000000, 0 },
  { 4957,  1119,  4, 208598,  10000.000, 0x00000000, 0 },
  { 4958,  1119,  5, 208598,  12000.000, 0x00000000, 0 },
  { 4977,  1120,  0, 209202,      0.000, 0x00000000, 0 },
  { 4978,  1121,  0, 209229,      5.000, 0x00000000, 0 },
  { 4979,  1121,  1, 209229,     10.000, 0x00000000, 0 },
  { 4980,  1121,  2, 209229,     15.000, 0x00000000, 0 },
  { 4981,  1121,  3, 209229,     20.000, 0x00000000, 0 },
  { 4982,  1121,  4, 209229,     25.000, 0x00000000, 0 },
  { 4983,  1121,  5, 209229,     30.000, 0x00000000, 0 },
  { 4984,  1122,  0, 209217,      3.000, 0x00000000, 0 },
  { 4985,  1122,  1, 209217,      6.000, 0x00000000, 0 },
  { 4986,  1122,  2, 209217,      9.000, 0x00000000, 0 },
  { 4987,  1122,  3, 209217,     12.000, 0x00000000, 0 },
  { 4988,  1122,  4, 209217,     15.000, 0x00000000, 0 },
  { 4989,  1122,  5, 209217,     18.000, 0x00000000, 0 },
  { 4990,  1123,  0, 209220, -10000.000, 0x00000000, 0 },
  { 5008,  1123,  1, 209220, -20000.000, 0x00000000, 0 },
  { 5009,  1123,  2, 209220, -30000.000, 0x00000000, 0 },
  { 5010,  1123,  3, 209220, -40000.000, 0x00000000, 0 },
  { 5011,  1123,  4, 209220, -50000.000, 0x00000000, 0 },
  { 5012,  1123,  5, 209220, -60000.000, 0x00000000, 0 },
  { 4996,  1124,  0, 209226,      0.000, 0x00000000, 0 },
  { 5013,  1124,  1, 213570,      0.000, 0x00000000, 0 },
  { 5014,  1124,  2, 213571,      0.000, 0x00000000, 0 },
  { 5015,  1124,  3, 213572,      0.000, 0x00000000, 0 },
  { 5016,  1124,  4, 213573,      0.000, 0x00000000, 0 },
  { 5017,  1124,  5, 213574,      0.000, 0x00000000, 0 },
  { 4997,  1125,  0, 209223,      6.000, 0x00000000, 0 },
  { 5018,  1125,  1, 209223,     12.000, 0x00000000, 0 },
  { 5019,  1125,  2, 209223,     18.000, 0x00000000, 0 },
  { 5020,  1125,  3, 209223,     24.000, 0x00000000, 0 },
  { 5021,  1125,  4, 209223,     30.000, 0x00000000, 0 },
  { 5022,  1125,  5, 209223,     36.000, 0x00000000, 0 },
  { 4998,  1126,  0, 209218,   1000.000, 0x00000000, 0 },
  { 5023,  1126,  1, 209218,   2000.000, 0x00000000, 0 },
  { 5024,  1126,  2, 209218,   3000.000, 0x00000000, 0 },
  { 5025,  1126,  3, 209218,   4000.000, 0x00000000, 0 },
  { 5026,  1126,  4, 209218,   5000.000, 0x00000000, 0 },
  { 5027,  1126,  5, 209218,   6000.000, 0x00000000, 0 },
  { 4999,  1127,  0, 209216,      2.000, 0x00000000, 0 },
  { 5028,  1127,  1, 209216,      4.000, 0x00000000, 0 },
  { 5029,  1127,  2, 209216,      6.000, 0x00000000, 0 },
  { 5030,  1127,  3, 209216,      8.000, 0x00000000, 0 },
  { 5031,  1127,  4, 209216,     10.000, 0x00000000, 0 },
  { 5032,  1127,  5, 209216,     12.000, 0x00000000, 0 },
  { 5000,  1128,  0, 209224,      1.000, 0x00000000, 0 },
  { 5033,  1128,  1, 209224,      2.000, 0x00000000, 0 },
  { 5034,  1128,  2, 209224,      3.000, 0x00000000, 0 },
  { 5035,  1128,  3, 209224,      4.000, 0x00000000, 0 },
  { 5036,  1128,  4, 209224,      5.000, 0x00000000, 0 },
  { 5037,  1128,  5, 209224,      6.000, 0x00000000, 0 },
  { 5310,  1129,  0, 209225,      0.000, 0x00000000, 0 },
  { 5311,  1129,  1, 211912,      0.000, 0x00000000, 0 },
  { 5312,  1129,  2, 211913,      0.000, 0x00000000, 0 },
  { 5313,  1129,  3, 211914,      0.000, 0x00000000, 0 },
  { 5314,  1129,  4, 211915,      0.000, 0x00000000, 0 },
  { 5315,  1129,  5, 211916,      0.000, 0x00000000, 0 },
  { 5002,  1130,  0, 209341,      0.000, 0x00000000, 0 },
  { 5003,  1131,  0, 209285,      0.000, 0x00000000, 0 },
  { 5004,  1132,  0, 209376,      0.000, 0x00000000, 0 },
  { 5005,  1133,  0, 209389,      0.000, 0x00000000, 0 },
  { 5006,  1134,  0, 209539,      0.000, 0x00000000, 0 },
  { 5007,  1135,  0, 209474,      0.000, 0x00000000, 0 },
  { 5043,  1136,  0, 209577,      0.000, 0x00000000, 0 },
  { 5044,  1137,  0, 209574,      0.000, 0x00000000, 0 },
  { 5045,  1138,  0, 209573,      0.000, 0x00000000, 0 },
  { 5046,  1139,  0, 209566,      0.000, 0x00000000, 0 },
  { 5047,  1140,  0, 209559,      0.000, 0x00000000, 0 },
  { 5048,  1141,  0, 209554,      0.000, 0x00000000, 0 },
  { 5049,  1142,  0, 209548,      0.000, 0x00000000, 0 },
  { 5050,  1143,  0, 209459,      0.000, 0x00000000, 0 },
  { 5059,  1143,  1, 209459,    200.000, 0x00000000, 0 },
  { 5060,  1143,  2, 209459,    300.000, 0x00000000, 0 },
  { 5061,  1143,  3, 209459,    400.000, 0x00000000, 0 },
  { 5062,  1143,  4, 209459,    500.000, 0x00000000, 0 },
  { 5063,  1143,  5, 209459,    600.000, 0x00000000, 0 },
  { 5051,  1144,  0, 209462,      0.000, 0x00000000, 0 },
  { 5064,  1144,  1, 209462,      4.000, 0x00000000, 0 },
  { 5065,  1144,  2, 209462,      6.000, 0x00000000, 0 },
  { 5066,  1144,  3, 209462,      8.000, 0x00000000, 0 },
  { 5067,  1144,  4, 209462,     10.000, 0x00000000, 0 },
  { 5068,  1144,  5, 209462,     12.000, 0x00000000, 0 },
  { 5052,  1145,  0, 209472,      0.000, 0x00000000, 0 },
  { 5069,  1145,  1, 209472,     10.000, 0x00000000, 0 },
  { 5070,  1145,  2, 209472,     15.000, 0x00000000, 0 },
  { 5071,  1145,  3, 209472,     20.000, 0x00000000, 0 },
  { 5072,  1145,  4, 209472,     25.000, 0x00000000, 0 },
  { 5073,  1145,  5, 209472,     30.000, 0x00000000, 0 },
  { 5053,  1146,  0, 216274,      0.000, 0x00000000, 0 },
  { 6468,  1146,  1, 216274,     10.000, 0x00000000, 0 },
  { 6469,  1146,  2, 216274,     15.000, 0x00000000, 0 },
  { 6470,  1146,  3, 216274,     20.000, 0x00000000, 0 },
  { 6471,  1146,  4, 216274,     25.000, 0x00000000, 0 },
  { 6472,  1146,  5, 216274,     30.000, 0x00000000, 0 },
  { 5054,  1147,  0, 209481,      0.000, 0x00000000, 0 },
  { 5079,  1147,  1, 209481,     10.000, 0x00000000, 0 },
  { 5080,  1147,  2, 209481,     15.000, 0x00000000, 0 },
  { 5081,  1147,  3, 209481,     20.000, 0x00000000, 0 },
  { 5082,  1147,  4, 209481,     25.000, 0x00000000, 0 },
  { 5083,  1147,  5, 209481,     30.000, 0x00000000, 0 },
  { 5055,  1148,  0, 209483,      0.000, 0x00000000, 0 },
  { 5084,  1148,  1, 209483,     20.000, 0x00000000, 0 },
  { 5085,  1148,  2, 209483,     30.000, 0x00000000, 0 },
  { 5086,  1148,  3, 209483,     40.000, 0x00000000, 0 },
  { 5087,  1148,  4, 209483,     50.000, 0x00000000, 0 },
  { 5088,  1148,  5, 209483,     60.000, 0x00000000, 0 },
  { 5056,  1149,  0, 209492,      0.000, 0x00000000, 0 },
  { 5089,  1149,  1, 209492,    -30.000, 0x00000000, 0 },
  { 5090,  1149,  2, 209492,    -45.000, 0x00000000, 0 },
  { 5091,  1149,  3, 209492,    -60.000, 0x00000000, 0 },
  { 5092,  1149,  4, 209492,    -75.000, 0x00000000, 0 },
  { 5093,  1149,  5, 209492,    -90.000, 0x00000000, 0 },
  { 5057,  1150,  0, 209494,      4.000, 0x00000000, 0 },
  { 5094,  1150,  1, 209494,      8.000, 0x00000000, 0 },
  { 5095,  1150,  2, 209494,     12.000, 0x00000000, 0 },
  { 5096,  1150,  3, 209494,     16.000, 0x00000000, 0 },
  { 5097,  1150,  4, 209494,     20.000, 0x00000000, 0 },
  { 5098,  1150,  5, 209494,     24.000, 0x00000000, 0 },
  { 5058,  1151,  0, 209541,      0.000, 0x00000000, 0 },
  { 5099,  1151,  1, 209541,      6.000, 0x00000000, 0 },
  { 5100,  1151,  2, 209541,      9.000, 0x00000000, 0 },
  { 5101,  1151,  3, 209541,     12.000, 0x00000000, 0 },
  { 5102,  1151,  4, 209541,     15.000, 0x00000000, 0 },
  { 5103,  1151,  5, 209541,     18.000, 0x00000000, 0 },
  { 5141,  1153,  0, 210722,      0.000, 0x00000000, 0 },
  { 5147,  1154,  0, 210702,      0.000, 0x00000000, 0 },
  { 5148,  1155,  0, 210676,      0.000, 0x00000000, 0 },
  { 5149,  1156,  0, 210666,      0.000, 0x00000000, 0 },
  { 5150,  1157,  0, 210663,      0.000, 0x00000000, 0 },
  { 5151,  1158,  0, 210650,      0.000, 0x00000000, 0 },
  { 5152,  1159,  0, 210638,      0.000, 0x00000000, 0 },
  { 5153,  1160,  0, 210557,      0.000, 0x00000000, 0 },
  { 5154,  1160,  1, 210557,   2000.000, 0x00000000, 0 },
  { 5155,  1160,  2, 210557,   3000.000, 0x00000000, 0 },
  { 5156,  1160,  3, 210557,   4000.000, 0x00000000, 0 },
  { 5157,  1160,  4, 210557,   5000.000, 0x00000000, 0 },
  { 5158,  1160,  5, 210557,   6000.000, 0x00000000, 0 },
  { 5159,  1161,  0, 210570,      7.000, 0x00000000, 0 },
  { 5160,  1161,  1, 210570,     14.000, 0x00000000, 0 },
  { 5161,  1161,  2, 210570,     21.000, 0x00000000, 0 },
  { 5162,  1161,  3, 210570,     28.000, 0x00000000, 0 },
  { 5163,  1161,  4, 210570,     35.000, 0x00000000, 0 },
  { 5164,  1161,  5, 210570,     42.000, 0x00000000, 0 },
  { 5165,  1162,  0, 210571,      0.000, 0x00000000, 0 },
  { 5166,  1162,  1, 210571,     10.000, 0x00000000, 0 },
  { 5167,  1162,  2, 210571,     15.000, 0x00000000, 0 },
  { 5168,  1162,  3, 210571,     20.000, 0x00000000, 0 },
  { 5169,  1162,  4, 210571,     25.000, 0x00000000, 0 },
  { 5170,  1162,  5, 210571,     30.000, 0x00000000, 0 },
  { 5171,  1163,  0, 210575,      7.000, 0x00000000, 0 },
  { 5172,  1163,  1, 210575,     14.000, 0x00000000, 0 },
  { 5173,  1163,  2, 210575,     21.000, 0x00000000, 0 },
  { 5174,  1163,  3, 210575,     28.000, 0x00000000, 0 },
  { 5175,  1163,  4, 210575,     35.000, 0x00000000, 0 },
  { 5176,  1163,  5, 210575,     42.000, 0x00000000, 0 },
  { 5177,  1164,  0, 210579,      0.000, 0x00000000, 0 },
  { 5178,  1164,  1, 210579,     10.000, 0x00000000, 0 },
  { 5179,  1164,  2, 210579,     15.000, 0x00000000, 0 },
  { 5180,  1164,  3, 210579,     20.000, 0x00000000, 0 },
  { 5181,  1164,  4, 210579,     25.000, 0x00000000, 0 },
  { 5182,  1164,  5, 210579,     30.000, 0x00000000, 0 },
  { 5183,  1165,  0, 210590,      0.000, 0x00000000, 0 },
  { 5184,  1165,  1, 210590,     30.000, 0x00000000, 0 },
  { 5185,  1165,  2, 210590,     45.000, 0x00000000, 0 },
  { 5186,  1165,  3, 210590,     60.000, 0x00000000, 0 },
  { 5187,  1165,  4, 210590,     75.000, 0x00000000, 0 },
  { 5188,  1165,  5, 210590,     90.000, 0x00000000, 0 },
  { 5189,  1166,  0, 210593,      7.000, 0x00000000, 0 },
  { 5190,  1166,  1, 210593,     14.000, 0x00000000, 0 },
  { 5191,  1166,  2, 210593,     21.000, 0x00000000, 0 },
  { 5192,  1166,  3, 210593,     28.000, 0x00000000, 0 },
  { 5193,  1166,  4, 210593,     35.000, 0x00000000, 0 },
  { 5194,  1166,  5, 210593,     42.000, 0x00000000, 0 },
  { 5195,  1167,  0, 210631,      0.000, 0x00000000, 0 },
  { 5196,  1167,  1, 210631,     10.000, 0x00000000, 0 },
  { 5197,  1167,  2, 210631,     15.000, 0x00000000, 0 },
  { 5198,  1167,  3, 210631,     20.000, 0x00000000, 0 },
  { 5199,  1167,  4, 210631,     25.000, 0x00000000, 0 },
  { 5200,  1167,  5, 210631,     30.000, 0x00000000, 0 },
  { 5201,  1168,  0, 210637,      7.000, 0x00000000, 0 },
  { 5202,  1168,  1, 210637,     14.000, 0x00000000, 0 },
  { 5203,  1168,  2, 210637,     21.000, 0x00000000, 0 },
  { 5204,  1168,  3, 210637,     28.000, 0x00000000, 0 },
  { 5205,  1168,  4, 210637,     35.000, 0x00000000, 0 },
  { 5206,  1168,  5, 210637,     42.000, 0x00000000, 0 },
  { 5207,  1169,  0, 210730,      0.000, 0x00000000, 0 },
  { 5208,  1170,  0, 211714,      0.000, 0x00000000, 0 },
  { 5256,  1171,  0, 211108,      2.000, 0x00000000, 0 },
  { 5257,  1171,  1, 211108,      4.000, 0x00000000, 0 },
  { 5258,  1171,  2, 211108,      6.000, 0x00000000, 0 },
  { 5259,  1171,  3, 211108,      8.000, 0x00000000, 0 },
  { 5260,  1171,  4, 211108,     10.000, 0x00000000, 0 },
  { 5261,  1171,  5, 211108,     12.000, 0x00000000, 0 },
  { 5304,  1172,  0, 211126,     10.000, 0x00000000, 0 },
  { 5305,  1172,  1, 211126,     20.000, 0x00000000, 0 },
  { 5306,  1172,  2, 211126,     30.000, 0x00000000, 0 },
  { 5307,  1172,  3, 211126,     40.000, 0x00000000, 0 },
  { 5308,  1172,  4, 211126,     50.000, 0x00000000, 0 },
  { 5309,  1172,  5, 211126,     60.000, 0x00000000, 0 },
  { 5298,  1173,  0, 211106,     10.000, 0x00000000, 0 },
  { 5299,  1173,  1, 211106,     20.000, 0x00000000, 0 },
  { 5300,  1173,  2, 211106,     30.000, 0x00000000, 0 },
  { 5301,  1173,  3, 211106,     40.000, 0x00000000, 0 },
  { 5302,  1173,  4, 211106,     50.000, 0x00000000, 0 },
  { 5303,  1173,  5, 211106,     60.000, 0x00000000, 0 },
  { 5292,  1174,  0, 211123,      5.000, 0x00000000, 0 },
  { 5293,  1174,  1, 211123,     10.000, 0x00000000, 0 },
  { 5294,  1174,  2, 211123,     15.000, 0x00000000, 0 },
  { 5295,  1174,  3, 211123,     20.000, 0x00000000, 0 },
  { 5296,  1174,  4, 211123,     25.000, 0x00000000, 0 },
  { 5297,  1174,  5, 211123,     30.000, 0x00000000, 0 },
  { 5286,  1175,  0, 211105,      5.000, 0x00000000, 0 },
  { 5287,  1175,  1, 211105,     10.000, 0x00000000, 0 },
  { 5288,  1175,  2, 211105,     15.000, 0x00000000, 0 },
  { 5289,  1175,  3, 211105,     20.000, 0x00000000, 0 },
  { 5290,  1175,  4, 211105,     25.000, 0x00000000, 0 },
  { 5291,  1175,  5, 211105,     30.000, 0x00000000, 0 },
  { 5280,  1176,  0, 211119,     10.000, 0x00000000, 0 },
  { 5281,  1176,  1, 211119,     20.000, 0x00000000, 0 },
  { 5282,  1176,  2, 211119,     30.000, 0x00000000, 0 },
  { 5283,  1176,  3, 211119,     40.000, 0x00000000, 0 },
  { 5284,  1176,  4, 211119,     50.000, 0x00000000, 0 },
  { 5285,  1176,  5, 211119,     60.000, 0x00000000, 0 },
  { 5274,  1177,  0, 211099,      5.000, 0x00000000, 0 },
  { 5275,  1177,  1, 211099,     10.000, 0x00000000, 0 },
  { 5276,  1177,  2, 211099,     15.000, 0x00000000, 0 },
  { 5277,  1177,  3, 211099,     20.000, 0x00000000, 0 },
  { 5278,  1177,  4, 211099,     25.000, 0x00000000, 0 },
  { 5279,  1177,  5, 211099,     30.000, 0x00000000, 0 },
  { 5262,  1178,  0, 211144,     10.000, 0x00000000, 0 },
  { 5263,  1178,  1, 211144,     20.000, 0x00000000, 0 },
  { 5264,  1178,  2, 211144,     30.000, 0x00000000, 0 },
  { 5265,  1178,  3, 211144,     40.000, 0x00000000, 0 },
  { 5266,  1178,  4, 211144,     50.000, 0x00000000, 0 },
  { 5267,  1178,  5, 211144,     60.000, 0x00000000, 0 },
  { 5268,  1179,  0, 211131, -10000.000, 0x00000000, 0 },
  { 5269,  1179,  1, 211131, -20000.000, 0x00000000, 0 },
  { 5270,  1179,  2, 211131, -30000.000, 0x00000000, 0 },
  { 5271,  1179,  3, 211131, -40000.000, 0x00000000, 0 },
  { 5272,  1179,  4, 211131, -50000.000, 0x00000000, 0 },
  { 5273,  1179,  5, 211131, -60000.000, 0x00000000, 0 },
  { 5254,  1180,  0, 211158,      0.000, 0x00000000, 0 },
  { 5255,  1181,  0, 218567,      0.000, 0x00000000, 0 },
  { 5322,  1182,  0, 218572,      0.000, 0x00000000, 0 },
  { 5251,  1183,  0, 211219,      0.000, 0x00000000, 0 },
  { 5252,  1184,  0, 211720,      0.000, 0x00000000, 0 },
  { 5253,  1185,  0, 211530,      0.000, 0x00000000, 0 },
  { 5209,  1186,  0, 200482,      0.000, 0x00000000, 0 },
  { 5210,  1186,  1, 200482,     10.000, 0x00000000, 0 },
  { 5211,  1186,  2, 200482,     15.000, 0x00000000, 0 },
  { 5212,  1186,  3, 200482,     20.000, 0x00000000, 0 },
  { 5213,  1186,  4, 200482,     25.000, 0x00000000, 0 },
  { 5214,  1186,  5, 200482,     30.000, 0x00000000, 0 },
  { 5215,  1187,  0, 211309,      0.000, 0x00000000, 0 },
  { 5216,  1188,  0, 211309,      0.000, 0x00000000, 0 },
  { 5217,  1189,  0, 211309,      0.000, 0x00000000, 0 },
  { 5218,  1190,  0, 211309,      0.000, 0x00000000, 0 },
  { 5219,  1191,  0, 211309,      0.000, 0x00000000, 0 },
  { 5220,  1192,  0, 211309,      0.000, 0x00000000, 0 },
  { 5221,  1193,  0, 211309,      0.000, 0x00000000, 0 },
  { 5222,  1194,  0, 211309,      0.000, 0x00000000, 0 },
  { 5223,  1195,  0, 211309,      0.000, 0x00000000, 0 },
  { 5224,  1196,  0, 211309,      0.000, 0x00000000, 0 },
  { 5225,  1197,  0, 211309,      0.000, 0x00000000, 0 },
  { 5226,  1198,  0, 211309,      0.000, 0x00000000, 0 },
  { 5227,  1199,  0, 211309,      0.000, 0x00000000, 0 },
  { 5228,  1200,  0, 211309,      0.000, 0x00000000, 0 },
  { 5229,  1201,  0, 211309,      0.000, 0x00000000, 0 },
  { 5230,  1202,  0, 211309,      0.000, 0x00000000, 0 },
  { 5231,  1203,  0, 211309,      0.000, 0x00000000, 0 },
  { 5232,  1204,  0, 211309,      0.000, 0x00000000, 0 },
  { 5233,  1205,  0, 211309,      0.000, 0x00000000, 0 },
  { 5234,  1206,  0, 211309,      0.000, 0x00000000, 0 },
  { 5235,  1207,  0, 211309,      0.000, 0x00000000, 0 },
  { 5236,  1208,  0, 211309,      0.000, 0x00000000, 0 },
  { 5237,  1209,  0, 211309,      0.000, 0x00000000, 0 },
  { 5238,  1210,  0, 211309,      0.000, 0x00000000, 0 },
  { 5239,  1211,  0, 211309,      0.000, 0x00000000, 0 },
  { 5240,  1212,  0, 211309,      0.000, 0x00000000, 0 },
  { 5241,  1213,  0, 211309,      0.000, 0x00000000, 0 },
  { 5242,  1214,  0, 211309,      0.000, 0x00000000, 0 },
  { 5243,  1215,  0, 211309,      0.000, 0x00000000, 0 },
  { 5244,  1216,  0, 211309,      0.000, 0x00000000, 0 },
  { 5245,  1217,  0, 211309,      0.000, 0x00000000, 0 },
  { 5246,  1218,  0, 211309,      0.000, 0x00000000, 0 },
  { 5247,  1219,  0, 211309,      0.000, 0x00000000, 0 },
  { 5248,  1220,  0, 211309,      0.000, 0x00000000, 0 },
  { 5249,  1221,  0, 211309,      0.000, 0x00000000, 0 },
  { 5250,  1222,  0, 211309,      0.000, 0x00000000, 0 },
  { 5326,  1228,  0, 212829,      0.000, 0x00000000, 0 },
  { 5327,  1229,  0, 207375,      0.000, 0x00000000, 0 },
  { 5328,  1229,  1, 207375,     10.000, 0x00000000, 0 },
  { 5329,  1229,  2, 207375,     15.000, 0x00000000, 0 },
  { 5330,  1229,  3, 207375,     20.000, 0x00000000, 0 },
  { 5331,  1229,  4, 207375,     25.000, 0x00000000, 0 },
  { 5332,  1229,  5, 207375,     30.000, 0x00000000, 0 },
  { 5333,  1230,  0, 212816,      0.000, 0x00000000, 0 },
  { 5334,  1230,  1, 212816,     10.000, 0x00000000, 0 },
  { 5335,  1230,  2, 212816,     15.000, 0x00000000, 0 },
  { 5336,  1230,  3, 212816,     20.000, 0x00000000, 0 },
  { 5337,  1230,  4, 212816,     25.000, 0x00000000, 0 },
  { 5338,  1230,  5, 212816,     30.000, 0x00000000, 0 },
  { 5339,  1231,  0, 212817,      0.000, 0x00000000, 0 },
  { 5340,  1231,  1, 212817,     20.000, 0x00000000, 0 },
  { 5341,  1231,  2, 212817,     30.000, 0x00000000, 0 },
  { 5342,  1231,  3, 212817,     40.000, 0x00000000, 0 },
  { 5343,  1231,  4, 212817,     50.000, 0x00000000, 0 },
  { 5344,  1231,  5, 212817,     60.000, 0x00000000, 0 },
  { 5345,  1232,  0, 212819,      1.000, 0x00000000, 0 },
  { 5346,  1232,  1, 212819,      2.000, 0x00000000, 0 },
  { 5347,  1232,  2, 212819,      3.000, 0x00000000, 0 },
  { 5348,  1232,  3, 212819,      4.000, 0x00000000, 0 },
  { 5349,  1232,  4, 212819,      5.000, 0x00000000, 0 },
  { 5350,  1232,  5, 212819,      6.000, 0x00000000, 0 },
  { 5351,  1233,  0, 212821,      3.000, 0x00000000, 0 },
  { 5352,  1233,  1, 212821,      6.000, 0x00000000, 0 },
  { 5353,  1233,  2, 212821,      9.000, 0x00000000, 0 },
  { 5354,  1233,  3, 212821,     12.000, 0x00000000, 0 },
  { 5355,  1233,  4, 212821,     15.000, 0x00000000, 0 },
  { 5356,  1233,  5, 212821,     18.000, 0x00000000, 0 },
  { 5357,  1234,  0, 212827,      0.000, 0x00000000, 0 },
  { 5358,  1234,  1, 212827,     10.000, 0x00000000, 0 },
  { 5359,  1234,  2, 212827,     15.000, 0x00000000, 0 },
  { 5360,  1234,  3, 212827,     20.000, 0x00000000, 0 },
  { 5361,  1234,  4, 212827,     25.000, 0x00000000, 0 },
  { 5362,  1234,  5, 212827,     30.000, 0x00000000, 0 },
  { 5363,  1235,  0, 213010,      0.000, 0x00000000, 0 },
  { 5364,  1236,  0, 213017,      0.000, 0x00000000, 0 },
  { 5365,  1237,  0, 193108,      0.000, 0x00000000, 0 },
  { 5366,  1238,  0, 193058,      0.000, 0x00000000, 0 },
  { 5367,  1239,  0, 182234,      0.000, 0x00000000, 0 },
  { 5368,  1240,  0, 185086,      0.000, 0x00000000, 0 },
  { 5369,  1241,  0, 186788,      0.000, 0x00000000, 0 },
  { 5370,  1242,  0, 186945,      0.000, 0x00000000, 0 },
  { 5371,  1242,  1, 186945,   5000.000, 0x00000000, 0 },
  { 5372,  1242,  2, 186945,   7500.000, 0x00000000, 0 },
  { 5373,  1242,  3, 186945,  10000.000, 0x00000000, 0 },
  { 5374,  1242,  4, 186945,  12500.000, 0x00000000, 0 },
  { 5375,  1242,  5, 186945,  15000.000, 0x00000000, 0 },
  { 5376,  1243,  0, 186944,      0.000, 0x00000000, 0 },
  { 5377,  1243,  1, 186944, -20000.000, 0x00000000, 0 },
  { 5378,  1243,  2, 186944, -30000.000, 0x00000000, 0 },
  { 5379,  1243,  3, 186944, -40000.000, 0x00000000, 0 },
  { 5380,  1243,  4, 186944, -50000.000, 0x00000000, 0 },
  { 5381,  1243,  5, 186944, -60000.000, 0x00000000, 0 },
  { 5382,  1244,  0, 185368,      0.000, 0x00000000, 0 },
  { 5383,  1244,  1, 185368,      8.000, 0x00000000, 0 },
  { 5384,  1244,  2, 185368,     12.000, 0x00000000, 0 },
  { 5385,  1244,  3, 185368,     16.000, 0x00000000, 0 },
  { 5386,  1244,  4, 185368,     20.000, 0x00000000, 0 },
  { 5387,  1244,  5, 185368,     24.000, 0x00000000, 0 },
  { 5388,  1245,  0, 186927,      0.000, 0x00000000, 0 },
  { 5389,  1245,  1, 186927,     16.000, 0x00000000, 0 },
  { 5390,  1245,  2, 186927,     24.000, 0x00000000, 0 },
  { 5391,  1245,  3, 186927,     32.000, 0x00000000, 0 },
  { 5392,  1245,  4, 186927,     40.000, 0x00000000, 0 },
  { 5393,  1245,  5, 186927,     48.000, 0x00000000, 0 },
  { 5394,  1246,  0, 186773,      0.000, 0x00000000, 0 },
  { 5395,  1247,  0, 184778,      0.000, 0x00000000, 0 },
  { 5396,  1247,  1, 184778, -10000.000, 0x00000000, 0 },
  { 5397,  1247,  2, 184778, -15000.000, 0x00000000, 0 },
  { 5398,  1247,  3, 184778, -20000.000, 0x00000000, 0 },
  { 5399,  1247,  4, 184778, -25000.000, 0x00000000, 0 },
  { 5400,  1247,  5, 184778, -30000.000, 0x00000000, 0 },
  { 5401,  1248,  0, 186934,      0.000, 0x00000000, 0 },
  { 5402,  1248,  1, 186934,     30.000, 0x00000000, 0 },
  { 5403,  1248,  2, 186934,     45.000, 0x00000000, 0 },
  { 5404,  1248,  3, 186934,     60.000, 0x00000000, 0 },
  { 5405,  1248,  4, 186934,     75.000, 0x00000000, 0 },
  { 5406,  1248,  5, 186934,     90.000, 0x00000000, 0 },
  { 5407,  1249,  0, 184843,      0.000, 0x00000000, 0 },
  { 5408,  1249,  1, 184843,     10.000, 0x00000000, 0 },
  { 5409,  1249,  2, 184843,     15.000, 0x00000000, 0 },
  { 5410,  1249,  3, 184843,     20.000, 0x00000000, 0 },
  { 5411,  1249,  4, 184843,     25.000, 0x00000000, 0 },
  { 5412,  1249,  5, 184843,     30.000, 0x00000000, 0 },
  { 5413,  1250,  0, 184759,      0.000, 0x00000000, 0 },
  { 5414,  1250,  1, 184759,     12.000, 0x00000000, 0 },
  { 5415,  1250,  2, 184759,     18.000, 0x00000000, 0 },
  { 5416,  1250,  3, 184759,     24.000, 0x00000000, 0 },
  { 5417,  1250,  4, 184759,     30.000, 0x00000000, 0 },
  { 5418,  1250,  5, 184759,     36.000, 0x00000000, 0 },
  { 5419,  1251,  0, 186941,      0.000, 0x00000000, 0 },
  { 5420,  1251,  1, 186941,     30.000, 0x00000000, 0 },
  { 5421,  1251,  2, 186941,     45.000, 0x00000000, 0 },
  { 5422,  1251,  3, 186941,     60.000, 0x00000000, 0 },
  { 5423,  1251,  4, 186941,     75.000, 0x00000000, 0 },
  { 5424,  1251,  5, 186941,     90.000, 0x00000000, 0 },
  { 5425,  1252,  0, 205273,      0.000, 0x00000000, 0 },
  { 5426,  1253,  0, 179546,      0.000, 0x00000000, 0 },
  { 5427,  1254,  0, 213428,      0.000, 0x00000000, 0 },
  { 5428,  1255,  0, 213428,      0.000, 0x00000000, 0 },
  { 5429,  1256,  0, 213428,      0.000, 0x00000000, 0 },
  { 5430,  1257,  0, 213428,      0.000, 0x00000000, 0 },
  { 5431,  1258,  0, 213428,      0.000, 0x00000000, 0 },
  { 5499,  1275,  0, 214081,      0.000, 0x00000000, 0 },
  { 5511,  1276,  0, 214368,      0.000, 0x00000000, 0 },
  { 5512,  1277,  0, 214326,      0.000, 0x00000000, 0 },
  { 5513,  1278,  0, 213051,      3.000, 0x00000000, 0 },
  { 5514,  1278,  1, 213051,      6.000, 0x00000000, 0 },
  { 5515,  1278,  2, 213051,      9.000, 0x00000000, 0 },
  { 5516,  1278,  3, 213051,     12.000, 0x00000000, 0 },
  { 5517,  1278,  4, 213051,     15.000, 0x00000000, 0 },
  { 5518,  1278,  5, 213051,     18.000, 0x00000000, 0 },
  { 5519,  1279,  0, 213049,      0.000, 0x00000000, 0 },
  { 5520,  1279,  1, 227683,      0.000, 0x00000000, 0 },
  { 5521,  1279,  2, 227684,      0.000, 0x00000000, 0 },
  { 5522,  1279,  3, 227685,      0.000, 0x00000000, 0 },
  { 5523,  1279,  4, 227686,      0.000, 0x00000000, 0 },
  { 5524,  1279,  5, 227687,      0.000, 0x00000000, 0 },
  { 5525,  1280,  0, 213180,      5.000, 0x00000000, 0 },
  { 5526,  1280,  1, 213180,     10.000, 0x00000000, 0 },
  { 5527,  1280,  2, 213180,     15.000, 0x00000000, 0 },
  { 5528,  1280,  3, 213180,     20.000, 0x00000000, 0 },
  { 5529,  1280,  4, 213180,     25.000, 0x00000000, 0 },
  { 5530,  1280,  5, 213180,     30.000, 0x00000000, 0 },
  { 5531,  1281,  0, 213062,      0.000, 0x00000000, 0 },
  { 5532,  1281,  1, 227688,      0.000, 0x00000000, 0 },
  { 5533,  1281,  2, 227689,      0.000, 0x00000000, 0 },
  { 5534,  1281,  3, 227690,      0.000, 0x00000000, 0 },
  { 5535,  1281,  4, 227691,      0.000, 0x00000000, 0 },
  { 5536,  1281,  5, 227692,      0.000, 0x00000000, 0 },
  { 5537,  1282,  0, 213055,      2.000, 0x00000000, 0 },
  { 5538,  1282,  1, 213055,      4.000, 0x00000000, 0 },
  { 5539,  1282,  2, 213055,      6.000, 0x00000000, 0 },
  { 5540,  1282,  3, 213055,      8.000, 0x00000000, 0 },
  { 5541,  1282,  4, 213055,     10.000, 0x00000000, 0 },
  { 5542,  1282,  5, 213055,     12.000, 0x00000000, 0 },
  { 5543,  1283,  0, 213133,      1.000, 0x00000000, 0 },
  { 5544,  1283,  1, 213133,      2.000, 0x00000000, 0 },
  { 5545,  1283,  2, 213133,      3.000, 0x00000000, 0 },
  { 5546,  1283,  3, 213133,      4.000, 0x00000000, 0 },
  { 5547,  1283,  4, 213133,      5.000, 0x00000000, 0 },
  { 5548,  1283,  5, 213133,      6.000, 0x00000000, 0 },
  { 5549,  1284,  0, 213136,      1.000, 0x00000000, 0 },
  { 5550,  1284,  1, 213136,      2.000, 0x00000000, 0 },
  { 5551,  1284,  2, 213136,      3.000, 0x00000000, 0 },
  { 5552,  1284,  3, 213136,      4.000, 0x00000000, 0 },
  { 5553,  1284,  4, 213136,      5.000, 0x00000000, 0 },
  { 5554,  1284,  5, 213136,      6.000, 0x00000000, 0 },
  { 5555,  1285,  0, 213047,    500.000, 0x00000000, 0 },
  { 5556,  1285,  1, 213047,   1000.000, 0x00000000, 0 },
  { 5557,  1285,  2, 213047,   1500.000, 0x00000000, 0 },
  { 5558,  1285,  3, 213047,   2000.000, 0x00000000, 0 },
  { 5559,  1285,  4, 213047,   2500.000, 0x00000000, 0 },
  { 5560,  1285,  5, 213047,   3000.000, 0x00000000, 0 },
  { 5561,  1286,  0, 213116,     10.000, 0x00000000, 0 },
  { 5562,  1286,  1, 213116,     20.000, 0x00000000, 0 },
  { 5563,  1286,  2, 213116,     30.000, 0x00000000, 0 },
  { 5564,  1286,  3, 213116,     40.000, 0x00000000, 0 },
  { 5565,  1286,  4, 213116,     50.000, 0x00000000, 0 },
  { 5566,  1286,  5, 213116,     60.000, 0x00000000, 0 },
  { 5567,  1287,  0, 213161,      0.000, 0x00000000, 0 },
  { 5571,  1288,  0, 216424,      0.000, 0x00000000, 0 },
  { 5572,  1289,  0, 213050,     10.000, 0x00000000, 0 },
  { 5568,  1290,  0, 213183,      0.000, 0x00000000, 0 },
  { 5570,  1291,  0, 214372,      0.000, 0x00000000, 0 },
  { 5569,  1292,  0, 213340,      0.000, 0x00000000, 0 },
  { 5578,  1293,  0, 214428,      0.000, 0x00000000, 0 },
  { 5579,  1294,  0, 214514,      0.000, 0x00000000, 0 },
  { 5580,  1295,  0, 214516,      0.000, 0x00000000, 0 },
  { 5581,  1296,  0, 214629,      0.000, 0x00000000, 0 },
  { 5613,  1327,  0, 214736,      0.000, 0x00000000, 0 },
  { 5614,  1328,  0, 214744,      0.000, 0x00000000, 0 },
  { 5617,  1330,  0, 214795,      0.000, 0x00000000, 0 },
  { 5619,  1331,  0, 221775,      0.000, 0x00000000, 0 },
  { 5620,  1332,  0, 218931,      0.000, 0x00000000, 0 },
  { 5621,  1333,  0, 218280,      0.000, 0x00000000, 0 },
  { 5622,  1334,  0, 215061,      0.000, 0x00000000, 0 },
  { 5623,  1335,  0, 222644,      0.000, 0x00000000, 0 },
  { 5624,  1336,  0, 215779,      0.000, 0x00000000, 0 },
  { 5625,  1337,  0, 214826,      0.000, 0x00000000, 0 },
  { 5626,  1338,  0, 221773,      0.000, 0x00000000, 0 },
  { 5627,  1339,  0, 187318,      0.000, 0x00000000, 0 },
  { 5628,  1340,  0, 221844,      0.000, 0x00000000, 0 },
  { 5629,  1341,  0, 195265,      0.000, 0x00000000, 0 },
  { 5630,  1342,  0, 222648,      0.000, 0x00000000, 0 },
  { 5631,  1343,  0, 221841,      0.000, 0x00000000, 0 },
  { 5633,  1345,  0, 216122,      0.000, 0x00000000, 0 },
  { 5634,  1346,  0, 222646,      0.000, 0x00000000, 0 },
  { 5635,  1347,  0, 215322,      0.000, 0x00000000, 0 },
  { 5636,  1348,  0, 202463,      0.000, 0x00000000, 0 },
  { 5637,  1349,  0, 221856,      0.000, 0x00000000, 0 },
  { 5638,  1350,  0, 215414,      0.000, 0x00000000, 0 },
  { 5639,  1351,  0, 198228,      0.000, 0x00000000, 0 },
  { 5640,  1352,  0, 224841,      0.000, 0x00000000, 0 },
  { 5641,  1353,  0, 221862,      0.000, 0x00000000, 0 },
  { 5642,  1354,  0, 221882,      0.000, 0x00000000, 0 },
  { 5643,  1355,  0, 215183,      0.000, 0x00000000, 0 },
  { 5644,  1356,  0, 209480,      0.000, 0x00000000, 0 },
  { 5645,  1357,  0, 200847,      0.000, 0x00000000, 0 },
  { 5646,  1358,  0, 188647,      0.000, 0x00000000, 0 },
  { 5657,  1359,  0, 214903,     10.000, 0x00000000, 0 },
  { 5658,  1359,  1, 214903,     11.000, 0x00000000, 0 },
  { 5659,  1359,  2, 214903,     12.000, 0x00000000, 0 },
  { 5660,  1359,  3, 214903,     13.000, 0x00000000, 0 },
  { 5661,  1359,  4, 214903,     14.000, 0x00000000, 0 },
  { 5662,  1359,  5, 214903,     15.000, 0x00000000, 0 },
  { 5663,  1359,  6, 214903,     16.000, 0x00000000, 0 },
  { 5664,  1359,  7, 214903,     17.000, 0x00000000, 0 },
  { 5665,  1359,  8, 214903,     18.000, 0x00000000, 0 },
  { 5666,  1359,  9, 214903,     19.000, 0x00000000, 0 },
  { 5667,  1359, 10, 214903,     20.000, 0x00000000, 0 },
  { 5668,  1359, 11, 214903,     21.000, 0x00000000, 0 },
  { 5669,  1359, 12, 214903,     22.000, 0x00000000, 0 },
  { 5670,  1359, 13, 214903,     23.000, 0x00000000, 0 },
  { 5671,  1359, 14, 214903,     24.000, 0x00000000, 0 },
  { 5672,  1359, 15, 214903,     25.000, 0x00000000, 0 },
  { 5673,  1359, 16, 214903,     26.000, 0x00000000, 0 },
  { 5674,  1359, 17, 214903,     27.000, 0x00000000, 0 },
  { 5675,  1359, 18, 214903,     28.000, 0x00000000, 0 },
  { 5676,  1359, 19, 214903,     29.000, 0x00000000, 0 },
  { 5677,  1360,  0, 214904,      5.000, 0x00000000, 0 },
  { 5678,  1360,  1, 214904,      5.500, 0x00000000, 0 },
  { 5679,  1360,  2, 214904,      6.000, 0x00000000, 0 },
  { 5680,  1360,  3, 214904,      6.500, 0x00000000, 0 },
  { 5681,  1360,  4, 214904,      7.000, 0x00000000, 0 },
  { 5682,  1360,  5, 214904,      7.500, 0x00000000, 0 },
  { 5683,  1360,  6, 214904,      8.000, 0x00000000, 0 },
  { 5684,  1360,  7, 214904,      8.500, 0x00000000, 0 },
  { 5685,  1360,  8, 214904,      9.000, 0x00000000, 0 },
  { 5686,  1360,  9, 214904,      9.500, 0x00000000, 0 },
  { 5687,  1360, 10, 214904,     10.000, 0x00000000, 0 },
  { 5688,  1360, 11, 214904,     10.500, 0x00000000, 0 },
  { 5689,  1360, 12, 214904,     11.000, 0x00000000, 0 },
  { 5690,  1360, 13, 214904,     11.500, 0x00000000, 0 },
  { 5691,  1360, 14, 214904,     12.000, 0x00000000, 0 },
  { 5692,  1360, 15, 214904,     12.500, 0x00000000, 0 },
  { 5693,  1360, 16, 214904,     13.000, 0x00000000, 0 },
  { 5694,  1360, 17, 214904,     13.500, 0x00000000, 0 },
  { 5695,  1360, 18, 214904,     14.000, 0x00000000, 0 },
  { 5696,  1360, 19, 214904,     14.500, 0x00000000, 0 },
  { 5697,  1361,  0, 214906,      5.000, 0x00000000, 0 },
  { 5698,  1361,  1, 214906,      5.500, 0x00000000, 0 },
  { 5699,  1361,  2, 214906,      6.000, 0x00000000, 0 },
  { 5700,  1361,  3, 214906,      6.500, 0x00000000, 0 },
  { 5701,  1361,  4, 214906,      7.000, 0x00000000, 0 },
  { 5702,  1361,  5, 214906,      7.500, 0x00000000, 0 },
  { 5703,  1361,  6, 214906,      8.000, 0x00000000, 0 },
  { 5704,  1361,  7, 214906,      8.500, 0x00000000, 0 },
  { 5705,  1361,  8, 214906,      9.000, 0x00000000, 0 },
  { 5706,  1361,  9, 214906,      9.500, 0x00000000, 0 },
  { 5707,  1361, 10, 214906,     10.000, 0x00000000, 0 },
  { 5708,  1361, 11, 214906,     10.500, 0x00000000, 0 },
  { 5709,  1361, 12, 214906,     11.000, 0x00000000, 0 },
  { 5710,  1361, 13, 214906,     11.500, 0x00000000, 0 },
  { 5711,  1361, 14, 214906,     12.000, 0x00000000, 0 },
  { 5712,  1361, 15, 214906,     12.500, 0x00000000, 0 },
  { 5713,  1361, 16, 214906,     13.000, 0x00000000, 0 },
  { 5714,  1361, 17, 214906,     13.500, 0x00000000, 0 },
  { 5715,  1361, 18, 214906,     14.000, 0x00000000, 0 },
  { 5716,  1361, 19, 214906,     14.500, 0x00000000, 0 },
  { 5717,  1362,  0, 214907,      5.000, 0x00000000, 0 },
  { 5718,  1362,  1, 214907,      5.500, 0x00000000, 0 },
  { 5719,  1362,  2, 214907,      6.000, 0x00000000, 0 },
  { 5720,  1362,  3, 214907,      6.500, 0x00000000, 0 },
  { 5721,  1362,  4, 214907,      7.000, 0x00000000, 0 },
  { 5722,  1362,  5, 214907,      7.500, 0x00000000, 0 },
  { 5723,  1362,  6, 214907,      8.000, 0x00000000, 0 },
  { 5724,  1362,  7, 214907,      8.500, 0x00000000, 0 },
  { 5725,  1362,  8, 214907,      9.000, 0x00000000, 0 },
  { 5726,  1362,  9, 214907,      9.500, 0x00000000, 0 },
  { 5727,  1362, 10, 214907,     10.000, 0x00000000, 0 },
  { 5728,  1362, 11, 214907,     10.500, 0x00000000, 0 },
  { 5729,  1362, 12, 214907,     11.000, 0x00000000, 0 },
  { 5730,  1362, 13, 214907,     11.500, 0x00000000, 0 },
  { 5731,  1362, 14, 214907,     12.000, 0x00000000, 0 },
  { 5732,  1362, 15, 214907,     12.500, 0x00000000, 0 },
  { 5733,  1362, 16, 214907,     13.000, 0x00000000, 0 },
  { 5734,  1362, 17, 214907,     13.500, 0x00000000, 0 },
  { 5735,  1362, 18, 214907,     14.000, 0x00000000, 0 },
  { 5736,  1362, 19, 214907,     14.500, 0x00000000, 0 },
  { 5737,  1363,  0, 214909,     10.000, 0x00000000, 0 },
  { 5738,  1363,  1, 214909,     11.000, 0x00000000, 0 },
  { 5739,  1363,  2, 214909,     12.000, 0x00000000, 0 },
  { 5740,  1363,  3, 214909,     13.000, 0x00000000, 0 },
  { 5741,  1363,  4, 214909,     14.000, 0x00000000, 0 },
  { 5742,  1363,  5, 214909,     15.000, 0x00000000, 0 },
  { 5743,  1363,  6, 214909,     16.000, 0x00000000, 0 },
  { 5744,  1363,  7, 214909,     17.000, 0x00000000, 0 },
  { 5745,  1363,  8, 214909,     18.000, 0x00000000, 0 },
  { 5746,  1363,  9, 214909,     19.000, 0x00000000, 0 },
  { 5747,  1363, 10, 214909,     20.000, 0x00000000, 0 },
  { 5748,  1363, 11, 214909,     21.000, 0x00000000, 0 },
  { 5749,  1363, 12, 214909,     22.000, 0x00000000, 0 },
  { 5750,  1363, 13, 214909,     23.000, 0x00000000, 0 },
  { 5751,  1363, 14, 214909,     24.000, 0x00000000, 0 },
  { 5752,  1363, 15, 214909,     25.000, 0x00000000, 0 },
  { 5753,  1363, 16, 214909,     26.000, 0x00000000, 0 },
  { 5754,  1363, 17, 214909,     27.000, 0x00000000, 0 },
  { 5755,  1363, 18, 214909,     28.000, 0x00000000, 0 },
  { 5756,  1363, 19, 214909,     29.000, 0x00000000, 0 },
  { 5757,  1364,  0, 214910,      5.000, 0x00000000, 0 },
  { 5758,  1364,  1, 214910,      5.500, 0x00000000, 0 },
  { 5759,  1364,  2, 214910,      6.000, 0x00000000, 0 },
  { 5760,  1364,  3, 214910,      6.500, 0x00000000, 0 },
  { 5761,  1364,  4, 214910,      7.000, 0x00000000, 0 },
  { 5762,  1364,  5, 214910,      7.500, 0x00000000, 0 },
  { 5763,  1364,  6, 214910,      8.000, 0x00000000, 0 },
  { 5764,  1364,  7, 214910,      8.500, 0x00000000, 0 },
  { 5765,  1364,  8, 214910,      9.000, 0x00000000, 0 },
  { 5766,  1364,  9, 214910,      9.500, 0x00000000, 0 },
  { 5767,  1364, 10, 214910,     10.000, 0x00000000, 0 },
  { 5768,  1364, 11, 214910,     10.500, 0x00000000, 0 },
  { 5769,  1364, 12, 214910,     11.000, 0x00000000, 0 },
  { 5770,  1364, 13, 214910,     11.500, 0x00000000, 0 },
  { 5771,  1364, 14, 214910,     12.000, 0x00000000, 0 },
  { 5772,  1364, 15, 214910,     12.500, 0x00000000, 0 },
  { 5773,  1364, 16, 214910,     13.000, 0x00000000, 0 },
  { 5774,  1364, 17, 214910,     13.500, 0x00000000, 0 },
  { 5775,  1364, 18, 214910,     14.000, 0x00000000, 0 },
  { 5776,  1364, 19, 214910,     14.500, 0x00000000, 0 },
  { 5777,  1365,  0, 214911,      5.000, 0x00000000, 0 },
  { 5778,  1365,  1, 214911,      5.500, 0x00000000, 0 },
  { 5779,  1365,  2, 214911,      6.000, 0x00000000, 0 },
  { 5780,  1365,  3, 214911,      6.500, 0x00000000, 0 },
  { 5781,  1365,  4, 214911,      7.000, 0x00000000, 0 },
  { 5782,  1365,  5, 214911,      7.500, 0x00000000, 0 },
  { 5783,  1365,  6, 214911,      8.000, 0x00000000, 0 },
  { 5784,  1365,  7, 214911,      8.500, 0x00000000, 0 },
  { 5785,  1365,  8, 214911,      9.000, 0x00000000, 0 },
  { 5786,  1365,  9, 214911,      9.500, 0x00000000, 0 },
  { 5787,  1365, 10, 214911,     10.000, 0x00000000, 0 },
  { 5788,  1365, 11, 214911,     10.500, 0x00000000, 0 },
  { 5789,  1365, 12, 214911,     11.000, 0x00000000, 0 },
  { 5790,  1365, 13, 214911,     11.500, 0x00000000, 0 },
  { 5791,  1365, 14, 214911,     12.000, 0x00000000, 0 },
  { 5792,  1365, 15, 214911,     12.500, 0x00000000, 0 },
  { 5793,  1365, 16, 214911,     13.000, 0x00000000, 0 },
  { 5794,  1365, 17, 214911,     13.500, 0x00000000, 0 },
  { 5795,  1365, 18, 214911,     14.000, 0x00000000, 0 },
  { 5796,  1365, 19, 214911,     14.500, 0x00000000, 0 },
  { 5797,  1366,  0, 214912,     10.000, 0x00000000, 0 },
  { 5798,  1366,  1, 214912,     11.000, 0x00000000, 0 },
  { 5799,  1366,  2, 214912,     12.000, 0x00000000, 0 },
  { 5800,  1366,  3, 214912,     13.000, 0x00000000, 0 },
  { 5801,  1366,  4, 214912,     14.000, 0x00000000, 0 },
  { 5802,  1366,  5, 214912,     15.000, 0x00000000, 0 },
  { 5803,  1366,  6, 214912,     16.000, 0x00000000, 0 },
  { 5804,  1366,  7, 214912,     17.000, 0x00000000, 0 },
  { 5805,  1366,  8, 214912,     18.000, 0x00000000, 0 },
  { 5806,  1366,  9, 214912,     19.000, 0x00000000, 0 },
  { 5807,  1366, 10, 214912,     20.000, 0x00000000, 0 },
  { 5808,  1366, 11, 214912,     21.000, 0x00000000, 0 },
  { 5809,  1366, 12, 214912,     22.000, 0x00000000, 0 },
  { 5810,  1366, 13, 214912,     23.000, 0x00000000, 0 },
  { 5811,  1366, 14, 214912,     24.000, 0x00000000, 0 },
  { 5812,  1366, 15, 214912,     25.000, 0x00000000, 0 },
  { 5813,  1366, 16, 214912,     26.000, 0x00000000, 0 },
  { 5814,  1366, 17, 214912,     27.000, 0x00000000, 0 },
  { 5815,  1366, 18, 214912,     28.000, 0x00000000, 0 },
  { 5816,  1366, 19, 214912,     29.000, 0x00000000, 0 },
  { 5817,  1367,  0, 214913,      5.000, 0x00000000, 0 },
  { 5818,  1367,  1, 214913,      5.500, 0x00000000, 0 },
  { 5819,  1367,  2, 214913,      6.000, 0x00000000, 0 },
  { 5820,  1367,  3, 214913,      6.500, 0x00000000, 0 },
  { 5821,  1367,  4, 214913,      7.000, 0x00000000, 0 },
  { 5822,  1367,  5, 214913,      7.500, 0x00000000, 0 },
  { 5823,  1367,  6, 214913,      8.000, 0x00000000, 0 },
  { 5824,  1367,  7, 214913,      8.500, 0x00000000, 0 },
  { 5825,  1367,  8, 214913,      9.000, 0x00000000, 0 },
  { 5826,  1367,  9, 214913,      9.500, 0x00000000, 0 },
  { 5827,  1367, 10, 214913,     10.000, 0x00000000, 0 },
  { 5828,  1367, 11, 214913,     10.500, 0x00000000, 0 },
  { 5829,  1367, 12, 214913,     11.000, 0x00000000, 0 },
  { 5830,  1367, 13, 214913,     11.500, 0x00000000, 0 },
  { 5831,  1367, 14, 214913,     12.000, 0x00000000, 0 },
  { 5832,  1367, 15, 214913,     12.500, 0x00000000, 0 },
  { 5833,  1367, 16, 214913,     13.000, 0x00000000, 0 },
  { 5834,  1367, 17, 214913,     13.500, 0x00000000, 0 },
  { 5835,  1367, 18, 214913,     14.000, 0x00000000, 0 },
  { 5836,  1367, 19, 214913,     14.500, 0x00000000, 0 },
  { 5837,  1368,  0, 214914,      5.000, 0x00000000, 0 },
  { 5838,  1368,  1, 214914,      5.500, 0x00000000, 0 },
  { 5839,  1368,  2, 214914,      6.000, 0x00000000, 0 },
  { 5840,  1368,  3, 214914,      6.500, 0x00000000, 0 },
  { 5841,  1368,  4, 214914,      7.000, 0x00000000, 0 },
  { 5842,  1368,  5, 214914,      7.500, 0x00000000, 0 },
  { 5843,  1368,  6, 214914,      8.000, 0x00000000, 0 },
  { 5844,  1368,  7, 214914,      8.500, 0x00000000, 0 },
  { 5845,  1368,  8, 214914,      9.000, 0x00000000, 0 },
  { 5846,  1368,  9, 214914,      9.500, 0x00000000, 0 },
  { 5847,  1368, 10, 214914,     10.000, 0x00000000, 0 },
  { 5848,  1368, 11, 214914,     10.500, 0x00000000, 0 },
  { 5849,  1368, 12, 214914,     11.000, 0x00000000, 0 },
  { 5850,  1368, 13, 214914,     11.500, 0x00000000, 0 },
  { 5851,  1368, 14, 214914,     12.000, 0x00000000, 0 },
  { 5852,  1368, 15, 214914,     12.500, 0x00000000, 0 },
  { 5853,  1368, 16, 214914,     13.000, 0x00000000, 0 },
  { 5854,  1368, 17, 214914,     13.500, 0x00000000, 0 },
  { 5855,  1368, 18, 214914,     14.000, 0x00000000, 0 },
  { 5856,  1368, 19, 214914,     14.500, 0x00000000, 0 },
  { 5857,  1369,  0, 214915,      5.000, 0x00000000, 0 },
  { 5858,  1369,  1, 214915,      5.500, 0x00000000, 0 },
  { 5859,  1369,  2, 214915,      6.000, 0x00000000, 0 },
  { 5860,  1369,  3, 214915,      6.500, 0x00000000, 0 },
  { 5861,  1369,  4, 214915,      7.000, 0x00000000, 0 },
  { 5862,  1369,  5, 214915,      7.500, 0x00000000, 0 },
  { 5863,  1369,  6, 214915,      8.000, 0x00000000, 0 },
  { 5864,  1369,  7, 214915,      8.500, 0x00000000, 0 },
  { 5865,  1369,  8, 214915,      9.000, 0x00000000, 0 },
  { 5866,  1369,  9, 214915,      9.500, 0x00000000, 0 },
  { 5867,  1369, 10, 214915,     10.000, 0x00000000, 0 },
  { 5868,  1369, 11, 214915,     10.500, 0x00000000, 0 },
  { 5869,  1369, 12, 214915,     11.000, 0x00000000, 0 },
  { 5870,  1369, 13, 214915,     11.500, 0x00000000, 0 },
  { 5871,  1369, 14, 214915,     12.000, 0x00000000, 0 },
  { 5872,  1369, 15, 214915,     12.500, 0x00000000, 0 },
  { 5873,  1369, 16, 214915,     13.000, 0x00000000, 0 },
  { 5874,  1369, 17, 214915,     13.500, 0x00000000, 0 },
  { 5875,  1369, 18, 214915,     14.000, 0x00000000, 0 },
  { 5876,  1369, 19, 214915,     14.500, 0x00000000, 0 },
  { 5877,  1370,  0, 214916,      5.000, 0x00000000, 0 },
  { 5878,  1370,  1, 214916,      5.500, 0x00000000, 0 },
  { 5879,  1370,  2, 214916,      6.000, 0x00000000, 0 },
  { 5880,  1370,  3, 214916,      6.500, 0x00000000, 0 },
  { 5881,  1370,  4, 214916,      7.000, 0x00000000, 0 },
  { 5882,  1370,  5, 214916,      7.500, 0x00000000, 0 },
  { 5883,  1370,  6, 214916,      8.000, 0x00000000, 0 },
  { 5884,  1370,  7, 214916,      8.500, 0x00000000, 0 },
  { 5885,  1370,  8, 214916,      9.000, 0x00000000, 0 },
  { 5886,  1370,  9, 214916,      9.500, 0x00000000, 0 },
  { 5887,  1370, 10, 214916,     10.000, 0x00000000, 0 },
  { 5888,  1370, 11, 214916,     10.500, 0x00000000, 0 },
  { 5889,  1370, 12, 214916,     11.000, 0x00000000, 0 },
  { 5890,  1370, 13, 214916,     11.500, 0x00000000, 0 },
  { 5891,  1370, 14, 214916,     12.000, 0x00000000, 0 },
  { 5892,  1370, 15, 214916,     12.500, 0x00000000, 0 },
  { 5893,  1370, 16, 214916,     13.000, 0x00000000, 0 },
  { 5894,  1370, 17, 214916,     13.500, 0x00000000, 0 },
  { 5895,  1370, 18, 214916,     14.000, 0x00000000, 0 },
  { 5896,  1370, 19, 214916,     14.500, 0x00000000, 0 },
  { 5897,  1371,  0, 214917,      5.000, 0x00000000, 0 },
  { 5898,  1371,  1, 214917,      5.500, 0x00000000, 0 },
  { 5899,  1371,  2, 214917,      6.000, 0x00000000, 0 },
  { 5900,  1371,  3, 214917,      6.500, 0x00000000, 0 },
  { 5901,  1371,  4, 214917,      7.000, 0x00000000, 0 },
  { 5902,  1371,  5, 214917,      7.500, 0x00000000, 0 },
  { 5903,  1371,  6, 214917,      8.000, 0x00000000, 0 },
  { 5904,  1371,  7, 214917,      8.500, 0x00000000, 0 },
  { 5905,  1371,  8, 214917,      9.000, 0x00000000, 0 },
  { 5906,  1371,  9, 214917,      9.500, 0x00000000, 0 },
  { 5907,  1371, 10, 214917,     10.000, 0x00000000, 0 },
  { 5908,  1371, 11, 214917,     10.500, 0x00000000, 0 },
  { 5909,  1371, 12, 214917,     11.000, 0x00000000, 0 },
  { 5910,  1371, 13, 214917,     11.500, 0x00000000, 0 },
  { 5911,  1371, 14, 214917,     12.000, 0x00000000, 0 },
  { 5912,  1371, 15, 214917,     12.500, 0x00000000, 0 },
  { 5913,  1371, 16, 214917,     13.000, 0x00000000, 0 },
  { 5914,  1371, 17, 214917,     13.500, 0x00000000, 0 },
  { 5915,  1371, 18, 214917,     14.000, 0x00000000, 0 },
  { 5916,  1371, 19, 214917,     14.500, 0x00000000, 0 },
  { 5917,  1372,  0, 214918,      5.000, 0x00000000, 0 },
  { 5918,  1372,  1, 214918,      5.500, 0x00000000, 0 },
  { 5919,  1372,  2, 214918,      6.000, 0x00000000, 0 },
  { 5920,  1372,  3, 214918,      6.500, 0x00000000, 0 },
  { 5921,  1372,  4, 214918,      7.000, 0x00000000, 0 },
  { 5922,  1372,  5, 214918,      7.500, 0x00000000, 0 },
  { 5923,  1372,  6, 214918,      8.000, 0x00000000, 0 },
  { 5924,  1372,  7, 214918,      8.500, 0x00000000, 0 },
  { 5925,  1372,  8, 214918,      9.000, 0x00000000, 0 },
  { 5926,  1372,  9, 214918,      9.500, 0x00000000, 0 },
  { 5927,  1372, 10, 214918,     10.000, 0x00000000, 0 },
  { 5928,  1372, 11, 214918,     10.500, 0x00000000, 0 },
  { 5929,  1372, 12, 214918,     11.000, 0x00000000, 0 },
  { 5930,  1372, 13, 214918,     11.500, 0x00000000, 0 },
  { 5931,  1372, 14, 214918,     12.000, 0x00000000, 0 },
  { 5932,  1372, 15, 214918,     12.500, 0x00000000, 0 },
  { 5933,  1372, 16, 214918,     13.000, 0x00000000, 0 },
  { 5934,  1372, 17, 214918,     13.500, 0x00000000, 0 },
  { 5935,  1372, 18, 214918,     14.000, 0x00000000, 0 },
  { 5936,  1372, 19, 214918,     14.500, 0x00000000, 0 },
  { 5937,  1373,  0, 214919,      5.000, 0x00000000, 0 },
  { 5938,  1373,  1, 214919,      5.500, 0x00000000, 0 },
  { 5939,  1373,  2, 214919,      6.000, 0x00000000, 0 },
  { 5940,  1373,  3, 214919,      6.500, 0x00000000, 0 },
  { 5941,  1373,  4, 214919,      7.000, 0x00000000, 0 },
  { 5942,  1373,  5, 214919,      7.500, 0x00000000, 0 },
  { 5943,  1373,  6, 214919,      8.000, 0x00000000, 0 },
  { 5944,  1373,  7, 214919,      8.500, 0x00000000, 0 },
  { 5945,  1373,  8, 214919,      9.000, 0x00000000, 0 },
  { 5946,  1373,  9, 214919,      9.500, 0x00000000, 0 },
  { 5947,  1373, 10, 214919,     10.000, 0x00000000, 0 },
  { 5948,  1373, 11, 214919,     10.500, 0x00000000, 0 },
  { 5949,  1373, 12, 214919,     11.000, 0x00000000, 0 },
  { 5950,  1373, 13, 214919,     11.500, 0x00000000, 0 },
  { 5951,  1373, 14, 214919,     12.000, 0x00000000, 0 },
  { 5952,  1373, 15, 214919,     12.500, 0x00000000, 0 },
  { 5953,  1373, 16, 214919,     13.000, 0x00000000, 0 },
  { 5954,  1373, 17, 214919,     13.500, 0x00000000, 0 },
  { 5955,  1373, 18, 214919,     14.000, 0x00000000, 0 },
  { 5956,  1373, 19, 214919,     14.500, 0x00000000, 0 },
  { 5957,  1374,  0, 214920,     10.000, 0x00000000, 0 },
  { 5958,  1374,  1, 214920,     11.000, 0x00000000, 0 },
  { 5959,  1374,  2, 214920,     12.000, 0x00000000, 0 },
  { 5960,  1374,  3, 214920,     13.000, 0x00000000, 0 },
  { 5961,  1374,  4, 214920,     14.000, 0x00000000, 0 },
  { 5962,  1374,  5, 214920,     15.000, 0x00000000, 0 },
  { 5963,  1374,  6, 214920,     16.000, 0x00000000, 0 },
  { 5964,  1374,  7, 214920,     17.000, 0x00000000, 0 },
  { 5965,  1374,  8, 214920,     18.000, 0x00000000, 0 },
  { 5966,  1374,  9, 214920,     19.000, 0x00000000, 0 },
  { 5967,  1374, 10, 214920,     20.000, 0x00000000, 0 },
  { 5968,  1374, 11, 214920,     21.000, 0x00000000, 0 },
  { 5969,  1374, 12, 214920,     22.000, 0x00000000, 0 },
  { 5970,  1374, 13, 214920,     23.000, 0x00000000, 0 },
  { 5971,  1374, 14, 214920,     24.000, 0x00000000, 0 },
  { 5972,  1374, 15, 214920,     25.000, 0x00000000, 0 },
  { 5973,  1374, 16, 214920,     26.000, 0x00000000, 0 },
  { 5974,  1374, 17, 214920,     27.000, 0x00000000, 0 },
  { 5975,  1374, 18, 214920,     28.000, 0x00000000, 0 },
  { 5976,  1374, 19, 214920,     29.000, 0x00000000, 0 },
  { 5977,  1375,  0, 214921,      5.000, 0x00000000, 0 },
  { 5978,  1375,  1, 214921,      5.500, 0x00000000, 0 },
  { 5979,  1375,  2, 214921,      6.000, 0x00000000, 0 },
  { 5980,  1375,  3, 214921,      6.500, 0x00000000, 0 },
  { 5981,  1375,  4, 214921,      7.000, 0x00000000, 0 },
  { 5982,  1375,  5, 214921,      7.500, 0x00000000, 0 },
  { 5983,  1375,  6, 214921,      8.000, 0x00000000, 0 },
  { 5984,  1375,  7, 214921,      8.500, 0x00000000, 0 },
  { 5985,  1375,  8, 214921,      9.000, 0x00000000, 0 },
  { 5986,  1375,  9, 214921,      9.500, 0x00000000, 0 },
  { 5987,  1375, 10, 214921,     10.000, 0x00000000, 0 },
  { 5988,  1375, 11, 214921,     10.500, 0x00000000, 0 },
  { 5989,  1375, 12, 214921,     11.000, 0x00000000, 0 },
  { 5990,  1375, 13, 214921,     11.500, 0x00000000, 0 },
  { 5991,  1375, 14, 214921,     12.000, 0x00000000, 0 },
  { 5992,  1375, 15, 214921,     12.500, 0x00000000, 0 },
  { 5993,  1375, 16, 214921,     13.000, 0x00000000, 0 },
  { 5994,  1375, 17, 214921,     13.500, 0x00000000, 0 },
  { 5995,  1375, 18, 214921,     14.000, 0x00000000, 0 },
  { 5996,  1375, 19, 214921,     14.500, 0x00000000, 0 },
  { 5997,  1376,  0, 214922,      5.000, 0x00000000, 0 },
  { 5998,  1376,  1, 214922,      5.500, 0x00000000, 0 },
  { 5999,  1376,  2, 214922,      6.000, 0x00000000, 0 },
  { 6000,  1376,  3, 214922,      6.500, 0x00000000, 0 },
  { 6001,  1376,  4, 214922,      7.000, 0x00000000, 0 },
  { 6002,  1376,  5, 214922,      7.500, 0x00000000, 0 },
  { 6003,  1376,  6, 214922,      8.000, 0x00000000, 0 },
  { 6004,  1376,  7, 214922,      8.500, 0x00000000, 0 },
  { 6005,  1376,  8, 214922,      9.000, 0x00000000, 0 },
  { 6006,  1376,  9, 214922,      9.500, 0x00000000, 0 },
  { 6007,  1376, 10, 214922,     10.000, 0x00000000, 0 },
  { 6008,  1376, 11, 214922,     10.500, 0x00000000, 0 },
  { 6009,  1376, 12, 214922,     11.000, 0x00000000, 0 },
  { 6010,  1376, 13, 214922,     11.500, 0x00000000, 0 },
  { 6011,  1376, 14, 214922,     12.000, 0x00000000, 0 },
  { 6012,  1376, 15, 214922,     12.500, 0x00000000, 0 },
  { 6013,  1376, 16, 214922,     13.000, 0x00000000, 0 },
  { 6014,  1376, 17, 214922,     13.500, 0x00000000, 0 },
  { 6015,  1376, 18, 214922,     14.000, 0x00000000, 0 },
  { 6016,  1376, 19, 214922,     14.500, 0x00000000, 0 },
  { 6017,  1377,  0, 214923,      5.000, 0x00000000, 0 },
  { 6018,  1377,  1, 214923,      5.500, 0x00000000, 0 },
  { 6019,  1377,  2, 214923,      6.000, 0x00000000, 0 },
  { 6020,  1377,  3, 214923,      6.500, 0x00000000, 0 },
  { 6021,  1377,  4, 214923,      7.000, 0x00000000, 0 },
  { 6022,  1377,  5, 214923,      7.500, 0x00000000, 0 },
  { 6023,  1377,  6, 214923,      8.000, 0x00000000, 0 },
  { 6024,  1377,  7, 214923,      8.500, 0x00000000, 0 },
  { 6025,  1377,  8, 214923,      9.000, 0x00000000, 0 },
  { 6026,  1377,  9, 214923,      9.500, 0x00000000, 0 },
  { 6027,  1377, 10, 214923,     10.000, 0x00000000, 0 },
  { 6028,  1377, 11, 214923,     10.500, 0x00000000, 0 },
  { 6029,  1377, 12, 214923,     11.000, 0x00000000, 0 },
  { 6030,  1377, 13, 214923,     11.500, 0x00000000, 0 },
  { 6031,  1377, 14, 214923,     12.000, 0x00000000, 0 },
  { 6032,  1377, 15, 214923,     12.500, 0x00000000, 0 },
  { 6033,  1377, 16, 214923,     13.000, 0x00000000, 0 },
  { 6034,  1377, 17, 214923,     13.500, 0x00000000, 0 },
  { 6035,  1377, 18, 214923,     14.000, 0x00000000, 0 },
  { 6036,  1377, 19, 214923,     14.500, 0x00000000, 0 },
  { 6037,  1378,  0, 214924,     10.000, 0x00000000, 0 },
  { 6038,  1378,  1, 214924,     11.000, 0x00000000, 0 },
  { 6039,  1378,  2, 214924,     12.000, 0x00000000, 0 },
  { 6040,  1378,  3, 214924,     13.000, 0x00000000, 0 },
  { 6041,  1378,  4, 214924,     14.000, 0x00000000, 0 },
  { 6042,  1378,  5, 214924,     15.000, 0x00000000, 0 },
  { 6043,  1378,  6, 214924,     16.000, 0x00000000, 0 },
  { 6044,  1378,  7, 214924,     17.000, 0x00000000, 0 },
  { 6045,  1378,  8, 214924,     18.000, 0x00000000, 0 },
  { 6046,  1378,  9, 214924,     19.000, 0x00000000, 0 },
  { 6047,  1378, 10, 214924,     20.000, 0x00000000, 0 },
  { 6048,  1378, 11, 214924,     21.000, 0x00000000, 0 },
  { 6049,  1378, 12, 214924,     22.000, 0x00000000, 0 },
  { 6050,  1378, 13, 214924,     23.000, 0x00000000, 0 },
  { 6051,  1378, 14, 214924,     24.000, 0x00000000, 0 },
  { 6052,  1378, 15, 214924,     25.000, 0x00000000, 0 },
  { 6053,  1378, 16, 214924,     26.000, 0x00000000, 0 },
  { 6054,  1378, 17, 214924,     27.000, 0x00000000, 0 },
  { 6055,  1378, 18, 214924,     28.000, 0x00000000, 0 },
  { 6056,  1378, 19, 214924,     29.000, 0x00000000, 0 },
  { 6057,  1379,  0, 214925,      4.000, 0x00000000, 0 },
  { 6058,  1379,  1, 214925,      4.400, 0x00000000, 0 },
  { 6059,  1379,  2, 214925,      4.800, 0x00000000, 0 },
  { 6060,  1379,  3, 214925,      5.200, 0x00000000, 0 },
  { 6061,  1379,  4, 214925,      5.600, 0x00000000, 0 },
  { 6062,  1379,  5, 214925,      6.000, 0x00000000, 0 },
  { 6063,  1379,  6, 214925,      6.400, 0x00000000, 0 },
  { 6064,  1379,  7, 214925,      6.800, 0x00000000, 0 },
  { 6065,  1379,  8, 214925,      7.200, 0x00000000, 0 },
  { 6066,  1379,  9, 214925,      7.600, 0x00000000, 0 },
  { 6067,  1379, 10, 214925,      8.000, 0x00000000, 0 },
  { 6068,  1379, 11, 214925,      8.400, 0x00000000, 0 },
  { 6069,  1379, 12, 214925,      8.800, 0x00000000, 0 },
  { 6070,  1379, 13, 214925,      9.200, 0x00000000, 0 },
  { 6071,  1379, 14, 214925,      9.600, 0x00000000, 0 },
  { 6072,  1379, 15, 214925,     10.000, 0x00000000, 0 },
  { 6073,  1379, 16, 214925,     10.400, 0x00000000, 0 },
  { 6074,  1379, 17, 214925,     10.800, 0x00000000, 0 },
  { 6075,  1379, 18, 214925,     11.200, 0x00000000, 0 },
  { 6076,  1379, 19, 214925,     11.600, 0x00000000, 0 },
  { 6077,  1380,  0, 214926,      5.000, 0x00000000, 0 },
  { 6078,  1380,  1, 214926,      5.500, 0x00000000, 0 },
  { 6079,  1380,  2, 214926,      6.000, 0x00000000, 0 },
  { 6080,  1380,  3, 214926,      6.500, 0x00000000, 0 },
  { 6081,  1380,  4, 214926,      7.000, 0x00000000, 0 },
  { 6082,  1380,  5, 214926,      7.500, 0x00000000, 0 },
  { 6083,  1380,  6, 214926,      8.000, 0x00000000, 0 },
  { 6084,  1380,  7, 214926,      8.500, 0x00000000, 0 },
  { 6085,  1380,  8, 214926,      9.000, 0x00000000, 0 },
  { 6086,  1380,  9, 214926,      9.500, 0x00000000, 0 },
  { 6087,  1380, 10, 214926,     10.000, 0x00000000, 0 },
  { 6088,  1380, 11, 214926,     10.500, 0x00000000, 0 },
  { 6089,  1380, 12, 214926,     11.000, 0x00000000, 0 },
  { 6090,  1380, 13, 214926,     11.500, 0x00000000, 0 },
  { 6091,  1380, 14, 214926,     12.000, 0x00000000, 0 },
  { 6092,  1380, 15, 214926,     12.500, 0x00000000, 0 },
  { 6093,  1380, 16, 214926,     13.000, 0x00000000, 0 },
  { 6094,  1380, 17, 214926,     13.500, 0x00000000, 0 },
  { 6095,  1380, 18, 214926,     14.000, 0x00000000, 0 },
  { 6096,  1380, 19, 214926,     14.500, 0x00000000, 0 },
  { 6097,  1381,  0, 214927,      5.000, 0x00000000, 0 },
  { 6098,  1381,  1, 214927,      5.500, 0x00000000, 0 },
  { 6099,  1381,  2, 214927,      6.000, 0x00000000, 0 },
  { 6100,  1381,  3, 214927,      6.500, 0x00000000, 0 },
  { 6101,  1381,  4, 214927,      7.000, 0x00000000, 0 },
  { 6102,  1381,  5, 214927,      7.500, 0x00000000, 0 },
  { 6103,  1381,  6, 214927,      8.000, 0x00000000, 0 },
  { 6104,  1381,  7, 214927,      8.500, 0x00000000, 0 },
  { 6105,  1381,  8, 214927,      9.000, 0x00000000, 0 },
  { 6106,  1381,  9, 214927,      9.500, 0x00000000, 0 },
  { 6107,  1381, 10, 214927,     10.000, 0x00000000, 0 },
  { 6108,  1381, 11, 214927,     10.500, 0x00000000, 0 },
  { 6109,  1381, 12, 214927,     11.000, 0x00000000, 0 },
  { 6110,  1381, 13, 214927,     11.500, 0x00000000, 0 },
  { 6111,  1381, 14, 214927,     12.000, 0x00000000, 0 },
  { 6112,  1381, 15, 214927,     12.500, 0x00000000, 0 },
  { 6113,  1381, 16, 214927,     13.000, 0x00000000, 0 },
  { 6114,  1381, 17, 214927,     13.500, 0x00000000, 0 },
  { 6115,  1381, 18, 214927,     14.000, 0x00000000, 0 },
  { 6116,  1381, 19, 214927,     14.500, 0x00000000, 0 },
  { 6157,  1384,  0, 214928,      5.000, 0x00000000, 0 },
  { 6158,  1384,  1, 214928,      5.500, 0x00000000, 0 },
  { 6159,  1384,  2, 214928,      6.000, 0x00000000, 0 },
  { 6160,  1384,  3, 214928,      6.500, 0x00000000, 0 },
  { 6161,  1384,  4, 214928,      7.000, 0x00000000, 0 },
  { 6162,  1384,  5, 214928,      7.500, 0x00000000, 0 },
  { 6163,  1384,  6, 214928,      8.000, 0x00000000, 0 },
  { 6164,  1384,  7, 214928,      8.500, 0x00000000, 0 },
  { 6165,  1384,  8, 214928,      9.000, 0x00000000, 0 },
  { 6166,  1384,  9, 214928,      9.500, 0x00000000, 0 },
  { 6167,  1384, 10, 214928,     10.000, 0x00000000, 0 },
  { 6168,  1384, 11, 214928,     10.500, 0x00000000, 0 },
  { 6169,  1384, 12, 214928,     11.000, 0x00000000, 0 },
  { 6170,  1384, 13, 214928,     11.500, 0x00000000, 0 },
  { 6171,  1384, 14, 214928,     12.000, 0x00000000, 0 },
  { 6172,  1384, 15, 214928,     12.500, 0x00000000, 0 },
  { 6173,  1384, 16, 214928,     13.000, 0x00000000, 0 },
  { 6174,  1384, 17, 214928,     13.500, 0x00000000, 0 },
  { 6175,  1384, 18, 214928,     14.000, 0x00000000, 0 },
  { 6176,  1384, 19, 214928,     14.500, 0x00000000, 0 },
  { 6177,  1385,  0, 214929,      5.000, 0x00000000, 0 },
  { 6178,  1385,  1, 214929,      5.500, 0x00000000, 0 },
  { 6179,  1385,  2, 214929,      6.000, 0x00000000, 0 },
  { 6180,  1385,  3, 214929,      6.500, 0x00000000, 0 },
  { 6181,  1385,  4, 214929,      7.000, 0x00000000, 0 },
  { 6182,  1385,  5, 214929,      7.500, 0x00000000, 0 },
  { 6183,  1385,  6, 214929,      8.000, 0x00000000, 0 },
  { 6184,  1385,  7, 214929,      8.500, 0x00000000, 0 },
  { 6185,  1385,  8, 214929,      9.000, 0x00000000, 0 },
  { 6186,  1385,  9, 214929,      9.500, 0x00000000, 0 },
  { 6187,  1385, 10, 214929,     10.000, 0x00000000, 0 },
  { 6188,  1385, 11, 214929,     10.500, 0x00000000, 0 },
  { 6189,  1385, 12, 214929,     11.000, 0x00000000, 0 },
  { 6190,  1385, 13, 214929,     11.500, 0x00000000, 0 },
  { 6191,  1385, 14, 214929,     12.000, 0x00000000, 0 },
  { 6192,  1385, 15, 214929,     12.500, 0x00000000, 0 },
  { 6193,  1385, 16, 214929,     13.000, 0x00000000, 0 },
  { 6194,  1385, 17, 214929,     13.500, 0x00000000, 0 },
  { 6195,  1385, 18, 214929,     14.000, 0x00000000, 0 },
  { 6196,  1385, 19, 214929,     14.500, 0x00000000, 0 },
  { 6197,  1386,  0, 214930,      5.000, 0x00000000, 0 },
  { 6198,  1386,  1, 214930,      5.500, 0x00000000, 0 },
  { 6199,  1386,  2, 214930,      6.000, 0x00000000, 0 },
  { 6200,  1386,  3, 214930,      6.500, 0x00000000, 0 },
  { 6201,  1386,  4, 214930,      7.000, 0x00000000, 0 },
  { 6202,  1386,  5, 214930,      7.500, 0x00000000, 0 },
  { 6203,  1386,  6, 214930,      8.000, 0x00000000, 0 },
  { 6204,  1386,  7, 214930,      8.500, 0x00000000, 0 },
  { 6205,  1386,  8, 214930,      9.000, 0x00000000, 0 },
  { 6206,  1386,  9, 214930,      9.500, 0x00000000, 0 },
  { 6207,  1386, 10, 214930,     10.000, 0x00000000, 0 },
  { 6208,  1386, 11, 214930,     10.500, 0x00000000, 0 },
  { 6209,  1386, 12, 214930,     11.000, 0x00000000, 0 },
  { 6210,  1386, 13, 214930,     11.500, 0x00000000, 0 },
  { 6211,  1386, 14, 214930,     12.000, 0x00000000, 0 },
  { 6212,  1386, 15, 214930,     12.500, 0x00000000, 0 },
  { 6213,  1386, 16, 214930,     13.000, 0x00000000, 0 },
  { 6214,  1386, 17, 214930,     13.500, 0x00000000, 0 },
  { 6215,  1386, 18, 214930,     14.000, 0x00000000, 0 },
  { 6216,  1386, 19, 214930,     14.500, 0x00000000, 0 },
  { 6217,  1387,  0, 214931,      5.000, 0x00000000, 0 },
  { 6218,  1387,  1, 214931,      5.500, 0x00000000, 0 },
  { 6219,  1387,  2, 214931,      6.000, 0x00000000, 0 },
  { 6220,  1387,  3, 214931,      6.500, 0x00000000, 0 },
  { 6221,  1387,  4, 214931,      7.000, 0x00000000, 0 },
  { 6222,  1387,  5, 214931,      7.500, 0x00000000, 0 },
  { 6223,  1387,  6, 214931,      8.000, 0x00000000, 0 },
  { 6224,  1387,  7, 214931,      8.500, 0x00000000, 0 },
  { 6225,  1387,  8, 214931,      9.000, 0x00000000, 0 },
  { 6226,  1387,  9, 214931,      9.500, 0x00000000, 0 },
  { 6227,  1387, 10, 214931,     10.000, 0x00000000, 0 },
  { 6228,  1387, 11, 214931,     10.500, 0x00000000, 0 },
  { 6229,  1387, 12, 214931,     11.000, 0x00000000, 0 },
  { 6230,  1387, 13, 214931,     11.500, 0x00000000, 0 },
  { 6231,  1387, 14, 214931,     12.000, 0x00000000, 0 },
  { 6232,  1387, 15, 214931,     12.500, 0x00000000, 0 },
  { 6233,  1387, 16, 214931,     13.000, 0x00000000, 0 },
  { 6234,  1387, 17, 214931,     13.500, 0x00000000, 0 },
  { 6235,  1387, 18, 214931,     14.000, 0x00000000, 0 },
  { 6236,  1387, 19, 214931,     14.500, 0x00000000, 0 },
  { 6237,  1388,  0, 214932,      5.000, 0x00000000, 0 },
  { 6238,  1388,  1, 214932,      5.500, 0x00000000, 0 },
  { 6239,  1388,  2, 214932,      6.000, 0x00000000, 0 },
  { 6240,  1388,  3, 214932,      6.500, 0x00000000, 0 },
  { 6241,  1388,  4, 214932,      7.000, 0x00000000, 0 },
  { 6242,  1388,  5, 214932,      7.500, 0x00000000, 0 },
  { 6243,  1388,  6, 214932,      8.000, 0x00000000, 0 },
  { 6244,  1388,  7, 214932,      8.500, 0x00000000, 0 },
  { 6245,  1388,  8, 214932,      9.000, 0x00000000, 0 },
  { 6246,  1388,  9, 214932,      9.500, 0x00000000, 0 },
  { 6247,  1388, 10, 214932,     10.000, 0x00000000, 0 },
  { 6248,  1388, 11, 214932,     10.500, 0x00000000, 0 },
  { 6249,  1388, 12, 214932,     11.000, 0x00000000, 0 },
  { 6250,  1388, 13, 214932,     11.500, 0x00000000, 0 },
  { 6251,  1388, 14, 214932,     12.000, 0x00000000, 0 },
  { 6252,  1388, 15, 214932,     12.500, 0x00000000, 0 },
  { 6253,  1388, 16, 214932,     13.000, 0x00000000, 0 },
  { 6254,  1388, 17, 214932,     13.500, 0x00000000, 0 },
  { 6255,  1388, 18, 214932,     14.000, 0x00000000, 0 },
  { 6256,  1388, 19, 214932,     14.500, 0x00000000, 0 },
  { 6257,  1389,  0, 214933,      5.000, 0x00000000, 0 },
  { 6258,  1389,  1, 214933,      5.500, 0x00000000, 0 },
  { 6259,  1389,  2, 214933,      6.000, 0x00000000, 0 },
  { 6260,  1389,  3, 214933,      6.500, 0x00000000, 0 },
  { 6261,  1389,  4, 214933,      7.000, 0x00000000, 0 },
  { 6262,  1389,  5, 214933,      7.500, 0x00000000, 0 },
  { 6263,  1389,  6, 214933,      8.000, 0x00000000, 0 },
  { 6264,  1389,  7, 214933,      8.500, 0x00000000, 0 },
  { 6265,  1389,  8, 214933,      9.000, 0x00000000, 0 },
  { 6266,  1389,  9, 214933,      9.500, 0x00000000, 0 },
  { 6267,  1389, 10, 214933,     10.000, 0x00000000, 0 },
  { 6268,  1389, 11, 214933,     10.500, 0x00000000, 0 },
  { 6269,  1389, 12, 214933,     11.000, 0x00000000, 0 },
  { 6270,  1389, 13, 214933,     11.500, 0x00000000, 0 },
  { 6271,  1389, 14, 214933,     12.000, 0x00000000, 0 },
  { 6272,  1389, 15, 214933,     12.500, 0x00000000, 0 },
  { 6273,  1389, 16, 214933,     13.000, 0x00000000, 0 },
  { 6274,  1389, 17, 214933,     13.500, 0x00000000, 0 },
  { 6275,  1389, 18, 214933,     14.000, 0x00000000, 0 },
  { 6276,  1389, 19, 214933,     14.500, 0x00000000, 0 },
  { 6277,  1390,  0, 214934,      5.000, 0x00000000, 0 },
  { 6278,  1390,  1, 214934,      5.500, 0x00000000, 0 },
  { 6279,  1390,  2, 214934,      6.000, 0x00000000, 0 },
  { 6280,  1390,  3, 214934,      6.500, 0x00000000, 0 },
  { 6281,  1390,  4, 214934,      7.000, 0x00000000, 0 },
  { 6282,  1390,  5, 214934,      7.500, 0x00000000, 0 },
  { 6283,  1390,  6, 214934,      8.000, 0x00000000, 0 },
  { 6284,  1390,  7, 214934,      8.500, 0x00000000, 0 },
  { 6285,  1390,  8, 214934,      9.000, 0x00000000, 0 },
  { 6286,  1390,  9, 214934,      9.500, 0x00000000, 0 },
  { 6287,  1390, 10, 214934,     10.000, 0x00000000, 0 },
  { 6288,  1390, 11, 214934,     10.500, 0x00000000, 0 },
  { 6289,  1390, 12, 214934,     11.000, 0x00000000, 0 },
  { 6290,  1390, 13, 214934,     11.500, 0x00000000, 0 },
  { 6291,  1390, 14, 214934,     12.000, 0x00000000, 0 },
  { 6292,  1390, 15, 214934,     12.500, 0x00000000, 0 },
  { 6293,  1390, 16, 214934,     13.000, 0x00000000, 0 },
  { 6294,  1390, 17, 214934,     13.500, 0x00000000, 0 },
  { 6295,  1390, 18, 214934,     14.000, 0x00000000, 0 },
  { 6296,  1390, 19, 214934,     14.500, 0x00000000, 0 },
  { 6297,  1391,  0, 214935,      5.000, 0x00000000, 0 },
  { 6298,  1391,  1, 214935,      5.500, 0x00000000, 0 },
  { 6299,  1391,  2, 214935,      6.000, 0x00000000, 0 },
  { 6300,  1391,  3, 214935,      6.500, 0x00000000, 0 },
  { 6301,  1391,  4, 214935,      7.000, 0x00000000, 0 },
  { 6302,  1391,  5, 214935,      7.500, 0x00000000, 0 },
  { 6303,  1391,  6, 214935,      8.000, 0x00000000, 0 },
  { 6304,  1391,  7, 214935,      8.500, 0x00000000, 0 },
  { 6305,  1391,  8, 214935,      9.000, 0x00000000, 0 },
  { 6306,  1391,  9, 214935,      9.500, 0x00000000, 0 },
  { 6307,  1391, 10, 214935,     10.000, 0x00000000, 0 },
  { 6308,  1391, 11, 214935,     10.500, 0x00000000, 0 },
  { 6309,  1391, 12, 214935,     11.000, 0x00000000, 0 },
  { 6310,  1391, 13, 214935,     11.500, 0x00000000, 0 },
  { 6311,  1391, 14, 214935,     12.000, 0x00000000, 0 },
  { 6312,  1391, 15, 214935,     12.500, 0x00000000, 0 },
  { 6313,  1391, 16, 214935,     13.000, 0x00000000, 0 },
  { 6314,  1391, 17, 214935,     13.500, 0x00000000, 0 },
  { 6315,  1391, 18, 214935,     14.000, 0x00000000, 0 },
  { 6316,  1391, 19, 214935,     14.500, 0x00000000, 0 },
  { 6317,  1392,  0, 214936,      5.000, 0x00000000, 0 },
  { 6318,  1392,  1, 214936,      5.500, 0x00000000, 0 },
  { 6319,  1392,  2, 214936,      6.000, 0x00000000, 0 },
  { 6320,  1392,  3, 214936,      6.500, 0x00000000, 0 },
  { 6321,  1392,  4, 214936,      7.000, 0x00000000, 0 },
  { 6322,  1392,  5, 214936,      7.500, 0x00000000, 0 },
  { 6323,  1392,  6, 214936,      8.000, 0x00000000, 0 },
  { 6324,  1392,  7, 214936,      8.500, 0x00000000, 0 },
  { 6325,  1392,  8, 214936,      9.000, 0x00000000, 0 },
  { 6326,  1392,  9, 214936,      9.500, 0x00000000, 0 },
  { 6327,  1392, 10, 214936,     10.000, 0x00000000, 0 },
  { 6328,  1392, 11, 214936,     10.500, 0x00000000, 0 },
  { 6329,  1392, 12, 214936,     11.000, 0x00000000, 0 },
  { 6330,  1392, 13, 214936,     11.500, 0x00000000, 0 },
  { 6331,  1392, 14, 214936,     12.000, 0x00000000, 0 },
  { 6332,  1392, 15, 214936,     12.500, 0x00000000, 0 },
  { 6333,  1392, 16, 214936,     13.000, 0x00000000, 0 },
  { 6334,  1392, 17, 214936,     13.500, 0x00000000, 0 },
  { 6335,  1392, 18, 214936,     14.000, 0x00000000, 0 },
  { 6336,  1392, 19, 214936,     14.500, 0x00000000, 0 },
  { 6337,  1393,  0, 214937,      5.000, 0x00000000, 0 },
  { 6338,  1393,  1, 214937,      5.500, 0x00000000, 0 },
  { 6339,  1393,  2, 214937,      6.000, 0x00000000, 0 },
  { 6340,  1393,  3, 214937,      6.500, 0x00000000, 0 },
  { 6341,  1393,  4, 214937,      7.000, 0x00000000, 0 },
  { 6342,  1393,  5, 214937,      7.500, 0x00000000, 0 },
  { 6343,  1393,  6, 214937,      8.000, 0x00000000, 0 },
  { 6344,  1393,  7, 214937,      8.500, 0x00000000, 0 },
  { 6345,  1393,  8, 214937,      9.000, 0x00000000, 0 },
  { 6346,  1393,  9, 214937,      9.500, 0x00000000, 0 },
  { 6347,  1393, 10, 214937,     10.000, 0x00000000, 0 },
  { 6348,  1393, 11, 214937,     10.500, 0x00000000, 0 },
  { 6349,  1393, 12, 214937,     11.000, 0x00000000, 0 },
  { 6350,  1393, 13, 214937,     11.500, 0x00000000, 0 },
  { 6351,  1393, 14, 214937,     12.000, 0x00000000, 0 },
  { 6352,  1393, 15, 214937,     12.500, 0x00000000, 0 },
  { 6353,  1393, 16, 214937,     13.000, 0x00000000, 0 },
  { 6354,  1393, 17, 214937,     13.500, 0x00000000, 0 },
  { 6355,  1393, 18, 214937,     14.000, 0x00000000, 0 },
  { 6356,  1393, 19, 214937,     14.500, 0x00000000, 0 },
  { 6357,  1394,  0, 214938,      5.000, 0x00000000, 0 },
  { 6358,  1394,  1, 214938,      5.500, 0x00000000, 0 },
  { 6359,  1394,  2, 214938,      6.000, 0x00000000, 0 },
  { 6360,  1394,  3, 214938,      6.500, 0x00000000, 0 },
  { 6361,  1394,  4, 214938,      7.000, 0x00000000, 0 },
  { 6362,  1394,  5, 214938,      7.500, 0x00000000, 0 },
  { 6363,  1394,  6, 214938,      8.000, 0x00000000, 0 },
  { 6364,  1394,  7, 214938,      8.500, 0x00000000, 0 },
  { 6365,  1394,  8, 214938,      9.000, 0x00000000, 0 },
  { 6366,  1394,  9, 214938,      9.500, 0x00000000, 0 },
  { 6367,  1394, 10, 214938,     10.000, 0x00000000, 0 },
  { 6368,  1394, 11, 214938,     10.500, 0x00000000, 0 },
  { 6369,  1394, 12, 214938,     11.000, 0x00000000, 0 },
  { 6370,  1394, 13, 214938,     11.500, 0x00000000, 0 },
  { 6371,  1394, 14, 214938,     12.000, 0x00000000, 0 },
  { 6372,  1394, 15, 214938,     12.500, 0x00000000, 0 },
  { 6373,  1394, 16, 214938,     13.000, 0x00000000, 0 },
  { 6374,  1394, 17, 214938,     13.500, 0x00000000, 0 },
  { 6375,  1394, 18, 214938,     14.000, 0x00000000, 0 },
  { 6376,  1394, 19, 214938,     14.500, 0x00000000, 0 },
  { 6377,  1395,  0, 214939,     10.000, 0x00000000, 0 },
  { 6378,  1395,  1, 214939,     11.000, 0x00000000, 0 },
  { 6379,  1395,  2, 214939,     12.000, 0x00000000, 0 },
  { 6380,  1395,  3, 214939,     13.000, 0x00000000, 0 },
  { 6381,  1395,  4, 214939,     14.000, 0x00000000, 0 },
  { 6382,  1395,  5, 214939,     15.000, 0x00000000, 0 },
  { 6383,  1395,  6, 214939,     16.000, 0x00000000, 0 },
  { 6384,  1395,  7, 214939,     17.000, 0x00000000, 0 },
  { 6385,  1395,  8, 214939,     18.000, 0x00000000, 0 },
  { 6386,  1395,  9, 214939,     19.000, 0x00000000, 0 },
  { 6387,  1395, 10, 214939,     20.000, 0x00000000, 0 },
  { 6388,  1395, 11, 214939,     21.000, 0x00000000, 0 },
  { 6389,  1395, 12, 214939,     22.000, 0x00000000, 0 },
  { 6390,  1395, 13, 214939,     23.000, 0x00000000, 0 },
  { 6391,  1395, 14, 214939,     24.000, 0x00000000, 0 },
  { 6392,  1395, 15, 214939,     25.000, 0x00000000, 0 },
  { 6393,  1395, 16, 214939,     26.000, 0x00000000, 0 },
  { 6394,  1395, 17, 214939,     27.000, 0x00000000, 0 },
  { 6395,  1395, 18, 214939,     28.000, 0x00000000, 0 },
  { 6396,  1395, 19, 214939,     29.000, 0x00000000, 0 },
  { 6491,  1434,  0, 218910,      0.000, 0x00000000, 0 },
  { 6492,  1436,  0, 219655,      0.000, 0x00000000, 0 },
  { 6520,  1451,  0, 226829,      0.000, 0x00000000, 0 },
  { 6521,  1452,  0, 226829,      0.000, 0x00000000, 0 },
  { 6522,  1453,  0, 226829,      0.000, 0x00000000, 0 },
  { 6523,  1454,  0, 226829,      0.000, 0x00000000, 0 },
  { 6524,  1455,  0, 226829,      0.000, 0x00000000, 0 },
  { 6525,  1456,  0, 226829,      0.000, 0x00000000, 0 },
  {    0,     0,  0,      0,      0.000, 0x00000000, 0 },
};
#define PTR_PTR_ARTIFACT_POWER_RANK_HOTFIX_SIZE (0)

// 0 ptr_artifact_power_rank hotfix entries, wow build level 23038
static hotfix::client_hotfix_entry_t __ptr_ptr_artifact_power_rank_hotfix_data[] = {
  {      0,  0, 0, 0 },
};

