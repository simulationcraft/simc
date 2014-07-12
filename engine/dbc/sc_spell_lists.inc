#define CLASS_ABILITY_SIZE (49)
#define CLASS_ABILITY_TREE_SIZE (6)

#ifndef MAX_CLASS
#define MAX_CLASS (12)
#endif
// Class based active abilities, wow build 18546
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
    // General tree, 17 abilities
    {
        6673, // Battle Shout
        2457, // Battle Stance
       18499, // Berserker Rage
         100, // Charge
         469, // Commanding Shout
          71, // Defensive Stance
        1715, // Hamstring
        6544, // Heroic Leap
          78, // Heroic Strike
       57755, // Heroic Throw
        3411, // Intervene
        5246, // Intimidating Shout
      114192, // Mocking Banner
        6552, // Pummel
       23920, // Spell Reflection
         355, // Taunt
       34428, // Victory Rush
           0,
    },
    // Arms tree, 10 abilities
    {
      167105, // Colossus Smash
      118038, // Die by the Sword
      163201, // Execute
       12294, // Mortal Strike
       97462, // Rallying Cry
        1719, // Recklessness
         772, // Rend
       12328, // Sweeping Strikes
        6343, // Thunder Clap
        1680, // Whirlwind
           0,
    },
    // Fury tree, 9 abilities
    {
       23881, // Bloodthirst
       86346, // Colossus Smash
      118038, // Die by the Sword
        5308, // Execute
       85288, // Raging Blow
       97462, // Rallying Cry
        1719, // Recklessness
        1680, // Whirlwind
      100130, // Wild Strike
           0,
    },
    // Protection tree, 10 abilities
    {
        1160, // Demoralizing Shout
       20243, // Devastate
        5308, // Execute
       12975, // Last Stand
        6572, // Revenge
      112048, // Shield Barrier
        2565, // Shield Block
       23922, // Shield Slam
         871, // Shield Wall
        6343, // Thunder Clap
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
    // General tree, 23 abilities
    {
       20217, // Blessing of Kings
       19740, // Blessing of Might
        4987, // Cleanse
       35395, // Crusader Strike
         498, // Divine Protection
         642, // Divine Shield
         853, // Hammer of Justice
       24275, // Hammer of Wrath
       53595, // Hammer of the Righteous
        1044, // Hand of Freedom
        1022, // Hand of Protection
        6940, // Hand of Sacrifice
        1038, // Hand of Salvation
       20271, // Judgment
         633, // Lay on Hands
       96231, // Rebuke
       62124, // Reckoning
       25780, // Righteous Fury
       20925, // Sacred Shield
      105361, // Seal of Command
       20165, // Seal of Insight
       10326, // Turn Evil
       85673, // Word of Glory
           0,
    },
    // Holy tree, 9 abilities
    {
       31842, // Avenging Wrath
       53563, // Beacon of Light
        2812, // Denounce
       31821, // Devotion Aura
       19750, // Flash of Light
       82326, // Holy Light
       82327, // Holy Radiance
       20473, // Holy Shock
       85222, // Light of Dawn
           0,
    },
    // Protection tree, 11 abilities
    {
       31850, // Ardent Defender
       31935, // Avenger's Shield
       26573, // Consecration
       19750, // Flash of Light
       86659, // Guardian of Ancient Kings
       53595, // Hammer of the Righteous
        1038, // Hand of Salvation
      119072, // Holy Wrath
       20154, // Seal of Righteousness
       31801, // Seal of Truth
       53600, // Shield of the Righteous
           0,
    },
    // Retribution tree, 10 abilities
    {
       31884, // Avenging Wrath
       53385, // Divine Storm
      121783, // Emancipate
         879, // Exorcism
       19750, // Flash of Light
       53595, // Hammer of the Righteous
       20164, // Seal of Justice
       20154, // Seal of Righteousness
       31801, // Seal of Truth
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
    // General tree, 28 abilities
    {
        5118, // Aspect of the Cheetah
       13159, // Aspect of the Pack
        1462, // Beast Lore
         883, // Call Pet 1
       83242, // Call Pet 2
       83243, // Call Pet 3
       83244, // Call Pet 4
       83245, // Call Pet 5
       51753, // Camouflage
        5116, // Concussive Shot
      147362, // Counter Shot
       19263, // Deterrence
       20736, // Distracting Shot
       13813, // Explosive Trap (Fire Trap)
        6991, // Feed Pet
        5384, // Feign Death
        1543, // Flare
        1499, // Freezing Trap (Frost Trap)
       13809, // Ice Trap (Frost Trap)
       53271, // Master's Call
         136, // Mend Pet
       34477, // Misdirection
        2643, // Multi-Shot
         982, // Revive Pet
       56641, // Steady Shot
        1515, // Tame Beast
       19801, // Tranquilizing Shot
       77769, // Trap Launcher
           0,
    },
    // Beast Mastery tree, 6 abilities
    {
        3044, // Arcane Shot
       19574, // Bestial Wrath
       77767, // Cobra Shot
       82692, // Focus Fire
       34026, // Kill Command
       53351, // Kill Shot
           0,
    },
    // Marksmanship tree, 4 abilities
    {
       19434, // Aimed Shot
       53209, // Chimera Shot
       53351, // Kill Shot
        3045, // Rapid Fire
           0,
    },
    // Survival tree, 4 abilities
    {
        3044, // Arcane Shot
        3674, // Black Arrow
       77767, // Cobra Shot
       53301, // Explosive Shot
           0,
    },
    {
           0,
    },
    // Pet tree, 49 abilities
    {
       55749, // Acid Spit (Special Ability)
      160011, // Agile Reflexes (Special Ability)
       90355, // Ancient Hysteria (Exotic Ability)
       50433, // Ankle Crack (Special Ability)
      159988, // Bark of the Wild
       17253, // Bite (Basic Attack)
      160017, // Blessing of Kongs
       24423, // Bloody Screech (Special Ability)
       93433, // Burrow Attack (Exotic Ability)
      128432, // Cackling Howl (Special Ability)
       16827, // Claw (Basic Attack)
      160060, // Deadly Sting (Special Ability)
      159956, // Dust of Life (Exotic Ability)
      135678, // Energizing Spores
      159953, // Feast (Special Ability)
       34889, // Fire Breath (Special Ability)
       54644, // Frost Breath (Special Ability)
       92380, // Froststorm Breath (Exotic Ability)
       24604, // Furious Howl
       35290, // Gore (Special Ability)
        2649, // Growl (Basic Ability)
       19577, // Intimidation
       50256, // Invigorating Roar (Special Ability)
       58604, // Lava Breath (Special Ability)
      138374, // Lightning Blast
       24844, // Lightning Breath (Special Ability)
       54680, // Monstrous Bite (Exotic Ability)
      160073, // Plainswalking (Special Ability)
       90347, // Play (Bonus Ability)
       24450, // Prowl (Bonus Ability)
       90364, // Qiraji Fortitude (Exotic Ability)
       50518, // Ravage (Special Ability)
       94019, // Rest (Bonus Ability)
       93435, // Roar of Courage (Special Ability)
      128433, // Serpent's Cunning (Special Ability)
       49966, // Smack (Basic Attack)
       50519, // Sonic Blast (Special Ability)
      160074, // Speed of the Swarm (Special Ability)
      128997, // Spirit Beast Blessing (Special Ability)
       90361, // Spirit Mend (Exotic Ability)
       90328, // Spirit Walk (Bonus Ability)
       57386, // Stampede (Special Ability)
      160077, // Strength of the Earth
      160052, // Strength of the Pack (Special Ability)
      160065, // Tendon Rip (Special Ability)
       90309, // Terrifying Roar (Special Ability)
       94022, // Trick (Bonus Ability)
       35346, // Warp Time (Special Ability)
      160067, // Web Spray (Special Ability)
    },
  },
  // Class active abilities for Rogue
  {
    // General tree, 30 abilities
    {
        8676, // Ambush
        2094, // Blind
        1833, // Cheap Shot
       31224, // Cloak of Shadows
      121411, // Crimson Tempest
        3408, // Crippling Poison
        2823, // Deadly Poison
      152150, // Death from Above
        1725, // Distract
        5277, // Evasion
        2098, // Eviscerate
        1966, // Feint
         703, // Garrote
        1776, // Gouge
        1766, // Kick
         408, // Kidney Shot
       14185, // Preparation
       73651, // Recuperate
        6770, // Sap
        5938, // Shiv
      114018, // Shroud of Concealment
        1752, // Sinister Strike
        5171, // Slice and Dice
       76577, // Smoke Bomb
        2983, // Sprint
        1784, // Stealth
      121733, // Throw
       57934, // Tricks of the Trade
        1856, // Vanish
        8679, // Wound Poison
           0,
    },
    // Assassination tree, 6 abilities
    {
      111240, // Dispatch
       32645, // Envenom
       51723, // Fan of Knives
        1329, // Mutilate
        1943, // Rupture
       79140, // Vendetta
           0,
    },
    // Combat tree, 4 abilities
    {
       13750, // Adrenaline Rush
       13877, // Blade Flurry
       51690, // Killing Spree
       84617, // Revealing Strike
           0,
    },
    // Subtlety tree, 5 abilities
    {
          53, // Backstab
       51723, // Fan of Knives
       16511, // Hemorrhage
        1943, // Rupture
       51713, // Shadow Dance
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
    // General tree, 15 abilities
    {
         528, // Dispel Magic
         586, // Fade
        6346, // Fear Ward
        2061, // Flash Heal
       73325, // Leap of Faith
        1706, // Levitate
       32375, // Mass Dispel
       48045, // Mind Sear
       21562, // Power Word: Fortitude
          17, // Power Word: Shield
       33076, // Prayer of Mending
        9484, // Shackle Undead
         589, // Shadow Word: Pain
       34433, // Shadowfiend
         585, // Smite
           0,
    },
    // Discipline tree, 9 abilities
    {
       81700, // Archangel
        2060, // Heal
       14914, // Holy Fire
      132157, // Holy Nova (Discipline)
       33206, // Pain Suppression
       47540, // Penance
       62618, // Power Word: Barrier
         596, // Prayer of Healing
         527, // Purify
           0,
    },
    // Holy tree, 14 abilities
    {
       32546, // Binding Heal
       81209, // Chakra: Chastise
       81206, // Chakra: Sanctuary
       81208, // Chakra: Serenity
       34861, // Circle of Healing
       64843, // Divine Hymn
       47788, // Guardian Spirit
        2060, // Heal
       14914, // Holy Fire
       88625, // Holy Word: Chastise
      126135, // Lightwell
         596, // Prayer of Healing
         527, // Purify
         139, // Renew
           0,
    },
    // Shadow tree, 12 abilities
    {
        2944, // Devouring Plague
       47585, // Dispersion
        8092, // Mind Blast
       15407, // Mind Flay
      129197, // Mind Flay (Insanity)
       73510, // Mind Spike
       64044, // Psychic Horror
       32379, // Shadow Word: Death
       15473, // Shadowform
       15487, // Silence
       15286, // Vampiric Embrace
       34914, // Vampiric Touch
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
    // General tree, 23 abilities
    {
       48707, // Anti-Magic Shell
       42650, // Army of the Dead
       48263, // Blood Presence
       45524, // Chains of Ice
      111673, // Control Undead
       77606, // Dark Simulacrum
       47541, // Death Coil
       49576, // Death Grip
       49998, // Death Strike
       43265, // Death and Decay
       47568, // Empower Rune Weapon
       48266, // Frost Presence
       57330, // Horn of Winter
       48792, // Icebound Fortitude
       45477, // Icy Touch
       47528, // Mind Freeze
       77575, // Outbreak
        3714, // Path of Frost
       50842, // Pestilence
       45462, // Plague Strike
       61999, // Raise Ally
       47476, // Strangulate
       48265, // Unholy Presence
           0,
    },
    // Blood tree, 6 abilities
    {
       49222, // Bone Shield
       49028, // Dancing Rune Weapon
       56222, // Dark Command
       48982, // Rune Tap
      114866, // Soul Reaper
       55233, // Vampiric Blood
           0,
    },
    // Frost tree, 5 abilities
    {
       49143, // Frost Strike
       49184, // Howling Blast
       49020, // Obliterate
       51271, // Pillar of Frost
      130735, // Soul Reaper
           0,
    },
    // Unholy tree, 6 abilities
    {
       63560, // Dark Transformation
       85948, // Festering Strike
       46584, // Raise Dead
       55090, // Scourge Strike
      130736, // Soul Reaper
       49206, // Summon Gargoyle
           0,
    },
    {
           0,
    },
    // Pet tree, 4 abilities
    {
       47468, // Claw
       51963, // Gargoyle Strike
       47481, // Gnaw
       47484, // Huddle
           0,
    },
  },
  // Class active abilities for Shaman
  {
    // General tree, 27 abilities
    {
        2008, // Ancestral Spirit
      114049, // Ascendance
        2825, // Bloodlust
      108269, // Capacitor Totem (Air Totem)
         421, // Chain Lightning
       51886, // Cleanse Spirit
        2062, // Earth Elemental Totem (Earth Totem)
        2484, // Earthbind Totem (Earth Totem)
        2894, // Fire Elemental Totem (Fire Totem)
        8050, // Flame Shock
        8056, // Frost Shock
        2645, // Ghost Wolf
        8177, // Grounding Totem (Air Totem)
       73920, // Healing Rain
        5394, // Healing Stream Totem (Water Totem)
        8004, // Healing Surge
       32182, // Heroism
       51514, // Hex
         403, // Lightning Bolt
         324, // Lightning Shield
       73899, // Primal Strike
         370, // Purge
        3599, // Searing Totem (Fire Totem)
        8143, // Tremor Totem (Earth Totem)
       73685, // Unleash Life
       57994, // Wind Shear
      115356, // Windstrike
           0,
    },
    // Elemental tree, 8 abilities
    {
      165339, // Ascendance
        8042, // Earth Shock
       61882, // Earthquake
       51505, // Lava Burst
       30823, // Shamanistic Rage
       79206, // Spiritwalker's Grace
       51490, // Thunderstorm
      165462, // Unleash Flame
           0,
    },
    // Enhancement tree, 9 abilities
    {
      165341, // Ascendance
       51533, // Feral Spirit
        1535, // Fire Nova
       60103, // Lava Lash
        8190, // Magma Totem (Fire Totem)
       30823, // Shamanistic Rage
       58875, // Spirit Walk
       17364, // Stormstrike
       73680, // Unleash Elements
           0,
    },
    // Restoration tree, 12 abilities
    {
      165344, // Ascendance
        1064, // Chain Heal
         974, // Earth Shield
      108280, // Healing Tide Totem (Water Totem)
       77472, // Healing Wave
       51505, // Lava Burst
       77130, // Purify Spirit
       61295, // Riptide
       98008, // Spirit Link Totem (Air Totem)
       79206, // Spiritwalker's Grace
       73685, // Unleash Life
       52127, // Water Shield
           0,
    },
    {
           0,
    },
    // Pet tree, 7 abilities
    {
       36213, // Angered Earth
      157348, // Call Lightning
       57984, // Fire Blast
      117588, // Fire Nova
      118297, // Immolate
      118345, // Pulverize
      157331, // Wind Gust
           0,
    },
  },
  // Class active abilities for Mage
  {
    // General tree, 15 abilities
    {
      159916, // Amplify Magic
      120145, // Ancient Teleport: Dalaran
        1459, // Arcane Brilliance
        1953, // Blink
         120, // Cone of Cold
        2139, // Counterspell
       61316, // Dalaran Brilliance
        2136, // Fire Blast
         122, // Frost Nova
       44614, // Frostfire Bolt
       45438, // Ice Block
          66, // Invisibility
      125430, // Mage Bomb
         475, // Remove Curse
       80353, // Time Warp
           0,
    },
    // Arcane tree, 8 abilities
    {
       44425, // Arcane Barrage
       30451, // Arcane Blast
        1449, // Arcane Explosion
        5143, // Arcane Missiles
       12042, // Arcane Power
       12051, // Evocation
       12043, // Presence of Mind
       31589, // Slow
           0,
    },
    // Fire tree, 7 abilities
    {
       11129, // Combustion
       31661, // Dragon's Breath
         133, // Fireball
        2120, // Flamestrike
      108853, // Inferno Blast
       11366, // Pyroblast
        2948, // Scorch
           0,
    },
    // Frost tree, 7 abilities
    {
          10, // Blizzard
       44572, // Deep Freeze
         116, // Frostbolt
       84714, // Frozen Orb
       30455, // Ice Lance
       12472, // Icy Veins
       31687, // Summon Water Elemental
           0,
    },
    {
           0,
    },
    // Pet tree, 5 abilities
    {
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
    // General tree, 33 abilities
    {
         710, // Banish
         172, // Corruption
        6201, // Create Healthstone
      109773, // Dark Intent
       77801, // Dark Soul
      111771, // Demonic Gateway
         689, // Drain Life
        1098, // Enslave Demon
        5782, // Fear
      111897, // Grimoire: Felhunter (Summon)
      111859, // Grimoire: Imp (Summon)
      111896, // Grimoire: Succubus (Summon)
      111895, // Grimoire: Voidwalker (Summon)
        5484, // Howl of Terror
        1454, // Life Tap
      114790, // Seed of Corruption (Soulburn)
         686, // Shadow Bolt
      119678, // Soul Swap (Soulburn)
       29858, // Soulshatter
       20707, // Soulstone
      112921, // Summon Abyssal (Guardian)
       18540, // Summon Doomguard (Guardian)
      112866, // Summon Fel Imp (Summon)
         691, // Summon Felhunter (Summon)
         688, // Summon Imp (Summon)
        1122, // Summon Infernal (Guardian)
      112869, // Summon Observer (Summon)
      112868, // Summon Shivarra (Summon)
         712, // Summon Succubus (Summon)
      112927, // Summon Terrorguard (Guardian)
      112867, // Summon Voidlord (Summon)
         697, // Summon Voidwalker (Summon)
      104773, // Unending Resolve
           0,
    },
    // Affliction tree, 10 abilities
    {
         980, // Agony
      113860, // Dark Soul: Misery
      103103, // Drain Soul
       48181, // Haunt
         755, // Health Funnel
      114790, // Seed of Corruption (Soulburn)
       27243, // Seed of Corruption
       86121, // Soul Swap
       74434, // Soulburn
       30108, // Unstable Affliction
           0,
    },
    // Demonology tree, 14 abilities
    {
      124916, // Chaos Wave (Metamorphosis)
      113861, // Dark Soul: Knowledge
      109151, // Demonic Leap
      111898, // Grimoire: Felguard (Summon)
      105174, // Hand of Gul'dan
         755, // Health Funnel
        1949, // Hellfire
      104025, // Immolation Aura (Metamorphosis)
      103958, // Metamorphosis
        6353, // Soul Fire
      104027, // Soul Fire (Metamorphosis)
       30146, // Summon Felguard (Summon)
      112870, // Summon Wrathguard (Summon)
      103964, // Touch of Chaos (Metamorphosis)
           0,
    },
    // Destruction tree, 11 abilities
    {
      116858, // Chaos Bolt
       17962, // Conflagrate
      113858, // Dark Soul: Instability
      114635, // Ember Tap
      108683, // Fire and Brimstone
       80240, // Havoc
         348, // Immolate
       29722, // Incinerate
        5740, // Rain of Fire
      104232, // Rain of Fire
       17877, // Shadowburn
           0,
    },
    {
           0,
    },
    // Pet tree, 28 abilities
    {
       89766, // Axe Toss (Special Ability)
      115748, // Bladedance (Basic Attack)
      119899, // Cauterize Master (Special Ability)
       19505, // Devour Magic (Special Ability)
       19658, // Devour Magic
       85692, // Doom Bolt
      115746, // Felbolt (Basic Attack)
      115770, // Fellash (Special Ability)
       89751, // Felstorm (Special Ability)
        3110, // Firebolt (Basic Attack)
       89792, // Flee (Special Ability)
        7814, // Lash of Pain (Basic Attack)
       30213, // Legion Strike (Basic Attack)
        7870, // Lesser Invisibility (Special Ability)
      115625, // Mortal Cleave (Basic Attack)
       30151, // Pursuit (Special Ability)
        6358, // Seduction (Special Ability)
       54049, // Shadow Bite (Basic Attack)
      115232, // Shadow Shield (Special Ability)
       89808, // Singe Magic (Special Ability)
       19647, // Spell Lock (Special Ability)
       17735, // Suffering (Special Ability)
      134477, // Threatening Presence (Special Ability)
      112042, // Threatening Presence (Special Ability)
      115778, // Tongue Lash (Basic Attack)
        3716, // Torment (Basic Attack)
        6360, // Whiplash (Special Ability)
      115831, // Wrathstorm (Special Ability)
           0,
    },
  },
  // Class active abilities for Monk
  {
    // General tree, 22 abilities
    {
      100784, // Blackout Kick
      117952, // Crackling Jade Lightning
      125170, // Detox
      115450, // Detox
      115072, // Expel Harm
      115203, // Fortifying Brew
      100780, // Jab
      115543, // Leer of the Ox (Major Glyph)
      137562, // Nimble Brew
      115078, // Paralysis
      115546, // Provoke
      109132, // Roll
      116705, // Spear Hand Strike
      101546, // Spinning Crane Kick
      103985, // Stance of the Fierce Tiger (Stance)
      123273, // Surging Mist
      116694, // Surging Mist
      100787, // Tiger Palm
      115080, // Touch of Death
      101643, // Transcendence
      119996, // Transcendence: Transfer
      126892, // Zen Pilgrimage
           0,
    },
    // Brewmaster tree, 10 abilities
    {
      115181, // Breath of Fire
      115180, // Dizzying Haze
      115308, // Elusive Brew
      115295, // Guard
      121253, // Keg Smash
      116781, // Legacy of the White Tiger
      119582, // Purifying Brew
      115069, // Stance of the Sturdy Ox (Brewmaster Stance)
      115315, // Summon Black Ox Statue
      115176, // Zen Meditation
           0,
    },
    // Mistweaver tree, 14 abilities
    {
      115460, // Detonate Chi
      124682, // Enveloping Mist
      115921, // Legacy of the Emperor
      116849, // Life Cocoon
      115294, // Mana Tea
      115151, // Renewing Mist
      115310, // Revival
      107428, // Rising Sun Kick
      115175, // Soothing Mist
      154436, // Stance of the Spirited Crane (Mistweaver Stance)
      115070, // Stance of the Wise Serpent (Mistweaver Stance)
      115313, // Summon Jade Serpent Statue
      116680, // Thunder Focus Tea
      116670, // Uplift
           0,
    },
    // Windwalker tree, 12 abilities
    {
      116768, // Combo Breaker: Blackout Kick
      118864, // Combo Breaker: Tiger Palm
      116095, // Disable
      115288, // Energizing Brew
      113656, // Fists of Fury
      101545, // Flying Serpent Kick
      116781, // Legacy of the White Tiger
      107428, // Rising Sun Kick
      137639, // Storm, Earth, and Fire
      116740, // Tigereye Brew
      122470, // Touch of Karma
      115176, // Zen Meditation
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
        5487, // Bear Form (Shapeshift)
         768, // Cat Form (Shapeshift)
       33786, // Cyclone
        1850, // Dash
         339, // Entangling Roots
      102355, // Faerie Swarm (Talent)
       22568, // Ferocious Bite
       22842, // Frenzied Regeneration
        6795, // Growl
        5185, // Healing Touch
       16914, // Hurricane (Solar)
      102560, // Incarnation: Chosen of Elune (Talent, Shapeshift)
      102543, // Incarnation: King of the Jungle (Talent, Shapeshift)
      102558, // Incarnation: Son of Ursoc (Talent, Shapeshift)
       33917, // Mangle
        1126, // Mark of the Wild
        8921, // Moonfire (Lunar)
        5215, // Prowl
       20484, // Rebirth
         774, // Rejuvenation
        5221, // Shred
        2908, // Soothe
      106898, // Stampeding Roar
         783, // Travel Form (Shapeshift)
        5176, // Wrath (Solar)
           0,
    },
    // Balance tree, 12 abilities
    {
      127663, // Astral Communion
      106996, // Astral Storm (Lunar)
       22812, // Barkskin
      112071, // Celestial Alignment
       24858, // Moonkin Form
        2782, // Remove Corruption
       78675, // Solar Beam
       48505, // Starfall (Lunar)
        2912, // Starfire (Lunar)
       78674, // Starsurge (Lunar and Solar)
       93402, // Sunfire (Solar)
       88747, // Wild Mushroom
           0,
    },
    // Feral tree, 11 abilities
    {
      106952, // Berserk
         770, // Faerie Fire
       22570, // Maim
        1822, // Rake
        2782, // Remove Corruption
        1079, // Rip
       52610, // Savage Roar
      106839, // Skull Bash
       61336, // Survival Instincts
      106785, // Swipe
        5217, // Tiger's Fury
           0,
    },
    // Guardian tree, 10 abilities
    {
       22812, // Barkskin
      106952, // Berserk
         770, // Faerie Fire
       33745, // Lacerate
        6807, // Maul
      106922, // Might of Ursoc
        2782, // Remove Corruption
       62606, // Savage Defense
      106839, // Skull Bash
       61336, // Survival Instincts
           0,
    },
    // Restoration tree, 11 abilities
    {
       22812, // Barkskin
      145518, // Genesis
      102342, // Ironbark
       33763, // Lifebloom
       88423, // Nature's Cure
      132158, // Nature's Swiftness
        8936, // Regrowth
       18562, // Swiftmend
         740, // Tranquility
       48438, // Wild Growth
      145205, // Wild Mushroom
           0,
    },
    // Pet tree, 2 abilities
    {
      150017, // Rake
      113769, // Wrath
           0,
    },
  },
};

#define TREE_SPECIALIZATION_SIZE (28)

// Talent tree specialization abilities, wow build 18546
static unsigned __tree_specialization_data[][MAX_SPECS_PER_CLASS][TREE_SPECIALIZATION_SIZE] = {
  // Specialization abilities for Hunter pets
  {
    // Specialization abilities for Ferocity
    {
       53482, // Blood of the Rhino
       19596, // Boar's Speed
       20782, // Combat Experience
      156843, // Combat Experience
       53497, // Cornered
       61684, // Dash
       61684, // Dash
       61688, // Great Stamina
       55709, // Heart of the Phoenix
       55709, // Heart of the Phoenix
       53478, // Last Stand
       53480, // Roar of Sacrifice
       53184, // Spiked Collar
       53184, // Spiked Collar
           0,
    },
    // Specialization abilities for Tenacious Adaptation
    {
       53482, // Blood of the Rhino
       53482, // Blood of the Rhino
       19596, // Boar's Speed
       61685, // Charge
       61685, // Charge
       20782, // Combat Experience
      156843, // Combat Experience
       53497, // Cornered
       61688, // Great Stamina
       61688, // Great Stamina
       53478, // Last Stand
       53478, // Last Stand
       53480, // Roar of Sacrifice
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
      156843, // Combat Experience
       53497, // Cornered
       53497, // Cornered
       61684, // Dash
       61684, // Dash
       61688, // Great Stamina
       53478, // Last Stand
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
      167105, // Colossus Smash
      118038, // Die by the Sword
      163201, // Execute
      137049, // Hotfix Passive
      167188, // Inspiring Presence
       76838, // Mastery: Weapons Master
       12294, // Mortal Strike
       86101, // Plate Specialization
       97462, // Rallying Cry
      145990, // Readiness: Arms
        1719, // Recklessness
         772, // Rend
       12712, // Seasoned Soldier
      162698, // Stat Negation Aura - Strength DPS
       12328, // Sweeping Strikes
        6343, // Thunder Clap
      165365, // Weapon Mastery
        1680, // Whirlwind
           0,
    },
    // Specialization abilities for Fury
    {
       46915, // Bloodsurge
       23881, // Bloodthirst
       86346, // Colossus Smash
       23588, // Crazed Berserker
      165383, // Cruelty
      118038, // Die by the Sword
       13046, // Enrage
        5308, // Execute
      137050, // Hotfix Passive
      167188, // Inspiring Presence
       76856, // Mastery: Unshackled Fury
       12950, // Meat Cleaver
       86110, // Plate Specialization
       85288, // Raging Blow
       97462, // Rallying Cry
      145991, // Readiness: Fury
        1719, // Recklessness
       81099, // Single-Minded Fury
      162698, // Stat Negation Aura - Strength DPS
       46917, // Titan's Grip
        1680, // Whirlwind
      100130, // Wild Strike
           0,
    },
    // Specialization abilities for Protection
    {
       84608, // Bastion of Defense
      161608, // Bladed Armor
      131086, // Bladestorm (Protection buff) DND
      159362, // Blood Craze
      115768, // Deep Wounds
        1160, // Demoralizing Shout
       20243, // Devastate
       13046, // Enrage
        5308, // Execute
      137048, // Hotfix Passive
       12975, // Last Stand
       76857, // Mastery: Critical Block
       86535, // Plate Specialization
      145992, // Readiness: Protection
      158298, // Resolve
        6572, // Revenge
      161798, // Riposte
      112048, // Shield Barrier
        2565, // Shield Block
      165393, // Shield Mastery
       23922, // Shield Slam
         871, // Shield Wall
      162702, // Stat Negation Aura - Strength Tank
       46953, // Sword and Board
        6343, // Thunder Clap
      122509, // Ultimatum
       29144, // Unwavering Sentinel
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
       31842, // Avenging Wrath
       53563, // Beacon of Light
       88821, // Daybreak
        2812, // Denounce
       31821, // Devotion Aura
       19750, // Flash of Light
      158920, // Healer Level 90 Mana Fix
      112859, // Holy Insight
       82326, // Holy Light
       82327, // Holy Radiance
       20473, // Holy Shock
      137029, // Hotfix Passive
       53576, // Infusion of Light
       85222, // Light of Dawn
       53651, // Light's Beacon
       76669, // Mastery: Illuminated Healing
       86103, // Plate Specialization
      145975, // Readiness: Holy
       53551, // Sacred Cleansing
      148040, // Sacred Shield Overridde (Holy)
      165380, // Sanctified Light
      162701, // Stat Negation Aura - Intellect Healer
       31868, // Supplication
           0,
    },
    // Specialization abilities for Protection
    {
       31850, // Ardent Defender
       31935, // Avenger's Shield
      161608, // Bladed Armor
       26573, // Consecration
       19750, // Flash of Light
       85043, // Grand Crusader
       53592, // Guarded by the Light
       86659, // Guardian of Ancient Kings
       53595, // Hammer of the Righteous
        1038, // Hand of Salvation
      119072, // Holy Wrath
      137028, // Hotfix Passive
      105424, // Judgments of the Wise
       76671, // Mastery: Divine Bulwark
       86102, // Plate Specialization
      145976, // Readiness: Protection
      158298, // Resolve
      161800, // Riposte
      165375, // Sacred Duty
      105805, // Sanctuary
       20154, // Seal of Righteousness
       31801, // Seal of Truth
       53600, // Shield of the Righteous
      159374, // Shining Protector
      162702, // Stat Negation Aura - Strength Tank
           0,
    },
    // Specialization abilities for Retribution
    {
      140333, // Absolve
       31884, // Avenging Wrath
       53385, // Divine Storm
      121783, // Emancipate
       87138, // Exorcism
         879, // Exorcism
       19750, // Flash of Light
       53595, // Hammer of the Righteous
       53595, // Hammer of the Righteous
      137027, // Hotfix Passive
       76672, // Mastery: Hand of Light
       86539, // Plate Specialization
      145978, // Readiness: Retribution
      165381, // Righteous Vengeance
      167187, // Sanctity Aura
       20164, // Seal of Justice
       20154, // Seal of Righteousness
       31801, // Seal of Truth
      162698, // Stat Negation Aura - Strength DPS
       31868, // Supplication
       53503, // Sword of Light
       20113, // Sword of Light part two (Do not display)
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
      165389, // Animal Handler
        3044, // Arcane Shot
      115939, // Beast Cleave
       19574, // Bestial Wrath
       77767, // Cobra Shot
       53260, // Cobra Strikes
       53270, // Exotic Beasts
       82692, // Focus Fire
       19623, // Frenzy
       34954, // Go for the Throat
      137015, // Hotfix Passive
       53253, // Invigoration
       34026, // Kill Command
       53351, // Kill Shot
       56315, // Kindred Spirits
       76657, // Mastery: Master of Beasts
      145964, // Readiness: Beast Mastery
           0,
    },
    // Specialization abilities for Marksmanship
    {
       19434, // Aimed Shot
       35110, // Bombardment
       34483, // Careful Aim
       53209, // Chimera Shot
      137016, // Hotfix Passive
       53351, // Kill Shot
      165378, // Lethal Shots
       76659, // Mastery: Sniper Training
        3045, // Rapid Fire
      145965, // Readiness: Marksmanship
           0,
    },
    // Specialization abilities for Survival
    {
        3044, // Arcane Shot
        3674, // Black Arrow
       77767, // Cobra Shot
       19387, // Entrapment
       53301, // Explosive Shot
      137017, // Hotfix Passive
      165396, // Lightning Reflexes
       56343, // Lock and Load
       76658, // Mastery: Essence of the Viper
      145966, // Readiness: Survival
       87935, // Serpent Sting
      164856, // Survivalist
       63458, // Trap Mastery
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
      121152, // Blindside
       51667, // Cut to the Chase
      111240, // Dispatch
       32645, // Envenom
       51723, // Fan of Knives
      137037, // Hotfix Passive
       14117, // Improved Poisons
      165390, // Master Poisoner
       76803, // Mastery: Potent Poisons
        1329, // Mutilate
      145983, // Readiness: Assassination
       58423, // Relentless Strikes
        1943, // Rupture
       14190, // Seal Fate
       79140, // Vendetta
       79134, // Venomous Wounds
           0,
    },
    // Specialization abilities for Combat
    {
       13750, // Adrenaline Rush
       84654, // Bandit's Guile
       13877, // Blade Flurry
       35551, // Combat Potency
      137036, // Hotfix Passive
       51690, // Killing Spree
       76806, // Mastery: Main Gauche
      145984, // Readiness: Combat
       84617, // Revealing Strike
       14161, // Ruthlessness
       61329, // Vitality
           0,
    },
    // Specialization abilities for Subtlety
    {
          53, // Backstab
       79152, // Energetic Recovery
       51723, // Fan of Knives
       91023, // Find Weakness
       16511, // Hemorrhage
       51701, // Honor Among Thieves
      137035, // Hotfix Passive
       31223, // Master of Subtlety
       76808, // Mastery: Executioner
       14183, // Premeditation
      145985, // Readiness: Subtlety
       58423, // Relentless Strikes
        1943, // Rupture
       79147, // Sanguinary Vein
       51713, // Shadow Dance
       31220, // Sinister Calling
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
       81700, // Archangel
       81749, // Atonement
       52798, // Borrowed Time
      165376, // Critical Strike Attunement
       47515, // Divine Aegis
      122098, // Divine Fury
       81662, // Evangelism
       45243, // Focused Will
       47517, // Grace
        2060, // Heal
      158920, // Healer Level 90 Mana Fix
       14914, // Holy Fire
      132157, // Holy Nova
      137032, // Hotfix Passive
       77484, // Mastery: Shield Discipline
       95860, // Meditation
       33206, // Pain Suppression
       47540, // Penance
       62618, // Power Word: Barrier
         596, // Prayer of Healing
         527, // Purify
       47536, // Rapture
      145981, // Readiness: Discipline
       87336, // Spiritual Healing
      162701, // Stat Negation Aura - Intellect Healer
           0,
    },
    // Specialization abilities for Holy
    {
       32546, // Binding Heal
       81209, // Chakra: Chastise
       81206, // Chakra: Sanctuary
       81208, // Chakra: Serenity
       34861, // Circle of Healing
      122098, // Divine Fury
       64843, // Divine Hymn
      165362, // Divine Providence
       45243, // Focused Will
       47788, // Guardian Spirit
        2060, // Heal
      158920, // Healer Level 90 Mana Fix
       14914, // Holy Fire
       88625, // Holy Word: Chastise
      137031, // Hotfix Passive
      126135, // Lightwell
       77485, // Mastery: Echo of Light
       95861, // Meditation
         596, // Prayer of Healing
         527, // Purify
       95649, // Rapid Renewal
      145982, // Readiness: Holy
         139, // Renew
       63733, // Serendipity
       20711, // Spirit of Redemption
       87336, // Spiritual Healing
      162701, // Stat Negation Aura - Intellect Healer
           0,
    },
    // Specialization abilities for Shadow
    {
        2944, // Devouring Plague
       47585, // Dispersion
      165370, // Haste Attunement
      137033, // Hotfix Passive
      165201, // Mana Attunement
       77486, // Mastery: Mental Anguish
        8092, // Mind Blast
       15407, // Mind Flay
       49868, // Mind Quickening
       73510, // Mind Spike
       64044, // Psychic Horror
      151022, // Readiness: Shadow
       95740, // Shadow Orbs
      121196, // Shadow Passive
       32379, // Shadow Word: Death
       15473, // Shadowform
       78203, // Shadowy Apparitions
       15487, // Silence
      162699, // Stat Negation Aura - Intellect DPS
       15286, // Vampiric Embrace
       34914, // Vampiric Touch
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
      161608, // Bladed Armor
       50034, // Blood Rites
      163952, // Blood Rites
       49222, // Bone Shield
       81136, // Crimson Scourge
       49028, // Dancing Rune Weapon
       56222, // Dark Command
      137008, // Hotfix Passive
       50371, // Improved Blood Presence
       77513, // Mastery: Blood Shield
       86537, // Plate Specialization
      155522, // Power of the Grave
      145958, // Readiness: Blood
      158298, // Resolve
      161797, // Riposte
       48982, // Rune Tap
      165394, // Runic Strikes
       81127, // Sanguine Fortitude
       49509, // Scent of Blood
      114866, // Soul Reaper
      162702, // Stat Negation Aura - Strength Tank
       55233, // Vampiric Blood
       50029, // Veteran of the Third War
       81164, // Will of the Necropolis
           0,
    },
    // Specialization abilities for Frost
    {
       54637, // Blood of the North
         674, // Dual Wield
       49143, // Frost Strike
      137006, // Hotfix Passive
       49184, // Howling Blast
       50887, // Icy Talons
       50385, // Improved Frost Presence
       51128, // Killing Machine
       77514, // Mastery: Frozen Heart
       81333, // Might of the Frozen Wastes
       49020, // Obliterate
       51271, // Pillar of Frost
       86113, // Plate Specialization
      145959, // Readiness: Frost
       59057, // Rime
      130735, // Soul Reaper
      162698, // Stat Negation Aura - Strength DPS
       66192, // Threat of Thassarian
      165361, // Tundra Stalker
       55610, // Unholy Aura
           0,
    },
    // Specialization abilities for Unholy
    {
       63560, // Dark Transformation
       51160, // Ebon Plaguebringer
       85948, // Festering Strike
      137007, // Hotfix Passive
       50392, // Improved Unholy Presence
       77515, // Mastery: Dreadblade
      165395, // Necrosis
       86536, // Plate Specialization
       46584, // Raise Dead
      145960, // Readiness: Unholy
       56835, // Reaping
       55090, // Scourge Strike
       49572, // Shadow Infusion
      130736, // Soul Reaper
      162698, // Stat Negation Aura - Strength DPS
       49530, // Sudden Doom
       49206, // Summon Gargoyle
       55610, // Unholy Aura
       91107, // Unholy Might
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
      165339, // Ascendance
        8042, // Earth Shock
       61882, // Earthquake
       60188, // Elemental Fury
      165399, // Elemental Overload
       29000, // Elemental Reach
       88766, // Fulmination
      137040, // Hotfix Passive
       51505, // Lava Burst
       77756, // Lava Surge
       86108, // Mail Specialization
      170374, // Mastery: Molten Earth
      151023, // Readiness: Elemental
       62099, // Shamanism
       30823, // Shamanistic Rage
      123099, // Spiritual Insight
       79206, // Spiritwalker's Grace
      162699, // Stat Negation Aura - Intellect DPS
       51490, // Thunderstorm
      165462, // Unleash Flame
           0,
    },
    // Specialization abilities for Enhancement
    {
      165341, // Ascendance
      157444, // Critical Strikes
       86629, // Dual Wield
      166221, // Enhanced Weapons
       51533, // Feral Spirit
        1535, // Fire Nova
       10400, // Flametongue
       16282, // Flurry
      137041, // Hotfix Passive
       60103, // Lava Lash
      165368, // Lightning Strikes
       51530, // Maelstrom Weapon
        8190, // Magma Totem
       86099, // Mail Specialization
       77223, // Mastery: Enhanced Elements
       30814, // Mental Quickness
       51522, // Primal Wisdom
      145986, // Readiness: Enhancement
       30823, // Shamanistic Rage
       58875, // Spirit Walk
      162697, // Stat Negation Aura - Agility DPS
       17364, // Stormstrike
       73680, // Unleash Elements
       33757, // Windfury
           0,
    },
    // Specialization abilities for Restoration
    {
      165344, // Ascendance
        1064, // Chain Heal
         974, // Earth Shield
      158920, // Healer Level 90 Mana Fix
      108280, // Healing Tide Totem
       77472, // Healing Wave
      137039, // Hotfix Passive
       51505, // Lava Burst
       86100, // Mail Specialization
       77226, // Mastery: Deep Healing
       95862, // Meditation
      165391, // Purification
       16213, // Purification
       77130, // Purify Spirit
      145988, // Readiness: Restoration
       16196, // Resurgence
       61295, // Riptide
       98008, // Spirit Link Totem
      112858, // Spiritual Insight
       79206, // Spiritwalker's Grace
      162701, // Stat Negation Aura - Intellect Healer
       55453, // Telluric Currents
       51564, // Tidal Waves
       73685, // Unleash Life
       52127, // Water Shield
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
      114664, // Arcane Charge
        1449, // Arcane Explosion
      165359, // Arcane Mind
        5143, // Arcane Missiles
       79684, // Arcane Missiles
       12042, // Arcane Power
       12051, // Evocation
      137021, // Hotfix Passive
        6117, // Mage Armor
       76547, // Mastery: Mana Adept
       12043, // Presence of Mind
      151024, // Readiness: Arcane
       31589, // Slow
           0,
    },
    // Specialization abilities for Fire
    {
       11129, // Combustion
      117216, // Critical Mass
       31661, // Dragon's Breath
         133, // Fireball
        2120, // Flamestrike
      137019, // Hotfix Passive
      165357, // Incineration
      108853, // Inferno Blast
       12846, // Mastery: Ignite
       30482, // Molten Armor
       11366, // Pyroblast
       44448, // Pyroblast Clearcasting Driver
      151026, // Readiness: Fire
        2948, // Scorch
           0,
    },
    // Specialization abilities for Frost
    {
          10, // Blizzard
       44549, // Brain Freeze
       44572, // Deep Freeze
      112965, // Fingers of Frost
        7302, // Frost Armor
         116, // Frostbolt
       84714, // Frozen Orb
      137020, // Hotfix Passive
       30455, // Ice Lance
      165360, // Ice Shards
       12472, // Icy Veins
       76613, // Mastery: Icicles
      151027, // Readiness: Frost
       12982, // Shatter
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
         980, // Agony
      113860, // Dark Soul: Misery
      103103, // Drain Soul
      165367, // Eradication
       48181, // Haunt
         755, // Health Funnel
      137043, // Hotfix Passive
       77215, // Mastery: Potent Afflictions
      108558, // Nightfall
      151028, // Readiness: Affliction
       27243, // Seed of Corruption
      117198, // Soul Shards
       86121, // Soul Swap
       74434, // Soulburn
       74434, // Soulburn
       74434, // Soulburn
       30108, // Unstable Affliction
           0,
    },
    // Specialization abilities for Demonology
    {
      113861, // Dark Soul: Knowledge
      108869, // Decimation
      104315, // Demonic Fury
      109145, // Demonic Fury
      109151, // Demonic Leap
      165392, // Demonic Tactics
      105174, // Hand of Gul'dan
         755, // Health Funnel
        1949, // Hellfire
      137044, // Hotfix Passive
       77219, // Mastery: Master Demonologist
      103958, // Metamorphosis
      122351, // Molten Core
      151029, // Readiness: Demonology
        6353, // Soul Fire
       30146, // Summon Felguard
      114592, // Wild Imps
           0,
    },
    // Specialization abilities for Destruction
    {
      109784, // Aftermath
      117896, // Backdraft
      116852, // Burning Embers
      108647, // Burning Embers
      116858, // Chaos Bolt
      111546, // Chaotic Energy
       17962, // Conflagrate
      113858, // Dark Soul: Instability
      165363, // Devastation
      114635, // Ember Tap
      108683, // Fire and Brimstone
       80240, // Havoc
      137046, // Hotfix Passive
         348, // Immolate
       29722, // Incinerate
       77220, // Mastery: Emberstorm
        5740, // Rain of Fire
      151030, // Readiness: Destruction
       17877, // Shadowburn
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
      161608, // Bladed Armor
      115181, // Breath of Fire
      128938, // Brewing: Elusive Brew
      117967, // Brewmaster Training
      157445, // Critical Strikes
      126086, // Desperate Measures
      126060, // Desperate Measures
      115180, // Dizzying Haze
      124146, // Dual Wield
      115308, // Elusive Brew
      165379, // Ferment
      124502, // Gift of the Ox
      115295, // Guard
      137023, // Hotfix Passive
      121253, // Keg Smash
      120225, // Leather Specialization
      116781, // Legacy of the White Tiger
      117906, // Mastery: Elusive Brawler
      119582, // Purifying Brew
      145967, // Readiness: Brewmaster
      158298, // Resolve
      115069, // Stance of the Sturdy Ox
      115315, // Summon Black Ox Statue
      115176, // Zen Meditation
           0,
    },
    // Specialization abilities for Mistweaver
    {
      116346, // <DND> Soothing Mists Trigger
      123766, // Brewing: Mana Tea
      139598, // Crane Style Techniques
      115460, // Detonate Chi
      124682, // Enveloping Mist
      154555, // Focus and Harmony
      158920, // Healer Level 90 Mana Fix
      137024, // Hotfix Passive
      115451, // Internal Medicine
      165397, // Jade Mists
      120224, // Leather Specialization
      115921, // Legacy of the Emperor
      116849, // Life Cocoon
      121278, // Mana Meditation
      115294, // Mana Tea
      115869, // Mana Tea Driver <DND>
      117907, // Mastery: Gift of the Serpent
      145968, // Readiness: Mistweaver
      115151, // Renewing Mist
      115310, // Revival
      107428, // Rising Sun Kick
      115175, // Soothing Mist
      154436, // Stance of the Spirited Crane
      115070, // Stance of the Wise Serpent
      115313, // Summon Jade Serpent Statue
      116645, // Teachings of the Monastery
      116680, // Thunder Focus Tea
      116670, // Uplift
    },
    // Specialization abilities for Windwalker
    {
      116092, // Afterlife
      165398, // Battle Trance
      123980, // Brewing: Tigereye Brew
      128595, // Combat Conditioning
      137384, // Combo Breaker
      157445, // Critical Strikes
      116095, // Disable
      124146, // Dual Wield
      115288, // Energizing Brew
      113656, // Fists of Fury
      101545, // Flying Serpent Kick
      137025, // Hotfix Passive
      120227, // Leather Specialization
      116781, // Legacy of the White Tiger
      115636, // Mastery: Bottled Fury
      145969, // Readiness: Windwalker
      107428, // Rising Sun Kick
      137639, // Storm, Earth, and Fire
      116740, // Tigereye Brew
      122470, // Touch of Karma
      166916, // Windflurry
      115176, // Zen Meditation
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
      127663, // Astral Communion
       33605, // Astral Showers
      166163, // Balance
      106732, // Balance Overrides Passive
       22812, // Barkskin
      112071, // Celestial Alignment
       79577, // Eclipse
      137013, // Hotfix Passive
      108299, // Killer Instinct
       86104, // Leather Specialization
      165386, // Lunar Guidance
       77492, // Mastery: Total Eclipse
       24858, // Moonkin Form
      112857, // Natural Insight
      151021, // Readiness: Balance
        2782, // Remove Corruption
       93399, // Shooting Stars
       78675, // Solar Beam
       48505, // Starfall
        2912, // Starfire
       78674, // Starsurge
      162699, // Stat Negation Aura - Intellect DPS
       88747, // Wild Mushroom
           0,
    },
    // Specialization abilities for Feral
    {
      106952, // Berserk
      157447, // Critical Strikes
         770, // Faerie Fire
       16949, // Feral Instinct
      106733, // Feral Overrides Passive
      137011, // Hotfix Passive
       48484, // Infected Wounds
       17007, // Leader of the Pack
       86097, // Leather Specialization
       22570, // Maim
      166142, // Mana Attunement
       77493, // Mastery: Razor Claws
       33873, // Nurturing Instinct
       16864, // Omen of Clarity
       16974, // Predatory Swiftness
        1822, // Rake
      145961, // Readiness: Feral
        2782, // Remove Corruption
        1079, // Rip
       52610, // Savage Roar
      165372, // Sharpened Claws
      106839, // Skull Bash
      162697, // Stat Negation Aura - Agility DPS
       61336, // Survival Instincts
      106785, // Swipe
        5217, // Tiger's Fury
           0,
    },
    // Specialization abilities for Guardian
    {
       22812, // Barkskin
      106952, // Berserk
      161608, // Bladed Armor
      157447, // Critical Strikes
         770, // Faerie Fire
      106734, // Guardian Overrides Passive
      137010, // Hotfix Passive
       48484, // Infected Wounds
       33745, // Lacerate
       86096, // Leather Specialization
      166142, // Mana Attunement
      155783, // Mastery: Primal Tenacity
      159195, // Mastery: Primal Tenacity
        6807, // Maul
      106922, // Might of Ursoc
       33873, // Nurturing Instinct
      145962, // Readiness: Guardian
        2782, // Remove Corruption
      158298, // Resolve
       62606, // Savage Defense
      106839, // Skull Bash
      162700, // Stat Negation Aura - Agility Tank
       61336, // Survival Instincts
      165387, // Survival of the Fittest
       16931, // Thick Hide
      135288, // Tooth and Claw
      159232, // Ursa Major
           0,
    },
    // Specialization abilities for Restoration
    {
       22812, // Barkskin
      145518, // Genesis
      158920, // Healer Level 90 Mana Fix
      137012, // Hotfix Passive
      102342, // Ironbark
      108299, // Killer Instinct
       86093, // Leather Specialization
       33763, // Lifebloom
       48500, // Living Seed
       92364, // Malfurion's Gift
       77495, // Mastery: Harmony
       85101, // Meditation
      112857, // Natural Insight
      165374, // Naturalist
       17073, // Naturalist
       88423, // Nature's Cure
      132158, // Nature's Swiftness
      113043, // Omen of Clarity
      145963, // Readiness: Restoration
        8936, // Regrowth
      106735, // Restoration Overrides Passive
      162701, // Stat Negation Aura - Intellect Healer
       18562, // Swiftmend
         740, // Tranquility
       48438, // Wild Growth
      145205, // Wild Mushroom
           0,
    },
  },
};

#define CLASS_MASTERY_ABILITY_SIZE (1)

// Class mastery abilities, wow build 18546
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
      76838, // Mastery: Weapons Master
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
      76669, // Mastery: Illuminated Healing
    },
    // Masteries for Protection specialization
    {
      76671, // Mastery: Divine Bulwark
    },
    // Masteries for Retribution specialization
    {
      76672, // Mastery: Hand of Light
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
      76659, // Mastery: Sniper Training
    },
    // Masteries for Survival specialization
    {
      76658, // Mastery: Essence of the Viper
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
    // Masteries for Combat specialization
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
      77484, // Mastery: Shield Discipline
    },
    // Masteries for Holy specialization
    {
      77485, // Mastery: Echo of Light
    },
    // Masteries for Shadow specialization
    {
      77486, // Mastery: Mental Anguish
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
     170374, // Mastery: Molten Earth
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
      76547, // Mastery: Mana Adept
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
      77220, // Mastery: Emberstorm
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
     117907, // Mastery: Gift of the Serpent
    },
    // Masteries for Windwalker specialization
    {
     115636, // Mastery: Bottled Fury
    },
    {
          0,
    },
  },
  // Class mastery abilities for Druid
  {
    // Masteries for Balance specialization
    {
      77492, // Mastery: Total Eclipse
    },
    // Masteries for Feral specialization
    {
      77493, // Mastery: Razor Claws
    },
    // Masteries for Guardian specialization
    {
     155783, // Mastery: Primal Tenacity
    },
    // Masteries for Restoration specialization
    {
      77495, // Mastery: Harmony
    },
  },
};

