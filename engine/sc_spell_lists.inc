#define CLASS_ABILITY_SIZE (58)
#define CLASS_ABILITY_TREE_SIZE (4)

// Class based active abilities, wow build 12984
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
  },
  // Class active abilities for Warrior
  {
    // Arms tree, 16 abilities
    {
       2457, // Battle Stance
      46924, // Bladestorm
        100, // Charge
      86346, // Colossus Smash
       1715, // Hamstring
         78, // Heroic Strike
      57755, // Heroic Throw
      12294, // Mortal Strike
       7384, // Overpower
        772, // Rend
      20230, // Retaliation
      64382, // Shattering Throw
      88161, // Strike
      12328, // Sweeping Strikes
      85388, // Throwdown
       6343, // Thunder Clap
          0,
    },
    // Fury tree, 23 abilities
    {
       6673, // Battle Shout
      18499, // Berserker Rage
       2458, // Berserker Stance
      23881, // Bloodthirst
       1161, // Challenging Shout
        845, // Cleave
        469, // Commanding Shout
      12292, // Death Wish
       1160, // Demoralizing Shout
      55694, // Enraged Regeneration
       5308, // Execute
      60970, // Heroic Fury
       6544, // Heroic Leap
       1134, // Inner Rage
      20252, // Intercept
       5246, // Intimidating Shout
      12323, // Piercing Howl
       6552, // Pummel
      85288, // Raging Blow
       1719, // Recklessness
       1464, // Slam
      34428, // Victory Rush
       1680, // Whirlwind
          0,
    },
    // Protection tree, 16 abilities
    {
      12809, // Concussion Blow
         71, // Defensive Stance
      20243, // Devastate
        676, // Disarm
       3411, // Intervene
      12975, // Last Stand
       6572, // Revenge
         72, // Shield Bash
       2565, // Shield Block
      23922, // Shield Slam
        871, // Shield Wall
      46968, // Shockwave
      23920, // Spell Reflection
       7386, // Sunder Armor
        355, // Taunt
      50720, // Vigilance
          0,
    },
    {
          0,
    },
  },
  // Class active abilities for Paladin
  {
    // Holy tree, 17 abilities
    {
      53563, // Beacon of Light
       4987, // Cleanse
      19746, // Concentration Aura
      26573, // Consecration
      31842, // Divine Favor
      82326, // Divine Light
      54428, // Divine Plea
        879, // Exorcism
      19750, // Flash of Light
        635, // Holy Light
      82327, // Holy Radiance
      20473, // Holy Shock
       2812, // Holy Wrath
        633, // Lay on Hands
      85222, // Light of Dawn
      20165, // Seal of Insight
      85673, // Word of Glory
          0,
    },
    // Protection tree, 21 abilities
    {
      31850, // Ardent Defender
      31935, // Avenger's Shield
      20217, // Blessing of Kings
        465, // Devotion Aura
      70940, // Divine Guardian
        498, // Divine Protection
        642, // Divine Shield
      86150, // Guardian of Ancient Kings
        853, // Hammer of Justice
      53595, // Hammer of the Righteous
       1044, // Hand of Freedom
       1022, // Hand of Protection
      62124, // Hand of Reckoning
       6940, // Hand of Sacrifice
       1038, // Hand of Salvation
      19891, // Resistance Aura
      31789, // Righteous Defense
      25780, // Righteous Fury
      20164, // Seal of Justice
      20154, // Seal of Righteousness
      53600, // Shield of the Righteous
          0,
    },
    // Retribution tree, 14 abilities
    {
      31884, // Avenging Wrath
      19740, // Blessing of Might
      32223, // Crusader Aura
      35395, // Crusader Strike
      53385, // Divine Storm
      24275, // Hammer of Wrath
      84963, // Inquisition
      20271, // Judgement
      85285, // Rebuke
      20066, // Repentance
       7294, // Retribution Aura
      31801, // Seal of Truth
      85256, // Templar's Verdict
      85696, // Zealotry
          0,
    },
    {
          0,
    },
  },
  // Class active abilities for Hunter
  {
    // Beast Mastery tree, 20 abilities
    {
       5118, // Aspect of the Cheetah
      82661, // Aspect of the Fox
      13165, // Aspect of the Hawk
      13159, // Aspect of the Pack
      20043, // Aspect of the Wild
       1462, // Beast Lore
      19574, // Bestial Wrath
        883, // Call Pet 1
      83242, // Call Pet 2
      83243, // Call Pet 3
      83244, // Call Pet 4
      83245, // Call Pet 5
       2641, // Dismiss Pet
      82726, // Fervor
      19577, // Intimidation
      34026, // Kill Command
        136, // Mend Pet
        982, // Revive Pet
       1515, // Tame Beast
      82654, // Widow Venom
          0,
    },
    // Marksmanship tree, 15 abilities
    {
      19434, // Aimed Shot
       3044, // Arcane Shot
         75, // Auto Shot
      53209, // Chimera Shot
       5116, // Concussive Shot
      20736, // Distracting Shot
       1543, // Flare
       1130, // Hunter's Mark
      53351, // Kill Shot
       2643, // Multi-Shot
       3045, // Rapid Fire
      23989, // Readiness
      34490, // Silencing Shot
      56641, // Steady Shot
      19801, // Tranquilizing Shot
          0,
    },
    // Survival tree, 18 abilities
    {
       3674, // Black Arrow
      51753, // Camouflage
      77767, // Cobra Shot
      19306, // Counterattack
      19263, // Deterrence
      53301, // Explosive Shot
      13813, // Explosive Trap
       1499, // Freezing Trap
      13809, // Ice Trap
      13795, // Immolation Trap
      34477, // Misdirection
       2973, // Raptor Strike
      19503, // Scatter Shot
       1978, // Serpent Sting
      34600, // Snake Trap
      77769, // Trap Launcher
       2974, // Wing Clip
      19386, // Wyvern Sting
          0,
    },
    // Pet tree, 58 abilities
    {
      90355, // Ancient Hysteria
      50433, // Ankle Crack
      17253, // Bite
      53434, // Call of the Wild
      61685, // Charge
      16827, // Claw
      50541, // Clench
      35387, // Corrosive Spit
       1742, // Cower
      50256, // Demoralizing Roar
      24423, // Demoralizing Screech
      50285, // Dust Cloud
      75447, // Ferocious Inspiration
      34889, // Fire Breath
      54644, // Frost Breath
      92380, // Froststorm Breath
      24604, // Furious Howl
      35290, // Gore
       2649, // Growl
      55709, // Heart of the Phoenix
      93434, // Horn Toss
      53476, // Intervene
      58604, // Lava Breath
      53426, // Lick Your Wounds
      24844, // Lightning Breath
      54680, // Monstrous Bite
      50479, // Nether Shock
      50245, // Pin
      90347, // Play
      24450, // Prowl
      26090, // Pummel
      90364, // Qiraji Fortitude
      50518, // Ravage
      94019, // Rest
      93435, // Roar of Courage
      53517, // Roar of Recovery
      53480, // Roar of Sacrifice
      50318, // Serenity Dust
      49966, // Smack
      91644, // Snatch
      50519, // Sonic Blast
      90361, // Spirit Mend
      90328, // Spirit Walk
      50274, // Spore Cloud
      57386, // Stampede
      56626, // Sting
      52825, // Swoop
      90314, // Tailspin
      53477, // Taunt
      50498, // Tear Armor
      50271, // Tendon Rip
      90309, // Terrifying Roar
      63900, // Thunderstomp
      35346, // Time Warp
      94022, // Trick
      54706, // Venom Web Spray
       4167, // Web
      53508, // Wolverine Bite
    },
  },
  // Class active abilities for Rogue
  {
    // Assassination tree, 13 abilities
    {
       8676, // Ambush
       1833, // Cheap Shot
      26679, // Deadly Throw
      51722, // Dismantle
      32645, // Envenom
       2098, // Eviscerate
       8647, // Expose Armor
        703, // Garrote
        408, // Kidney Shot
       1329, // Mutilate
       1943, // Rupture
       5171, // Slice and Dice
      79140, // Vendetta
          0,
    },
    // Combat tree, 14 abilities
    {
      13750, // Adrenaline Rush
         53, // Backstab
      13877, // Blade Flurry
       5277, // Evasion
      51723, // Fan of Knives
       1966, // Feint
       1776, // Gouge
       1766, // Kick
      51690, // Killing Spree
      73651, // Recuperate
      84617, // Revealing Strike
      14251, // Riposte
       5938, // Shiv
       1752, // Sinister Strike
          0,
    },
    // Subtlety tree, 12 abilities
    {
       2094, // Blind
      31224, // Cloak of Shadows
       1725, // Distract
      16511, // Hemorrhage
      14185, // Preparation
      73981, // Redirect
       6770, // Sap
      36554, // Shadowstep
      76577, // Smoke Bomb
       1784, // Stealth
      57934, // Tricks of the Trade
       1856, // Vanish
          0,
    },
    {
          0,
    },
  },
  // Class active abilities for Priest
  {
    // Discipline tree, 16 abilities
    {
      87151, // Archangel
        527, // Dispel Magic
       6346, // Fear Ward
        588, // Inner Fire
      89485, // Inner Focus
      73413, // Inner Will
       1706, // Levitate
       8129, // Mana Burn
      32375, // Mass Dispel
      33206, // Pain Suppression
      47540, // Penance
      10060, // Power Infusion
      62618, // Power Word: Barrier
      21562, // Power Word: Fortitude
         17, // Power Word: Shield
       9484, // Shackle Undead
          0,
    },
    // Holy tree, 20 abilities
    {
      32546, // Binding Heal
      14751, // Chakra
      34861, // Circle of Healing
        528, // Cure Disease
      19236, // Desperate Prayer
      64843, // Divine Hymn
       2061, // Flash Heal
       2060, // Greater Heal
      47788, // Guardian Spirit
       2050, // Heal
      14914, // Holy Fire
      15237, // Holy Nova
      88625, // Holy Word: Chastise
      64901, // Hymn of Hope
      73325, // Leap of Faith
        724, // Lightwell
        596, // Prayer of Healing
      33076, // Prayer of Mending
        139, // Renew
        585, // Smite
          0,
    },
    // Shadow Magic tree, 18 abilities
    {
       2944, // Devouring Plague
      47585, // Dispersion
        586, // Fade
       8092, // Mind Blast
        605, // Mind Control
      15407, // Mind Flay
      48045, // Mind Sear
      73510, // Mind Spike
      64044, // Psychic Horror
       8122, // Psychic Scream
      27683, // Shadow Protection
      32379, // Shadow Word: Death
        589, // Shadow Word: Pain
      34433, // Shadowfiend
      15473, // Shadowform
      15487, // Silence
      15286, // Vampiric Embrace
      34914, // Vampiric Touch
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
    // Blood tree, 14 abilities
    {
      48721, // Blood Boil
      48263, // Blood Presence
      45902, // Blood Strike
      45529, // Blood Tap
      49222, // Bone Shield
      49028, // Dancing Rune Weapon
      56222, // Dark Command
      77606, // Dark Simulacrum
      49998, // Death Strike
      55050, // Heart Strike
      50842, // Pestilence
      48982, // Rune Tap
      47476, // Strangulate
      55233, // Vampiric Blood
          0,
    },
    // Frost tree, 12 abilities
    {
      45524, // Chains of Ice
      85948, // Festering Strike
      48266, // Frost Presence
      49184, // Howling Blast
      49203, // Hungering Cold
      48792, // Icebound Fortitude
      45477, // Icy Touch
      47528, // Mind Freeze
      49020, // Obliterate
       3714, // Path of Frost
      51271, // Pillar of Frost
      56815, // Rune Strike
          0,
    },
    // Unholy tree, 14 abilities
    {
      48707, // Anti-Magic Shell
      51052, // Anti-Magic Zone
      42650, // Army of the Dead
      63560, // Dark Transformation
      47541, // Death Coil
      49576, // Death Grip
      43265, // Death and Decay
      73975, // Necrotic Strike
      77575, // Outbreak
      45462, // Plague Strike
      55090, // Scourge Strike
      49206, // Summon Gargoyle
      49016, // Unholy Frenzy
      48265, // Unholy Presence
          0,
    },
    // Pet tree, 4 abilities
    {
      47468, // Claw
      47481, // Gnaw
      47484, // Huddle
      47482, // Leap
          0,
    },
  },
  // Class active abilities for Shaman
  {
    // Elemental Combat tree, 20 abilities
    {
      76780, // Bind Elemental
        421, // Chain Lightning
       8042, // Earth Shock
       2484, // Earthbind Totem
      61882, // Earthquake
      16166, // Elemental Mastery
       2894, // Fire Elemental Totem
       1535, // Fire Nova
       8050, // Flame Shock
       8056, // Frost Shock
      51514, // Hex
      51505, // Lava Burst
        403, // Lightning Bolt
       8190, // Magma Totem
        370, // Purge
       3599, // Searing Totem
      79206, // Spiritwalker's Grace
       5730, // Stoneclaw Totem
      51490, // Thunderstorm
      57994, // Wind Shear
          0,
    },
    // Enhancement tree, 20 abilities
    {
       2062, // Earth Elemental Totem
       8184, // Elemental Resistance Totem
      51533, // Feral Spirit
       8227, // Flametongue Totem
       8024, // Flametongue Weapon
       8033, // Frostbrand Weapon
       2645, // Ghost Wolf
       8177, // Grounding Totem
      60103, // Lava Lash
        324, // Lightning Shield
      73899, // Primal Strike
       8017, // Rockbiter Weapon
      30823, // Shamanistic Rage
       8071, // Stoneskin Totem
      17364, // Stormstrike
       8075, // Strength of Earth Totem
      73680, // Unleash Elements
       8512, // Windfury Totem
       8232, // Windfury Weapon
       3738, // Wrath of Air Totem
          0,
    },
    // Restoration tree, 15 abilities
    {
       1064, // Chain Heal
      51886, // Cleanse Spirit
        974, // Earth Shield
      51730, // Earthliving Weapon
      77472, // Greater Healing Wave
       5394, // Healing Stream Totem
       8004, // Healing Surge
        331, // Healing Wave
       5675, // Mana Spring Totem
      16190, // Mana Tide Totem
      16188, // Nature's Swiftness
      61295, // Riptide
      87718, // Totem of Tranquil Mind
       8143, // Tremor Totem
      52127, // Water Shield
          0,
    },
    {
          0,
    },
  },
  // Class active abilities for Mage
  {
    // Arcane tree, 21 abilities
    {
      44425, // Arcane Barrage
      30451, // Arcane Blast
       1459, // Arcane Brilliance
       1449, // Arcane Explosion
       5143, // Arcane Missiles
      12042, // Arcane Power
       1953, // Blink
        759, // Conjure Mana Gem
       2139, // Counterspell
      61316, // Dalaran Brilliance
      12051, // Evocation
      54646, // Focus Magic
         66, // Invisibility
       6117, // Mage Armor
        543, // Mage Ward
       1463, // Mana Shield
      55342, // Mirror Image
      12043, // Presence of Mind
        475, // Remove Curse
      31589, // Slow
      80353, // Time Warp
          0,
    },
    // Fire tree, 12 abilities
    {
      11113, // Blast Wave
      11129, // Combustion
      31661, // Dragon's Breath
       2136, // Fire Blast
        133, // Fireball
      82731, // Flame Orb
       2120, // Flamestrike
      44614, // Frostfire Bolt
      44457, // Living Bomb
      30482, // Molten Armor
      11366, // Pyroblast
       2948, // Scorch
          0,
    },
    // Frost tree, 13 abilities
    {
         10, // Blizzard
      11958, // Cold Snap
        120, // Cone of Cold
      82676, // Curtain of Frost
      44572, // Deep Freeze
       7302, // Frost Armor
        122, // Frost Nova
        116, // Frostbolt
      11426, // Ice Barrier
      45438, // Ice Block
      30455, // Ice Lance
      12472, // Icy Veins
      31687, // Summon Water Elemental
          0,
    },
    // Pet tree, 2 abilities
    {
      33395, // Freeze
      31707, // Waterbolt
          0,
    },
  },
  // Class active abilities for Warlock
  {
    // Affliction tree, 18 abilities
    {
        980, // Bane of Agony
        603, // Bane of Doom
        172, // Corruption
      18223, // Curse of Exhaustion
       1714, // Curse of Tongues
        702, // Curse of Weakness
       1490, // Curse of the Elements
      80398, // Dark Intent
       6789, // Death Coil
        689, // Drain Life
       5138, // Drain Mana
       1120, // Drain Soul
       5782, // Fear
      48181, // Haunt
       1454, // Life Tap
      27243, // Seed of Corruption
      86121, // Soul Swap
      30108, // Unstable Affliction
          0,
    },
    // Demonology tree, 27 abilities
    {
        710, // Banish
       6201, // Create Healthstone
        693, // Create Soulstone
        687, // Demon Armor
      54785, // Demon Leap
      77801, // Demon Soul
      47193, // Demonic Empowerment
      88448, // Demonic Rebirth
       1098, // Enslave Demon
      28176, // Fel Armor
      71521, // Hand of Gul'dan
        755, // Health Funnel
      85403, // Hellfire
      50589, // Immolation Aura
      47241, // Metamorphosis
       6229, // Shadow Ward
      79268, // Soul Harvest
      19028, // Soul Link
      74434, // Soulburn
      29858, // Soulshatter
      18540, // Summon Doomguard
      30146, // Summon Felguard
        691, // Summon Felhunter
        688, // Summon Imp
       1122, // Summon Infernal
        712, // Summon Succubus
        697, // Summon Voidwalker
          0,
    },
    // Destruction tree, 14 abilities
    {
      80240, // Bane of Havoc
      50796, // Chaos Bolt
      17962, // Conflagrate
      77799, // Fel Flame
       1949, // Hellfire
        348, // Immolate
      29722, // Incinerate
       5740, // Rain of Fire
       5676, // Searing Pain
        686, // Shadow Bolt
      17877, // Shadowburn
      47897, // Shadowflame
      30283, // Shadowfury
       6353, // Soul Fire
          0,
    },
    // Pet tree, 20 abilities
    {
      89766, // Axe Toss
       6307, // Blood Pact
      17767, // Consume Shadows
      19505, // Devour Magic
      85692, // Doom Bolt
      54424, // Fel Intelligence
      89751, // Felstorm
       3110, // Firebolt
      89792, // Flee
       7814, // Lash of Pain
      30213, // Legion Strike
       7870, // Lesser Invisibility
      30151, // Pursuit
       6358, // Seduction
      54049, // Shadow Bite
      89808, // Singe Magic
      19647, // Spell Lock
      17735, // Suffering
       3716, // Torment
       6360, // Whiplash
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
  // Class active abilities for Druid
  {
    // Balance tree, 22 abilities
    {
      22812, // Barkskin
      33786, // Cyclone
        339, // Entangling Roots
        770, // Faerie Fire
      33831, // Force of Nature
       2637, // Hibernate
      16914, // Hurricane
      29166, // Innervate
       5570, // Insect Swarm
       8921, // Moonfire
      24858, // Moonkin Form
      16689, // Nature's Grasp
      78675, // Solar Beam
       2908, // Soothe
      48505, // Starfall
       2912, // Starfire
      78674, // Starsurge
        467, // Thorns
      50516, // Typhoon
      88747, // Wild Mushroom
      88751, // Wild Mushroom: Detonate
       5176, // Wrath
          0,
    },
    // Feral tree, 37 abilities
    {
       1066, // Aquatic Form
       5211, // Bash
       5487, // Bear Form
      50334, // Berserk
        768, // Cat Form
       5209, // Challenging Roar
       1082, // Claw
       8998, // Cower
         99, // Demoralizing Roar
       5229, // Enrage
      16857, // Faerie Fire (Feral)
      49376, // Feral Charge
      16979, // Feral Charge
      22568, // Ferocious Bite
      33943, // Flight Form
      22842, // Frenzied Regeneration
       6795, // Growl
      33745, // Lacerate
      22570, // Maim
       6807, // Maul
       9005, // Pounce
       5215, // Prowl
      80313, // Pulverize
       1822, // Rake
       6785, // Ravage
       1079, // Rip
      52610, // Savage Roar
       5221, // Shred
      80965, // Skull Bash
      80964, // Skull Bash
      61336, // Survival Instincts
      40120, // Swift Flight Form
      62078, // Swipe
        779, // Swipe
      77758, // Thrash
       5217, // Tiger's Fury
        783, // Travel Form
          0,
    },
    // Restoration tree, 12 abilities
    {
       5185, // Healing Touch
      33763, // Lifebloom
       1126, // Mark of the Wild
      17116, // Nature's Swiftness
      50464, // Nourish
       8936, // Regrowth
        774, // Rejuvenation
       2782, // Remove Corruption
      18562, // Swiftmend
        740, // Tranquility
      33891, // Tree of Life
      48438, // Wild Growth
          0,
    },
    {
          0,
    },
  },
};

#define TREE_SPECIALIZATION_SIZE (4)

// Talent tree specialization abilities, wow build 12984
static unsigned __tree_specialization_data[][MAX_TALENT_TABS][TREE_SPECIALIZATION_SIZE] = {
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
  // Talent Tree specialization abilities for Warrior
  {
    // Specialization abilities for Arms tree
    {
      12296, // Anger Management
      12294, // Mortal Strike
      12712, // Two-Handed Weapon Specialization
          0,
    },
    // Specialization abilities for Fury tree
    {
      23881, // Bloodthirst
      23588, // Dual Wield Specialization
      29592, // Precision
          0,
    },
    // Specialization abilities for Protection tree
    {
      29144, // Sentinel
      23922, // Shield Slam
      93098, // Vengeance
          0,
    },
  },
  // Talent Tree specialization abilities for Paladin
  {
    // Specialization abilities for Holy tree
    {
      20473, // Holy Shock
      85101, // Meditation
      85102, // Walk in the Light
          0,
    },
    // Specialization abilities for Protection tree
    {
      31935, // Avenger's Shield
      31878, // Judgements of the Wise
      53592, // Touched by the Light
      84839, // Vengeance
    },
    // Specialization abilities for Retribution tree
    {
      89901, // Judgements of the Bold
      53503, // Sheath of Light
      85256, // Templar's Verdict
      20113, // Two-Handed Weapon Specialization
    },
  },
  // Talent Tree specialization abilities for Hunter
  {
    // Specialization abilities for Beast Mastery tree
    {
      87325, // Animal Handler
      19577, // Intimidation
          0,
    },
    // Specialization abilities for Marksmanship tree
    {
      19434, // Aimed Shot
      87326, // Artisan Quiver
          0,
    },
    // Specialization abilities for Survival tree
    {
      53301, // Explosive Shot
      84729, // Into the Wilderness
          0,
    },
  },
  // Talent Tree specialization abilities for Rogue
  {
    // Specialization abilities for Assassination tree
    {
      84601, // Assassin's Resolve
      14117, // Improved Poisons
       1329, // Mutilate
          0,
    },
    // Specialization abilities for Combat tree
    {
      13877, // Blade Flurry
      13852, // Dual Wield Specialization
      61329, // Vitality
          0,
    },
    // Specialization abilities for Subtlety tree
    {
      31223, // Master of Subtlety
      36554, // Shadowstep
      31220, // Sinister Calling
          0,
    },
  },
  // Talent Tree specialization abilities for Priest
  {
    // Specialization abilities for Discipline tree
    {
      84732, // Enlightenment
      85101, // Meditation
      47540, // Penance
          0,
    },
    // Specialization abilities for Holy tree
    {
      88625, // Holy Word: Chastise
      85101, // Meditation
      87336, // Spiritual Healing
          0,
    },
    // Specialization abilities for Shadow tree
    {
      15407, // Mind Flay
      87327, // Shadow Power
          0,
    },
  },
  // Talent Tree specialization abilities for Death Knight
  {
    // Specialization abilities for Blood tree
    {
      50034, // Blood Rites
      55050, // Heart Strike
      93099, // Vengeance
      50029, // Veteran of the Third War
    },
    // Specialization abilities for Frost tree
    {
      54637, // Blood of the North
      49143, // Frost Strike
      50887, // Icy Talons
          0,
    },
    // Specialization abilities for Unholy tree
    {
      52143, // Master of Ghouls
      56835, // Reaping
      55090, // Scourge Strike
      91107, // Unholy Might
    },
  },
  // Talent Tree specialization abilities for Shaman
  {
    // Specialization abilities for Elemental tree
    {
      60188, // Elemental Fury
      62099, // Shamanism
      51490, // Thunderstorm
          0,
    },
    // Specialization abilities for Enhancement tree
    {
      86629, // Dual Wield
      60103, // Lava Lash
      30814, // Mental Quickness
      51522, // Primal Wisdom
    },
    // Specialization abilities for Restoration tree
    {
        974, // Earth Shield
      85101, // Meditation
      16213, // Purification
          0,
    },
  },
  // Talent Tree specialization abilities for Mage
  {
    // Specialization abilities for Arcane tree
    {
      44425, // Arcane Barrage
      84671, // Arcane Specialization
          0,
    },
    // Specialization abilities for Fire tree
    {
      84668, // Fire Specialization
      11366, // Pyroblast
          0,
    },
    // Specialization abilities for Frost tree
    {
      84669, // Frost Specialization
      31687, // Summon Water Elemental
          0,
    },
  },
  // Talent Tree specialization abilities for Warlock
  {
    // Specialization abilities for Affliction tree
    {
      87339, // Shadow Mastery
      30108, // Unstable Affliction
          0,
    },
    // Specialization abilities for Demonology tree
    {
      84740, // Demonic Knowledge
      30146, // Summon Felguard
          0,
    },
    // Specialization abilities for Destruction tree
    {
      84739, // Cataclysm
      17962, // Conflagrate
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
  },
  // Talent Tree specialization abilities for Druid
  {
    // Specialization abilities for Balance tree
    {
      16913, // Moonfury
      78674, // Starsurge
          0,
    },
    // Specialization abilities for Feral Combat tree
    {
      84735, // Aggression
      87335, // Feral Instinct
      33917, // Mangle
      84840, // Vengeance
    },
    // Specialization abilities for Restoration tree
    {
      87305, // Gift of Nature
      85101, // Meditation
      18562, // Swiftmend
          0,
    },
  },
};

#define CLASS_MASTERY_ABILITY_SIZE (4)

// Class mastery abilities, wow build 12984
static unsigned __class_mastery_ability_data[][CLASS_MASTERY_ABILITY_SIZE] = {
  {
        0,
  },
  // Class mastery abilities for Warrior
  {
    76857, // Critical Block
    76838, // Strikes of Opportunity
    76856, // Unshackled Fury
        0,
  },
  // Class mastery abilities for Paladin
  {
    76671, // Divine Bulwark
    76672, // Hand of Light
    76669, // Illuminated Healing
        0,
  },
  // Class mastery abilities for Hunter
  {
    76658, // Essence of the Viper
    76657, // Master of Beasts
    76659, // Wild Quiver
        0,
  },
  // Class mastery abilities for Rogue
  {
    76808, // Executioner
    76806, // Main Gauche
    76803, // Potent Poisons
        0,
  },
  // Class mastery abilities for Priest
  {
    77485, // Echo of Light
    77486, // Shadow Orbs
    77484, // Shield Discipline
        0,
  },
  // Class mastery abilities for Death Knight
  {
    77515, // Blightcaller
    77513, // Blood Shield
    77514, // Frozen Heart
        0,
  },
  // Class mastery abilities for Shaman
  {
    77226, // Deep Healing
    77222, // Elemental Overload
    77223, // Enhanced Elements
        0,
  },
  // Class mastery abilities for Mage
  {
    76595, // Flashburn
    76613, // Frostburn
    76547, // Mana Adept
        0,
  },
  // Class mastery abilities for Warlock
  {
    77220, // Fiery Apocalypse
    77219, // Master Demonologist
    77215, // Potent Afflictions
        0,
  },
  {
        0,
  },
  // Class mastery abilities for Druid
  {
    77495, // Deep Healing
    77493, // Razor Claws
    77494, // Savage Defender
    77492, // Total Eclipse
  },
};

#define RACE_ABILITY_SIZE (6)
#ifndef CLASS_SIZE
#define CLASS_SIZE (12)
#endif

// Racial abilities, wow build 12984
static unsigned __race_ability_data[][CLASS_SIZE][RACE_ABILITY_SIZE] = {
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
    {     0, },
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
      20596, // Frost Resistance
      20595, // Gun Specialization
      59224, // Mace Specialization
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
    {     0, },
    {     0, },
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
  // Racial abilities for Undead
  {
    // Generic racial abilities
    {
      20577, // Cannibalize
      20579, // Shadow Resistance
       5227, // Underwater Breathing
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
      26290, // Bow Specialization
      58943, // Da Voodoo Shuffle
      20555, // Regeneration
      20558, // Throwing Specialization
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
        822, // Magic Resistance
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
      28730, // Arcane Torrent
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
    {     0, },
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
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Paladin class
    {
      68976, // Aberration
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Hunter class
    {
      68976, // Aberration
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Rogue class
    {
      68976, // Aberration
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Priest class
    {
      68976, // Aberration
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Death Knight class
    {
      68976, // Aberration
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Shaman class
    {
      68976, // Aberration
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Mage class
    {
      68976, // Aberration
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Warlock class
    {
      68976, // Aberration
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    {     0, },
    // Racial abilities for Druid class
    {
      68976, // Aberration
      69001, // Transform: Worgen
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
};

