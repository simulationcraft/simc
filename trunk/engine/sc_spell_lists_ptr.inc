#define PTR_CLASS_ABILITY_SIZE (64)
#define PTR_CLASS_ABILITY_TREE_SIZE (4)

// Class based active abilities, wow build 13707
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
  },
  // Class active abilities for Warrior
  {
    // Arms tree, 16 abilities
    {
       2457, // Battle Stance
      46924, // Bladestorm
        100, // Charge (Battle Stance)
      86346, // Colossus Smash (Battle, Berserker Stance)
       1715, // Hamstring (Battle, Berserker Stance)
         78, // Heroic Strike
      57755, // Heroic Throw
      12294, // Mortal Strike
       7384, // Overpower (Battle Stance)
        772, // Rend (Battle, Defensive Stance)
      20230, // Retaliation (Battle Stance)
      64382, // Shattering Throw (Battle Stance)
      88161, // Strike
      12328, // Sweeping Strikes (Battle, Berserker Stance)
      85388, // Throwdown (Battle Stance)
       6343, // Thunder Clap (Battle, Defensive Stance)
          0,
    },
    // Fury tree, 24 abilities
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
       5308, // Execute (Battle, Berserker Stance)
      60970, // Heroic Fury
       6544, // Heroic Leap
       1134, // Inner Rage
      20252, // Intercept (Berserker Stance)
       5246, // Intimidating Shout
      12323, // Piercing Howl
       6552, // Pummel
      85288, // Raging Blow (Berserker Stance)
      97462, // Rallying Cry
       1719, // Recklessness (Berserker Stance)
       1464, // Slam
      34428, // Victory Rush
       1680, // Whirlwind (Berserker Stance)
          0,
    },
    // Protection tree, 15 abilities
    {
      12809, // Concussion Blow
         71, // Defensive Stance
      20243, // Devastate
        676, // Disarm (Defensive Stance)
       3411, // Intervene (Defensive Stance)
      12975, // Last Stand
       6572, // Revenge (Defensive Stance)
       2565, // Shield Block (Defensive Stance)
      23922, // Shield Slam
        871, // Shield Wall (Defensive Stance)
      46968, // Shockwave
      23920, // Spell Reflection (Battle, Defensive Stance)
       7386, // Sunder Armor
        355, // Taunt (Defensive Stance)
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
    // Protection tree, 20 abilities
    {
      31850, // Ardent Defender
      31935, // Avenger's Shield
      20217, // Blessing of Kings
        465, // Devotion Aura
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
      96231, // Rebuke
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
    // Beast Mastery tree, 19 abilities
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
      13813, // Explosive Trap (Fire)
       1499, // Freezing Trap (Frost)
      13809, // Ice Trap (Frost)
      13795, // Immolation Trap (Fire)
      34477, // Misdirection
       2973, // Raptor Strike
      19503, // Scatter Shot
       1978, // Serpent Sting
      34600, // Snake Trap (Nature)
      77769, // Trap Launcher
       2974, // Wing Clip
      19386, // Wyvern Sting
          0,
    },
    // Pet tree, 64 abilities
    {
      55749, // Acid Spit (Special Ability)
      90355, // Ancient Hysteria
      50433, // Ankle Crack (Special Ability)
      90337, // Bad Manner
      17253, // Bite (Basic Attack)
      93433, // Burrow Attack (Exotic Ability)
      53434, // Call of the Wild
      61685, // Charge
      16827, // Claw (Basic Attack)
      50541, // Clench (Special Ability)
      35387, // Corrosive Spit (Special Ability)
       1742, // Cower
      50256, // Demoralizing Roar (Special Ability)
      24423, // Demoralizing Screech
      50285, // Dust Cloud (Special Ability)
      90363, // Embrace of the Shale Spider (Exotic Ability)
      75447, // Ferocious Inspiration
      34889, // Fire Breath (Special Ability)
      54644, // Frost Breath (Special Ability)
      92380, // Froststorm Breath (Exotic Ability)
      24604, // Furious Howl (Special Ability)
      35290, // Gore (Special Ability)
       2649, // Growl
      55709, // Heart of the Phoenix (Talent)
      93434, // Horn Toss (Exotic Ability)
      53476, // Intervene
      58604, // Lava Breath (Special Ability)
      53426, // Lick Your Wounds
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
      53517, // Roar of Recovery
      53480, // Roar of Sacrifice
      50318, // Serenity Dust (Special Ability)
      49966, // Smack (Basic Attack)
      91644, // Snatch (Special Ability)
      50519, // Sonic Blast (Special Ability)
      90361, // Spirit Mend (Exotic Ability)
      90328, // Spirit Walk (Bonus Ability)
      50274, // Spore Cloud (Special Ability)
      57386, // Stampede (Special Ability)
      56626, // Sting (Special Ability)
      52825, // Swoop
      90314, // Tailspin (Special Ability)
      53477, // Taunt
      50498, // Tear Armor
      50271, // Tendon Rip (Special Ability)
      90309, // Terrifying Roar (Special Ability)
      63900, // Thunderstomp
      35346, // Time Warp (Special Ability)
      94022, // Trick (Bonus Ability)
      54706, // Venom Web Spray (Special Ability)
       4167, // Web (Special Ability)
      96201, // Web Wrap (Special Ability)
      53508, // Wolverine Bite
    },
  },
  // Class active abilities for Rogue
  {
    // Assassination tree, 16 abilities
    {
       8676, // Ambush
       1833, // Cheap Shot
       2818, // Deadly Poison
      26679, // Deadly Throw
      51722, // Dismantle
      32645, // Envenom
       2098, // Eviscerate
       8647, // Expose Armor
        703, // Garrote
       8680, // Instant Poison
        408, // Kidney Shot
       1329, // Mutilate
       1943, // Rupture
       5171, // Slice and Dice
      79140, // Vendetta
      13218, // Wound Poison
          0,
    },
    // Combat tree, 13 abilities
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
    // Blood tree, 15 abilities
    {
      48721, // Blood Boil
      48263, // Blood Presence
      45902, // Blood Strike
      45529, // Blood Tap
      49222, // Bone Shield
      49028, // Dancing Rune Weapon
      56222, // Dark Command
      77606, // Dark Simulacrum
      48743, // Death Pact
      49998, // Death Strike
      66188, // Death Strike Off-Hand
      55050, // Heart Strike
      50842, // Pestilence
      48982, // Rune Tap
      47476, // Strangulate
          0,
    },
    // Frost tree, 13 abilities
    {
      45524, // Chains of Ice
      85948, // Festering Strike
      48266, // Frost Presence
      49143, // Frost Strike
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
    // Unholy tree, 16 abilities
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
      61999, // Raise Ally
      46584, // Raise Dead
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
    // Enhancement tree, 22 abilities
    {
       2825, // Bloodlust
       2062, // Earth Elemental Totem
       8184, // Elemental Resistance Totem
      51533, // Feral Spirit
       8227, // Flametongue Totem
       8024, // Flametongue Weapon
       8033, // Frostbrand Weapon
       2645, // Ghost Wolf
       8177, // Grounding Totem
      32182, // Heroism
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
    // Pet tree, 3 abilities
    {
      57984, // Fire Blast
      12470, // Fire Nova
      13376, // Fire Shield
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
    // Frost tree, 14 abilities
    {
         10, // Blizzard
      11958, // Cold Snap
        120, // Cone of Cold
      44572, // Deep Freeze
       7302, // Frost Armor
        122, // Frost Nova
        116, // Frostbolt
      92283, // Frostfire Orb
      11426, // Ice Barrier
      45438, // Ice Block
      30455, // Ice Lance
      12472, // Icy Veins
      82676, // Ring of Frost
      31687, // Summon Water Elemental
          0,
    },
    // Pet tree, 6 abilities
    {
      88084, // Arcane Blast
      59637, // Fire Blast
      88082, // Fireball
      33395, // Freeze
      59638, // Frostbolt
      31707, // Waterbolt
          0,
    },
  },
  // Class active abilities for Warlock
  {
    // Affliction tree, 17 abilities
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
      54785, // Demon Leap (Metamorphosis)
      77801, // Demon Soul
      47193, // Demonic Empowerment
      88448, // Demonic Rebirth
       1098, // Enslave Demon
      28176, // Fel Armor
      71521, // Hand of Gul'dan
        755, // Health Funnel
      85403, // Hellfire
      50589, // Immolation Aura (Metamorphosis)
      47241, // Metamorphosis
       6229, // Shadow Ward
      79268, // Soul Harvest
      19028, // Soul Link
      74434, // Soulburn
      29858, // Soulshatter
      18540, // Summon Doomguard (Guardian)
      30146, // Summon Felguard (Summon)
        691, // Summon Felhunter (Summon)
        688, // Summon Imp (Summon)
       1122, // Summon Infernal (Guardian)
        712, // Summon Succubus (Summon)
        697, // Summon Voidwalker (Summon)
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
      89766, // Axe Toss (Special Ability)
       6307, // Blood Pact (Special Ability)
      17767, // Consume Shadows (Special Ability)
      19505, // Devour Magic (Special Ability)
      85692, // Doom Bolt
      54424, // Fel Intelligence (Special Ability)
      89751, // Felstorm (Special Ability)
       3110, // Firebolt (Basic Attack)
      89792, // Flee (Special Ability)
       7814, // Lash of Pain (Basic Attack)
      30213, // Legion Strike (Basic Attack)
       7870, // Lesser Invisibility (Special Ability)
      30151, // Pursuit (Special Ability)
       6358, // Seduction (Special Ability)
      54049, // Shadow Bite (Basic Attack)
      89808, // Singe Magic (Special Ability)
      19647, // Spell Lock (Special Ability)
      17735, // Suffering (Special Ability)
       3716, // Torment (Basic Attack)
       6360, // Whiplash (Special Ability)
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
      24858, // Moonkin Form (Shapeshift)
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
    // Feral tree, 38 abilities
    {
       1066, // Aquatic Form (Shapeshift)
       5211, // Bash (Bear Form)
       5487, // Bear Form (Shapeshift)
      50334, // Berserk (Cat or Bear Form)
        768, // Cat Form (Shapeshift)
       5209, // Challenging Roar (Bear Form)
       1082, // Claw (Cat Form)
       8998, // Cower (Cat Form)
         99, // Demoralizing Roar (Bear Form)
       5229, // Enrage (Bear Form)
      16857, // Faerie Fire (Feral) (Feral)
      49376, // Feral Charge (Cat Form)
      16979, // Feral Charge (Bear Form)
      22568, // Ferocious Bite (Cat Form)
      33943, // Flight Form (Shapeshift)
      22842, // Frenzied Regeneration (Bear Form)
       6795, // Growl (Bear Form)
      33745, // Lacerate (Bear Form)
      22570, // Maim (Cat Form)
       6807, // Maul (Bear Form)
       9005, // Pounce (Cat Form)
       5215, // Prowl (Cat Form)
      80313, // Pulverize (Bear Form)
       1822, // Rake (Cat Form)
       6785, // Ravage (Cat Form)
       1079, // Rip (Cat Form)
      52610, // Savage Roar (Cat Form)
       5221, // Shred (Cat Form)
      80965, // Skull Bash (Cat Form)
      80964, // Skull Bash (Bear Form)
      77764, // Stampeding Roar (Cat Form)
      61336, // Survival Instincts (Cat or Bear Form)
      40120, // Swift Flight Form (Shapeshift)
      62078, // Swipe (Cat Form)
        779, // Swipe (Bear Form)
      77758, // Thrash (Bear Form)
       5217, // Tiger's Fury (Cat Form)
        783, // Travel Form (Shapeshift)
          0,
    },
    // Restoration tree, 13 abilities
    {
       5185, // Healing Touch
      33763, // Lifebloom
       1126, // Mark of the Wild
      17116, // Nature's Swiftness
      50464, // Nourish
      20484, // Rebirth
       8936, // Regrowth
        774, // Rejuvenation
       2782, // Remove Corruption
      18562, // Swiftmend
        740, // Tranquility
      33891, // Tree of Life (Shapeshift)
      48438, // Wild Growth
          0,
    },
    {
          0,
    },
  },
};

#define PTR_TREE_SPECIALIZATION_SIZE (4)

// Talent tree specialization abilities, wow build 13707
static unsigned __ptr_tree_specialization_data[][MAX_TALENT_TABS][PTR_TREE_SPECIALIZATION_SIZE] = {
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
      95859, // Meditation
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
      13852, // Ambidexterity
      13877, // Blade Flurry
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
      33167, // Absolution
      84732, // Enlightenment
      95860, // Meditation
      47540, // Penance
    },
    // Specialization abilities for Holy tree
    {
      33167, // Absolution
      88625, // Holy Word: Chastise
      95861, // Meditation
      87336, // Spiritual Healing
    },
    // Specialization abilities for Shadow tree
    {
      15407, // Mind Flay
      95740, // Shadow Orbs
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
      95862, // Meditation
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
      96429, // Disentanglement
      87305, // Gift of Nature
      85101, // Meditation
      18562, // Swiftmend
    },
  },
};

