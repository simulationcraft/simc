---
title: Characters
---
_This documentation is a part of the [TCI](TextualConfigurationInterface) reference._

**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**


# Declaration

**Warning**: Make sure to define your character BEFORE you define enemies (if you tinker with those). Otherwise some profileset simulations can potentially become funky.

## Manual
  * **death\_knight**, **demon_hunter**, **druid**, **hunter**, **mage**, **monk**, **paladin**, **priest**, **shaman**,  **rogue**, **warlock**, **warrior** (scope:new character; default: "") can be used to manually create a new character of the corresponding class.
```
 warrior=John
```

  * **copy** (scope: new character; default: "") can be used to copy a character to another one. The syntax is either `<new_char_name>[,<source_char_name>]`. If you omit the source character, SimulationCraft will use the current character.
```
 # This creates a copy of John, named John_evil_twin
 armory=us,illidan,john
 copy=john_evil_twin

 # This creates a copy of John, named John_evil_twin2
 copy=john_evil_twin2,john
```

## Importation: armory
  * **armory** (scope: new character; default: "") can be used to import a character from the armory. The syntax is `<region>,<server>,<player1>[,<player2>,<player3>,...]`. Inactive talent specs are imported with the spec=inactive option.
```
 # This will import John
 armory=us,illidan,john

 # Same here but we're importing the inactive talents spec.
 armory=us,illidan,john,spec=inactive

 # This will import Bill and Roger
 armory=eu,archimonde,bill,roger

 # Once the character has been imported, we can override some of his settings. For example, we can change his name and the total amount of strength on his gear now.
 gear_strength=20000
 name=john2
```

  * **guild** (scope: new characters; default: "") can be used to import many players of the same guild at once. The syntax is `<guild_name>[,options_list]`. The available options are:
    1. _region_ is the guild's region (eu, us, cn, tw, kr). If you omit this option, the application will rely on **default\_region** (but it must have been set before).
    1. _server_ is the guild's server name. If you omit this option, the application will rely on **default\_server** (but it must have been set before).
    1. _class_ can be used to force the application to only import players of this class. Acceptable values are "death\_knight", "druid", etc.
    1. _max\_rank_ can be used to specify a rank limit for players: only players whose rank is strictly greater than this limit will be imported.
    1. _ranks_ can be used to specify a list of ranks to import. The ranks must be separated by a "/".
    1. _cache_ can be set to non-zero to allow the application to check the cache for any recently downloaded copy of the armory pages. See also **http\_clear\_cache**.
```
 # Call with explicit region and server
 guild=willyoumaryme,region=us,server=illidan

 # Use " " if your guild name has spaces
 guild="will you mary me",region=us,server=illidan

 # Call with default region and server
 default_region=us
 default_server=illidan
 guild=willyoumaryme

 # Only import death knights with ranks 1, 3 or 4
 guild=willyoumaryme,ranks=1/3/4,class=death_knight
```

  * **default\_region** (scope: global; default: "us") is the default region to use for armory importation when the **guild** and **player** commands are used without explicitly specifying the region to use. This setting has to be set _before_ you use the **guild** or **player** commands.
  * **default\_server** (scope: global; default: "") is the default server to use for armory importation when the **guild** and **player** commands are used without explicitly specifying the server to use. This setting has to be set _before_ you use the **guild** or **player** commands.
```
 default_server=eitrigg
 default_region=eu
 guild=willyoumaryme
```

### From local JSON file
  * If you have your armory profile as a JSON file, you can directly import from it instead of connecting to the armory servers.
    * **local\_json** (scope: new character; default: "") can be used to import a character from the armory. The syntax is `local_json=mainfile,spec=specfile,equipment=equipmentfile`. Additionally, there's a `media=mediafile` suboption available if you want to have character backgrounds in the HTML report. `soulbinds=soulbindsfile` is available for Shadowlands covenants/soulbinds character data.

The files in question, are the sub-documents for a character profile that you get, as per information found in https://us.battle.net/forums/en/bnet/topic/20772457051


