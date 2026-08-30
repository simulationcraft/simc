---
title: Equipment
---
_This documentation is a part of the [TCI](TextualConfigurationInterface) reference._

**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Declaring items

## Syntax
Equipment pieces are declared through the following syntax: `<slot>=<item_name>[,<option1=value1>,<option2=value2>,...]` The item name is used by replacing any white spaces with underscores. Note that some names have built-in support: for example, the Shadowmourne proc is hardcoded into Simulationcraft and you don't need to specify it. For example:
```
 # The following line is valid but it only adds a dummy helmet with no stats.
 head=earthen_helmet

 # This will import a earthen helmet (id=60325), an epic plate helm, with a 359 ilevel.
 # Its stats are 2784 armor, 512 stamina, 281 strength, 228 haste and 168 critical strike.
 # It has a a destructive shadowspirit, along with a 20 crit + 20 str gem, and a 30 str gemming bonus (the bonus is only mentioned when active).
 # Finally, it has a 60 strength + 35 mastery enchant.
 head=earthen_helmet,type=plate,ilevel=359,quality=epic,stats=2784armor_168crit_228haste_512sta_281str,gems=destructive_shadowspirit_20crit_20str_30str,enchant=60str_35mastery

 # We can also provide the item's ID, this will force the application to query the stats from the armory. We only have to specify how we customized it: gems, enchant, reforging.
 head=earthen_helmet,id=60325,gems=destructive_shadowspirit_20crit_20str_30str,enchant=60str_35mastery

```

## Slots keywords
Acceptable slots are:
  * **meta\_gem**
  * **head**
  * **neck**
  * **shoulder**, **shoulders**
  * **back**
  * **shirt**
  * **chest**
  * **waist**
  * **wrist**, **wrists**
  * **hand**, **hands**
  * **legs**
  * **feet**
  * **finger1**, **ring1**, **finger2**, **ring2**
  * **trinket1**, **trinket2**
  * **main\_hand**, **off\_hand**
  * **ranged**
  * **tabard**

## Options
<a />
### Importing stats through ID
  * _id_ (default: 0) can be used to specify the item's id and force the application to query the stats, the quality and armory type from the items importation sources (see the [Item Data Importation](#Item_Data_Importation) section). The returned parameters will replace those you may have specified.
