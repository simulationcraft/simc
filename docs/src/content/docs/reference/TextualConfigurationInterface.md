---
title: Textual Configuration Interface
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**

# Presentation
The textual configuration interface (TCI) is a set of textual options or commands one can use in Simulationcraft. The TCI can be fully and extensively used in anyone of those contexts:
  * The **overrides** tab within the graphical user interface (GUI), Simulationcraft.exe.
  * Text files (usually named with the .simc extension) which can be used either by the GUI or the command-line client.
  * Directly as arguments for the command-line clients.

# Options scopes
Simc files are parsed in a sequential way and as soon as you declare an option, it takes effect. For some of them, it does not matter: whether they're at the beginning or the end of file, it makes no difference. For some others, however, order matters.

Options can have different kind of scopes. The most common are:
  * _global:_ the option can be declared anywhere in the file, its location does not matter (unless redundant or conflicting declarations are made: most of the time, the last one will prevail). Example: **optimal\_raid**.
  * _current character:_ the option affects the current character (the last declared one). Example: **level**.
  * _ulterior characters:_ the option affects the characters declared later in the file, excluding the current character. Example: **ptr**.

Characters declaration can be manual (through **warlock**, **warrior**, ...) or automatic (through **armory**, **wowhead**, ...). See [Characters declaration](Characters.md#Declaration).

# Characters encoding
Simulationcraft works with an UTF-8 encoding (basically, a text file is just a sequence of numbers, the encoding is the convention used to translate numbers to characters; a convention has to be chosen to know which character is represented by a given number). Latin1 works also since the common characters are encoded in the same way they are in UTF-8. UTF-8 is universal and the modern standard but older, region-specific, encodings are still very common.

Simple text editors such as Microsoft Notepad typically encode your files with your operating system's default encoding, which depends on your regional settings. With western regional settings (US, Canada, Australia, Western Europe, etc) it will be latin1 and you will have no problem. With different regional settings, the file will have an encoding incompatible with Simulationcraft.

There are many good, free and open-source, text editors such as [Notepad++](http://notepad-plus-plus.org/) for Windows. Those softwares will allow you to choose an UTF8-encoding and use Simulationcraft with any alphabet and all regional armory or battle.net websites.

# Textual formatting

## Comments

Comments can be made through the # symbol, as shown below:
```
 # This is a comment
```

## Multiline Options
Some options, such as **actions** and **raid\_events** may yield incredibly long lines.

They can be split across multiple lines in input by using the **+=** operator.

The **/** token is often used to split many types of multiline options.

```
actions=foo,bar,baz

# is equivalent to
actions=foo
actions+=,bar
actions+=,baz

actions=fireball
actions+=/fire_blast
```

## Whitespace
By default, all forms of white space (the standard space character, tab, newline, carriage return) are all treated identically as a terminator for the current parsed line.

Some options such as the class creators (**copy**, **monk**, **enemy**, etc) may benefit from whitespace in an option token. A double quote **"** causes all previously mentioned types of whitespace to be ignored until the next **"** is found.

```
output =/dev/null
# is equivalent to
output
=/dev/null
# neither of which are valid options by themselves

enemy="foo bar"
# would create an enemy actor of name 'foo bar'
```

## Sequences

Some options, such as **actions** or **raid\_events**, are very long strings containing sequences of commands. By default, those string are empty. You can either write them on a single line or on multiple lines (see the previous section). There is one additional rule regarding the chaining of commands:
> All commands need to be separated with an operator, typically it will be "/". You can use it at the very beginning but it is optional.

```
 #This is licit
 raid_events+=/event1,option1,option2
 raid_events+=/event2,option1,option2

 #This is too
 raid_events=/event1,option1,option2
 raid_events+=/event2,option1,option2

 #This is too
 raid_events=event1,option1,option2
 raid_events+=/event2,option1,option2

 #This is too
 raid_events=event1,option1,option2/event2,option1,option2

 #This is too
 raid_events=/event1,option1,option2/event2,option1,option2
```

## Standard String Tokenization

Sometimes, you need to translate a string into an identifier. For example, "nature's majesty" will become **natures\_majesty**. The rules are simple:
  1. White spaces are replaced with underscores (`_`).
  1. Other non-alphanumeric characters are removed.

# Text templating
Simulationcraft provides a templating mechanism to declare and reuse pieces of text.
  1. Templates are declared with the syntax: `$(variable)=content`
  1. Templates are referred and used with the syntax: `$(variable)`

For example, the following file:
```
 # Declare a new template named light_the_fire
 $(light_the_fire)=!ticking&buff.t11_4pc_caster.down

 # The references to light_the_fire will be replaced with its content
 armory=us,illidan,some_balance_druid
 actions+=/sunfire,if=$(light_the_fire)&!dot.moonfire.remains>0
 actions+=/moonfire,if=$(light_the_fire)&!dot.sunfire.remains>0

```

Is equivalent to:
```
 armory=us,illidan,some_balance_druid
 actions+=/sunfire,if=!ticking&buff.t11_4pc_caster.down&!dot.moonfire.remains>0
 actions+=/moonfire,if=!ticking&buff.t11_4pc_caster.down&!dot.sunfire.remains>0
```
As you can see the `$(light_the_fire)` reference has been replaced with the content assigned to it.


# Includes

You can easily include external TCI files (usually named with the .simc extension) in your current TCI stream/file: either explicitly, through the `input=<filename>` syntax, or implicitly by just writing the file name. The application will look up for an existing file with this name within the directories specified in **path** list (see below). Including a file means that all its content will be included at the very point you referenced it within the parent file or stream.
```
 # The following line will include the "global-config.simc" file, which must be located in the current working directory (see the relevant section)
 global-config.simc

 # Or we can use this syntax:
 input=global-config.simc

 # We can also specify a path (remember, through: white spaces are not allowed)
 c:\global-config.simc

```
  * **path** (scope: global; default: ".|profiles|profiles\_heal|../profiles|../profiles\_heal") specifies the directories where the application should search for the files to include. The list of directories have to separated with "|", "," or ";". This option can be written on many lines, see the [long strings](#Long_strings) section.
```
 # The following will make the application look for includes in c:\includes, .\profiles and ..\simc_scripts
 path="c:\includes|.\profiles|..\simc_scripts"
```

**Since Simulationcraft 7.0.3, release 1** If you include a file through the `input` option, a text template variable `current_base_name` is automatically assigned to contain the base file name of the inputted file during the parsing of that file.