## Importation: other sources
  * **wowhead** (scope: new character; default: "") allows you to import a character from wowhead. The syntax is either `wowhead=<id>` or `wowhead=<region>,<server>,<playername1>[,<playername2>,...]`. If the player id or the player name are prefixed with an exclamation mark, the application will import the inactive talents spec.
```
 # This will import the character with ID 12359
 wowhead=12359

 # Same here but we're importing the inactive talents spec.
 wowhead=!12359

 # This will import John
 wowhead=us,illidan,john

 # Once the character has been imported, we can override some of his settings. For example, we can change his level and give him a new trinket now.
 level=85
 trinket2=darkmoon_card_hurricane,ilevel=359,quality=epic,stats=321str,equip=onattackhit_5000nature_1ppm

```

## Importation: alternative method

  * **player** (scope: new characters; default: "") allows you to specify the name of a character to import, either from wowhead or the armory. Here are the available options:
    1. _wowhead_ (default: "") allows you to specify the wowhead's id of the character to import. When empty, the character will be imported from the armory.
    1. _region_ (default: the current value of **default\_region**) allows you to specify the character's region to import a player from the armory. Wowhead importation is not affected.
    1. _server_ (default: the current value the **default\_server**) allows you to specify the character's server to import a player from the armory. Wowhead importation is not affected.
    1. _talents_ (default: "active") allows you to specify, when importing a character from wowhead, whether to use the active talents spec (must be "active") or the inactive one (muse be different from "active"). Armory importation is not affected: we will always import the active spec.
    1. _cache_ (default: 0), when different from zero, will force the application to first check the cache for any recently downloaded version of the page. Applies to both armory and wowhead importation. See also **http\_clear\_cache**.
```
 # Those lines will import characters from wowhead
 player=john,wowhead=1745
 player=bill,wowhead=4596,talents=inactive

 # This will import a character from the armory
 player=roger,region=us,server=illidan
```

## Importation: Caching
By default, Simulationcraft will import a fresh copy of your character profile on every simulation run incurring delay due to network latency. This behavior is controllable, see [Cache Control](CacheControl).

## Pets
  * **pet** (scope: new character) allows you to create a pet that owned by the current character. The pet will then become the new current character, see **active** to switch back to the owner. The syntax is either: `pet=<name>,<type>` OR `pet=<type>`. When the second syntax is used, the name will be the same as the type. Note that you do not have to specify talents for pets: if you don't, they will be automatically set with the relevant default template.
```
 # Import John the warlock and give him a felguard
 armory=us,illidan,john
 pet=felguard

 # Import Bill the hunter and give him two pets with their talents.
 armory=us,illidan,bill
 pet=cat,cat
 talents=200000030300003010122000000000000000000000000000000000000000000

 # Cat is now the current character, we have to switch back to Bill first before we can declare his devlisaur.
 active=owner
 pet=devilsaur,devilsaur
 talents=200000030300003010122000000000000000000000000000000000000000000
```

Here is the list of supported types:
 * Death knight: army\_of\_the\_dead\_ghoul\_8, bloodworms, dancing\_rune\_weapon, gargoyle, ghoul
 * Druid: treants
 * Mage: mirror\_image\_3, water\_elemental
 * Hunters: the list is too long, just use underscores instead of spaces. in doubt, search for the _create\_pet_ function's definition in the _sc\_hunter.cpp_ file
 * Paladin: guardian\_of\_ancient\_kings\_ret
 * Priest: shadow\_fiend
 * Shaman: spirit\_wolf, fire\_elemental
 * Warlock: felguard, felhunter, imp, succubus, voidwalker, infernal, doomguard, ebon\_imp

## Changing the current character

  * **active** (scope: global; default: _last created or imported character or pet_) allows you to change the current character. You can specify "owner" if the current character is a pet, a player's name, or "none" or "0" for selecting none. The current character is the character who is going to be affected by the next character-related settings. It does not affect the simulation, only the definition of the parameters of the simulation.
```
 # Let's import John, a warlock and give him a felguard
 armory=us,illidan,john
 pet=felguard

 # At this point, the felguard is now the current character. If we use the "name" setting, it will change the pet's name, not John's name. So we have to make John the active character again before we can rename John into JohnJohn.
 active=owner
 name=JohnJohn

 # Let's import Bill and make John the active character again
 armory=us,illidan,john
 active=john
```

