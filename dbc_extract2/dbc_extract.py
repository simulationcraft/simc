#!/usr/bin/env python3

import optparse, sys, os, glob, re, datetime, signal
import dbc.generator, dbc.db, dbc.parser

parser = optparse.OptionParser( usage= "%prog [-otlbp] [ARGS]", version = "%prog 1.0" )
parser.add_option("-t", "--type", dest = "type", 
                  help    = "Processing type [spell]", metavar = "TYPE", 
                  default = "spell", action = "store", type = "choice",
                  choices = [ 'spell', 'class_list', 'talent', 'scale', 'view', 'csv',
                              'header', 'spec_spell_list', 'mastery_list', 'racial_list', 'perk_list',
                              'glyph_list', 'glyph_property_list', 'class_flags', 'set_list', 'random_property_points', 'random_suffix',
                              'item_ench', 'weapon_damage', 'item', 'item_armor', 'gem_properties',
                              'random_suffix_groups', 'spec_enum', 'spec_list', 'item_upgrade',
                              'rppm_coeff', 'set_list2', 'item_bonus', 'item_scaling',
                              'item_name_desc' ]), 
parser.add_option("-o", dest = "output", action = "store", type = "str", default = None)
parser.add_option("-a", dest = "append", action = "store", type = "str", default = None)
parser.add_option("-f", dest = "format",
                  help = "DBC Format file",
                  action = "store", type = "str")
parser.add_option("--delim", dest = "delim",
                  help = "Delimiter for -t csv",
                  default = ",", action = "store", type = "str")
parser.add_option("-l", "--level", dest = "level", 
                  help    = "Scaling values up to level [115]", 
                  default = 115, action = "store", type = "int")
parser.add_option("-p", "--path", dest = "path", 
                  help    = "DBC input directory [cwd]", 
                  default = r'.', action = "store", type = "string")
parser.add_option("-b", "--build", dest = "build", 
                  help    = "World of Warcraft build number", 
                  default = 0, action = "store", type = "int")
parser.add_option("--prefix", dest = "prefix", 
                  help    = "Data structure prefix string", 
                  default = r'', action = "store", type = "string")
parser.add_option("--suffix", dest = "suffix", 
                  help    = "Data structure suffix string", 
                  default = r'', action = "store", type = "string")
parser.add_option("--min-ilvl", dest = "min_ilevel",
                  help    = "Minimum inclusive ilevel for item-related extraction",
                  default = 90, action = "store", type = "int" )
parser.add_option("--max-ilvl", dest = "max_ilevel",
                  help    = "Maximum inclusive ilevel for item-related extraction",
                  default = 940, action = "store", type = "int" )
parser.add_option("--scale-ilvl", dest = "scale_ilevel",
                  help    = "Maximum inclusive ilevel for game table related extraction",
                  default = 1000, action = "store", type = "int" )
parser.add_option("--cache", dest = "cache_dir",
                  help    = "World of Warcraft Cache directory.", 
                  default = r'', action = "store", type = "string" )
parser.add_option("--as", dest = "as_dbc", 
                  help    = "Treat given DBC file as this option",
                  action = "store", type = "string", default = '' )
parser.add_option("--debug", dest = "debug", default = False, action = "store_true")
(options, args) = parser.parse_args()

if options.build == 0 and options.type != 'header':
    parser.error('-b is a mandatory parameter for extraction type "%s"' % options.type)

if options.min_ilevel < 0 or options.max_ilevel > 999:
    parser.error('--min/max-ilevel range is 0..999')

if options.level % 5 != 0 or options.level > 115:
    parser.error('-l must be given as a multiple of 5 and be smaller than 100')

if options.type == 'view' and len(args) == 0:
    parser.error('View requires a DBC file name and an optional ID number')

if options.type == 'header' and len(args) == 0:
    parser.error('Header parsing requires at least a single DBC file to parse it from')

# Initialize the base model for dbc.data, creating the relevant classes for all patch levels
# up to options.build
dbc.data.initialize_data_model(options, dbc.data)

if options.type == 'spell':
    _start = datetime.datetime.now()
    g = dbc.generator.SpellDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    ids = g.filter()

    g.generate(ids)
    #sys.stderr.write('done, %s\n' % (datetime.datetime.now() - _start))
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
elif options.type == 'perk_list':
    g = dbc.generator.PerkSpellGenerator(options)
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
elif options.type == 'header':
    dbcs = [ ]
    for fn in args:
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
elif options.type == 'view':
    path = os.path.abspath(os.path.join(options.path, args[0]))
    id = None
    if len(args) > 1:
        id = int(args[1])

    dbc_file = dbc.parser.get_parser(options, path)
    if not dbc_file.open_dbc():
        sys.exit(1)

    if options.debug:
        print(dbc_file)
    if id == None:
        record = dbc_file.next_record()
        while record != None:
            #mo = re.findall(r'(\$(?:{.+}|[0-9]*(?:<.+>|[^l][A-z:;]*)[0-9]?))', str(record))
            #if mo:
            #    print record, set(mo)
            #if (record.flags_1 & 0x00000040) == 0:
            sys.stdout.write('%s\n' % str(record))
            record = dbc_file.next_record()
    else:
        record = dbc_file.find(id)
        print(record)
elif options.type == 'csv':
    path = os.path.abspath(os.path.join(options.path, args[0]))
    id = None
    if len(args) > 1:
        id = int(args[1])

    dbc_file = dbc.parser.DBCParser(options, path)
    if not dbc_file.open_dbc():
        sys.exit(1)

    first = True
    if id == None:
        record = dbc_file.next_record()
        while record != None:
            #mo = re.findall(r'(\$(?:{.+}|[0-9]*(?:<.+>|[^l][A-z:;]*)[0-9]?))', str(record))
            #if mo:
            #    print record, set(mo)
            #if (record.flags_1 & 0x00000040) == 0:
            if first:
                sys.stdout.write('%s\n' % record.field_names(options.delim))

            sys.stdout.write('%s\n' % record.csv(options.delim, first))
            record = dbc_file.next_record()
            first = False
    else:
        record = dbc_file.find(id)
        record.parse()
        print(record.csv(options.delim, first))

elif options.type == 'scale':
    g = dbc.generator.LevelScalingDataGenerator(options, [ 'gtOCTHpPerStamina' ] )
    if not g.initialize():
        sys.exit(1)
    g.generate()

    # Swap to appending
    options.append = options.output
    options.output = None

    g = dbc.generator.SpellScalingDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    g.generate()

    tables = [ 'gtChanceToMeleeCritBase', 'gtChanceToSpellCritBase', 'gtChanceToMeleeCrit', 'gtChanceToSpellCrit', 'gtRegenMPPerSpt', 'gtOCTBaseHPByClass', 'gtOCTBaseMPByClass' ]
    g = dbc.generator.ClassScalingDataGenerator(options, tables)
    if not g.initialize():
        sys.exit(1)
    g.generate()

    g = dbc.generator.CombatRatingsDataGenerator(options)
    if not g.initialize():
        sys.exit(1)
    g.generate()

    g = dbc.generator.IlevelScalingDataGenerator(options, 'gtItemSocketCostPerLevel')
    if not g.initialize():
        sys.exit(1)

    g.generate()

    g = dbc.generator.MonsterLevelScalingDataGenerator(options, 'gtArmorMitigationByLvl')
    if not g.initialize():
        sys.exit(1)

    g.generate()