```
 # Let's import our helm's stats
 head=earthen_helmet,id=60325
```

  * _source_ (default: "") can be used to override the **item\_db\_source** global setting for this item (see the [Item Data Importation](#Item_Data_Importation) section). It uses the same syntax.
```
 # Let's import our helm's stats from mmo-champion, or local if mmo-champion doesn't work.
 head=earthen_helmet,id=60325,source=mmoc|local
```

### Basic properties
  * _quality_ can be used to specify the item's quality. All values are accepted but, for random enchants, it has to be one of "rare", "epic" or "uncommon" i order to figure out the stats values.
```
 tabard=some_tabard,quality=epic
```
  * _ilevel_ is the item level. It is especially important for random enchants, in order to figure out the stats values.
```
 tabard=some_tabard,ilevel=359
```
  * _type_ is the armor type, it should be one of "cloth", "leather", "mail", "plate". It is used to check whether the character is eligible for the armor specialization bonus (+5% strength for a warrior only wearing plate for example).
```
 head=some_helm,type=plate
```
  * _lfr_, when set to 1, flags the item as a looking for raid item difficulty. It is used in conjunction with special effects to determine the power of the special effect on that item, typically on trinkets.
```
 head=some_casual_item,lfr=1
```
  * _heroic_, when set to 1, flags the item as heroic. It is used in conjunction with _addon_ when the application has to decide between the heroic or normal proc.
```
 head=some_helm,heroic=1
```
  * _warforged_, when set to 1, flags the item as an elite item, e.g., thunderforged or warforged items. It is used in conjunction with special effects to determine the power of the special effect on that item, typically on trinkets.
```
 head=helm_of_complete_doom_and_despair,warforged=1
```
  * _mythic_, when set to 1, flags the item as a mythic item. It is used in conjunction with special effects to determine the power of the special effect on that item, typically on trinkets.
```
 head=helm_of_complete_doom_and_despair,mythic=1
```
  * _upgrade_, when set to 1 or 2, applies a level of upgrade on the piece of gear (value times 4 for epic/legendary, value times 8 for rare). Simc offers two different ways to upgrade items:
    1. If an _id_ option is given, _stats_ are not given, and the item can be found from the local item database (_source_ option is empty, or contains _local_, and the item is found), the simulator will use a precise upgrade formula.
    1. If no _id_ option is provided, or the item stats are given through the _stats_ option, or the item is not found in the local database, or the local database is not used, the simulator uses an approximate formula for item upgrades, which may result in stats that are a couple of points off. **This method _requires_ that _quality_, and _ilevel_ parameters are set on the item, either through the _id_ option based item data parsing, or by the specific options.**
> Also note that you may use the upgrade option to upgrade an item more than two times. In this case, the simulator has a global ilevel cap of 1000.
```
 head=darkfang_mask,id=105542,upgrade=2
```

### Stats
  * _stats_ is the sequence describing the stats. The syntax is : `<value1><stat1>[_<value2><stat2>_...]`. For example: "500sta\_250str". Note that manually specified stats **will override** any stats queried through the _id_ option.
```
 head=earthen_helmet,stats=2784armor_168exp_228haste_512sta_281str
```
  * _weapon_ is used to specify the weapons' damage ranges. It is a sequence of options. Syntax is: `weapon=<type>[_<option1>=<value1>...]`. Type can be one of: dagger, fist, beast, beast2h, sword, sword2h, mace, mace2h, axe, axe2h, staff, polearm, bow, gun, crossbow, thrown. Additional types for wands are: arcane, bleed, chaos, fire, frost, frostfire, holy, nature, physical, shadow, spellstorm, shadowfrost, shadowflame, drain. Options are:
    1. _speed_ (mandatory) is the weapon's speed, in seconds.
    1. _min_ and _max_ are the damages range. When not set, they will be inferred from _dps_ and _speed_ (constant damages weapon) or, in last resort, from _dmg_ or _damage_.
    1. _dps_ is the weapon's dps. It is not used when _min_ and _max_ are set.
    1. _dmg_ (or _damage_) is the weapon's constant damages. It is not used when _min_ and _max_, or _dps_, are set.
```
 # Here is a 4.0s and 500dps weapon definition with min and max.
 main_hand=some_axe,weapon=axe2h_4.0speed_1750min_2250max

 # Same axe but it will do constant damages of 2000 points.
 main_hand=some_axe,weapon=axe2h_4.0speed_500dps

 # Alernative syntax for the constant damages weapon
 main_hand=some_axe,weapon=axe2h_4.0speed_2000dmg
```

### Gems
  * _gems_ is the sequence of bonuses brought by the gems and gemming bonuses. The syntax is `[<metaprefix>_<metasuffix>][_<value1><stat1>...]`. For example: "destructive\_shadowspirit\_40str" for a destructive shadowspirit diamong (meta) and a 40 strength gem. Use this chain to include the gemming bonus when it is activated.
```
 # Here is the definition for a earthen helmet.
 # Meta socket: destructive shadowspirit diamond
 # Yellow socket: 20crit + 20str orange gem
 # Gemming bonus: 30str
 head=earthen_helmet,gems=destructive_shadowspirit_20crit_20str_30str
```

### Enchants
  * _enchant_ can be either a stats sequence (using the same pattern than _stats_ or _gems_)  or one of the recognized keywords, such as "landslide", "lightweave\_embroidery", "power\_torrent", etc.
```
 # We can use keywords
 main_hand=shalugdoom_the_axe_of_unmaking,enchant=landslide

 # Note that post-profession revamp from Dragonflight, keywords will need to be followed by the numerical tier of the enchant
 main_hand=shalugdoom_the_axe_of_unmaking,enchant=burning_writ_3

 # Or specify a custom 500 strength enchant
 main_hand=shalugdoom_the_axe_of_unmaking,enchant=500str
```

### Embellishments
  * _embellishment_ will accept a tokenized name of the embellishment you want to add to the item. Note that there are no restrictions; you can add any embellishment to any item, including items that already have an embellishment via bonus id.
```
 # add the 'Blue Silken Lining' embellishment to the Dragonflight tailoring crafted cloak
 back=vibrant_wildercloth_shawl,id=193511,embellishment=blue_silken_lining
```

### Procs
By default, custom on-use effects on items will have a 20 second shared cooldown. This default shared cooldown can be adjusted for all items with the _default_item_group_cooldown_ player option.
  * _use_ allows you to specify on-use effects. The syntax is: `<value1><param1>[_<value2><param2>...]`. The parameters can be:
    * Any school abbreviation, for  damages effects. For example: "451physical".
    * Any stat abbreviation, for stats gains. For example: "1500str".
    * _cd_ or _cooldown_ for a cooldown. For example: "45cd" for a 45s cooldown.
    * _dur_ or _duration_ for a duration. For example: "15dur" for a 15s duration.
    * _stack_ or _stacks_ for a maximum stack for a buff
    * _aoe_ to specify the number of enemies to hit, -1 for all enemies
    * _driver_ to specify the on-use spell for the item. If a spell is given, the system attempts to automatically detect what kind of buff (or damage spell) to make.
    * _tick_ can be used to specify the period of the ticks for the effect
    * _reverse_ can be used to specify whether the triggered buff counts from _stack_ to 0
```
 # Unsolvable riddle: 1605 str on-use effect every 2mins, 20s duration.
 trinket1=unsolvable_riddle,stats=321mastery,use=1605str_120cd_20dur

 # A custom trinket ticking 100 Agility every second, lasting for 10 seconds
 trinket1=custom_trinket,use=100Agi_10stack_1tick_10dur_60cd
```
  * _equip_ allows you to specify procs. The syntax is: `<trigger>_<value1><param1>[_<value2><param2>...]`. The parameters are the same as with _use_ and also:
    * _procby_ is used to define what types of abilities proc the effect. Each different type is given after _procby_ using a forward slash and a specifier. Allowed specifiers are
      * _aoespell_, Area of effect harmful spells
      * _aoeheal_, Area of effect healing spells
      * _spell_, Harmful spells and harmful periodic effects
      * _directspell_, Harmful spells
      * _heal_, Healing spells and healing periodic effects
      * _directheal_, Healing spells
      * _attack_, Melee and ranged autoattacks and special abilities
      * _wattack_, Melee and ranged autoattacks
      * _sattack_, Melee and ranged special abilities
      * _melee_, Melee special abilities
      * _wmelee_, Melee autoattacks
      * _ranged_, Ranged special abilities
      * _wranged_, Ranged autoattacks
    * _procon_ is used to define what types of results proc the effect. Each different type is given after _procon_ using a forward slash and a specifier. Allowed specifiers are
      * _impact_, any positive result (hit, glance, crit)
      * _hit_, any positive result (hit, glance, crit, implies a non-zero amount)
      * _crit_, any critical strike result (implies a non-zero amount)
      * _glance_, any glancing result (implies a non-zero amount)
      * _dodge_, any dodge result
      * _parry_, any parry result
      * _miss_, any miss result
      * _cast_, casting finishes
      * _multistrike_, any multistrike result (implies a non-zero amount)
      * _ms\_hit_, any multistrike hit result (implies a non-zero amount)
      * _ms\_crit_, any multistrike critical strike result (implies a non-zero amount)
    * _%_ can be used to specify a proc chance as a percentage. For example: "25%" for a 25% proc chance.
    * _ppm_ can be used to specify a proc chance per minute. For example: "2ppm" for a proc chance leading to 2 procs per minute.
    * _rppm_ can be used to specify a real procs per minute value. For example: "2rppm" for a proc chance leading to 2 real procs per minute.
      * A suffix of _spellhaste_, _attackhaste_, or _crit_ can be appended to _rppm_, causing the value to be scaled with the corresponding stat
    * _driver_ can be used to specify the spell that is used to "proc" the effect. If given, the system attempts to automatically detect what kind of effect to make.
    * _trigger_ can be used to specify the spell that is triggered by the proc. If given, the system attempts to automatically detect what kind of effect to make.
    * _refresh_ can be used to allow the refreshing of the procced buff. This is the default behavior.
    * _norefresh_ can be used to disallow the refreshing of the procced buff. This is the default behavior for buffs that gain/lose stacks by ticking.
```
 # "Darkmoon card: hurricane": 5k nature damage, about 10% chance to proc on every attack
 trinket1=darkmoon_card_hurricane,stats=321str,equip=procby/attack_5000nature_10%

 # Custom trinket that procs on attack crits
 trinket1=custom_trinket,equip=procby/attack_procon/crit_100agi_10dur_5%
```
  * _addon_ is a shortcut for adding proc effects with a built-in support. For example, `addon=pyrorocket` is equivalent to specifying `use=1165Fire_45cd`. For a list of supported procs, you can look at the sc\_unique\_gear.cpp file.
```
 # Gloves with pyro-rockets (id=54998) on it.
 hands=some_gloves,addon=pyrorocket
```
  * _initial_cd_ allows the user to specify the minimum amount of time that must elapse before any procs the item provides occur. The proc effect must have a cooldown, and the initial cooldown is limited to a maximum of the normal duration of the item's cooldown. This allows the user to emulate equipping an item at certain time prior to the beginning of combat.
```
 # This emulates equipping the trinket (which has an 11s ICD) at 7.5s prior to the pull.
 trinket1=soul_capacitor,id=124225,bonus_id=567,initial_cd=3.5
```

### Crafted stats (Shadowlands)

Shadowlands normally crafted items use a new way to convey which random secondary stats the item receives.
  * _crafted_stats_ (default: "") can be used to define a list of secondary stat tokens delimited by `/` for the (maximum of two) random secondary stats on a crafted item. The tokens can be either numbers, in which case they come from the `item_mod_type` enumeration (engine/dbc/data_enums.hpp), or strings, in which case the normal "short" stat names (e.g., `vers`) will function.

### Itemlevel
To manipulate the itemlevel of an item you have two options.
- `,ilevel=X`, where X is the wanted itemlevel, sets the item to a fixed itemlevel. If an item has a trait (Battle for Azeroth), that would change the itemlevel, this trait would be ignored with this option.
- `,bonus_id=X`, where X is one or multiple bonus ids. Multiple bonus ids use "/" as a delimiter

### Bonus IDs
To get a full list of all known bonus IDs, execute simc with the parameter `show_bonus_ids=1`. Most useful:

bonus_id | effect
--- | ---
1674 | "epic" item quality
1473 | +1 itemlevel, 
... | +... itemlevel
1672| +200 itemlevel
## Set bonuses
Set bonuses have to be manually added, even if you have enough set pieces equipped. This is done with keywords such as **tier11\_2pc\_caster**. The generic syntax is `tier<number>_[2pc/4pc]_[melee/caster/tank/heal]`. The correct bonus is chosen according to your class and the suffix you used.

> Note that enabling the 4pc bonus does not automatically enable the 2pc bonus, so you may want to toggle both of them. Reciprocally, you can activate those bonuses while your gear actually does not satisfy the requirements.
```
 # Enable the tier 11 bonuses for a balance druid.
 tier11_2pc_caster=1
 tier11_4pc_caster=1
```

**(Since Simulationcraft 6.0.2)** From Tier 17 onwards, you must use the new `set_bonus` option to override set bonuses, and omit the role. You can also specify earlier set bonuses with the `set_bonus` option, by simply appending set\_bonus= to the "old option style".
```
 # Enable the tier17 bonuses
 set_bonus=tier17_2pc=1
 set_bonus=tier17_4pc=1
```

**(For sets that are enabled by non-spec conditions)** Some tier sets are enabled by non-class/spec conditions which are not necessarily known to the simulator at `set_bonus` option parsing time. To enable these sets, you can use one of the following syntax:

* For the standard syntax, you must have a hero tree selected either via `talents=<hash string>` or `hero_talents=<talent syntax>`.
* For the shorthand syntax, only single word tier such as `tww3` will work. Multi-word tiers such as `tww3_sotv` (for shard of the void set) must use the verbose syntax.
```
# Enable the TWW3 Felscarred Hero Talent tier set 2pc bonus using shorthand syntax.
set_bonus=tww3_felscarred_2pc=1
```
* For the full verbose syntax, valid `name` and `pc` parameters are required for parsing to succeed, using the same names as used by the previous `set_bonus` option. Parameters of name `hero_tree` and `spec` are optional, controlling the selected `hero_tree` and `spec` respectively. An optional parameter `enable` also exists, which defaults to `0`. Additonally, providing an un-named `1` or `0` parameter can enable or disable sets.
```
# Enable the TWW3 Felscarred Hero Talent tier set 2pc bonus using verbose syntax.
set_bonus=name=thewarwithin_season_3,pc=2,hero_tree=felscarred,enable=1
```

# Item Data Importation
When automatically importing items data (see the [importing stats through ID](#Importing_stats_through_ID.md) section), the application will sequentially query multiple sources until one of them successfully returns the stats. (For more information on how item import interacts with caching, see [Cache Control](CacheControl).)

  * **item\_db\_source** (scope: global; default: "wowhead,mmoc,bcpapi,ptrhead,local") is a sequence of sources, separated by ":", "/" or "|". Simulationcraft will try to retrieve items from each source in turn. Acceptable values are:
    1. "wowhead" is [Wowhead](http://www.wowhead.com/)
    1. "bcpapi" is [Battle.net](http://eu.battle.net/wow/fr/)
    1. "ptrhead" is [Wowhead - ptr](http://ptr.wowhead.com/)
    1. "local" is Simulationcraft's local, offline database. It is built directly from the game's files and contains most ilevel 277-400 items.
```
 # Use the local items database as the default source, with Wowhead as a backup.
 item_db_source=local|wowhead
```

# Additional commands
```
scale_to_itemlevel=600
#Used to scale all gear on character to ilevel 600 when simulating
```

# Appendix: stats abbreviations
Stats abbreviations are (non-exhaustive list):
  * Resources: health, mana, energy, rage, runic, focus
  * Primary: str, agi, sta, int, spi
  * Secondary: mastery, crit, haste, vers, mult, sp, ap, mp5
  * Defensive: armor, bonus\_armor
  * Weapon damages: wdps, wspeed, wohdmg, wohspeed
