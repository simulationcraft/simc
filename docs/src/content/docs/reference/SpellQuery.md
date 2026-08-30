---
title: Spell Query
---
_This documentation is a part of the [TCI](TextualConfigurationInterface.md) reference._

Spell data searches can now be done by using the "spell\_query" option in
simc. This does not do any simulation, but rather reads the expression given as
a value for the option, and runs it against the spell (or talent) data for live or PTR (by specifying ptr=1 before the spell\_query option). The result is a textual output of all spells (or talents) matching the given query expression. You can also append "@**level**" at the end of your spell query, which will execute the query using an actor level that was specified at **level**. In addition, from version 623-1 forward you can also specify the item level through the **level** option. Spell query will interpret any level higher than the global player maximum level of the simulator to indicate item level scaling for the spell.

To accomplish this, a new expression data type has been defined
(TOK\_SPELL\_LIST), which contains a list of spell or talent identifiers. To this
end, spell\_query works on an expression level by first taking a full list of
spell ids, and performing filtering on that list to limit it. Thus, all
spell\_query related operations are binary, and the left side operator must
always be a TOK\_SPELL\_LIST operand. Binary operators may be chained normally,
as the binary operator inherits both data source and the result type from the
left side operator.

The expressions for spell query follow the normal simc expression input,
however the spell query must be told where to get the initial list of
identifiers. The following identifier data sources have been defined:
  * spell - The master spell list, containing all spell data in simc
  * talent - The master talent list, containing all talent data (NOTE: not spells)
  * talent\_spell - All spell ids used by the master talent list for talent ranks
  * class\_spell - All spells belonging to a class and "clickable", this includes pets
  * race\_spell - All racial spells
  * mastery - All class masteries
  * spec\_spell - All specialization spells, passive or active
  * glyph - All glyph spells
  * set\_bonus - All set bonus spells
  * effect - All effects
  * perk\_spell - All perk spells (NOTE: WoD only)
  * artifact - All artifact spells (**Added in version 703-1, Removed in 8.0**)
  * azerite - All azerite power spells (**Added in version 8.0**)
  * covenant_spell - All Covenant abilities (**Added in version 9.0.1, release 1**)
  * soulbind_spell - All Soulbind abilities (**Added in version 9.0.1, release 1**)
  * conduit_spell - All Soulbind conduit abilities (**Added in version 9.0.1, release 1**)
  * runeforge_spell - All runeforge legendary spells (**Added in version 9.0.2, release 1**)

A data source can be filtered by giving it a data field name, by suffixing the
data source name with a period and the filtering data field name. Currently the
system does not support deeper "paths" than 2, i.e. "spell.name" is the deepest
it will go. The expression is then given an operand, e.g. == !=, <, > and so
on, after which a string, numeric type or another spell identifier list (if the
operands &, | or - are used). The following data field names are currently
supported, listing the name, data\_source\_types (spell covers all but "talent"
data source), operand\_type and a brief description:
  * name, spell/talent, STRING
  * id, spell/talent, NUMBER
  * flags, spell/talent, NUMBER (not used for anything currently)
  * speed, spell, NUMBER (projectile speed)
  * school, spell, STRING (spell school name)
  * class, spell/talent, STRING (class name)
  * pet\_class, talent, STRING (pet talent tree name)
  * scaling, spell, NUMBER (spell scaling type, -1 for "generic scaling", 0 for no scaling, otherwise class number)
  * extra\_coeff, spell, NUMBER (spell-wide coefficient, usually used for spells scaling with both SP and AP)
  * level, spell, NUMBER (spell learned level)
  * max\_level, spell, NUMBER (spell "maximum" level in a scaling sense)
  * min\_range, spell, NUMBER (minimum range in yards)
  * max\_range, spell, NUMBER (maximum range in yards)
  * cooldown, spell, NUMBER (spell cooldown, in milliseconds)
  * gcd, spell, NUMBER (spell gcd duration, in milliseconds)
  * category_cooldown, spell, NUMBER (shared cooldown duration, in milliseconds, **New in version 710-03**)
  * charges, spell, NUMBER (number of charges for the spell)
  * charge\_cooldown, spell, NUMBER (charge cooldown in milliseconds)
  * category, spell, NUMBER (spell cooldown category)
  * duration, spell, NUMBER (spell duration in milliseconds)
  * rune, spell, STRING, (b = blood, f = frost, u = unholy, will match minimum rune requirement, **Removed in version 701-1**)
  * power\_gain, spell, NUMBER (amount of runic power gained, **Removed in version 701-1**)
  * max\_stack, spell, NUMBER (maximum stack of spell)
  * proc\_chance, spell, NUMBER (spell proc chance in percent (0..100))
  * icd, spell, NUMBER (internal cooldown of a spell in milliseconds)
  * initial\_stack, spell, NUMBER (initial amount of stacks)
  * cast\_min, spell, NUMBER (minimum cast time in milliseconds)
  * cast\_max, spell, NUMBER (maximum cast time in milliseconds)
  * cast\_div, spell, NUMBER (scaling divisor for cast time, always 20)
  * m\_scaling, spell, NUMBER (unknown scaling multiplier)
  * attribute, spell, NUMBER (spell attribute bit index, 0 indexed)
  * flag, spell, NUMBER (spell family flag bit index, 0 indexed, **Added in version 801-1**)
  * label, spell, NUMBER (spell label number, **Added in version 905-1** )
  * scaling\_level, spell, NUMBER (level threshold for m\_scaling)
  * desc, spell, STRING (spell description)
  * tooltip, spell, STRING (spell tooltip)
  * stance\_mask, spell, NUMBER (stance mask of the spell, **Added in version 701-1**)
  * tab, talent, NUMBER (talent tab number, 0..2)
  * dependence, talent, NUMBER (talent id this talent depends on)
  * depend\_rank, talent, NUMBER (talent rank of talent id this talent depends on)
  * col, talent, NUMBER (talent column 0..3)
  * row, talent, NUMBER (talent "tier" 0..6)
  * power\_id, spell/azerite, NUMBER (azerite armor trait id, **Added in version 8.0**)
  * essence\_id, spell, NUMBER (heart of azeroth essence id, **Added in version 8.2**)
  * covenant, spell, STRING (covenant name in lowercase, **Added in version 9.0.1 release 1**)
  * conduit_id, spell, NUMBER (conduit identifier, **Added in version 9.0.1 release 1**)

