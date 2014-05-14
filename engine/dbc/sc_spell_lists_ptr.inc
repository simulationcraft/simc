#define PTR_CLASS_ABILITY_SIZE (53)
#define PTR_CLASS_ABILITY_TREE_SIZE (6)

#ifndef ptr_MAX_CLASS
#define ptr_MAX_CLASS (12)
#endif
// Class based active abilities, wow build 18224
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
    // General tree, 30 abilities
    {
        6673, // Battle Shout
        2457, // Battle Stance
       18499, // Berserker Rage
        2458, // Berserker Stance
         100, // Charge
         845, // Cleave
         469, // Commanding Shout
          71, // Defensive Stance
      114203, // Demoralizing Banner
         676, // Disarm
        5308, // Execute
        1715, // Hamstring
        6544, // Heroic Leap
          78, // Heroic Strike
       57755, // Heroic Throw
        3411, // Intervene
        5246, // Intimidating Shout
      114192, // Mocking Banner
        6552, // Pummel
       97462, // Rallying Cry
        1719, // Recklessness
       64382, // Shattering Throw
         871, // Shield Wall
      114207, // Skull Banner
       23920, // Spell Reflection
        7386, // Sunder Armor
         355, // Taunt
      122475, // Throw
        6343, // Thunder Clap
       34428, // Victory Rush
           0,
    },
    // Arms tree, 7 abilities
    {
       86346, // Colossus Smash
      118038, // Die by the Sword
       12294, // Mortal Strike
        7384, // Overpower
        1464, // Slam
       12328, // Sweeping Strikes
        1680, // Whirlwind
           0,
    },
    // Fury tree, 6 abilities
    {
       23881, // Bloodthirst
       86346, // Colossus Smash
      118038, // Die by the Sword
       85288, // Raging Blow
        1680, // Whirlwind
      100130, // Wild Strike
           0,
    },
    // Protection tree, 7 abilities
    {
        1160, // Demoralizing Shout
       20243, // Devastate
       12975, // Last Stand
        6572, // Revenge
      112048, // Shield Barrier
        2565, // Shield Block
       23922, // Shield Slam
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
    // General tree, 29 abilities
    {
       31884, // Avenging Wrath
       20217, // Blessing of Kings
       19740, // Blessing of Might
      115750, // Blinding Light
        4987, // Cleanse
       35395, // Crusader Strike
       31821, // Devotion Aura
         498, // Divine Protection
         642, // Divine Shield
       19750, // Flash of Light
         853, // Hammer of Justice
       24275, // Hammer of Wrath
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
       20154, // Seal of Righteousness
       31801, // Seal of Truth
       10326, // Turn Evil
       85673, // Word of Glory
      122287, // Wrath (Symbiosis)
           0,
    },
    // Holy tree, 10 abilities
    {
       53563, // Beacon of Light
        2812, // Denounce
       31842, // Divine Favor
       82326, // Divine Light
       54428, // Divine Plea
       86669, // Guardian of Ancient Kings
         635, // Holy Light
       82327, // Holy Radiance
       20473, // Holy Shock
       85222, // Light of Dawn
           0,
    },
    // Protection tree, 7 abilities
    {
       31850, // Ardent Defender
       31935, // Avenger's Shield
       26573, // Consecration
       86659, // Guardian of Ancient Kings
       53595, // Hammer of the Righteous
      119072, // Holy Wrath
       53600, // Shield of the Righteous
           0,
    },
    // Retribution tree, 8 abilities
    {
       53385, // Divine Storm
      121783, // Emancipate
         879, // Exorcism
       86698, // Guardian of Ancient Kings
       53595, // Hammer of the Righteous
       84963, // Inquisition
       20164, // Seal of Justice
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
    // General tree, 40 abilities
    {
        3044, // Arcane Shot
        5118, // Aspect of the Cheetah
       13165, // Aspect of the Hawk
       13159, // Aspect of the Pack
          75, // Auto Shot
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
       13813, // Explosive Trap (Fire)
        6991, // Feed Pet
        5384, // Feign Death
        1543, // Flare
        1499, // Freezing Trap (Frost)
        1130, // Hunter's Mark
       13809, // Ice Trap (Frost)
       53351, // Kill Shot
       53271, // Master's Call
         136, // Mend Pet
       34477, // Misdirection
        2643, // Multi-Shot
        3045, // Rapid Fire
         982, // Revive Pet
        1513, // Scare Beast
       19503, // Scatter Shot
        1978, // Serpent Sting
       34600, // Snake Trap (Nature)
      121818, // Stampede
       56641, // Steady Shot
        1515, // Tame Beast
       19801, // Tranquilizing Shot
       77769, // Trap Launcher
       82654, // Widow Venom
           0,
    },
    // Beast Mastery tree, 4 abilities
    {
       19574, // Bestial Wrath
       77767, // Cobra Shot
       82692, // Focus Fire
       34026, // Kill Command
           0,
    },
    // Marksmanship tree, 3 abilities
    {
       19434, // Aimed Shot
       53209, // Chimera Shot
       34490, // Silencing Shot
           0,
    },
    // Survival tree, 3 abilities
    {
        3674, // Black Arrow
       77767, // Cobra Shot
       53301, // Explosive Shot
           0,
    },
    {
           0,
    },
    // Pet tree, 53 abilities
    {
       55749, // Acid Spit (Special Ability)
       90355, // Ancient Hysteria (Exotic Ability)
       50433, // Ankle Crack (Special Ability)
       17253, // Bite (Basic Attack)
       93433, // Burrow Attack (Exotic Ability)
      128432, // Cackling Howl (Special Ability)
       16827, // Claw (Basic Attack)
       50541, // Clench (Special Ability)
        1742, // Cower (Basic Ability)
       50256, // Demoralizing Roar (Special Ability)
       24423, // Demoralizing Screech (Special Ability)
       50285, // Dust Cloud (Special Ability)
      135678, // Energizing Spores
       34889, // Fire Breath (Special Ability)
       54644, // Frost Breath (Special Ability)
       92380, // Froststorm Breath (Exotic Ability)
       24604, // Furious Howl (Special Ability)
       35290, // Gore (Special Ability)
        2649, // Growl (Basic Ability)
       93434, // Horn Toss (Exotic Ability)
       19577, // Intimidation
       58604, // Lava Breath (Special Ability)
      138374, // Lightning Blast
       24844, // Lightning Breath (Special Ability)
       90327, // Lock Jaw (Special Ability)
       54680, // Monstrous Bite (Exotic Ability)
       50479, // Nether Shock (Special Ability)
       50245, // Pin (Special Ability)
       90347, // Play (Bonus Ability)
       24450, // Prowl (Bonus Ability)
       26090, // Pummel (Special Ability)
       90364, // Qiraji Fortitude (Exotic Ability)
       50518, // Ravage (Special Ability)
       94019, // Rest (Bonus Ability)
       93435, // Roar of Courage (Special Ability)
       50318, // Serenity Dust (Special Ability)
      128433, // Serpent's Swiftness (Special Ability)
       49966, // Smack (Basic Attack)
       91644, // Snatch (Special Ability)
       50519, // Sonic Blast (Special Ability)
      128997, // Spirit Beast Blessing (Special Ability)
       90361, // Spirit Mend (Exotic Ability)
       90328, // Spirit Walk (Bonus Ability)
       50274, // Spore Cloud (Special Ability)
       57386, // Stampede (Special Ability)
       56626, // Sting (Special Ability)
       90314, // Tailspin (Special Ability)
       50498, // Tear Armor (Special Ability)
       90309, // Terrifying Roar (Special Ability)
       35346, // Time Warp (Special Ability)
       94022, // Trick (Bonus Ability)
       54706, // Venom Web Spray (Special Ability)
        4167, // Web (Special Ability)
    },
  },
  // Class active abilities for Rogue
  {
    // General tree, 35 abilities
    {
        8676, // Ambush
        2094, // Blind
        1833, // Cheap Shot
       31224, // Cloak of Shadows
      121411, // Crimson Tempest
        3408, // Crippling Poison
        2823, // Deadly Poison
       51722, // Dismantle
        1725, // Distract
        5277, // Evasion
        2098, // Eviscerate
        8647, // Expose Armor
       51723, // Fan of Knives
        1966, // Feint
         703, // Garrote
        1776, // Gouge
        1766, // Kick
         408, // Kidney Shot
        5761, // Mind-numbing Poison
       14185, // Preparation
       73651, // Recuperate
       73981, // Redirect
        1943, // Rupture
        6770, // Sap
      121471, // Shadow Blades
        5938, // Shiv
      114018, // Shroud of Concealment
        1752, // Sinister Strike
        5171, // Slice and Dice
       76577, // Smoke Bomb
        1784, // Stealth
      121733, // Throw
       57934, // Tricks of the Trade
        1856, // Vanish
        8679, // Wound Poison
           0,
    },
    // Assassination tree, 4 abilities
    {
      111240, // Dispatch
       32645, // Envenom
        1329, // Mutilate
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
    // Subtlety tree, 3 abilities
    {
          53, // Backstab
       16511, // Hemorrhage
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
    // General tree, 22 abilities
    {
       32546, // Binding Heal
         528, // Dispel Magic
         586, // Fade
        6346, // Fear Ward
        2061, // Flash Heal
       64901, // Hymn of Hope
         588, // Inner Fire
       73413, // Inner Will
       73325, // Leap of Faith
        1706, // Levitate
       32375, // Mass Dispel
       48045, // Mind Sear
       21562, // Power Word: Fortitude
          17, // Power Word: Shield
       33076, // Prayer of Mending
        8122, // Psychic Scream
         139, // Renew
        9484, // Shackle Undead
       32379, // Shadow Word: Death
         589, // Shadow Word: Pain
       34433, // Shadowfiend
         585, // Smite
           0,
    },
    // Discipline tree, 12 abilities
    {
       81700, // Archangel
        2060, // Greater Heal
        2050, // Heal
       14914, // Holy Fire
       89485, // Inner Focus
       33206, // Pain Suppression
       47540, // Penance
       62618, // Power Word: Barrier
         596, // Prayer of Healing
         527, // Purify
      109964, // Spirit Shell
      108968, // Void Shift
           0,
    },
    // Holy tree, 14 abilities
    {
       81209, // Chakra: Chastise
       81206, // Chakra: Sanctuary
       81208, // Chakra: Serenity
       34861, // Circle of Healing
       64843, // Divine Hymn
        2060, // Greater Heal
       47788, // Guardian Spirit
        2050, // Heal
       14914, // Holy Fire
       88625, // Holy Word: Chastise
      126135, // Lightwell
         596, // Prayer of Healing
         527, // Purify
      108968, // Void Shift
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
       15473, // Shadowform
       15487, // Silence
       15286, // Vampiric Embrace
       34914, // Vampiric Touch
      142723, // Void Shift
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
    // General tree, 28 abilities
    {
       48707, // Anti-Magic Shell
       42650, // Army of the Dead
       48721, // Blood Boil
       48263, // Blood Presence
       45902, // Blood Strike
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
       73975, // Necrotic Strike
       77575, // Outbreak
        3714, // Path of Frost
       50842, // Pestilence
       45462, // Plague Strike
       61999, // Raise Ally
       46584, // Raise Dead
       47476, // Strangulate
       48265, // Unholy Presence
      113516, // Wild Mushroom: Plague (Symbiosis)
           0,
    },
    // Blood tree, 8 abilities
    {
       49222, // Bone Shield
       49028, // Dancing Rune Weapon
       56222, // Dark Command
       55050, // Heart Strike
       56815, // Rune Strike
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
       55090, // Scourge Strike
      130736, // Soul Reaper
       49206, // Summon Gargoyle
       49016, // Unholy Frenzy
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
    // General tree, 38 abilities
    {
        2008, // Ancestral Spirit
      114049, // Ascendance
       76780, // Bind Elemental
        2825, // Bloodlust
      108269, // Capacitor Totem
        1064, // Chain Heal
         421, // Chain Lightning
       51886, // Cleanse Spirit
        2062, // Earth Elemental Totem
        8042, // Earth Shock
        2484, // Earthbind Totem
        2894, // Fire Elemental Totem
        8050, // Flame Shock
        8024, // Flametongue Weapon
        8056, // Frost Shock
        8033, // Frostbrand Weapon
        2645, // Ghost Wolf
        8177, // Grounding Totem
       73920, // Healing Rain
        5394, // Healing Stream Totem
        8004, // Healing Surge
      108280, // Healing Tide Totem
       32182, // Heroism
       51514, // Hex
         403, // Lightning Bolt
         324, // Lightning Shield
        8190, // Magma Totem
       73899, // Primal Strike
         370, // Purge
        8017, // Rockbiter Weapon
        3599, // Searing Totem
       79206, // Spiritwalker's Grace
      115356, // Stormblast
      120668, // Stormlash Totem
        8143, // Tremor Totem
       73680, // Unleash Elements
       52127, // Water Shield
       57994, // Wind Shear
           0,
    },
    // Elemental tree, 4 abilities
    {
       61882, // Earthquake
       51505, // Lava Burst
       30823, // Shamanistic Rage
       51490, // Thunderstorm
           0,
    },
    // Enhancement tree, 7 abilities
    {
       51533, // Feral Spirit
        1535, // Fire Nova
       60103, // Lava Lash
       30823, // Shamanistic Rage
       58875, // Spirit Walk
       17364, // Stormstrike
        8232, // Windfury Weapon
           0,
    },
    // Restoration tree, 9 abilities
    {
         974, // Earth Shield
       51730, // Earthliving Weapon
       77472, // Greater Healing Wave
         331, // Healing Wave
       51505, // Lava Burst
       16190, // Mana Tide Totem
       77130, // Purify Spirit
       61295, // Riptide
       98008, // Spirit Link Totem
           0,
    },
    {
           0,
    },
    // Pet tree, 6 abilities
    {
       36213, // Angered Earth
       57984, // Fire Blast
      117588, // Fire Nova
      118297, // Immolate
      118345, // Pulverize
       58859, // Spirit Bite
           0,
    },
  },
  // Class active abilities for Mage
  {
    // General tree, 26 abilities
    {
      108978, // Alter Time
      120145, // Ancient Teleport: Dalaran
        1459, // Arcane Brilliance
        1449, // Arcane Explosion
        1953, // Blink
          10, // Blizzard
         120, // Cone of Cold
         759, // Conjure Mana Gem
        2139, // Counterspell
       61316, // Dalaran Brilliance
       44572, // Deep Freeze
       12051, // Evocation
        2136, // Fire Blast
        2120, // Flamestrike
        7302, // Frost Armor
         122, // Frost Nova
       44614, // Frostfire Bolt
       45438, // Ice Block
       30455, // Ice Lance
          66, // Invisibility
        6117, // Mage Armor
      125430, // Mage Bomb
       55342, // Mirror Image
       30482, // Molten Armor
         475, // Remove Curse
       80353, // Time Warp
           0,
    },
    // Arcane tree, 5 abilities
    {
       44425, // Arcane Barrage
       30451, // Arcane Blast
        5143, // Arcane Missiles
       12042, // Arcane Power
       31589, // Slow
           0,
    },
    // Fire tree, 6 abilities
    {
       11129, // Combustion
       31661, // Dragon's Breath
         133, // Fireball
      108853, // Inferno Blast
       11366, // Pyroblast
        2948, // Scorch
           0,
    },
    // Frost tree, 4 abilities
    {
         116, // Frostbolt
       84714, // Frozen Orb
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
    // General tree, 38 abilities
    {
         710, // Banish
         172, // Corruption
        6201, // Create Healthstone
      109466, // Curse of Enfeeblement
        1490, // Curse of the Elements
      109773, // Dark Intent
       77801, // Dark Soul
      111771, // Demonic Gateway
         689, // Drain Life
        1098, // Enslave Demon
        5782, // Fear
       77799, // Fel Flame
      111897, // Grimoire: Felhunter (Summon)
      111859, // Grimoire: Imp (Summon)
      111896, // Grimoire: Succubus (Summon)
      111895, // Grimoire: Voidwalker (Summon)
         755, // Health Funnel
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
        6229, // Twilight Ward
      104773, // Unending Resolve
           0,
    },
    // Affliction tree, 12 abilities
    {
         980, // Agony
       18223, // Curse of Exhaustion
      113860, // Dark Soul: Misery
        1120, // Drain Soul
       48181, // Haunt
      103103, // Malefic Grasp
        5740, // Rain of Fire
      114790, // Seed of Corruption (Soulburn)
       27243, // Seed of Corruption
       86121, // Soul Swap
       74434, // Soulburn
       30108, // Unstable Affliction
           0,
    },
    // Demonology tree, 13 abilities
    {
      103967, // Carrion Swarm (Metamorphosis)
      113861, // Dark Soul: Knowledge
      109151, // Demonic Leap
      111898, // Grimoire: Felguard (Summon)
      105174, // Hand of Gul'dan
        1949, // Hellfire
      104025, // Immolation Aura (Metamorphosis)
      103958, // Metamorphosis
        6353, // Soul Fire
      104027, // Soul Fire (Metamorphosis)
       30146, // Summon Felguard (Summon)
      112870, // Summon Wrathguard (Summon)
      115422, // Void Ray (Metamorphosis)
           0,
    },
    // Destruction tree, 12 abilities
    {
      116858, // Chaos Bolt
       17962, // Conflagrate
      113858, // Dark Soul: Instability
      114635, // Ember Tap
      108683, // Fire and Brimstone
      120451, // Flames of Xoroth
       80240, // Havoc
         348, // Immolate
       29722, // Incinerate
      104232, // Rain of Fire (Destruction)
        5740, // Rain of Fire
       17877, // Shadowburn
           0,
    },
    {
           0,
    },
    // Pet tree, 29 abilities
    {
       89766, // Axe Toss (Special Ability)
      115748, // Bladedance (Basic Attack)
      119899, // Cauterize Master (Special Ability)
       19505, // Devour Magic (Special Ability)
       19658, // Devour Magic
      118093, // Disarm (Special Ability)
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
      112042, // Threatening Presence (Special Ability)
      134477, // Threatening Presence (Special Ability)
      115778, // Tongue Lash (Basic Attack)
        3716, // Torment (Basic Attack)
        6360, // Whiplash (Special Ability)
      115831, // Wrathstorm (Special Ability)
           0,
    },
  },
  // Class active abilities for Monk
  {
    // General tree, 27 abilities
    {
      100784, // Blackout Kick
      117952, // Crackling Jade Lightning
      125170, // Detox
      115450, // Detox
      116095, // Disable
      118022, // Dizzying Haze
      115072, // Expel Harm
      115203, // Fortifying Brew
      117368, // Grapple Weapon
      115460, // Healing Sphere
      100780, // Jab
      115543, // Leer of the Ox (Major Glyph)
      115921, // Legacy of the Emperor
      137562, // Nimble Brew
      115078, // Paralysis
      115546, // Provoke
      109132, // Roll
      116705, // Spear Hand Strike
      101546, // Spinning Crane Kick
      103985, // Stance of the Fierce Tiger (Stance)
      123273, // Surging Mist
      100787, // Tiger Palm
      115080, // Touch of Death
      101643, // Transcendence
      119996, // Transcendence: Transfer
      115176, // Zen Meditation
      126892, // Zen Pilgrimage
           0,
    },
    // Brewmaster tree, 10 abilities
    {
      115213, // Avert Harm
      115181, // Breath of Fire
      122057, // Clash
      115180, // Dizzying Haze
      115308, // Elusive Brew
      115295, // Guard
      121253, // Keg Smash
      119582, // Purifying Brew
      115069, // Stance of the Sturdy Ox (Brewmaster Stance)
      115315, // Summon Black Ox Statue
           0,
    },
    // Mistweaver tree, 11 abilities
    {
      124682, // Enveloping Mist
      116849, // Life Cocoon
      115294, // Mana Tea
      115151, // Renewing Mist
      115310, // Revival
      115175, // Soothing Mist
      115070, // Stance of the Wise Serpent (Mistweaver Stance)
      115313, // Summon Jade Serpent Statue
      116694, // Surging Mist
      116680, // Thunder Focus Tea
      116670, // Uplift
           0,
    },
    // Windwalker tree, 11 abilities
    {
      116768, // Combo Breaker: Blackout Kick
      118864, // Combo Breaker: Tiger Palm
      115288, // Energizing Brew
      113656, // Fists of Fury
      101545, // Flying Serpent Kick
      116781, // Legacy of the White Tiger
      107428, // Rising Sun Kick
      115073, // Spinning Fire Blossom
      137639, // Storm, Earth, and Fire
      116740, // Tigereye Brew
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
    // General tree, 40 abilities
    {
        1066, // Aquatic Form (Shapeshift)
       22812, // Barkskin
        5487, // Bear Form (Shapeshift)
         768, // Cat Form (Shapeshift)
       33786, // Cyclone
        1850, // Dash
      122283, // Death Coil
         339, // Entangling Roots
         770, // Faerie Fire
      110807, // Feral Spirit
       22568, // Ferocious Bite
       33943, // Flight Form (Shapeshift)
       22842, // Frenzied Regeneration
        6795, // Growl
        5185, // Healing Touch
        2637, // Hibernate
       16914, // Hurricane
       29166, // Innervate
       33745, // Lacerate
       22570, // Maim
        1126, // Mark of the Wild
        6807, // Maul
      106922, // Might of Ursoc
      110621, // Mirror Image
        8921, // Moonfire
       16689, // Nature's Grasp
        9005, // Pounce
        5215, // Prowl
        1822, // Rake
       20484, // Rebirth
         774, // Rejuvenation
      112997, // Shattering Blow
        2908, // Soothe
      106898, // Stampeding Roar
       40120, // Swift Flight Form (Shapeshift)
      106785, // Swipe
      110309, // Symbiosis
         740, // Tranquility
         783, // Travel Form (Shapeshift)
        5176, // Wrath
           0,
    },
    // Balance tree, 13 abilities
    {
      127663, // Astral Communion
      106996, // Astral Storm (Balance)
      112071, // Celestial Alignment
       24858, // Moonkin Form
      132158, // Nature's Swiftness
        2782, // Remove Corruption
       78675, // Solar Beam
       48505, // Starfall
        2912, // Starfire
       78674, // Starsurge
       93402, // Sunfire
       88747, // Wild Mushroom
       88751, // Wild Mushroom: Detonate
           0,
    },
    // Feral tree, 9 abilities
    {
      106952, // Berserk
        6785, // Ravage
        2782, // Remove Corruption
        1079, // Rip
       52610, // Savage Roar
        5221, // Shred
      106839, // Skull Bash
       61336, // Survival Instincts
        5217, // Tiger's Fury
           0,
    },
    // Guardian tree, 9 abilities
    {
      102795, // Bear Hug
      106952, // Berserk
        5229, // Enrage
        6785, // Ravage
        2782, // Remove Corruption
        1079, // Rip
       62606, // Savage Defense
      106839, // Skull Bash
       61336, // Survival Instincts
           0,
    },
    // Restoration tree, 11 abilities
    {
      145518, // Genesis
      102342, // Ironbark
       33763, // Lifebloom
       88423, // Nature's Cure
      132158, // Nature's Swiftness
       50464, // Nourish
        8936, // Regrowth
       18562, // Swiftmend
       48438, // Wild Growth
      145205, // Wild Mushroom
      102791, // Wild Mushroom: Bloom
           0,
    },
    // Pet tree, 3 abilities
    {
      150017, // Rake
      110691, // Wrath
      113769, // Wrath
           0,
    },
  },
};

#define PTR_TREE_SPECIALIZATION_SIZE (26)

// Talent tree specialization abilities, wow build 18224
static unsigned __ptr_tree_specialization_data[][MAX_SPECS_PER_CLASS][PTR_TREE_SPECIALIZATION_SIZE] = {
  // Specialization abilities for Hunter pets
  {
    // Specialization abilities for Ferocity
    {
       20782, // Combat Experience
       61684, // Dash
       55709, // Heart of the Phoenix
       53401, // Rabid
       53184, // Spiked Collar
           0,
    },
    // Specialization abilities for Tenacity
    {
       53482, // Blood of the Rhino
       61685, // Charge
       20782, // Combat Experience
       61688, // Great Stamina
       53478, // Last Stand
       63900, // Thunderstomp
           0,
    },
    // Specialization abilities for Cunning
    {
       19596, // Boar's Speed
       53490, // Bullheaded
       20782, // Combat Experience
       53497, // Cornered
       61684, // Dash
       53480, // Roar of Sacrifice
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
       84615, // Blood and Thunder
       86346, // Colossus Smash
      118038, // Die by the Sword
      137049, // Hotfix Passive
       76838, // Mastery: Strikes of Opportunity
       12294, // Mortal Strike
        7384, // Overpower
      119938, // Overpower Driver (DND)
       86101, // Plate Specialization
       12712, // Seasoned Soldier
        1464, // Slam
       29725, // Sudden Death
       12328, // Sweeping Strikes
       56636, // Taste for Blood
      143268, // Unbridled Wrath
        1680, // Whirlwind
           0,
    },
    // Specialization abilities for Fury
    {
       46915, // Bloodsurge
       23881, // Bloodthirst
       86346, // Colossus Smash
       23588, // Crazed Berserker
      118038, // Die by the Sword
       12972, // Flurry
      137050, // Hotfix Passive
       76856, // Mastery: Unshackled Fury
       12950, // Meat Cleaver
       86110, // Plate Specialization
       85288, // Raging Blow
       81099, // Single-Minded Fury
       46917, // Titan's Grip
      143268, // Unbridled Wrath
        1680, // Whirlwind
      100130, // Wild Strike
           0,
    },
    // Specialization abilities for Protection
    {
       84608, // Bastion of Defense
      131086, // Bladestorm (Protection buff) DND
       84615, // Blood and Thunder
        1160, // Demoralizing Shout
       20243, // Devastate
      137048, // Hotfix Passive
       12975, // Last Stand
       76857, // Mastery: Critical Block
       86535, // Plate Specialization
        6572, // Revenge
      145672, // Riposte
      112048, // Shield Barrier
        2565, // Shield Block
       23922, // Shield Slam
       46953, // Sword and Board
      122509, // Ultimatum
       29144, // Unwavering Sentinel
       93098, // Vengeance
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
       53563, // Beacon of Light
       88821, // Daybreak
        2812, // Denounce
       31842, // Divine Favor
       82326, // Divine Light
       54428, // Divine Plea
       86669, // Guardian of Ancient Kings
      112859, // Holy Insight
         635, // Holy Light
       82327, // Holy Radiance
       20473, // Holy Shock
      137029, // Hotfix Passive
       53576, // Infusion of Light
       85222, // Light of Dawn
       76669, // Mastery: Illuminated Healing
       86103, // Plate Specialization
       53551, // Sacred Cleansing
      148040, // Sacred Shield Overridde (Holy)
       85512, // Tower of Radiance
           0,
    },
    // Specialization abilities for Protection
    {
       31850, // Ardent Defender
       31935, // Avenger's Shield
       26573, // Consecration
       85043, // Grand Crusader
       53592, // Guarded by the Light
       86659, // Guardian of Ancient Kings
      115801, // Hammer of the Righteous
       53595, // Hammer of the Righteous
      119072, // Holy Wrath
      137028, // Hotfix Passive
      105424, // Judgments of the Wise
       76671, // Mastery: Divine Bulwark
       86102, // Plate Specialization
      105805, // Sanctuary
       53600, // Shield of the Righteous
       84839, // Vengeance
           0,
    },
    // Specialization abilities for Retribution
    {
      140333, // Absolve
       53385, // Divine Storm
      121783, // Emancipate
         879, // Exorcism
       86698, // Guardian of Ancient Kings
       53595, // Hammer of the Righteous
      115801, // Hammer of the Righteous
      137027, // Hotfix Passive
       84963, // Inquisition
      111529, // Judgments of the Bold
       76672, // Mastery: Hand of Light
       86539, // Plate Specialization
       20164, // Seal of Justice
       53503, // Sword of Light
       20113, // Sword of Light part two (Do not display)
       85256, // Templar's Verdict
       87138, // The Art of War
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
       56315, // Kindred Spirits
       76657, // Mastery: Master of Beasts
       34692, // The Beast Within
           0,
    },
    // Specialization abilities for Marksmanship
    {
       19434, // Aimed Shot
       35110, // Bombardment
       34483, // Careful Aim
       53209, // Chimera Shot
       35102, // Concussive Barrage
      137016, // Hotfix Passive
       34487, // Master Marksman
       76659, // Mastery: Wild Quiver
       53238, // Piercing Shots
       53232, // Rapid Recuperation
       34490, // Silencing Shot
       53224, // Steady Focus
      136311, // Wild Quiver
      143392, // Wild Quiver
           0,
    },
    // Specialization abilities for Survival
    {
        3674, // Black Arrow
       77767, // Cobra Shot
       19387, // Entrapment
       53301, // Explosive Shot
      137017, // Hotfix Passive
       82834, // Improved Serpent Sting
       56343, // Lock and Load
       76658, // Mastery: Essence of the Viper
       87935, // Serpent Spread
       63458, // Trap Mastery
      118976, // Viper Venom
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
      137037, // Hotfix Passive
       14117, // Improved Poisons
       76803, // Mastery: Potent Poisons
        1329, // Mutilate
       14190, // Seal Fate
       79140, // Vendetta
       79134, // Venomous Wounds
           0,
    },
    // Specialization abilities for Combat
    {
       13750, // Adrenaline Rush
       13852, // Ambidexterity
       84654, // Bandit's Guile
       13877, // Blade Flurry
       35551, // Combat Potency
      137036, // Hotfix Passive
       51690, // Killing Spree
       76806, // Mastery: Main Gauche
       79096, // Restless Blades
       84617, // Revealing Strike
       14161, // Ruthlessness
       61329, // Vitality
           0,
    },
    // Specialization abilities for Subtlety
    {
          53, // Backstab
       79152, // Energetic Recovery
       91023, // Find Weakness
       16511, // Hemorrhage
       51701, // Honor Among Thieves
      137035, // Hotfix Passive
       31223, // Master of Subtlety
       76808, // Mastery: Executioner
       14183, // Premeditation
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
       47515, // Divine Aegis
      122098, // Divine Fury
       81662, // Evangelism
       45243, // Focused Will
       47517, // Grace
        2060, // Greater Heal
        2050, // Heal
       14914, // Holy Fire
      137032, // Hotfix Passive
       89485, // Inner Focus
       77484, // Mastery: Shield Discipline
       95860, // Meditation
       33206, // Pain Suppression
       47540, // Penance
       62618, // Power Word: Barrier
         596, // Prayer of Healing
         527, // Purify
       47536, // Rapture
      109964, // Spirit Shell
       87336, // Spiritual Healing
       89488, // Strength of Soul
       92297, // Train of Thought
      108968, // Void Shift
    },
    // Specialization abilities for Holy
    {
       81209, // Chakra: Chastise
       81206, // Chakra: Sanctuary
       81208, // Chakra: Serenity
       34861, // Circle of Healing
      122098, // Divine Fury
       64843, // Divine Hymn
       81662, // Evangelism
       45243, // Focused Will
        2060, // Greater Heal
       47788, // Guardian Spirit
        2050, // Heal
       14914, // Holy Fire
       88625, // Holy Word: Chastise
      137031, // Hotfix Passive
      126135, // Lightwell
       77485, // Mastery: Echo of Light
       95861, // Meditation
         596, // Prayer of Healing
         527, // Purify
       95649, // Rapid Renewal
       63733, // Serendipity
       20711, // Spirit of Redemption
       87336, // Spiritual Healing
      108968, // Void Shift
           0,
    },
    // Specialization abilities for Shadow
    {
        2944, // Devouring Plague
       47585, // Dispersion
      137033, // Hotfix Passive
       77486, // Mastery: Shadowy Recall
        8092, // Mind Blast
       15407, // Mind Flay
       73510, // Mind Spike
       64044, // Psychic Horror
       95740, // Shadow Orbs
      121196, // Shadow Passive
       15473, // Shadowform
       78203, // Shadowy Apparitions
       15487, // Silence
       47573, // Spiritual Precision
       15286, // Vampiric Embrace
       34914, // Vampiric Touch
      114884, // Vampiric Touch <DND>
      142723, // Void Shift
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
       49542, // Blood Parasite
       50034, // Blood Rites
       49222, // Bone Shield
       81136, // Crimson Scourge
       49028, // Dancing Rune Weapon
       56222, // Dark Command
       55050, // Heart Strike
      137008, // Hotfix Passive
       50371, // Improved Blood Presence
       77513, // Mastery: Blood Shield
       86537, // Plate Specialization
      145676, // Riposte
       56815, // Rune Strike
       48982, // Rune Tap
       81127, // Sanguine Fortitude
       81132, // Scarlet Fever
       49509, // Scent of Blood
      148211, // Scent of Blood
      114866, // Soul Reaper
       55233, // Vampiric Blood
       93099, // Vengeance
       50029, // Veteran of the Third War
       81164, // Will of the Necropolis
           0,
    },
    // Specialization abilities for Frost
    {
       54637, // Blood of the North
       81328, // Brittle Bones
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
       59057, // Rime
      130735, // Soul Reaper
       66192, // Threat of Thassarian
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
       52143, // Master of Ghouls
       77515, // Mastery: Dreadblade
       86536, // Plate Specialization
       56835, // Reaping
       55090, // Scourge Strike
       49572, // Shadow Infusion
      130736, // Soul Reaper
       49530, // Sudden Doom
       49206, // Summon Gargoyle
       55610, // Unholy Aura
       49016, // Unholy Frenzy
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
       89920, // Ancestral Focus
       61882, // Earthquake
       16164, // Elemental Focus
       60188, // Elemental Fury
       51470, // Elemental Oath
       30674, // Elemental Precision
       29000, // Elemental Reach
       88766, // Fulmination
      137040, // Hotfix Passive
       51505, // Lava Burst
       77756, // Lava Surge
       86108, // Mail Specialization
       77222, // Mastery: Elemental Overload
       88764, // Rolling Thunder
       62099, // Shamanism
       30823, // Shamanistic Rage
      123099, // Spiritual Insight
       51490, // Thunderstorm
           0,
    },
    // Specialization abilities for Enhancement
    {
       86629, // Dual Wield
       51533, // Feral Spirit
        1535, // Fire Nova
       16282, // Flurry
      137041, // Hotfix Passive
       60103, // Lava Lash
       51530, // Maelstrom Weapon
       86099, // Mail Specialization
       77223, // Mastery: Enhanced Elements
       30814, // Mental Quickness
       51522, // Primal Wisdom
       77657, // Searing Flames
       30823, // Shamanistic Rage
       58875, // Spirit Walk
       51527, // Static Shock
       17364, // Stormstrike
       30809, // Unleashed Rage
        8232, // Windfury Weapon
           0,
    },
    // Specialization abilities for Restoration
    {
       51558, // Ancestral Awakening
       89920, // Ancestral Focus
         974, // Earth Shield
       51730, // Earthliving Weapon
       77472, // Greater Healing Wave
         331, // Healing Wave
      137039, // Hotfix Passive
       51505, // Lava Burst
       86100, // Mail Specialization
       16190, // Mana Tide Totem
       77226, // Mastery: Deep Healing
       95862, // Meditation
       16213, // Purification
       77130, // Purify Spirit
       16196, // Resurgence
       61295, // Riptide
       98008, // Spirit Link Totem
      112858, // Spiritual Insight
       51564, // Tidal Waves
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
        5143, // Arcane Missiles
       79684, // Arcane Missiles
       12042, // Arcane Power
      137021, // Hotfix Passive
       76547, // Mastery: Mana Adept
       31589, // Slow
           0,
    },
    // Specialization abilities for Fire
    {
       11129, // Combustion
      117216, // Critical Mass
       31661, // Dragon's Breath
         133, // Fireball
      137019, // Hotfix Passive
      108853, // Inferno Blast
       12846, // Mastery: Ignite
       11366, // Pyroblast
       44448, // Pyroblast Clearcasting Driver
      132209, // Pyromaniac
        2948, // Scorch
           0,
    },
    // Specialization abilities for Frost
    {
       44549, // Brain Freeze
      112965, // Fingers of Frost
         116, // Frostbolt
       84714, // Frozen Orb
      137020, // Hotfix Passive
       12472, // Icy Veins
       76613, // Mastery: Icicles
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
       18223, // Curse of Exhaustion
      113860, // Dark Soul: Misery
        1120, // Drain Soul
       48181, // Haunt
      137043, // Hotfix Passive
       53759, // Improved Fear
      103103, // Malefic Grasp
       77215, // Mastery: Potent Afflictions
      108558, // Nightfall
        5740, // Rain of Fire
       27243, // Seed of Corruption
      117198, // Soul Shards
       86121, // Soul Swap
       74434, // Soulburn
      103112, // Soulburn: Curse
      104243, // Soulburn: Demonic Circle: Teleport
      103101, // Soulburn: Health Funnel
       86664, // Soulburn: Seed of Corruption
      141931, // Soulburn: Soul Swap
       30108, // Unstable Affliction
           0,
    },
    // Specialization abilities for Demonology
    {
      103967, // Carrion Swarm
      113861, // Dark Soul: Knowledge
      108869, // Decimation
      104315, // Demonic Fury
      109145, // Demonic Fury
      109151, // Demonic Leap
      108559, // Demonic Rebirth
      103131, // Felguard: Axe Toss
      103134, // Felguard: Felstorm
      103129, // Felguard: Pursuit
      105174, // Hand of Gul'dan
        1949, // Hellfire
      137044, // Hotfix Passive
       77219, // Mastery: Master Demonologist
      103958, // Metamorphosis
      124917, // Metamorphosis: Chaos Wave
      116208, // Metamorphosis: Cursed Auras
      124913, // Metamorphosis: Doom
      109797, // Metamorphosis: Immolation Aura
      112089, // Metamorphosis: Touch of Chaos
      129343, // Metamorphosis: Void Ray
      122351, // Molten Core
      114129, // Nether Plating
        6353, // Soul Fire
       30146, // Summon Felguard
      114592, // Wild Imps
    },
    // Specialization abilities for Destruction
    {
      109784, // Aftermath
      117896, // Backdraft
      108563, // Backlash
      108647, // Burning Embers
      116858, // Chaos Bolt
      111546, // Chaotic Energy
       17962, // Conflagrate
      113858, // Dark Soul: Instability
      114635, // Ember Tap
      108683, // Fire and Brimstone
      120451, // Flames of Xoroth
       80240, // Havoc
      137046, // Hotfix Passive
         348, // Immolate
       29722, // Incinerate
       77220, // Mastery: Emberstorm
      123686, // Pyroclasm
        5740, // Rain of Fire
      116852, // Self Burning Triggers
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
      115213, // Avert Harm
      115181, // Breath of Fire
      128938, // Brewing: Elusive Brew
      117967, // Brewmaster Training
      122057, // Clash
      126086, // Desperate Measures
      126060, // Desperate Measures
      115180, // Dizzying Haze
      124146, // Dual Wield
      115308, // Elusive Brew
      124502, // Gift of the Ox
      115295, // Guard
      137023, // Hotfix Passive
      121253, // Keg Smash
      123192, // Keg Smash
      120225, // Leather Specialization
      117906, // Mastery: Elusive Brawler
      119582, // Purifying Brew
      115069, // Stance of the Sturdy Ox
      115315, // Summon Black Ox Statue
      120267, // Vengeance
           0,
    },
    // Specialization abilities for Mistweaver
    {
      116346, // <DND> Soothing Mists Trigger
      123766, // Brewing: Mana Tea
      122464, // Dematerialize
      124682, // Enveloping Mist
      137024, // Hotfix Passive
      115451, // Internal Medicine
      120224, // Leather Specialization
      116849, // Life Cocoon
      121278, // Mana Meditation
      115294, // Mana Tea
      115869, // Mana Tea Driver <DND>
      117907, // Mastery: Gift of the Serpent
      139598, // Muscle Memory
      115151, // Renewing Mist
      115310, // Revival
      115175, // Soothing Mist
      115070, // Stance of the Wise Serpent
      115313, // Summon Jade Serpent Statue
      116694, // Surging Mist
      116645, // Teachings of the Monastery
      118672, // Teachings of the Monastery
      116680, // Thunder Focus Tea
      120272, // Tiger Strikes
      116670, // Uplift
           0,
    },
    // Specialization abilities for Windwalker
    {
      126046, // Adaptation
      116092, // Afterlife
      123980, // Brewing: Tigereye Brew
      128595, // Combat Conditioning
      137384, // Combo Breaker
      124146, // Dual Wield
      115288, // Energizing Brew
      113656, // Fists of Fury
      101545, // Flying Serpent Kick
      137025, // Hotfix Passive
      120227, // Leather Specialization
      116781, // Legacy of the White Tiger
      115636, // Mastery: Bottled Fury
      107428, // Rising Sun Kick
      116023, // Sparring
      115073, // Spinning Fire Blossom
      137639, // Storm, Earth, and Fire
      120272, // Tiger Strikes
      116740, // Tigereye Brew
      122470, // Touch of Karma
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
      106732, // Balance Overrides Passive
       33596, // Balance of Power
      112071, // Celestial Alignment
       84738, // Celestial Focus
       79577, // Eclipse
       81062, // Euphoria
      137013, // Hotfix Passive
      108299, // Killer Instinct
       86104, // Leather Specialization
       33605, // Lunar Shower
       77492, // Mastery: Total Eclipse
       24858, // Moonkin Form
      112857, // Natural Insight
      132158, // Nature's Swiftness
       48393, // Owlkin Frenzy
        2782, // Remove Corruption
       93399, // Shooting Stars
       78675, // Solar Beam
       48505, // Starfall
        2912, // Starfire
       78674, // Starsurge
       93402, // Sunfire
       88747, // Wild Mushroom
       88751, // Wild Mushroom: Detonate
           0,
    },
    // Specialization abilities for Feral
    {
      106952, // Berserk
       16949, // Feral Instinct
      106733, // Feral Overrides Passive
      137011, // Hotfix Passive
       48484, // Infected Wounds
       17007, // Leader of the Pack
       86097, // Leather Specialization
       77493, // Mastery: Razor Claws
       33873, // Nurturing Instinct
       16864, // Omen of Clarity
       16974, // Predatory Swiftness
        6785, // Ravage
        2782, // Remove Corruption
        1079, // Rip
       52610, // Savage Roar
        5221, // Shred
      106839, // Skull Bash
       61336, // Survival Instincts
      106832, // Thrash
      115800, // Thrash
        5217, // Tiger's Fury
           0,
    },
    // Specialization abilities for Guardian
    {
      102795, // Bear Hug
      106952, // Berserk
        5229, // Enrage
      106734, // Guardian Overrides Passive
      137010, // Hotfix Passive
       48484, // Infected Wounds
       17007, // Leader of the Pack
       86096, // Leather Specialization
       77494, // Mastery: Nature's Guardian
       33873, // Nurturing Instinct
        6785, // Ravage
        2782, // Remove Corruption
        1079, // Rip
       62606, // Savage Defense
      106839, // Skull Bash
       61336, // Survival Instincts
       16931, // Thick Hide
      106832, // Thrash
      115800, // Thrash
      135288, // Tooth and Claw
       84840, // Vengeance
           0,
    },
    // Specialization abilities for Restoration
    {
      145518, // Genesis
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
       17073, // Naturalist
       88423, // Nature's Cure
       84736, // Nature's Focus
      132158, // Nature's Swiftness
       50464, // Nourish
      113043, // Omen of Clarity
        8936, // Regrowth
      106735, // Restoration Overrides Passive
       33886, // Swift Rejuvenation
       18562, // Swiftmend
       48438, // Wild Growth
      138611, // Wild Mushroom
      145205, // Wild Mushroom
      102791, // Wild Mushroom: Bloom
           0,
    },
  },
};

#define PTR_CLASS_MASTERY_ABILITY_SIZE (1)

// Class mastery abilities, wow build 18224
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
      76838, // Mastery: Strikes of Opportunity
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
      76659, // Mastery: Wild Quiver
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
      77486, // Mastery: Shadowy Recall
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
      77222, // Mastery: Elemental Overload
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
      77494, // Mastery: Nature's Guardian
    },
    // Masteries for Restoration specialization
    {
      77495, // Mastery: Harmony
    },
  },
};

#define PTR_RACE_ABILITY_SIZE (5)

#ifndef ptr_MAX_RACE
#define ptr_MAX_RACE (28)
#endif

// Racial abilities, wow build 18224
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
  },
  // Racial abilities for Human
  {
    // Generic racial abilities
    {
      59752, // Every Man for Himself
      20864, // Mace Specialization
      20597, // Sword Specialization
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
      20574, // Axe Specialization
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
      20576, // Command
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
      54562, // Command
          0,
    },
    // Racial abilities for Shaman class
    {
      33697, // Blood Fury
      65222, // Command
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
      20575, // Command
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
      20595, // Crack Shot
      92682, // Explorer
      20596, // Frost Resistance
      59224, // Mace Specialization
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
          0,
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
  // Racial abilities for Tauren
  {
    // Generic racial abilities
    {
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
  },
  // Racial abilities for Gnome
  {
    // Generic racial abilities
    {
      20592, // Arcane Resistance
      20589, // Escape Artist
      20591, // Expansive Mind
      92680, // Shortblade Specialization
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
  // Racial abilities for Troll
  {
    // Generic racial abilities
    {
      20557, // Beast Slaying
      26297, // Berserking
      58943, // Da Voodoo Shuffle
      26290, // Dead Eye
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
      28730, // Arcane Torrent
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
    {     0, },
    // Racial abilities for Warrior class
    {
      28880, // Gift of the Naaru
       6562, // Heroic Presence
      59221, // Shadow Resistance
          0,
    },
    // Racial abilities for Paladin class
    {
      59542, // Gift of the Naaru
       6562, // Heroic Presence
      59535, // Shadow Resistance
          0,
    },
    // Racial abilities for Hunter class
    {
      59543, // Gift of the Naaru
       6562, // Heroic Presence
      59536, // Shadow Resistance
          0,
    },
    {     0, },
    // Racial abilities for Priest class
    {
      59544, // Gift of the Naaru
      28878, // Heroic Presence
      59538, // Shadow Resistance
          0,
    },
    // Racial abilities for Death Knight class
    {
      59545, // Gift of the Naaru
       6562, // Heroic Presence
      59539, // Shadow Resistance
          0,
    },
    // Racial abilities for Shaman class
    {
      59547, // Gift of the Naaru
      28878, // Heroic Presence
      59540, // Shadow Resistance
          0,
    },
    // Racial abilities for Mage class
    {
      59548, // Gift of the Naaru
      28878, // Heroic Presence
      59541, // Shadow Resistance
          0,
    },
    {     0, },
    // Racial abilities for Monk class
    {
      121093, // Gift of the Naaru
       6562, // Heroic Presence
      132295, // Shadow Resistance
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
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
    },
    // Racial abilities for Paladin class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
    },
    // Racial abilities for Hunter class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
    },
    // Racial abilities for Rogue class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
    },
    // Racial abilities for Priest class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
    },
    // Racial abilities for Death Knight class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
    },
    // Racial abilities for Shaman class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
    },
    // Racial abilities for Mage class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
    },
    // Racial abilities for Warlock class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
    },
    {     0, },
    // Racial abilities for Druid class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      68975, // Viciousness
          0,
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

#define PTR_GLYPH_ABILITIES_SIZE (33)

// Glyph spells for classes, wow build 18224
static unsigned __ptr_glyph_abilities_data[][3][PTR_GLYPH_ABILITIES_SIZE] = {
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
    // Major Glyphs (30 spells)
    {
       58377, // Glyph of Blitz
       58367, // Glyph of Bloodthirst
       94372, // Glyph of Bull Rush
       89003, // Glyph of Colossus Smash
       63325, // Glyph of Death From Above
       58386, // Glyph of Die by the Sword
       58355, // Glyph of Enraged Speed
       58357, // Glyph of Gag Order
       58385, // Glyph of Hamstring
       58388, // Glyph of Heavy Repercussions
       58366, // Glyph of Hindering Strikes
       58387, // Glyph of Hoarse Voice
       58364, // Glyph of Hold the Line
      146970, // Glyph of Impaling Throws
      122013, // Glyph of Incite
       58097, // Glyph of Long Charge
       58368, // Glyph of Mortal Strike
       58370, // Glyph of Raging Wind
       94374, // Glyph of Recklessness
       58356, // Glyph of Resonating Power
       58372, // Glyph of Rude Interruption
       58375, // Glyph of Shield Slam
       63329, // Glyph of Shield Wall
       63328, // Glyph of Spell Reflection
       58384, // Glyph of Sweeping Strikes
       58098, // Glyph of Unending Rage
      146965, // Glyph of Victorious Throw
       58382, // Glyph of Victory Rush
       63324, // Glyph of Whirlwind
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
    // Major Glyphs (33 spells)
    {
       54927, // Glyph of Avenging Wrath
       63218, // Glyph of Beacon of Light
       54943, // Glyph of Blessed Life
       54934, // Glyph of Blinding Light
       54931, // Glyph of Burden of Guilt
       54928, // Glyph of Consecration
       56414, // Glyph of Dazing Shield
       56420, // Glyph of Denounce
      146955, // Glyph of Devotion Aura
       63223, // Glyph of Divine Plea
       54924, // Glyph of Divine Protection
      146956, // Glyph of Divine Shield
       63220, // Glyph of Divine Storm
       54939, // Glyph of Divinity
       54922, // Glyph of Double Jeopardy
       54935, // Glyph of Final Wrath
       57955, // Glyph of Flash of Light
       54930, // Glyph of Focused Shield
       63219, // Glyph of Hammer of the Righteous
      146957, // Glyph of Hand of Sacrifice
       54938, // Glyph of Harsh Words
       63224, // Glyph of Holy Shock
       54923, // Glyph of Holy Wrath
       54937, // Glyph of Illumination
       56416, // Glyph of Immediate Truth
       63225, // Glyph of Inquisition
       54940, // Glyph of Light of Dawn
      122028, // Glyph of Mass Exorcism
       93466, // Glyph of Protector of the Innocent
       54926, // Glyph of Templar's Verdict
       54936, // Glyph of Word of Glory
       63222, // Glyph of the Alabaster Shield
      119477, // Glyph of the Battle Healer
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
    // Major Glyphs (26 spells)
    {
      126095, // Glyph of Aimed Shot
       20895, // Glyph of Animal Bond
      109263, // Glyph of Black Ice
      119449, // Glyph of Camouflage
      119447, // Glyph of Chimera Shot
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
       56829, // Glyph of Misdirection
       53299, // Glyph of No Escape
       19560, // Glyph of Pathfinding
       63069, // Glyph of Scatter Shot
       56849, // Glyph of Snake Trap
      119407, // Glyph of Solace
      119384, // Glyph of Tranquilizing Shot
      146657, // Glyph of the Lean Pack
           0,
    },
    // Minor Glyphs (12 spells)
    {
      125042, // Glyph of Aspect of the Beast
      119462, // Glyph of Aspect of the Cheetah
       57904, // Glyph of Aspect of the Pack
      122492, // Glyph of Aspects
      126179, // Glyph of Direction
      126746, // Glyph of Fetch
       57903, // Glyph of Fireworks
       57870, // Glyph of Lesser Proportion
      126193, // Glyph of Marking
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
    // Major Glyphs (24 spells)
    {
       56813, // Glyph of Ambush
       56818, // Glyph of Blade Flurry
       91299, // Glyph of Blind
       56801, // Glyph of Cheap Shot
       63269, // Glyph of Cloak of Shadows
       63254, // Glyph of Deadly Momentum
       56799, // Glyph of Evasion
       56803, // Glyph of Expose Armor
       56804, // Glyph of Feint
       56812, // Glyph of Garrote
       56809, // Glyph of Gouge
      146631, // Glyph of Hemorrhaging Veins
       56805, // Glyph of Kick
      146625, // Glyph of Recovery
       56806, // Glyph of Recuperate
      146629, // Glyph of Redirect
       56808, // Glyph of Shadow Walk
      146628, // Glyph of Sharpened Knives
       56810, // Glyph of Shiv
       56819, // Glyph of Smoke Bomb
       56811, // Glyph of Sprint
       63253, // Glyph of Stealth
       89758, // Glyph of Vanish
       63249, // Glyph of Vendetta
           0,
    },
    // Minor Glyphs (14 spells)
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
       63256, // Glyph of Tricks of the Trade
      146960, // Glyph of the Headhunter
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Priest
  {
    // Major Glyphs (33 spells)
    {
       63248, // Glyph of Binding Heal
       55675, // Glyph of Circle of Healing
       55673, // Glyph of Deep Wells
      119864, // Glyph of Dispel Magic
       63229, // Glyph of Dispersion
       55684, // Glyph of Fade
       55678, // Glyph of Fear Ward
      147778, // Glyph of Focused Mending
      119853, // Glyph of Holy Fire
      125045, // Glyph of Holy Nova
       55686, // Glyph of Inner Fire
       14771, // Glyph of Inner Sanctum
      119850, // Glyph of Leap of Faith
      108939, // Glyph of Levitate
      126133, // Glyph of Lightwell
       55691, // Glyph of Mass Dispel
       87195, // Glyph of Mind Blast
      120585, // Glyph of Mind Flay
       33371, // Glyph of Mind Spike
      119866, // Glyph of Penance
       55672, // Glyph of Power Word: Shield
       55685, // Glyph of Prayer of Mending
       55688, // Glyph of Psychic Horror
       55676, // Glyph of Psychic Scream
       55677, // Glyph of Purify
       33202, // Glyph of Reflective Shield
      119872, // Glyph of Renew
       55690, // Glyph of Scourge Imprisonment
      120583, // Glyph of Shadow Word: Death
       55692, // Glyph of Smite
      119873, // Glyph of Spirit of Redemption
      120584, // Glyph of Vampiric Embrace
       89489, // Glyph of Weakened Soul
    },
    // Minor Glyphs (14 spells)
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
      147779, // Glyph of Shifted Appearances
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
    // Major Glyphs (24 spells)
    {
       58623, // Glyph of Anti-Magic Shell
       58620, // Glyph of Chains of Ice
       63330, // Glyph of Dancing Rune Weapon
       63331, // Glyph of Dark Simulacrum
       96279, // Glyph of Dark Succor
       63333, // Glyph of Death Coil
       62259, // Glyph of Death Grip
       58629, // Glyph of Death and Decay
       58671, // Glyph of Enduring Infection
      146650, // Glyph of Festering Blood
       58673, // Glyph of Icebound Fortitude
       58631, // Glyph of Icy Touch
       58686, // Glyph of Mind Freeze
       59332, // Glyph of Outbreak
       58657, // Glyph of Pestilence
       58635, // Glyph of Pillar of Frost
      146648, // Glyph of Regenerative Magic
       58647, // Glyph of Shifting Presences
       58618, // Glyph of Strangulate
      146645, // Glyph of Swift Death
       59327, // Glyph of Unholy Command
       58616, // Glyph of Unholy Frenzy
       58676, // Glyph of Vampiric Blood
      146646, // Glyph of the Loud Horn
           0,
    },
    // Minor Glyphs (12 spells)
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
      146653, // Glyph of the Long Winter
      146652, // Glyph of the Skeleton
           0,
    },
    {
           0,
    },
  },
  // Glyph spells for Shaman
  {
    // Major Glyphs (30 spells)
    {
       55442, // Glyph of Capacitor Totem
       55449, // Glyph of Chain Lightning
       55452, // Glyph of Chaining
       55445, // Glyph of Cleansing Waters
      147781, // Glyph of Eternal Earth
       63271, // Glyph of Feral Spirit
       55455, // Glyph of Fire Elemental Totem
       55450, // Glyph of Fire Nova
       55447, // Glyph of Flame Shock
       55443, // Glyph of Frost Shock
       59289, // Glyph of Ghost Wolf
       55441, // Glyph of Grounding Totem
       89646, // Glyph of Healing Storm
       55456, // Glyph of Healing Stream Totem
       55440, // Glyph of Healing Wave
       63291, // Glyph of Hex
      101052, // Glyph of Lightning Shield
       55439, // Glyph of Purge
      147762, // Glyph of Purging
       63273, // Glyph of Riptide
       63280, // Glyph of Shamanistic Rage
       55454, // Glyph of Spirit Walk
       55446, // Glyph of Spiritwalker's Grace
       55453, // Glyph of Telluric Currents
       63270, // Glyph of Thunder
       55438, // Glyph of Totemic Recall
       63298, // Glyph of Totemic Vigor
       55437, // Glyph of Unstable Earth
       55436, // Glyph of Water Shield
       55451, // Glyph of Wind Shear
           0,
    },
    // Minor Glyphs (16 spells)
    {
      147787, // Glyph of Astral Fixation
       58058, // Glyph of Astral Recall
       63279, // Glyph of Deluge
      147788, // Glyph of Elemental Familiars
       58059, // Glyph of Far Sight
      147772, // Glyph of Flaming Serpents
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
       98397, // Glyph of Armors
       56365, // Glyph of Blink
       56368, // Glyph of Combustion
      115705, // Glyph of Cone of Cold
      115703, // Glyph of Counterspell
      115710, // Glyph of Deep Freeze
       56380, // Glyph of Evocation
       56376, // Glyph of Frost Nova
       61205, // Glyph of Frostfire Bolt
      115723, // Glyph of Ice Block
       56364, // Glyph of Icy Veins
       89926, // Glyph of Inferno Blast
       56383, // Glyph of Mana Gem
       56375, // Glyph of Polymorph
      146659, // Glyph of Rapid Displacement
      115700, // Glyph of Remove Curse
       86209, // Glyph of Slow
      115713, // Glyph of Spellsteal
       56377, // Glyph of Splitting Ice
       63090, // Glyph of Water Elemental
           0,
    },
    // Minor Glyphs (15 spells)
    {
       57925, // Glyph of Arcane Language
      147353, // Glyph of Condensation
      126748, // Glyph of Conjure Familiar
       56382, // Glyph of Crittermorph
      146662, // Glyph of Evaporation
       63092, // Glyph of Illusion
       56363, // Glyph of Loose Mana
       63093, // Glyph of Mirror Image
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
    // Major Glyphs (19 spells)
    {
       56235, // Glyph of Conflagrate
       58080, // Glyph of Curse of Exhaustion
      146963, // Glyph of Curse of the Elements
       63303, // Glyph of Demon Hunting
       56249, // Glyph of Demon Training
       63309, // Glyph of Demonic Circle
       63302, // Glyph of Drain Life
       63304, // Glyph of Ember Tap
      148683, // Glyph of Eternal Resolve
       56244, // Glyph of Fear
      146962, // Glyph of Havoc
       56224, // Glyph of Healthstone
       56242, // Glyph of Imp Swarm
       63320, // Glyph of Life Tap
       56218, // Glyph of Siphon Life
       58070, // Glyph of Soul Consumption
       56231, // Glyph of Soulstone
      146964, // Glyph of Unending Resolve
       56233, // Glyph of Unstable Affliction
           0,
    },
    // Minor Glyphs (15 spells)
    {
       56250, // Glyph of Carrion Swarm
       63312, // Glyph of Crimson Banish
       58107, // Glyph of Enslave Demon
       58081, // Glyph of Eye of Kilrogg
       56247, // Glyph of Falling Meteor
       56246, // Glyph of Felguard
      135557, // Glyph of Gateway Attunement
       56248, // Glyph of Hand of Gul'dan
       56238, // Glyph of Health Funnel
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
    // Major Glyphs (24 spells)
    {
      125676, // Glyph of Afterlife
      123394, // Glyph of Breath of Fire
      123399, // Glyph of Clash
      146954, // Glyph of Detox
      120482, // Glyph of Enduring Healing Sphere
      125671, // Glyph of Fists of Fury
      124997, // Glyph of Fortifying Brew
      146953, // Glyph of Fortuitous Spheres
      123401, // Glyph of Guard
      125967, // Glyph of Leer of the Ox
      124989, // Glyph of Life Cocoon
      123763, // Glyph of Mana Tea
      146952, // Glyph of Nimble Brew
      125755, // Glyph of Paralysis
      146951, // Glyph of Rapid Rolling
      123334, // Glyph of Renewing Mist
      125673, // Glyph of Sparring
      120479, // Glyph of Spinning Crane Kick
      120483, // Glyph of Surging Mist
      146950, // Glyph of Targeted Expulsion
      123391, // Glyph of Touch of Death
      125678, // Glyph of Touch of Karma
      123023, // Glyph of Transcendence
      120477, // Glyph of Zen Meditation
           0,
    },
    // Minor Glyphs (11 spells)
    {
      132005, // Glyph of Blackout Kick
      125931, // Glyph of Crackling Tiger Lightning
      125872, // Glyph of Fighting Pose
      123403, // Glyph of Flying Serpent Kick
      125732, // Glyph of Honor
      125660, // Glyph of Jab
      125151, // Glyph of Rising Tiger Kick
      123405, // Glyph of Spinning Fire Blossom
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
    // Major Glyphs (32 spells)
    {
       63057, // Glyph of Barkskin
      121840, // Glyph of Blooming
       47180, // Glyph of Cat Form
       48514, // Glyph of Cyclone
       59219, // Glyph of Dash
      145529, // Glyph of Efflorescence
       54760, // Glyph of Entangling Roots
      114237, // Glyph of Fae Silence
       94386, // Glyph of Faerie Fire
       67598, // Glyph of Ferocious Bite
       54810, // Glyph of Frenzied Regeneration
      146655, // Glyph of Guided Stars
       54825, // Glyph of Healing Touch
       54831, // Glyph of Hurricane
       54832, // Glyph of Innervate
       54811, // Glyph of Maul
      116238, // Glyph of Might of Ursoc
      116203, // Glyph of Nature's Grasp
       54812, // Glyph of Omens
       54821, // Glyph of Pounce
      116186, // Glyph of Prowl
       54733, // Glyph of Rebirth
      116218, // Glyph of Regrowth
       17076, // Glyph of Rejuvenation
      127540, // Glyph of Savagery
      114234, // Glyph of Shred
      116216, // Glyph of Skull Bash
      114300, // Glyph of Stampede
      114222, // Glyph of Stampeding Roar
      114223, // Glyph of Survival Instincts
       62970, // Glyph of Wild Growth
      116172, // Glyph of the Master Shapeshifter
           0,
    },
    // Minor Glyphs (12 spells)
    {
       57856, // Glyph of Aquatic Form
       57855, // Glyph of Charm Woodland Creature
      114295, // Glyph of Grace
      146656, // Glyph of One With Nature
      114301, // Glyph of Stars
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

#define PTR_TIER_BONUSES_SIZE (8)
#define PTR_TIER_BONUSES_MAX_TIER (5)

#define PTR_TIER_BONUSES_TIER_BASE (13)

// Tier item set bonuses for class, wow build 18224
static unsigned __ptr_tier_bonuses_data[][PTR_TIER_BONUSES_MAX_TIER][PTR_TIER_BONUSES_SIZE] = {
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