# Specifications

## Basics
  * **race** (scope: current character; default: "") is the player's race. It may be one of the following:
    - Player Races: blood\_elf, draenai, dwarf, gnome, human, night\_elf, orc, tauren, troll, undead, goblin, worgen, pandaren, pandaren\_alliance, pandaren\_horde, void\_elf, highmountain\_tauren, lightforged\_draenei, nightborne, dark\_iron\_dwarf, maghar\_orc, zandalari\_troll, kul\_tiran, vulpera, mechagnome
    - NPC Races: none, aberration, beast, demon, dragonkin, elemental, giant, humanoid
  * **level** (scope: global; default: 70 Dragonflight, 80 The War Within) is the player's level.
```
 # Import a character and change his race
 armory=us,illidan,john
 level=80
 race=orc
```
  * **professions** (scope: current character; default: "") is the case-insensitive sequence of primary professions your character have. The professions are separated by a "/" and you can add as many of them as you want. Valid keywords are: alchemy, blacksmithing, enchanting, engineering, herbalism, inscription, jewelcrafting, leatherworking, mining, skinning, tailoring.
```
 professions=Blacksmithing=525/Jewelcrafting=525
```

  * **target** (scope: current character; default: "") is the name of the player's target for attacks when the action list doesn't specify otherwise. Note that this is not true multi-target support -- there's no way to switch targets mid-fight, even when the original target dies! Leave blank for the fight's main target. Pets inherit their owner's target by default.
```
 # Great news everyone! The duels are flowing again!
 alice.simc 
 target=bob 
 
 bob.simc 
 target=alice
```

## Talents
  * **talents** (scope: current character; default: "") accepts Blizzard's in-game generated talent export hash, from either the talent UI export or the Simulationcraft addon.

If you want to specify talents individually (as in a custom .simc file) and/or append to an existing hash defined as above, you can use tree-specific inputs for the class, spec, and hero trees.
1. These all take a `/` delimited list of pairs of talent:rank combinations.
2. When providing talent you can either use the Tokenized Name or the TalentID, i.e. `19979:1/shadowfiend:1` would give the Shadow Word: Death talent at rank 1 and the Shadowfiend talent at rank 1.
3. You must use TalentID for any talents that share a name with another talent in the same tree (class, spec, or hero).
  * **class_talents** (scope: current character; default: "") is the new class talent construct starting with Dragonflight.
```
# Manually defined class talents example
class_talents=19979:1/20024:1/shadowfiend:1/improved_shadowfiend:1/mindbender:1/rabid_shadows:2/shadowflame_prism:1/improved_mind_blast:2/power_infusion:1/twist_of_fate:2/mindgames:1/throes_of_pain:2/puppet_master:2/translucent_image:2/19944:2
```
  * **spec_talents** (scope: current character; default: "") is the new spec talent construct starting with Dragonflight.
```
# Manually defined spec talents example
spec_talents=mind_flay:1/vampiric_touch:1/devouring_plague:1/mind_sear:1/misery:1/fortress_of_the_mind:2/vampiric_insight:1/shadowy_apparitions:1/void_eruption:1/monomania:1/auspicious_spirits:1/hungering_void:1/ancient_madness:1/damnation:1/void_touched:2/void_torrent:1/shadow_crash:1/malediction:1/mental_fortitude:2/insidious_ire:2/sanguine_teachings:3/mind_devourer:2
```
  * **hero_talents** (scope: current character; default: "") is the new hero talent construct starting with The War Within. You can also pass the tokenized name of the hero tree to enable all talents in that hero tree, including both talents of all choice nodes.
```
# Manually defined hero talents example
hero_talents=thriving_growth:1/implant:1

# Enabling all hero talents of a tree
hero_talents=keeper_of_the_grove
```

## Optional
  * **name** (scope: current character; default: "") is the character name to be displayed in reports and logs.
```
 name=John
```
  * **origin** (scope: current character; default: "") is a special comment for the profile's origin. It has no use, it is just a comment. However, it will be mentioned in the reports and, if you specified an url, it will be presented as a link in html reports.
