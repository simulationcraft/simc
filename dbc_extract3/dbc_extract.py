#!/usr/bin/env python3

import argparse, sys, os, glob, re, datetime, signal, logging
import dbc.generator, dbc.db, dbc.parser, dbc.file, dbc.config


logging.basicConfig(level = logging.INFO,
        datefmt = '%H:%M:%S',
        format = '[%(asctime)s] %(levelname)s: %(message)s')

parser = argparse.ArgumentParser(usage= "%(prog)s [-otlbp] [ARGS]")
parser.add_argument("-t", "--type", dest = "type", 
                  help    = "Processing type [spell]", metavar = "TYPE", 
                  default = "spell", action = "store",
                  choices = [ 'batchoutput', 'spell', 'class_list', 'talent', 'scale', 'view', 'csv',
                              'header', 'spec_spell_list', 'mastery_list', 'racial_list',
                              'class_flags', 'random_property_points', 'random_suffix',
                              'item_ench', 'weapon_damage', 'item', 'item_armor', 'gem_properties',
                              'random_suffix_groups', 'spec_enum', 'spec_list', 'item_upgrade',
                              'rppm_coeff', 'set_list2', 'item_bonus', 'item_scaling',
                              'item_name_desc', 'artifact', 'bench', 'item_child' ])
parser.add_argument("-o",            dest = "output")
parser.add_argument("-a",            dest = "append")
parser.add_argument("--raw",         dest = "raw",          default = False, action = "store_true")
parser.add_argument("--debug",       dest = "debug",        default = False, action = "store_true")
parser.add_argument("-f",            dest = "format",
                    help = "DBC Format file")
parser.add_argument("--delim",       dest = "delim",        default = ',',
                    help = "Delimiter for -t csv")
parser.add_argument("-l", "--level", dest = "level",        default = 110, type = int,
                    help = "Scaling values up to level [115]")
parser.add_argument("-b", "--build", dest = "build",        default = 0, type = int,
                    help = "World of Warcraft build number")
parser.add_argument("--prefix",      dest = "prefix",       default = '',
                    help = "Data structure prefix string")
parser.add_argument("--suffix",      dest = "suffix",       default = '',
                    help = "Data structure suffix string")
parser.add_argument("--min-ilvl",    dest = "min_ilevel",   default = 90, type = int,
                    help = "Minimum inclusive ilevel for item-related extraction")
parser.add_argument("--max-ilvl",    dest = "max_ilevel",   default = 940, type = int,
                    help = "Maximum inclusive ilevel for item-related extraction")
parser.add_argument("--scale-ilvl",  dest = "scale_ilevel", default = 1000, type = int,
                    help = "Maximum inclusive ilevel for game table related extraction")
parser.add_argument("--as",          dest = "as_dbc",       default = '',
                    help = "Treat given DBC file as this option" )
parser.add_argument("-p", "--path",  dest = "path",         default = '.',
                    help = "DBC input directory [cwd]")
parser.add_argument("--cache",       dest = "cache_dir",    default = '',
                    help = "World of Warcraft Cache directory.")
parser.add_argument("--wdbfile",     dest = "wdb_file",     default = '',
                    help = "Path to WDB file to determine attributes when using 'view' type on adb files")
parser.add_argument("args", metavar = "ARGS", type = str, nargs = argparse.REMAINDER)
options = parser.parse_args()

if options.build == 0 and options.type != 'header':
    parser.error('-b is a mandatory parameter for extraction type "%s"' % options.type)

if options.min_ilevel < 0 or options.max_ilevel > 999:
    parser.error('--min/max-ilevel range is 0..999')

if options.level % 5 != 0 or options.level > 115:
    parser.error('-l must be given as a multiple of 5 and be smaller than 100')

if options.type == 'view' and len(options.args) == 0:
    parser.error('View requires a DBC file name and an optional ID number')

if options.type == 'header' and len(options.args) == 0:
    parser.error('Header parsing requires at least a single DBC file to parse it from')

