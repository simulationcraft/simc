#!/usr/bin/env python3

import argparse, sys, os, glob, logging, importlib, json

try:
    from bitarray import bitarray
except Exception as error:
    print('ERROR: %s, dbc_extract.py requires the Python bitarray (https://pypi.python.org/pypi/bitarray) package to function' % error, file = sys.stderr)
    sys.exit(1)

import dbc
from dbc.data import initialize_data_model
from dbc.db import DataStore
from dbc.file import DBCFile, HotfixFile
from dbc.generator import CSVDataGenerator, DataGenerator
from dbc.config import Config

def parse_fields(value):
    return [ x.strip() for x in value.split(',') ]

def parse_version(value):
    try:
        return dbc.WowVersion(value)
    except ValueError as e:
        # User may have given just a build number, try a workaround and warn
        if value.isdigit():
            logging.warn('Presuming -b input "{}" as a build number for World of Warcraft 8.0.1, this will become an error in 8.1.0'.format(
                value))
            return dbc.WowVersion('8.0.1.{}'.format(value))
        raise argparse.ArgumentTypeError(e)

logging.basicConfig(level = logging.INFO,
        datefmt = '%Y-%m-%d %H:%M:%S',
        format = '[%(asctime)s] %(levelname)s: %(message)s')

parser = argparse.ArgumentParser(usage= "%(prog)s [-otlbp] [ARGS]")
parser.add_argument("-t", "--type", dest = "type", 
                  help    = "Processing type [output]", metavar = "TYPE", 
                  default = "output", action = "store",
                  choices = [ 'output', 'scale', 'view', 'csv', 'header', 'json',
                              'class_flags', 'generator', 'validate', 'generate_format' ])
parser.add_argument("-o",            dest = "output")
parser.add_argument("-a",            dest = "append")
parser.add_argument("--raw",         dest = "raw",          default = False, action = "store_true")
parser.add_argument("--debug",       dest = "debug",        default = False, action = "store_true")
parser.add_argument("-f",            dest = "format",
                    help = "DBC Format file")
parser.add_argument("--delim",       dest = "delim",        default = ',',
                    help = "Delimiter for -t csv")
parser.add_argument("-l", "--level", dest = "level",        default = 120, type = int,
                    help = "Scaling values up to level [125]")
parser.add_argument("-b", "--build", dest = "build",        default = None, type = parse_version,
                    help = "World of Warcraft build version (8.0.1.12345)")
parser.add_argument("--prefix",      dest = "prefix",       default = '',
                    help = "Data structure prefix string")
parser.add_argument("--suffix",      dest = "suffix",       default = '',
                    help = "Data structure suffix string")
parser.add_argument("--min-ilvl",    dest = "min_ilevel",   default = 90, type = int,
                    help = "Minimum inclusive ilevel for item-related extraction")
parser.add_argument("--max-ilvl",    dest = "max_ilevel",   default = 1300, type = int,
                    help = "Maximum inclusive ilevel for item-related extraction")
parser.add_argument("--scale-ilvl",  dest = "scale_ilevel", default = 1300, type = int,
                    help = "Maximum inclusive ilevel for game table related extraction")
parser.add_argument("-p", "--path",  dest = "path",         default = '.',
                    help = "DBC input directory [cwd]")
parser.add_argument("--hotfix",      dest = "hotfix_file",
                    type = argparse.FileType('rb'),
                    help = "Path to World of Warcraft DBCache.bin file.")
parser.add_argument("--fields",      dest = "fields", default = [],
                    type = parse_fields,
                    help = "Comma separated list of fields to output for -t view/csv")
parser.add_argument("args", metavar = "ARGS", type = str, nargs = argparse.REMAINDER)
options = parser.parse_args()

if options.build == 0 and options.type not in['header', 'generate_format']:
    parser.error('-b is a mandatory parameter for extraction type "%s"' % options.type)

if options.min_ilevel < 0 or options.max_ilevel > 1300:
    parser.error('--min/max-ilevel range is 0..1300')

if options.level % 5 != 0 or options.level > 125:
    parser.error('-l must be given as a multiple of 5 and be smaller than 125')

if options.type == 'view' and len(options.args) == 0:
    parser.error('View requires a DBC file name and an optional ID number')

if options.type == 'header' and len(options.args) == 0:
    parser.error('Header parsing requires at least a single DBC file to parse it from')

if options.type == 'parse' and len(options.args) < 2:
    parser.error('Parse needs at least two options, the wow binary and at least one db2 file')

if options.debug:
    logging.getLogger().setLevel(logging.DEBUG)

if options.type in ['validate', 'generate_format']:
    from dbc.pe import PeStructParser

# Initialize the base model for dbc.data, creating the relevant classes for all patch levels
# up to options.build
initialize_data_model(options)