```
 origin="http://us.battle.net/wow/en/character/illidan/john/advanced"
```
  * **thumbnail** (scope: current character; default: none) is the URL for an image to display for this character in reports.
  * **distance** (scope: current character; default: 0) is the character's distance, in yard, from the boss. When left to zero, Simulationcraft will dynamically assign it depending on your class: 3 (demon hunter), 5 (most melee), 8 (shadow priest), 10 (arcane mage), 30 (druid, elem shaman, non-arcane mage, warlock), and 40 (non-survival hunter). Pets will be at the same distance with a few exceptions (shaman's fire elemental will move to melee). This setting will be used for computing spells flight time and for distance conditions on raid events (see **raid\_event**).
```
 # Let's put the players 20 yards away from the target.
 distance=20
```
  * **position** (scope: current character; default: back/ranged\_back, tanks are front) is the character's position in relation to the boss. When not defined, players will automatically be assigned back, hunters ranged\_back, and tanks front. Being in front of the target causes the mob to be able to block and parry your attacks.
```
 # Available options: front, back, ranged_front, ranged_back
 # Simulate MajorDomo Scorpion Form
 position=front
```
  * **comment** (scope: current character; default: "") will be used when you export profiles. This setting's value will be displayed as a comment (using `#`) at the beginning of the exported profiles. It has no other uses (not displayed in reports, etc).