if options.cache_dir and not os.path.isdir(options.cache_dir):
    parser.error('Invalid cache directory %s' % options.cache_dir)

if options.debug:
    logging.getLogger().setLevel(logging.DEBUG)

# Initialize the base model for dbc.data, creating the relevant classes for all patch levels
# up to options.build
dbc.data.initialize_data_model(options, dbc.data)

if options.type == 'batchoutput':
    config = dbc.config.Config(options)
    if not config.open():
        sys.exit(1)

    config.generate()

if options.type == 'spell':
    _start = datetime.datetime.now()
    g = dbc.generator.SpellDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
    #sys.stderr.write('done, %s\n' % (datetime.datetime.now() - _start))
    #input()
elif options.type == 'class_list':
    g = dbc.generator.SpellListGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
elif options.type == 'class_flags':
    g = dbc.generator.ClassFlagGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter(args[0])

    g.generate(ids)
elif options.type == 'racial_list':
    g = dbc.generator.RacialSpellGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
elif options.type == 'mastery_list':
    g = dbc.generator.MasteryAbilityGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'spec_spell_list':
    g = dbc.generator.SpecializationSpellGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'random_property_points':
    g = dbc.generator.RandomPropertyPointsGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'random_suffix':
    g = dbc.generator.RandomSuffixGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'random_suffix_groups':
    g = dbc.generator.RandomSuffixGroupGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
elif options.type == 'item':
    g = dbc.generator.ItemDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'item_upgrade':
    g = dbc.generator.RulesetItemUpgradeGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    g.generate(ids)

    if options.output:
        options.append = options.output
        options.output = None

    g = dbc.generator.ItemUpgradeDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    g.generate()
elif options.type == 'item_ench':
    g = dbc.generator.SpellItemEnchantmentGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'weapon_damage':
    g = dbc.generator.WeaponDamageDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'item_armor':
    g = dbc.generator.ArmorValueDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
    if options.output:
        options.append = options.output
        options.output = None

    g = dbc.generator.ArmorSlotDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'gem_properties':
    g = dbc.generator.GemPropertyDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'glyph_list':
    g = dbc.generator.GlyphListGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'glyph_property_list':
    g = dbc.generator.GlyphPropertyGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'spec_enum':
    g = dbc.generator.SpecializationEnumGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
elif options.type == 'rppm_coeff':
    g = dbc.generator.RealPPMModifierGenerator(options)
    if not g.initialize():
        sys.exit(1)

    g.generate()
elif options.type == 'spec_list':
    g = dbc.generator.SpecializationListGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'set_list2':
    g = dbc.generator.SetBonusListGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'item_bonus':
    g = dbc.generator.ItemBonusDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
elif options.type == 'item_scaling':
    g = dbc.generator.ScalingStatDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
elif options.type == 'item_name_desc':
    g = dbc.generator.ItemNameDescriptionDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
elif options.type == 'item_child':
    g = dbc.generator.ItemChildEquipmentGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
elif options.type == 'artifact':
    g = dbc.generator.ArtifactDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
elif options.type == 'header':
    dbcs = [ ]
    for fn in options.args:
        for i in glob.glob(fn):
            dbcs.append(i)
            
    for i in dbcs:
        dbc_file = dbc.parser.DBCParser(options, i)
        if not dbc_file.open_dbc():
            continue

        sys.stdout.write('%s\n' % dbc_file)
elif options.type == 'talent':
    g = dbc.generator.TalentDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()
    
    g.generate(ids)
elif options.type == 'bench':
    path = os.path.abspath(os.path.join(options.path, options.args[0]))
    dbc_file = dbc.file.DBCFile(options, path)
    if not dbc_file.open():
        sys.exit(1)

    x = {}
    logging.debug(dbc_file)
    for record in dbc_file:
        #x[record.id] = record
        pass