#define PTR_CLASS_MASTERY_ABILITY_SIZE (4)

// Class mastery abilities, wow build 13707
static unsigned __ptr_class_mastery_ability_data[][PTR_CLASS_MASTERY_ABILITY_SIZE] = {
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
    77486, // Shadow Orb Power
    77484, // Shield Discipline
        0,
  },
  // Class mastery abilities for Death Knight
  {
    77513, // Blood Shield
    77515, // Dreadblade
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
    77493, // Razor Claws
    77494, // Savage Defender
    77495, // Symbiosis
    77492, // Total Eclipse
  },
};

#define PTR_RACE_ABILITY_SIZE (6)
#ifndef CLASS_SIZE
#define CLASS_SIZE (12)
#endif

// Racial abilities, wow build 13707
static unsigned __ptr_race_ability_data[][CLASS_SIZE][PTR_RACE_ABILITY_SIZE] = {
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
      92682, // Explorer
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
    {     0, },
    {     0, },
    {     0, },
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
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Paladin class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Hunter class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Rogue class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Priest class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Death Knight class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Shaman class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Mage class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    // Racial abilities for Warlock class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
      69001, // Transform: Worgen
      68975, // Viciousness
          0,
    },
    {     0, },
    // Racial abilities for Druid class
    {
      68976, // Aberration
      68992, // Darkflight
      94293, // Enable Worgen Altered Form
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

#define PTR_GLYPH_ABILITIES_SIZE (18)

// Glyph spells for classes, wow build 13707
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
    // Major Glyphs (16 spells)
    {
      58366, // Glyph of Cleaving
      94374, // Glyph of Death Wish
      58357, // Glyph of Heroic Throw
      94372, // Glyph of Intercept
      58377, // Glyph of Intervene
      58097, // Glyph of Long Charge
      58372, // Glyph of Piercing Howl
      58355, // Glyph of Rapid Charge
      58356, // Glyph of Resonating Power
      63329, // Glyph of Shield Wall
      63325, // Glyph of Shockwave
      63328, // Glyph of Spell Reflection
      58387, // Glyph of Sunder Armor
      58384, // Glyph of Sweeping Strikes
      58098, // Glyph of Thunder Clap
      58382, // Glyph of Victory Rush
          0,
    },
    // Minor Glyphs (8 spells)
    {
      58095, // Glyph of Battle
      58096, // Glyph of Berserker Rage
      58369, // Glyph of Bloody Healing
      68164, // Glyph of Command
      58099, // Glyph of Demoralizing Shout
      58104, // Glyph of Enduring Victory
      63326, // Glyph of Furious Sundering
      63327, // Glyph of Intimidating Shout
          0,
    },
    // Prime Glyphs (9 spells)
    {
      63324, // Glyph of Bladestorm
      58367, // Glyph of Bloodthirst
      58388, // Glyph of Devastate
      58368, // Glyph of Mortal Strike
      58386, // Glyph of Overpower
      58370, // Glyph of Raging Blow
      58364, // Glyph of Revenge
      58375, // Glyph of Shield Slam
      58385, // Glyph of Slam
          0,
    },
  },
  // Glyph spells for Paladin
  {
    // Major Glyphs (18 spells)
    {
      63218, // Glyph of Beacon of Light
      54935, // Glyph of Cleansing
      54928, // Glyph of Consecration
      56414, // Glyph of Dazing Shield
      63223, // Glyph of Divine Plea
      54924, // Glyph of Divine Protection
      54939, // Glyph of Divinity
      54930, // Glyph of Focused Shield
      54923, // Glyph of Hammer of Justice
      54926, // Glyph of Hammer of Wrath
      56420, // Glyph of Holy Wrath
      57955, // Glyph of Lay on Hands
      54940, // Glyph of Light of Dawn
      54925, // Glyph of Rebuke
      63225, // Glyph of Salvation
      54931, // Glyph of Turn Evil
      54938, // Glyph of the Ascetic Crusader
      93466, // Glyph of the Long Word
    },
    // Minor Glyphs (6 spells)
    {
      57937, // Glyph of Blessing of Kings
      57958, // Glyph of Blessing of Might
      57979, // Glyph of Insight
      57954, // Glyph of Justice
      89401, // Glyph of Righteousness
      57947, // Glyph of Truth
          0,
    },
    // Prime Glyphs (11 spells)
    {
      54927, // Glyph of Crusader Strike
      54937, // Glyph of Divine Favor
      54934, // Glyph of Exorcism
      63219, // Glyph of Hammer of the Righteous
      63224, // Glyph of Holy Shock
      54922, // Glyph of Judgement
      54943, // Glyph of Seal of Insight
      56416, // Glyph of Seal of Truth
      63222, // Glyph of Shield of the Righteous
      63220, // Glyph of Templar's Verdict
      54936, // Glyph of Word of Glory
          0,
    },
  },
  // Glyph spells for Hunter
  {
    // Major Glyphs (16 spells)
    {
      56830, // Glyph of Bestial Wrath
      56851, // Glyph of Concussive Shot
      56850, // Glyph of Deterrence
      56844, // Glyph of Disengage
      56845, // Glyph of Freezing Trap
      56847, // Glyph of Ice Trap
      56846, // Glyph of Immolation Trap
      63068, // Glyph of Master's Call
      56833, // Glyph of Mending
      56829, // Glyph of Misdirection
      63086, // Glyph of Raptor Strike
      63069, // Glyph of Scatter Shot
      56836, // Glyph of Silencing Shot
      56849, // Glyph of Snake Trap
      56857, // Glyph of Trap Launcher
      56848, // Glyph of Wyvern Sting
          0,
    },
    // Minor Glyphs (5 spells)
    {
      57904, // Glyph of Aspect of the Pack
      57903, // Glyph of Feign Death
      57870, // Glyph of Mend Pet
      57866, // Glyph of Revive Pet
      57902, // Glyph of Scare Beast
          0,
    },
    // Prime Glyphs (10 spells)
    {
      56824, // Glyph of Aimed Shot
      56841, // Glyph of Arcane Shot
      63065, // Glyph of Chimera Shot
      56856, // Glyph of Dazzled Prey
      63066, // Glyph of Explosive Shot
      56842, // Glyph of Kill Command
      63067, // Glyph of Kill Shot
      56828, // Glyph of Rapid Fire
      56832, // Glyph of Serpent Sting
      56826, // Glyph of Steady Shot
          0,
    },
  },
  // Glyph spells for Rogue
  {
    // Major Glyphs (18 spells)
    {
      56813, // Glyph of Ambush
      56818, // Glyph of Blade Flurry
      91299, // Glyph of Blind
      63269, // Glyph of Cloak of Shadows
      56820, // Glyph of Crippling Poison
      56806, // Glyph of Deadly Throw
      56799, // Glyph of Evasion
      56803, // Glyph of Expose Armor
      63254, // Glyph of Fan of Knives
      56804, // Glyph of Feint
      56812, // Glyph of Garrote
      56809, // Glyph of Gouge
      56805, // Glyph of Kick
      56819, // Glyph of Preparation
      56798, // Glyph of Sap
      56811, // Glyph of Sprint
      63256, // Glyph of Tricks of the Trade
      89758, // Glyph of Vanish
    },
    // Minor Glyphs (6 spells)
    {
      58039, // Glyph of Blurred Speed
      58032, // Glyph of Distract
      58027, // Glyph of Pick Lock
      58017, // Glyph of Pick Pocket
      58038, // Glyph of Poisons
      58033, // Glyph of Safe Fall
          0,
    },
    // Prime Glyphs (12 spells)
    {
      56808, // Glyph of Adrenaline Rush
      56800, // Glyph of Backstab
      56802, // Glyph of Eviscerate
      56807, // Glyph of Hemorrhage
      63252, // Glyph of Killing Spree
      63268, // Glyph of Mutilate
      56814, // Glyph of Revealing Strike
      56801, // Glyph of Rupture
      63253, // Glyph of Shadow Dance
      56821, // Glyph of Sinister Strike
      56810, // Glyph of Slice and Dice
      63249, // Glyph of Vendetta
          0,
    },
  },
  // Glyph spells for Priest
  {
    // Major Glyphs (15 spells)
    {
      55675, // Glyph of Circle of Healing
      63248, // Glyph of Desperation
      55677, // Glyph of Dispel Magic
      63246, // Glyph of Divine Accuracy
      55684, // Glyph of Fade
      55678, // Glyph of Fear Ward
      55683, // Glyph of Holy Nova
      55686, // Glyph of Inner Fire
      55691, // Glyph of Mass Dispel
      55685, // Glyph of Prayer of Mending
      55688, // Glyph of Psychic Horror
      55676, // Glyph of Psychic Scream
      55690, // Glyph of Scourge Imprisonment
      55692, // Glyph of Smite
      63237, // Glyph of Spirit Tap
          0,
    },
    // Minor Glyphs (6 spells)
    {
      57985, // Glyph of Fading
      58009, // Glyph of Fortitude
      57987, // Glyph of Levitate
      57986, // Glyph of Shackle Undead
      58015, // Glyph of Shadow Protection
      58228, // Glyph of Shadowfiend
          0,
    },
    // Prime Glyphs (12 spells)
    {
      63229, // Glyph of Dispersion
      55679, // Glyph of Flash Heal
      63231, // Glyph of Guardian Spirit
      55673, // Glyph of Lightwell
      55687, // Glyph of Mind Flay
      63235, // Glyph of Penance
      55689, // Glyph of Power Word: Barrier
      55672, // Glyph of Power Word: Shield
      55680, // Glyph of Prayer of Healing
      55674, // Glyph of Renew
      55682, // Glyph of Shadow Word: Death
      55681, // Glyph of Shadow Word: Pain
          0,
    },
  },
  // Glyph spells for Death Knight
  {
    // Major Glyphs (13 spells)
    {
      58623, // Glyph of Anti-Magic Shell
      59332, // Glyph of Blood Boil
      58673, // Glyph of Bone Shield
      58620, // Glyph of Chains of Ice
      63330, // Glyph of Dancing Rune Weapon
      96279, // Glyph of Dark Succor
      62259, // Glyph of Death Grip
      63331, // Glyph of Hungering Cold
      58657, // Glyph of Pestilence
      58635, // Glyph of Pillar of Frost
      59327, // Glyph of Rune Tap
      58618, // Glyph of Strangulate
      58676, // Glyph of Vampiric Blood
          0,
    },
    // Minor Glyphs (6 spells)
    {
      58640, // Glyph of Blood Tap
      60200, // Glyph of Death Gate
      58677, // Glyph of Death's Embrace
      58680, // Glyph of Horn of Winter
      59307, // Glyph of Path of Frost
      59309, // Glyph of Resilient Grip
          0,
    },
    // Prime Glyphs (11 spells)
    {
      63333, // Glyph of Death Coil
      59336, // Glyph of Death Strike
      58629, // Glyph of Death and Decay
      58647, // Glyph of Frost Strike
      58616, // Glyph of Heart Strike
      63335, // Glyph of Howling Blast
      58631, // Glyph of Icy Touch
      58671, // Glyph of Obliterate
      58686, // Glyph of Raise Dead
      58669, // Glyph of Rune Strike
      58642, // Glyph of Scourge Strike
          0,
    },
  },
  // Glyph spells for Shaman
  {
    // Major Glyphs (15 spells)
    {
      55437, // Glyph of Chain Heal
      55449, // Glyph of Chain Lightning
      55452, // Glyph of Elemental Mastery
      55450, // Glyph of Fire Nova
      55443, // Glyph of Frost Shock
      59289, // Glyph of Ghost Wolf
      55441, // Glyph of Grounding Totem
      55456, // Glyph of Healing Stream Totem
      55440, // Glyph of Healing Wave
      63291, // Glyph of Hex
      55448, // Glyph of Lightning Shield
      63280, // Glyph of Shamanistic Rage
      63298, // Glyph of Stoneclaw Totem
      63270, // Glyph of Thunder
      55438, // Glyph of Totemic Recall
          0,
    },
    // Minor Glyphs (6 spells)
    {
      58058, // Glyph of Astral Recall
      58059, // Glyph of Renewed Life
      62132, // Glyph of Thunderstorm
      89646, // Glyph of Water Breathing
      58057, // Glyph of Water Walking
      58135, // Glyph of the Arctic Wolf
          0,
    },
    // Prime Glyphs (14 spells)
    {
      63279, // Glyph of Earth Shield
      55439, // Glyph of Earthliving Weapon
      63271, // Glyph of Feral Spirit
      55455, // Glyph of Fire Elemental Totem
      55447, // Glyph of Flame Shock
      55451, // Glyph of Flametongue Weapon
      55454, // Glyph of Lava Burst
      55444, // Glyph of Lava Lash
      55453, // Glyph of Lightning Bolt
      63273, // Glyph of Riptide
      55442, // Glyph of Shocking
      55446, // Glyph of Stormstrike
      55436, // Glyph of Water Shield
      55445, // Glyph of Windfury Weapon
          0,
    },
  },
  // Glyph spells for Mage
  {
    // Major Glyphs (13 spells)
    {
      56381, // Glyph of Arcane Power
      62126, // Glyph of Blast Wave
      56365, // Glyph of Blink
      56373, // Glyph of Dragon's Breath
      56380, // Glyph of Evocation
      56376, // Glyph of Frost Nova
      63095, // Glyph of Ice Barrier
      56372, // Glyph of Ice Block
      56374, // Glyph of Icy Veins
      56366, // Glyph of Invisibility
      70937, // Glyph of Mana Shield
      56375, // Glyph of Polymorph
      63091, // Glyph of Slow
          0,
    },
    // Minor Glyphs (6 spells)
    {
      57924, // Glyph of Arcane Brilliance
      57928, // Glyph of Conjuring
      63093, // Glyph of Mirror Image
      57925, // Glyph of Slow Fall
      57927, // Glyph of the Monkey
      52648, // Glyph of the Penguin
          0,
    },
    // Prime Glyphs (13 spells)
    {
      63092, // Glyph of Arcane Barrage
      62210, // Glyph of Arcane Blast
      56363, // Glyph of Arcane Missiles
      56364, // Glyph of Cone of Cold
      63090, // Glyph of Deep Freeze
      56368, // Glyph of Fireball
      56370, // Glyph of Frostbolt
      61205, // Glyph of Frostfire
      56377, // Glyph of Ice Lance
      89926, // Glyph of Living Bomb
      56383, // Glyph of Mage Armor
      56382, // Glyph of Molten Armor
      56384, // Glyph of Pyroblast
          0,
    },
  },
  // Glyph spells for Warlock
  {
    // Major Glyphs (14 spells)
    {
      56232, // Glyph of Death Coil
      63309, // Glyph of Demonic Circle
      56244, // Glyph of Fear
      56249, // Glyph of Felhunter
      56224, // Glyph of Healthstone
      56217, // Glyph of Howl of Terror
      63320, // Glyph of Life Tap
      56250, // Glyph of Seduction
      56240, // Glyph of Shadow Bolt
      63310, // Glyph of Shadowflame
      63312, // Glyph of Soul Link
      56226, // Glyph of Soul Swap
      56231, // Glyph of Soulstone
      56247, // Glyph of Voidwalker
          0,
    },
    // Minor Glyphs (7 spells)
    {
      58080, // Glyph of Curse of Exhaustion
      58070, // Glyph of Drain Soul
      58107, // Glyph of Enslave Demon
      58081, // Glyph of Eye of Kilrogg
      56238, // Glyph of Health Funnel
      58094, // Glyph of Ritual of Souls
      58079, // Glyph of Unending Breath
          0,
    },
    // Prime Glyphs (13 spells)
    {
      56241, // Glyph of Bane of Agony
      63304, // Glyph of Chaos Bolt
      56235, // Glyph of Conflagrate
      56218, // Glyph of Corruption
      56246, // Glyph of Felguard
      63302, // Glyph of Haunt
      56228, // Glyph of Immolate
      56248, // Glyph of Imp
      56242, // Glyph of Incinerate
      70947, // Glyph of Lash of Pain
      63303, // Glyph of Metamorphosis
      56229, // Glyph of Shadowburn
      56233, // Glyph of Unstable Affliction
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
  // Glyph spells for Druid
  {
    // Major Glyphs (18 spells)
    {
      63057, // Glyph of Barkskin
      54760, // Glyph of Entangling Roots
      94386, // Glyph of Faerie Fire
      94388, // Glyph of Feral Charge
      67598, // Glyph of Ferocious Bite
      62080, // Glyph of Focus
      54810, // Glyph of Frenzied Regeneration
      54825, // Glyph of Healing Touch
      54831, // Glyph of Hurricane
      54832, // Glyph of Innervate
      54811, // Glyph of Maul
      63056, // Glyph of Monsoon
      54821, // Glyph of Pounce
      54733, // Glyph of Rebirth
      54812, // Glyph of Solar Beam
      54828, // Glyph of Starfall
      57862, // Glyph of Thorns
      62970, // Glyph of Wild Growth
    },
    // Minor Glyphs (7 spells)
    {
      57856, // Glyph of Aquatic Form
      57858, // Glyph of Challenging Roar
      59219, // Glyph of Dash
      57855, // Glyph of Mark of the Wild
      62135, // Glyph of Typhoon
      57857, // Glyph of Unburdened Rebirth
      95212, // Glyph of the Treant
          0,
    },
    // Prime Glyphs (16 spells)
    {
      62969, // Glyph of Berserk
      54830, // Glyph of Insect Swarm
      94382, // Glyph of Lacerate
      54826, // Glyph of Lifebloom
      54813, // Glyph of Mangle
      54829, // Glyph of Moonfire
      54743, // Glyph of Regrowth
      54754, // Glyph of Rejuvenation
      54818, // Glyph of Rip
      63055, // Glyph of Savage Roar
      54815, // Glyph of Shred
      54845, // Glyph of Starfire
      62971, // Glyph of Starsurge
      54824, // Glyph of Swiftmend
      94390, // Glyph of Tiger's Fury
      54756, // Glyph of Wrath
          0,
    },
  },
};

#define PTR_TIER_BONUSES_SIZE (6)

// Tier item set bonuses for class, wow build 13707
static unsigned __ptr_tier_bonuses_data[][12][PTR_TIER_BONUSES_SIZE] = {
  {
    {
          0,
    },
  },
  // Tier bonuses for Warrior
  {
    {
          0,
    },
    // Tier 1 bonuses (3 spells)
    {
      21838, // Battlegear of Might - 5 Piece Bonus (Battlegear of Might)
      23561, // Battlegear of Might - 8 Piece Bonus (Enhanced Sunder Armor)
      23562, // Battlegear of Might - 3 Piece Bonus (Block Value 60)
          0,
    },
    // Tier 2 bonuses (3 spells)
    {
      21890, // Battlegear of Wrath - 5 Piece Bonus (Warrior's Wrath)
      23548, // Battlegear of Wrath - 8 Piece Bonus (Parry)
      23563, // Battlegear of Wrath - 3 Piece Bonus (Enhanced Battle Shout)
          0,
    },
    // Tier 3 bonuses (4 spells)
    {
      28842, // Dreadnaught's Battlegear - 6 Piece Bonus (Increased Hit Chance)
      28843, // Dreadnaught's Battlegear - 4 Piece Bonus ( Increased Spell Hit Chance)
      28844, // Dreadnaught's Battlegear - 2 Piece Bonus (Revenge)
      28845, // Dreadnaught's Battlegear - 8 Piece Bonus (Cheat Death)
          0,
    },
    // Tier 4 bonuses (4 spells)
    {
      37514, // Warbringer Armor - 2 Piece Bonus (Blade Turning)
      37516, // Warbringer Armor - 4 Piece Bonus (Revenge Bonus)
      37518, // Warbringer Battlegear - 2 Piece Bonus (Whirlwind Discount)
      37519, // Warbringer Battlegear - 4 Piece Bonus (Rage Bonus)
          0,
    },
    // Tier 5 bonuses (4 spells)
    {
      37522, // Destroyer Armor - 2 Piece Bonus (Shield Block Block Value)
      37525, // Destroyer Armor - 4 Piece Bonus (Battle Rush)
      37528, // Destroyer Battlegear - 2 Piece Bonus (Overpower Bonus)
      37535, // Destroyer Battlegear - 4 Piece Bonus (Bloodthirst and Mortal Strike Discount)
          0,
    },
    // Tier 6 bonuses (4 spells)
    {
      38398, // Onslaught Battlegear - 2 Piece Bonus (Increased Execute Damage)
      38399, // Onslaught Battlegear - 4 Piece Bonus (Improved Mortal Strike and Bloodthirst)
      38407, // Onslaught Armor - 4 Piece Bonus (Improved Shield Slam)
      38408, // Onslaught Armor - 2 Piece Bonus (Improved Commanding Shout)
          0,
    },
    // Tier 7 bonuses (4 spells)
    {
      60173, // Dreadnaught Plate - 2 Piece Bonus (Shield Slam Damage Up)
      60174, // Dreadnaught Battlegear - 2 Piece Bonus (Slam Damage Up)
      60175, // Dreadnaught Plate - 4 Piece Bonus (Shield Wall Duration)
      60176, // Dreadnaught Battlegear - 4 Piece Bonus (Bleed Cost Reduction)
          0,
    },
    // Tier 8 bonuses (4 spells)
    {
      64933, // Siegebreaker Plate - 2 Piece Bonus (Item - Warrior T8 Protection 2P Bonus)
      64936, // Siegebreaker Plate - 4 Piece Bonus (Item - Warrior T8 Protection 4P Bonus)
      64938, // Siegebreaker Battlegear - 2 Piece Bonus (Item - Warrior T8 Melee 2P Bonus)
      64939, // Siegebreaker Battlegear - 4 Piece Bonus (Item - Warrior T8 Melee 4P Bonus)
          0,
    },
    // Tier 9 bonuses (4 spells)
    {
      67234, // Hellscream's Battlegear - 2 Piece Bonus (Item - Warrior T9 Melee 2P Bonus (Berserker Stance and Battle Stance))
      67268, // Hellscream's Battlegear - 4 Piece Bonus (Item - Warrior T9 Melee 4P Bonus (Heroic Strike and Slam))
      67269, // Hellscream's Plate - 2 Piece Bonus (Item - Warrior T9 Tank 2P Bonus (Taunt))
      67273, // Hellscream's Plate - 4 Piece Bonus (Item - Warrior T9 Tank 4P Bonus (Shield Block))
          0,
    },
    // Tier 10 bonuses (4 spells)
    {
      70843, // Ymirjar Lord's Plate - 2 Piece Bonus (Item - Warrior T10 Protection 2P Bonus)
      70844, // Ymirjar Lord's Plate - 4 Piece Bonus (Item - Warrior T10 Protection 4P Bonus)
      70847, // Ymirjar Lord's Battlegear - 4 Piece Bonus (Item - Warrior T10 Melee 4P Bonus)
      70854, // Ymirjar Lord's Battlegear - 2 Piece Bonus (Item - Warrior T10 Melee 2P Bonus)
          0,
    },
    // Tier 11 bonuses (4 spells)
    {
      90293, // Earthen Warplate - 2 Piece Bonus (Item - Warrior T11 DPS 2P Bonus)
      90295, // Earthen Warplate - 4 Piece Bonus (Item - Warrior T11 DPS 4P Bonus)
      90296, // Earthen Battleplate - 2 Piece Bonus (Item - Warrior T11 Protection 2P Bonus)
      90297, // Earthen Battleplate - 4 Piece Bonus (Item - Warrior T11 Protection 4P Bonus)
          0,
    },
  },
  // Tier bonuses for Paladin
  {
    {
          0,
    },
    // Tier 1 bonuses (3 spells)
    {
      21747, // Lawbringer Armor - 8 Piece Bonus (Lawbringer)
      23564, // Lawbringer Armor - 3 Piece Bonus (Improved Judgement of Light)
      41784, // Lawbringer Armor - 5 Piece Bonus (Increased Critical 14)
          0,
    },
    // Tier 2 bonuses (3 spells)
    {
      23565, // Judgement Armor - 3 Piece Bonus (Increased Aura Radii)
      23591, // Judgement Armor - 8 Piece Bonus (Judgement)
      41782, // Judgement Armor - 5 Piece Bonus (Increase Spell Dam 47)
          0,
    },
    // Tier 3 bonuses (4 spells)
    {
      28774, // Redemption Armor - 4 Piece Bonus (Lay Hands)
      28775, // Redemption Armor - 2 Piece Bonus (Judgement of Light)
      28787, // Redemption Armor - 8 Piece Bonus (Cleanse)
      28789, // Redemption Armor - 6 Piece Bonus (Holy Power)
          0,
    },
    // Tier 4 bonuses (4 spells)
    {
      37184, // Justicar Armor - 2 Piece Bonus (Increased Right./Venge./Corruption)
      37185, // Justicar Armor - 4 Piece Bonus (Increased Holy Shield)
      37186, // Justicar Battlegear - 2 Piece Bonus (Increased Judgement of Crusader)
      37187, // Justicar Battlegear - 4 Piece Bonus (Increased Judgement of Command)
          0,
    },
    // Tier 5 bonuses (4 spells)
    {
      37190, // Crystalforge Armor - 2 Piece Bonus (Increased Retribution Aura)
      37191, // Crystalforge Armor - 4 Piece Bonus (Holy Shield Block Value)
      37194, // Crystalforge Battlegear - 2 Piece Bonus (Reduced Judgement Cost)
      37195, // Crystalforge Battlegear - 4 Piece Bonus (Judgement Group Heal)
          0,
    },
    // Tier 6 bonuses (4 spells)
    {
      38421, // Lightbringer Armor - 2 Piece Bonus (Improved Spiritual Attunement)
      38422, // Lightbringer Armor - 4 Piece Bonus (Improved Consecration)
      38424, // Lightbringer Battlegear - 4 Piece Bonus (Improved Hammer of Wrath)
      38427, // Lightbringer Battlegear - 2 Piece Bonus (Mana Regen Proc)
          0,
    },
    // Tier 7 bonuses (4 spells)
    {
      60149, // Redemption Plate - 2 Piece Bonus (Hammer of Righteousness Boost)
      60150, // Redemption Plate - 4 Piece Bonus (Divine Protection Duration)
      60152, // Redemption Battlegear - 2 Piece Bonus (Divine Storm Damage)
      60153, // Redemption Battlegear - 4 Piece Bonus (Judgment Cooldown Reduction)
          0,
    },
    // Tier 8 bonuses (4 spells)
    {
      64878, // Aegis Battlegear - 2 Piece Bonus (Item - Paladin T8 Retribution 2P Bonus)
      64879, // Aegis Battlegear - 4 Piece Bonus (Item - Paladin T8 Retribution 4P Bonus)
      64881, // Aegis Plate - 2 Piece Bonus (Item - Paladin T8 Protection 2P Bonus)
      64882, // Aegis Plate - 4 Piece Bonus (Item - Paladin T8 Protection 4P Bonus)
          0,
    },
    // Tier 9 bonuses (4 spells)
    {
      67186, // Turalyon's Plate - 2 Piece Bonus (Item - Paladin T9 Tank 2P Bonus)
      67187, // Turalyon's Plate - 4 Piece Bonus (Item - Paladin T9 Tank 4P Bonus)
      67188, // Turalyon's Battlegear - 2 Piece Bonus (Item - Paladin T9 Retribution 2P Bonus (Righteous Vengeance))
      67189, // Turalyon's Battlegear - 4 Piece Bonus (Item - Paladin T9 Retribution 4P Bonus (Judgement))
          0,
    },
    // Tier 10 bonuses (4 spells)
    {
      70758, // Lightsworn Plate - 2 Piece Bonus (Item - Paladin T10 Protection 2P Bonus)
      70761, // Lightsworn Plate - 4 Piece Bonus (Item - Paladin T10 Protection 4P Bonus)
      70762, // Lightsworn Battlegear - 4 Piece Bonus (Item - Paladin T10 Retribution 4P Bonus)
      70765, // Lightsworn Battlegear - 2 Piece Bonus (Item - Paladin T10 Retribution 2P Bonus)
          0,
    },
    // Tier 11 bonuses (6 spells)
    {
      90298, // Reinforced Sapphirium Battleplate - 2 Piece Bonus (Item - Paladin T11 Retribution 2P Bonus)
      90299, // Reinforced Sapphirium Battleplate - 4 Piece Bonus (Item - Paladin T11 Retribution 4P Bonus)
      90301, // Reinforced Sapphirium Battlearmor - 2 Piece Bonus (Item - Paladin T11 Protection 2P Bonus)
      90306, // Reinforced Sapphirium Battlearmor - 4 Piece Bonus (Item - Paladin T11 Protection 4P Bonus)
      90310, // Reinforced Sapphirium Regalia - 2 Piece Bonus (Item - Paladin T11 Holy 2P Bonus)
      90313, // Reinforced Sapphirium Regalia - 4 Piece Bonus (Item - Paladin T11 Holy 4P Bonus)
    },
  },
  // Tier bonuses for Hunter
  {
    {
          0,
    },
    // Tier 1 bonuses (3 spells)
    {
      21926, // Giantstalker Armor - 5 Piece Bonus (Nature's Ally)
      23560, // Giantstalker Armor - 3 Piece Bonus (Improved Mend Pet)
      23566, // Giantstalker Armor - 8 Piece Bonus (Improved Volley and Multishot)
          0,
    },
    // Tier 2 bonuses (3 spells)
    {
      21928, // Dragonstalker Armor - 5 Piece Bonus (Nature's Ally)
      23559, // Dragonstalker Armor - 3 Piece Bonus (Improved Aspect of the Hawk)
      23578, // Dragonstalker Armor - 8 Piece Bonus (Expose Weakness)
          0,
    },
    // Tier 3 bonuses (4 spells)
    {
      28751, // Cryptstalker Armor - 8 Piece Bonus (Multi-Shot)
      28752, // Cryptstalker Armor - 6 Piece Bonus (Adrenaline Rush)
      28755, // Cryptstalker Armor - 2 Piece Bonus (Rapid Fire)
      28756, // Cryptstalker Armor - 4 Piece Bonus (Stalker's Ally)
          0,
    },
    // Tier 4 bonuses (2 spells)
    {
      37484, // Demon Stalker Armor - 2 Piece Bonus (Improved Feign Death)
      37485, // Demon Stalker Armor - 4 Piece Bonus (Improved Multi-Shot)
          0,
    },
    // Tier 5 bonuses (2 spells)
    {
      37381, // Rift Stalker Armor - 2 Piece Bonus (Pet Healing)
      37505, // Rift Stalker Armor - 4 Piece Bonus (Improved Steady Shot)
          0,
    },
    // Tier 6 bonuses (2 spells)
    {
      38390, // Gronnstalker's Armor - 2 Piece Bonus (Improved Aspect of the Viper)
      38392, // Gronnstalker's Armor - 4 Piece Bonus (Improved Steady Shot)
          0,
    },
    // Tier 7 bonuses (2 spells)
    {
      60143, // Cryptstalker Battlegear - 2 Piece Bonus (Kill Command Pet Boost)
      60144, // Cryptstalker Battlegear - 4 Piece Bonus (Viper Attack Speed)
          0,
    },
    // Tier 8 bonuses (2 spells)
    {
      64854, // Scourgestalker Battlegear - 2 Piece Bonus (Item - Hunter T8 2P Bonus)
      64860, // Scourgestalker Battlegear - 4 Piece Bonus (Item - Hunter T8 4P Bonus)
          0,
    },
    // Tier 9 bonuses (2 spells)
    {
      67150, // Windrunner's Pursuit - 2 Piece Bonus (Item - Hunter T9 2P Bonus)
      67151, // Windrunner's Pursuit - 4 Piece Bonus (Item - Hunter T9 4P Bonus (Steady Shot))
          0,
    },
    // Tier 10 bonuses (2 spells)
    {
      70727, // Ahn'Kahar Blood Hunter's Battlegear - 2 Piece Bonus (Item - Hunter T10 2P Bonus)
      70730, // Ahn'Kahar Blood Hunter's Battlegear - 4 Piece Bonus (Item - Hunter T10 4P Bonus)
          0,
    },
    // Tier 11 bonuses (2 spells)
    {
      89923, // Lightning-Charged Battlegear - 2 Piece Bonus (Item - Hunter T11 2P Bonus)
      96411, // Lightning-Charged Battlegear - 4 Piece Bonus (Item - Hunter T11 4P Bonus)
          0,
    },
  },
  // Tier bonuses for Rogue
  {
    {
          0,
    },
    // Tier 1 bonuses (3 spells)
    {
      21874, // Nightslayer Armor - 3 Piece Bonus (Improved Vanish)
      21975, // Nightslayer Armor - 5 Piece Bonus (Vigor)
      23582, // Nightslayer Armor - 8 Piece Bonus (Clean Escape)
          0,
    },
    // Tier 2 bonuses (3 spells)
    {
      21881, // Bloodfang Armor - 3 Piece Bonus (Improved Poisons)
      23558, // Bloodfang Armor - 5 Piece Bonus (Improved Feint)
      23581, // Bloodfang Armor - 8 Piece Bonus (Bloodfang)
          0,
    },
    // Tier 3 bonuses (4 spells)
    {
      28811, // Bonescythe Armor - 6 Piece Bonus (Reduced Threat)
      28812, // Bonescythe Armor - 4 Piece Bonus (Head Rush)
      28814, // Bonescythe Armor - 8 Piece Bonus (Revealed Flaw)
      28816, // Bonescythe Armor - 2 Piece Bonus (Invigorate)
          0,
    },
    // Tier 4 bonuses (2 spells)
    {
      37167, // Netherblade - 2 Piece Bonus (Increased Slice and Dice Duration)
      37168, // Netherblade - 4 Piece Bonus (Finisher Combo)
          0,
    },
    // Tier 5 bonuses (2 spells)
    {
      37169, // Deathmantle - 2 Piece Bonus (Eviscerate and Envenom Bonus Damage)
      37170, // Deathmantle - 4 Piece Bonus (Free Finisher Chance)
          0,
    },
    // Tier 6 bonuses (2 spells)
    {
      38388, // Slayer's Armor - 2 Piece Bonus (Increased Slice and Dice Haste)
      38389, // Slayer's Armor - 4 Piece Bonus (Improved Backstab and Sinister Strike)
          0,
    },
    // Tier 7 bonuses (2 spells)
    {
      60162, // Bonescythe Battlegear - 2 Piece Bonus (Increase Rupture Damage)
      60163, // Bonescythe Battlegear - 4 Piece Bonus (Cheaper Combo Moves)
          0,
    },
    // Tier 8 bonuses (2 spells)
    {
      64914, // Terrorblade Battlegear - 2 Piece Bonus (Item - Rogue T8 2P Bonus)
      64915, // Terrorblade Battlegear - 4 Piece Bonus (Item - Rogue T8 4P Bonus)
          0,
    },
    // Tier 9 bonuses (2 spells)
    {
      67209, // VanCleef's Battlegear - 2 Piece Bonus (Item - Rogue T9 2P Bonus (Rupture))
      67211, // VanCleef's Battlegear - 4 Piece Bonus (Item - Rogue T9 4P Bonus (Hemorrhage, Sinister Strike, Backstab, and Mutilate))
          0,
    },
    // Tier 10 bonuses (2 spells)
    {
      70803, // Shadowblade's Battlegear - 4 Piece Bonus (Item - Rogue T10 4P Bonus)
      70805, // Shadowblade's Battlegear - 2 Piece Bonus (Item - Rogue T10 2P Bonus)
          0,
    },
    // Tier 11 bonuses (2 spells)
    {
      90460, // Wind Dancer's Regalia - 2 Piece Bonus (Item - Rogue T11 2P Bonus)
      90473, // Wind Dancer's Regalia - 4 Piece Bonus (Item - Rogue T11 4P Bonus)
          0,
    },
  },
  // Tier bonuses for Priest
  {
    {
          0,
    },
    // Tier 1 bonuses (3 spells)
    {
      21092, // Vestments of Prophecy - 5 Piece Bonus (Increased Critical Holy)
      21973, // Vestments of Prophecy - 3 Piece Bonus (Prophecy Flash Heal Bonus)
      23550, // Vestments of Prophecy - 8 Piece Bonus (Increased Prayer of Healing Criticals)
          0,
    },
    // Tier 2 bonuses (2 spells)
    {
      21853, // Vestments of Transcendence - 5 Piece Bonus (Reactive Fade)
      22010, // Vestments of Transcendence - 8 Piece Bonus (Greater Heal Renew)
          0,
    },
    // Tier 3 bonuses (4 spells)
    {
      28802, // Vestments of Faith - 8 Piece Bonus (Epiphany)
      28807, // Vestments of Faith - 2 Piece Bonus (Renew)
      28808, // Vestments of Faith - 6 Piece Bonus (Reduced Threat)
      28809, // Vestments of Faith - 4 Piece Bonus (Greater Heal)
          0,
    },
    // Tier 4 bonuses (2 spells)
    {
      37570, // Incarnate Regalia - 2 Piece Bonus (Improved Shadowfiend)
      37571, // Incarnate Regalia - 4 Piece Bonus (Improved Mind Flay and Smite)
          0,
    },
    // Tier 5 bonuses (2 spells)
    {
      37600, // Avatar Regalia - 2 Piece Bonus (Offensive Discount)
      37603, // Avatar Regalia - 4 Piece Bonus (Shadow Word Pain Damage)
          0,
    },
    // Tier 6 bonuses (2 spells)
    {
      38412, // Absolution Regalia - 4 Piece Bonus (Improved Mind Blast)
      38413, // Absolution Regalia - 2 Piece Bonus (Increased Shadow Word: Pain Duration)
          0,
    },
    // Tier 7 bonuses (2 spells)
    {
      60156, // Garb of Faith - 2 Piece Bonus (Mind Blast Cost Reduction)
      60157, // Garb of Faith - 4 Piece Bonus (Shadow Word Death Crit)
          0,
    },
    // Tier 8 bonuses (2 spells)
    {
      64906, // Sanctification Garb - 2 Piece Bonus (Item - Priest T8 Shadow 2P Bonus)
      64908, // Sanctification Garb - 4 Piece Bonus (Item - Priest T8 Shadow 4P Bonus)
          0,
    },
    // Tier 9 bonuses (2 spells)
    {
      67193, // Zabra's Regalia - 2 Piece Bonus (Item - Priest T9 Shadow 2P Bonus (Vampiric Touch))
      67198, // Zabra's Regalia - 4 Piece Bonus (Item - Priest T9 Shadow 4P Bonus (Mind Flay))
          0,
    },
    // Tier 10 bonuses (2 spells)
    {
      70800, // Crimson Acolyte's Regalia - 2 Piece Bonus (Item - Priest T10 Shadow 2P Bonus)
      70801, // Crimson Acolyte's Regalia - 4 Piece Bonus (Item - Priest T10 Shadow 4P Bonus)
          0,
    },
    // Tier 11 bonuses (4 spells)
    {
      89910, // Mercurial Vestments - 2 Piece Bonus (Item - Priest T11 Healer 2P Bonus)
      89911, // Mercurial Vestments - 4 Piece Bonus (Item - Priest T11 Healer 4P Bonus)
      89915, // Mercurial Regalia - 2 Piece Bonus (Item - Priest T11 Shadow 2P Bonus)
      89922, // Mercurial Regalia - 4 Piece Bonus (Item - Priest T11 Shadow 4P Bonus)
          0,
    },
  },
  // Tier bonuses for Death Knight
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
    {
          0,
    },
    // Tier 7 bonuses (4 spells)
    {
      60130, // Scourgeborne Plate - 2 Piece Bonus (Increased Plague Strike Crit)
      60132, // Scourgeborne Battlegear - 4 Piece Bonus (Oblit/Scourge Strike Runic Power Up)
      60133, // Scourgeborne Plate - 4 Piece Bonus (Icebound Duration Increase)
      60134, // Scourgeborne Battlegear - 2 Piece Bonus (Scourge/Oblit Crit Up)
          0,
    },
    // Tier 8 bonuses (4 spells)
    {
      64735, // Darkruned Battlegear - 2 Piece Bonus (Item - Death Knight T8 Melee 2P Bonus)
      64736, // Darkruned Battlegear - 4 Piece Bonus (Item - Death Knight T8 Melee 4P Bonus)
      64744, // Darkruned Plate - 2 Piece Bonus (Item - Death Knight T8 Tank 2P Bonus)
      64745, // Darkruned Plate - 4 Piece Bonus (Item - Death Knight T8 Tank 4P Bonus)
          0,
    },
    // Tier 9 bonuses (4 spells)
    {
      67115, // Thassarian's Battlegear - 2 Piece Bonus (Item - Death Knight T9 Melee 2P Bonus)
      67118, // Thassarian's Battlegear - 4 Piece Bonus (Item - Death Knight T9 Melee 4P Bonus)
      67119, // Thassarian's Plate - 2 Piece Bonus (Item - Death Knight T9 Tank 2P Bonus)
      67120, // Thassarian's Plate - 4 Piece Bonus (Item - Death Knight T9 Tank 4P Bonus)
          0,
    },
    // Tier 10 bonuses (4 spells)
    {
      70650, // Scourgelord's Plate - 2 Piece Bonus (Item - Death Knight T10 Tank 2P Bonus)
      70652, // Scourgelord's Plate - 4 Piece Bonus (Item - Death Knight T10 Tank 4P Bonus)
      70655, // Scourgelord's Battlegear - 2 Piece Bonus (Item - Death Knight T10 Melee 2P Bonus)
      70656, // Scourgelord's Battlegear - 4 Piece Bonus (Item - Death Knight T10 Melee 4P Bonus)
          0,
    },
    // Tier 11 bonuses (4 spells)
    {
      90454, // Magma Plated Battlearmor - 2 Piece Bonus (Item - Death Knight T11 Blood 2P Bonus)
      90456, // Magma Plated Battlearmor - 4 Piece Bonus (Item - Death Knight T11 Blood 4P Bonus)
      90457, // Magma Plated Battlegear - 2 Piece Bonus (Item - Death Knight T11 DPS 2P Bonus)
      90459, // Magma Plated Battlegear - 4 Piece Bonus (Item - Death Knight T11 DPS 4P Bonus)
          0,
    },
  },
  // Tier bonuses for Shaman
  {
    {
          0,
    },
    // Tier 1 bonuses (3 spells)
    {
      21895, // The Earthfury - 3 Piece Bonus (Increased Totem Radius)
      23572, // The Earthfury - 5 Piece Bonus (Mana Surge)
      23573, // The Earthfury - 8 Piece Bonus (Chain Healing Wave)
          0,
    },
    // Tier 2 bonuses (3 spells)
    {
      21899, // The Ten Storms - 3 Piece Bonus (Improved Chain Heal)
      23551, // The Ten Storms - 8 Piece Bonus (Lightning Shield)
      23570, // The Ten Storms - 5 Piece Bonus (Increased Critical Spell)
          0,
    },
    // Tier 3 bonuses (4 spells)
    {
      28818, // The Earthshatterer - 2 Piece Bonus (Totemic Energy)
      28821, // The Earthshatterer - 8 Piece Bonus (Lightning Shield)
      28823, // The Earthshatterer - 6 Piece Bonus (Totemic Power)
      29171, // The Earthshatterer - 4 Piece Bonus (Strong Current)
          0,
    },
    // Tier 4 bonuses (4 spells)
    {
      37212, // Cyclone Regalia - 2 Piece Bonus (Improved Flametongue Weapon)
      37213, // Cyclone Regalia - 4 Piece Bonus (Mana Cost Reduction)
      37223, // Cyclone Harness - 2 Piece Bonus (Improved Strength of Earth)
      37224, // Cyclone Harness - 4 Piece Bonus (Improved Storm Strike)
          0,
    },
    // Tier 5 bonuses (4 spells)
    {
      37228, // Cataclysm Regalia - 2 Piece Bonus (Healing Surge Discount)
      37237, // Cataclysm Regalia - 4 Piece Bonus (Lightning Bolt Discount)
      37239, // Cataclysm Harness - 2 Piece Bonus (Fast Healing Surge)
      37241, // Cataclysm Harness - 4 Piece Bonus (Improved Flurry)
          0,
    },
    // Tier 6 bonuses (4 spells)
    {
      38429, // Skyshatter Harness - 2 Piece Bonus (Shock Discount)
      38432, // Skyshatter Harness - 4 Piece Bonus (Stormstrike AP Buff)
      38436, // Skyshatter Regalia - 4 Piece Bonus (Improved Lightning Bolt)
      38443, // Skyshatter Regalia - 2 Piece Bonus (Totemic Mastery)
          0,
    },
    // Tier 7 bonuses (4 spells)
    {
      60164, // Earthshatter Garb - 2 Piece Bonus (Lightning Bolt Crit Mana Back)
      60165, // Earthshatter Garb - 4 Piece Bonus (Lava Burst Bonus Crit Damage)
      60168, // Earthshatter Battlegear - 2 Piece Bonus (Lightning Shield Boost)
      60169, // Earthshatter Battlegear - 4 Piece Bonus (Flurry Haste Speedup)
          0,
    },
    // Tier 8 bonuses (4 spells)
    {
      64916, // Worldbreaker Battlegear - 2 Piece Bonus (Item - Shaman T8 Enhancement 2P Bonus)
      64917, // Worldbreaker Battlegear - 4 Piece Bonus (Item - Shaman T8 Enhancement 4P Bonus)
      64925, // Worldbreaker Garb - 2 Piece Bonus (Item - Shaman T8 Elemental 2P Bonus)
      64928, // Worldbreaker Garb - 4 Piece Bonus (Item - Shaman T8 Elemental 4P Bonus)
          0,
    },
    // Tier 9 bonuses (4 spells)
    {
      67220, // Thrall's Battlegear - 2 Piece Bonus (Item - Shaman T9 Enhancement 2P Bonus (Static Shock))
      67221, // Thrall's Battlegear - 4 Piece Bonus (Item - Shaman T9 Enhancment 4P Bonus (Stormstrike))
      67227, // Thrall's Regalia - 2 Piece Bonus (Item - Shaman T9 Elemental 2P Bonus (Flame Shock))
      67228, // Thrall's Regalia - 4 Piece Bonus (Item - Shaman T9 Elemental 4P Bonus (Lava Burst))
          0,
    },
    // Tier 10 bonuses (4 spells)
    {
      70811, // Frost Witch's Regalia - 2 Piece Bonus (Item - Shaman T10 Elemental 2P Bonus)
      70817, // Frost Witch's Regalia - 4 Piece Bonus (Item - Shaman T10 Elemental 4P Bonus)
      70830, // Frost Witch's Battlegear - 2 Piece Bonus (Item - Shaman T10 Enhancement 2P Bonus)
      70832, // Frost Witch's Battlegear - 4 Piece Bonus (Item - Shaman T10 Enhancement 4P Bonus)
          0,
    },
    // Tier 11 bonuses (6 spells)
    {
      90494, // Vestments of the Raging Elements - 2 Piece Bonus (Item - Shaman T11 Restoration 2P Bonus)
      90499, // Vestments of the Raging Elements - 4 Piece Bonus (Item - Shaman T11 Restoration 4P Bonus)
      90501, // Battlegear of the Raging Elements - 2 Piece Bonus (Item - Shaman T11 Enhancement 2P Bonus)
      90502, // Battlegear of the Raging Elements - 4 Piece Bonus (Item - Shaman T11 Enhancment 4P Bonus)
      90503, // Regalia of the Raging Elements - 2 Piece Bonus (Item - Shaman T11 Elemental 2P Bonus)
      90505, // Regalia of the Raging Elements - 4 Piece Bonus (Item - Shaman T11 Elemental 4P Bonus)
    },
  },
  // Tier bonuses for Mage
  {
    {
          0,
    },
    // Tier 1 bonuses (3 spells)
    {
      23545, // Arcanist Regalia - 8 Piece Bonus (Subtlety)
      41656, // Arcanist Regalia - 5 Piece Bonus (Increased Spell Penetration 38)
      41657, // Arcanist Regalia - 3 Piece Bonus (Increase Spell Dam 18)
          0,
    },
    // Tier 2 bonuses (3 spells)
    {
      21879, // Netherwind Regalia - 3 Piece Bonus (Friendly Nukes)
      22007, // Netherwind Regalia - 8 Piece Bonus (Netherwind Focus)
      23549, // Netherwind Regalia - 5 Piece Bonus (Increased Area)
          0,
    },
    // Tier 3 bonuses (4 spells)
    {
      28761, // Frostfire Regalia - 8 Piece Bonus (Not There)
      28763, // Frostfire Regalia - 2 Piece Bonus (Evocation)
      28764, // Frostfire Regalia - 4 Piece Bonus (Adaptive Warding)
      28771, // Frostfire Regalia - 6 Piece Bonus (Elemental Vulnerability)
          0,
    },
    // Tier 4 bonuses (2 spells)
    {
      37438, // Aldor Regalia - 2 Piece Bonus (Pushback Resistance)
      37439, // Aldor Regalia - 4 Piece Bonus (Cooldown Reduction)
          0,
    },
    // Tier 5 bonuses (2 spells)
    {
      37441, // Tirisfal Regalia - 2 Piece Bonus (Improved Arcane Blast)
      37443, // Tirisfal Regalia - 4 Piece Bonus (Crit Bonus Damage)
          0,
    },
    // Tier 6 bonuses (2 spells)
    {
      38396, // Tempest Regalia - 2 Piece Bonus (Improved Evocation)
      38397, // Tempest Regalia - 4 Piece Bonus (Improved Fireball, Frostbolt, and Arcane Missiles)
          0,
    },
    // Tier 7 bonuses (2 spells)
    {
      60146, // Frostfire Garb - 4 Piece Bonus (Crit Strike Damage Percent)
      61062, // Frostfire Garb - 2 Piece Bonus (Improved Mana Gems)
          0,
    },
    // Tier 8 bonuses (2 spells)
    {
      64867, // Kirin Tor Garb - 2 Piece Bonus (Item - Mage T8 2P Bonus)
      64869, // Kirin Tor Garb - 4 Piece Bonus (Item - Mage T8 4P Bonus)
          0,
    },
    // Tier 9 bonuses (2 spells)
    {
      67164, // Sunstrider's Regalia - 2 Piece Bonus (Item - Mage T9 2P Bonus)
      67185, // Sunstrider's Regalia - 4 Piece Bonus (Item - Mage T9 4P Bonus (Fireball, Frostbolt, Frostfire Bolt, and Arcane Blast))
          0,
    },
    // Tier 10 bonuses (2 spells)
    {
      70748, // Bloodmage's Regalia - 4 Piece Bonus (Item - Mage T10 4P Bonus)
      70752, // Bloodmage's Regalia - 2 Piece Bonus (Item - Mage T10 2P Bonus)
          0,
    },
    // Tier 11 bonuses (2 spells)
    {
      90290, // Firelord's Vestments - 2 Piece Bonus (Item - Mage T11 2P Bonus)
      90291, // Firelord's Vestments - 4 Piece Bonus (Item - Mage T11 4P Bonus)
          0,
    },
  },
  // Tier bonuses for Warlock
  {
    {
          0,
    },
    // Tier 1 bonuses (3 spells)
    {
      21741, // Felheart Raiment - 5 Piece Bonus (Demonic Ally)
      23553, // Felheart Raiment - 8 Piece Bonus (Shadow Cost Reduction)
      23554, // Felheart Raiment - 3 Piece Bonus (Improved Drain Life)
          0,
    },
    // Tier 2 bonuses (3 spells)
    {
      21922, // Nemesis Raiment - 5 Piece Bonus (Demonic Ally)
      23555, // Nemesis Raiment - 8 Piece Bonus (Decreased Destruction Threat)
      32449, // Nemesis Raiment - 3 Piece Bonus (Increase Spell Dam 23)
          0,
    },
    // Tier 3 bonuses (4 spells)
    {
      28746, // Plagueheart Raiment - 6 Piece Bonus (Plagueheart)
      28829, // Plagueheart Raiment - 4 Piece Bonus (Corruption)
      28830, // Plagueheart Raiment - 8 Piece Bonus (Life Tap)
      28831, // Plagueheart Raiment - 2 Piece Bonus (Vampirism)
          0,
    },
    // Tier 4 bonuses (3 spells)
    {
      37377, // Voidheart Raiment - 2 Piece Bonus (Shadowflame)
      37380, // Voidheart Raiment - 4 Piece Bonus (Improved Corruption and Immolate)
      39437, // Voidheart Raiment - 2 Piece Bonus (Shadowflame Hellfire and RoF)
          0,
    },
    // Tier 5 bonuses (1 spells)
    {
      61992, // Corruptor Raiment - 4 Piece Bonus (Improved Corruption and Immolate (Updated))
          0,
    },
    // Tier 6 bonuses (2 spells)
    {
      38393, // Malefic Raiment - 4 Piece Bonus (Improved Shadow Bolt and Incinerate)
      38394, // Malefic Raiment - 2 Piece Bonus (Dot Heals)
          0,
    },
    // Tier 7 bonuses (2 spells)
    {
      60170, // Plagueheart Garb - 2 Piece Bonus (Corruption Triggers Crit)
      60172, // Plagueheart Garb - 4 Piece Bonus (Life Tap Bonus Spirit)
          0,
    },
    // Tier 8 bonuses (2 spells)
    {
      64931, // Deathbringer Garb - 2 Piece Bonus (Item - Warlock T8 2P Bonus)
      64932, // Deathbringer Garb - 4 Piece Bonus (Item - Warlock T8 4P Bonus)
          0,
    },
    // Tier 9 bonuses (2 spells)
    {
      67230, // Kel'Thuzad's Regalia - 2 Piece Bonus (Item - Warlock T9 2P Bonus)
      67231, // Kel'Thuzad's Regalia - 4 Piece Bonus (Item - Warlock T9 4P Bonus (Immolate, Corruption, and Unstable Affliction))
          0,
    },
    // Tier 10 bonuses (2 spells)
    {
      70839, // Dark Coven's Regalia - 2 Piece Bonus (Item - Warlock T10 2P Bonus)
      70841, // Dark Coven's Regalia - 4 Piece Bonus (Item - Warlock T10 4P Bonus)
          0,
    },
    // Tier 11 bonuses (2 spells)
    {
      89934, // Shadowflame Regalia - 2 Piece Bonus (Item - Warlock T11 2P Bonus)
      89935, // Shadowflame Regalia - 4 Piece Bonus (Item - Warlock T11 4P Bonus)
          0,
    },
  },
  {
    {
          0,
    },
  },
  // Tier bonuses for Druid
  {
    {
          0,
    },
    // Tier 1 bonuses (3 spells)
    {
      23556, // Cenarion Raiment - 8 Piece Bonus (Increased Tranquility and Hurricane Effect)
      23557, // Cenarion Raiment - 3 Piece Bonus (Improved Thorns Damage)
      39507, // Cenarion Raiment - 5 Piece Bonus (Increased Critical Spell)
          0,
    },
    // Tier 2 bonuses (3 spells)
    {
      21871, // Stormrage Raiment - 8 Piece Bonus (Increased Rejuvenation Duration)
      21872, // Stormrage Raiment - 5 Piece Bonus (Faster Regrowth Cast)
      21894, // Stormrage Raiment - 3 Piece Bonus (Meditation)
          0,
    },
    // Tier 3 bonuses (4 spells)
    {
      28716, // Dreamwalker Raiment - 2 Piece Bonus (Rejuvenation)
      28719, // Dreamwalker Raiment - 8 Piece Bonus (Healing Touch)
      28743, // Dreamwalker Raiment - 4 Piece Bonus (Dreamwalker)
      28744, // Dreamwalker Raiment - 6 Piece Bonus (Regrowth)
          0,
    },
    // Tier 4 bonuses (6 spells)
    {
      37295, // Malorne Regalia - 2 Piece Bonus (Mana Restore)
      37297, // Malorne Regalia - 4 Piece Bonus (Improved Innervate)
      37298, // Malorne Harness - 4 Piece Bonus (Bear Armor)
      37299, // Malorne Harness - 4 Piece Bonus (Cat Strength)
      37306, // Malorne Harness - 2 Piece Bonus (Bear Rage)
      37311, // Malorne Harness - 2 Piece Bonus (Cat Energy)
    },
    // Tier 5 bonuses (4 spells)
    {
      37315, // Nordrassil Harness - 2 Piece Bonus (Feral Regrowth Bonus)
      37324, // Nordrassil Regalia - 2 Piece Bonus (Moonkin Regrowth Bonus)
      37327, // Nordrassil Regalia - 4 Piece Bonus (Starfire Bonus)
      37333, // Nordrassil Harness - 4 Piece Bonus (Increased Shred and Lacerate)
          0,
    },
    // Tier 6 bonuses (4 spells)
    {
      38414, // Thunderheart Regalia - 2 Piece Bonus (Increased Moonfire Duration)
      38415, // Thunderheart Regalia - 4 Piece Bonus (Starfire Crit)
      38416, // Thunderheart Harness - 4 Piece Bonus (Improved Rip and Ferocious Bite)
      38447, // Thunderheart Harness - 2 Piece Bonus (Improved Mangle)
          0,
    },
    // Tier 7 bonuses (4 spells)
    {
      60138, // Dreamwalker Garb - 2 Piece Bonus (Insect Swarm Damage)
      60139, // Dreamwalker Garb - 4 Piece Bonus (Wrath/Starfire Crit)
      60141, // Dreamwalker Battlegear - 2 Piece Bonus (Rip Duration/Lacerate Damage)
      60142, // Dreamwalker Battlegear - 4 Piece Bonus (Barkskin Duration Increase)
          0,
    },
    // Tier 8 bonuses (4 spells)
    {
      64752, // Nightsong Battlegear - 2 Piece Bonus (Item - Druid T8 Feral 2P Bonus)
      64754, // Nightsong Battlegear - 4 Piece Bonus (Item - Druid T8 Feral 4P Bonus)
      64818, // Nightsong Garb - 2 Piece Bonus (Item - Druid T8 Balance 2P Bonus)
      64824, // Nightsong Garb - 4 Piece Bonus (Item - Druid T8 Balance 4P Bonus)
          0,
    },
    // Tier 9 bonuses (4 spells)
    {
      67121, // Runetotem's Battlegear - 2 Piece Bonus (Item - Druid T9 Feral 2P Bonus)
      67123, // Runetotem's Battlegear - 4 Piece Bonus (Item - Druid T9 Feral 4P Bonus)
      67125, // Runetotem's Regalia - 2 Piece Bonus (Item - Druid T9 Balance 2P Bonus (Moonfire))
      67126, // Runetotem's Regalia - 4 Piece Bonus (Item - Druid T9 Balance 4P Bonus (Starfire))
          0,
    },
    // Tier 10 bonuses (4 spells)
    {
      70718, // Lasherweave Regalia - 2 Piece Bonus (Item - Druid T10 Balance 2P Bonus)
      70723, // Lasherweave Regalia - 4 Piece Bonus (Item - Druid T10 Balance 4P Bonus)
      70724, // Lasherweave Battlegear - 2 Piece Bonus (Item - Druid T10 Feral 2P Bonus)
      70726, // Lasherweave Battlegear - 4 Piece Bonus (Item - Druid T10 Feral 4P Bonus)
          0,
    },
    // Tier 11 bonuses (6 spells)
    {
      90157, // Stormrider's Vestments - 2 Piece Bonus (Item - Druid T11 Restoration 2P Bonus)
      90158, // Stormrider's Vestments - 4 Piece Bonus (Item - Druid T11 Restoration 4P Bonus)
      90160, // Stormrider's Regalia - 2 Piece Bonus (Item - Druid T11 Balance 2P Bonus)
      90162, // Stormrider's Battlegarb - 2 Piece Bonus (Item - Druid T11 Feral 2P Bonus)
      90163, // Stormrider's Regalia - 4 Piece Bonus (Item - Druid T11 Balance 4P Bonus)
      90165, // Stormrider's Battlegarb - 4 Piece Bonus (Item - Druid T11 Feral 4P Bonus)
    },
  },
};