```
 comment=Generated by importmytoon.simc
```
  * **use\_pre\_potion** (scope: churrent character; default: 1), when different from zero, will allow the player to use a potion just before he enters combat, so that he can use another one later during the fight. In order to effectively use the potion before the combat, the actions list must contain a compatible action (no restriction such as being in combat, or target's health percentage lesser than 100%, etc). This should be the case for all default actions lists for all classes and specs.
```
 armory=us,illidan,john
 use_pre_potion=1

 # This is a part of the default warrior's actions list: use a potion either before combat, or during the bloodlust.
 <beginning of the list>
 actions+=/golemblood_potion,if=!in_combat|buff.bloodlust.react
 <end of the list>
```
  * **id** (scope: current character; default: "") can be used to specify the GUID to display in combat logs (see **combat\_log**). When left empty, the application will generate a player GUID based on the characters declaration order.
```
 # Give another GUID to John
 armory=us,illidan,john
 id=0x00000000012729FD
```
  * **timeofday** (scope: current character; default: "nighttime") can be used to specify the InGame Time of Day, relevant for the WoD Night Elf racial. Available options are:
    1. night / nighttime
    1. day / daytime
```
 # Set timeofday to 'daytime'
 timeofday=daytime
```

  * **zandalari_loa** (scope: current character; default: "paku") can be used to specify the Embrace of the Loa racial, relevant for BfA Zandalari Trolls. Available options are:
    1. akunda / embrace_of_akunda
    1. bwonsamdi / embrace_of_bwonsamdi
    1. gonk / embrace_of_gonk
    1. kimbul / embrace_of_kimbul
    1. kragwa / embrace_of_kragwa
    1. paku / embrace_of_paku
```
 # set loa to kimbul
 zandalari_loa=kimbul
```

  * **earthen_mineral** (scope: current character; default: "ruby") can be used to specify the Earthen racial. Available options are:
    1. amber (stamina)
    1. emerald (haste)
    1. onyx (mastery)
    1. sapphire (vers)
    1. ruby (critical strike)
```
 # set earthen to onyx
 earthen_mineral=onyx
```

## Status
  * **sleeping** (scope: current character; default: 0), when different from zero, will make the character inactive: he won't do anything. The purpose of this setting if to allow you to quickly remove a character from the simulation without removing everything about him in your .simc files.
```
 armory=us,illidan,john
 sleeping=1
```
  * **quiet** (scope: current character; default: 0), when different from zero, will remove data about this character from the reports. The player will still take part into the simulation though, and he will perform his actions as usual.
```
 # Let's say John is a shadowpriest and we want to study the effect of dark intent on him.
 armory=us,illidan,john

 # Now let's say Bill is a warlock and let's make him cast dark intent on John
 armory=us,illidan,bill
 quiet=1
 actions=/dark_intent,target=john
 # Insert the rest of actions here
```

## Skill
  * **skill** (scope: character; default: the value of **default\_skill** when the character was created) allows you to specify the player's skill. A 1.0 skill means a perfect player who never do any mistake. A 0.8 skill means a player who have a 20% chance anytime he should perform an action to think it's not actually ready and skip to the next action in list, or even the one after (20% chance on every level).
```
 # John is such a bad player...
 armory=us,illidan,john
 skill=0.6
```
  * **default\_skill** (scope: ulterior characters; default: 1.0) is the default skill for ulterior characters.
```
 # John will have the default skill (1.0)
 armory=us,illidan,john

 # Bill and Roger will have a 0.8 skill
 default_skill=0.8
 armory=us,illidan,bill
 armory=us,illidan,roger
```

## Role
The character's role will affect different settings. Here are some examples, the list may not be exhaustive and, when using edge cases, we encourage you to carefully read the report in order to check it matches your expectations.:
  1. The default actions list will be changed: if you import a healer and set a dps role, Simulationcraft will make the default actions list damages-oriented.
  1. A tank player will be placed in front of the boss, rather than behind. He will also suffer damages from the target (100k - 120k on every 3s), that will result in rage for druids and warriors.
  1. A healer player will report healing-related statistics, such as hps (although it will still be labeled "dps").

  * **role** (scope: current character; default: "unknown") allows you to specify the character's role. Accepted values are "dps", "heal" and "tank". Other values will force Simulationcraft to pick a role based on your spec. Note that the actual character's role will depend on your spec: Simulationcraft does not know how to force every spec in every role and, as a result, may ignore the role you specified and pick one more appropriate for the character's spec.
```
 armory=us,illidan,john
 role=tank
```

## Bugs reproduction
  * **bugs** (scope: current character; default: 1), when different from 0, forces the application to reproduce the bugs observed on the live servers and related to the character's class.
```
 # Let's say that some major bug will be fixed soon(tm) by Blizzard, so we want to see what will be our performances after that.
 armory=us,illidan,john
 bugs=0
```

## Schedule Ready
There are two types of scheduling a ready event for a player:

- Poll based (**default**): The action priority list is checked every 100ms until a action is ready.

- Event based: When no action is ready, the player isn't scheduled for another round of cycling through the action list. Instead he waits until a specific "trigger-event" brings the player back to life, at which point the action list is checked again.

Those events may be: Cooldown expiration, dot ticking "near" expiration, buff triggering, unique item triggering, etc.

To activate the event based system, use
> _**ready\_trigger** (scope: current character; default: false )_

# Modifying the total stats

## Challenge Modes
**challenge\_mode** (scope: global; default: false) activates down scaling of the gear for challenge mode dungeons in Warlords of Draenor. Based on the difference in "power" of the current and the target item level (620) primary/secondary stats and weapon damage are scaled down. Scaling results in WoD should match perfectly in-game, but if you see a difference we would appreciate if you submit an issue providing details on it.

## Timewalking
**Timewalk** (scope: global; default: 0) enables player and gear scaling as you would find in a Timewalking dungeon. The option should be set to the player level to scale to (Eg: timewalk=80) which will automatically set the appropriate gear scaling, while allowing characters to keep all of their high level talents.

## Disable Set Bonuses
  * **disable\_set\_bonuses** (scope: global; default: false ) will deactivate all set bonuses in the simulation.

> - Added in 610-02 Release -
> The following options will accept a tier number, and will either enable/disable all of that tiers set bonus.
  * **enable\_2\_set** (scope: global; default: 0 )
  * **enable\_4\_set** (scope: global; default: 0 )
  * **disable\_2\_set** (scope: global; default: 0 )
  * **disable\_4\_set** (scope: global; default: 0 )

```
Example:
disable_2_set=17
enable_2_set=18
# This will enable all T18 2PC bonuses in the simulation, while also disabling all T17 2P bonuses.
```

## Overriding the gear contribution

  * **gear\_strength** (scope: global; default: _total gear contribution_) is the total contribution of your gear to strength, including the gear itself along with its gems, enchants, reforging, etc. Note that gear\_strength=0 cannot be used to disable strength contribution from gear. For now, you will have to use 1 to simulate almost the same effect.
```
 # With this line, your character will have almost as much strength as a naked character would have. Other stats, strength-based procs, weapon damages and speed, etc, will remain unchanged.
 gear_strength=1
```

The available keywords are:
 1. Resources: gear\_health, gear\_mana, gear\_rage, gear\_energy, gear\_focus, gear\_runic
 1. Primary: gear\_strength, gear\_agility, gear\_intellect, gear\_stamina, gear\_spirit
 1. Secondary: gear\_mastery\_rating, gear\_haste\_rating, gear\_versatility\_rating, gear\_crit\_rating, gear\_spell\_power, gear\_attack\_power, gear\_hit\_rating, gear\_expertise\_rating
 1. Defensive: gear\_armor, gear\_parry\_rating, gear\_dodge\_rating, gear\_bonus\_armor
 1. Battle for Azeroth specific: gear\_corruption, gear\_corruption\_resistance

## Bonuses and maluses

  * **enchant\_strength** (scope: current character; default: 0) is the bonus (or malus when negative) added to the current character's strength. See it as a fake enchant.
  * **default\_enchant\_strength** (scope: global; default: 0) is the bonus (or malus when negative) added to all characters' strength. See it as a fake enchant. It won't apply to pets.
```
 # Let's import John and Bill. John will be the new current character
 armory=us,illidan,bill
 armory=us,illidan,john

 # Let's subtract 150 strength to John
 enchant_strength=-150

 # Let's now add 500 strength to John and Bill. John now has a total bonus of 350 strength.
 default_enchant_strength=350
```

The available keywords are:
 1. Resources: enchant\_health, enchant\_mana, enchant\_rage, enchant\_energy, enchant\_focus, enchant\_runic
 1. Primary: enchant\_strength, enchant\_agility, enchant\_intellect, enchant\_stamina, enchant\_spirit
 1. Secondary: enchant\_mastery\_rating, enchant\_haste\_rating, enchant\_versatility\_rating, enchant\_crit\_rating, enchant\_spell\_power, enchant\_attack\_power, enchant\_hit\_rating, enchant\_expertise\_rating
 1. Other: enchant\_armor, enchant\_bonus\_armor, enchant\_leech\_rating, enchant\_run\_speed\_rating
 1. Battle for Azeroth specific: enchant\_corruption, enchant\_corruption\_resistance

## Parties
  * **party** (scope: global) can be used to group players in parties. Parties will be used for any effect that is restricted to party members rather than raid-wide. Each occurrence of the party option will create a new party containing the named players and their pets. Characters that do not appear in any party option are all grouped together in a default party that is not limited in size.
```
 # John will be in the default party 0
 armory=us,illidan,john
 
 # Bill and Roger will be in party 1
 armory=us,illidan,bill
 armory=us,illidan,roger
 party=bill,roger

 # Lucy will be in party 2
 armory=us,illidan,lucy
 party=lucy
```

## Modifying Flask/Food/Potion/Rune

By default, SimC will use all consumables. The default is determined by the class module.

* Flask
```
flask=whispered_pact
```
* Food
```
food=lemon_herb_filet
```
* Potion
```
potion=prolonged_power
```
* Rune
```
augmentation=defiled
```

* Temporary Enchant
```
temporary_enchant=main_hand:shadowcore_oil/off_hand:shadowcore_oil
```

**Since Simulationcraft 10.0.5.48317**: You can also add an `if=` option to the temporary enchant.
The option will be parsed in the player scope. Any action-based expressions will not work, and more 
generally any expressions that rely on state that changes during iteration is likely going to fail,
since the expression is evaluated relatively early in the actor initialization process.

If multiple temporary enchants match for a given slot, the first available one is selected. An enchant
is considered available if it has no expression to evaluate, or if the evaluation of a given expression
results in a non-zero value.

```
temporary_enchant=main_hand:howling_rune_3,if=!talent.improved_flametongue_weapon
```

To disable, use the `disabled` text:

```
potion=disabled
food=disabled
flask=disabled
augmentation=disabled
temporary_enchant=disabled
```