For numeric data fields, the following numeric operators between a spell list
and a numeric right-side operand are available: ==, !=, >, <, >=, <=. All string data
fields can be filtered by a string right-side operand and the following operators:
  * == : case insensitive string equality
  * != : case insensitive string inequality
  * ~  : case insensitive substring in data field
  * !~ : case insensitive substring not in data field

IMPORTANT NOTE Due to how simc does command line parsing, all string based
filtering needs to be input with whitespaces converted to underscores and without any
special characters. The spell filtering will automatically "canonicalize" any
strings it compares your textual input against, and all comparisons are done
case insensitive. This means that for example "Shadow Word: Pain" becomes
"Shadow\_Word\_Pain" when given as a right-side string operand to filter with
IMPORTANT NOTE

Furthermore, for identifier list operands on the right side of the expression, the
following operators are available:
  * &  : intersection (identifiers in both left and right side)
  * |  : union (identifiers from both left and right side, removing duplicates)
  * -  : subtraction (right side identifiers removed from left side list of identifiers)

Examples:

a) All activatable class spells of shaman
> $ simc spell\_query=class\_spell.class=shaman

b) All cataclysm activatable spells
> $ simc spell\_query="class\_spell.level>80"

c) All activatable class spells of shaman and priest
> $ simc spell\_query="class\_spell.class=shaman|class\_spell.class=priest"

c) All mastery spells
> $ simc spell\_query=mastery

d) All class (and pet) spells costing resources
> $ simc spell\_query="class\_spell.cost>0"

e) All spells that contain "shadow damage" in their description or tooltip
> $ simc spell\_query="spell.desc~shadow\_damage|spell.tooltip~shadow\_damage"

f) All death knight spells that use at minimum a blood and an unholy rune
> $ simc spell\_query="spell.class=death\_knight&spell.rune=bu"

g) All spells using the constant scaling
> $ simc spell\_query=spell.scaling=-1

h) All spells that are fire or frost based (including derivations)
> $ simc spell\_query="spell.school=fire|spell.school=frost"

Note that depending on your shell interpreter, you may need to escape some special
characters used in the spell queries.

..........................................................................................

A new data source called "effect" allows for filtering effects based on data
field, currently all data fields accept a numeric operand to compare against.
Additionally, spells can now be filtered by using "effect" as a field name,
followed by one of the actual data field names:
  * id - Effect id
  * flags - Effect flags (not used for anything)
  * spell\_id - Spell the effect belongs to
  * index - Effect index on the spell (effect indexes in data are 0..2)
  * type - Effect type (see data\_enums.hh for effect\_type\_t)
  * sub\_type - Effect (aura) sub type (see data\_enums.hh for effect\_subtype\_t)
  * m\_coefficient - Effect average scaling multiplier (class-based scaling)
  * m\_delta - Effect delta scaling multiplier (class-based scaling)
  * m\_bonus - Effect bonus scaling multiplier (class-based scaling)
  * coefficient - Effect coefficient
  * amplitude - Effect amplitude (base tick time)
  * radius - Effect minimum radius
  * max\_radius - Effect maximum radius
  * base\_value - Effect base value
  * misc\_value - Effect miscellaneous value
  * misc\_value2 - Effect secondary miscellaneous value
  * trigger\_spell - A spell (identifier) that is triggered by this effect
  * m\_chain - Effect chain multiplier
  * p\_combo\_points - Effect combo point based multiplier (usage unknown at the moment)
  * p\_level - Effect level based multiplier
  * damage\_range - Effect damage range modifier (usage unknown at the moment)
  * chain\_target - Number of targets the effect chains to
  * target\_1 - First targeting type identifier (**Added in version 703-1**)
  * target\_2 - Second targeting type identifier (**Added in version 703-1**)

An example:

a) All shaman talents that additively increase the critical strike bonus of spells
> $ ./simc spell\_query="talent\_spell.class=shaman&talent\_spell.effect.sub\_type=108&talent\_spell.effect.misc\_value=15"

..........................................................................................

When using the spell\_query\_xml\_output\_file option, the output will be stored into the chosen file and formated as XML.
