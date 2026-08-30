---
title: Output
---
_This documentation is a part of the [TCI](TextualConfigurationInterface.md) reference._

**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**

Note: the standard output mentioned on this page is the console output, not your report.

# Combat logs
  * **log** (scope: global; default: 0), when different from zero, will force the application to print a human-readable log on the standard output. It is easier to read than the wow combat log. Enabling this setting will force **iterations** to 1.
```
 # Here is how we enable the log.
 log=1

 # It may also be a good idea to print it out on a file rather than the standard output:
 output=log.txt
```
  * **output** (scope: global; default: "") can be used to specify a file to redirect most of the standard output to. The logs, notable, will be redirected to this output. When debugging (through **debug**), the options listing will be redirected while some other things will remain on the standard output.
```
 # We can put the log in the current working directory (see the relevant section)
 output=log.txt

 # Or we can explicity specify the path (windows example)
 output=c:\log.txt
```

  * **log_spell_id** (scope: global; default: 0) output spell data ids when logging actions or buffs.

# Reports

  * **html** (scope: global; default: "") can be used to specify a file to write a html report to.
  * **xml** (scope: global; default: "") can be used to specify a file to write a xml report to. It uses a custom format. You can use those reports with a xslt or anything else but you should be aware it has gone unmaintained and may disappear in the future.
```
 # We can put those files in the current working directory (see the relevant section)
 html=report.html
 xml=report.xml

 # Or we can explicity specify the path (windows example)
 html=c:\report.html
 xml=c:\report.xml
```
  * **json** (***DEPRECATED***) (scope: global; default: "") can be used to specify a file to write a json report to. This option is deprecated, please use **json2**
  * **json2** (scope: global; default: "") can be used to specify a file to write a json report to.
  * **report\_details** (scope: global; default: 1), when different from zero, will change the abilities list in the html report so that abilities can be expanded to display the static data extracted from the DBC game files, and modified by your glyphs, talents and such.
```
 # I don't care and I want my html reports to be smaller.
 report_details=0
```
  * **report\_precision** (scope: global; default: 3) is the number of digits to display after the decimal point for the scale factors in the reports.
```
 calculate_scale_factors=1
 report_precision=2
```
  * **report\_rng** (scope: global; default: 0), when different from zero, will make the application include the maximum dps deviation, expressed as a percentage of the average dps, in the summary section of the html report.
```
 # Add in the maximum dps deviation in the report.
 report_rng=1
```
  * **report\_pets\_separately** (scope: global; default: 0), when different from zero, will force the pets to be reported as players, with their own charts, actions list, etc.
```
 report_pets_separately=1
```
  * **full_damage_sources_chart** (scope: global; default: 0), when set will make the player "Damage Sources" chart in the HTML report display a data point for every damage sources, instead of the default behavior which is to only report the combined "top level" damage sources (the entries listed in the abilities table at the left-most position without any indentation, which all child damage sources accumulated in).

  * **buff_uptime_timeline** (scope: global; default: 1), when set, will record uptime timelines of all non-constant buffs. This will be reported to the buff's JSON output as `"stack_uptime"`. If used in conjunctions with `report_details=1` the HTML report will display the buff uptime timeline chart in the details pane of each buff.
  * **buff_stack_uptime_timeline** (scope: global; default: 1), when set in conjunction with `buff_uptime_timeline=1`, will record uptime timelines as above but multiply the value by their current stack. This will be reported in the same manner as above.

  * **hosted\_html** (scope: global; default: 0), when different from zero, will have the javascript and css contents removed from the html reports and hosted on [simulationcraft.org](http://www.simulationcraft.org). There will be no visible changes but you will need to be able to connect to our website to correctly view the report.
```
 html=mytoon.html
 hosted_html=1
```

# Others
  * **reference\_player** (scope: global; default: "") can be used to specify the name of a reference player. This will force the application to print on the standard output, and for each player, the dps difference, expressed as a percentage, in respect to the reference player.
```
 armory=us,illidan,john
 armory=us,illidan,bill
 reference_player=john
```

# Massive profiles exportation
  * **save\_profiles** (scope: global; default: 0), when different from zero, will force the application to save every player profile under a .simc file. The file name will be: `<prefix><playername>.simc`.
  * **save\_prefix!** (scope: global; default: "") can be used to specify to prefix to add to all of the profiles saved through **save\_profiles** (and only this option).
```
 # This will export John's profile to test_john.simc
 armory=us,illidan,john
 save_profiles=1
 save_prefix=test_
```

# Per-character profile exportation
  * **save** (scope: current character; default: "") can be used to specify a file to write a complete profile to.
```
 # This example will export John's profile. Bill won't be affected.
 armory=us,illidan,john
 save=john_profile.simc
 armory=us,illidan,bill
```
  * **save\_gear** (scope: current character; default: "") can be used to specify a file to write a profile to. This profile will only contain the current character's gear.
  * **save\_talents** (scope: current character; default: "") can be used to specify a file to write a profile to. This profile will only contain the current character's talents.
  * **save\_actions** (scope: current character; default: "") can be used to specify a file to write a profile to. This profile will only contain the current character's actions list.
```
 # This example will export John's gear, talents and actions to three different simc files.
 armory=us,illidan,john
 save_gear=john_gear.simc
 save_talents=john_talents.simc
 save_actions=john_actions.simc
```

# Debugging

  * **dps\_plot\_debug** (scope: global; default: 0), when different from zero, will force the application to output a full report (on the standard output stream) for every run made for stats plotting.
```
 dps_plot_debug=1
```
  * **debug** (scope: global; default: 0), when different from zero, will output debugging informations for developers. Many informations will be displayed, including a complete trace of events occurring during the simulation. Enabling this setting will also enable **log** and will force the **iterations** setting to 1.
```
 debug=1
```
  * **debug\_scale\_factors** (scope: global; default: 0), when different from 0 and scale factors are computed, will force the application to print on the standard output a full report at the end of every performed simulation (one report per scaled stat). It allows you, for example, to compare the spells and buffs details between the baseline simulation and the one with the inflated stat of your choice.
```
 debug_scale_factors=1
```
  * **reforge\_plot\_debug** (scope: global; default: 0), when different from 0 and reforge plots are generated, will force the application to print additional data about the computations done for every point.
```
 reforge_plot_debug=1
```

# Exit Codes

A successful simulation will result in the CLI executable returning an exit code of `0`. To assist in debugging, abnormal termination will display an error message to the console as well as returning one of the following exit codes, depending on the nature of the issue that caused the termination:
* ` 1`: General termination, reason unknown or not properly captured by a more specific code
* `30`: Invalid APL argument, the Action Priority List for a player could not be successfully parsed
* `40`: Initialization error, issue while setting up the sim, a player, an action, or a buff
* `50`: Iteration error, issue while running the simulation
* `51`: Simulation stuck, too many events being created at once, potentially indicates an infinite loop
* `60`: Network/file error, could not successfully access data from external sources
* `61`: Report out error, could not successfully print the report to the console, JSON, HTML, or profile
* `70`: Invalid sim-scope argument, an issue with an option that applies to the entire sim
* `71`: Invalid fight style, one of the players does not support the desired fight style and the resulting sim will give incorrect information
* `72`: Unsupported specialization, one of the players has a specialization that's not currently supported
* `80`: Invalid player-scope argument, an issue with an option that applies to a specific player
* `81`: Invalid talent string, a talent string (`talents=`, `class_talents=`, `spec_talents=`, `hero_talents=`) has an error
* `82`: Invalid item string, an item string (`<slot>=<name>,<id>,...`) has an error
