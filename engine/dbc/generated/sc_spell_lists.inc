#define CLASS_ABILITY_SIZE (29)
#define CLASS_ABILITY_TREE_SIZE (6)

#ifndef MAX_CLASS
#define MAX_CLASS (13)
#endif
// Class based active abilities, wow build 21134
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
    // General tree, 9 abilities
    {
      117313, // Bloodthirst Heal
         100, // Charge
      163558, // Execute Off-Hand
        6544, // Heroic Leap
       57755, // Heroic Throw
      118340, // Impending Victory (Talent)
        6552, // Pummel
      156287, // Ravager
      175710, // Unleashed Rage
           0,
    },
    // Arms tree, 12 abilities
    {
        1719, // Battle Cry
       18499, // Berserker Rage
         845, // Cleave
      167105, // Colossus Smash
       97462, // Commanding Shout
      197690, // Defensive Stance
      163201, // Execute
        1715, // Hamstring
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
      118038, // Die by the Sword
      184364, // Enraged Regeneration
        5308, // Execute
      100130, // Furious Slash
       12323, // Piercing Howl
       85288, // Raging Blow
      184367, // Rampage
      190411, // Whirlwind
           0,
    },
    // Protection tree, 14 abilities
    {
        1160, // Demoralizing Shout
       20243, // Devastate
      204488, // Heroic Strike
      190456, // Ignore Pain (Active Mitigation)
      198304, // Intercept
       12975, // Last Stand
        6572, // Revenge
        2565, // Shield Block (Active Mitigation)
       23922, // Shield Slam
         871, // Shield Wall
       23920, // Spell Reflection
         355, // Taunt
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
    // General tree, 13 abilities
    {
       86704, // Ancient Fury
      114919, // Arcing Light
       66235, // Ardent Defender
        1044, // Blessing of Freedom
      121183, // Contemplation
       35395, // Crusader Strike
         642, // Divine Shield
      181197, // Guiding Light
       96172, // Hand of Light
      162056, // Jailer's Judgment
       20271, // Judgment
         633, // Lay on Hands
      175699, // Weapons of the Light
           0,
    },
    // Holy tree, 16 abilities
    {
      212056, // Absolution
       31821, // Aura Mastery
       31842, // Avenging Wrath
       53563, // Beacon of Light
        1022, // Blessing of Protection
        6940, // Blessing of Sacrifice
        4987, // Cleanse
       26573, // Consecration
        2812, // Denounce
         498, // Divine Protection
       19750, // Flash of Light
         853, // Hammer of Justice
       82326, // Holy Light
       20473, // Holy Shock
       85222, // Light of Dawn
      183998, // Light of the Martyr
           0,
    },
    // Protection tree, 14 abilities
    {
       31850, // Ardent Defender
       31935, // Avenger's Shield
        1022, // Blessing of Protection
        6940, // Blessing of Sacrifice
      213644, // Cleanse Toxins
       26573, // Consecration
         498, // Divine Protection
       86659, // Guardian of Ancient Kings
         853, // Hammer of Justice
       53595, // Hammer of the Righteous
       62124, // Hand of Reckoning
      184092, // Light of the Protector
       96231, // Rebuke
       53600, // Shield of the Righteous (Active Mitigation)
           0,
    },
    // Retribution tree, 15 abilities
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
      183222, // Hand of Light
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
    // General tree, 11 abilities
    {
       61648, // Aspect of the Chameleon
      186257, // Aspect of the Cheetah
      186265, // Aspect of the Turtle
      193756, // Call Eagle Spirit
        5384, // Feign Death
      125050, // Fetch
      127933, // Fireworks
        1543, // Flare
      175687, // Heavy Artilery
      175686, // Stopping Power
      210000, // Wake Up
           0,
    },
    // Beast Mastery tree, 20 abilities
    {
      193530, // Aspect of the Wild
        1462, // Beast Lore
       19574, // Bestial Wrath
         883, // Call Pet 1
       83242, // Call Pet 2
       83243, // Call Pet 3
       83244, // Call Pet 4
       83245, // Call Pet 5
      193455, // Cobra Shot
        5116, // Concussive Shot
      147362, // Counter Shot
      120679, // Dire Beast
      109304, // Exhilaration
        6991, // Feed Pet
       34026, // Kill Command
         136, // Mend Pet
       34477, // Misdirection
        2643, // Multi-Shot
         982, // Revive Pet
        1515, // Tame Beast
           0,
    },
    // Marksmanship tree, 11 abilities
    {
       19434, // Aimed Shot
      185358, // Arcane Shot
      186387, // Bursting Shot
        5116, // Concussive Shot
      147362, // Counter Shot
      194291, // Exhilaration
      157708, // Kill Shot
      185901, // Marked Shot
       34477, // Misdirection
        2643, // Multi-Shot
      193526, // Trueshot
           0,
    },
    // Survival tree, 24 abilities
    {
      186289, // Aspect of the Eagle
        1462, // Beast Lore
         883, // Call Pet 1
       83242, // Call Pet 2
       83243, // Call Pet 3
       83244, // Call Pet 4
       83245, // Call Pet 5
      187708, // Carve
      109304, // Exhilaration
      191433, // Explosive Trap
        6991, // Feed Pet
      202800, // Flanking Strike
      187650, // Freezing Trap
      190925, // Harpoon
      193265, // Hatchet Toss
      185855, // Lacerate
         136, // Mend Pet
      190928, // Mongoose Bite
      187707, // Muzzle
      186270, // Raptor Strike
         982, // Revive Pet
        1515, // Tame Beast
      187698, // Tar Trap
      195645, // Wing Clip
           0,
    },
    {
           0,
    },
    // Pet tree, 29 abilities
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
    // General tree, 14 abilities
    {
       22482, // Blade Flurry
       31224, // Cloak of Shadows
      185311, // Crimson Vial
        2818, // Deadly Poison
       56814, // Detection
        5277, // Evasion
      193761, // Extraction Point
        1966, // Feint
        1766, // Kick
         408, // Kidney Shot
      112974, // Leeching Poison (Talent)
        1804, // Pick Lock
        2983, // Sprint
        8680, // Wound Poison
           0,
    },
    // Assassination tree, 16 abilities
    {
        1833, // Cheap Shot
        3408, // Crippling Poison
        2823, // Deadly Poison
        1725, // Distract
       32645, // Envenom
       51723, // Fan of Knives
         703, // Garrote
        1329, // Mutilate
      185565, // Poisoned Knife
        1943, // Rupture
        6770, // Sap
       36554, // Shadowstep
        1784, // Stealth
        1856, // Vanish
       79140, // Vendetta
        8679, // Wound Poison
           0,
    },
    // Outlaw tree, 15 abilities
    {
       13750, // Adrenaline Rush
        8676, // Ambush
      199804, // Between the Eyes
       13877, // Blade Flurry
        2094, // Blind
        1833, // Cheap Shot
        1725, // Distract
        1776, // Gouge
      185763, // Pistol Shot
      199754, // Riposte
      193316, // Roll the Bones
        2098, // Run Through
      193315, // Saber Slash
        1784, // Stealth
        1856, // Vanish
           0,
    },
    // Subtlety tree, 16 abilities
    {
          53, // Backstab
        2094, // Blind
        1833, // Cheap Shot
        1725, // Distract
      196819, // Eviscerate
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
    // General tree, 10 abilities
    {
      161862, // Ascension
         528, // Dispel Magic
      175702, // Divine Burst
         605, // Dominate Mind
       77489, // Echo of Light
         586, // Fade
       32375, // Mass Dispel
        9484, // Shackle Undead
      175701, // Shadowy Grasp
         585, // Smite
           0,
    },
    // Discipline tree, 15 abilities
    {
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
    // Holy tree, 16 abilities
    {
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
    // Shadow tree, 19 abilities
    {
       47585, // Dispersion
        1706, // Levitate
        8092, // Mind Blast
       15407, // Mind Flay
       48045, // Mind Sear
          17, // Power Word: Shield
       64044, // Psychic Horror
        8122, // Psychic Scream
      213634, // Purify Disease
      179338, // Searing Insanity
      186263, // Shadow Mend
       32379, // Shadow Word: Death
         589, // Shadow Word: Pain
       34433, // Shadowfiend
       15487, // Silence
       15286, // Vampiric Embrace
       34914, // Vampiric Touch
      205448, // Void Bolt
      194249, // Voidform
           0,
    },
    {
           0,
    },
    // Pet tree, 1 abilities
    {
       63619, // Shadowcrawl
           0,
    },
  },
  // Class active abilities for Death Knight
  {
    // General tree, 12 abilities
    {
       48707, // Anti-Magic Shell
      111673, // Control Undead
      127344, // Corpse Explosion
       50977, // Death Gate
       49576, // Death Grip
      175678, // Death Heal
       49998, // Death Strike (Active Mitigation)
       47568, // Empower Rune Weapon
       57330, // Horn of Winter
       47528, // Mind Freeze
        3714, // Path of Frost
       61999, // Raise Ally
           0,
    },
    // Blood tree, 10 abilities
    {
       50842, // Blood Boil
       45902, // Blood Strike
       49028, // Dancing Rune Weapon
       56222, // Dark Command
       43265, // Death and Decay
      195292, // Death's Caress
      108199, // Gorefiend's Grasp
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
    // Pet tree, 6 abilities
    {
       81280, // Blood Burst
       47468, // Claw
       51963, // Gargoyle Strike
       47481, // Gnaw
       47484, // Huddle
       47482, // Leap
           0,
    },
  },
  // Class active abilities for Shaman
  {
    // General tree, 11 abilities
    {
      114911, // Ancestral Guidance
        2008, // Ancestral Spirit
      108271, // Astral Shift
        2825, // Bloodlust
        2645, // Ghost Wolf
       32182, // Heroism
       51514, // Hex
      210873, // Hex
         370, // Purge
      193755, // Stormcall
      115356, // Windstrike
           0,
    },
    // Elemental tree, 13 abilities
    {
      188443, // Chain Lightning
       51886, // Cleanse Spirit
      198103, // Earth Elemental
        8042, // Earth Shock
       61882, // Earthquake Totem
      198067, // Fire Elemental
      188389, // Flame Shock
      196840, // Frost Shock
      188070, // Healing Surge
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
      108280, // Healing Tide Totem (Water Totem)
       77472, // Healing Wave
       51505, // Lava Burst
         403, // Lightning Bolt
       77130, // Purify Spirit
       61295, // Riptide
       98008, // Spirit Link Totem (Air Totem)
       79206, // Spiritwalker's Grace
       57994, // Wind Shear
           0,
    },
    {
           0,
    },
    // Pet tree, 10 abilities
    {
       36213, // Angered Earth
      157348, // Call Lightning
      118350, // Empower
       57984, // Fire Blast
      117588, // Fire Nova
      118337, // Harden Skin
      118297, // Immolate
      118345, // Pulverize
      118347, // Reinforce
      157331, // Wind Gust
           0,
    },
  },
  // Class active abilities for Mage
  {
    // General tree, 11 abilities
    {
      120145, // Ancient Teleport: Dalaran
        1953, // Blink
      190336, // Conjure Refreshment
        2139, // Counterspell
      175689, // Freezing Grasp
         122, // Frost Nova
       11426, // Ice Barrier
       45438, // Ice Block
      131784, // Illusion
      125430, // Mage Bomb
       80353, // Time Warp
           0,
    },
    // Arcane tree, 9 abilities
    {
       44425, // Arcane Barrage
       30451, // Arcane Blast
        1449, // Arcane Explosion
        5143, // Arcane Missiles
       12042, // Arcane Power
      195676, // Displacement
       12051, // Evocation
      110959, // Greater Invisibility
       31589, // Slow
           0,
    },
    // Fire tree, 8 abilities
    {
      190319, // Combustion
       31661, // Dragon's Breath
         133, // Fireball
        2120, // Flamestrike
      108853, // Inferno Blast
          66, // Invisibility
       11366, // Pyroblast
        2948, // Scorch
           0,
    },
    // Frost tree, 8 abilities
    {
      190356, // Blizzard
         120, // Cone of Cold
         116, // Frostbolt
       84714, // Frozen Orb
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
       88084, // Arcane Blast
       88082, // Fireball
       33395, // Freeze
       59638, // Frostbolt
      155152, // Prismatic Crystal
       31707, // Waterbolt
           0,
    },
  },
  // Class active abilities for Warlock
  {
    // General tree, 25 abilities
    {
         710, // Banish
      119898, // Command Demon
         172, // Corruption
        6201, // Create Healthstone
      111771, // Demonic Gateway
         689, // Drain Life
        1098, // Enslave Demon
        5782, // Fear
      157900, // Grimoire: Doomguard (Summon)
      111897, // Grimoire: Felhunter (Summon)
      111859, // Grimoire: Imp (Summon)
      157901, // Grimoire: Infernal (Summon)
      111896, // Grimoire: Succubus (Summon)
      111895, // Grimoire: Voidwalker (Summon)
         755, // Health Funnel
      108396, // Mortal Coil (Talent)
       63106, // Siphon Life
       20707, // Soulstone
       18540, // Summon Doomguard (Guardian)
         691, // Summon Felhunter (Summon)
         688, // Summon Imp (Summon)
        1122, // Summon Infernal (Guardian)
         712, // Summon Succubus (Summon)
         697, // Summon Voidwalker (Summon)
      104773, // Unending Resolve
           0,
    },
    // Affliction tree, 4 abilities
    {
         980, // Agony
        1454, // Life Tap
       27243, // Seed of Corruption
       30108, // Unstable Affliction
           0,
    },
    // Demonology tree, 9 abilities
    {
      104316, // Call Dreadstalkers
      193396, // Demonic Empowerment
      193440, // Demonwrath
         603, // Doom
      111898, // Grimoire: Felguard (Summon)
      105174, // Hand of Gul'dan
        1454, // Life Tap
         686, // Shadow Bolt
       30146, // Summon Felguard (Summon)
           0,
    },
    // Destruction tree, 7 abilities
    {
      116858, // Chaos Bolt
       17962, // Conflagrate
       80240, // Havoc
         348, // Immolate
       29722, // Incinerate
        1454, // Life Tap
        5740, // Rain of Fire
           0,
    },
    {
           0,
    },
    // Pet tree, 27 abilities
    {
       89766, // Axe Toss (Special Ability)
      171011, // Burning Presence (Special Ability)
      119899, // Cauterize Master (Special Ability)
        3716, // Consuming Shadows (Basic Attack)
      170995, // Cripple
       19658, // Devour Magic
       85692, // Doom Bolt
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
      112042, // Threatening Presence (Special Ability)
      134477, // Threatening Presence (Special Ability)
      171021, // Torch Magic (Special Ability)
        6360, // Whiplash (Special Ability)
           0,
    },
  },
  // Class active abilities for Monk
  {
    // General tree, 9 abilities
    {
      100784, // Blackout Kick
      117952, // Crackling Jade Lightning
      115078, // Paralysis
      109132, // Roll
      100780, // Tiger Palm
      101643, // Transcendence
      119996, // Transcendence: Transfer
      125883, // Zen Flight
      126892, // Zen Pilgrimage
           0,
    },
    // Brewmaster tree, 11 abilities
    {
      205523, // Blackout Strike
      115181, // Breath of Fire
      116694, // Effuse
      195630, // Elusive Brawler
      115203, // Fortifying Brew
      115308, // Ironskin Brew (Active Mitigation)
      121253, // Keg Smash
      115546, // Provoke
      119582, // Purifying Brew (Active Mitigation)
      116705, // Spear Hand Strike
      115176, // Zen Meditation
           0,
    },
    // Mistweaver tree, 12 abilities
    {
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
    // Windwalker tree, 12 abilities
    {
      116768, // Blackout Kick!
      116095, // Disable
      116694, // Effuse
      113656, // Fists of Fury
      101545, // Flying Serpent Kick
      107428, // Rising Sun Kick
      116705, // Spear Hand Strike
      101546, // Spinning Crane Kick
      137639, // Storm, Earth, and Fire
      116740, // Tigereye Brew
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
    // General tree, 15 abilities
    {
        5487, // Bear Form (Shapeshift)
         768, // Cat Form (Shapeshift)
      127757, // Charm Woodland Creature
      193753, // Dreamwalk
         339, // Entangling Roots
      164862, // Flap
      165962, // Flight Form (Shapeshift)
        8921, // Moonfire (Lunar)
      164812, // Moonfire (Lunar)
      175682, // Nature's Control
       20484, // Rebirth
      210053, // Stag Form (Shapeshift)
      164815, // Sunfire (Solar)
         783, // Travel Form (Shapeshift)
      175683, // Ursol's Power
           0,
    },
    // Balance tree, 17 abilities
    {
       22812, // Barkskin (Active Mitigation)
      194223, // Celestial Alignment
      122114, // Chosen of Elune
      202771, // Full Moon
      203001, // Goldrinn's Fang
      202768, // Half Moon
        5185, // Healing Touch
       29166, // Innervate
      194153, // Lunar Strike (Lunar)
       24858, // Moonkin Form
        5215, // Prowl
        2782, // Remove Corruption
       78675, // Solar Beam
      190984, // Solar Wrath (Solar)
      191034, // Starfall
       78674, // Starsurge
       93402, // Sunfire (Solar)
           0,
    },
    // Feral tree, 23 abilities
    {
      210713, // Ashamane's Rake
      210705, // Ashamane's Rip
      106951, // Berserk
      145152, // Bloodtalons
        1850, // Dash
      210649, // Feral Instinct
      146874, // Feral Rage
       22568, // Ferocious Bite
        5185, // Healing Touch
       22570, // Maim
      155625, // Moonfire
        5215, // Prowl
        1822, // Rake
        2782, // Remove Corruption
        1079, // Rip
        5221, // Shred
      210721, // Shredded Wounds
      106839, // Skull Bash
      106898, // Stampeding Roar
       61336, // Survival Instincts (Active Mitigation)
      213764, // Swipe
      106832, // Thrash
        5217, // Tiger's Fury
           0,
    },
    // Guardian tree, 18 abilities
    {
       22812, // Barkskin (Active Mitigation)
      203958, // Brambles
       22842, // Frenzied Regeneration
        6795, // Growl
        5185, // Healing Touch
          99, // Incapacitating Roar
      192081, // Ironfur (Active Mitigation)
       33917, // Mangle
      192083, // Mark of Ursol (Active Mitigation)
        6807, // Maul
        5215, // Prowl
        2782, // Remove Corruption
      106839, // Skull Bash
      185321, // Stalwart Guardian
      106898, // Stampeding Roar
       61336, // Survival Instincts (Active Mitigation)
      213764, // Swipe
      106832, // Thrash
           0,
    },
    // Restoration tree, 18 abilities
    {
       22812, // Barkskin (Active Mitigation)
      145205, // Efflorescence
        5185, // Healing Touch
       16914, // Hurricane
       29166, // Innervate
      102342, // Ironbark
       33763, // Lifebloom
       88423, // Nature's Cure
        5215, // Prowl
        8936, // Regrowth
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
    // General tree, 14 abilities
    {
      188499, // Blade Dance
      198589, // Blur
      179057, // Chaos Nova
      162794, // Chaos Strike
      183752, // Consume Magic
      162243, // Demon's Bite
      198013, // Eye Beam
      195072, // Fel Rush
      193752, // Gateway: The Fel Hammer
      131347, // Glide
      191427, // Metamorphosis
      188501, // Spectral Sight
      185123, // Throw Glaive
      198793, // Vengeful Retreat
           0,
    },
    {
           0,
    },
    // Vengeance tree, 12 abilities
    {
      203720, // Demon Spikes (Active Mitigation)
      204021, // Fiery Brand
      178740, // Immolation Aura
      189110, // Infernal Strike
      187827, // Metamorphosis
      203782, // Shear
      202138, // Sigil of Chains
      204596, // Sigil of Flame
      202137, // Sigil of Silence
      203798, // Soul Cleave
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
#define TREE_SPECIALIZATION_SIZE (32)

// Talent tree specialization abilities, wow build 21134
static unsigned __tree_specialization_data[][MAX_SPECS_PER_CLASS][TREE_SPECIALIZATION_SIZE] = {
  // Specialization abilities for Hunter pets
  {
    // Specialization abilities for Ferocity
    {
       53482, // Blood of the Rhino
       19596, // Boar's Speed
       20782, // Combat Experience
       20782, // Combat Experience
       53497, // Cornered
       61684, // Dash
       61684, // Dash
       61688, // Great Stamina
       55709, // Heart of the Phoenix
       55709, // Heart of the Phoenix
       53184, // Spiked Collar
       53184, // Spiked Collar
           0,
    },
    // Specialization abilities for Tenacity
    {
       53482, // Blood of the Rhino
       53482, // Blood of the Rhino
       19596, // Boar's Speed
       61685, // Charge
       61685, // Charge
       20782, // Combat Experience
       20782, // Combat Experience
       53497, // Cornered
       61688, // Great Stamina
       61688, // Great Stamina
       53478, // Last Stand
       53478, // Last Stand
       53184, // Spiked Collar
       63900, // Thunderstomp
       63900, // Thunderstomp
           0,
    },
    // Specialization abilities for Cunning
    {
       53482, // Blood of the Rhino
       19596, // Boar's Speed
       19596, // Boar's Speed
       53490, // Bullheaded
       53490, // Bullheaded
       20782, // Combat Experience
       20782, // Combat Experience
       53497, // Cornered
       53497, // Cornered
       61684, // Dash
       61684, // Dash
       61688, // Great Stamina
       53480, // Roar of Sacrifice
       53480, // Roar of Sacrifice
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
      137049, // Arms Warrior
        1719, // Battle Cry
       18499, // Berserker Rage
         845, // Cleave
      167105, // Colossus Smash
       97462, // Commanding Shout
      197690, // Defensive Stance
      163201, // Execute
        1715, // Hamstring
       76838, // Mastery: Colossal Might
       12294, // Mortal Strike
       86101, // Plate Specialization
       12712, // Seasoned Soldier
        1464, // Slam
      162698, // Stat Negation Aura - Strength DPS
      184783, // Tactician
       34428, // Victory Rush
        1680, // Whirlwind
           0,
    },
    // Specialization abilities for Fury
    {
        1719, // Battle Cry
       18499, // Berserker Rage
       23881, // Bloodthirst
       97462, // Commanding Shout
      118038, // Die by the Sword
      184361, // Enrage
      184364, // Enraged Regeneration
        5308, // Execute
      100130, // Furious Slash
      137050, // Fury Warrior
       76856, // Mastery: Unshackled Fury
       12950, // Meat Cleaver
       12323, // Piercing Howl
       86110, // Plate Specialization
       85288, // Raging Blow
      184367, // Rampage
      209694, // Rampage!
       81099, // Single-Minded Fury
      162698, // Stat Negation Aura - Strength DPS
       46917, // Titan's Grip
      190411, // Whirlwind
           0,
    },
    // Specialization abilities for Protection
    {
       84608, // Bastion of Defense
      161608, // Bladed Armor
      115768, // Deep Wounds
          71, // Defensive Stance
        1160, // Demoralizing Shout
       20243, // Devastate
      204488, // Heroic Strike
      190456, // Ignore Pain
      198304, // Intercept
       12975, // Last Stand
       76857, // Mastery: Critical Block
       86535, // Plate Specialization
      131086, // Protection
      137048, // Protection Warrior
        6572, // Revenge
      161798, // Riposte
        2565, // Shield Block
       23922, // Shield Slam
         871, // Shield Wall
       23920, // Spell Reflection
      162702, // Stat Negation Aura - Strength Tank
       46953, // Sword and Board
         355, // Taunt
        6343, // Thunder Clap
       29144, // Unwavering Sentinel
       34428, // Victory Rush
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
      212056, // Absolution
       31821, // Aura Mastery
       31842, // Avenging Wrath
       53563, // Beacon of Light
        1022, // Blessing of Protection
        6940, // Blessing of Sacrifice
        4987, // Cleanse
       26573, // Consecration
        2812, // Denounce
      184132, // Divine Intervention
         498, // Divine Protection
       19750, // Flash of Light
         853, // Hammer of Justice
       82326, // Holy Light
      137029, // Holy Paladin
       20473, // Holy Shock
       53576, // Infusion of Light
       85222, // Light of Dawn
      183998, // Light of the Martyr
       53651, // Light's Beacon
      183997, // Mastery: Lightbringer
       86103, // Plate Specialization
      148040, // Sacred Shield Overridde (Holy)
      162701, // Stat Negation Aura - Intellect Healer
           0,
    },
    // Specialization abilities for Protection
    {
       31850, // Ardent Defender
       31935, // Avenger's Shield
      161608, // Bladed Armor
        1022, // Blessing of Protection
        6940, // Blessing of Sacrifice
      213644, // Cleanse Toxins
       26573, // Consecration
         498, // Divine Protection
      190784, // Divine Steed
       85043, // Grand Crusader
       53592, // Guarded by the Light
       86659, // Guardian of Ancient Kings
         853, // Hammer of Justice
       53595, // Hammer of the Righteous
       62124, // Hand of Reckoning
      157488, // Improved Block
      105424, // Judgments of the Wise
      184092, // Light of the Protector
       76671, // Mastery: Divine Bulwark
       86102, // Plate Specialization
      137028, // Protection Paladin
       96231, // Rebuke
      190774, // Redoubt
       25780, // Righteous Fury
      161800, // Riposte
      105805, // Sanctuary
       53600, // Shield of the Righteous
      162702, // Stat Negation Aura - Strength Tank
           0,
    },
    // Specialization abilities for Retribution
    {
       31884, // Avenging Wrath
      184575, // Blade of Justice
        1022, // Blessing of Protection
      213644, // Cleanse Toxins
      185817, // Conviction
       53385, // Divine Storm
       19750, // Flash of Light
      203538, // Greater Blessing of Kings
      203528, // Greater Blessing of Might
      203539, // Greater Blessing of Wisdom
         853, // Hammer of Justice
      183218, // Hand of Hindrance
      183222, // Hand of Light
       76672, // Mastery: Divine Judgment
       86539, // Plate Specialization
       96231, // Rebuke
      183435, // Retribution
      137027, // Retribution Paladin
      184662, // Shield of Vengeance
      162698, // Stat Negation Aura - Strength DPS
       53503, // Sword of Light
       20113, // Sword of Light
       85256, // Templar's Verdict
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
      193530, // Aspect of the Wild
      115939, // Beast Cleave
        1462, // Beast Lore
      137015, // Beast Mastery Hunter
       19574, // Bestial Wrath
         883, // Call Pet 1
       83242, // Call Pet 2
       83243, // Call Pet 3
       83244, // Call Pet 4
       83245, // Call Pet 5
      193455, // Cobra Shot
        5116, // Concussive Shot
       93321, // Control Pet
       93322, // Control Pet
      147362, // Counter Shot
      120679, // Dire Beast
         781, // Disengage
        2641, // Dismiss Pet
      109304, // Exhilaration
       53270, // Exotic Beasts
        6991, // Feed Pet
       34954, // Go for the Throat
       34026, // Kill Command
       56315, // Kindred Spirits
       76657, // Mastery: Master of Beasts
         136, // Mend Pet
       34477, // Misdirection
        2643, // Multi-Shot
         982, // Revive Pet
      162697, // Stat Negation Aura - Agility DPS
        1515, // Tame Beast
      185789, // Wild Call
    },
    // Specialization abilities for Marksmanship
    {
       19434, // Aimed Shot
      185358, // Arcane Shot
       35110, // Bombardment
      186387, // Bursting Shot
        5116, // Concussive Shot
      147362, // Counter Shot
         781, // Disengage
      194291, // Exhilaration
      212658, // Hunting Party
      155228, // Lone Wolf
      185901, // Marked Shot
      137016, // Marksmanship Hunter
      193468, // Mastery: Sniper Training
       34477, // Misdirection
        2643, // Multi-Shot
      185987, // Seek Vulnerabilities
      162697, // Stat Negation Aura - Agility DPS
      193526, // Trueshot
           0,
    },
    // Specialization abilities for Survival
    {
      186289, // Aspect of the Eagle
        1462, // Beast Lore
         883, // Call Pet 1
       83242, // Call Pet 2
       83243, // Call Pet 3
       83244, // Call Pet 4
       83245, // Call Pet 5
      187708, // Carve
       93321, // Control Pet
       93322, // Control Pet
        2641, // Dismiss Pet
      109304, // Exhilaration
      191433, // Explosive Trap
        6991, // Feed Pet
      202800, // Flanking Strike
      187650, // Freezing Trap
      190925, // Harpoon
      193265, // Hatchet Toss
      185855, // Lacerate
      191334, // Mastery: Hunting Companion
         136, // Mend Pet
      190928, // Mongoose Bite
      187707, // Muzzle
      186270, // Raptor Strike
         982, // Revive Pet
      162697, // Stat Negation Aura - Agility DPS
      137017, // Survival Hunter
      164856, // Survivalist
        1515, // Tame Beast
      187698, // Tar Trap
      195645, // Wing Clip
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
      137037, // Assassination Rogue
        1833, // Cheap Shot
        3408, // Crippling Poison
       51667, // Cut to the Chase
        2823, // Deadly Poison
        1725, // Distract
       32645, // Envenom
       51723, // Fan of Knives
         703, // Garrote
       14117, // Improved Poisons
       76803, // Mastery: Potent Poisons
        1329, // Mutilate
         921, // Pick Pocket
      185565, // Poisoned Knife
        1943, // Rupture
        6770, // Sap
       14190, // Seal Fate
       36554, // Shadowstep
      162697, // Stat Negation Aura - Agility DPS
        1784, // Stealth
        1856, // Vanish
       79140, // Vendetta
       79134, // Venomous Wounds
        8679, // Wound Poison
           0,
    },
    // Specialization abilities for Outlaw
    {
       13750, // Adrenaline Rush
        8676, // Ambush
      199804, // Between the Eyes
       13877, // Blade Flurry
        2094, // Blind
      199740, // Bribe
        1833, // Cheap Shot
       35551, // Combat Potency
        1725, // Distract
      199736, // Find Treasure
        1776, // Gouge
       76806, // Mastery: Main Gauche
      137036, // Outlaw Rogue
         921, // Pick Pocket
      185763, // Pistol Shot
      199754, // Riposte
      193316, // Roll the Bones
        2098, // Run Through
       14161, // Ruthlessness
      193315, // Saber Slash
      162697, // Stat Negation Aura - Agility DPS
        1784, // Stealth
      157660, // Swashbuckler
        1856, // Vanish
       61329, // Vitality
           0,
    },
    // Specialization abilities for Subtlety
    {
          53, // Backstab
        2094, // Blind
        1833, // Cheap Shot
      185314, // Deepening Shadows
        1725, // Distract
       79152, // Energetic Recovery
      196819, // Eviscerate
      196976, // Master of Shadows
       76808, // Mastery: Executioner
      195452, // Nightblade
         921, // Pick Pocket
        6770, // Sap
      121471, // Shadow Blades
      185313, // Shadow Dance
      196912, // Shadow Techniques
       36554, // Shadowstep
      185438, // Shadowstrike
      197835, // Shuriken Storm
      114014, // Shuriken Toss
      162697, // Stat Negation Aura - Agility DPS
        1784, // Stealth
      137035, // Subtlety Rogue
      212283, // Symbols of Death
        1856, // Vanish
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
       81749, // Atonement
      137032, // Discipline Priest
       45243, // Focused Will
       73325, // Leap of Faith
        1706, // Levitate
      212036, // Mass Resurrection
       77484, // Mastery: Absolution
        2096, // Mind Vision
       33206, // Pain Suppression
       47540, // Penance
      200829, // Plea
       62618, // Power Word: Barrier
      194509, // Power Word: Radiance
          17, // Power Word: Shield
        8122, // Psychic Scream
         527, // Purify
       47536, // Rapture
      186440, // Shadow Mend
      186263, // Shadow Mend
         589, // Shadow Word: Pain
       34433, // Shadowfiend
      162701, // Stat Negation Aura - Intellect Healer
           0,
    },
    // Specialization abilities for Holy
    {
       64843, // Divine Hymn
        2061, // Flash Heal
       45243, // Focused Will
       47788, // Guardian Spirit
        2060, // Heal
       14914, // Holy Fire
      132157, // Holy Nova
      137031, // Holy Priest
       88625, // Holy Word: Chastise
       34861, // Holy Word: Sanctify
        2050, // Holy Word: Serenity
       73325, // Leap of Faith
        1706, // Levitate
      212036, // Mass Resurrection
       77485, // Mastery: Echo of Light
         596, // Prayer of Healing
       33076, // Prayer of Mending
         527, // Purify
         139, // Renew
       63733, // Serendipity
       20711, // Spirit of Redemption
      162701, // Stat Negation Aura - Intellect Healer
           0,
    },
    // Specialization abilities for Shadow
    {
       47585, // Dispersion
      194248, // Insanity Visual Controller
        1706, // Levitate
       77486, // Mastery: Madness
        8092, // Mind Blast
       15407, // Mind Flay
       48045, // Mind Sear
        2096, // Mind Vision
          17, // Power Word: Shield
       64044, // Psychic Horror
        8122, // Psychic Scream
      213634, // Purify Disease
      186263, // Shadow Mend
      186440, // Shadow Mend
      137033, // Shadow Priest
      190719, // Shadow Word: Death
       32379, // Shadow Word: Death
         589, // Shadow Word: Pain
       34433, // Shadowfiend
       78203, // Shadowy Apparitions
       15487, // Silence
       15286, // Vampiric Embrace
       34914, // Vampiric Touch
      205448, // Void Bolt
      185916, // Voidform
      194249, // Voidform
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
       50842, // Blood Boil
      137008, // Blood Death Knight
      195740, // Blood Plague
       45902, // Blood Strike
       81136, // Crimson Scourge
       49028, // Dancing Rune Weapon
       56222, // Dark Command
       43265, // Death and Decay
      195292, // Death's Caress
      108199, // Gorefiend's Grasp
      195182, // Marrowrend
       77513, // Mastery: Blood Shield
       86537, // Plate Specialization
      161797, // Riposte
      162702, // Stat Negation Aura - Strength Tank
       55233, // Vampiric Blood
       48263, // Veteran of the Third War
      212552, // Wraith Walk
           0,
    },
    // Specialization abilities for Frost
    {
       45524, // Chains of Ice
      178819, // Dark Succor
         674, // Dual Wield
       47568, // Empower Rune Weapon
      137006, // Frost Death Knight
      195621, // Frost Fever
       49143, // Frost Strike
       49184, // Howling Blast
       48792, // Icebound Fortitude
       51128, // Killing Machine
       77514, // Mastery: Frozen Heart
       49020, // Obliterate
       51271, // Pillar of Frost
       86113, // Plate Specialization
      196770, // Remorseless Winter
       59057, // Rime
       81229, // Runic Empowerment
      162698, // Stat Negation Aura - Strength DPS
      212552, // Wraith Walk
           0,
    },
    // Specialization abilities for Unholy
    {
       42650, // Army of the Dead
       45524, // Chains of Ice
      178819, // Dark Succor
       63560, // Dark Transformation
       47541, // Death Coil
       43265, // Death and Decay
       48265, // Death's Advance
       85948, // Festering Strike
      197147, // Festering Wound
       48792, // Icebound Fortitude
       77515, // Mastery: Dreadblade
       77575, // Outbreak
       86536, // Plate Specialization
       46584, // Raise Dead
       51462, // Runic Corruption
       55090, // Scourge Strike
      162698, // Stat Negation Aura - Strength DPS
       49530, // Sudden Doom
       49206, // Summon Gargoyle
      137007, // Unholy Death Knight
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
      188443, // Chain Lightning
       51886, // Cleanse Spirit
      198103, // Earth Elemental
        8042, // Earth Shock
       61882, // Earthquake Totem
       16164, // Elemental Focus
       60188, // Elemental Fury
      137040, // Elemental Shaman
      198067, // Fire Elemental
      188389, // Flame Shock
      196840, // Frost Shock
      190493, // Fulmination
      188070, // Healing Surge
       51505, // Lava Burst
       77756, // Lava Surge
      188196, // Lightning Bolt
      190488, // Maelstrom Controller
       86108, // Mail Specialization
      168534, // Mastery: Elemental Overload
      123099, // Spiritual Insight
      162699, // Stat Negation Aura - Intellect DPS
       51490, // Thunderstorm
       57994, // Wind Shear
           0,
    },
    // Specialization abilities for Enhancement
    {
       51886, // Cleanse Spirit
      187874, // Crash Lightning
      157444, // Critical Strikes
       86629, // Dual Wield
      137041, // Enhancement Shaman
       51533, // Feral Spirit
      193796, // Flametongue
       16282, // Flurry
      196834, // Frostbrand
      188070, // Healing Surge
       60103, // Lava Lash
      187837, // Lightning Bolt
      195771, // Maelstrom Controller Decreasing
      190899, // Maelstrom Controller Increasing
      187880, // Maelstrom Weapon
       86099, // Mail Specialization
       77223, // Mastery: Enhanced Elements
       51522, // Primal Wisdom
      193786, // Rockbiter
       58875, // Spirit Walk
      162697, // Stat Negation Aura - Agility DPS
      201845, // Stormfury
      195255, // Stormlash
       17364, // Stormstrike
       57994, // Wind Shear
       33757, // Windfury
           0,
    },
    // Specialization abilities for Restoration
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
       77756, // Lava Surge
         403, // Lightning Bolt
       86100, // Mail Specialization
       77226, // Mastery: Deep Healing
       77130, // Purify Spirit
      137039, // Restoration Shaman
       16196, // Resurgence
       61295, // Riptide
       98008, // Spirit Link Totem
       79206, // Spiritwalker's Grace
      162701, // Stat Negation Aura - Intellect Healer
       51564, // Tidal Waves
       57994, // Wind Shear
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
       44425, // Arcane Barrage
       30451, // Arcane Blast
      190427, // Arcane Charge
      114664, // Arcane Charge
        1449, // Arcane Explosion
      137021, // Arcane Mage
        5143, // Arcane Missiles
       79684, // Arcane Missiles
       12042, // Arcane Power
      195676, // Displacement
       12051, // Evocation
      110959, // Greater Invisibility
        6117, // Mage Armor
      190740, // Mastery: Savant
       31589, // Slow
      162699, // Stat Negation Aura - Intellect DPS
           0,
    },
    // Specialization abilities for Fire
    {
      190319, // Combustion
      117216, // Critical Mass
       31661, // Dragon's Breath
      157642, // Enhanced Pyrotechnics
      137019, // Fire Mage
         133, // Fireball
        2120, // Flamestrike
      195283, // Hot Streak
      108853, // Inferno Blast
          66, // Invisibility
       12846, // Mastery: Ignite
       30482, // Molten Armor
       11366, // Pyroblast
       44448, // Pyroblast Clearcasting Driver
        2948, // Scorch
      162699, // Stat Negation Aura - Intellect DPS
           0,
    },
    // Specialization abilities for Frost
    {
      190356, // Blizzard
      190447, // Brain Freeze
         120, // Cone of Cold
      112965, // Fingers of Frost
        7302, // Frost Armor
      137020, // Frost Mage
         116, // Frostbolt
       84714, // Frozen Orb
       30455, // Ice Lance
       12472, // Icy Veins
      157727, // Improved Blizzard
          66, // Invisibility
       76613, // Mastery: Icicles
       12982, // Shatter
      162699, // Stat Negation Aura - Intellect DPS
       31687, // Summon Water Elemental
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
      137043, // Affliction Warlock
         980, // Agony
        1454, // Life Tap
       77215, // Mastery: Potent Afflictions
      205183, // Secrets of the Necrolyte
       27243, // Seed of Corruption
      162699, // Stat Negation Aura - Intellect DPS
       30108, // Unstable Affliction
           0,
    },
    // Specialization abilities for Demonology
    {
      104316, // Call Dreadstalkers
      193396, // Demonic Empowerment
      137044, // Demonology Warlock
      193440, // Demonwrath
         603, // Doom
      105174, // Hand of Gul'dan
        1454, // Life Tap
       77219, // Mastery: Master Demonologist
         686, // Shadow Bolt
      108415, // Soul Link
      162699, // Stat Negation Aura - Intellect DPS
       30146, // Summon Felguard
           0,
    },
    // Specialization abilities for Destruction
    {
      116858, // Chaos Bolt
      193535, // Chaotic Energy
       17962, // Conflagrate
      137046, // Destruction Warlock
       80240, // Havoc
         348, // Immolate
      193541, // Immolate
       29722, // Incinerate
        1454, // Life Tap
       77220, // Mastery: Chaotic Energies
        5740, // Rain of Fire
      162699, // Stat Negation Aura - Intellect DPS
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
      205523, // Blackout Strike
      161608, // Bladed Armor
      115181, // Breath of Fire
      137023, // Brewmaster Monk
      157445, // Critical Strikes
      116694, // Effuse
      115203, // Fortifying Brew
      124502, // Gift of the Ox
      115308, // Ironskin Brew
      121253, // Keg Smash
      120225, // Leather Specialization
      117906, // Mastery: Elusive Brawler
      115546, // Provoke
      119582, // Purifying Brew
      116705, // Spear Hand Strike
      115069, // Stagger
      162700, // Stat Negation Aura - Agility Tank
      115176, // Zen Meditation
           0,
    },
    // Specialization abilities for Mistweaver
    {
      115450, // Detox
      116694, // Effuse
      124682, // Enveloping Mist
      191837, // Essence Font
      120224, // Leather Specialization
      116849, // Life Cocoon
      117907, // Mastery: Gust of Mists
      137024, // Mistweaver Monk
      212051, // Reawaken
      115151, // Renewing Mist
      115310, // Revival
      107428, // Rising Sun Kick
      193884, // Soothing Mist
      101546, // Spinning Crane Kick
      162701, // Stat Negation Aura - Intellect Healer
      116645, // Teachings of the Monastery
      116680, // Thunder Focus Tea
      116670, // Vivify
           0,
    },
    // Specialization abilities for Windwalker
    {
      116092, // Afterlife
      128595, // Combat Conditioning
      137384, // Combo Breaker
      157445, // Critical Strikes
      116095, // Disable
      124146, // Dual Wield
      116694, // Effuse
      113656, // Fists of Fury
      101545, // Flying Serpent Kick
      120227, // Leather Specialization
      115636, // Mastery: Combo Strikes
      107428, // Rising Sun Kick
      116705, // Spear Hand Strike
      101546, // Spinning Crane Kick
      103985, // Stance of the Fierce Tiger
      162697, // Stat Negation Aura - Agility DPS
      137639, // Storm, Earth, and Fire
      116740, // Tigereye Brew
      115080, // Touch of Death
      122470, // Touch of Karma
      137025, // Windwalker Monk
      157411, // Windwalking
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
      197524, // Astral Influence
      137013, // Balance Druid
      106732, // Balance Overrides Passive
       22812, // Barkskin
      194223, // Celestial Alignment
        5185, // Healing Touch
       29166, // Innervate
      108299, // Killer Instinct
       86104, // Leather Specialization
      194153, // Lunar Strike
       77492, // Mastery: Starlight
       24858, // Moonkin Form
      112857, // Natural Insight
        5215, // Prowl
        2782, // Remove Corruption
       78675, // Solar Beam
      190984, // Solar Wrath
      191034, // Starfall
       78674, // Starsurge
      162699, // Stat Negation Aura - Intellect DPS
       93402, // Sunfire
           0,
    },
    // Specialization abilities for Feral
    {
      106951, // Berserk
      157447, // Critical Strikes
        1850, // Dash
      131768, // Feline Swiftness
      137011, // Feral Druid
       16949, // Feral Instinct
      106733, // Feral Overrides Passive
      197692, // Feral Overrides Passive
       22568, // Ferocious Bite
        5185, // Healing Touch
       48484, // Infected Wounds
       86097, // Leather Specialization
       22570, // Maim
      166142, // Mana Attunement
       77493, // Mastery: Razor Claws
       33873, // Nurturing Instinct
       16864, // Omen of Clarity
       16974, // Predatory Swiftness
      159286, // Primal Fury
        5215, // Prowl
        1822, // Rake
        2782, // Remove Corruption
        1079, // Rip
        5221, // Shred
      106839, // Skull Bash
      106898, // Stampeding Roar
      162697, // Stat Negation Aura - Agility DPS
       61336, // Survival Instincts
      213764, // Swipe
      106832, // Thrash
        5217, // Tiger's Fury
           0,
    },
    // Specialization abilities for Guardian
    {
       22812, // Barkskin
      161608, // Bladed Armor
      157447, // Critical Strikes
       22842, // Frenzied Regeneration
      210706, // Gore
        6795, // Growl
      137010, // Guardian Druid
      106734, // Guardian Overrides Passive
        5185, // Healing Touch
          99, // Incapacitating Roar
       48484, // Infected Wounds
      192081, // Ironfur
       86096, // Leather Specialization
      166142, // Mana Attunement
       33917, // Mangle
      192083, // Mark of Ursol
      155783, // Mastery: Nature's Guardian
      159195, // Mastery: Nature's Guardian
        6807, // Maul
       33873, // Nurturing Instinct
        5215, // Prowl
        2782, // Remove Corruption
      106839, // Skull Bash
      106898, // Stampeding Roar
      162700, // Stat Negation Aura - Agility Tank
       61336, // Survival Instincts
      213764, // Swipe
       16931, // Thick Hide
      106832, // Thrash
           0,
    },
    // Specialization abilities for Restoration
    {
       22812, // Barkskin
      145205, // Efflorescence
        5185, // Healing Touch
       16914, // Hurricane
       29166, // Innervate
      102342, // Ironbark
      108299, // Killer Instinct
       86093, // Leather Specialization
       33763, // Lifebloom
       48500, // Living Seed
       77495, // Mastery: Harmony
       88423, // Nature's Cure
      113043, // Omen of Clarity
        5215, // Prowl
        8936, // Regrowth
         774, // Rejuvenation
      137012, // Restoration Druid
      212040, // Revitalize
        5176, // Solar Wrath
      162701, // Stat Negation Aura - Intellect Healer
       93402, // Sunfire
       18562, // Swiftmend
         740, // Tranquility
      102793, // Ursol's Vortex
       48438, // Wild Growth
      145108, // Ysera's Gift
           0,
    },
  },
  // Specialization abilities for Demon Hunter
  {
    // Specialization abilities for Havoc
    {
      212612, // Havoc Demon Hunter
      178940, // Shattered Souls
           0,
    },
    // Specialization abilities for Vengeance
    {
      203720, // Demon Spikes
      203513, // Demonic Wards
      204021, // Fiery Brand
      178740, // Immolation Aura
      189926, // Increased Threat
      189110, // Infernal Strike
      187827, // Metamorphosis
      185244, // Pain
      207197, // Riposte
      204254, // Shattered Souls
      203782, // Shear
      203783, // Shear
      202138, // Sigil of Chains
      204596, // Sigil of Flame
      202137, // Sigil of Silence
      203798, // Soul Cleave
      204157, // Throw Glaive
      185245, // Torment
      212613, // Vengeance Demon Hunter
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

// Class mastery abilities, wow build 21134
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
#define RACE_ABILITY_SIZE (4)

#ifndef MAX_RACE
#define MAX_RACE (28)
#endif

// Racial abilities, wow build 21134
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
    {     0, },
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
      28730, // Arcane Torrent
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
#define GLYPH_ABILITIES_SIZE (8)

// Glyph spells for classes, wow build 21134
static unsigned __glyph_abilities_data[][3][GLYPH_ABILITIES_SIZE] = {
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
  },
  // Glyph spells for Warrior
  {
    {
           0,
    },
    // Minor Glyphs (8 spells)
    {
      115946, // Glyph of Burning Anger
      115943, // Glyph of Crow Feast
       58099, // Glyph of Gushing Wound
       58104, // Glyph of Mighty Victory
       58095, // Glyph of Mystic Shout
       68164, // Glyph of Thunder Strike
      123779, // Glyph of the Blazing Trail
      146974, // Glyph of the Weaponmaster
    },
    {
           0,
    },
  },
  // Glyph spells for Paladin
  {
    {
           0,
    },
    // Minor Glyphs (5 spells)
    {
      127755, // Glyph of Bladed Judgment
       57954, // Glyph of Fire From the Heavens
      146959, // Glyph of Pillar of Light
       57979, // Glyph of Winged Vengeance
       89401, // Glyph of the Luminous Charger
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Hunter
  {
    {
           0,
    },
    // Minor Glyphs (1 spells)
    {
       57870, // Glyph of Lesser Proportion
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Rogue
  {
    {
           0,
    },
    // Minor Glyphs (1 spells)
    {
       63268, // Glyph of Disguise
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Priest
  {
    {
           0,
    },
    // Minor Glyphs (8 spells)
    {
      145722, // Glyph of Angels
      147072, // Glyph of Inspired Hymns
       57986, // Glyph of Shackle Undead
      107906, // Glyph of Shadow
      126745, // Glyph of Shadowy Friends
      120581, // Glyph of the Heavens
      147776, // Glyph of the Sha
      126094, // Glyph of the Val'kyr
    },
    {
           0,
    },
  },
  // Glyph spells for Death Knight
  {
    {
           0,
    },
    // Minor Glyphs (4 spells)
    {
       58642, // Glyph of Foul Menagerie
       58680, // Glyph of Horn of Winter
       58640, // Glyph of the Geist
      146652, // Glyph of the Skeleton
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Shaman
  {
    {
           0,
    },
    // Minor Glyphs (4 spells)
    {
       63279, // Glyph of Deluge
      147784, // Glyph of Lingering Ancestors
      147783, // Glyph of Spirit Raptors
       58135, // Glyph of the Spectral Wolf
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Mage
  {
    {
           0,
    },
    // Minor Glyphs (3 spells)
    {
       56382, // Glyph of Crittermorph
      146662, // Glyph of Evaporation
      146976, // Glyph of the Unbound Elemental
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Warlock
  {
    {
           0,
    },
    // Minor Glyphs (3 spells)
    {
       63312, // Glyph of Crimson Banish
       56246, // Glyph of Felguard
       58094, // Glyph of Soulwell
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Monk
  {
    {
           0,
    },
    // Minor Glyphs (4 spells)
    {
      125931, // Glyph of Crackling Tiger Lightning
      125872, // Glyph of Fighting Pose
      125732, // Glyph of Honor
      125151, // Glyph of Rising Tiger Kick
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Druid
  {
    {
           0,
    },
    // Minor Glyphs (4 spells)
    {
      131113, // Glyph of the Cheetah
      114333, // Glyph of the Orca
      171803, // Glyph of the Sun
      107059, // Glyph of the Ursol Chameleon
           0,
    },
    {
           0,
    },
  },
};
#define SET_BONUS_DATA_SIZE (518)

// Set bonus data, wow build 21134
static item_set_bonus_t __set_bonus_data[SET_BONUS_DATA_SIZE] = {
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   2,   1,              {   0 },    0,   73, 165641, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   4,   1,              {   0 },    0,   73, 165670, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   2,   1,              {   0 },    2,   71, 165636, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   2,   1,              {   0 },    2,   72, 165639, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   4,   1,              {   0 },    2,   72, 165670, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Battlegear"                     ,        "pvp",      0,  1230,    0,   4,   1,              {   0 },    2,   71, 165670, { 127055, 127081, 127092, 127103, 127114,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   2,   2,              {   0 },    0,   66, 165905, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   4,   2,              {   0 },    0,   66, 171427, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   2,   2,              {   0 },    1,   65, 170860, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   4,   2,              {   0 },    1,   65, 180766, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   2,   2,              {   0 },    2,   70, 165886, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Vindication"                    ,        "pvp",      0,  1225,    0,   4,   2,              {   0 },    2,   70, 165895, { 127052, 127076, 127087, 127098, 127109,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   2,   3,              {   0 },    2,  255, 166005, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   2,   3,              {   0 },    2,  253, 166005, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   2,   3,              {   0 },    2,  254, 166005, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   4,   3,              {   0 },    2,  253, 171478, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   4,   3,              {   0 },    2,  254, 170884, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Pursuit"                        ,        "pvp",      0,  1222,    0,   4,   3,              {   0 },    2,  255, 166012, { 127050, 127073, 127084, 127095, 127106,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   2,   4,              {   0 },    2,  259, 165995, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   2,   4,              {   0 },    2,  261, 165995, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   2,   4,              {   0 },    2,  260, 165995, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   4,   4,              {   0 },    2,  259, 170883, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   4,   4,              {   0 },    2,  261, 170877, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Vestments"                      ,        "pvp",      0,  1227,    0,   4,   4,              {   0 },    2,  260, 182303, { 127053, 127078, 127089, 127100, 127111,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   2,   5,              {   0 },    1,  256, 171124, { 127065, 127077, 127088, 127099, 127110,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   2,   5,              {   0 },    1,  257, 171158, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   4,   5,              {   0 },    1,  256, 171141, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   4,   5,              {   0 },    1,  257, 171142, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   2,   5,              {   0 },    3,  258, 171146, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Investiture"                    ,        "pvp",      0,  1226,    0,   4,   5,              {   0 },    3,  258, 171151, { 127065, 127077, 127088, 127099, 127110,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   2,   6,              {   0 },    0,  250, 166020, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   4,   6,              {   0 },    0,  250, 171456, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   2,   6,              {   0 },    2,  252, 166020, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   2,   6,              {   0 },    2,  251, 166020, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   4,   6,              {   0 },    2,  251, 166056, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Desecration"                    ,        "pvp",      0,  1220,    0,   4,   6,              {   0 },    2,  252, 166061, { 127049, 127071, 127082, 127093, 127104,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   2,   7,              {   0 },    1,  264, 171381, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   4,   7,              {   0 },    1,  264, 166063, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   2,   7,              {   0 },    2,  263, 166103, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   4,   7,              {   0 },    2,  263, 171121, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   2,   7,              {   0 },    3,  262, 171109, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Wartide"                        ,        "pvp",      0,  1228,    0,   4,   7,              {   0 },    3,  262, 166096, { 127054, 127079, 127090, 127101, 127112,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   2,   8,              {   0 },    3,   62, 171349, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   2,   8,              {   0 },    3,   63, 165977, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   2,   8,              {   0 },    3,   64, 180721, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   4,   8,              {   0 },    3,   62, 171351, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   4,   8,              {   0 },    3,   63, 171169, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Regalia"                        ,        "pvp",      0,  1223,    0,   4,   8,              {   0 },    3,   64, 180741, { 127064, 127074, 127085, 127096, 127107,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   2,   9,              {   0 },    3,  265, 171377, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   2,   9,              {   0 },    3,  266, 171393, { 127066, 127080, 127091, 127102, 127113,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   2,   9,              {   0 },    3,  267, 171383, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   4,   9,              {   0 },    3,  265, 171379, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   4,   9,              {   0 },    3,  266, 165967, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Felshroud"                      ,        "pvp",      0,  1229,    0,   4,   9,              {   0 },    3,  267, 189209, { 127066, 127080, 127091, 127102, 127113,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   2,  10,              {   0 },    0,  268, 165691, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   4,  10,              {   0 },    0,  268, 171445, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   2,  10,              {   0 },    1,  270, 170799, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   4,  10,              {   0 },    1,  270, 165697, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   2,  10,              {   0 },    2,  269, 180743, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Ironskin"                       ,        "pvp",      0,  1224,    0,   4,  10,              {   0 },    2,  269, 181742, { 127051, 127075, 127086, 127097, 127108,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   2,  11,              {   0 },    0,  104, 171408, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   4,  11,              {   0 },    0,  104, 171489, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   2,  11,              {   0 },    1,  105, 170853, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   4,  11,              {   0 },    1,  105, 165707, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   2,  11,              {   0 },    2,  103, 170848, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   4,  11,              {   0 },    2,  103, 165698, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   2,  11,              {   0 },    3,  102, 165701, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Gladiator's Sanctuary"                      ,        "pvp",      0,  1221,    0,   4,  11,              {   0 },    3,  102, 180717, { 127063, 127072, 127083, 127094, 127105,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   1,              {   0 },    0,   73, 179142, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   1,              {   0 },    0,   73, 179143, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   1,              {   0 },    2,   71, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   1,              {   0 },    2,   72, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   1,              {   0 },    2,   71, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   1,              {   0 },    2,   72, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   2,              {   0 },    0,   66, 179142, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   2,              {   0 },    0,   66, 179143, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   2,              {   0 },    1,   65, 179145, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   2,              {   0 },    1,   65, 179146, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   2,              {   0 },    2,   70, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   2,              {   0 },    2,   70, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   3,              {   0 },    2,  255, 179130, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   3,              {   0 },    2,  253, 179130, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   3,              {   0 },    2,  254, 179130, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   3,              {   0 },    2,  255, 179131, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   3,              {   0 },    2,  253, 179131, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   3,              {   0 },    2,  254, 179131, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,   4,              {   0 },    2,  259, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,   4,              {   0 },    2,  261, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,   4,              {   0 },    2,  260, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,   4,              {   0 },    2,  259, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,   4,              {   0 },    2,  261, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,   4,              {   0 },    2,  260, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   5,              {   0 },    1,  257, 179110, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   5,              {   0 },    1,  256, 179110, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   5,              {   0 },    1,  257, 179111, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   5,              {   0 },    1,  256, 179111, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   5,              {   0 },    3,  258, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   5,              {   0 },    3,  258, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   6,              {   0 },    0,  250, 179142, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   6,              {   0 },    0,  250, 179143, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   6,              {   0 },    2,  251, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   2,   6,              {   0 },    2,  252, 179139, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   6,              {   0 },    2,  251, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Blacksteel Battleplate"                     ,  "tier17lfr",      1,  1245,   17,   4,   6,              {   0 },    2,  252, 179140, { 120396, 120395, 120394, 120393, 120392, 120391,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   7,              {   0 },    1,  264, 179137, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   7,              {   0 },    1,  264, 179138, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   7,              {   0 },    2,  263, 179130, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   7,              {   0 },    2,  263, 179131, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   2,   7,              {   0 },    3,  262, 179133, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ashlink Armor"                              ,  "tier17lfr",      1,  1246,   17,   4,   7,              {   0 },    3,  262, 179134, { 120390, 120389, 120388, 120387, 120386, 120385,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   8,              {   0 },    3,   62, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   8,              {   0 },    3,   63, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   8,              {   0 },    3,   64, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   8,              {   0 },    3,   62, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   8,              {   0 },    3,   63, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   8,              {   0 },    3,   64, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   9,              {   0 },    3,  265, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   9,              {   0 },    3,  266, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   2,   9,              {   0 },    3,  267, 179108, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   9,              {   0 },    3,  265, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   9,              {   0 },    3,  266, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Ebonflame Raiment"                          ,  "tier17lfr",      1,  1248,   17,   4,   9,              {   0 },    3,  267, 179107, { 120378, 120377, 120376, 120375, 120374, 120373,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  10,              {   0 },    0,  268, 179127, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  10,              {   0 },    0,  268, 179128, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  10,              {   0 },    1,  270, 179121, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  10,              {   0 },    1,  270, 179122, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  10,              {   0 },    2,  269, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  10,              {   0 },    2,  269, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  11,              {   0 },    0,  104, 179127, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  11,              {   0 },    0,  104, 179128, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  11,              {   0 },    1,  105, 179121, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  11,              {   0 },    1,  105, 179122, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  11,              {   0 },    2,  103, 179114, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  11,              {   0 },    2,  103, 179115, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   2,  11,              {   0 },    3,  102, 179117, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "Sootfur Garb"                               ,  "tier17lfr",      1,  1247,   17,   4,  11,              {   0 },    3,  102, 179118, { 120384, 120383, 120382, 120381, 120380, 120379,  0 } },
  { "The Twin Blades of Azzinoth"                ,    "glaives",      2,   699,    0,   2,  -1,              {   0 },   -1,   -1,  41434, {  32838,  32837,  0 } },
  { "The Twin Blades of Azzinoth"                ,    "glaives",      2,   699,    0,   2,  -1,              {   0 },   -1,   -1,  41433, {  32838,  32837,  0 } },
  { "Colossal Dragonplate Armor"                 ,     "tier13",      3,  1074,   13,   2,   1,              {  73 },    0,   -1, 105908, {  76988,  76989,  76990,  76991,  76992,  0 } },
  { "Colossal Dragonplate Armor"                 ,     "tier13",      3,  1074,   13,   4,   1,              {  73 },    0,   -1, 105911, {  76988,  76989,  76990,  76991,  76992,  0 } },
  { "Colossal Dragonplate Battlegear"            ,     "tier13",      3,  1073,   13,   2,   1,         {  71,  72 },    2,   -1, 105797, {  76983,  76984,  76985,  76986,  76987,  0 } },
  { "Colossal Dragonplate Battlegear"            ,     "tier13",      3,  1073,   13,   4,   1,         {  71,  72 },    2,   -1, 105907, {  76983,  76984,  76985,  76986,  76987,  0 } },
  { "Armor of Radiant Glory"                     ,     "tier13",      3,  1065,   13,   2,   2,              {  66 },    0,   -1, 105800, {  77003,  77004,  77005,  77006,  77007,  0 } },
  { "Armor of Radiant Glory"                     ,     "tier13",      3,  1065,   13,   4,   2,              {  66 },    0,   -1, 105744, {  77003,  77004,  77005,  77006,  77007,  0 } },
  { "Regalia of Radiant Glory"                   ,     "tier13",      3,  1063,   13,   2,   2,              {  65 },    1,   -1, 105743, {  76765,  76766,  76767,  76768,  76769,  0 } },
  { "Regalia of Radiant Glory"                   ,     "tier13",      3,  1063,   13,   4,   2,              {  65 },    1,   -1, 105798, {  76765,  76766,  76767,  76768,  76769,  0 } },
  { "Battleplate of Radiant Glory"               ,     "tier13",      3,  1064,   13,   2,   2,              {  70 },    2,   -1, 105765, {  76874,  76875,  76876,  76877,  76878,  0 } },
  { "Battleplate of Radiant Glory"               ,     "tier13",      3,  1064,   13,   4,   2,              {  70 },    2,   -1, 105820, {  76874,  76875,  76876,  76877,  76878,  0 } },
  { "Wyrmstalker Battlegear"                     ,     "tier13",      3,  1061,   13,   2,   3,    { 253, 254, 255 },    2,   -1, 105732, {  77028,  77029,  77030,  77031,  77032,  0 } },
  { "Wyrmstalker Battlegear"                     ,     "tier13",      3,  1061,   13,   4,   3,    { 253, 254, 255 },    2,   -1, 105921, {  77028,  77029,  77030,  77031,  77032,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Blackfang Battleweave"                      ,     "tier13",      3,  1068,   13,   2,   4,    { 259, 260, 261 },    2,   -1, 105849, {  77023,  77024,  77025,  77026,  77027,  0 } },
  { "Blackfang Battleweave"                      ,     "tier13",      3,  1068,   13,   4,   4,    { 259, 260, 261 },    2,   -1, 105865, {  77023,  77024,  77025,  77026,  77027,  0 } },
  { "Vestments of Dying Light"                   ,     "tier13",      3,  1066,   13,   2,   5,         { 256, 257 },    1,   -1, 105827, {  76357,  76358,  76359,  76360,  76361,  0 } },
  { "Vestments of Dying Light"                   ,     "tier13",      3,  1066,   13,   4,   5,         { 256, 257 },    1,   -1, 105832, {  76357,  76358,  76359,  76360,  76361,  0 } },
  { "Regalia of Dying Light"                     ,     "tier13",      3,  1067,   13,   2,   5,              { 258 },    3,   -1, 105843, {  76348,  76347,  76346,  76345,  76344,  0 } },
  { "Regalia of Dying Light"                     ,     "tier13",      3,  1067,   13,   4,   5,              { 258 },    3,   -1, 105844, {  76348,  76347,  76346,  76345,  76344,  0 } },
  { "Necrotic Boneplate Armor"                   ,     "tier13",      3,  1056,   13,   2,   6,              { 250 },    0,   -1, 105552, {  77008,  77009,  77010,  77011,  77012,  0 } },
  { "Necrotic Boneplate Armor"                   ,     "tier13",      3,  1056,   13,   4,   6,              { 250 },    0,   -1, 105587, {  77008,  77009,  77010,  77011,  77012,  0 } },
  { "Necrotic Boneplate Battlegear"              ,     "tier13",      3,  1057,   13,   2,   6,         { 251, 252 },    2,   -1, 105609, {  76974,  76975,  76976,  76977,  76978,  0 } },
  { "Necrotic Boneplate Battlegear"              ,     "tier13",      3,  1057,   13,   4,   6,         { 251, 252 },    2,   -1, 105646, {  76974,  76975,  76976,  76977,  76978,  0 } },
  { "Spiritwalker's Vestments"                   ,     "tier13",      3,  1069,   13,   2,   7,              { 264 },    1,   -1, 105764, {  76756,  76757,  76758,  76759,  76760,  0 } },
  { "Spiritwalker's Vestments"                   ,     "tier13",      3,  1069,   13,   4,   7,              { 264 },    1,   -1, 105876, {  76756,  76757,  76758,  76759,  76760,  0 } },
  { "Spiritwalker's Battlegear"                  ,     "tier13",      3,  1071,   13,   2,   7,              { 263 },    2,   -1, 105866, {  77040,  77041,  77042,  77043,  77044,  0 } },
  { "Spiritwalker's Battlegear"                  ,     "tier13",      3,  1071,   13,   4,   7,              { 263 },    2,   -1, 105872, {  77040,  77041,  77042,  77043,  77044,  0 } },
  { "Spiritwalker's Regalia"                     ,     "tier13",      3,  1070,   13,   2,   7,              { 262 },    3,   -1, 105780, {  77039,  77038,  77037,  77036,  77035,  0 } },
  { "Spiritwalker's Regalia"                     ,     "tier13",      3,  1070,   13,   4,   7,              { 262 },    3,   -1, 105816, {  77039,  77038,  77037,  77036,  77035,  0 } },
  { "Time Lord's Regalia"                        ,     "tier13",      3,  1062,   13,   2,   8,    {  62,  63,  64 },    3,   -1, 105788, {  76212,  76213,  76214,  76215,  76216,  0 } },
  { "Time Lord's Regalia"                        ,     "tier13",      3,  1062,   13,   4,   8,    {  62,  63,  64 },    3,   -1, 105790, {  76212,  76213,  76214,  76215,  76216,  0 } },
  { "Vestments of the Faceless Shroud"           ,     "tier13",      3,  1072,   13,   2,   9,    { 265, 266, 267 },    3,   -1, 105888, {  76343,  76342,  76341,  76340,  76339,  0 } },
  { "Vestments of the Faceless Shroud"           ,     "tier13",      3,  1072,   13,   4,   9,    { 265, 266, 267 },    3,   -1, 105787, {  76343,  76342,  76341,  76340,  76339,  0 } },
  { "Deep Earth Vestments"                       ,     "tier13",      3,  1060,   13,   2,  11,              { 105 },    1,   -1, 105715, {  76749,  76750,  76751,  76752,  76753,  0 } },
  { "Deep Earth Vestments"                       ,     "tier13",      3,  1060,   13,   4,  11,              { 105 },    1,   -1, 105770, {  76749,  76750,  76751,  76752,  76753,  0 } },
  { "Deep Earth Battlegarb"                      ,     "tier13",      3,  1058,   13,   2,  11,              { 103 },    2,   -1, 105725, {  77013,  77014,  77015,  77016,  77017,  0 } },
  { "Deep Earth Battlegarb"                      ,     "tier13",      3,  1058,   13,   4,  11,              { 103 },    2,   -1,  60142, {  77013,  77014,  77015,  77016,  77017,  0 } },
  { "Deep Earth Regalia"                         ,     "tier13",      3,  1059,   13,   2,  11,              { 102 },    3,   -1, 105722, {  77018,  77019,  77020,  77021,  77022,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Deep Earth Regalia"                         ,     "tier13",      3,  1059,   13,   4,  11,              { 102 },    3,   -1, 105717, {  77018,  77019,  77020,  77021,  77022,  0 } },
  { "Plate of Resounding Rings"                  ,     "tier14",      4,  1145,   14,   2,   1,              {  73 },    0,   -1, 123146, {  85324,  85325,  85326,  85327,  85328,  0 } },
  { "Plate of Resounding Rings"                  ,     "tier14",      4,  1145,   14,   4,   1,              {  73 },    0,   -1, 123147, {  85324,  85325,  85326,  85327,  85328,  0 } },
  { "Battleplate of Resounding Rings"            ,     "tier14",      4,  1144,   14,   2,   1,         {  71,  72 },    2,   -1, 123142, {  85329,  85330,  85331,  85332,  85333,  0 } },
  { "Battleplate of Resounding Rings"            ,     "tier14",      4,  1144,   14,   4,   1,         {  71,  72 },    2,   -1, 123144, {  85329,  85330,  85331,  85332,  85333,  0 } },
  { "White Tiger Plate"                          ,     "tier14",      4,  1136,   14,   2,   2,              {  66 },    0,   -1, 123104, {  85319,  85320,  85321,  85322,  85323,  0 } },
  { "White Tiger Plate"                          ,     "tier14",      4,  1136,   14,   4,   2,              {  66 },    0,   -1, 123107, {  85319,  85320,  85321,  85322,  85323,  0 } },
  { "White Tiger Vestments"                      ,     "tier14",      4,  1134,   14,   2,   2,              {  65 },    1,   -1, 123102, {  85344,  85345,  85346,  85347,  85348,  0 } },
  { "White Tiger Vestments"                      ,     "tier14",      4,  1134,   14,   4,   2,              {  65 },    1,   -1, 123103, {  85344,  85345,  85346,  85347,  85348,  0 } },
  { "White Tiger Battlegear"                     ,     "tier14",      4,  1135,   14,   2,   2,              {  70 },    2,   -1, 123108, {  85339,  85340,  85341,  85342,  85343,  0 } },
  { "White Tiger Battlegear"                     ,     "tier14",      4,  1135,   14,   4,   2,              {  70 },    2,   -1,  70762, {  85339,  85340,  85341,  85342,  85343,  0 } },
  { "Yaungol Slayer Battlegear"                  ,     "tier14",      4,  1129,   14,   2,   3,    { 253, 254, 255 },    2,   -1, 123090, {  85294,  85295,  85296,  85297,  85298,  0 } },
  { "Yaungol Slayer Battlegear"                  ,     "tier14",      4,  1129,   14,   4,   3,    { 253, 254, 255 },    2,   -1, 123091, {  85294,  85295,  85296,  85297,  85298,  0 } },
  { "Battlegear of the Thousandfold Blades"      ,     "tier14",      4,  1139,   14,   2,   4,    { 259, 260, 261 },    2,   -1, 123116, {  85299,  85300,  85301,  85302,  85303,  0 } },
  { "Battlegear of the Thousandfold Blades"      ,     "tier14",      4,  1139,   14,   4,   4,    { 259, 260, 261 },    2,   -1, 123122, {  85299,  85300,  85301,  85302,  85303,  0 } },
  { "Vestments of the Guardian Serpent"          ,     "tier14",      4,  1137,   14,   2,   5,         { 256, 257 },    1,   -1, 123111, {  85359,  85360,  85361,  85362,  85363,  0 } },
  { "Vestments of the Guardian Serpent"          ,     "tier14",      4,  1137,   14,   4,   5,         { 256, 257 },    1,   -1, 123113, {  85359,  85360,  85361,  85362,  85363,  0 } },
  { "Regalia of the Guardian Serpent"            ,     "tier14",      4,  1138,   14,   2,   5,              { 258 },    3,   -1, 123114, {  85368,  85367,  85366,  85365,  85364,  0 } },
  { "Regalia of the Guardian Serpent"            ,     "tier14",      4,  1138,   14,   4,   5,              { 258 },    3,   -1, 123115, {  85368,  85367,  85366,  85365,  85364,  0 } },
  { "Plate of the Lost Catacomb"                 ,     "tier14",      4,  1124,   14,   2,   6,              { 250 },    0,   -1, 123079, {  85314,  85315,  85316,  85317,  85318,  0 } },
  { "Plate of the Lost Catacomb"                 ,     "tier14",      4,  1124,   14,   4,   6,              { 250 },    0,   -1, 123080, {  85314,  85315,  85316,  85317,  85318,  0 } },
  { "Battlegear of the Lost Catacomb"            ,     "tier14",      4,  1123,   14,   2,   6,         { 251, 252 },    2,   -1, 123077, {  85334,  85335,  85336,  85337,  85338,  0 } },
  { "Battlegear of the Lost Catacomb"            ,     "tier14",      4,  1123,   14,   4,   6,         { 251, 252 },    2,   -1, 123078, {  85334,  85335,  85336,  85337,  85338,  0 } },
  { "Vestments of the Firebird"                  ,     "tier14",      4,  1140,   14,   2,   7,              { 264 },    1,   -1, 123134, {  85349,  85350,  85351,  85352,  85353,  0 } },
  { "Vestments of the Firebird"                  ,     "tier14",      4,  1140,   14,   4,   7,              { 264 },    1,   -1, 123135, {  85349,  85350,  85351,  85352,  85353,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Battlegear of the Firebird"                 ,     "tier14",      4,  1142,   14,   2,   7,              { 263 },    2,   -1, 123132, {  85284,  85285,  85286,  85287,  85288,  0 } },
  { "Battlegear of the Firebird"                 ,     "tier14",      4,  1142,   14,   4,   7,              { 263 },    2,   -1, 123133, {  85284,  85285,  85286,  85287,  85288,  0 } },
  { "Regalia of the Firebird"                    ,     "tier14",      4,  1141,   14,   2,   7,              { 262 },    3,   -1, 123123, {  85293,  85292,  85291,  85290,  85289,  0 } },
  { "Regalia of the Firebird"                    ,     "tier14",      4,  1141,   14,   4,   7,              { 262 },    3,   -1, 123124, {  85293,  85292,  85291,  85290,  85289,  0 } },
  { "Regalia of the Burning Scroll"              ,     "tier14",      4,  1130,   14,   2,   8,    {  62,  63,  64 },    3,   -1, 123097, {  85374,  85375,  85376,  85377,  85378,  0 } },
  { "Regalia of the Burning Scroll"              ,     "tier14",      4,  1130,   14,   4,   8,    {  62,  63,  64 },    3,   -1, 123101, {  85374,  85375,  85376,  85377,  85378,  0 } },
  { "Sha-Skin Regalia"                           ,     "tier14",      4,  1143,   14,   2,   9,    { 265, 266, 267 },    3,   -1, 123136, {  85373,  85372,  85371,  85370,  85369,  0 } },
  { "Sha-Skin Regalia"                           ,     "tier14",      4,  1143,   14,   4,   9,    { 265, 266, 267 },    3,   -1, 123141, {  85373,  85372,  85371,  85370,  85369,  0 } },
  { "Armor of the Red Crane"                     ,     "tier14",      4,  1133,   14,   2,  10,              { 268 },    0,   -1, 123157, {  85384,  85385,  85386,  85387,  85388,  0 } },
  { "Armor of the Red Crane"                     ,     "tier14",      4,  1133,   14,   4,  10,              { 268 },    0,   -1, 123159, {  85384,  85385,  85386,  85387,  85388,  0 } },
  { "Vestments of the Red Crane"                 ,     "tier14",      4,  1131,   14,   2,  10,              { 270 },    1,   -1, 123152, {  85393,  85392,  85391,  85390,  85389,  0 } },
  { "Vestments of the Red Crane"                 ,     "tier14",      4,  1131,   14,   4,  10,              { 270 },    1,   -1, 123153, {  85393,  85392,  85391,  85390,  85389,  0 } },
  { "Battlegear of the Red Crane"                ,     "tier14",      4,  1132,   14,   2,  10,              { 269 },    2,   -1, 123149, {  85398,  85397,  85396,  85395,  85394,  0 } },
  { "Battlegear of the Red Crane"                ,     "tier14",      4,  1132,   14,   4,  10,              { 269 },    2,   -1, 123150, {  85398,  85397,  85396,  85395,  85394,  0 } },
  { "Armor of the Eternal Blossom"               ,     "tier14",      4,  1128,   14,   2,  11,              { 104 },    0,   -1, 144879, {  85383,  85382,  85381,  85380,  85379,  0 } },
  { "Armor of the Eternal Blossom"               ,     "tier14",      4,  1128,   14,   4,  11,              { 104 },    0,   -1, 123087, {  85383,  85382,  85381,  85380,  85379,  0 } },
  { "Vestments of the Eternal Blossom"           ,     "tier14",      4,  1125,   14,   2,  11,              { 105 },    1,   -1, 123088, {  85354,  85355,  85356,  85357,  85358,  0 } },
  { "Vestments of the Eternal Blossom"           ,     "tier14",      4,  1125,   14,   4,  11,              { 105 },    1,   -1, 123089, {  85354,  85355,  85356,  85357,  85358,  0 } },
  { "Battlegear of the Eternal Blossom"          ,     "tier14",      4,  1127,   14,   2,  11,              { 103 },    2,   -1, 123084, {  85309,  85310,  85311,  85312,  85313,  0 } },
  { "Battlegear of the Eternal Blossom"          ,     "tier14",      4,  1127,   14,   4,  11,              { 103 },    2,   -1, 123085, {  85309,  85310,  85311,  85312,  85313,  0 } },
  { "Regalia of the Eternal Blossom"             ,     "tier14",      4,  1126,   14,   2,  11,              { 102 },    3,   -1, 123082, {  85304,  85305,  85306,  85307,  85308,  0 } },
  { "Regalia of the Eternal Blossom"             ,     "tier14",      4,  1126,   14,   4,  11,              { 102 },    3,   -1, 123083, {  85304,  85305,  85306,  85307,  85308,  0 } },
  { "Plate of the Last Mogu"                     ,     "tier15",      5,  1173,   15,   2,   1,              {  73 },    0,   -1, 138280, {  95335,  95336,  95337,  95338,  95339,  0 } },
  { "Plate of the Last Mogu"                     ,     "tier15",      5,  1173,   15,   4,   1,              {  73 },    0,   -1, 138281, {  95335,  95336,  95337,  95338,  95339,  0 } },
  { "Battleplate of the Last Mogu"               ,     "tier15",      5,  1172,   15,   2,   1,         {  71,  72 },    2,   -1, 138120, {  95330,  95331,  95332,  95333,  95334,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Battleplate of the Last Mogu"               ,     "tier15",      5,  1172,   15,   4,   1,         {  71,  72 },    2,   -1, 138126, {  95330,  95331,  95332,  95333,  95334,  0 } },
  { "Plate of the Lightning Emperor"             ,     "tier15",      5,  1164,   15,   2,   2,              {  66 },    0,   -1, 138238, {  95290,  95291,  95292,  95293,  95294,  0 } },
  { "Plate of the Lightning Emperor"             ,     "tier15",      5,  1164,   15,   4,   2,              {  66 },    0,   -1, 138244, {  95290,  95291,  95292,  95293,  95294,  0 } },
  { "Vestments of the Lightning Emperor"         ,     "tier15",      5,  1163,   15,   2,   2,              {  65 },    1,   -1, 138291, {  95285,  95286,  95287,  95288,  95289,  0 } },
  { "Vestments of the Lightning Emperor"         ,     "tier15",      5,  1163,   15,   4,   2,              {  65 },    1,   -1, 138292, {  95285,  95286,  95287,  95288,  95289,  0 } },
  { "Battlegear of the Lightning Emperor"        ,     "tier15",      5,  1162,   15,   2,   2,              {  70 },    2,   -1, 138159, {  95280,  95281,  95282,  95283,  95284,  0 } },
  { "Battlegear of the Lightning Emperor"        ,     "tier15",      5,  1162,   15,   4,   2,              {  70 },    2,   -1, 138164, {  95280,  95281,  95282,  95283,  95284,  0 } },
  { "Battlegear of the Saurok Stalker"           ,     "tier15",      5,  1157,   15,   2,   3,    { 253, 254, 255 },    2,   -1, 138365, {  95255,  95256,  95257,  95258,  95259,  0 } },
  { "Battlegear of the Saurok Stalker"           ,     "tier15",      5,  1157,   15,   4,   3,    { 253, 254, 255 },    2,   -1, 138367, {  95255,  95256,  95257,  95258,  95259,  0 } },
  { "Nine-Tail Battlegear"                       ,     "tier15",      5,  1167,   15,   2,   4,    { 259, 260, 261 },    2,   -1, 138148, {  95305,  95306,  95307,  95308,  95309,  0 } },
  { "Nine-Tail Battlegear"                       ,     "tier15",      5,  1167,   15,   4,   4,    { 259, 260, 261 },    2,   -1, 138150, {  95305,  95306,  95307,  95308,  95309,  0 } },
  { "Vestments of the Exorcist"                  ,     "tier15",      5,  1165,   15,   2,   5,         { 256, 257 },    1,   -1, 138293, {  95295,  95296,  95297,  95298,  95299,  0 } },
  { "Vestments of the Exorcist"                  ,     "tier15",      5,  1165,   15,   4,   5,         { 256, 257 },    1,   -1, 138301, {  95295,  95296,  95297,  95298,  95299,  0 } },
  { "Regalia of the Exorcist"                    ,     "tier15",      5,  1166,   15,   2,   5,              { 258 },    3,   -1, 138156, {  95300,  95301,  95302,  95303,  95304,  0 } },
  { "Regalia of the Exorcist"                    ,     "tier15",      5,  1166,   15,   4,   5,              { 258 },    3,   -1, 138158, {  95300,  95301,  95302,  95303,  95304,  0 } },
  { "Plate of the All-Consuming Maw"             ,     "tier15",      5,  1151,   15,   2,   6,              { 250 },    0,   -1, 138195, {  95230,  95231,  95232,  95233,  95234,  0 } },
  { "Plate of the All-Consuming Maw"             ,     "tier15",      5,  1151,   15,   4,   6,              { 250 },    0,   -1, 138197, {  95230,  95231,  95232,  95233,  95234,  0 } },
  { "Battleplate of the All-Consuming Maw"       ,     "tier15",      5,  1152,   15,   2,   6,         { 251, 252 },    2,   -1, 138343, {  95225,  95226,  95227,  95228,  95229,  0 } },
  { "Battleplate of the All-Consuming Maw"       ,     "tier15",      5,  1152,   15,   4,   6,         { 251, 252 },    2,   -1, 138347, {  95225,  95226,  95227,  95228,  95229,  0 } },
  { "Vestments of the Witch Doctor"              ,     "tier15",      5,  1168,   15,   2,   7,              { 264 },    1,   -1, 138303, {  95310,  95311,  95312,  95313,  95314,  0 } },
  { "Vestments of the Witch Doctor"              ,     "tier15",      5,  1168,   15,   4,   7,              { 264 },    1,   -1, 138305, {  95310,  95311,  95312,  95313,  95314,  0 } },
  { "Battlegear of the Witch Doctor"             ,     "tier15",      5,  1169,   15,   2,   7,              { 263 },    2,   -1, 138136, {  95315,  95316,  95317,  95318,  95319,  0 } },
  { "Battlegear of the Witch Doctor"             ,     "tier15",      5,  1169,   15,   4,   7,              { 263 },    2,   -1, 138141, {  95315,  95316,  95317,  95318,  95319,  0 } },
  { "Regalia of the Witch Doctor"                ,     "tier15",      5,  1170,   15,   2,   7,              { 262 },    3,   -1, 138145, {  95320,  95321,  95322,  95323,  95324,  0 } },
  { "Regalia of the Witch Doctor"                ,     "tier15",      5,  1170,   15,   4,   7,              { 262 },    3,   -1, 138144, {  95320,  95321,  95322,  95323,  95324,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Regalia of the Chromatic Hydra"             ,     "tier15",      5,  1158,   15,   2,   8,    {  62,  63,  64 },    3,   -1, 138316, {  95260,  95261,  95262,  95263,  95264,  0 } },
  { "Regalia of the Chromatic Hydra"             ,     "tier15",      5,  1158,   15,   4,   8,    {  62,  63,  64 },    3,   -1, 138376, {  95260,  95261,  95262,  95263,  95264,  0 } },
  { "Regalia of the Thousandfold Hells"          ,     "tier15",      5,  1171,   15,   2,   9,    { 265, 266, 267 },    3,   -1, 138129, {  95325,  95326,  95327,  95328,  95329,  0 } },
  { "Regalia of the Thousandfold Hells"          ,     "tier15",      5,  1171,   15,   4,   9,    { 265, 266, 267 },    3,   -1, 138134, {  95325,  95326,  95327,  95328,  95329,  0 } },
  { "Fire-Charm Armor"                           ,     "tier15",      5,  1161,   15,   2,  10,              { 268 },    0,   -1, 138231, {  95275,  95276,  95277,  95278,  95279,  0 } },
  { "Fire-Charm Armor"                           ,     "tier15",      5,  1161,   15,   4,  10,              { 268 },    0,   -1, 138236, {  95275,  95276,  95277,  95278,  95279,  0 } },
  { "Fire-Charm Vestments"                       ,     "tier15",      5,  1160,   15,   2,  10,              { 270 },    1,   -1, 138290, {  95270,  95271,  95272,  95273,  95274,  0 } },
  { "Fire-Charm Vestments"                       ,     "tier15",      5,  1160,   15,   4,  10,              { 270 },    1,   -1, 138289, {  95270,  95271,  95272,  95273,  95274,  0 } },
  { "Fire-Charm Battlegear"                      ,     "tier15",      5,  1159,   15,   2,  10,              { 269 },    2,   -1, 138177, {  95265,  95266,  95267,  95268,  95269,  0 } },
  { "Fire-Charm Battlegear"                      ,     "tier15",      5,  1159,   15,   4,  10,              { 269 },    2,   -1, 138315, {  95265,  95266,  95267,  95268,  95269,  0 } },
  { "Armor of the Haunted Forest"                ,     "tier15",      5,  1156,   15,   2,  11,              { 104 },    0,   -1, 138216, {  95250,  95251,  95252,  95253,  95254,  0 } },
  { "Armor of the Haunted Forest"                ,     "tier15",      5,  1156,   15,   4,  11,              { 104 },    0,   -1, 138222, {  95250,  95251,  95252,  95253,  95254,  0 } },
  { "Vestments of the Haunted Forest"            ,     "tier15",      5,  1154,   15,   2,  11,              { 105 },    1,   -1, 138284, {  95240,  95241,  95242,  95243,  95244,  0 } },
  { "Vestments of the Haunted Forest"            ,     "tier15",      5,  1154,   15,   4,  11,              { 105 },    1,   -1, 138286, {  95240,  95241,  95242,  95243,  95244,  0 } },
  { "Battlegear of the Haunted Forest"           ,     "tier15",      5,  1153,   15,   2,  11,              { 103 },    2,   -1, 138352, {  95235,  95236,  95237,  95238,  95239,  0 } },
  { "Battlegear of the Haunted Forest"           ,     "tier15",      5,  1153,   15,   4,  11,              { 103 },    2,   -1, 138357, {  95235,  95236,  95237,  95238,  95239,  0 } },
  { "Regalia of the Haunted Forest"              ,     "tier15",      5,  1155,   15,   2,  11,              { 102 },    3,   -1, 138348, {  95245,  95246,  95247,  95248,  95249,  0 } },
  { "Regalia of the Haunted Forest"              ,     "tier15",      5,  1155,   15,   4,  11,              { 102 },    3,   -1, 138350, {  95245,  95246,  95247,  95248,  95249,  0 } },
  { "Plate of the Prehistoric Marauder"          ,     "tier16",      6,  1179,   16,   2,   1,              {  73 },    0,   -1, 144503, {  99195,  99196,  99201,  99202,  99203,  0 } },
  { "Plate of the Prehistoric Marauder"          ,     "tier16",      6,  1179,   16,   4,   1,              {  73 },    0,   -1, 144502, {  99195,  99196,  99201,  99202,  99203,  0 } },
  { "Battleplate of the Prehistoric Marauder"    ,     "tier16",      6,  1180,   16,   2,   1,         {  71,  72 },    2,   -1,  60174, {  99197,  99198,  99199,  99200,  99206,  0 } },
  { "Battleplate of the Prehistoric Marauder"    ,     "tier16",      6,  1180,   16,   4,   1,         {  71,  72 },    2,   -1, 144441, {  99197,  99198,  99199,  99200,  99206,  0 } },
  { "Plate of Winged Triumph"                    ,     "tier16",      6,  1188,   16,   2,   2,              {  66 },    0,   -1, 144580, {  99126,  99127,  99128,  99129,  99130,  0 } },
  { "Plate of Winged Triumph"                    ,     "tier16",      6,  1188,   16,   4,   2,              {  66 },    0,   -1, 144566, {  99126,  99127,  99128,  99129,  99130,  0 } },
  { "Vestments of Winged Triumph"                ,     "tier16",      6,  1189,   16,   2,   2,              {  65 },    1,   -1, 144625, {  99133,  99124,  99125,  99134,  99135,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Vestments of Winged Triumph"                ,     "tier16",      6,  1189,   16,   4,   2,              {  65 },    1,   -1, 144613, {  99133,  99124,  99125,  99134,  99135,  0 } },
  { "Battlegear of Winged Triumph"               ,     "tier16",      6,  1190,   16,   2,   2,              {  70 },    2,   -1, 144586, {  99132,  99136,  99137,  99138,  99139,  0 } },
  { "Battlegear of Winged Triumph"               ,     "tier16",      6,  1190,   16,   4,   2,              {  70 },    2,   -1, 144593, {  99132,  99136,  99137,  99138,  99139,  0 } },
  { "Battlegear of the Unblinking Vigil"         ,     "tier16",      6,  1195,   16,   2,   3,    { 253, 254, 255 },    2,   -1, 144637, {  99157,  99158,  99159,  99167,  99168,  0 } },
  { "Battlegear of the Unblinking Vigil"         ,     "tier16",      6,  1195,   16,   4,   3,    { 253, 254, 255 },    2,   -1, 144641, {  99157,  99158,  99159,  99167,  99168,  0 } },
  { "Barbed Assassin Battlegear"                 ,     "tier16",      6,  1185,   16,   2,   4,    { 259, 260, 261 },    2,   -1, 145185, {  99113,  99112,  99114,  99115,  99116,  0 } },
  { "Barbed Assassin Battlegear"                 ,     "tier16",      6,  1185,   16,   4,   4,    { 259, 260, 261 },    2,   -1, 145210, {  99113,  99112,  99114,  99115,  99116,  0 } },
  { "Vestments of Ternion Glory"                 ,     "tier16",      6,  1187,   16,   2,   5,         { 256, 257 },    1,   -1, 145306, {  99117,  99118,  99119,  99120,  99131,  0 } },
  { "Vestments of Ternion Glory"                 ,     "tier16",      6,  1187,   16,   4,   5,         { 256, 257 },    1,   -1, 145334, {  99117,  99118,  99119,  99120,  99131,  0 } },
  { "Regalia of Ternion Glory"                   ,     "tier16",      6,  1186,   16,   2,   5,              { 258 },    3,   -1, 145174, {  99123,  99110,  99111,  99121,  99122,  0 } },
  { "Regalia of Ternion Glory"                   ,     "tier16",      6,  1186,   16,   4,   5,              { 258 },    3,   -1, 145179, {  99123,  99110,  99111,  99121,  99122,  0 } },
  { "Plate of Cyclopean Dread"                   ,     "tier16",      6,  1201,   16,   2,   6,              { 250 },    0,   -1, 144934, {  99179,  99188,  99189,  99190,  99191,  0 } },
  { "Plate of Cyclopean Dread"                   ,     "tier16",      6,  1201,   16,   4,   6,              { 250 },    0,   -1, 144950, {  99179,  99188,  99189,  99190,  99191,  0 } },
  { "Battleplate of Cyclopean Dread"             ,     "tier16",      6,  1200,   16,   2,   6,         { 251, 252 },    2,   -1, 144899, {  99186,  99187,  99192,  99193,  99194,  0 } },
  { "Battleplate of Cyclopean Dread"             ,     "tier16",      6,  1200,   16,   4,   6,         { 251, 252 },    2,   -1, 144907, {  99186,  99187,  99192,  99193,  99194,  0 } },
  { "Celestial Harmony Vestment"                 ,     "tier16",      6,  1184,   16,   2,   7,              { 264 },    1,   -1, 145378, {  99099,  99100,  99107,  99108,  99109,  0 } },
  { "Celestial Harmony Vestment"                 ,     "tier16",      6,  1184,   16,   4,   7,              { 264 },    1,   -1, 145380, {  99099,  99100,  99107,  99108,  99109,  0 } },
  { "Celestial Harmony Battlegear"               ,     "tier16",      6,  1183,   16,   2,   7,              { 263 },    2,   -1, 144962, {  99101,  99102,  99103,  99104,  99105,  0 } },
  { "Celestial Harmony Battlegear"               ,     "tier16",      6,  1183,   16,   4,   7,              { 263 },    2,   -1, 144966, {  99101,  99102,  99103,  99104,  99105,  0 } },
  { "Celestial Harmony Regalia"                  ,     "tier16",      6,  1182,   16,   2,   7,              { 262 },    3,   -1, 144998, {  99092,  99093,  99094,  99095,  99106,  0 } },
  { "Celestial Harmony Regalia"                  ,     "tier16",      6,  1182,   16,   4,   7,              { 262 },    3,   -1, 145003, {  99092,  99093,  99094,  99095,  99106,  0 } },
  { "Chronomancer Regalia"                       ,     "tier16",      6,  1194,   16,   2,   8,    {  62,  63,  64 },    3,   -1, 145251, {  99152,  99153,  99160,  99161,  99162,  0 } },
  { "Chronomancer Regalia"                       ,     "tier16",      6,  1194,   16,   4,   8,    {  62,  63,  64 },    3,   -1, 145257, {  99152,  99153,  99160,  99161,  99162,  0 } },
  { "Regalia of the Horned Nightmare"            ,     "tier16",      6,  1181,   16,   2,   9,    { 265, 266, 267 },    3,   -1, 145072, {  99096,  99097,  99098,  99204,  99205,  0 } },
  { "Regalia of the Horned Nightmare"            ,     "tier16",      6,  1181,   16,   4,   9,    { 265, 266, 267 },    3,   -1, 145091, {  99096,  99097,  99098,  99204,  99205,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Armor of Seven Sacred Seals"                ,     "tier16",      6,  1191,   16,   2,  10,              { 268 },    0,   -1, 145049, {  99142,  99143,  99144,  99140,  99141,  0 } },
  { "Armor of Seven Sacred Seals"                ,     "tier16",      6,  1191,   16,   4,  10,              { 268 },    0,   -1, 145055, {  99142,  99143,  99144,  99140,  99141,  0 } },
  { "Vestments of Seven Sacred Seals"            ,     "tier16",      6,  1192,   16,   2,  10,              { 270 },    1,   -1, 145439, {  99151,  99147,  99148,  99149,  99150,  0 } },
  { "Vestments of Seven Sacred Seals"            ,     "tier16",      6,  1192,   16,   4,  10,              { 270 },    1,   -1, 145449, {  99151,  99147,  99148,  99149,  99150,  0 } },
  { "Battlegear of Seven Sacred Seals"           ,     "tier16",      6,  1193,   16,   2,  10,              { 269 },    2,   -1, 145004, {  99154,  99155,  99156,  99145,  99146,  0 } },
  { "Battlegear of Seven Sacred Seals"           ,     "tier16",      6,  1193,   16,   4,  10,              { 269 },    2,   -1, 145022, {  99154,  99155,  99156,  99145,  99146,  0 } },
  { "Armor of the Shattered Vale"                ,     "tier16",      6,  1196,   16,   2,  11,              { 104 },    0,   -1, 144879, {  99163,  99164,  99165,  99166,  99170,  0 } },
  { "Armor of the Shattered Vale"                ,     "tier16",      6,  1196,   16,   4,  11,              { 104 },    0,   -1, 144887, {  99163,  99164,  99165,  99166,  99170,  0 } },
  { "Vestments of the Shattered Vale"            ,     "tier16",      6,  1198,   16,   2,  11,              { 105 },    1,   -1, 144869, {  99171,  99172,  99173,  99178,  99185,  0 } },
  { "Vestments of the Shattered Vale"            ,     "tier16",      6,  1198,   16,   4,  11,              { 105 },    1,   -1, 144875, {  99171,  99172,  99173,  99178,  99185,  0 } },
  { "Battlegear of the Shattered Vale"           ,     "tier16",      6,  1199,   16,   2,  11,              { 103 },    2,   -1, 144864, {  99180,  99181,  99182,  99183,  99184,  0 } },
  { "Battlegear of the Shattered Vale"           ,     "tier16",      6,  1199,   16,   4,  11,              { 103 },    2,   -1, 144841, {  99180,  99181,  99182,  99183,  99184,  0 } },
  { "Regalia of the Shattered Vale"              ,     "tier16",      6,  1197,   16,   2,  11,              { 102 },    3,   -1, 144767, {  99169,  99174,  99175,  99176,  99177,  0 } },
  { "Regalia of the Shattered Vale"              ,     "tier16",      6,  1197,   16,   4,  11,              { 102 },    3,   -1, 144756, {  99169,  99174,  99175,  99176,  99177,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",      7,  1242,   17,   2,   1,              {   0 },    0,   73, 165338, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",      7,  1242,   17,   4,   1,              {   0 },    0,   73, 165351, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",      7,  1242,   17,   2,   1,              {   0 },    2,   71, 165345, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",      7,  1242,   17,   2,   1,              {   0 },    2,   72, 165337, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",      7,  1242,   17,   4,   1,              {   0 },    2,   71, 165336, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Blackhand's Battlegear"                     ,     "tier17",      7,  1242,   17,   4,   1,              {   0 },    2,   72, 165349, { 115580, 115581, 115582, 115583, 115584,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",      7,  1238,   17,   2,   2,              {   0 },    0,   66, 165446, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",      7,  1238,   17,   4,   2,              {   0 },    0,   66, 167740, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",      7,  1238,   17,   2,   2,              {   0 },    1,   65, 165438, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",      7,  1238,   17,   4,   2,              {   0 },    1,   65, 167697, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Battlegear of Guiding Light"                ,     "tier17",      7,  1238,   17,   2,   2,              {   0 },    2,   70, 165440, { 115565, 115566, 115567, 115568, 115569,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Battlegear of Guiding Light"                ,     "tier17",      7,  1238,   17,   4,   2,              {   0 },    2,   70, 165439, { 115565, 115566, 115567, 115568, 115569,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",      7,  1236,   17,   2,   3,              {   0 },    2,  253, 165517, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",      7,  1236,   17,   2,   3,              {   0 },    2,  254, 165519, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",      7,  1236,   17,   2,   3,              {   0 },    2,  255, 165544, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",      7,  1236,   17,   4,   3,              {   0 },    2,  253, 165518, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",      7,  1236,   17,   4,   3,              {   0 },    2,  254, 165525, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Rylakstalker's Battlegear"                  ,     "tier17",      7,  1236,   17,   4,   3,              {   0 },    2,  255, 165545, { 115545, 115546, 115547, 115548, 115549,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",      7,  1240,   17,   2,   4,              {   0 },    2,  259, 165516, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",      7,  1240,   17,   2,   4,              {   0 },    2,  260, 165480, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",      7,  1240,   17,   2,   4,              {   0 },    2,  261, 165482, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",      7,  1240,   17,   4,   4,              {   0 },    2,  259, 166886, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",      7,  1240,   17,   4,   4,              {   0 },    2,  260, 165478, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Poisoner's Battlegear"                      ,     "tier17",      7,  1240,   17,   4,   4,              {   0 },    2,  261, 165514, { 115570, 115571, 115572, 115573, 115574,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",      7,  1239,   17,   2,   5,              {   0 },    1,  256, 165614, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",      7,  1239,   17,   2,   5,              {   0 },    1,  257, 165621, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",      7,  1239,   17,   4,   5,              {   0 },    1,  256, 167694, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",      7,  1239,   17,   4,   5,              {   0 },    1,  257, 167684, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",      7,  1239,   17,   2,   5,              {   0 },    3,  258, 165628, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Soul Priest's Raiment"                      ,     "tier17",      7,  1239,   17,   4,   5,              {   0 },    3,  258, 165629, { 115560, 115561, 115562, 115563, 115564,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",      7,  1234,   17,   2,   6,              {   0 },    0,  250, 167192, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",      7,  1234,   17,   4,   6,              {   0 },    0,  250, 165571, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",      7,  1234,   17,   2,   6,              {   0 },    2,  251, 165547, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",      7,  1234,   17,   2,   6,              {   0 },    2,  252, 165575, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",      7,  1234,   17,   4,   6,              {   0 },    2,  251, 165568, { 115535, 115536, 115537, 115538, 115539,  0 } },
  { "Ogreskull Boneplate Battlegear"             ,     "tier17",      7,  1234,   17,   4,   6,              {   0 },    2,  252, 165574, { 115535, 115536, 115537, 115538, 115539,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Windspeaker's Regalia"                      ,     "tier17",      7,  1241,   17,   2,   7,              {   0 },    1,  264, 165576, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",      7,  1241,   17,   4,   7,              {   0 },    1,  264, 167702, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",      7,  1241,   17,   2,   7,              {   0 },    2,  263, 165605, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",      7,  1241,   17,   4,   7,              {   0 },    2,  263, 165610, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",      7,  1241,   17,   2,   7,              {   0 },    3,  262, 165577, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Windspeaker's Regalia"                      ,     "tier17",      7,  1241,   17,   4,   7,              {   0 },    3,  262, 165580, { 115575, 115576, 115577, 115578, 115579,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",      7,  1235,   17,   2,   8,              {   0 },    3,   62, 165475, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",      7,  1235,   17,   2,   8,              {   0 },    3,   63, 165458, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",      7,  1235,   17,   2,   8,              {   0 },    3,   64, 165469, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",      7,  1235,   17,   4,   8,              {   0 },    3,   62, 165476, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",      7,  1235,   17,   4,   8,              {   0 },    3,   63, 165459, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Arcanoshatter Regalia"                      ,     "tier17",      7,  1235,   17,   4,   8,              {   0 },    3,   64, 165470, { 115550, 115551, 115552, 115553, 115554,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",      7,  1243,   17,   2,   9,              {   0 },    3,  265, 165448, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",      7,  1243,   17,   2,   9,              {   0 },    3,  266, 165450, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",      7,  1243,   17,   2,   9,              {   0 },    3,  267, 165455, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",      7,  1243,   17,   4,   9,              {   0 },    3,  265, 165449, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",      7,  1243,   17,   4,   9,              {   0 },    3,  266, 165451, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Shadow Council's Garb"                      ,     "tier17",      7,  1243,   17,   4,   9,              {   0 },    3,  267, 165452, { 115585, 115586, 115587, 115588, 115589,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",      7,  1237,   17,   2,  10,              {   0 },    0,  268, 165353, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",      7,  1237,   17,   4,  10,              {   0 },    0,  268, 165352, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",      7,  1237,   17,   2,  10,              {   0 },    1,  270, 165404, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",      7,  1237,   17,   4,  10,              {   0 },    1,  270, 167718, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",      7,  1237,   17,   2,  10,              {   0 },    2,  269, 165403, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Battlegear of the Somber Gaze"              ,     "tier17",      7,  1237,   17,   4,  10,              {   0 },    2,  269, 165402, { 115555, 115556, 115557, 115558, 115559,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",      7,  1233,   17,   2,  11,              {   0 },    0,  104, 165410, { 115540, 115541, 115542, 115543, 115544,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Living Wood Battlegear"                     ,     "tier17",      7,  1233,   17,   4,  11,              {   0 },    0,  104, 165423, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",      7,  1233,   17,   2,  11,              {   0 },    1,  105, 165437, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",      7,  1233,   17,   4,  11,              {   0 },    1,  105, 167714, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",      7,  1233,   17,   2,  11,              {   0 },    2,  103, 165431, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",      7,  1233,   17,   4,  11,              {   0 },    2,  103, 165432, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",      7,  1233,   17,   2,  11,              {   0 },    3,  102, 165433, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Living Wood Battlegear"                     ,     "tier17",      7,  1233,   17,   4,  11,              {   0 },    3,  102, 166677, { 115540, 115541, 115542, 115543, 115544,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",      8,  1258,   18,   2,   1,              {   0 },    0,   73, 185796, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",      8,  1258,   18,   4,   1,              {   0 },    0,   73, 185797, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",      8,  1258,   18,   2,   1,              {   0 },    2,   71, 185800, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",      8,  1258,   18,   2,   1,              {   0 },    2,   72, 185798, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",      8,  1258,   18,   4,   1,              {   0 },    2,   71, 185804, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Battlegear of Iron Wrath"                   ,     "tier18",      8,  1258,   18,   4,   1,              {   0 },    2,   72, 185799, { 124319, 124329, 124334, 124340, 124346,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",      8,  1254,   18,   2,   2,              {   0 },    0,   66, 185675, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",      8,  1254,   18,   4,   2,              {   0 },    0,   66, 185677, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",      8,  1254,   18,   2,   2,              {   0 },    1,   65, 185545, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",      8,  1254,   18,   4,   2,              {   0 },    1,   65, 185571, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",      8,  1254,   18,   2,   2,              {   0 },    2,   70, 185581, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Watch of the Ceaseless Vigil"               ,     "tier18",      8,  1254,   18,   4,   2,              {   0 },    2,   70, 185648, { 124318, 124328, 124333, 124339, 124345,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",      8,  1253,   18,   2,   3,              {   0 },    2,  253, 188200, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",      8,  1253,   18,   2,   3,              {   0 },    2,  254, 188190, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",      8,  1253,   18,   2,   3,              {   0 },    2,  255, 188198, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",      8,  1253,   18,   4,   3,              {   0 },    2,  253, 188201, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",      8,  1253,   18,   4,   3,              {   0 },    2,  254, 188191, { 124284, 124292, 124296, 124301, 124307,  0 } },
  { "Battlegear of the Savage Hunt"              ,     "tier18",      8,  1253,   18,   4,   3,              {   0 },    2,  255, 188199, { 124284, 124292, 124296, 124301, 124307,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Felblade Armor"                             ,     "tier18",      8,  1256,   18,   2,   4,              {   0 },    2,  259, 186182, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Felblade Armor"                             ,     "tier18",      8,  1256,   18,   2,   4,              {   0 },    2,  260, 186285, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Felblade Armor"                             ,     "tier18",      8,  1256,   18,   2,   4,              {   0 },    2,  261, 186278, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Felblade Armor"                             ,     "tier18",      8,  1256,   18,   4,   4,              {   0 },    2,  259, 186277, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Felblade Armor"                             ,     "tier18",      8,  1256,   18,   4,   4,              {   0 },    2,  260, 186288, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Felblade Armor"                             ,     "tier18",      8,  1256,   18,   4,   4,              {   0 },    2,  261, 186279, { 124248, 124257, 124263, 124269, 124274,  0 } },
  { "Attire of Piety"                            ,     "tier18",      8,  1255,   18,   2,   5,              {   0 },    1,  256, 186477, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",      8,  1255,   18,   2,   5,              {   0 },    1,  257, 186298, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",      8,  1255,   18,   4,   5,              {   0 },    1,  256, 186492, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",      8,  1255,   18,   4,   5,              {   0 },    1,  257, 186378, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",      8,  1255,   18,   2,   5,              {   0 },    3,  258, 186980, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Attire of Piety"                            ,     "tier18",      8,  1255,   18,   4,   5,              {   0 },    3,  258, 186981, { 124155, 124161, 124166, 124172, 124178,  0 } },
  { "Demongaze Armor"                            ,     "tier18",      8,  1249,   18,   2,   6,              {   0 },    0,  250, 187872, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",      8,  1249,   18,   4,   6,              {   0 },    0,  250, 187873, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",      8,  1249,   18,   2,   6,              {   0 },    2,  251, 187868, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",      8,  1249,   18,   2,   6,              {   0 },    2,  252, 187865, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",      8,  1249,   18,   4,   6,              {   0 },    2,  251, 187870, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Demongaze Armor"                            ,     "tier18",      8,  1249,   18,   4,   6,              {   0 },    2,  252, 187866, { 124317, 124327, 124332, 124338, 124344,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",      8,  1257,   18,   2,   7,              {   0 },    1,  264, 185873, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",      8,  1257,   18,   4,   7,              {   0 },    1,  264, 185874, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",      8,  1257,   18,   2,   7,              {   0 },    2,  263, 185871, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",      8,  1257,   18,   4,   7,              {   0 },    2,  263, 185872, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",      8,  1257,   18,   2,   7,              {   0 },    3,  262, 185880, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Embrace of the Living Mountain"             ,     "tier18",      8,  1257,   18,   4,   7,              {   0 },    3,  262, 185881, { 124293, 124297, 124302, 124303, 124308,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",      8,  1251,   18,   2,   8,              {   0 },    3,   62, 186166, { 124154, 124160, 124165, 124171, 124177,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Raiment of the Arcanic Conclave"            ,     "tier18",      8,  1251,   18,   2,   8,              {   0 },    3,   63, 186167, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",      8,  1251,   18,   2,   8,              {   0 },    3,   64, 185969, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",      8,  1251,   18,   4,   8,              {   0 },    3,   62, 186165, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",      8,  1251,   18,   4,   8,              {   0 },    3,   63, 186168, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Raiment of the Arcanic Conclave"            ,     "tier18",      8,  1251,   18,   4,   8,              {   0 },    3,   64, 185971, { 124154, 124160, 124165, 124171, 124177,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",      8,  1259,   18,   2,   9,              {   0 },    3,  265, 185882, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",      8,  1259,   18,   2,   9,              {   0 },    3,  266, 185884, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",      8,  1259,   18,   2,   9,              {   0 },    3,  267, 185965, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",      8,  1259,   18,   4,   9,              {   0 },    3,  265, 185883, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",      8,  1259,   18,   4,   9,              {   0 },    3,  266, 185964, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Deathrattle Regalia"                        ,     "tier18",      8,  1259,   18,   4,   9,              {   0 },    3,  267, 185967, { 124156, 124162, 124167, 124173, 124179,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",      8,  1252,   18,   2,  10,              {   0 },    0,  268, 185398, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",      8,  1252,   18,   4,  10,              {   0 },    0,  268, 185399, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",      8,  1252,   18,   2,  10,              {   0 },    1,  270, 185126, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",      8,  1252,   18,   4,  10,              {   0 },    1,  270, 185258, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",      8,  1252,   18,   2,  10,              {   0 },    2,  269, 185542, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Battlewrap of the Hurricane's Eye"          ,     "tier18",      8,  1252,   18,   4,  10,              {   0 },    2,  269, 185543, { 124247, 124256, 124262, 124268, 124273,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",      8,  1250,   18,   2,  11,              {   0 },    0,  104, 185813, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",      8,  1250,   18,   4,  11,              {   0 },    0,  104, 185814, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",      8,  1250,   18,   2,  11,              {   0 },    1,  105, 185294, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",      8,  1250,   18,   4,  11,              {   0 },    1,  105, 185397, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",      8,  1250,   18,   2,  11,              {   0 },    2,  103, 185805, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",      8,  1250,   18,   4,  11,              {   0 },    2,  103, 185811, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",      8,  1250,   18,   2,  11,              {   0 },    3,  102, 187875, { 124246, 124255, 124261, 124267, 124272,  0 } },
  { "Oathclaw Wargarb"                           ,     "tier18",      8,  1250,   18,   4,  11,              {   0 },    3,  102, 187877, { 124246, 124255, 124261, 124267, 124272,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   2,   1,              {   0 },    0,   73, 187142, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   4,   1,              {   0 },    0,   73, 187143, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   2,   1,              {   0 },    2,   72, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   2,   1,              {   0 },    2,   71, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   4,   1,              {   0 },    2,   72, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   4,   1,              {   0 },    2,   71, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   2,   2,              {   0 },    0,   66, 187142, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   4,   2,              {   0 },    0,   66, 187143, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   2,   2,              {   0 },    1,   65, 187140, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   4,   2,              {   0 },    1,   65, 187155, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   2,   2,              {   0 },    2,   70, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   4,   2,              {   0 },    2,   70, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   2,   3,              {   0 },    2,  255, 187137, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   2,   3,              {   0 },    2,  254, 187137, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   2,   3,              {   0 },    2,  253, 187137, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   4,   3,              {   0 },    2,  255, 187688, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   4,   3,              {   0 },    2,  254, 187688, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   4,   3,              {   0 },    2,  253, 187688, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,   4,              {   0 },    2,  259, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,   4,              {   0 },    2,  260, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,   4,              {   0 },    2,  261, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,   4,              {   0 },    2,  259, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,   4,              {   0 },    2,  260, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,   4,              {   0 },    2,  261, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   2,   5,              {   0 },    1,  257, 187132, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   2,   5,              {   0 },    1,  256, 187132, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   4,   5,              {   0 },    1,  257, 187430, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   4,   5,              {   0 },    1,  256, 187430, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   2,   5,              {   0 },    3,  258, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   4,   5,              {   0 },    3,  258, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   2,   6,              {   0 },    0,  250, 187142, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   4,   6,              {   0 },    0,  250, 187143, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   2,   6,              {   0 },    2,  252, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   2,   6,              {   0 },    2,  251, 187141, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   4,   6,              {   0 },    2,  252, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Demonbreaker Wargear"                       ,  "tier18lfr",      9,  1263,   18,   4,   6,              {   0 },    2,  251, 187151, { 128127, 128032, 128139, 128134, 128135, 128123,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   2,   7,              {   0 },    1,  264, 187139, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   4,   7,              {   0 },    1,  264, 187158, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   2,   7,              {   0 },    2,  263, 187137, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   4,   7,              {   0 },    2,  263, 187688, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   2,   7,              {   0 },    3,  262, 187138, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Rancorbite Battlegear"                      ,  "tier18lfr",      9,  1262,   18,   4,   7,              {   0 },    3,  262, 187778, { 128126, 128080, 128138, 128132, 128133, 128122,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   2,   8,              {   0 },    3,   62, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   2,   8,              {   0 },    3,   63, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   2,   8,              {   0 },    3,   64, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   4,   8,              {   0 },    3,   62, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   4,   8,              {   0 },    3,   63, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   4,   8,              {   0 },    3,   64, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   2,   9,              {   0 },    3,  265, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   2,   9,              {   0 },    3,  266, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  // Set bonus name                              ,      OptName, EnumID, SetID, Tier, Bns, Cls,         Derived Spec, Role, Spec,  Spell, Items
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   2,   9,              {   0 },    3,  267, 187075, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   4,   9,              {   0 },    3,  265, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   4,   9,              {   0 },    3,  266, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Felfume Raiment"                            ,  "tier18lfr",      9,  1260,   18,   4,   9,              {   0 },    3,  267, 187079, { 128052, 128136, 128128, 128129, 128124, 128120,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,  10,              {   0 },    0,  268, 187136, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,  10,              {   0 },    0,  268, 187862, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,  10,              {   0 },    1,  270, 187134, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,  10,              {   0 },    1,  270, 187604, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,  10,              {   0 },    2,  269, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,  10,              {   0 },    2,  269, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,  11,              {   0 },    0,  104, 187136, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,  11,              {   0 },    0,  104, 187862, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,  11,              {   0 },    1,  105, 187134, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,  11,              {   0 },    1,  105, 187604, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,  11,              {   0 },    2,  103, 187135, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,  11,              {   0 },    2,  103, 187863, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   2,  11,              {   0 },    3,  102, 187133, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
  { "Ironpelt Armor"                             ,  "tier18lfr",      9,  1261,   18,   4,  11,              {   0 },    3,  102, 187435, { 128125, 128054, 128137, 128130, 128131, 128121,  0 } },
};
#define PERK_SIZE (3)

// Perk specialization abilities, wow build 21134
static unsigned __perk_data[][MAX_SPECS_PER_CLASS][PERK_SIZE] = {
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
  {
    {
           0,
    },
    // Protection
    {
      157488, // 2: Improved Block
           0,
    },
    {
           0,
    },
    {
           0,
    },
  },
  {
    {
           0,
    },
    // Marksmanship
    {
      157726, // 0: Improved Focus
           0,
    },
    {
           0,
    },
    {
           0,
    },
  },
  {
    {
           0,
    },
    {
           0,
    },
    // Subtlety
    {
      157666, // 0: Enhanced Vanish
      157671, // 2: Empowered Fan of Knives
      157704, // 3: Enhanced Stealth
    },
    {
           0,
    },
  },
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
};
// Glyph properties, wow build 21134
static glyph_property_data_t __glyph_property_data[72] = {
  {  1028, 125151 },
  {  1039, 125732 },
  {  1041, 125872 },
  {  1043, 125872 },
  {  1045, 125931 },
  {  1049, 126094 },
  {  1051, 126193 },
  {  1052, 126745 },
  {  1054, 131113 },
  {  1060, 145722 },
  {  1074, 146652 },
  {  1079, 146976 },
  {  1080, 146662 },
  {  1083, 146959 },
  {  1084, 145722 },
  {  1085, 147776 },
  {  1087, 147072 },
  {  1090, 147783 },
  {  1091, 147784 },
  {  1101, 146974 },
  {   176, 114333 },
  {  1206, 171803 },
  {   190,  89401 },
  {  1225, 211910 },
  {  1230, 212339 },
  {  1231, 212354 },
  {  1232, 212358 },
  {  1234, 212407 },
  {  1235, 212410 },
  {   278,  56246 },
  {  1244, 212636 },
  {  1245, 212642 },
  {  1243, 212635 },
  {   328,  56382 },
  {  1239, 212462 },
  {  1240, 212524 },
  {  1237, 212439 },
  {   432, 107059 },
  {   440,  57870 },
  {   454,  57979 },
  {   457,  57954 },
  {   461,  57986 },
  {   471,  58135 },
  {   482,  58094 },
  {   483,  58095 },
  {   486,  58099 },
  {   488,  58104 },
  {   514,  58640 },
  {   522,  58680 },
  {   529,  58642 },
  {  1238, 212451 },
  {  1224, 211899 },
  {  1241, 212526 },
  {   613, 114301 },
  {   733,  63268 },
  {   751,  63279 },
  {   761,  63312 },
  {  1242, 212631 },
  {  1220, 210333 },
  {  1221, 165020 },
  {   851,  68164 },
  {  1222, 127756 },
  {  1223, 211829 },
  {   961, 107906 },
  {   970, 112104 },
  {   989, 127755 },
  {   990, 115943 },
  {   991, 115946 },
  {  1004, 120581 },
  {  1007, 120581 },
  {  1020, 123779 },
  {     0,      0 }
};
#define RPPMMOD_SIZE (155)

// 155 RPPM Modifiers, wow build level 21134
static struct rppm_modifier_t __rppmmodifier_data[] = {
  {  29725,    4,  1,  1.0000 },
  {  49530,    4,  1,  1.0000 },
  {  51128,    4,  1,  1.0000 },
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
  { 137595,  250,  4, -0.4000 },
  { 137595,  104,  4, -0.4000 },
  { 137595,  268,  4, -0.4000 },
  { 137595,   66,  4, -0.4000 },
  { 137595,   73,  4, -0.4000 },
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
  { 138120,   72,  4, -0.6250 },
  { 138365,    4,  1,  1.0000 },
  { 138367,    4,  1,  1.0000 },
  { 138849,    4,  1,  1.0000 },
  { 138924,    4,  1,  1.0000 },
  { 138964,  256,  3, -0.4000 },
  { 138964,  528,  6,  1.0000 },
  { 138964,  102,  4, -0.3500 },
  { 138964,    0,  7, -0.7500 },
  { 138968,    4,  1,  1.0000 },
  { 139116,  528,  6,  1.0000 },
  { 139134,    4,  2,  1.0000 },
  { 139171,    4,  2,  1.0000 },
  { 139190,    4,  1,  1.0000 },
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
  { 146195,    4,  1,  1.0000 },
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
  { 146197,    4,  1,  1.0000 },
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
  { 187450,    4,  1,  1.0000 },
  { 188534,    4,  1,  1.0000 },
  { 188782,    4,  1,  1.0000 },
  { 188783,    4,  1,  1.0000 },
  { 191048,    4,  1,  1.0000 },
  { 191602,    4,  1,  1.0000 },
  { 191861,    4,  1,  1.0000 },
  { 192623,    4,  1,  1.0000 },
  { 192630,    4,  1,  1.0000 },
  { 192645,    4,  1,  1.0000 },
  { 192646,    4,  1,  1.0000 },
  { 192657,    4,  1,  1.0000 },
  { 192759,    4,  1,  1.0000 },
  { 192923,    4,  1,  1.0000 },
  { 194909,    4,  1,  1.0000 },
  { 195255,    4,  1,  1.0000 },
  { 196099,    5,  1,  1.0000 },
  { 196777,    4,  1,  1.0000 },
  { 197846,    4,  1,  1.0000 },
  { 198068,    4,  1,  1.0000 },
  { 198735,    4,  1,  1.0000 },
  { 198736,    4,  1,  1.0000 },
  { 199806,    4,  1,  1.0000 },
  { 201336,    4,  1,  1.0000 },
  { 201407,    4,  1,  1.0000 },
  { 201466,    4,  1,  1.0000 },
  { 201471,    4,  1,  1.0000 },
  { 202387,    4,  1,  1.0000 },
  { 203557,    4,  1,  1.0000 },
  { 203576,    4,  1,  1.0000 },
  { 207320,    4,  1,  1.0000 },
  { 208213,    4,  1,  1.0000 },
  { 209579,    4,  1,  1.0000 },
  { 210676,    4,  1,  1.0000 },
  { 210702,    4,  1,  1.0000 },
  { 210719,    4,  1,  1.0000 },
  { 211984,    4,  1,  1.0000 },
  {      0,    0,  0,  0.0000 },
};
