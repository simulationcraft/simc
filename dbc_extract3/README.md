DBC Spell Data extraction tool for Simulationcraft
==================================================

Requirements
============

- A decently new Python implementation (2.5+)

Extracting data
================

The extractor tool is able to parse through a set of DBC files, extracting the
following information into a "simulationcraft" representation (spell data
files):
 * A "master spell list" for relevant spells
 * A talent list, including all pet talents
 * Scaling information for spells, combat ratings and mana regen
 * Various lists used by simc to automate certain aspects of modeling
 * Patch DBC files to a new build version
 * A raw view into the fields of the DBC data

All output data is written to stdout.

Using the tool
==============

To extract data, the tool typically requires two things:
 1) An input path where your DBC files are located (-p switch)
 2) An extraction type, specifying what kind of output will be generated
    (-t switch)
 3) A build number, specifying the WoW build number where your DBC data
    is from (-b switch). Note that some extraction types do not require
    a build number (-t header, -t patch).

Some optional parameters may also be specified that modify the output (others
can be seen with dbc_extract.py --help):
 --prefix  specify a modifier for the data output variable names
 --suffix

Additionally, the tool contains a "-t view" switch to view the raw data of a (known)
DBC file. The switch takes two additional parameters:
 1) The name of the DBC file in the directory pointed by -p
 2) An optional, numerical identifier for the record you are looking for.
    If omitted, all records are output.

*NOTE* The identifier search is done as a binary search, so if a DBC file
records are not ordered, things may not be found. *NOTE*

Examples
========

Extract all relevant spell data from the DBC files
  $ ./dbc_extract.py -b 13286 -t spell -p /path/to/your/dbc/files > sc_spell_data.inc

Extract scaling data from the DBC files, with a prefix
  $ ./dbc_extract.py -b 13286 -t scale -p /path/to/your/dbc/files --prefix=ptr > sc_scale_data_ptr.inc

Patch DBC files to a new build version
  $ ./dbc_extract.py -t patch /path/to/your/old/dbc/files /path/to/your/new/dbc/directory \
    /path/to/your/dbc/patch/files

Check the DBC header information of a DBC file
  $ ./dbc_extract.py -t header /path/to/your/dbc/file

View the spell data of Lightning Bolt
  $ ./dbc_extract.py -b 13286 -t view -p /path/to/your/dbc/files Spell.dbc 403

View the spell relationships for a class (VERY EXPERIMENTAL)
  $ ./ dbc_extract.py -b 13286 -t class_flags -p /path/to/your/dbc/files Shaman