if options.type == 'validate':
    p = PeStructParser(options, options.args[0], options.args[1:])
    if not p.initialize():
        sys.exit(1)

    p.validate()

elif options.type == 'generate_format':
    p = PeStructParser(options, options.args[0], options.args[1:])
    if not p.initialize():
        sys.exit(1)

    json_obj = p.generate()

    json.dump(json_obj, fp = sys.stdout, indent = 2)

elif options.type == 'output':
    if len(options.args) < 1:
        logging.error('Output type requires an output configuration file')
        sys.exit(1)

    config = Config(options)
    if not config.open():
        sys.exit(1)

    config.generate()
elif options.type == 'generator':
    if len(options.args) < 1:
        logging.error('Generator type is required')
        sys.exit(1)

    try:
        generators = importlib.import_module('dbc.generator')
        generator = getattr(generators, options.args[0])
    except:
        logging.error('Unable to import %s', options.args[0])
        sys.exit(1)

    db = DataStore(options)
    obj = generator(options, db)

    if not obj.initialize():
        sys.exit(1)

    ids = obj.filter()

    obj.generate(ids)
elif options.type == 'class_flags':
    g = dbc.generator.ClassFlagGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter(options.args[0])

    g.generate(ids)
elif options.type == 'header':
    dbcs = [ ]
    for fn in options.args:
        for i in glob.glob(fn):
            dbcs.append(i)

    for i in dbcs:
        dbc_file = DBCFile(options, i)
        if not dbc_file.open():
            continue

        sys.stdout.write('%s\n' % dbc_file)
elif options.type == 'view':
    path = os.path.abspath(os.path.join(options.path, options.args[0]))
    id = 0
    if len(options.args) > 1:
        id = int(options.args[1])

    dbc_file = DBCFile(options, path)
    if not dbc_file.open():
        sys.exit(1)

    cache = HotfixFile(options)
    if not cache.open():
        sys.exit(1)
    else:
        entries = {}
        for entry in cache.entries(dbc_file.parser):
            entries[entry.id] = entry

    logging.debug(dbc_file)
    if id == 0:
        replaced_ids = []
        # If cache has entries for the dbc_file, grab cache values into a database
        for record in dbc_file:
            if not options.raw and record.id in entries:
                print('{} [hotfix]'.format(str(entries[record.id])))
                replaced_ids.append(record.id)
            else:
                print('{}'.format(str(record)))

        for id, entry in entries.items():
            if id in replaced_ids:
                continue

            print('{} [hotfix]'.format(entry))
    else:
        if id in entries:
            record = entries[id]
            hotfix = True
        else:
            record = dbc_file.find(id)
            hotfix = False

        if record:
            print('{}{}'.format(record, hotfix and ' [hotfix]' or ''))
        else:
            print('No record for DBC ID {} found'.format(id))
elif options.type == 'json':
    path = os.path.abspath(os.path.join(options.path, options.args[0]))
    id = None
    if len(options.args) > 1:
        id = int(options.args[1])

    dbc_file = DBCFile(options, path)
    if not dbc_file.open():
        sys.exit(1)

    cache = HotfixFile(options)
    if not cache.open():
        sys.exit(1)
    else:
        entries = {}
        for entry in cache.entries(dbc_file.parser):
            entries[entry.id] = entry

    str_ = '[\n'
    logging.debug(dbc_file)
    if id == None:
        replaced_ids = []
        for record in dbc_file:
            if not options.raw and record.id in entries:
                data_ = entries[record.id].obj()
                data_['hotfixed'] = True
                replaced_ids.append(record.id)
                str_ += '\t{},\n'.format(json.dumps(data_))
            else:
                str_ += '\t{},\n'.format(json.dumps(record.obj()))

        for id, entry in entries.items():
            if id in replaced_ids:
                continue

            data_ = entry.obj()
            data_['hotfixed'] = True

            str_ += '\t{},\n'.format(json.dumps(data_))

    else:
        if id in entries:
            record = entries[id]
        else:
            record = dbc_file.find(id)

        if record:
            data_ = record.obj()
            if id in entries:
                data_['hotfixed'] = True

            str_ += '\t{},\n'.format(json.dumps(data_))
        else:
            print('No record for DBC ID {} found'.format(id))

    str_ = str_[:-2] + '\n'
    str_ += ']\n'

    print(str_)

