---
title: Using CASC Extract and DBC Extract
---
# Overview

SimC has a couple of useful command line tools for downloading and extracting data from WoW client data. This page covers some basic usage that may be useful if you want to use game client data for your own projects.

`casc_extract/casc_extract.py` can extract binary DBC files or images/icons.

`dbc_extract/dbc_extract.py` can extract data from DBCs into more "friendly" formats like JSON or CSV

(These notes were written for 8.x / bfa-dev versions)

# casc_extract.py Examples

## Download game data from CDN:

`python casc_extract.py --cdn -m batch -o [OUTPUT_DIR]`

`--ptr` or `--beta` can be added. Can also use product like: `--product=wowxptr`

`--locale [LOCALE]` will grab data for other locales (en_US is default when not specified)

`casc_extract.py` will create a directory inside of `OUTPUT_DIR` using the current WoW version from that environment so you'll end up with something like `OUTPUT_DIR/8.0.1.26715`

# dbc_extract.py Examples

## Extract Game Data

Extract JSON files from DBC:

`python dbc_extract.py -b [WOW_BUILD] -t json -p [PATH_TO_DBCS] [DBC]`

`WOW_BUILD` is the full WoW version (e.g. `8.0.1.26715`)

`PATH_TO_DBCS` will likely be based on the `OUTPUT_DIR` you used with `casc_extract.py` and will usually need to point to the `[WOW_VERSION]/DBFilesClient/` subdirectory

`--hotfix [PATH_TO_DBCACHEBIN]` is used to include hotfix data from a local game installation.

# Download images/icons from CDN

You can use `casc_extract.py` to extract assets from the game data (such as icons). You need a file with the names of assets you want to extract.

You can generate a list of icon paths by looking at the `ManifestInterfaceData` DBC.

`python casc_extract.py --cdn -m batch -o [OUTPUT_DIR] -b files.txt`

`files.txt` is a new-line delimited file of the paths used in the DBC. Partial example:

```
Interface\ICONS\Ability_Ambush.blp
Interface\ICONS\Ability_BackStab.blp
Interface\ICONS\Ability_BullRush.blp
Interface\ICONS\Ability_CheapShot.blp
...
```

# Full Example

Full example to convert ItemSparse DBC into a JSON file

```bash
python casc_extract.py --cdn -m batch -o ./tmp/casc-data

# This would create /tmp/casc-data/8.0.1.26715

python dbc_extract.py -b 8.0.1.26715 -t json -p ./tmp/casc-data/8.0.1.26715/DBFilesClient ItemSparse > ./tmp/ItemSparse.json
```

To include hotfix data, use the `--hotfix` flag pointed at your WoW install:

```
./dbc_extract.py -b 8.0.1.26715 -t csv -p ./tmp/casc-data/8.0.1.26715/DBFilesClient --hotfix /path/to/wow/Cache/ADB/enUS/DBCache.bin ItemSparse > /tmp/ItemSparse.csv
```

# Full Walkthrough for pulling Spell Data

1. `python casc_extract.py -m batch --cdn -o <path>` (`--product`, `--beta`, or `--ptr` if needed)
2. Place `DBCache.bin` from the wow folder into the proper `dbc_extract3\cache\live\` or `dbc_extract3\cache\ptr\`
3. Run the `dbc_extract3` generate command, i.e.: `./generate.bat [ptr] <full build number> ../casc_extract/<casc_extract temp path> [--hotfix] [path to hotfix DBCache.bin]`
4. If running for PTR, make sure to change `#define SC_USE_PTR 0` to `#define SC_USE_PTR 1` in `config.hpp`. Do not upload this change.
5. Rebuild SimulationCraft
6. Run the spell data dump script in that folder for that output