#define RACE_ABILITY_SIZE (4)

#ifndef MAX_RACE
#define MAX_RACE (28)
#endif

// Racial abilities, wow build 18546
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
    // Racial abilities for Warrior class
    {
      21009, // Elusiveness
          0,
    },
    {     0, },
    // Racial abilities for Hunter class
    {
      21009, // Elusiveness
          0,
    },
    // Racial abilities for Rogue class
    {
      21009, // Elusiveness
          0,
    },
    // Racial abilities for Priest class
    {
      21009, // Elusiveness
          0,
    },
    // Racial abilities for Death Knight class
    {
      21009, // Elusiveness
          0,
    },
    {     0, },
    // Racial abilities for Mage class
    {
      21009, // Elusiveness
          0,
    },
    {     0, },
    // Racial abilities for Monk class
    {
      21009, // Elusiveness
          0,
    },
    // Racial abilities for Druid class
    {
      21009, // Elusiveness
          0,
    },
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
  },
};

#define GLYPH_ABILITIES_SIZE (40)

// Glyph spells for classes, wow build 18546
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
    // Major Glyphs (32 spells)
    {
       58377, // Glyph of Blitz
       58367, // Glyph of Bloodthirst
       94372, // Glyph of Bull Rush
      159701, // Glyph of Cleave
       89003, // Glyph of Colossus Smash
       63325, // Glyph of Death From Above
       58386, // Glyph of Die by the Sword
       58355, // Glyph of Enraged Speed
      159761, // Glyph of Flawless Defense
       58357, // Glyph of Gag Order
      159708, // Glyph of Heroic Leap
       58366, // Glyph of Hindering Strikes
       58097, // Glyph of Long Charge
      159738, // Glyph of Mocking Banner
       58368, // Glyph of Mortal Strike
      159740, // Glyph of Raging Blow
       58370, // Glyph of Raging Wind
      159754, // Glyph of Rallying Cry
       94374, // Glyph of Recklessness
       58356, // Glyph of Resonating Power
       58372, // Glyph of Rude Interruption
      159759, // Glyph of Shattering Throw
       58375, // Glyph of Shield Slam
       63329, // Glyph of Shield Wall
       63328, // Glyph of Spell Reflection
       58384, // Glyph of Sweeping Strikes
       58098, // Glyph of Unending Rage
      146965, // Glyph of Victorious Throw
       58382, // Glyph of Victory Rush
       63324, // Glyph of Whirlwind
      159703, // Glyph of the Drawn Sword
      146971, // Glyph of the Executor
           0,
    },
    // Minor Glyphs (14 spells)
    {
       58096, // Glyph of Bloodcurdling Shout
       58369, // Glyph of Bloody Healing
      115946, // Glyph of Burning Anger
      115943, // Glyph of Crow Feast
       58099, // Glyph of Gushing Wound
       63327, // Glyph of Intimidating Shout
       58104, // Glyph of Mighty Victory
       58095, // Glyph of Mystic Shout
       68164, // Glyph of Thunder Strike
      123779, // Glyph of the Blazing Trail
      146968, // Glyph of the Raging Whirlwind
      146969, // Glyph of the Subtle Defender
      146973, // Glyph of the Watchful Eye
      146974, // Glyph of the Weaponmaster
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Paladin
  {
    // Major Glyphs (36 spells)
    {
      159548, // Glyph of Ardent Defender
       54927, // Glyph of Avenging Wrath
       63218, // Glyph of Beacon of Light
       54943, // Glyph of Blessed Life
       54931, // Glyph of Burden of Guilt
       54928, // Glyph of Consecration
       56414, // Glyph of Dazing Shield
       56420, // Glyph of Denounce
      146955, // Glyph of Devotion Aura
       54924, // Glyph of Divine Protection
      146956, // Glyph of Divine Shield
       63220, // Glyph of Divine Storm
      159572, // Glyph of Divine Wrath
       54939, // Glyph of Divinity
       54922, // Glyph of Double Jeopardy
       54935, // Glyph of Final Wrath
       57955, // Glyph of Flash of Light
       54930, // Glyph of Focused Shield
      159579, // Glyph of Hand of Freedom
      146957, // Glyph of Hand of Sacrifice
       54938, // Glyph of Harsh Words
       63224, // Glyph of Holy Shock
       54923, // Glyph of Holy Wrath
       54937, // Glyph of Illumination
       56416, // Glyph of Immediate Truth
      159592, // Glyph of Judgment
       54940, // Glyph of Light of Dawn
      122028, // Glyph of Mass Exorcism
      162604, // Glyph of Merciful Wrath
       93466, // Glyph of Protector of the Innocent
       54926, // Glyph of Templar's Verdict
       54936, // Glyph of Word of Glory
       63222, // Glyph of the Alabaster Shield
      119477, // Glyph of the Battle Healer
      159557, // Glyph of the Consecrator
      159573, // Glyph of the Liberator
           0,
    },
    // Minor Glyphs (12 spells)
    {
      115934, // Glyph of Bladed Judgment
      125043, // Glyph of Contemplation
       57954, // Glyph of Fire From the Heavens
      115738, // Glyph of Focused Wrath
      146959, // Glyph of Pillar of Light
      115933, // Glyph of Righteous Retreat
       57947, // Glyph of Seal of Blood
       57979, // Glyph of Winged Vengeance
      146958, // Glyph of the Exorcist
      115931, // Glyph of the Falling Avenger
       89401, // Glyph of the Luminous Charger
       57958, // Glyph of the Mounted King
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Hunter
  {
    // Major Glyphs (25 spells)
    {
       20895, // Glyph of Animal Bond
      109263, // Glyph of Black Ice
      119449, // Glyph of Camouflage
      119447, // Glyph of Chimera Shot
      159467, // Glyph of Deep Frost
       56850, // Glyph of Deterrence
       56844, // Glyph of Disengage
      123632, // Glyph of Distracting Shot
      119410, // Glyph of Endless Wrath
      148475, // Glyph of Enduring Deceit
      119403, // Glyph of Explosive Trap
       56845, // Glyph of Freezing Trap
       56847, // Glyph of Ice Trap
      132106, // Glyph of Liberation
       63068, // Glyph of Master's Call
       19573, // Glyph of Mend Pet
       56833, // Glyph of Mending
       83495, // Glyph of Mirrored Blades
       53299, // Glyph of No Escape
       19560, // Glyph of Pathfinding
      159469, // Glyph of Quick Revival
      159470, // Glyph of Snake Trap
      119407, // Glyph of Solace
      119384, // Glyph of Tranquilizing Shot
      146657, // Glyph of the Lean Pack
           0,
    },
    // Minor Glyphs (11 spells)
    {
      125042, // Glyph of Aspect of the Beast
      119462, // Glyph of Aspect of the Cheetah
       57904, // Glyph of Aspect of the Pack
      122492, // Glyph of Aspects
      126746, // Glyph of Fetch
       57903, // Glyph of Fireworks
       57870, // Glyph of Lesser Proportion
      159459, // Glyph of Play Dead
       57866, // Glyph of Revive Pet
       57902, // Glyph of Stampede
      119464, // Glyph of Tame Beast
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Rogue
  {
    // Major Glyphs (25 spells)
    {
       56813, // Glyph of Ambush
       56818, // Glyph of Blade Flurry
       91299, // Glyph of Blind
       56801, // Glyph of Cheap Shot
       63269, // Glyph of Cloak of Shadows
       63254, // Glyph of Deadly Momentum
      159638, // Glyph of Disappearance
      159635, // Glyph of Elusiveness
      159634, // Glyph of Energy
      159636, // Glyph of Energy Flows
       56799, // Glyph of Evasion
       56804, // Glyph of Feint
       56812, // Glyph of Garrote
       56809, // Glyph of Gouge
      146631, // Glyph of Hemorrhaging Veins
       56805, // Glyph of Kick
      146625, // Glyph of Recovery
       56806, // Glyph of Recuperate
       56808, // Glyph of Shadow Walk
       56810, // Glyph of Shiv
       56819, // Glyph of Smoke Bomb
       56811, // Glyph of Sprint
       63253, // Glyph of Stealth
       89758, // Glyph of Vanish
       63249, // Glyph of Vendetta
           0,
    },
    // Minor Glyphs (13 spells)
    {
       58039, // Glyph of Blurred Speed
       56800, // Glyph of Decoy
      125044, // Glyph of Detection
       63268, // Glyph of Disguise
       58032, // Glyph of Distract
       56807, // Glyph of Hemorrhage
      146961, // Glyph of Improved Distraction
       63252, // Glyph of Killing Spree
       58027, // Glyph of Pick Lock
       58017, // Glyph of Pick Pocket
       58038, // Glyph of Poisons
       58033, // Glyph of Safe Fall
      146960, // Glyph of the Headhunter
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Priest
  {
    // Major Glyphs (40 spells)
    {
       63248, // Glyph of Binding Heal
       55675, // Glyph of Circle of Healing
       55673, // Glyph of Deep Wells
      159598, // Glyph of Delayed Coalescence
      119864, // Glyph of Dispel Magic
       63229, // Glyph of Dispersion
       55684, // Glyph of Fade
       55678, // Glyph of Fear Ward
      147778, // Glyph of Focused Mending
      159596, // Glyph of Free Action
      159599, // Glyph of Guardian Spirit
      119853, // Glyph of Holy Fire
      119850, // Glyph of Leap of Faith
      108939, // Glyph of Levitate
      126133, // Glyph of Lightwell
       55691, // Glyph of Mass Dispel
       87195, // Glyph of Mind Blast
      120585, // Glyph of Mind Flay
      162532, // Glyph of Mind Harvest
       33371, // Glyph of Mind Spike
      159608, // Glyph of Miraculous Dispelling
      119866, // Glyph of Penance
       55672, // Glyph of Power Word: Shield
       55685, // Glyph of Prayer of Mending
       55688, // Glyph of Psychic Horror
       55676, // Glyph of Psychic Scream
       55677, // Glyph of Purify
       33202, // Glyph of Reflective Shield
      119872, // Glyph of Renew
      159606, // Glyph of Restored Faith
       55690, // Glyph of Scourge Imprisonment
      159628, // Glyph of Shadow Magic
      120583, // Glyph of Shadow Word: Death
      159626, // Glyph of Silence
       55692, // Glyph of Smite
      119873, // Glyph of Spirit of Redemption
      120584, // Glyph of Vampiric Embrace
       89489, // Glyph of Weakened Soul
      159624, // Glyph of the Inquisitor
      159627, // Glyph of the Redeemer
    },
    // Minor Glyphs (13 spells)
    {
      145722, // Glyph of Angels
       58009, // Glyph of Borrowed Time
      126152, // Glyph of Confession
       58228, // Glyph of Dark Archangel
      126174, // Glyph of Holy Resurrection
      147072, // Glyph of Inspired Hymns
       57986, // Glyph of Shackle Undead
      107906, // Glyph of Shadow
       57985, // Glyph of Shadow Ravens
      126745, // Glyph of Shadowy Friends
      120581, // Glyph of the Heavens
      147776, // Glyph of the Sha
      126094, // Glyph of the Val'kyr
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Death Knight
  {
    // Major Glyphs (28 spells)
    {
      159415, // Glyph of Absorb Magic
       58623, // Glyph of Anti-Magic Shell
       58620, // Glyph of Chains of Ice
       63330, // Glyph of Dancing Rune Weapon
       63331, // Glyph of Dark Simulacrum
       63333, // Glyph of Death Coil
       62259, // Glyph of Death Grip
       58629, // Glyph of Death and Decay
      159421, // Glyph of Empowerment
       58671, // Glyph of Enduring Infection
       58673, // Glyph of Icebound Fortitude
      159418, // Glyph of Icy Runes
       58631, // Glyph of Icy Touch
       58686, // Glyph of Mind Freeze
      159422, // Glyph of Necrotic Strike
       59332, // Glyph of Outbreak
       58657, // Glyph of Pestilence
       58635, // Glyph of Pillar of Frost
      159427, // Glyph of Raise Ally
      146648, // Glyph of Regenerative Magic
      159428, // Glyph of Rune Tap
      159429, // Glyph of Runic Power
       58647, // Glyph of Shifting Presences
       58618, // Glyph of Strangulate
      146645, // Glyph of Swift Death
       59327, // Glyph of Unholy Command
       58676, // Glyph of Vampiric Blood
      159416, // Glyph of the Ice Reaper
           0,
    },
    // Minor Glyphs (11 spells)
    {
       58669, // Glyph of Army of the Dead
       59336, // Glyph of Corpse Explosion
       60200, // Glyph of Death Gate
       58677, // Glyph of Death's Embrace
       58642, // Glyph of Foul Menagerie
       58680, // Glyph of Horn of Winter
       59307, // Glyph of Path of Frost
       59309, // Glyph of Resilient Grip
       63335, // Glyph of Tranquil Grip
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
    // Major Glyphs (37 spells)
    {
       55442, // Glyph of Capacitor Totem
       55449, // Glyph of Chain Lightning
       55452, // Glyph of Chaining
       55445, // Glyph of Cleansing Waters
      159640, // Glyph of Ephemeral Spirits
      147781, // Glyph of Eternal Earth
       63271, // Glyph of Feral Spirit
       55455, // Glyph of Fire Elemental Totem
       55450, // Glyph of Fire Nova
       55447, // Glyph of Flame Shock
       55443, // Glyph of Frost Shock
      161654, // Glyph of Frostflame Weapon
       59289, // Glyph of Ghost Wolf
      159643, // Glyph of Grounding
       55441, // Glyph of Grounding Totem
       55456, // Glyph of Healing Stream Totem
       55440, // Glyph of Healing Wave
       63291, // Glyph of Hex
      159644, // Glyph of Lava Spread
      101052, // Glyph of Lightning Shield
       55439, // Glyph of Purge
      147762, // Glyph of Purging
      159647, // Glyph of Reactive Shielding
       63273, // Glyph of Riptide
       63280, // Glyph of Shamanistic Rage
      159648, // Glyph of Shamanistic Resolve
      159649, // Glyph of Shocks
       55454, // Glyph of Spirit Walk
      159651, // Glyph of Spiritwalker's Aegis
      159650, // Glyph of Spiritwalker's Focus
       55446, // Glyph of Spiritwalker's Grace
       63270, // Glyph of Thunder
       55438, // Glyph of Totemic Recall
       63298, // Glyph of Totemic Vigor
       55437, // Glyph of Unstable Earth
       55436, // Glyph of Water Shield
       55451, // Glyph of Wind Shear
           0,
    },
    // Minor Glyphs (17 spells)
    {
      147787, // Glyph of Astral Fixation
       58058, // Glyph of Astral Recall
       63279, // Glyph of Deluge
      147788, // Glyph of Elemental Familiars
       58059, // Glyph of Far Sight
      147772, // Glyph of Flaming Serpents
      159642, // Glyph of Ghostly Speed
       55444, // Glyph of Lava Lash
      147784, // Glyph of Lingering Ancestors
      147707, // Glyph of Rain of Frogs
      147783, // Glyph of Spirit Raptors
      147770, // Glyph of Spirit Wolf
       62132, // Glyph of Thunderstorm
       58057, // Glyph of Totemic Encirclement
      147785, // Glyph of the Compy
       55448, // Glyph of the Lakestrider
       58135, // Glyph of the Spectral Wolf
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Mage
  {
    // Major Glyphs (22 spells)
    {
      115718, // Glyph of Arcane Explosion
       62210, // Glyph of Arcane Power
       56365, // Glyph of Blink
       56368, // Glyph of Combustion
      115705, // Glyph of Cone of Cold
      115703, // Glyph of Counterspell
      115710, // Glyph of Deep Freeze
      159485, // Glyph of Dragon's Breath
       56376, // Glyph of Frost Nova
       61205, // Glyph of Frostfire Bolt
      115723, // Glyph of Ice Block
       56364, // Glyph of Icy Veins
       89926, // Glyph of Inferno Blast
      159484, // Glyph of Interruption
       56375, // Glyph of Polymorph
      146659, // Glyph of Rapid Displacement
      159486, // Glyph of Regenerative Ice
      115700, // Glyph of Remove Curse
       86209, // Glyph of Slow
      115713, // Glyph of Spellsteal
       56377, // Glyph of Splitting Ice
       63090, // Glyph of Water Elemental
           0,
    },
    // Minor Glyphs (13 spells)
    {
       57925, // Glyph of Arcane Language
      147353, // Glyph of Condensation
      126748, // Glyph of Conjure Familiar
       56382, // Glyph of Crittermorph
      146662, // Glyph of Evaporation
       63092, // Glyph of Illusion
       56384, // Glyph of Momentum
       89749, // Glyph of Rapid Teleportation
       58136, // Glyph of the Bear Cub
       57927, // Glyph of the Monkey
       52648, // Glyph of the Penguin
       57924, // Glyph of the Porcupine
      146976, // Glyph of the Unbound Elemental
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Warlock
  {
    // Major Glyphs (25 spells)
    {
       56235, // Glyph of Conflagrate
       58080, // Glyph of Curse of Exhaustion
      159653, // Glyph of Curses
      159665, // Glyph of Dark Soul
       56249, // Glyph of Demon Training
       63309, // Glyph of Demonic Circle
       63302, // Glyph of Drain Life
       63304, // Glyph of Ember Tap
      148683, // Glyph of Eternal Resolve
       56244, // Glyph of Fear
      159667, // Glyph of Flames of Xoroth
      146962, // Glyph of Havoc
       56224, // Glyph of Healthstone
       56242, // Glyph of Imp Swarm
      159669, // Glyph of Life Pact
       63320, // Glyph of Life Tap
      159688, // Glyph of Shadowflame
       56218, // Glyph of Siphon Life
       58070, // Glyph of Soul Consumption
      159691, // Glyph of Soul Swap
       56231, // Glyph of Soulstone
      159697, // Glyph of Strengthened Resolve
      159693, // Glyph of Twilight Ward
      146964, // Glyph of Unending Resolve
       56233, // Glyph of Unstable Affliction
           0,
    },
    // Minor Glyphs (15 spells)
    {
       63312, // Glyph of Crimson Banish
       58107, // Glyph of Enslave Demon
       58081, // Glyph of Eye of Kilrogg
       56247, // Glyph of Falling Meteor
       56246, // Glyph of Felguard
      135557, // Glyph of Gateway Attunement
       56248, // Glyph of Hand of Gul'dan
       56238, // Glyph of Health Funnel
      159680, // Glyph of Metamorphosis
       56232, // Glyph of Nightmares
       56240, // Glyph of Shadow Bolt
       58094, // Glyph of Soulwell
       56217, // Glyph of Subtlety
       58079, // Glyph of Unending Breath
       56241, // Glyph of Verdant Spheres
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Monk
  {
    // Major Glyphs (31 spells)
    {
      123394, // Glyph of Breath of Fire
      146954, // Glyph of Detox
      120482, // Glyph of Enduring Healing Sphere
      159487, // Glyph of Expel Harm
      125671, // Glyph of Fists of Fury
      124997, // Glyph of Fortifying Brew
      146953, // Glyph of Fortuitous Spheres
      159534, // Glyph of Freedom Roll
      123401, // Glyph of Guard
      159495, // Glyph of Keg Smash
      125967, // Glyph of Leer of the Ox
      124989, // Glyph of Life Cocoon
      123763, // Glyph of Mana Tea
      146952, // Glyph of Nimble Brew
      125755, // Glyph of Paralysis
      146951, // Glyph of Rapid Rolling
      159496, // Glyph of Renewed Tea
      123334, // Glyph of Renewing Mist
      159536, // Glyph of Soothing Mist
      125673, // Glyph of Sparring
      120479, // Glyph of Spinning Crane Kick
      120483, // Glyph of Surging Mist
      146950, // Glyph of Targeted Expulsion
      123391, // Glyph of Touch of Death
      125678, // Glyph of Touch of Karma
      123023, // Glyph of Transcendence
      159497, // Glyph of Victory Roll
      159545, // Glyph of Zen Focus
      120477, // Glyph of Zen Meditation
      159490, // Glyph of the Floating Butterfly
      159492, // Glyph of the Flying Serpent
           0,
    },
    // Minor Glyphs (10 spells)
    {
      132005, // Glyph of Blackout Kick
      125931, // Glyph of Crackling Tiger Lightning
      125872, // Glyph of Fighting Pose
      123403, // Glyph of Flying Serpent Kick
      125732, // Glyph of Honor
      125660, // Glyph of Jab
      125151, // Glyph of Rising Tiger Kick
      125154, // Glyph of Spirit Roll
      125901, // Glyph of Water Roll
      125893, // Glyph of Zen Flight
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Druid
  {
    // Major Glyphs (36 spells)
    {
      159432, // Glyph of Astral Communion
       63057, // Glyph of Barkskin
      121840, // Glyph of Blooming
       47180, // Glyph of Cat Form
      159445, // Glyph of Celestial Alignment
       48514, // Glyph of Cyclone
       59219, // Glyph of Dash
      159436, // Glyph of Enchanted Bark
       54812, // Glyph of Entangling Energy
       54760, // Glyph of Entangling Roots
      114237, // Glyph of Fae Silence
       94386, // Glyph of Faerie Fire
       67598, // Glyph of Ferocious Bite
      146655, // Glyph of Guided Stars
       54825, // Glyph of Healing Touch
       54831, // Glyph of Hurricane
      159434, // Glyph of Imbued Bark
      159450, // Glyph of Maim
       54811, // Glyph of Maul
      116238, // Glyph of Might of Ursoc
      159455, // Glyph of Moonwarding
      116203, // Glyph of Nature's Grasp
       54821, // Glyph of Rake
       54733, // Glyph of Rebirth
      116218, // Glyph of Regrowth
       17076, // Glyph of Rejuvenation
      127540, // Glyph of Savage Roar
      116216, // Glyph of Skull Bash
      114222, // Glyph of Stampeding Roar
      159448, // Glyph of Sudden Eclipse
      114223, // Glyph of Survival Instincts
      159441, // Glyph of Ursol's Defense
       62970, // Glyph of Wild Growth
      116172, // Glyph of the Master Shapeshifter
      159444, // Glyph of the Ninth Life
      159453, // Glyph of the Shapemender
           0,
    },
    // Minor Glyphs (13 spells)
    {
       57856, // Glyph of Aquatic Form
       57855, // Glyph of Charm Woodland Creature
      114295, // Glyph of Grace
      146656, // Glyph of One With Nature
      114301, // Glyph of Stars
      159456, // Glyph of Travel
      107059, // Glyph of the Chameleon
      131113, // Glyph of the Cheetah
      114333, // Glyph of the Orca
      114280, // Glyph of the Predator
      146654, // Glyph of the Sprouting Mushroom
      114338, // Glyph of the Stag
      125047, // Glyph of the Treant
           0,
    },
    {
           0,
    },
  },
};

#define TIER_BONUSES_SIZE (8)
#define TIER_BONUSES_MAX_TIER (5)

#define TIER_BONUSES_TIER_BASE (13)

// Tier item set bonuses for class, wow build 18546
static unsigned __tier_bonuses_data[][TIER_BONUSES_MAX_TIER][TIER_BONUSES_SIZE] = {
  {
    {
          0,
    },
  },
  // Tier bonuses for Warrior
  {
    // Tier 13 bonuses (4 spells)
    {
      105797, // Colossal Dragonplate Battlegear - 2 Piece Bonus (Item - Warrior T13 Arms and Fury 2P Bonus (Inner Rage))
      105907, // Colossal Dragonplate Battlegear - 4 Piece Bonus (Item - Warrior T13 Arms and Fury 4P Bonus (Colossus Smash))
      105908, // Colossal Dragonplate Armor - 2 Piece Bonus (Item - Warrior T13 Protection 2P Bonus (Revenge))
      105911, // Colossal Dragonplate Armor - 4 Piece Bonus (Item - Warrior T13 Protection 4P Bonus (Shield Wall))
          0,
    },
    // Tier 14 bonuses (4 spells)
    {
      123142, // Battleplate of Resounding Rings - 2 Piece Bonus (Item - Warrior T14 DPS 2P Bonus)
      123144, // Battleplate of Resounding Rings - 4 Piece Bonus (Item - Warrior T14 DPS 4P Bonus)
      123146, // Plate of Resounding Rings - 2 Piece Bonus (Item - Warrior T14 Protection 2P Bonus)
      123147, // Plate of Resounding Rings - 4 Piece Bonus (Item - Warrior T14 Protection 4P Bonus)
          0,
    },
    // Tier 15 bonuses (4 spells)
    {
      138120, // Battleplate of the Last Mogu - 2 Piece Bonus (Item - Warrior T15 DPS 2P Bonus)
      138126, // Battleplate of the Last Mogu - 4 Piece Bonus (Item - Warrior T15 DPS 4P Bonus)
      138280, // Plate of the Last Mogu - 2 Piece Bonus (Item - Warrior T15 Protection 2P Bonus)
      138281, // Plate of the Last Mogu - 4 Piece Bonus (Item - Warrior T15 Protection 4P Bonus)
          0,
    },
    // Tier 16 bonuses (4 spells)
    {
      144436, // Battleplate of the Prehistoric Marauder - 2 Piece Bonus (Item - Warrior T16 DPS 2P Bonus)
      144441, // Battleplate of the Prehistoric Marauder - 4 Piece Bonus (Item - Warrior T16 DPS 4P Bonus)
      144502, // Plate of the Prehistoric Marauder - 4 Piece Bonus (Item - Warrior T16 Protection 4P Bonus)
      144503, // Plate of the Prehistoric Marauder - 2 Piece Bonus (Item - Warrior T16 Protection 2P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Paladin
  {
    // Tier 13 bonuses (6 spells)
    {
      105743, // Regalia of Radiant Glory - 2 Piece Bonus (Item - Paladin T13 Holy 2P Bonus (Divine Favor))
      105744, // Armor of Radiant Glory - 4 Piece Bonus (Item - Paladin T13 Protection 4P Bonus (Divine Guardian))
      105765, // Battleplate of Radiant Glory - 2 Piece Bonus (Item - Paladin T13 Retribution 2P Bonus (Judgment))
      105798, // Regalia of Radiant Glory - 4 Piece Bonus (Item - Paladin T13 Holy 4P Bonus (Holy Radiance))
      105800, // Armor of Radiant Glory - 2 Piece Bonus (Item - Paladin T13 Protection 2P Bonus (Judgment))
      105820, // Battleplate of Radiant Glory - 4 Piece Bonus (Item - Paladin T13 Retribution 4P Bonus (Zealotry))
          0,
    },
    // Tier 14 bonuses (6 spells)
    {
      70762, // White Tiger Battlegear - 4 Piece Bonus (Item - Paladin T10 Retribution 4P Bonus)
      123102, // White Tiger Vestments - 2 Piece Bonus (Item - Paladin T14 Holy 2P Bonus)
      123103, // White Tiger Vestments - 4 Piece Bonus (Item - Paladin T14 Holy 4P Bonus)
      123104, // White Tiger Plate - 2 Piece Bonus (Item - Paladin T14 Protection 2P Bonus)
      123107, // White Tiger Plate - 4 Piece Bonus (Item - Paladin T14 Protection 4P Bonus)
      123108, // White Tiger Battlegear - 2 Piece Bonus (Item - Paladin T14 Retribution 2P Bonus)
          0,
    },
    // Tier 15 bonuses (6 spells)
    {
      138159, // Battlegear of the Lightning Emperor - 2 Piece Bonus (Item - Paladin T15 Retribution 2P Bonus)
      138164, // Battlegear of the Lightning Emperor - 4 Piece Bonus (Item - Paladin T15 Retribution 4P Bonus)
      138238, // Plate of the Lightning Emperor - 2 Piece Bonus (Item - Paladin T15 Protection 2P Bonus)
      138244, // Plate of the Lightning Emperor - 4 Piece Bonus (Item - Paladin T15 Protection 4P Bonus)
      138291, // Vestments of the Lightning Emperor - 2 Piece Bonus (Item - Paladin T15 Holy 2P Bonus)
      138292, // Vestments of the Lightning Emperor - 4 Piece Bonus (Item - Paladin T15 Holy 4P Bonus)
          0,
    },
    // Tier 16 bonuses (6 spells)
    {
      144566, // Plate of Winged Triumph - 4 Piece Bonus (Item - Paladin T16 Protection 4P Bonus)
      144580, // Plate of Winged Triumph - 2 Piece Bonus (Item - Paladin T16 Protection 2P Bonus)
      144586, // Battlegear of Winged Triumph - 2 Piece Bonus (Item - Paladin T16 Retribution 2P Bonus)
      144593, // Battlegear of Winged Triumph - 4 Piece Bonus (Item - Paladin T16 Retribution 4P Bonus)
      144613, // Vestments of Winged Triumph - 4 Piece Bonus (Item - Paladin T16 Holy 4P Bonus)
      144625, // Vestments of Winged Triumph - 2 Piece Bonus (Item - Paladin T16 Holy 2P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Hunter
  {
    // Tier 13 bonuses (2 spells)
    {
      105732, // Wyrmstalker Battlegear - 2 Piece Bonus (Item - Hunter T13 2P Bonus (Steady Shot and Cobra Shot))
      105921, // Wyrmstalker Battlegear - 4 Piece Bonus (Item - Hunter T13 4P Bonus (Arcane Shot))
          0,
    },
    // Tier 14 bonuses (2 spells)
    {
      123090, // Yaungol Slayer Battlegear - 2 Piece Bonus (Item - Hunter T14 2P Bonus)
      123091, // Yaungol Slayer Battlegear - 4 Piece Bonus (Item - Hunter T14 4P Bonus)
          0,
    },
    // Tier 15 bonuses (2 spells)
    {
      138365, // Battlegear of the Saurok Stalker - 2 Piece Bonus (Item - Hunter T15 2P Bonus)
      138367, // Battlegear of the Saurok Stalker - 4 Piece Bonus (Item - Hunter T15 4P Bonus)
          0,
    },
    // Tier 16 bonuses (2 spells)
    {
      144637, // Battlegear of the Unblinking Vigil - 2 Piece Bonus (Item - Hunter T16 2P Bonus)
      144641, // Battlegear of the Unblinking Vigil - 4 Piece Bonus (Item - Hunter T16 4P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Rogue
  {
    // Tier 13 bonuses (2 spells)
    {
      105849, // Blackfang Battleweave - 2 Piece Bonus (Item - Rogue T13 2P Bonus (Tricks of the Trade))
      105865, // Blackfang Battleweave - 4 Piece Bonus (Item - Rogue T13 4P Bonus (Shadow Dance, Adrenaline Rush, and Vendetta))
          0,
    },
    // Tier 14 bonuses (2 spells)
    {
      123116, // Battlegear of the Thousandfold Blades - 2 Piece Bonus (Item - Rogue T14 2P Bonus)
      123122, // Battlegear of the Thousandfold Blades - 4 Piece Bonus (Item - Rogue T14 4P Bonus)
          0,
    },
    // Tier 15 bonuses (2 spells)
    {
      138148, // Nine-Tail Battlegear - 2 Piece Bonus (Item - Rogue T15 2P Bonus)
      138150, // Nine-Tail Battlegear - 4 Piece Bonus (Item - Rogue T15 4P Bonus)
          0,
    },
    // Tier 16 bonuses (2 spells)
    {
      145185, // Barbed Assassin Battlegear - 2 Piece Bonus (Item - Rogue T16 2P Bonus)
      145210, // Barbed Assassin Battlegear - 4 Piece Bonus (Item - Rogue T16 4P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Priest
  {
    // Tier 13 bonuses (4 spells)
    {
      105827, // Vestments of Dying Light - 2 Piece Bonus (Item - Priest T13 Healer 2P Bonus (Power Infusion and Lightwell))
      105832, // Vestments of Dying Light - 4 Piece Bonus (Item - Priest T13 Healer 4P Bonus (Holy Word and Power Word: Shield))
      105843, // Regalia of Dying Light - 2 Piece Bonus (Item - Priest T13 Shadow 2P Bonus (Shadow Word: Death))
      105844, // Regalia of Dying Light - 4 Piece Bonus (Item - Priest T13 Shadow 4P Bonus (Shadow Word: Pain))
          0,
    },
    // Tier 14 bonuses (4 spells)
    {
      123111, // Vestments of the Guardian Serpent - 2 Piece Bonus (Item - Priest T14 Healer 2P Bonus)
      123113, // Vestments of the Guardian Serpent - 4 Piece Bonus (Item - Priest T14 Healer 4P Bonus)
      123114, // Regalia of the Guardian Serpent - 2 Piece Bonus (Item - Priest T14 Shadow 2P Bonus)
      123115, // Regalia of the Guardian Serpent - 4 Piece Bonus (Item - Priest T14 Shadow 4P Bonus)
          0,
    },
    // Tier 15 bonuses (4 spells)
    {
      138156, // Regalia of the Exorcist - 2 Piece Bonus (Item - Priest T15 Shadow 2P Bonus)
      138158, // Regalia of the Exorcist - 4 Piece Bonus (Item - Priest T15 Shadow 4P Bonus)
      138293, // Vestments of the Exorcist - 2 Piece Bonus (Item - Priest T15 Healer 2P Bonus)
      138301, // Vestments of the Exorcist - 4 Piece Bonus (Item - Priest T15 Healer 4P Bonus)
          0,
    },
    // Tier 16 bonuses (4 spells)
    {
      145174, // Regalia of Ternion Glory - 2 Piece Bonus (Item - Priest T16 Shadow 2P Bonus)
      145179, // Regalia of Ternion Glory - 4 Piece Bonus (Item - Priest T16 Shadow 4P Bonus)
      145306, // Vestments of Ternion Glory - 2 Piece Bonus (Item - Priest T16 Healer 2P Bonus)
      145334, // Vestments of Ternion Glory - 4 Piece Bonus (Item - Priest T16 Healer 4P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Death Knight
  {
    // Tier 13 bonuses (4 spells)
    {
      105552, // Necrotic Boneplate Armor - 2 Piece Bonus (Item - Death Knight T13 Blood 2P Bonus)
      105587, // Necrotic Boneplate Armor - 4 Piece Bonus (Item - Death Knight T13 Blood 4P Bonus)
      105609, // Necrotic Boneplate Battlegear - 2 Piece Bonus (Item - Death Knight T13 DPS 2P Bonus)
      105646, // Necrotic Boneplate Battlegear - 4 Piece Bonus (Item - Death Knight T13 DPS 4P Bonus)
          0,
    },
    // Tier 14 bonuses (4 spells)
    {
      123077, // Battlegear of the Lost Catacomb - 2 Piece Bonus (Item - Death Knight T14 DPS 2P Bonus)
      123078, // Battlegear of the Lost Catacomb - 4 Piece Bonus (Item - Death Knight T14 DPS 4P Bonus)
      123079, // Plate of the Lost Catacomb - 2 Piece Bonus (Item - Death Knight T14 Blood 2P Bonus)
      123080, // Plate of the Lost Catacomb - 4 Piece Bonus (Item - Death Knight T14 Blood 4P Bonus)
          0,
    },
    // Tier 15 bonuses (4 spells)
    {
      138195, // Plate of the All-Consuming Maw - 2 Piece Bonus (Item - Death Knight T15 Blood 2P Bonus)
      138197, // Plate of the All-Consuming Maw - 4 Piece Bonus (Item - Death Knight T15 Blood 4P Bonus)
      138343, // Battleplate of the All-Consuming Maw - 2 Piece Bonus (Item - Death Knight T15 DPS 2P Bonus)
      138347, // Battleplate of the All-Consuming Maw - 4 Piece Bonus (Item - Death Knight T15 DPS 4P Bonus)
          0,
    },
    // Tier 16 bonuses (4 spells)
    {
      144899, // Battleplate of Cyclopean Dread - 2 Piece Bonus (Item - Death Knight T16 DPS 2P Bonus)
      144907, // Battleplate of Cyclopean Dread - 4 Piece Bonus (Item - Death Knight T16 DPS 4P Bonus)
      144934, // Plate of Cyclopean Dread - 2 Piece Bonus (Item - Death Knight T16 Blood 2P Bonus)
      144950, // Plate of Cyclopean Dread - 4 Piece Bonus (Item - Death Knight T16 Blood 4P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Shaman
  {
    // Tier 13 bonuses (6 spells)
    {
      105764, // Spiritwalker's Vestments - 2 Piece Bonus (Item - Shaman T13 Restoration 2P Bonus (Mana Tide))
      105780, // Spiritwalker's Regalia - 2 Piece Bonus (Item - Shaman T13 Elemental 2P Bonus (Elemental Mastery))
      105816, // Spiritwalker's Regalia - 4 Piece Bonus (Item - Shaman T13 Elemental 4P Bonus (Elemental Overload))
      105866, // Spiritwalker's Battlegear - 2 Piece Bonus (Item - Shaman T13 Enhancement 2P Bonus (Maelstrom Weapon))
      105872, // Spiritwalker's Battlegear - 4 Piece Bonus (Item - Shaman T13 Enhancement 4P Bonus (Feral Spirits))
      105876, // Spiritwalker's Vestments - 4 Piece Bonus (Item - Shaman T13 Restoration 4P Bonus (Spiritwalker's Grace))
          0,
    },
    // Tier 14 bonuses (6 spells)
    {
      123123, // Regalia of the Firebird - 2 Piece Bonus (Item - Shaman T14 Elemental 2P Bonus)
      123124, // Regalia of the Firebird - 4 Piece Bonus (Item - Shaman T14 Elemental 4P Bonus)
      123132, // Battlegear of the Firebird - 2 Piece Bonus (Item - Shaman T14 Enhancement 2P Bonus)
      123133, // Battlegear of the Firebird - 4 Piece Bonus (Item - Shaman T14 Enhancement 4P Bonus)
      123134, // Vestments of the Firebird - 2 Piece Bonus (Item - Shaman T14 Restoration 2P Bonus)
      123135, // Vestments of the Firebird - 4 Piece Bonus (Item - Shaman T14 Restoration 4P Bonus)
          0,
    },
    // Tier 15 bonuses (6 spells)
    {
      138136, // Battlegear of the Witch Doctor - 2 Piece Bonus (Item - Shaman T15 Enhancement 2P Bonus)
      138141, // Battlegear of the Witch Doctor - 4 Piece Bonus (Item - Shaman T15 Enhancement 4P Bonus)
      138144, // Regalia of the Witch Doctor - 4 Piece Bonus (Item - Shaman T15 Elemental 4P Bonus)
      138145, // Regalia of the Witch Doctor - 2 Piece Bonus (Item - Shaman T15 Elemental 2P Bonus)
      138303, // Vestments of the Witch Doctor - 2 Piece Bonus (Item - Shaman T15 Restoration 2P Bonus)
      138305, // Vestments of the Witch Doctor - 4 Piece Bonus (Item - Shaman T15 Restoration 4P Bonus)
          0,
    },
    // Tier 16 bonuses (6 spells)
    {
      144962, // Celestial Harmony Battlegear - 2 Piece Bonus (Item - Shaman T16 Enhancement 2P Bonus)
      144966, // Celestial Harmony Battlegear - 4 Piece Bonus (Item - Shaman T16 Enhancement 4P Bonus)
      144998, // Celestial Harmony Regalia - 2 Piece Bonus (Item - Shaman T16 Elemental 2P Bonus)
      145003, // Celestial Harmony Regalia - 4 Piece Bonus (Item - Shaman T16 Elemental 4P Bonus)
      145378, // Celestial Harmony Vestment - 2 Piece Bonus (Item - Shaman T16 Restoration 2P Bonus)
      145380, // Celestial Harmony Vestment - 4 Piece Bonus (Item - Shaman T16 Restoration 4P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Mage
  {
    // Tier 13 bonuses (2 spells)
    {
      105788, // Time Lord's Regalia - 2 Piece Bonus (Item - Mage T13 2P Bonus (Haste))
      105790, // Time Lord's Regalia - 4 Piece Bonus (Item - Mage T13 4P Bonus (Arcane Power, Combustion, and Icy Veins))
          0,
    },
    // Tier 14 bonuses (2 spells)
    {
      123097, // Regalia of the Burning Scroll - 2 Piece Bonus (Item - Mage T14 2P Bonus)
      123101, // Regalia of the Burning Scroll - 4 Piece Bonus (Item - Mage T14 4P Bonus)
          0,
    },
    // Tier 15 bonuses (2 spells)
    {
      138316, // Regalia of the Chromatic Hydra - 2 Piece Bonus (Item - Mage T15 2P Bonus)
      138376, // Regalia of the Chromatic Hydra - 4 Piece Bonus (Item - Mage T15 4P Bonus)
          0,
    },
    // Tier 16 bonuses (2 spells)
    {
      145251, // Chronomancer Regalia - 2 Piece Bonus (Item - Mage T16 2P Bonus)
      145257, // Chronomancer Regalia - 4 Piece Bonus (Item - Mage T16 4P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Warlock
  {
    // Tier 13 bonuses (2 spells)
    {
      105787, // Vestments of the Faceless Shroud - 4 Piece Bonus (Item - Warlock T13 4P Bonus (Soulburn))
      105888, // Vestments of the Faceless Shroud - 2 Piece Bonus (Item - Warlock T13 2P Bonus (Doomguard and Infernal))
          0,
    },
    // Tier 14 bonuses (2 spells)
    {
      123136, // Sha-Skin Regalia - 2 Piece Bonus (Item - Warlock T14 2P Bonus)
      123141, // Sha-Skin Regalia - 4 Piece Bonus (Item - Warlock T14 4P Bonus)
          0,
    },
    // Tier 15 bonuses (2 spells)
    {
      138129, // Regalia of the Thousandfold Hells - 2 Piece Bonus (Item - Warlock T15 DPS 2P Bonus)
      138134, // Regalia of the Thousandfold Hells - 4 Piece Bonus (Item - Warlock T15 DPS 4P Bonus)
          0,
    },
    // Tier 16 bonuses (2 spells)
    {
      145072, // Regalia of the Horned Nightmare - 2 Piece Bonus (Item - Warlock T16 2P Bonus)
      145091, // Regalia of the Horned Nightmare - 4 Piece Bonus (Item - Warlock T16 4P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Monk
  {
    {
          0,
    },
    // Tier 14 bonuses (6 spells)
    {
      123149, // Battlegear of the Red Crane - 2 Piece Bonus (Item - Monk T14 Windwalker 2P Bonus)
      123150, // Battlegear of the Red Crane - 4 Piece Bonus (Item - Monk T14 Windwalker 4P Bonus)
      123152, // Vestments of the Red Crane - 2 Piece Bonus (Item - Monk T14 Mistweaver 2P Bonus)
      123153, // Vestments of the Red Crane - 4 Piece Bonus (Item - Monk T14 Mistweaver 4P Bonus)
      123157, // Armor of the Red Crane - 2 Piece Bonus (Item - Monk T14 Brewmaster 2P Bonus)
      123159, // Armor of the Red Crane - 4 Piece Bonus (Item - Monk T14 Brewmaster 4P Bonus)
          0,
    },
    // Tier 15 bonuses (6 spells)
    {
      138177, // Fire-Charm Battlegear - 2 Piece Bonus (Item - Monk T15 Windwalker 2P Bonus)
      138231, // Fire-Charm Armor - 2 Piece Bonus (Item - Monk T15 Brewmaster 2P Bonus)
      138236, // Fire-Charm Armor - 4 Piece Bonus (Item - Monk T15 Brewmaster 4P Bonus)
      138289, // Fire-Charm Vestments - 4 Piece Bonus (Item - Monk T15 Mistweaver 4P Bonus)
      138290, // Fire-Charm Vestments - 2 Piece Bonus (Item - Monk T15 Mistweaver 2P Bonus)
      138315, // Fire-Charm Battlegear - 4 Piece Bonus (Item - Monk T15 Windwalker 4P Bonus)
          0,
    },
    // Tier 16 bonuses (6 spells)
    {
      145004, // Battlegear of Seven Sacred Seals - 2 Piece Bonus (Item - Monk T16 Windwalker 2P Bonus)
      145022, // Battlegear of Seven Sacred Seals - 4 Piece Bonus (Item - Monk T16 Windwalker 4P Bonus)
      145049, // Armor of Seven Sacred Seals - 2 Piece Bonus (Item - Monk T16 Brewmaster 2P Bonus)
      145055, // Armor of Seven Sacred Seals - 4 Piece Bonus (Item - Monk T16 Brewmaster 4P Bonus)
      145439, // Vestments of Seven Sacred Seals - 2 Piece Bonus (Item - Monk T16 Mistweaver 2P Bonus)
      145449, // Vestments of Seven Sacred Seals - 4 Piece Bonus (Item - Monk T16 Mistweaver 4P Bonus)
          0,
    },
    {
          0,
    },
  },
  // Tier bonuses for Druid
  {
    // Tier 13 bonuses (6 spells)
    {
      105715, // Deep Earth Vestments - 2 Piece Bonus (Item - Druid T13 Restoration 2P Bonus (Innervate))
      105717, // Deep Earth Regalia - 4 Piece Bonus (Item - Druid T13 Balance 4P Bonus (Starsurge))
      105722, // Deep Earth Regalia - 2 Piece Bonus (Item - Druid T13 Balance 2P Bonus (Insect Swarm))
      105725, // Deep Earth Battlegarb - 2 Piece Bonus (Item - Druid T13 Feral 2P Bonus (Savage Defense and Blood In The Water))
      105735, // Deep Earth Battlegarb - 4 Piece Bonus (Item - Druid T13 Feral 4P Bonus (Might of Ursoc and Stampede))
      105770, // Deep Earth Vestments - 4 Piece Bonus (Item - Druid T13 Restoration 4P Bonus (Rejuvenation))
          0,
    },
    // Tier 14 bonuses (8 spells)
    {
      123082, // Regalia of the Eternal Blossom - 2 Piece Bonus (Item - Druid T14 Balance 2P Bonus)
      123083, // Regalia of the Eternal Blossom - 4 Piece Bonus (Item - Druid T14 Balance 4P Bonus)
      123084, // Battlegear of the Eternal Blossom - 2 Piece Bonus (Item - Druid T14 Feral 2P Bonus)
      123085, // Battlegear of the Eternal Blossom - 4 Piece Bonus (Item - Druid T14 Feral 4P Bonus)
      123086, // Armor of the Eternal Blossom - 2 Piece Bonus (Item - Druid T14 Guardian 2P Bonus)
      123087, // Armor of the Eternal Blossom - 4 Piece Bonus (Item - Druid T14 Guardian 4P Bonus)
      123088, // Vestments of the Eternal Blossom - 2 Piece Bonus (Item - Druid T14 Restoration 2P Bonus)
      123089, // Vestments of the Eternal Blossom - 4 Piece Bonus (Item - Druid T14 Restoration 4P Bonus)
    },
    // Tier 15 bonuses (8 spells)
    {
      138216, // Armor of the Haunted Forest - 2 Piece Bonus (Item - Druid T15 Guardian 2P Bonus)
      138222, // Armor of the Haunted Forest - 4 Piece Bonus (Item - Druid T15 Guardian 4P Bonus)
      138284, // Vestments of the Haunted Forest - 2 Piece Bonus (Item - Druid T15 Restoration 2P Bonus)
      138286, // Vestments of the Haunted Forest - 4 Piece Bonus (Item - Druid T15 Restoration 4P Bonus)
      138348, // Regalia of the Haunted Forest - 2 Piece Bonus (Item - Druid T15 Balance 2P Bonus)
      138350, // Regalia of the Haunted Forest - 4 Piece Bonus (Item - Druid T15 Balance 4P Bonus)
      138352, // Battlegear of the Haunted Forest - 2 Piece Bonus (Item - Druid T15 Feral 2P Bonus)
      138357, // Battlegear of the Haunted Forest - 4 Piece Bonus (Item - Druid T15 Feral 4P Bonus)
    },
    // Tier 16 bonuses (8 spells)
    {
      144756, // Regalia of the Shattered Vale - 4 Piece Bonus (Item - Druid T16 Balance 4P Bonus)
      144767, // Regalia of the Shattered Vale - 2 Piece Bonus (Item - Druid T16 Balance 2P Bonus)
      144841, // Battlegear of the Shattered Vale - 4 Piece Bonus (Item - Druid T16 Feral 4P Bonus)
      144864, // Battlegear of the Shattered Vale - 2 Piece Bonus (Item - Druid T16 Feral 2P Bonus)
      144869, // Vestments of the Shattered Vale - 2 Piece Bonus (Item - Druid T16 Restoration 2P Bonus)
      144875, // Vestments of the Shattered Vale - 4 Piece Bonus (Item - Druid T16 Restoration 4P Bonus)
      144879, // Armor of the Shattered Vale - 2 Piece Bonus (Item - Druid T16 Guardian 2P Bonus)
      144887, // Armor of the Shattered Vale - 4 Piece Bonus (Item - Druid T16 Guardian 4P Bonus)
    },
    {
          0,
    },
  },
};

#define SET_BONUS_DATA_SIZE (407)

// Set bonus data, wow build 18546
static item_set_bonus_t __set_bonus_data[SET_BONUS_DATA_SIZE] = {
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Siegebreaker Battlegear"                    ,    8,   2,   1,      {  71,  72, 0 },    0,  64938, {  46278,  46280,  46282,  46285,  46287,  0 } },
  { "Siegebreaker Battlegear"                    ,    8,   4,   1,      {  71,  72, 0 },    0,  64939, {  46278,  46280,  46282,  46285,  46287,  0 } },
  { "Siegebreaker Plate"                         ,    8,   2,   1,           {  73, 0 },    0,  64933, {  46279,  46281,  46283,  46284,  46286,  0 } },
  { "Siegebreaker Plate"                         ,    8,   4,   1,           {  73, 0 },    0,  64936, {  46279,  46281,  46283,  46284,  46286,  0 } },
  { "Aegis Regalia"                              ,    8,   2,   2,           {  65, 0 },    0,  64890, {  46217,  46218,  46220,  46227,  46228,  0 } },
  { "Aegis Regalia"                              ,    8,   4,   2,           {  65, 0 },    0,  64895, {  46217,  46218,  46220,  46227,  46228,  0 } },
  { "Aegis Plate"                                ,    8,   2,   2,           {  66, 0 },    0,  64881, {  46214,  46215,  46219,  46222,  46225,  0 } },
  { "Aegis Plate"                                ,    8,   4,   2,           {  66, 0 },    0,  64882, {  46214,  46215,  46219,  46222,  46225,  0 } },
  { "Aegis Battlegear"                           ,    8,   2,   2,           {  70, 0 },    0,  64878, {  46213,  46216,  46221,  46224,  46226,  0 } },
  { "Aegis Battlegear"                           ,    8,   4,   2,           {  70, 0 },    0,  64879, {  46213,  46216,  46221,  46224,  46226,  0 } },
  { "Scourgestalker Battlegear"                  ,    8,   2,   3, { 253, 254, 255, 0 },    0,  64854, {  46273,  46274,  46275,  46276,  46277,  0 } },
  { "Scourgestalker Battlegear"                  ,    8,   4,   3, { 253, 254, 255, 0 },    0,  64860, {  46273,  46274,  46275,  46276,  46277,  0 } },
  { "Terrorblade Battlegear"                     ,    8,   2,   4, { 259, 260, 261, 0 },    0,  64914, {  46288,  46289,  46290,  46291,  46292,  0 } },
  { "Terrorblade Battlegear"                     ,    8,   4,   4, { 259, 260, 261, 0 },    0,  64915, {  46288,  46289,  46290,  46291,  46292,  0 } },
  { "Sanctification Regalia"                     ,    8,   2,   5,      { 256, 257, 0 },    0,  64910, {  46231,  46247,  46254,  46258,  46259,  0 } },
  { "Sanctification Regalia"                     ,    8,   4,   5,      { 256, 257, 0 },    0,  64912, {  46231,  46247,  46254,  46258,  46259,  0 } },
  { "Sanctification Garb"                        ,    8,   2,   5,           { 258, 0 },    0,  64906, {  46230,  46248,  46255,  46256,  46257,  0 } },
  { "Sanctification Garb"                        ,    8,   4,   5,           { 258, 0 },    0,  64908, {  46230,  46248,  46255,  46256,  46257,  0 } },
  { "Darkruned Plate"                            ,    8,   2,   6,           { 250, 0 },    0,  64744, {  46233,  46234,  46236,  46238,  46240,  0 } },
  { "Darkruned Plate"                            ,    8,   4,   6,           { 250, 0 },    0,  64745, {  46233,  46234,  46236,  46238,  46240,  0 } },
  { "Darkruned Battlegear"                       ,    8,   2,   6,      { 251, 252, 0 },    0,  64735, {  46232,  46235,  46237,  46239,  46241,  0 } },
  { "Darkruned Battlegear"                       ,    8,   4,   6,      { 251, 252, 0 },    0,  64736, {  46232,  46235,  46237,  46239,  46241,  0 } },
  { "Worldbreaker Garb"                          ,    8,   2,   7,           { 262, 0 },    0,  64925, {  46295,  46298,  46300,  46301,  46304,  0 } },
  { "Worldbreaker Garb"                          ,    8,   4,   7,           { 262, 0 },    0,  64928, {  46295,  46298,  46300,  46301,  46304,  0 } },
  { "Worldbreaker Battlegear"                    ,    8,   2,   7,           { 263, 0 },    0,  64916, {  46293,  46294,  46296,  46303,  46307,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Worldbreaker Battlegear"                    ,    8,   4,   7,           { 263, 0 },    0,  64917, {  46293,  46294,  46296,  46303,  46307,  0 } },
  { "Worldbreaker Regalia"                       ,    8,   2,   7,           { 264, 0 },    0,  64921, {  46297,  46299,  46302,  46305,  46306,  0 } },
  { "Worldbreaker Regalia"                       ,    8,   4,   7,           { 264, 0 },    0,  64922, {  46297,  46299,  46302,  46305,  46306,  0 } },
  { "Kirin Tor Garb"                             ,    8,   2,   8, {  62,  63,  64, 0 },    0,  64867, {  46249,  46250,  46251,  46252,  46253,  0 } },
  { "Kirin Tor Garb"                             ,    8,   4,   8, {  62,  63,  64, 0 },    0,  64869, {  46249,  46250,  46251,  46252,  46253,  0 } },
  { "Deathbringer Garb"                          ,    8,   2,   9, { 265, 266, 267, 0 },    0,  64931, {  46242,  46243,  46244,  46245,  46246,  0 } },
  { "Deathbringer Garb"                          ,    8,   4,   9, { 265, 266, 267, 0 },    0,  64932, {  46242,  46243,  46244,  46245,  46246,  0 } },
  { "Nightsong Garb"                             ,    8,   2,  11,           { 102, 0 },    0,  64818, {  45350,  46309,  46266,  46271,  46272,  0 } },
  { "Nightsong Garb"                             ,    8,   4,  11,           { 102, 0 },    0,  64824, {  45350,  46309,  46266,  46271,  46272,  0 } },
  { "Nightsong Battlegear"                       ,    8,   2,  11,           { 103, 0 },    0,  64752, {  46260,  46262,  46265,  46267,  46269,  0 } },
  { "Nightsong Battlegear"                       ,    8,   4,  11,           { 103, 0 },    0,  64754, {  46260,  46262,  46265,  46267,  46269,  0 } },
  { "Nightsong Regalia"                          ,    8,   2,  11,           { 105, 0 },    0,  64756, {  46261,  46263,  46264,  46268,  46270,  0 } },
  { "Nightsong Regalia"                          ,    8,   4,  11,           { 105, 0 },    0,  64760, {  46261,  46263,  46264,  46268,  46270,  0 } },
  { "Hellscream's Battlegear"                    ,    9,   2,   1,      {  71,  72, 0 },    0,  67234, {  48860,  48861,  48862,  48863,  48864,  0 } },
  { "Wrynn's Battlegear"                         ,    9,   2,   1,      {  71,  72, 0 },    0,  67234, {  48865,  48866,  48867,  48868,  48869,  0 } },
  { "Hellscream's Battlegear"                    ,    9,   4,   1,      {  71,  72, 0 },    0,  67268, {  48860,  48861,  48862,  48863,  48864,  0 } },
  { "Wrynn's Battlegear"                         ,    9,   4,   1,      {  71,  72, 0 },    0,  67268, {  48865,  48866,  48867,  48868,  48869,  0 } },
  { "Wrynn's Plate"                              ,    9,   2,   1,           {  73, 0 },    0,  67269, {  48875,  48876,  48877,  48878,  48879,  0 } },
  { "Hellscream's Plate"                         ,    9,   2,   1,           {  73, 0 },    0,  67269, {  48870,  48871,  48872,  48873,  48874,  0 } },
  { "Wrynn's Plate"                              ,    9,   4,   1,           {  73, 0 },    0,  67273, {  48875,  48876,  48877,  48878,  48879,  0 } },
  { "Hellscream's Plate"                         ,    9,   4,   1,           {  73, 0 },    0,  67273, {  48870,  48871,  48872,  48873,  48874,  0 } },
  { "Liadrin's Garb"                             ,    9,   2,   2,           {  65, 0 },    0,  67190, {  48909,  48908,  48907,  48906,  48905,  0 } },
  { "Turalyon's Garb"                            ,    9,   2,   2,           {  65, 0 },    0,  67190, {  48904,  48903,  48902,  48901,  48900,  0 } },
  { "Liadrin's Garb"                             ,    9,   4,   2,           {  65, 0 },    0,  67191, {  48909,  48908,  48907,  48906,  48905,  0 } },
  { "Turalyon's Garb"                            ,    9,   4,   2,           {  65, 0 },    0,  67191, {  48904,  48903,  48902,  48901,  48900,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Turalyon's Plate"                           ,    9,   2,   2,           {  66, 0 },    0,  67186, {  48927,  48928,  48929,  48930,  48931,  0 } },
  { "Liadrin's Plate"                            ,    9,   2,   2,           {  66, 0 },    0,  67186, {  48922,  48923,  48924,  48925,  48926,  0 } },
  { "Turalyon's Plate"                           ,    9,   4,   2,           {  66, 0 },    0,  67187, {  48927,  48928,  48929,  48930,  48931,  0 } },
  { "Liadrin's Plate"                            ,    9,   4,   2,           {  66, 0 },    0,  67187, {  48922,  48923,  48924,  48925,  48926,  0 } },
  { "Turalyon's Battlegear"                      ,    9,   2,   2,           {  70, 0 },    0,  67188, {  48915,  48916,  48917,  48918,  48919,  0 } },
  { "Liadrin's Battlegear"                       ,    9,   2,   2,           {  70, 0 },    0,  67188, {  48910,  48911,  48912,  48913,  48914,  0 } },
  { "Turalyon's Battlegear"                      ,    9,   4,   2,           {  70, 0 },    0,  67189, {  48915,  48916,  48917,  48918,  48919,  0 } },
  { "Liadrin's Battlegear"                       ,    9,   4,   2,           {  70, 0 },    0,  67189, {  48910,  48911,  48912,  48913,  48914,  0 } },
  { "Windrunner's Battlegear"                    ,    9,   2,   3, { 253, 254, 255, 0 },    0,  67150, {  48819,  48820,  48821,  48822,  48823,  0 } },
  { "Windrunner's Pursuit"                       ,    9,   2,   3, { 253, 254, 255, 0 },    0,  67150, {  48814,  48815,  48816,  48817,  48818,  0 } },
  { "Windrunner's Battlegear"                    ,    9,   4,   3, { 253, 254, 255, 0 },    0,  67151, {  48819,  48820,  48821,  48822,  48823,  0 } },
  { "Windrunner's Pursuit"                       ,    9,   4,   3, { 253, 254, 255, 0 },    0,  67151, {  48814,  48815,  48816,  48817,  48818,  0 } },
  { "VanCleef's Battlegear"                      ,    9,   2,   4, { 259, 260, 261, 0 },    0,  67209, {  48809,  48810,  48811,  48812,  48813,  0 } },
  { "Garona's Battlegear"                        ,    9,   2,   4, { 259, 260, 261, 0 },    0,  67209, {  48804,  48805,  48806,  48807,  48808,  0 } },
  { "VanCleef's Battlegear"                      ,    9,   4,   4, { 259, 260, 261, 0 },    0,  67211, {  48809,  48810,  48811,  48812,  48813,  0 } },
  { "Garona's Battlegear"                        ,    9,   4,   4, { 259, 260, 261, 0 },    0,  67211, {  48804,  48805,  48806,  48807,  48808,  0 } },
  { "Zabra's Raiment"                            ,    9,   2,   5,      { 256, 257, 0 },    0,  67201, {  48750,  48751,  48752,  48753,  48754,  0 } },
  { "Velen's Raiment"                            ,    9,   2,   5,      { 256, 257, 0 },    0,  67201, {  48745,  48746,  48747,  48748,  48749,  0 } },
  { "Zabra's Raiment"                            ,    9,   4,   5,      { 256, 257, 0 },    0,  67202, {  48750,  48751,  48752,  48753,  48754,  0 } },
  { "Velen's Raiment"                            ,    9,   4,   5,      { 256, 257, 0 },    0,  67202, {  48745,  48746,  48747,  48748,  48749,  0 } },
  { "Zabra's Regalia"                            ,    9,   2,   5,           { 258, 0 },    0,  67193, {  48760,  48761,  48762,  48763,  48764,  0 } },
  { "Velen's Regalia"                            ,    9,   2,   5,           { 258, 0 },    0,  67193, {  48755,  48756,  48757,  48758,  48759,  0 } },
  { "Zabra's Regalia"                            ,    9,   4,   5,           { 258, 0 },    0,  67198, {  48760,  48761,  48762,  48763,  48764,  0 } },
  { "Velen's Regalia"                            ,    9,   4,   5,           { 258, 0 },    0,  67198, {  48755,  48756,  48757,  48758,  48759,  0 } },
  { "Thassarian's Plate"                         ,    9,   2,   6,           { 250, 0 },    0,  67119, {  48895,  48896,  48897,  48898,  48899,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Koltira's Plate"                            ,    9,   2,   6,           { 250, 0 },    0,  67119, {  48890,  48891,  48892,  48893,  48894,  0 } },
  { "Thassarian's Plate"                         ,    9,   4,   6,           { 250, 0 },    0,  67120, {  48895,  48896,  48897,  48898,  48899,  0 } },
  { "Koltira's Plate"                            ,    9,   4,   6,           { 250, 0 },    0,  67120, {  48890,  48891,  48892,  48893,  48894,  0 } },
  { "Thassarian's Battlegear"                    ,    9,   2,   6,      { 251, 252, 0 },    0,  67115, {  48885,  48886,  48887,  48888,  48889,  0 } },
  { "Koltira's Battlegear"                       ,    9,   2,   6,      { 251, 252, 0 },    0,  67115, {  48880,  48881,  48882,  48883,  48884,  0 } },
  { "Thassarian's Battlegear"                    ,    9,   4,   6,      { 251, 252, 0 },    0,  67118, {  48885,  48886,  48887,  48888,  48889,  0 } },
  { "Koltira's Battlegear"                       ,    9,   4,   6,      { 251, 252, 0 },    0,  67118, {  48880,  48881,  48882,  48883,  48884,  0 } },
  { "Thrall's Regalia"                           ,    9,   2,   7,           { 262, 0 },    0,  67227, {  48841,  48842,  48843,  48844,  48845,  0 } },
  { "Nobundo's Regalia"                          ,    9,   2,   7,           { 262, 0 },    0,  67227, {  48836,  48837,  48838,  48839,  48840,  0 } },
  { "Thrall's Regalia"                           ,    9,   4,   7,           { 262, 0 },    0,  67228, {  48841,  48842,  48843,  48844,  48845,  0 } },
  { "Nobundo's Regalia"                          ,    9,   4,   7,           { 262, 0 },    0,  67228, {  48836,  48837,  48838,  48839,  48840,  0 } },
  { "Nobundo's Battlegear"                       ,    9,   2,   7,           { 263, 0 },    0,  67220, {  48846,  48847,  48848,  48849,  48850,  0 } },
  { "Thrall's Battlegear"                        ,    9,   2,   7,           { 263, 0 },    0,  67220, {  48854,  48855,  48852,  48853,  48851,  0 } },
  { "Nobundo's Battlegear"                       ,    9,   4,   7,           { 263, 0 },    0,  67221, {  48846,  48847,  48848,  48849,  48850,  0 } },
  { "Thrall's Battlegear"                        ,    9,   4,   7,           { 263, 0 },    0,  67221, {  48854,  48855,  48852,  48853,  48851,  0 } },
  { "Nobundo's Garb"                             ,    9,   2,   7,           { 264, 0 },    0,  67225, {  48824,  48825,  48826,  48827,  48828,  0 } },
  { "Thrall's Garb"                              ,    9,   2,   7,           { 264, 0 },    0,  67225, {  48829,  48830,  48831,  48832,  48833,  0 } },
  { "Nobundo's Garb"                             ,    9,   4,   7,           { 264, 0 },    0,  67226, {  48824,  48825,  48826,  48827,  48828,  0 } },
  { "Thrall's Garb"                              ,    9,   4,   7,           { 264, 0 },    0,  67226, {  48829,  48830,  48831,  48832,  48833,  0 } },
  { "Khadgar's Regalia"                          ,    9,   2,   8, {  62,  63,  64, 0 },    0,  67164, {  48725,  48726,  48727,  48728,  48729,  0 } },
  { "Sunstrider's Regalia"                       ,    9,   2,   8, {  62,  63,  64, 0 },    0,  67164, {  48730,  48731,  48732,  48733,  48734,  0 } },
  { "Khadgar's Regalia"                          ,    9,   4,   8, {  62,  63,  64, 0 },    0,  67185, {  48725,  48726,  48727,  48728,  48729,  0 } },
  { "Sunstrider's Regalia"                       ,    9,   4,   8, {  62,  63,  64, 0 },    0,  67185, {  48730,  48731,  48732,  48733,  48734,  0 } },
  { "Gul'dan's Regalia"                          ,    9,   2,   9, { 265, 266, 267, 0 },    0,  67230, {  48735,  48736,  48737,  48738,  48739,  0 } },
  { "Kel'Thuzad's Regalia"                       ,    9,   2,   9, { 265, 266, 267, 0 },    0,  67230, {  48740,  48741,  48742,  48743,  48744,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Gul'dan's Regalia"                          ,    9,   4,   9, { 265, 266, 267, 0 },    0,  67231, {  48735,  48736,  48737,  48738,  48739,  0 } },
  { "Kel'Thuzad's Regalia"                       ,    9,   4,   9, { 265, 266, 267, 0 },    0,  67231, {  48740,  48741,  48742,  48743,  48744,  0 } },
  { "Malfurion's Regalia"                        ,    9,   2,  11,           { 102, 0 },    0,  67125, {  48786,  48787,  48788,  48789,  48790,  0 } },
  { "Runetotem's Regalia"                        ,    9,   2,  11,           { 102, 0 },    0,  67125, {  48781,  48782,  48783,  48784,  48785,  0 } },
  { "Malfurion's Regalia"                        ,    9,   4,  11,           { 102, 0 },    0,  67126, {  48786,  48787,  48788,  48789,  48790,  0 } },
  { "Runetotem's Regalia"                        ,    9,   4,  11,           { 102, 0 },    0,  67126, {  48781,  48782,  48783,  48784,  48785,  0 } },
  { "Runetotem's Battlegear"                     ,    9,   2,  11,           { 103, 0 },    0,  67121, {  48794,  48795,  48796,  48797,  48798,  0 } },
  { "Malfurion's Battlegear"                     ,    9,   2,  11,           { 103, 0 },    0,  67121, {  48799,  48800,  48801,  48802,  48803,  0 } },
  { "Runetotem's Battlegear"                     ,    9,   4,  11,           { 103, 0 },    0,  67123, {  48794,  48795,  48796,  48797,  48798,  0 } },
  { "Malfurion's Battlegear"                     ,    9,   4,  11,           { 103, 0 },    0,  67123, {  48799,  48800,  48801,  48802,  48803,  0 } },
  { "Runetotem's Garb"                           ,    9,   2,  11,           { 105, 0 },    0,  67127, {  48769,  48770,  48771,  48772,  48773,  0 } },
  { "Malfurion's Garb"                           ,    9,   2,  11,           { 105, 0 },    0,  67127, {  48774,  48775,  48776,  48777,  48778,  0 } },
  { "Runetotem's Garb"                           ,    9,   4,  11,           { 105, 0 },    0,  67128, {  48769,  48770,  48771,  48772,  48773,  0 } },
  { "Malfurion's Garb"                           ,    9,   4,  11,           { 105, 0 },    0,  67128, {  48774,  48775,  48776,  48777,  48778,  0 } },
  { "Ymirjar Lord's Battlegear"                  ,   10,   2,   1,      {  71,  72, 0 },    0,  70854, {  51771,  51770,  51769,  51768,  51767,  0 } },
  { "Ymirjar Lord's Battlegear"                  ,   10,   4,   1,      {  71,  72, 0 },    0,  70847, {  51771,  51770,  51769,  51768,  51767,  0 } },
  { "Ymirjar Lord's Plate"                       ,   10,   2,   1,           {  73, 0 },    0,  70843, {  51776,  51775,  51774,  51773,  51772,  0 } },
  { "Ymirjar Lord's Plate"                       ,   10,   4,   1,           {  73, 0 },    0,  70844, {  51776,  51775,  51774,  51773,  51772,  0 } },
  { "Lightsworn Garb"                            ,   10,   2,   2,           {  65, 0 },    0,  70755, {  51726,  51725,  51724,  51723,  51722,  0 } },
  { "Lightsworn Garb"                            ,   10,   4,   2,           {  65, 0 },    0,  70756, {  51726,  51725,  51724,  51723,  51722,  0 } },
  { "Lightsworn Plate"                           ,   10,   2,   2,           {  66, 0 },    0,  70758, {  51731,  51730,  51729,  51728,  51727,  0 } },
  { "Lightsworn Plate"                           ,   10,   4,   2,           {  66, 0 },    0,  70761, {  51731,  51730,  51729,  51728,  51727,  0 } },
  { "Lightsworn Battlegear"                      ,   10,   2,   2,           {  70, 0 },    0,  70765, {  51721,  51720,  51719,  51718,  51717,  0 } },
  { "Lightsworn Battlegear"                      ,   10,   4,   2,           {  70, 0 },    0,  70762, {  51721,  51720,  51719,  51718,  51717,  0 } },
  { "Ahn'Kahar Blood Hunter's Battlegear"        ,   10,   2,   3, { 253, 254, 255, 0 },    0,  70727, {  51711,  51710,  51709,  51708,  51707,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Ahn'Kahar Blood Hunter's Battlegear"        ,   10,   4,   3, { 253, 254, 255, 0 },    0,  70730, {  51711,  51710,  51709,  51708,  51707,  0 } },
  { "Shadowblade's Battlegear"                   ,   10,   2,   4, { 259, 260, 261, 0 },    0,  70805, {  51746,  51745,  51744,  51743,  51742,  0 } },
  { "Shadowblade's Battlegear"                   ,   10,   4,   4, { 259, 260, 261, 0 },    0,  70803, {  51746,  51745,  51744,  51743,  51742,  0 } },
  { "Crimson Acolyte's Raiment"                  ,   10,   2,   5,      { 256, 257, 0 },    0,  70770, {  51732,  51733,  51734,  51735,  51736,  0 } },
  { "Crimson Acolyte's Raiment"                  ,   10,   4,   5,      { 256, 257, 0 },    0,  70798, {  51732,  51733,  51734,  51735,  51736,  0 } },
  { "Crimson Acolyte's Regalia"                  ,   10,   2,   5,           { 258, 0 },    0,  70800, {  51741,  51740,  51739,  51738,  51737,  0 } },
  { "Crimson Acolyte's Regalia"                  ,   10,   4,   5,           { 258, 0 },    0,  70801, {  51741,  51740,  51739,  51738,  51737,  0 } },
  { "Scourgelord's Plate"                        ,   10,   2,   6,           { 250, 0 },    0,  70650, {  51691,  51690,  51689,  51688,  51687,  0 } },
  { "Scourgelord's Plate"                        ,   10,   4,   6,           { 250, 0 },    0,  70652, {  51691,  51690,  51689,  51688,  51687,  0 } },
  { "Scourgelord's Battlegear"                   ,   10,   2,   6,      { 251, 252, 0 },    0,  70655, {  51686,  51685,  51684,  51683,  51682,  0 } },
  { "Scourgelord's Battlegear"                   ,   10,   4,   6,      { 251, 252, 0 },    0,  70656, {  51686,  51685,  51684,  51683,  51682,  0 } },
  { "Frost Witch's Regalia"                      ,   10,   2,   7,           { 262, 0 },    0,  70811, {  51757,  51758,  51759,  51760,  51761,  0 } },
  { "Frost Witch's Regalia"                      ,   10,   4,   7,           { 262, 0 },    0,  70817, {  51757,  51758,  51759,  51760,  51761,  0 } },
  { "Frost Witch's Battlegear"                   ,   10,   2,   7,           { 263, 0 },    0,  70830, {  51752,  51753,  51754,  51755,  51756,  0 } },
  { "Frost Witch's Battlegear"                   ,   10,   4,   7,           { 263, 0 },    0,  70832, {  51752,  51753,  51754,  51755,  51756,  0 } },
  { "Frost Witch's Garb"                         ,   10,   2,   7,           { 264, 0 },    0,  70807, {  51747,  51748,  51749,  51750,  51751,  0 } },
  { "Frost Witch's Garb"                         ,   10,   4,   7,           { 264, 0 },    0,  70808, {  51747,  51748,  51749,  51750,  51751,  0 } },
  { "Bloodmage's Regalia"                        ,   10,   2,   8, {  62,  63,  64, 0 },    0,  70752, {  51716,  51715,  51714,  51713,  51712,  0 } },
  { "Bloodmage's Regalia"                        ,   10,   4,   8, {  62,  63,  64, 0 },    0,  70748, {  51716,  51715,  51714,  51713,  51712,  0 } },
  { "Dark Coven's Regalia"                       ,   10,   2,   9, { 265, 266, 267, 0 },    0,  70839, {  51766,  51765,  51764,  51763,  51762,  0 } },
  { "Dark Coven's Regalia"                       ,   10,   4,   9, { 265, 266, 267, 0 },    0,  70841, {  51766,  51765,  51764,  51763,  51762,  0 } },
  { "Lasherweave Regalia"                        ,   10,   2,  11,           { 102, 0 },    0,  70718, {  51706,  51705,  51704,  51703,  51702,  0 } },
  { "Lasherweave Regalia"                        ,   10,   4,  11,           { 102, 0 },    0,  70723, {  51706,  51705,  51704,  51703,  51702,  0 } },
  { "Lasherweave Battlegear"                     ,   10,   2,  11,           { 103, 0 },    0,  70724, {  51701,  51700,  51699,  51698,  51697,  0 } },
  { "Lasherweave Battlegear"                     ,   10,   4,  11,           { 103, 0 },    0,  70726, {  51701,  51700,  51699,  51698,  51697,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Lasherweave Garb"                           ,   10,   2,  11,           { 105, 0 },    0,  70658, {  51696,  51695,  51694,  51693,  51692,  0 } },
  { "Lasherweave Garb"                           ,   10,   4,  11,           { 105, 0 },    0,  70664, {  51696,  51695,  51694,  51693,  51692,  0 } },
  { "Earthen Warplate"                           ,   11,   2,   1,      {  71,  72, 0 },    0,  90293, {  60327,  60324,  60325,  60326,  60323,  0 } },
  { "Earthen Warplate"                           ,   11,   4,   1,      {  71,  72, 0 },    0,  90295, {  60327,  60324,  60325,  60326,  60323,  0 } },
  { "Earthen Battleplate"                        ,   11,   2,   1,           {  73, 0 },    0,  90296, {  60331,  60330,  60328,  60332,  60329,  0 } },
  { "Earthen Battleplate"                        ,   11,   4,   1,           {  73, 0 },    0,  90297, {  60331,  60330,  60328,  60332,  60329,  0 } },
  { "Reinforced Sapphirium Regalia"              ,   11,   2,   2,           {  65, 0 },    0,  90310, {  60362,  60361,  60359,  60363,  60360,  0 } },
  { "Reinforced Sapphirium Regalia"              ,   11,   4,   2,           {  65, 0 },    0,  90313, {  60362,  60361,  60359,  60363,  60360,  0 } },
  { "Reinforced Sapphirium Battlearmor"          ,   11,   2,   2,           {  66, 0 },    0,  90301, {  60358,  60357,  60356,  60355,  60354,  0 } },
  { "Reinforced Sapphirium Battlearmor"          ,   11,   4,   2,           {  66, 0 },    0,  90306, {  60358,  60357,  60356,  60355,  60354,  0 } },
  { "Reinforced Sapphirium Battleplate"          ,   11,   2,   2,           {  70, 0 },    0,  90298, {  60348,  60347,  60346,  60345,  60344,  0 } },
  { "Reinforced Sapphirium Battleplate"          ,   11,   4,   2,           {  70, 0 },    0,  90299, {  60348,  60347,  60346,  60345,  60344,  0 } },
  { "Lightning-Charged Battlegear"               ,   11,   2,   3, { 253, 254, 255, 0 },    0,  89923, {  60306,  60305,  60303,  60307,  60304,  0 } },
  { "Lightning-Charged Battlegear"               ,   11,   4,   3, { 253, 254, 255, 0 },    0,  96411, {  60306,  60305,  60303,  60307,  60304,  0 } },
  { "Wind Dancer's Regalia"                      ,   11,   2,   4, { 259, 260, 261, 0 },    0,  90460, {  60302,  60300,  60299,  60298,  60301,  0 } },
  { "Wind Dancer's Regalia"                      ,   11,   4,   4, { 259, 260, 261, 0 },    0,  90473, {  60302,  60300,  60299,  60298,  60301,  0 } },
  { "Mercurial Vestments"                        ,   11,   2,   5,      { 256, 257, 0 },    0,  89910, {  60262,  60259,  60261,  60258,  60275,  0 } },
  { "Mercurial Vestments"                        ,   11,   4,   5,      { 256, 257, 0 },    0,  89911, {  60262,  60259,  60261,  60258,  60275,  0 } },
  { "Mercurial Regalia"                          ,   11,   2,   5,           { 258, 0 },    0,  89915, {  60253,  60254,  60255,  60256,  60257,  0 } },
  { "Mercurial Regalia"                          ,   11,   4,   5,           { 258, 0 },    0,  89922, {  60253,  60254,  60255,  60256,  60257,  0 } },
  { "Magma Plated Battlearmor"                   ,   11,   2,   6,           { 250, 0 },    0,  90454, {  60349,  60350,  60351,  60352,  60353,  0 } },
  { "Magma Plated Battlearmor"                   ,   11,   4,   6,           { 250, 0 },    0,  90456, {  60349,  60350,  60351,  60352,  60353,  0 } },
  { "Magma Plated Battlegear"                    ,   11,   2,   6,      { 251, 252, 0 },    0,  90457, {  60339,  60340,  60341,  60342,  60343,  0 } },
  { "Magma Plated Battlegear"                    ,   11,   4,   6,      { 251, 252, 0 },    0,  90459, {  60339,  60340,  60341,  60342,  60343,  0 } },
  { "Regalia of the Raging Elements"             ,   11,   2,   7,           { 262, 0 },    0,  90503, {  60317,  60316,  60315,  60314,  60313,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Regalia of the Raging Elements"             ,   11,   4,   7,           { 262, 0 },    0,  90505, {  60317,  60316,  60315,  60314,  60313,  0 } },
  { "Battlegear of the Raging Elements"          ,   11,   2,   7,           { 263, 0 },    0,  90501, {  60322,  60321,  60320,  60319,  60318,  0 } },
  { "Battlegear of the Raging Elements"          ,   11,   4,   7,           { 263, 0 },    0,  90502, {  60322,  60321,  60320,  60319,  60318,  0 } },
  { "Vestments of the Raging Elements"           ,   11,   2,   7,           { 264, 0 },    0,  90494, {  60311,  60310,  60308,  60312,  60309,  0 } },
  { "Vestments of the Raging Elements"           ,   11,   4,   7,           { 264, 0 },    0,  90499, {  60311,  60310,  60308,  60312,  60309,  0 } },
  { "Firelord's Vestments"                       ,   11,   2,   8, {  62,  63,  64, 0 },    0,  90290, {  60246,  60244,  60245,  60243,  60247,  0 } },
  { "Firelord's Vestments"                       ,   11,   4,   8, {  62,  63,  64, 0 },    0,  90291, {  60246,  60244,  60245,  60243,  60247,  0 } },
  { "Shadowflame Regalia"                        ,   11,   2,   9, { 265, 266, 267, 0 },    0,  89934, {  60252,  60251,  60250,  60249,  60248,  0 } },
  { "Shadowflame Regalia"                        ,   11,   4,   9, { 265, 266, 267, 0 },    0,  89935, {  60252,  60251,  60250,  60249,  60248,  0 } },
  { "Stormrider's Regalia"                       ,   11,   2,  11,           { 102, 0 },    0,  90160, {  60284,  60281,  60283,  60282,  60285,  0 } },
  { "Stormrider's Regalia"                       ,   11,   4,  11,           { 102, 0 },    0,  90163, {  60284,  60281,  60283,  60282,  60285,  0 } },
  { "Stormrider's Battlegarb"                    ,   11,   2,  11,           { 103, 0 },    0,  90162, {  60290,  60286,  60288,  60287,  60289,  0 } },
  { "Stormrider's Battlegarb"                    ,   11,   4,  11,           { 103, 0 },    0,  90165, {  60290,  60286,  60288,  60287,  60289,  0 } },
  { "Stormrider's Vestments"                     ,   11,   2,  11,           { 105, 0 },    0,  90157, {  60280,  60277,  60278,  60276,  60279,  0 } },
  { "Stormrider's Vestments"                     ,   11,   4,  11,           { 105, 0 },    0,  90158, {  60280,  60277,  60278,  60276,  60279,  0 } },
  { "Molten Giant Warplate"                      ,   12,   2,   1,      {  71,  72, 0 },    0,  99234, {  71072,  71071,  71069,  71068,  71070,  0 } },
  { "Molten Giant Warplate"                      ,   12,   4,   1,      {  71,  72, 0 },    0,  99238, {  71072,  71071,  71069,  71068,  71070,  0 } },
  { "Molten Giant Battleplate"                   ,   12,   2,   1,           {  73, 0 },    0,  99239, {  70941,  70942,  70944,  70943,  70945,  0 } },
  { "Molten Giant Battleplate"                   ,   12,   4,   1,           {  73, 0 },    0,  99242, {  70941,  70942,  70944,  70943,  70945,  0 } },
  { "Regalia of Immolation"                      ,   12,   2,   2,           {  65, 0 },    0,  99067, {  71095,  71094,  71093,  71092,  71091,  0 } },
  { "Regalia of Immolation"                      ,   12,   4,   2,           {  65, 0 },    0,  99070, {  71095,  71094,  71093,  71092,  71091,  0 } },
  { "Battlearmor of Immolation"                  ,   12,   2,   2,           {  66, 0 },    0,  99074, {  70946,  70947,  70948,  70949,  70950,  0 } },
  { "Battlearmor of Immolation"                  ,   12,   4,   2,           {  66, 0 },    0,  99091, {  70946,  70947,  70948,  70949,  70950,  0 } },
  { "Battleplate of Immolation"                  ,   12,   2,   2,           {  70, 0 },    0,  99093, {  71067,  71066,  71065,  71064,  71063,  0 } },
  { "Battleplate of Immolation"                  ,   12,   4,   2,           {  70, 0 },    0,  99116, {  71067,  71066,  71065,  71064,  71063,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Flamewaker's Battlegear"                    ,   12,   2,   3, { 253, 254, 255, 0 },    0,  99057, {  71050,  71051,  71052,  71053,  71054,  0 } },
  { "Flamewaker's Battlegear"                    ,   12,   4,   3, { 253, 254, 255, 0 },    0,  99059, {  71050,  71051,  71052,  71053,  71054,  0 } },
  { "Vestments of the Dark Phoenix"              ,   12,   2,   4, { 259, 260, 261, 0 },    0,  99174, {  71045,  71046,  71047,  71048,  71049,  0 } },
  { "Vestments of the Dark Phoenix"              ,   12,   4,   4, { 259, 260, 261, 0 },    0,  99175, {  71045,  71046,  71047,  71048,  71049,  0 } },
  { "Vestments of the Cleansing Flame"           ,   12,   2,   5,      { 256, 257, 0 },    0,  99134, {  71271,  71272,  71273,  71274,  71275,  0 } },
  { "Vestments of the Cleansing Flame"           ,   12,   4,   5,      { 256, 257, 0 },    0,  99135, {  71271,  71272,  71273,  71274,  71275,  0 } },
  { "Regalia of the Cleansing Flame"             ,   12,   2,   5,           { 258, 0 },    0,  99154, {  71280,  71279,  71278,  71277,  71276,  0 } },
  { "Regalia of the Cleansing Flame"             ,   12,   4,   5,           { 258, 0 },    0,  99157, {  71280,  71279,  71278,  71277,  71276,  0 } },
  { "Elementium Deathplate Battlearmor"          ,   12,   2,   6,           { 250, 0 },    0,  98956, {  70951,  70952,  70953,  70954,  70955,  0 } },
  { "Elementium Deathplate Battlearmor"          ,   12,   4,   6,           { 250, 0 },    0,  98966, {  70951,  70952,  70953,  70954,  70955,  0 } },
  { "Elementium Deathplate Battlegear"           ,   12,   2,   6,      { 251, 252, 0 },    0,  98970, {  71058,  71059,  71060,  71061,  71062,  0 } },
  { "Elementium Deathplate Battlegear"           ,   12,   4,   6,      { 251, 252, 0 },    0,  98996, {  71058,  71059,  71060,  71061,  71062,  0 } },
  { "Volcanic Regalia"                           ,   12,   2,   7,           { 262, 0 },    0,  99204, {  71295,  71294,  71293,  71292,  71291,  0 } },
  { "Volcanic Regalia"                           ,   12,   4,   7,           { 262, 0 },    0,  99206, {  71295,  71294,  71293,  71292,  71291,  0 } },
  { "Volcanic Battlegear"                        ,   12,   2,   7,           { 263, 0 },    0,  99209, {  71305,  71304,  71303,  71302,  71301,  0 } },
  { "Volcanic Battlegear"                        ,   12,   4,   7,           { 263, 0 },    0,  99213, {  71305,  71304,  71303,  71302,  71301,  0 } },
  { "Volcanic Vestments"                         ,   12,   2,   7,           { 264, 0 },    0,  99190, {  71300,  71299,  71298,  71297,  71296,  0 } },
  { "Volcanic Vestments"                         ,   12,   4,   7,           { 264, 0 },    0,  99195, {  71300,  71299,  71298,  71297,  71296,  0 } },
  { "Firehawk Robes of Conflagration"            ,   12,   2,   8, {  62,  63,  64, 0 },    0,  99061, {  71290,  71286,  71287,  71288,  71289,  0 } },
  { "Firehawk Robes of Conflagration"            ,   12,   4,   8, {  62,  63,  64, 0 },    0,  99064, {  71290,  71286,  71287,  71288,  71289,  0 } },
  { "Balespider's Burning Vestments"             ,   12,   2,   9, { 265, 266, 267, 0 },    0,  99220, {  71281,  71283,  71282,  71284,  71285,  0 } },
  { "Balespider's Burning Vestments"             ,   12,   4,   9, { 265, 266, 267, 0 },    0,  99229, {  71281,  71283,  71282,  71284,  71285,  0 } },
  { "Obsidian Arborweave Regalia"                ,   12,   2,  11,           { 102, 0 },    0,  99019, {  71107,  71108,  71109,  71110,  71111,  0 } },
  { "Obsidian Arborweave Regalia"                ,   12,   4,  11,           { 102, 0 },    0,  99049, {  71107,  71108,  71109,  71110,  71111,  0 } },
  { "Obsidian Arborweave Battlegarb"             ,   12,   2,  11,           { 103, 0 },    0,  99001, {  71097,  71098,  71099,  71100,  71101,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Obsidian Arborweave Battlegarb"             ,   12,   4,  11,           { 103, 0 },    0,  99009, {  71097,  71098,  71099,  71100,  71101,  0 } },
  { "Obsidian Arborweave Vestments"              ,   12,   2,  11,           { 105, 0 },    0,  99013, {  71102,  71103,  71104,  71105,  71106,  0 } },
  { "Obsidian Arborweave Vestments"              ,   12,   4,  11,           { 105, 0 },    0,  99015, {  71102,  71103,  71104,  71105,  71106,  0 } },
  { "Colossal Dragonplate Battlegear"            ,   13,   2,   1,      {  71,  72, 0 },    0, 105797, {  76983,  76984,  76985,  76986,  76987,  0 } },
  { "Colossal Dragonplate Battlegear"            ,   13,   4,   1,      {  71,  72, 0 },    0, 105907, {  76983,  76984,  76985,  76986,  76987,  0 } },
  { "Colossal Dragonplate Armor"                 ,   13,   2,   1,           {  73, 0 },    0, 105908, {  76988,  76989,  76990,  76991,  76992,  0 } },
  { "Colossal Dragonplate Armor"                 ,   13,   4,   1,           {  73, 0 },    0, 105911, {  76988,  76989,  76990,  76991,  76992,  0 } },
  { "Regalia of Radiant Glory"                   ,   13,   2,   2,           {  65, 0 },    0, 105743, {  76765,  76766,  76767,  76768,  76769,  0 } },
  { "Regalia of Radiant Glory"                   ,   13,   4,   2,           {  65, 0 },    0, 105798, {  76765,  76766,  76767,  76768,  76769,  0 } },
  { "Armor of Radiant Glory"                     ,   13,   2,   2,           {  66, 0 },    0, 105800, {  77003,  77004,  77005,  77006,  77007,  0 } },
  { "Armor of Radiant Glory"                     ,   13,   4,   2,           {  66, 0 },    0, 105744, {  77003,  77004,  77005,  77006,  77007,  0 } },
  { "Battleplate of Radiant Glory"               ,   13,   2,   2,           {  70, 0 },    0, 105765, {  76874,  76875,  76876,  76877,  76878,  0 } },
  { "Battleplate of Radiant Glory"               ,   13,   4,   2,           {  70, 0 },    0, 105820, {  76874,  76875,  76876,  76877,  76878,  0 } },
  { "Wyrmstalker Battlegear"                     ,   13,   2,   3, { 253, 254, 255, 0 },    0, 105732, {  77028,  77029,  77030,  77031,  77032,  0 } },
  { "Wyrmstalker Battlegear"                     ,   13,   4,   3, { 253, 254, 255, 0 },    0, 105921, {  77028,  77029,  77030,  77031,  77032,  0 } },
  { "Blackfang Battleweave"                      ,   13,   2,   4, { 259, 260, 261, 0 },    0, 105849, {  77023,  77024,  77025,  77026,  77027,  0 } },
  { "Blackfang Battleweave"                      ,   13,   4,   4, { 259, 260, 261, 0 },    0, 105865, {  77023,  77024,  77025,  77026,  77027,  0 } },
  { "Vestments of Dying Light"                   ,   13,   2,   5,      { 256, 257, 0 },    0, 105827, {  76357,  76358,  76359,  76360,  76361,  0 } },
  { "Vestments of Dying Light"                   ,   13,   4,   5,      { 256, 257, 0 },    0, 105832, {  76357,  76358,  76359,  76360,  76361,  0 } },
  { "Regalia of Dying Light"                     ,   13,   2,   5,           { 258, 0 },    0, 105843, {  76348,  76347,  76346,  76345,  76344,  0 } },
  { "Regalia of Dying Light"                     ,   13,   4,   5,           { 258, 0 },    0, 105844, {  76348,  76347,  76346,  76345,  76344,  0 } },
  { "Necrotic Boneplate Armor"                   ,   13,   2,   6,           { 250, 0 },    0, 105552, {  77008,  77009,  77010,  77011,  77012,  0 } },
  { "Necrotic Boneplate Armor"                   ,   13,   4,   6,           { 250, 0 },    0, 105587, {  77008,  77009,  77010,  77011,  77012,  0 } },
  { "Necrotic Boneplate Battlegear"              ,   13,   2,   6,      { 251, 252, 0 },    0, 105609, {  76974,  76975,  76976,  76977,  76978,  0 } },
  { "Necrotic Boneplate Battlegear"              ,   13,   4,   6,      { 251, 252, 0 },    0, 105646, {  76974,  76975,  76976,  76977,  76978,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Spiritwalker's Regalia"                     ,   13,   2,   7,           { 262, 0 },    0, 105780, {  77039,  77038,  77037,  77036,  77035,  0 } },
  { "Spiritwalker's Regalia"                     ,   13,   4,   7,           { 262, 0 },    0, 105816, {  77039,  77038,  77037,  77036,  77035,  0 } },
  { "Spiritwalker's Battlegear"                  ,   13,   2,   7,           { 263, 0 },    0, 105866, {  77040,  77041,  77042,  77043,  77044,  0 } },
  { "Spiritwalker's Battlegear"                  ,   13,   4,   7,           { 263, 0 },    0, 105872, {  77040,  77041,  77042,  77043,  77044,  0 } },
  { "Spiritwalker's Vestments"                   ,   13,   2,   7,           { 264, 0 },    0, 105764, {  76756,  76757,  76758,  76759,  76760,  0 } },
  { "Spiritwalker's Vestments"                   ,   13,   4,   7,           { 264, 0 },    0, 105876, {  76756,  76757,  76758,  76759,  76760,  0 } },
  { "Time Lord's Regalia"                        ,   13,   2,   8, {  62,  63,  64, 0 },    0, 105788, {  76212,  76213,  76214,  76215,  76216,  0 } },
  { "Time Lord's Regalia"                        ,   13,   4,   8, {  62,  63,  64, 0 },    0, 105790, {  76212,  76213,  76214,  76215,  76216,  0 } },
  { "Vestments of the Faceless Shroud"           ,   13,   2,   9, { 265, 266, 267, 0 },    0, 105888, {  76343,  76342,  76341,  76340,  76339,  0 } },
  { "Vestments of the Faceless Shroud"           ,   13,   4,   9, { 265, 266, 267, 0 },    0, 105787, {  76343,  76342,  76341,  76340,  76339,  0 } },
  { "Deep Earth Regalia"                         ,   13,   2,  11,           { 102, 0 },    0, 105722, {  77018,  77019,  77020,  77021,  77022,  0 } },
  { "Deep Earth Regalia"                         ,   13,   4,  11,           { 102, 0 },    0, 105717, {  77018,  77019,  77020,  77021,  77022,  0 } },
  { "Deep Earth Battlegarb"                      ,   13,   2,  11,           { 103, 0 },    0, 105725, {  77013,  77014,  77015,  77016,  77017,  0 } },
  { "Deep Earth Battlegarb"                      ,   13,   4,  11,           { 103, 0 },    0, 105735, {  77013,  77014,  77015,  77016,  77017,  0 } },
  { "Deep Earth Vestments"                       ,   13,   2,  11,           { 105, 0 },    0, 105715, {  76749,  76750,  76751,  76752,  76753,  0 } },
  { "Deep Earth Vestments"                       ,   13,   4,  11,           { 105, 0 },    0, 105770, {  76749,  76750,  76751,  76752,  76753,  0 } },
  { "Battleplate of Resounding Rings"            ,   14,   2,   1,      {  71,  72, 0 },    0, 123142, {  85329,  85330,  85331,  85332,  85333,  0 } },
  { "Battleplate of Resounding Rings"            ,   14,   4,   1,      {  71,  72, 0 },    0, 123144, {  85329,  85330,  85331,  85332,  85333,  0 } },
  { "Plate of Resounding Rings"                  ,   14,   2,   1,           {  73, 0 },    0, 123146, {  85324,  85325,  85326,  85327,  85328,  0 } },
  { "Plate of Resounding Rings"                  ,   14,   4,   1,           {  73, 0 },    0, 123147, {  85324,  85325,  85326,  85327,  85328,  0 } },
  { "White Tiger Vestments"                      ,   14,   2,   2,           {  65, 0 },    0, 123102, {  85344,  85345,  85346,  85347,  85348,  0 } },
  { "White Tiger Vestments"                      ,   14,   4,   2,           {  65, 0 },    0, 123103, {  85344,  85345,  85346,  85347,  85348,  0 } },
  { "White Tiger Plate"                          ,   14,   2,   2,           {  66, 0 },    0, 123104, {  85319,  85320,  85321,  85322,  85323,  0 } },
  { "White Tiger Plate"                          ,   14,   4,   2,           {  66, 0 },    0, 123107, {  85319,  85320,  85321,  85322,  85323,  0 } },
  { "White Tiger Battlegear"                     ,   14,   2,   2,           {  70, 0 },    0, 123108, {  85339,  85340,  85341,  85342,  85343,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "White Tiger Battlegear"                     ,   14,   4,   2,           {  70, 0 },    0,  70762, {  85339,  85340,  85341,  85342,  85343,  0 } },
  { "Yaungol Slayer Battlegear"                  ,   14,   2,   3, { 253, 254, 255, 0 },    0, 123090, {  85294,  85295,  85296,  85297,  85298,  0 } },
  { "Yaungol Slayer Battlegear"                  ,   14,   4,   3, { 253, 254, 255, 0 },    0, 123091, {  85294,  85295,  85296,  85297,  85298,  0 } },
  { "Battlegear of the Thousandfold Blades"      ,   14,   2,   4, { 259, 260, 261, 0 },    0, 123116, {  85299,  85300,  85301,  85302,  85303,  0 } },
  { "Battlegear of the Thousandfold Blades"      ,   14,   4,   4, { 259, 260, 261, 0 },    0, 123122, {  85299,  85300,  85301,  85302,  85303,  0 } },
  { "Vestments of the Guardian Serpent"          ,   14,   2,   5,      { 256, 257, 0 },    0, 123111, {  85359,  85360,  85361,  85362,  85363,  0 } },
  { "Vestments of the Guardian Serpent"          ,   14,   4,   5,      { 256, 257, 0 },    0, 123113, {  85359,  85360,  85361,  85362,  85363,  0 } },
  { "Regalia of the Guardian Serpent"            ,   14,   2,   5,           { 258, 0 },    0, 123114, {  85368,  85367,  85366,  85365,  85364,  0 } },
  { "Regalia of the Guardian Serpent"            ,   14,   4,   5,           { 258, 0 },    0, 123115, {  85368,  85367,  85366,  85365,  85364,  0 } },
  { "Plate of the Lost Catacomb"                 ,   14,   2,   6,           { 250, 0 },    0, 123079, {  85314,  85315,  85316,  85317,  85318,  0 } },
  { "Plate of the Lost Catacomb"                 ,   14,   4,   6,           { 250, 0 },    0, 123080, {  85314,  85315,  85316,  85317,  85318,  0 } },
  { "Battlegear of the Lost Catacomb"            ,   14,   2,   6,      { 251, 252, 0 },    0, 123077, {  85334,  85335,  85336,  85337,  85338,  0 } },
  { "Battlegear of the Lost Catacomb"            ,   14,   4,   6,      { 251, 252, 0 },    0, 123078, {  85334,  85335,  85336,  85337,  85338,  0 } },
  { "Regalia of the Firebird"                    ,   14,   2,   7,           { 262, 0 },    0, 123123, {  85293,  85292,  85291,  85290,  85289,  0 } },
  { "Regalia of the Firebird"                    ,   14,   4,   7,           { 262, 0 },    0, 123124, {  85293,  85292,  85291,  85290,  85289,  0 } },
  { "Battlegear of the Firebird"                 ,   14,   2,   7,           { 263, 0 },    0, 123132, {  85284,  85285,  85286,  85287,  85288,  0 } },
  { "Battlegear of the Firebird"                 ,   14,   4,   7,           { 263, 0 },    0, 123133, {  85284,  85285,  85286,  85287,  85288,  0 } },
  { "Vestments of the Firebird"                  ,   14,   2,   7,           { 264, 0 },    0, 123134, {  85349,  85350,  85351,  85352,  85353,  0 } },
  { "Vestments of the Firebird"                  ,   14,   4,   7,           { 264, 0 },    0, 123135, {  85349,  85350,  85351,  85352,  85353,  0 } },
  { "Regalia of the Burning Scroll"              ,   14,   2,   8, {  62,  63,  64, 0 },    0, 123097, {  85374,  85375,  85376,  85377,  85378,  0 } },
  { "Regalia of the Burning Scroll"              ,   14,   4,   8, {  62,  63,  64, 0 },    0, 123101, {  85374,  85375,  85376,  85377,  85378,  0 } },
  { "Sha-Skin Regalia"                           ,   14,   2,   9, { 265, 266, 267, 0 },    0, 123136, {  85373,  85372,  85371,  85370,  85369,  0 } },
  { "Sha-Skin Regalia"                           ,   14,   4,   9, { 265, 266, 267, 0 },    0, 123141, {  85373,  85372,  85371,  85370,  85369,  0 } },
  { "Armor of the Red Crane"                     ,   14,   2,  10,           { 268, 0 },    0, 123157, {  85384,  85385,  85386,  85387,  85388,  0 } },
  { "Armor of the Red Crane"                     ,   14,   4,  10,           { 268, 0 },    0, 123159, {  85384,  85385,  85386,  85387,  85388,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Battlegear of the Red Crane"                ,   14,   2,  10,           { 269, 0 },    0, 123149, {  85398,  85397,  85396,  85395,  85394,  0 } },
  { "Battlegear of the Red Crane"                ,   14,   4,  10,           { 269, 0 },    0, 123150, {  85398,  85397,  85396,  85395,  85394,  0 } },
  { "Vestments of the Red Crane"                 ,   14,   2,  10,           { 270, 0 },    0, 123152, {  85393,  85392,  85391,  85390,  85389,  0 } },
  { "Vestments of the Red Crane"                 ,   14,   4,  10,           { 270, 0 },    0, 123153, {  85393,  85392,  85391,  85390,  85389,  0 } },
  { "Regalia of the Eternal Blossom"             ,   14,   2,  11,           { 102, 0 },    0, 123082, {  85304,  85305,  85306,  85307,  85308,  0 } },
  { "Regalia of the Eternal Blossom"             ,   14,   4,  11,           { 102, 0 },    0, 123083, {  85304,  85305,  85306,  85307,  85308,  0 } },
  { "Battlegear of the Eternal Blossom"          ,   14,   2,  11,           { 103, 0 },    0, 123084, {  85309,  85310,  85311,  85312,  85313,  0 } },
  { "Battlegear of the Eternal Blossom"          ,   14,   4,  11,           { 103, 0 },    0, 123085, {  85309,  85310,  85311,  85312,  85313,  0 } },
  { "Armor of the Eternal Blossom"               ,   14,   2,  11,           { 104, 0 },    0, 123086, {  85383,  85382,  85381,  85380,  85379,  0 } },
  { "Armor of the Eternal Blossom"               ,   14,   4,  11,           { 104, 0 },    0, 123087, {  85383,  85382,  85381,  85380,  85379,  0 } },
  { "Vestments of the Eternal Blossom"           ,   14,   2,  11,           { 105, 0 },    0, 123088, {  85354,  85355,  85356,  85357,  85358,  0 } },
  { "Vestments of the Eternal Blossom"           ,   14,   4,  11,           { 105, 0 },    0, 123089, {  85354,  85355,  85356,  85357,  85358,  0 } },
  { "Battleplate of the Last Mogu"               ,   15,   2,   1,      {  71,  72, 0 },    0, 138120, {  95330,  95331,  95332,  95333,  95334,  0 } },
  { "Battleplate of the Last Mogu"               ,   15,   4,   1,      {  71,  72, 0 },    0, 138126, {  95330,  95331,  95332,  95333,  95334,  0 } },
  { "Plate of the Last Mogu"                     ,   15,   2,   1,           {  73, 0 },    0, 138280, {  95335,  95336,  95337,  95338,  95339,  0 } },
  { "Plate of the Last Mogu"                     ,   15,   4,   1,           {  73, 0 },    0, 138281, {  95335,  95336,  95337,  95338,  95339,  0 } },
  { "Vestments of the Lightning Emperor"         ,   15,   2,   2,           {  65, 0 },    0, 138291, {  95285,  95286,  95287,  95288,  95289,  0 } },
  { "Vestments of the Lightning Emperor"         ,   15,   4,   2,           {  65, 0 },    0, 138292, {  95285,  95286,  95287,  95288,  95289,  0 } },
  { "Plate of the Lightning Emperor"             ,   15,   2,   2,           {  66, 0 },    0, 138238, {  95290,  95291,  95292,  95293,  95294,  0 } },
  { "Plate of the Lightning Emperor"             ,   15,   4,   2,           {  66, 0 },    0, 138244, {  95290,  95291,  95292,  95293,  95294,  0 } },
  { "Battlegear of the Lightning Emperor"        ,   15,   2,   2,           {  70, 0 },    0, 138159, {  95280,  95281,  95282,  95283,  95284,  0 } },
  { "Battlegear of the Lightning Emperor"        ,   15,   4,   2,           {  70, 0 },    0, 138164, {  95280,  95281,  95282,  95283,  95284,  0 } },
  { "Battlegear of the Saurok Stalker"           ,   15,   2,   3, { 253, 254, 255, 0 },    0, 138365, {  95255,  95256,  95257,  95258,  95259,  0 } },
  { "Battlegear of the Saurok Stalker"           ,   15,   4,   3, { 253, 254, 255, 0 },    0, 138367, {  95255,  95256,  95257,  95258,  95259,  0 } },
  { "Nine-Tail Battlegear"                       ,   15,   2,   4, { 259, 260, 261, 0 },    0, 138148, {  95305,  95306,  95307,  95308,  95309,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Nine-Tail Battlegear"                       ,   15,   4,   4, { 259, 260, 261, 0 },    0, 138150, {  95305,  95306,  95307,  95308,  95309,  0 } },
  { "Vestments of the Exorcist"                  ,   15,   2,   5,      { 256, 257, 0 },    0, 138293, {  95295,  95296,  95297,  95298,  95299,  0 } },
  { "Vestments of the Exorcist"                  ,   15,   4,   5,      { 256, 257, 0 },    0, 138301, {  95295,  95296,  95297,  95298,  95299,  0 } },
  { "Regalia of the Exorcist"                    ,   15,   2,   5,           { 258, 0 },    0, 138156, {  95300,  95301,  95302,  95303,  95304,  0 } },
  { "Regalia of the Exorcist"                    ,   15,   4,   5,           { 258, 0 },    0, 138158, {  95300,  95301,  95302,  95303,  95304,  0 } },
  { "Plate of the All-Consuming Maw"             ,   15,   2,   6,           { 250, 0 },    0, 138195, {  95230,  95231,  95232,  95233,  95234,  0 } },
  { "Plate of the All-Consuming Maw"             ,   15,   4,   6,           { 250, 0 },    0, 138197, {  95230,  95231,  95232,  95233,  95234,  0 } },
  { "Battleplate of the All-Consuming Maw"       ,   15,   2,   6,      { 251, 252, 0 },    0, 138343, {  95225,  95226,  95227,  95228,  95229,  0 } },
  { "Battleplate of the All-Consuming Maw"       ,   15,   4,   6,      { 251, 252, 0 },    0, 138347, {  95225,  95226,  95227,  95228,  95229,  0 } },
  { "Regalia of the Witch Doctor"                ,   15,   2,   7,           { 262, 0 },    0, 138145, {  95320,  95321,  95322,  95323,  95324,  0 } },
  { "Regalia of the Witch Doctor"                ,   15,   4,   7,           { 262, 0 },    0, 138144, {  95320,  95321,  95322,  95323,  95324,  0 } },
  { "Battlegear of the Witch Doctor"             ,   15,   2,   7,           { 263, 0 },    0, 138136, {  95315,  95316,  95317,  95318,  95319,  0 } },
  { "Battlegear of the Witch Doctor"             ,   15,   4,   7,           { 263, 0 },    0, 138141, {  95315,  95316,  95317,  95318,  95319,  0 } },
  { "Vestments of the Witch Doctor"              ,   15,   2,   7,           { 264, 0 },    0, 138303, {  95310,  95311,  95312,  95313,  95314,  0 } },
  { "Vestments of the Witch Doctor"              ,   15,   4,   7,           { 264, 0 },    0, 138305, {  95310,  95311,  95312,  95313,  95314,  0 } },
  { "Regalia of the Chromatic Hydra"             ,   15,   2,   8, {  62,  63,  64, 0 },    0, 138316, {  95260,  95261,  95262,  95263,  95264,  0 } },
  { "Regalia of the Chromatic Hydra"             ,   15,   4,   8, {  62,  63,  64, 0 },    0, 138376, {  95260,  95261,  95262,  95263,  95264,  0 } },
  { "Regalia of the Thousandfold Hells"          ,   15,   2,   9, { 265, 266, 267, 0 },    0, 138129, {  95325,  95326,  95327,  95328,  95329,  0 } },
  { "Regalia of the Thousandfold Hells"          ,   15,   4,   9, { 265, 266, 267, 0 },    0, 138134, {  95325,  95326,  95327,  95328,  95329,  0 } },
  { "Fire-Charm Armor"                           ,   15,   2,  10,           { 268, 0 },    0, 138231, {  95275,  95276,  95277,  95278,  95279,  0 } },
  { "Fire-Charm Armor"                           ,   15,   4,  10,           { 268, 0 },    0, 138236, {  95275,  95276,  95277,  95278,  95279,  0 } },
  { "Fire-Charm Battlegear"                      ,   15,   2,  10,           { 269, 0 },    0, 138177, {  95265,  95266,  95267,  95268,  95269,  0 } },
  { "Fire-Charm Battlegear"                      ,   15,   4,  10,           { 269, 0 },    0, 138315, {  95265,  95266,  95267,  95268,  95269,  0 } },
  { "Fire-Charm Vestments"                       ,   15,   2,  10,           { 270, 0 },    0, 138290, {  95270,  95271,  95272,  95273,  95274,  0 } },
  { "Fire-Charm Vestments"                       ,   15,   4,  10,           { 270, 0 },    0, 138289, {  95270,  95271,  95272,  95273,  95274,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Regalia of the Haunted Forest"              ,   15,   2,  11,           { 102, 0 },    0, 138348, {  95245,  95246,  95247,  95248,  95249,  0 } },
  { "Regalia of the Haunted Forest"              ,   15,   4,  11,           { 102, 0 },    0, 138350, {  95245,  95246,  95247,  95248,  95249,  0 } },
  { "Battlegear of the Haunted Forest"           ,   15,   2,  11,           { 103, 0 },    0, 138352, {  95235,  95236,  95237,  95238,  95239,  0 } },
  { "Battlegear of the Haunted Forest"           ,   15,   4,  11,           { 103, 0 },    0, 138357, {  95235,  95236,  95237,  95238,  95239,  0 } },
  { "Armor of the Haunted Forest"                ,   15,   2,  11,           { 104, 0 },    0, 138216, {  95250,  95251,  95252,  95253,  95254,  0 } },
  { "Armor of the Haunted Forest"                ,   15,   4,  11,           { 104, 0 },    0, 138222, {  95250,  95251,  95252,  95253,  95254,  0 } },
  { "Vestments of the Haunted Forest"            ,   15,   2,  11,           { 105, 0 },    0, 138284, {  95240,  95241,  95242,  95243,  95244,  0 } },
  { "Vestments of the Haunted Forest"            ,   15,   4,  11,           { 105, 0 },    0, 138286, {  95240,  95241,  95242,  95243,  95244,  0 } },
  { "Battleplate of the Prehistoric Marauder"    ,   16,   2,   1,      {  71,  72, 0 },    0, 144436, {  99197,  99198,  99199,  99200,  99206,  0 } },
  { "Battleplate of the Prehistoric Marauder"    ,   16,   4,   1,      {  71,  72, 0 },    0, 144441, {  99197,  99198,  99199,  99200,  99206,  0 } },
  { "Plate of the Prehistoric Marauder"          ,   16,   2,   1,           {  73, 0 },    0, 144503, {  99195,  99196,  99201,  99202,  99203,  0 } },
  { "Plate of the Prehistoric Marauder"          ,   16,   4,   1,           {  73, 0 },    0, 144502, {  99195,  99196,  99201,  99202,  99203,  0 } },
  { "Vestments of Winged Triumph"                ,   16,   2,   2,           {  65, 0 },    0, 144625, {  99133,  99124,  99125,  99134,  99135,  0 } },
  { "Vestments of Winged Triumph"                ,   16,   4,   2,           {  65, 0 },    0, 144613, {  99133,  99124,  99125,  99134,  99135,  0 } },
  { "Plate of Winged Triumph"                    ,   16,   2,   2,           {  66, 0 },    0, 144580, {  99126,  99127,  99128,  99129,  99130,  0 } },
  { "Plate of Winged Triumph"                    ,   16,   4,   2,           {  66, 0 },    0, 144566, {  99126,  99127,  99128,  99129,  99130,  0 } },
  { "Battlegear of Winged Triumph"               ,   16,   2,   2,           {  70, 0 },    0, 144586, {  99132,  99136,  99137,  99138,  99139,  0 } },
  { "Battlegear of Winged Triumph"               ,   16,   4,   2,           {  70, 0 },    0, 144593, {  99132,  99136,  99137,  99138,  99139,  0 } },
  { "Battlegear of the Unblinking Vigil"         ,   16,   2,   3, { 253, 254, 255, 0 },    0, 144637, {  99157,  99158,  99159,  99167,  99168,  0 } },
  { "Battlegear of the Unblinking Vigil"         ,   16,   4,   3, { 253, 254, 255, 0 },    0, 144641, {  99157,  99158,  99159,  99167,  99168,  0 } },
  { "Barbed Assassin Battlegear"                 ,   16,   2,   4, { 259, 260, 261, 0 },    0, 145185, {  99113,  99112,  99114,  99115,  99116,  0 } },
  { "Barbed Assassin Battlegear"                 ,   16,   4,   4, { 259, 260, 261, 0 },    0, 145210, {  99113,  99112,  99114,  99115,  99116,  0 } },
  { "Vestments of Ternion Glory"                 ,   16,   2,   5,      { 256, 257, 0 },    0, 145306, {  99117,  99118,  99119,  99120,  99131,  0 } },
  { "Vestments of Ternion Glory"                 ,   16,   4,   5,      { 256, 257, 0 },    0, 145334, {  99117,  99118,  99119,  99120,  99131,  0 } },
  { "Regalia of Ternion Glory"                   ,   16,   2,   5,           { 258, 0 },    0, 145174, {  99123,  99110,  99111,  99121,  99122,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Regalia of Ternion Glory"                   ,   16,   4,   5,           { 258, 0 },    0, 145179, {  99123,  99110,  99111,  99121,  99122,  0 } },
  { "6.0 test set"                               ,   16,   2,   5,                { 0 },  256, 145306, { 109278, 109379,  0 } },
  { "6.0 test set"                               ,   16,   2,   5,                { 0 },  257, 145174, { 109278, 109379,  0 } },
  { "6.0 test set"                               ,   16,   2,   5,                { 0 },  258, 145251, { 109278, 109379,  0 } },
  { "Plate of Cyclopean Dread"                   ,   16,   2,   6,           { 250, 0 },    0, 144934, {  99179,  99188,  99189,  99190,  99191,  0 } },
  { "Plate of Cyclopean Dread"                   ,   16,   4,   6,           { 250, 0 },    0, 144950, {  99179,  99188,  99189,  99190,  99191,  0 } },
  { "Battleplate of Cyclopean Dread"             ,   16,   2,   6,      { 251, 252, 0 },    0, 144899, {  99186,  99187,  99192,  99193,  99194,  0 } },
  { "Battleplate of Cyclopean Dread"             ,   16,   4,   6,      { 251, 252, 0 },    0, 144907, {  99186,  99187,  99192,  99193,  99194,  0 } },
  { "Celestial Harmony Regalia"                  ,   16,   2,   7,           { 262, 0 },    0, 144998, {  99092,  99093,  99094,  99095,  99106,  0 } },
  { "Celestial Harmony Regalia"                  ,   16,   4,   7,           { 262, 0 },    0, 145003, {  99092,  99093,  99094,  99095,  99106,  0 } },
  { "Celestial Harmony Battlegear"               ,   16,   2,   7,           { 263, 0 },    0, 144962, {  99101,  99102,  99103,  99104,  99105,  0 } },
  { "Celestial Harmony Battlegear"               ,   16,   4,   7,           { 263, 0 },    0, 144966, {  99101,  99102,  99103,  99104,  99105,  0 } },
  { "Celestial Harmony Vestment"                 ,   16,   2,   7,           { 264, 0 },    0, 145378, {  99099,  99100,  99107,  99108,  99109,  0 } },
  { "Celestial Harmony Vestment"                 ,   16,   4,   7,           { 264, 0 },    0, 145380, {  99099,  99100,  99107,  99108,  99109,  0 } },
  { "Chronomancer Regalia"                       ,   16,   2,   8, {  62,  63,  64, 0 },    0, 145251, {  99152,  99153,  99160,  99161,  99162,  0 } },
  { "Chronomancer Regalia"                       ,   16,   4,   8, {  62,  63,  64, 0 },    0, 145257, {  99152,  99153,  99160,  99161,  99162,  0 } },
  { "Regalia of the Horned Nightmare"            ,   16,   2,   9, { 265, 266, 267, 0 },    0, 145072, {  99096,  99097,  99098,  99204,  99205,  0 } },
  { "Regalia of the Horned Nightmare"            ,   16,   4,   9, { 265, 266, 267, 0 },    0, 145091, {  99096,  99097,  99098,  99204,  99205,  0 } },
  { "Armor of Seven Sacred Seals"                ,   16,   2,  10,           { 268, 0 },    0, 145049, {  99142,  99143,  99144,  99140,  99141,  0 } },
  { "Armor of Seven Sacred Seals"                ,   16,   4,  10,           { 268, 0 },    0, 145055, {  99142,  99143,  99144,  99140,  99141,  0 } },
  { "Battlegear of Seven Sacred Seals"           ,   16,   2,  10,           { 269, 0 },    0, 145004, {  99154,  99155,  99156,  99145,  99146,  0 } },
  { "Battlegear of Seven Sacred Seals"           ,   16,   4,  10,           { 269, 0 },    0, 145022, {  99154,  99155,  99156,  99145,  99146,  0 } },
  { "Vestments of Seven Sacred Seals"            ,   16,   2,  10,           { 270, 0 },    0, 145439, {  99151,  99147,  99148,  99149,  99150,  0 } },
  { "Vestments of Seven Sacred Seals"            ,   16,   4,  10,           { 270, 0 },    0, 145449, {  99151,  99147,  99148,  99149,  99150,  0 } },
  { "Regalia of the Shattered Vale"              ,   16,   2,  11,           { 102, 0 },    0, 144767, {  99169,  99174,  99175,  99176,  99177,  0 } },
  //                                    Tier name, Tier, Bns, Cls,         Derived Spec, Spec,  Spell, Items
  { "Regalia of the Shattered Vale"              ,   16,   4,  11,           { 102, 0 },    0, 144756, {  99169,  99174,  99175,  99176,  99177,  0 } },
  { "Battlegear of the Shattered Vale"           ,   16,   2,  11,           { 103, 0 },    0, 144864, {  99180,  99181,  99182,  99183,  99184,  0 } },
  { "Battlegear of the Shattered Vale"           ,   16,   4,  11,           { 103, 0 },    0, 144841, {  99180,  99181,  99182,  99183,  99184,  0 } },
  { "Armor of the Shattered Vale"                ,   16,   2,  11,           { 104, 0 },    0, 144879, {  99163,  99164,  99165,  99166,  99170,  0 } },
  { "Armor of the Shattered Vale"                ,   16,   4,  11,           { 104, 0 },    0, 144887, {  99163,  99164,  99165,  99166,  99170,  0 } },
  { "Vestments of the Shattered Vale"            ,   16,   2,  11,           { 105, 0 },    0, 144869, {  99171,  99172,  99173,  99178,  99185,  0 } },
  { "Vestments of the Shattered Vale"            ,   16,   4,  11,           { 105, 0 },    0, 144875, {  99171,  99172,  99173,  99178,  99185,  0 } },
};

#define PERK_SIZE (9)

// Perk specialization abilities, wow build 18546
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
    // Arms
    {
      157446, // 0: Improved Charge
      157449, // 1: Improved Heroic Leap
      157452, // 2: Improved Die by the Sword
      157454, // 3: Empowered Execute
      157457, // 4: Improved Colossus Smash
      157461, // 5: Enhanced Sweeping Strikes
      157464, // 6: Improved Mortal Strike
      157466, // 7: Improved Thunderclap
      157472, // 8: Improved Overpower
    },
    // Fury
    {
      157452, // 0: Improved Die by the Sword
      157454, // 1: Empowered Execute
      157457, // 2: Improved Colossus Smash
      157473, // 3: Improved Meat Cleaver
      157474, // 4: Improved Wild Strike
      157475, // 5: Improved Bloodthirst
      157477, // 6: Improved Raging Blow
      157446, // 7: Improved Charge
      157449, // 8: Improved Heroic Leap
    },
    // Protection
    {
      157446, // 0: Improved Charge
      157449, // 1: Improved Heroic Leap
      157497, // 2: Improved Bastion of Defense
      157487, // 3: Improved Revenge
      157479, // 4: Improved Heroic Throw
      157492, // 5: Improved Shield Barrier
      157481, // 6: Improved Shield Slam
      157498, // 7: Improved Thunder Clap
      157494, // 8: Improved Unwavering Sentinel
    },
    {
           0,
    },
  },
  {
    // Holy
    {
      157453, // 0: Empowered Beacon of Light
      157476, // 1: Empowered Holy Shock
      157478, // 2: Enhanced Holy Shock
      157455, // 3: Improved Daybreak
      157462, // 4: Improved Denounce
      157458, // 5: Improved Holy Light
      157482, // 6: Improved Forbearance
      157480, // 7: Improved Divine Protection
      157460, // 8: Improved Flash of Light
    },
    // Protection
    {
      157482, // 0: Improved Forbearance
      157480, // 1: Improved Divine Protection
      157485, // 2: Empowered Avenger's Shield
      157488, // 3: Improved Block
      157486, // 4: Improved Consecration
      157483, // 5: Improved Hammer of the Righteous
      157484, // 6: Improved Word of Glory
      157489, // 7: Improved Crusader Strike
      157490, // 8: Improved Judgment
    },
    // Retribution
    {
      157482, // 0: Improved Forbearance
      157480, // 1: Improved Divine Protection
      157460, // 2: Improved Flash of Light
      157489, // 3: Improved Crusader Strike
      157490, // 4: Improved Judgment
      157491, // 5: Empowered Seal of Truth
      157493, // 6: Enhanced Hand of Sacrifice
      157495, // 7: Improved Exorcism
      157496, // 8: Empowered Hammer of Wrath
    },
    {
           0,
    },
  },
  {
    // Beast Mastery
    {
      157713, // 0: Empowered Pets
      157715, // 1: Enhanced Basic Attacks
      157714, // 2: Improved Beast Cleave
      157705, // 3: Improved Focus Fire
      157709, // 4: Improved Kill Command
      157707, // 5: Enhanced Kill Shot
      157718, // 6: Enhanced Camouflage
      157711, // 7: Improved Arcane Shot
      157712, // 8: Improved Cobra Shot
    },
    // Marksmanship
    {
      157707, // 0: Enhanced Kill Shot
      157718, // 1: Enhanced Camouflage
      157724, // 2: Enhanced Aimed Shot
      157719, // 3: Enhanced Chimera Shot
      157733, // 4: Improved Chimera Shot
      157732, // 5: Enhanced Shots
      157728, // 6: Improved Aimed Shot
      157726, // 7: Improved Focus
      157729, // 8: Improved Steady Shot
    },
    // Survival
    {
      157718, // 0: Enhanced Camouflage
      157711, // 1: Improved Arcane Shot
      157712, // 2: Improved Cobra Shot
      157748, // 3: Empowered Explosive Shot
      157752, // 4: Enhanced Entrapment
      157751, // 5: Enhanced Traps
      157749, // 6: Improved Black Arrow
      157753, // 7: Improved Camouflage
      157768, // 8: Improved Viper Venom
    },
    {
           0,
    },
  },
  {
    // Assassination
    {
      157513, // 0: Improved Slice and Dice
      157514, // 1: Enhanced Vendetta
      157515, // 2: Improved Venomous Wounds
      157560, // 3: Enhanced Poisons
      157561, // 4: Enhanced Crimson Tempest
      157569, // 5: Empowered Envenom
      157570, // 6: Improved Rupture
      157572, // 7: Enhanced Envenom
      157576, // 8: Improved Recuperate
    },
    // Combat
    {
      157576, // 0: Improved Recuperate
      157581, // 1: Empowered Bandit's Guile
      157605, // 2: Instant Poison
      157613, // 3: Enhanced Adrenaline Rush
      157615, // 4: Enhanced Blade Flurry
      157645, // 5: Empowered Crimson Tempest
      157660, // 6: Improved Dual Wield
      157661, // 7: Improved Sinister Strike
      157662, // 8: Improved Eviscerate
    },
    // Subtlety
    {
      157576, // 0: Improved Recuperate
      157663, // 1: Improved Backstab
      157664, // 2: Improved Ambush
      157665, // 3: Improved Hemorrhage
      157666, // 4: Enhanced Vanish
      157667, // 5: Enhanced Eviscerate
      157669, // 6: Enhanced Shadow Dance
      157671, // 7: Empowered Fan of Knives
      157704, // 8: Enhanced Master of Subtlety
    },
    {
           0,
    },
  },
  {
    // Discipline
    {
      157181, // 0: Improved Heal
      157197, // 1: Enhanced Strength of Soul
      157182, // 2: Improved Penance
      157193, // 3: Enhanced Focused Will
      157180, // 4: Enhanced Holy Fire
      157145, // 5: Enhanced Leap of Faith
      157156, // 6: Improved Smite
      157129, // 7: Enhanced Power Word: Shield
      157144, // 8: Improved Flash Heal
    },
    // Holy
    {
      157129, // 0: Enhanced Power Word: Shield
      157144, // 1: Improved Flash Heal
      157193, // 2: Enhanced Focused Will
      157180, // 3: Enhanced Holy Fire
      157145, // 4: Enhanced Leap of Faith
      157156, // 5: Improved Smite
      157206, // 6: Enhanced Chakras
      157181, // 7: Improved Heal
      157207, // 8: Enhanced Renew
    },
    // Shadow
    {
      157129, // 0: Enhanced Power Word: Shield
      157144, // 1: Improved Flash Heal
      157221, // 2: Empowered Psychic Horror
      157223, // 3: Enhanced Mind Flay
      157217, // 4: Enhanced Shadow Orbs
      157218, // 5: Enhanced Shadow Word: Death
      157222, // 6: Improved Shadow Word: Pain
      157219, // 7: Improved Vampiric Touch
      157220, // 8: Improved Mind Spike
    },
    {
           0,
    },
  },
  {
    // Blood
    {
      157338, // 0: Enhanced Bone Shield
      157337, // 1: Enhanced Death Grip
      157343, // 2: Enhanced Death Coil
      157336, // 3: Enhanced Rune Tap
      157334, // 4: Enhanced Will of the Necropolis
      157341, // 5: Improved Death Strike
      157339, // 6: Improved Pestilence
      157340, // 7: Improved Diseases
      157342, // 8: Improved Soul Reaper
    },
    // Frost
    {
      157340, // 0: Improved Diseases
      157342, // 1: Improved Soul Reaper
      157403, // 2: Empowered Icebound Fortitude
      157389, // 3: Empowered Pillar of Frost
      157409, // 4: Empowered Rime
      157380, // 5: Improved Mind Freeze
      157376, // 6: Improved Runeforges
      157367, // 7: Improved Death Grip
      157408, // 8: Enhanced Death Strike
    },
    // Unholy
    {
      157340, // 0: Improved Diseases
      157342, // 1: Improved Soul Reaper
      157408, // 2: Enhanced Death Strike
      157441, // 3: Empowered Gargoyle
      157412, // 4: Enhanced Dark Transformation
      157439, // 5: Improved Death Coil
      157437, // 6: Enhanced Fallen Crusader
      157414, // 7: Improved Festering Strike
      157413, // 8: Improved Scourge Strike
    },
    {
           0,
    },
  },
  {
    // Elemental
    {
      157765, // 0: Improved Chain Lightning
      157774, // 1: Improved Lightning Shield
      157764, // 2: Improved Reincarnation
      157775, // 3: Improved Fire Totems
      157776, // 4: Improved Critical Strikes
      157777, // 5: Improved Lightning Bolt
      157778, // 6: Improved Lava Burst
      157779, // 7: Improved Shocks
      157784, // 8: Enhanced Unleash Elements
    },
    // Enhancement
    {
      157764, // 0: Improved Reincarnation
      157775, // 1: Improved Fire Totems
      157784, // 2: Enhanced Unleash Elements
      157803, // 3: Improved Lava Lash
      157804, // 4: Improved Flame Shock
      157807, // 5: Improved Maelstrom Weapon
      157808, // 6: Improved Stormstrike
      157809, // 7: Improved Lava Lash
      157810, // 8: Improved Feral Spirit
    },
    // Restoration
    {
      157764, // 0: Improved Reincarnation
      157777, // 1: Improved Lightning Bolt
      157812, // 2: Improved Riptide
      157813, // 3: Improved Chain Heal
      157814, // 4: Improved Healing Wave
      157816, // 5: Improved Healing Surge
      157818, // 6: Improved Tidal Waves
      157830, // 7: Improved Healing Rain
      157784, // 8: Enhanced Unleash Elements
    },
    {
           0,
    },
  },
  {
    // Arcane
    {
      157532, // 0: Improved Arcane Barrage
      157616, // 1: Improved Arcane Blast
      157619, // 2: Improved Arcane Explosion
      157599, // 3: Enhanced Arcane Missiles
      157617, // 4: Improved Arcane Missiles
      157604, // 5: Improved Arcane Power
      157606, // 6: Improved Blink
      157595, // 7: Enhanced Arcane Blast
      157614, // 8: Improved Evocation
    },
    // Fire
    {
      157630, // 0: Improved Combustion
      157640, // 1: Improved Dragon's Breath
      157642, // 2: Enhanced Pyrotechnics
      157622, // 3: Enhanced Inferno Blast
      157637, // 4: Improved Fireball and Frostfire Bolt
      157621, // 5: Improved Flamestrike
      157629, // 6: Improved Inferno Blast
      157624, // 7: Improved Pyroblast
      157632, // 8: Improved Scorch
    },
    // Frost
    {
      157727, // 0: Improved Blizzard
      157646, // 1: Enhanced Frostbolt
      157668, // 2: Enhanced Fingers of Frost
      157670, // 3: Improved Frost Nova
      157723, // 4: Improved Frostbolt
      157770, // 5: Improved Frostfire Bolt
      157725, // 6: Improved Ice Lance
      157721, // 7: Improved Icy Veins
      157738, // 8: Improved Water Elemental
    },
    {
           0,
    },
  },
  {
    // Affliction
    {
      157070, // 0: Improved Life Tap
      157071, // 1: Empowered Agony
      157072, // 2: Enhanced Haunt
      157073, // 3: Improved Corruption
      157076, // 4: Improved Unstable Affliction
      157077, // 5: Improved Drain Soul
      157078, // 6: Enhanced Nightfall
      157068, // 7: Improved Demons
      157069, // 8: Empowered Drain Life
    },
    // Demonology
    {
      157068, // 0: Improved Demons
      157069, // 1: Empowered Drain Life
      157095, // 2: Improved Molten Core
      157096, // 3: Empowered Demons
      157097, // 4: Improved Shadow Bolt
      157098, // 5: Enhanced Corruption
      157100, // 6: Empowered Doom
      157101, // 7: Enhanced Hand of Gul'dan
      157102, // 8: Improved Touch of Chaos
    },
    // Destruction
    {
      157103, // 0: Enhanced Chaos Bolt
      157114, // 1: Empowered Immolate
      157118, // 2: Improved Conflagrate
      157120, // 3: Empowered Incinerate
      157121, // 4: Improved Ember Tap
      157125, // 5: Improved Shadowburn
      157126, // 6: Enhanced Backlash
      157068, // 7: Improved Demons
      158362, // 8: Enhanced Drain Life
    },
    {
           0,
    },
  },
  {
    // Brewmaster
    {
      157365, // 0: Improved Dizzying Haze
      157378, // 1: Improved Elusive Brew
      157362, // 2: Improved Breath of Fire
      157363, // 3: Improved Guard
      157377, // 4: Empowered Keg Smash
      157381, // 5: Improved Stance of the Sturdy Ox
      157374, // 6: Empowered Blackout Kick
      157361, // 7: Enhanced Roll
      157366, // 8: Enhanced Transcendence
    },
    // Mistweaver
    {
      157400, // 0: Empowered Surging Mist
      157392, // 1: Improved Expel Harm
      157401, // 2: Improved Life Cocoon
      157398, // 3: Improved Renewing Mist
      157399, // 4: Empowered Tiger Palm
      157374, // 5: Empowered Blackout Kick
      157361, // 6: Enhanced Roll
      157366, // 7: Enhanced Transcendence
      157771, // 8: Improved Soothing Mist
    },
    // Windwalker
    {
      157399, // 0: Empowered Tiger Palm
      157411, // 1: Empowered Chi
      157415, // 2: Empowered Spinning Crane Kick
      157404, // 3: Improved Energizing Brew
      157427, // 4: Empowered Fists of Fury
      157426, // 5: Empowered Rising Sun Kick
      157374, // 6: Empowered Blackout Kick
      157361, // 7: Enhanced Roll
      157366, // 8: Enhanced Transcendence
    },
    {
           0,
    },
  },
  {
    // Balance
    {
      157233, // 0: Empowered Starfall
      157229, // 1: Enhanced Owlkin Frenzy
      157227, // 2: Enhanced Moonkin Form
      157225, // 3: Improved Nature's Swiftness
      157232, // 4: Enhanced Starsurge
      157226, // 5: Enhanced Storms
      157230, // 6: Improved Starfire
      157235, // 7: Improved Moonfire
      157231, // 8: Improved Wrath
    },
    // Feral
    {
      157269, // 0: Enhanced Berserk
      157272, // 1: Enhanced Cat Form
      157274, // 2: Enhanced Prowl
      157280, // 3: Enhanced Rejuvenation
      157273, // 4: Enhanced Tiger's Fury
      157277, // 5: Improved Ferocious Bite
      157276, // 6: Improved Rake
      157275, // 7: Improved Shred
      157279, // 8: Improved Healing Touch
    },
    // Guardian
    {
      157292, // 0: Empowered Bear Form
      157284, // 1: Empowered Berserk
      157290, // 2: Empowered Thrash
      157286, // 3: Enhanced Bear Hug
      157283, // 4: Enhanced Tooth and Claw
      157296, // 5: Improved Barkskin
      157298, // 6: Improved Frenzied Regeneration
      157287, // 7: Improved Mangle
      157288, // 8: Improved Maul
    },
    // Restoration
    {
      157235, // 0: Improved Moonfire
      157231, // 1: Improved Wrath
      157279, // 2: Improved Healing Touch
      157305, // 3: Empowered Ironbark
      157302, // 4: Empowered Regrowth
      157300, // 5: Empowered Rejuvenation
      157312, // 6: Enhanced Lifebloom
      157301, // 7: Enhanced Rebirth
      157309, // 8: Improved Living Seed
    },
  },
};

// Glyph properties, wow build 18546
static glyph_property_data_t __glyph_property_data[521] = {
  {   162,  54811 },
  {   163,  54812 },
  {   164, 114338 },
  {   166, 116186 },
  {   167,  54821 },
  {   170,  54733 },
  {   171, 116218 },
  {   172,  17076 },
  {   173,  54825 },
  {   175, 146655 },
  {   176, 114333 },
  {   177,  54831 },
  {   178, 116216 },
  {   179, 116203 },
  {   180, 127540 },
  {   181,  54760 },
  {   183,  54922 },
  {   184, 146955 },
  {   185,  54923 },
  {   186,  54924 },
  {   187,  54926 },
  {   188,  54927 },
  {   189,  54928 },
  {   190,  89401 },
  {   191,  54930 },
  {   192,  54931 },
  {   194,  54935 },
  {   195,  54936 },
  {   196,  54937 },
  {   197,  54938 },
  {   198,  54939 },
  {   199,  54940 },
  {   200,  54943 },
  {   211,  55436 },
  {   212,  55437 },
  {   213,  55449 },
  {   214,  55454 },
  {   215,  55442 },
  {   216,  55439 },
  {   217,  55455 },
  {   218,  55450 },
  {   219,  55447 },
  {   220,  55451 },
  {   221,  55443 },
  {   222,  55456 },
  {   223,  55440 },
  {   224,  55438 },
  {   225,  55448 },
  {   226,  55453 },
  {   227,  55441 },
  {   228,  55446 },
  {   229,  55444 },
  {   230,  55452 },
  {   231,  55445 },
  {   251,  55675 },
  {   252,  55677 },
  {   253,  55684 },
  {   254,  55678 },
  {   255,  14771 },
  {   256, 125045 },
  {   257,  55686 },
  {   258,  55673 },
  {   259,  55691 },
  {   260,  55688 },
  {   261, 119853 },
  {   262,  89489 },
  {   263,  55672 },
  {   264, 119873 },
  {   265,  55676 },
  {   266, 119872 },
  {   267,  55690 },
  {   268,  87195 },
  {   269, 119864 },
  {   270,  55692 },
  {   271,  55685 },
  {   272,  56248 },
  {   273,  56235 },
  {   274,  56218 },
  {   275,  56241 },
  {   276,  56232 },
  {   277,  56244 },
  {   278,  56246 },
  {   279,  56249 },
  {   280,  56238 },
  {   281,  56224 },
  {   282,  56217 },
  {   284,  56242 },
  {   285,  56226 },
  {   286,  56240 },
  {   289,  56231 },
  {   290,  56250 },
  {   291,  56233 },
  {   292,  56247 },
  {   312,  56363 },
  {   313, 115718 },
  {   314,  56365 },
  {   316,  56368 },
  {   318,  56376 },
  {   320,  56384 },
  {   321, 115723 },
  {   322,  56377 },
  {   323, 115705 },
  {   325, 146659 },
  {   328,  56382 },
  {   329,  56375 },
  {   330,  56364 },
  {   331, 115713 },
  {   351, 122492 },
  {   352, 119449 },
  {   353, 132106 },
  {   354,  56833 },
  {   355, 123632 },
  {   356, 119410 },
  {   357,  56850 },
  {   358,  56844 },
  {   359,  56845 },
  {   360,  56847 },
  {   362, 119403 },
  {   363,  20895 },
  {   364,  53299 },
  {   365,  19560 },
  {   366, 119464 },
  {   368, 126095 },
  {   369,  19573 },
  {   371, 119407 },
  {   391,  56808 },
  {   392,  56813 },
  {   393,  56800 },
  {   394,  56818 },
  {   395, 146628 },
  {   396,  56806 },
  {   397,  56799 },
  {   398, 146625 },
  {   399,  56803 },
  {   400,  56804 },
  {   401,  56812 },
  {   402, 125044 },
  {   403,  56809 },
  {   404,  56807 },
  {   405,  56819 },
  {   406,  56801 },
  {   407, 146631 },
  {   409, 146629 },
  {   410,  56810 },
  {   411,  56811 },
  {   431,  57856 },
  {   432, 107059 },
  {   433,  57855 },
  {   434, 121840 },
  {   435, 114295 },
  {   439,  57866 },
  {   440,  57870 },
  {   441,  57903 },
  {   442,  57902 },
  {   443,  57904 },
  {   445,  57924 },
  {   447,  57927 },
  {   448, 126748 },
  {   449,  58136 },
  {   450,  52648 },
  {   451,  57925 },
  {   452, 125043 },
  {   453,  57958 },
  {   454,  57979 },
  {   455,  57955 },
  {   456,  57947 },
  {   457,  57954 },
  {   458,  57985 },
  {   459, 108939 },
  {   460,  58009 },
  {   461,  57986 },
  {   462,  33202 },
  {   463,  58228 },
  {   464,  58032 },
  {   465,  58027 },
  {   466,  58017 },
  {   467,  58033 },
  {   468,  58039 },
  {   469,  58038 },
  {   470,  58058 },
  {   471,  58135 },
  {   473,  58059 },
  {   476,  58057 },
  {   477,  58079 },
  {   478,  58070 },
  {   479,  58081 },
  {   480,  58080 },
  {   481,  58107 },
  {   482,  58094 },
  {   483,  58095 },
  {   484,  58096 },
  {   485,  58097 },
  {   486,  58099 },
  {   487,  58098 },
  {   488,  58104 },
  {   489,  58368 },
  {   490,  58369 },
  {   491,  58355 },
  {   492,  58366 },
  {   493,  58388 },
  {   494,  58367 },
  {   495,  58372 },
  {   496,  58357 },
  {   497,  58377 },
  {   499,  58386 },
  {   500,  58385 },
  {   501,  58364 },
  {   502,  58375 },
  {   504,  58387 },
  {   505,  58384 },
  {   507,  58356 },
  {   508,  58382 },
  {   509,  58370 },
  {   512,  58623 },
  {   513,  58616 },
  {   514,  58640 },
  {   515,  58673 },
  {   516,  58620 },
  {   518,  58677 },
  {   519,  62259 },
  {   520,  58629 },
  {   521,  58647 },
  {   522,  58680 },
  {   524,  58631 },
  {   525,  58671 },
  {   526,  58657 },
  {   527,  58686 },
  {   529,  58642 },
  {   530,  58618 },
  {   531,  58635 },
  {   532,  58676 },
  {   551,  59219 },
  {   552,  59289 },
  {   553,  59309 },
  {   554,  59307 },
  {   555,  60200 },
  {   556,  59327 },
  {   557,  59332 },
  {   558,  59336 },
  {   559,  56420 },
  {   560,  56414 },
  {   561,  56416 },
  {   591,  61205 },
  {   611, 115700 },
  {   612,  62132 },
  {   613, 114301 },
  {   631, 116172 },
  {   651,  62210 },
  {   671, 114223 },
  {   672,  62970 },
  {   673, 116238 },
  {   674, 114222 },
  {   675,  48514 },
  {   676,  63057 },
  {   677, 119447 },
  {   691, 119384 },
  {   692, 119462 },
  {   693,  63068 },
  {   694,  63069 },
  {   695,  83495 },
  {   696,  63090 },
  {   697,  86209 },
  {   698,  63092 },
  {   700, 115710 },
  {   701,  63218 },
  {   702,  63219 },
  {   703,  63220 },
  {   704,  63222 },
  {   705,  63223 },
  {   706,  63224 },
  {   707,  63225 },
  {   708,  63229 },
  {   709, 119850 },
  {   710, 119866 },
  {   711, 147778 },
  {   712,  33371 },
  {   713,  63248 },
  {   714,  63249 },
  {   715,  63252 },
  {   716,  63253 },
  {   731,  63254 },
  {   733,  63268 },
  {   734,  63269 },
  {   735,  63270 },
  {   736,  63271 },
  {   737,  63273 },
  {   751,  63279 },
  {   752,  63280 },
  {   753,  63291 },
  {   754,  63298 },
  {   755,  63302 },
  {   757,  63304 },
  {   758,  63309 },
  {   759, 146964 },
  {   760,  63320 },
  {   761,  63312 },
  {   762,  63324 },
  {   763,  63325 },
  {   764, 146965 },
  {   765,  63327 },
  {   766,  63328 },
  {   767,  63329 },
  {   768,  63330 },
  {   769,  63331 },
  {   771,  63333 },
  {   773,  63335 },
  {   831,  67598 },
  {   851,  68164 },
  {   871, 115703 },
  {   911, 148683 },
  {   920,  57927 },
  {   923,  89749 },
  {   924,  89758 },
  {   926,  56805 },
  {   927,  89003 },
  {   928,  89926 },
  {   929,  91299 },
  {   930,  93466 },
  {   931,  94372 },
  {   932,  94374 },
  {   933, 114237 },
  {   934,  94386 },
  {   935, 114280 },
  {   936,  47180 },
  {   937, 125047 },
  {   945,  96279 },
  {   948,  98397 },
  {   950, 101052 },
  {   961, 107906 },
  {   967, 111546 },
  {   970, 112104 },
  {   986, 115738 },
  {   987, 115931 },
  {   988, 115933 },
  {   989, 115934 },
  {   990, 115943 },
  {   991, 115946 },
  {   996, 119477 },
  {   997, 146951 },
  {   998, 123401 },
  {   999, 120477 },
  {  1000, 120479 },
  {  1001, 120483 },
  {  1002, 120482 },
  {  1003, 120585 },
  {  1004, 120581 },
  {  1005, 120583 },
  {  1006, 120584 },
  {  1007, 120581 },
  {  1008, 122013 },
  {  1009, 122028 },
  {  1011, 123023 },
  {  1012, 120479 },
  {  1013, 123334 },
  {  1014, 123391 },
  {  1015, 123394 },
  {  1017, 123403 },
  {  1019, 123763 },
  {  1020, 123779 },
  {  1021, 125967 },
  {  1022, 125042 },
  {  1023, 109263 },
  {  1024,  58136 },
  {  1025, 146953 },
  {  1026, 124989 },
  {  1028, 125151 },
  {  1029, 125154 },
  {  1030, 124997 },
  {  1031, 146954 },
  {  1032, 146952 },
  {  1033, 125660 },
  {  1034, 146950 },
  {  1035, 125671 },
  {  1036, 125673 },
  {  1038, 125678 },
  {  1039, 125732 },
  {  1040, 125755 },
  {  1041, 125872 },
  {  1042, 125893 },
  {  1043, 125872 },
  {  1044, 125901 },
  {  1045, 125931 },
  {  1046, 126133 },
  {  1047, 126152 },
  {  1048, 126174 },
  {  1049, 126094 },
  {  1050, 126179 },
  {  1052, 126745 },
  {  1053, 126746 },
  {  1054, 131113 },
  {  1055, 132005 },
  {  1058, 135557 },
  {  1060, 145722 },
  {  1061, 147707 },
  {  1062, 145529 },
  {  1063, 146645 },
  {  1064, 146646 },
  {  1065, 146648 },
  {  1066, 146650 },
  {  1067, 146956 },
  {  1068, 146957 },
  {  1069, 147762 },
  {  1070, 147781 },
  {  1071, 146962 },
  {  1072, 146970 },
  {  1073, 146971 },
  {  1074, 146652 },
  {  1075, 146653 },
  {  1076, 146654 },
  {  1077, 146656 },
  {  1078, 146656 },
  {  1079, 146976 },
  {  1080, 146662 },
  {  1081, 147353 },
  {  1082, 146958 },
  {  1083, 146959 },
  {  1084, 145722 },
  {  1085, 147776 },
  {  1086, 147779 },
  {  1087, 147072 },
  {  1088, 146960 },
  {  1089, 146961 },
  {  1090, 147783 },
  {  1091, 147784 },
  {  1092, 147770 },
  {  1093, 147772 },
  {  1094, 147785 },
  {  1095, 147788 },
  {  1096, 147787 },
  {  1097, 147707 },
  {  1098, 146968 },
  {  1099, 146969 },
  {  1100, 146973 },
  {  1101, 146974 },
  {  1102, 146657 },
  {  1104, 148475 },
  {  1109, 159415 },
  {  1110, 159416 },
  {  1111, 159418 },
  {  1112, 159421 },
  {  1113, 159422 },
  {  1114, 159427 },
  {  1115, 159428 },
  {  1116, 159429 },
  {  1117, 159432 },
  {  1118, 159434 },
  {  1119, 159436 },
  {  1120, 159441 },
  {  1121, 159444 },
  {  1122, 159445 },
  {  1123, 159448 },
  {  1124, 159450 },
  {  1125, 159453 },
  {  1126, 159455 },
  {  1127, 159456 },
  {  1128, 159459 },
  {  1129, 159467 },
  {  1130, 159469 },
  {  1131, 159470 },
  {  1132, 159484 },
  {  1133, 159485 },
  {  1134, 159486 },
  {  1135, 159487 },
  {  1136, 159490 },
  {  1137, 159492 },
  {  1138, 159495 },
  {  1139, 159496 },
  {  1140, 159497 },
  {  1141, 159534 },
  {  1142, 159536 },
  {  1143, 159545 },
  {  1144, 159548 },
  {  1145, 159557 },
  {  1146, 159572 },
  {  1147, 159573 },
  {  1148, 159579 },
  {  1149, 159592 },
  {  1150, 159596 },
  {  1151, 159598 },
  {  1152, 159599 },
  {  1153, 159606 },
  {  1154, 159608 },
  {  1155, 159624 },
  {  1156, 159626 },
  {  1157, 159627 },
  {  1158, 159628 },
  {  1159, 159634 },
  {  1160, 159635 },
  {  1161, 159636 },
  {  1162, 159638 },
  {  1163, 159640 },
  {  1164, 159642 },
  {  1165, 159643 },
  {  1166, 159644 },
  {  1167, 159647 },
  {  1168, 159648 },
  {  1169, 159649 },
  {  1170, 159650 },
  {  1171, 159651 },
  {  1172, 159653 },
  {  1173, 159665 },
  {  1174, 159667 },
  {  1175, 159669 },
  {  1176, 159680 },
  {  1177, 159688 },
  {  1178, 159691 },
  {  1179, 159693 },
  {  1180, 159697 },
  {  1181, 159701 },
  {  1182, 159703 },
  {  1183, 159708 },
  {  1184, 159738 },
  {  1185, 159740 },
  {  1186, 159754 },
  {  1187, 159759 },
  {  1188, 159761 },
  {  1201, 161654 },
  {  1202, 162532 },
  {  1203, 162604 },
  {  1204,  58669 },
  {     0,      0 }
};