elif options.type == 'csv':
    path = os.path.abspath(os.path.join(options.path, options.args[0]))
    id = None
    if len(options.args) > 1:
        id = int(options.args[1])

    dbc_file = DBCFile(options, path)
    if not dbc_file.open():
        sys.exit(1)

    cache = HotfixFile(options)
    if not cache.open():
        sys.exit(1)
    else:
        entries = {}
        for entry in cache.entries(dbc_file.parser):
            entries[entry.id] = entry

    first = True
    logging.debug(dbc_file)
    if id == None:
        replaced_ids = []
        for record in dbc_file:
            if first:
                print('{}'.format(record.field_names(options.delim)))

            if not options.raw and record.id in entries:
                print('{}'.format(entries[record.id].csv(options.delim, first)))
                replaced_ids.append(record.id)
            else:
                print('{}'.format(record.csv(options.delim, first)))

            first = False

        for id, entry in entries.items():
            if id in replaced_ids:
                continue

            print('{}'.format(entry.csv(options.delim, first)))

    else:
        if id in entries:
            record = entries[id]
        else:
            record = dbc_file.find(id)

        if record:
            print(record.csv(options.delim, first))
        else:
            print('No record for DBC ID {} found'.format(id))

elif options.type == 'scale':
    g = CSVDataGenerator(options, {
        'file': 'HpPerSta.txt',
        'comment': '// Hit points per stamina for level 1 - %d, wow build %s\n' % (
            options.level, options.build),
        'values': [ 'Health', ]
    })
    if not g.initialize():
        sys.exit(1)
    g.generate()

    # Swap to appending
    if options.output:
        options.append = options.output
        options.output = None

    g = CSVDataGenerator(options, {
        'file': 'SpellScaling.txt',
        'comment': '// Spell scaling multipliers for levels 1 - %d, wow build %s\n' % (
            options.level, options.build),
        'values': DataGenerator._class_names + [ 'Item', 'Consumable', 'Gem1', 'Gem2', 'Gem3', 'Health', 'DamageReplaceStat' ]
    })
    if not g.initialize():
        sys.exit(1)
    g.generate()

    g = CSVDataGenerator(options, {
        'file': 'BaseMp.txt',
        'comment': '// Base mana points for levels 1 - %d, wow build %s\n' % (
            options.level, options.build),
        'values': DataGenerator._class_names
    })
    if not g.initialize():
        sys.exit(1)
    g.generate()

    g = CSVDataGenerator(options, {
        'file': 'CombatRatings.txt',
        'comment': '// Combat rating values for level 1 - %d, wow build %s\n' % (
            options.level, options.build),
        'values': [ 'Dodge', 'Parry', 'Block', 'Hit - Melee', 'Hit - Ranged',
                    'Hit - Spell', 'Crit - Melee', 'Crit - Ranged', 'Crit - Spell',
                    'Resilience - Player Damage', 'Lifesteal', 'Haste - Melee', 'Haste - Ranged',
                    'Haste - Spell', 'Expertise', 'Mastery', 'PvP Power',
                    'Versatility - Damage Done', 'Versatility - Healing Done',
                    'Versatility - Damage Taken', 'Speed', 'Avoidance' ]
    })
    if not g.initialize():
        sys.exit(1)
    g.generate()

    combat_rating_values = [ 'Rating Multiplier' ]
    # CombatRatingsMultByILvl.txt gets more complicated starting 7.1.5,
    # earliest build published to the public is 23038 on PTR
    if options.build >= 23038:
        combat_rating_values = [ 'Armor Multiplier', 'Weapon Multiplier', 'Trinket Multiplier', 'Jewelry Multiplier' ]

    g = CSVDataGenerator(options, [ {
        'file': 'ItemSocketCostPerLevel.txt',
        'key': '5.0 Level',
        'comment': '// Item socket costs for item levels 1 - %d, wow build %s\n' % (
            options.max_ilevel, options.build),
        'values': [ 'Socket Cost' ],
        'max_rows': options.max_ilevel
    }, {
        'file': 'CombatRatingsMultByILvl.txt',
        'key': 'Item Level',
        'comment': '// Combat rating multipliers for item level 1 - %d, wow build %s\n' % (
            options.max_ilevel, options.build),
        'values': combat_rating_values,
        'max_rows': options.max_ilevel
    }, {
        'file': 'StaminaMultByILvl.txt',
        'key': 'Item Level',
        'comment': '// Stamina multipliers for item level 1 - %d, wow build %s\n' % (
            options.max_ilevel, options.build),
        'values': combat_rating_values,
        'max_rows': options.max_ilevel
    }, {
        'file': 'ItemLevelSquish.txt',
        'key': 0,
        'comment': '// Item level translation for item level 1 - %d, wow build %s\n' % (
            options.max_ilevel, options.build),
        'values': [ 1 ],
        'max_rows': options.max_ilevel,
        'simple_reader': True
    }])
    if not g.initialize():
        sys.exit(1)

    g.generate()

    g = CSVDataGenerator(options, {
        'file': 'AzeriteLevelToItemLevel.txt',
        'key': 'Azerite Level',
        'comment': '// Azerite level to item level 1 - %d, wow build %s\n' % (
            300, options.build),
        'values': [ 'Item Level' ],
        'base_type': 'unsigned',
        'max_rows': 300
    })
    if not g.initialize():
        sys.exit(1)

    g.generate()
