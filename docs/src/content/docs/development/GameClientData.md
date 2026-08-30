---
title: Game Client Data
---
# Game Client Data

The World of Warcraft game client stores various bits of data about the game in data files (DBC files). SimulationCraft currently uses many DBC files to automate various aspects of the simulator initialization, such as items, spells, enchants, gems, ratings conversions, and stat conversions. The data is first extracted from the World of Warcraft game client, and further processed (and filtered) by a separate script to produce text files containing various attributes from the data.

## Extracting relevant game client data

SimulationCraft uses a Python script (`dbc_extract.py`) to automatically extract relevant information from the World of Warcraft game client files (DBC files). The script is located in the `dbc_extract` directory of the source tree. WoW game client files are located in the larger MPQ files inside the `Data` directory of the game client. First, the DBC files need to be extracted from the MPQ files. On Windows, [MPQEditor](http://www.zezula.net/en/mpq/download.html) can be used to extract them.

The MPQ files themselves consist of a "base" MPQ file, and a set of patch files numbered with the builds of the client. To correctly extract the DBC files for the build, the base file needs to be patched up to the build version during the extraction. MPQEditor can handle this by itself (using the "Custom Open MPQ" dialog). The DBC files are located in two MPQ files in the World of Warcraft game client. Data should be first extracted from `Data/misc.MPQ` (patched by `wow-update-base-<build>.MPQ`), and then from `Data/<locale>/locale-<locale>.MPQ` (patched by `wow-update-<locale>-<build>.MPQ`), where locale is one of `enGB` or `enUS`, overwriting any files necessary.

The `dbc_extract` program has several different "modes" and options, you can see a list of the command line parameters with `dbc_extract.py --help`. For full data extraction, the `dbc_extract` directory also contains a shell script (`generate.sh`) and a bat file (`generate.bat`) to automate the extraction process. The generation scripts have the following invocation format:
```
 $ generate.[sh|bat] [ptr] [wowversion] <patch> <input_base>
```

The options used are as follows:
  * `ptr` is an optional parameter that specifies that the extraction is run for the PTR client of WoW. This affects the naming conventions for the outputted files, but otherwise performs the identical task as live client extraction.
  * `wowversion` is an optional parameter only found in the Unix version of the script (`generate.sh`), and was temporarily used as a workaround for a DBC schema difference between live and PTR. Unnecessary at the moment.
  * `patch` is the build level of the World of Warcraft client that you have the DBC files for
  * `input_base` is the base directory where your DBC files are found. The base directory for the script has a special format. It requires that the directory `<input_base>/<patch>/DBFilesClient` contains the actual DBC files for the build.

Additionally, we also distribute a copy of the game client (item) cache in `dbc_extract/cache/live`. This directory is used during the extraction process to supplement it, when Blizzard hotfixes items, or leaves them out of the game client data. Any file in the directory containing a name of a DBC or DB2 file will be used to overwrite values in the game client DBC files. Multiple cache files for a single DBC file will also work, and are automatically organized based on the last updated timestamp found in the cache file.

**Please make sure to use Python 2.7 when running the generator**

## Gathering Scale Data for Dragonflight-era Talents
For the new talent re-design brought in Dragonflight there are various talents that scale with ranks or how many points given for that talent. You can use the generated SpellDataDump files to find these ranks when SimC can parse them.

```
Name             : Sanguine Teachings (id=376202) [Spell Family (6), Passive] 
Talent Entry     : Shadow [tree=spec, entry_id=19961, max_rank=3]
                 : Effect#1 [op=set, values=(5, 10, 15)]
```

The example above is from the Sanguine Teachings Shadow Priest talent. From this output you can clearly see that it has 3 rank scaling for Effect#1 with values 5, 10, and 15. If you ever want to cross check this with the raw data you can take the entry_id to lookup the scaling in `TraitNodeEntry` table and use the `TraitDefinitionID` in the `TraitDefinitionEffectPoints` table.