elif options.type == 'view':
    path = os.path.abspath(os.path.join(options.path, options.args[0]))
    id = 0
    if len(options.args) > 1:
        id = int(options.args[1])

    dbc_file = dbc.file.DBCFile(options, path)
    if not dbc_file.open():
        sys.exit(1)

    logging.debug(dbc_file)
    if id == 0:
        for record in dbc_file:
            sys.stdout.write('%s\n' % str(record))
    else:
        if options.raw and not dbc_file.searchable():
            logging.error('DBC file %s is not searchable in raw mode', path)
            sys.exit(1)
        else:
            record = dbc_file.find(id)
            if record:
                print(record)
            else:
                print('No record for DBC ID %d found', id)
elif options.type == 'csv':
    path = os.path.abspath(os.path.join(options.path, options.args[0]))
    id = None
    if len(options.args) > 1:
        id = int(options.args[1])

    dbc_file = dbc.file.DBCFile(options, path)
    if not dbc_file.open():
        sys.exit(1)

    first = True
    logging.debug(dbc_file)
    if id == None:
        for record in dbc_file:
            if first:
                sys.stdout.write('%s\n' % record.field_names(options.delim))

            sys.stdout.write('%s\n' % record.csv(options.delim, first))
            first = False
    else:
        if options.raw and not dbc_file.searchable():
            logging.error('DBC file %s is not searchable in raw mode', path)
            sys.exit(1)
        else:
            record = dbc_file.find(id)
            if record:
                print(record.csv(options.delim, first))
            else:
                print('No record for DBC ID %d found', id)

elif options.type == 'scale':
    g = dbc.generator.CSVDataGenerator(options, {
        'file': 'HpPerSta.txt',
        'comment': '// Hit points per stamina for level 1 - %d, wow build %d\n' % (
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

    g = dbc.generator.CSVDataGenerator(options, {
        'file': 'SpellScaling.txt',
        'comment': '// Spell scaling multipliers for levels 1 - %d, wow build %d\n' % (
            options.level, options.build),
        'values': dbc.generator.DataGenerator._class_names + [ 'Item', 'Consumable', 'Gem1', 'Gem2', 'Gem3', 'Health' ]
    })
    if not g.initialize():
        sys.exit(1)
    g.generate()

    g = dbc.generator.CSVDataGenerator(options, {
        'file': 'BaseMp.txt',
        'comment': '// Base mana points for levels 1 - %d, wow build %d\n' % (
            options.level, options.build),
        'values': dbc.generator.DataGenerator._class_names
    })
    if not g.initialize():
        sys.exit(1)
    g.generate()

    g = dbc.generator.CSVDataGenerator(options, {
        'file': 'CombatRatings.txt',
        'comment': '// Combat rating values for level 1 - %d, wow build %d\n' % (
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

    g = dbc.generator.CSVDataGenerator(options, [ {
        'file': 'ItemSocketCostPerLevel.txt',
        'key': '5.0 Level',
        'comment': '// Item socket costs for item levels 1 - %d, wow build %d\n' % (
            options.max_ilevel, options.build),
        'values': [ 'Socket Cost' ],
        'max_rows': options.max_ilevel
    }, {
        'file': 'CombatRatingsMultByILvl.txt',
        'key': 'Item Level',
        'comment': '// Combat rating multipliers for item level 1 - %d, wow build %d\n' % (
            options.max_ilevel, options.build),
        'values': combat_rating_values,
        'max_rows': options.max_ilevel
    }])
    if not g.initialize():
        sys.exit(1)

    g.generate()

    g = dbc.generator.CSVDataGenerator(options, {
        'file': 'ArmorMitigationByLvl.txt',
        'comment': '// Enemy armor mitigation constants (K-value) for level 1 - %d, wow build %d\n' % (
            options.level + 3, options.build),
        'values': [ 'Mitigation' ],
        'max_rows': options.level + 3
    })
    if not g.initialize():
        sys.exit(1)

    g.generate()

