import sys, os, re, types, HTMLParser, urllib2

import parser, db, data, constants

def _rune_cost(generator, filter_data, record, *args):
    cost = 0
    for rune_type in xrange(0, 4):
        for i in xrange(0, getattr(record, 'rune_cost_%d' % (rune_type + 1))):
            cost |= 1 << (rune_type * 2 + i)
    
    return args[0] % cost

class DataGenerator(object):
    _class_names = [ None, 'Warrior', 'Paladin', 'Hunter', 'Rogue',     'Priest', 'Death Knight', 'Shaman', 'Mage',  'Warlock', 'Monk',       'Druid'  ]
    _class_masks = [ None, 0x1,       0x2,       0x4,      0x8,         0x10,     0x20,           0x40,     0x80,    0x100,     0x200,        0x400   ]
    _race_names  = [ None, 'Human',   'Orc',     'Dwarf',  'Night Elf', 'Undead', 'Tauren',       'Gnome',  'Troll', 'Goblin',  'Blood Elf', 'Draenei' ] + [ None ] * 10 + [ 'Worgen', None, 'Pandaren', 'Pandaren', 'Pandaren', None ]
    _race_masks  = [ None, 0x1,       0x2,       0x4,      0x8,         0x10,     0x20,           0x40,     0x80,    0x100,     0x200,       0x400     ] + [ None ] * 10 + [ 0x200000, None, 0x800000, 0x1000000, 0x2000000, None ]
    _pet_names   = [ None, 'Ferocity', 'Tenacity', None, 'Cunning' ]
    _pet_masks   = [ None, 0x1,        0x2,        None, 0x4       ]

    def debug(self, msg):
        if self._options.debug == True:
            sys.stderr.write("%s: %s\n" % ( self.__class__.__name__, msg ))

    def dbc_version(self, wow, build):
        if self._options.wowversion == 0:
            return self._options.build >= build
        else:
            return self._options.wowversion >= wow and self._options.build >= build
    
    def __init__(self, options):
        self._options = options

        self._class_map = { }
        # Build some maps to help us output things
        for i in xrange(0, len(DataGenerator._class_names)):
            if not DataGenerator._class_names[i]:
                continue

            self._class_map[DataGenerator._class_names[i]] = i
            self._class_map[1 << (i - 1)] = i
            #self._class_map[DataGenerator._class_masks[i]] = i

        self._race_map = { }
        for i in xrange(0, len(DataGenerator._race_names)):
            if not DataGenerator._race_names[i]:
                continue

            self._race_map[DataGenerator._race_names[i]] = i
            self._race_map[1 << (i - 1)] = i
        
        #print self._class_map, self._race_map
    
    def format_str(self, string):
        return '%s%s%s' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            string,
            self._options.suffix and ('_%s' % self._options.suffix) or '' )

    def initialize(self):
        for i in self._dbc:
            dbcname = i.replace('-', '_').lower()
            setattr(self, '_%s' % dbcname, 
                parser.DBCParser(self._options, os.path.abspath(os.path.join(self._options.path, i))))
            dbc = getattr(self, '_%s' % dbcname)

            if not dbc.open_dbc():
                return False

            if '_%s_db' % dbc.name() not in dir(self):
                setattr(self, '_%s_db' % dbc.name(), db.DBCDB(dbc._class))

            dbase = getattr(self, '_%s_db' % dbc.name())
            record = dbc.next_record()
            while record != None:
                dbase[record.id] = record
                record = dbc.next_record()

        if not self._options.cache_dir or not os.access(os.path.abspath(self._options.cache_dir), os.R_OK):
            return True

        cache_files = []
        files = os.listdir(self._options.cache_dir)
        for f in files:
            fn = f[:f.find('.')]
            if fn in self._dbc:
                cache_files.append((fn, os.path.abspath(os.path.join(self._options.cache_dir, f))))
        
        cache_parsers = { }
        for cache_file in cache_files:
            if cache_file[0] not in cache_parsers:
                cache_parsers[cache_file[0]] = { 'parsers': [], 'ids': [ ] }

            p = parser.DBCParser(self._options, cache_file[1])
            if not p.open_dbc():
                continue

            cache_parsers[cache_file[0]]['parsers'].append(p)

        for dbc, data in cache_parsers.iteritems():
            if len(data['parsers']) == 0:
                continue

            data['parsers'].sort(cmp = lambda x, y: y._timestamp - x._timestamp)
            dbase = getattr(self, '_%s_db' % data['parsers'][0].name())

            for cache_parser in data['parsers']:
                record = cache_parser.next_record()
                while record != None:
                    if record.id not in data['ids']:
                        if dbase.get(record.id):
                            self.debug('Overwrote id %d using cache %s' % (record.id, cache_parser._fname))
                        else:
                            self.debug('Added id %d using cache %s' % (record.id, cache_parser._fname))
                        dbase[record.id] = record
                        data['ids'].append(record.id)
                    record = cache_parser.next_record()

        return True

    def filter(self):
        return None

    def generate(self, ids = None):
        return ''

class RealPPMModifierGenerator(DataGenerator):
    def __init__(self, options):
        DataGenerator.__init__(self, options)

        self._dbc = [ 'ChrSpecialization', 'SpellProcsPerMinute', 'SpellProcsPerMinuteMod', 'SpellAuraOptions' ]
        self._specmap = { 0: 'SPEC_NONE' }

    def initialize(self):
        DataGenerator.initialize(self)

        for i, data in self._chrspecialization_db.iteritems():
            if data.class_id > 0:
                self._specmap[i] = '%s_%s' % (
                    DataGenerator._class_names[data.class_id].upper().replace(" ", "_"),
                    data.name.upper().replace(" ", "_"),
                )

        return True

    def generate(self, ids = None):
        output_data = []
        
        for i, data in self._spellprocsperminutemod_db.iteritems():
            if data.id_chr_spec not in self._specmap.keys() or data.id_chr_spec == 0:
                continue

            spell_id = 0
            for aopts_id, aopts_data in self._spellauraoptions_db.iteritems():
                if aopts_data.id_ppm != data.id_ppm:
                    continue
                
                spell_id = aopts_data.id_spell
                break

            if spell_id == 0:
                continue

            output_data.append((data.id_chr_spec, data.coefficient, spell_id))
        
        output_data.sort(cmp = lambda l, r: l[2] - r[2])

        s = '#include "specialization.hpp"\n'
        s += '#include "data_definitions.hh"\n\n'
        s += '#define %sRPPMMOD%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(output_data)
        )
        s += '// %d RPPM Modifiers, wow build level %d\n' % ( len(output_data), self._options.build )
        s += 'static struct rppm_modifier_t __%srppmmodifier%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''
        )

        for data in output_data + [(0, 0, 0)]:
            s += '  { %6u, %-20s, %.4f },\n' % (data[2], self._specmap[data[0]], data[1])

        s += '};\n'

        return s

class SpecializationEnumGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'ChrSpecialization' ]
        
        DataGenerator.__init__(self, options)
    
    
    def generate(self, ids = None):
        enum_ids = [
            [ None, None, None, None ], # pets come here
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
        ]
        
        spec_translations = [
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
        ]
        
        spec_to_idx_map = [ ]
        max_specialization = 0
        for spec_id, spec_data in self._chrspecialization_db.iteritems():
            if spec_data.class_id > 0:
                spec_name = '%s_%s' % (
                    DataGenerator._class_names[spec_data.class_id].upper().replace(" ", "_"),
                    spec_data.name.upper().replace(" ", "_"),
                )

                if spec_data.spec_id > max_specialization:
                    max_specialization = spec_data.spec_id

                if len(spec_to_idx_map) < spec_id + 1:
                    spec_to_idx_map += [ -1 ] * ( ( spec_id - len(spec_to_idx_map) ) + 1 )

                spec_to_idx_map[ spec_id ] = spec_data.spec_id
            else:
                spec_name = 'PET_%s' % (
                    spec_data.name.upper().replace(" ", "_")
                )

            for i in xrange(0, (max_specialization + 1) - len(enum_ids[ spec_data.class_id ] ) ):
                enum_ids[ spec_data.class_id ].append( None )
            
            enum_ids[ spec_data.class_id ][ spec_data.spec_id ] = { 'id': spec_id, 'name': spec_name }

        spec_arr = []
        s  = 'enum specialization_e {\n'
        s += '  SPEC_NONE            = 0,\n'
        s += '  SPEC_PET             = 1,\n'
        for cls in xrange(0, len(enum_ids)):
            if enum_ids[cls][0] == None:
                continue
            
            for spec in xrange(0, len(enum_ids[cls])):
                if enum_ids[cls][spec] == None:
                    continue

                enum_str = '  %s%s= %u,\n' % (
                    enum_ids[cls][spec]['name'],
                    ( 21 - len(enum_ids[cls][spec]['name']) ) * ' ',
                    enum_ids[cls][spec]['id'] )
                
                s += enum_str
                spec_arr.append('%s' % enum_ids[cls][spec]['name'])
        s += '};\n\n'

        spec_idx_str = ''
        for i in xrange(0, len(spec_to_idx_map)):
            if i % 25 == 0:
                spec_idx_str += '\n  '

            spec_idx_str += '%2d' % spec_to_idx_map[i]
            if i < len(spec_to_idx_map) - 1:
                spec_idx_str += ','
                if (i + 1) % 25 != 0:
                    spec_idx_str += ' '

        # Ugliness abound, but the easiest way to iterate over all specs is this ...
        s += 'namespace specdata {\n';
        s += 'static const unsigned n_specs = %u;\n' % len(spec_arr);
        s += 'static const specialization_e __specs[%u] = {\n  %s\n};\n\n' % (len(spec_arr), ', \n  '.join(spec_arr))
        s += 'static const int __idx_specs[%u] = {%s\n};\n\n' % (len(spec_to_idx_map), spec_idx_str)
        s += 'inline unsigned spec_count()\n{ return n_specs; }\n\n';
        s += 'inline specialization_e spec_id( unsigned idx )\n{ assert( idx < n_specs ); return __specs[ idx ]; }\n\n'
        s += 'inline int spec_idx( specialization_e spec )\n{ assert( spec < %d ); return __idx_specs[ spec ]; }\n\n' % len(spec_to_idx_map)
        s += '}\n'
               
        return s

class SpecializationListGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'ChrSpecialization' ]
        
        DataGenerator.__init__(self, options)
    
    
    def generate(self, ids = None):
        enum_ids = [
            [ None, None, None, None ], # pets come here
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
            [ None, None, None, None ],
        ]
        
        spec_translations = [
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
            [],
        ]
        
        max_specialization = 0
        for spec_id, spec_data in self._chrspecialization_db.iteritems():
            if spec_data.class_id > 0:
                spec_name = '%s_%s' % (
                    DataGenerator._class_names[spec_data.class_id].upper().replace(" ", "_"),
                    spec_data.name.upper().replace(" ", "_"),
                )
            else:
                spec_name = 'PET_%s' % (
                    spec_data.name.upper().replace(" ", "_")
                )
                
            if spec_data.spec_id > max_specialization:
                max_specialization = spec_data.spec_id
            
            for i in xrange(0, (max_specialization + 1) - len(enum_ids[ spec_data.class_id ] ) ):
                enum_ids[ spec_data.class_id ].append( None )
            
            enum_ids[ spec_data.class_id ][ spec_data.spec_id ] = { 'id': spec_id, 'name': spec_name }
        
        s  = '#define MAX_SPECS_PER_CLASS (%u)\n' % (max_specialization + 1)
        s += '#define MAX_SPEC_CLASS  (%u)\n\n' % len(enum_ids)

        s += 'static specialization_e __class_spec_id[MAX_SPEC_CLASS][MAX_SPECS_PER_CLASS] = \n{\n'
        for cls in xrange(0, len(enum_ids)):
            if enum_ids[cls][0] == None:
                s += '  {\n'
                s += '    SPEC_NONE,\n'
                s += '  },\n'
                continue
            
            s += '  {\n'
            for spec in xrange(0, len(enum_ids[cls])):
                if enum_ids[cls][spec] == None:
                    s += '    SPEC_NONE,\n'
                    continue

                s += '    %s,\n' % enum_ids[cls][spec]['name']

            s += '  },\n'
            
        s += '};\n\n'
        
        return s

class BaseScalingDataGenerator(DataGenerator):
    def __init__(self, options, scaling_data):
        if isinstance(scaling_data, str):
            self._dbc = [ scaling_data ]
        else:
            self._dbc = scaling_data

        DataGenerator.__init__(self, options)

    def generate(self, ids = None):
        s = ''

        for i in self._dbc:
            s += '// Base scaling data for classes, wow build %d\n' % self._options.build
            s += 'static double __%s%s%s[] = {\n' % ( 
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                re.sub(r'([A-Z]+)', r'_\1', i).lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '' )
            s += '%20.15f, ' % 0

            for k in xrange(0, len(self._class_names) - 1):
                val = getattr(self, '_%s_db' % i.lower())[k]

                s += '%20.15f, ' % val.gt_value

                if k > 0 and (k + 2) % 5 == 0:
                    s += '\n'
            
            s += '\n};\n\n'

        return s

class LevelScalingDataGenerator(DataGenerator):
    def __init__(self, options, scaling_data):
        if isinstance(scaling_data, str):
            self._dbc = [ scaling_data ]
        else:
            self._dbc = scaling_data

        DataGenerator.__init__(self, options)

    def generate(self, ids = None):
        s = ''

        for i in self._dbc:
            s += '// Level scaling data, wow build %d\n' % self._options.build
            s += 'static double __%s%s%s[%u] = {\n' % ( 
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                re.sub(r'([A-Z]+)', r'_\1', i).lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '',
                self._options.level)

            for k in xrange(0, self._options.level):
                val = getattr(self, '_%s_db' % i.lower())[k]

                s += '%20.15f, ' % val.gt_value

                if k > 0 and (k + 1) % 5 == 0:
                    s += '\n'

            s += '\n};\n\n'

        return s

class MonsterLevelScalingDataGenerator(DataGenerator):
    def __init__(self, options, scaling_data):
        if isinstance(scaling_data, str):
            self._dbc = [ scaling_data ]
        else:
            self._dbc = scaling_data

        DataGenerator.__init__(self, options)

    def generate(self, ids = None):
        s = ''

        for i in self._dbc:
            s += '// Monster(?) Level scaling data, wow build %d\n' % self._options.build
            s += 'static double __%s%s%s[%u] = {\n' % ( 
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                re.sub(r'([A-Z]+)', r'_\1', i).lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '',
                self._options.level + 3)

            for k in xrange(0, self._options.level + 3):
                val = getattr(self, '_%s_db' % i.lower())[k]

                s += '%20.10f' % val.gt_value
                if k < self._options.level + 3 - 1:
                    s += ', '

                if k > 0 and (k + 1) % 5 == 0:
                    s += '\n'

            s += '\n};\n\n'

        return s

class IlevelScalingDataGenerator(DataGenerator):
    def __init__(self, options, scaling_data):
        if isinstance(scaling_data, str):
            self._dbc = [ scaling_data ]
        else:
            self._dbc = scaling_data

        DataGenerator.__init__(self, options)

    def generate(self, ids = None):
        s = ''

        for i in self._dbc:
            s += '// Item Level scaling data, wow build %d\n' % self._options.build
            s += 'static double __%s%s%s[%u] = {\n' % ( 
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                re.sub(r'([A-Z]+)', r'_\1', i).lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '',
                self._options.scale_ilevel)

            for k in xrange(0, self._options.scale_ilevel):
                val = getattr(self, '_%s_db' % i.lower())[k]

                s += '%20.15f, ' % val.gt_value

                if k > 0 and (k + 1) % 5 == 0:
                    s += '\n'

            s += '\n};\n\n'

        return s

class CombatRatingsDataGenerator(DataGenerator):
    # From UIParent.lua, seems to match to gtCombatRatings too for lvl80 stuff
    _combat_ratings = [ 'Dodge',               'Parry',                  'Block',       'Melee hit',   'Ranged hit',
                        'Spell hit',           'Melee crit',             'Ranged crit', 'Spell crit',  'Multistrike',
                        'Readiness',           'PvP Resilience',         'Leech',       'Melee haste', 'Ranged haste',
                        'Spell haste',         'Expertise',              'Mastery',     'PvP Power',   'Damage Versatility',
                        'Healing Versatility', 'Mitigation Versatility', 'Speed',       'Avoidance' ]
    _combat_rating_ids = [  2,  3,  4,  5,  6,
                            7,  8,  9, 10, 11,
                           12, 15, 16, 17, 18,
                           19, 23, 25, 26, 28,
                           29, 30, 13, 20 ]
    def __init__(self, options):
        # Hardcode these, as we need two different kinds of databases for output, using the same combat rating ids
        self._dbc = [ 'gtCombatRatings' ]

        DataGenerator.__init__(self, options)

    def generate(self, ids = None):
        s = ''

        db = self._gtcombatratings_db
        s += '// Combat ratings for levels 1 - %d, wow build %d \n' % ( 
            self._options.level, self._options.build )
        s += 'static double __%s%s%s[][%d] = {\n' % ( 
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            re.sub(r'([A-Z]+)', r'_\1', self._dbc[0]).lower(),
            self._options.suffix and ('_%s' % self._options.suffix) or '', 
            self._options.level )
        for j in xrange(0, len(CombatRatingsDataGenerator._combat_rating_ids)):
            s += '  // %s rating multipliers\n' % CombatRatingsDataGenerator._combat_ratings[j]
            s += '  {\n'
            m  = CombatRatingsDataGenerator._combat_rating_ids[j]
            for k in xrange(m * 123, m * 123 + self._options.level, 5):
                s += '    %20.15f, %20.15f, %20.15f, %20.15f, %20.15f,\n' % (
                    db[k].gt_value, db[k + 1].gt_value, db[k + 2].gt_value,
                    db[k + 3].gt_value, db[k + 4].gt_value )

            s += '  },\n'
        
        s += '};\n\n'

#        db = self._gtoctclasscombatratingscalar_db
#        s += '// Combat Rating scalar multipliers for classes, wow build %d\n' % self._options.build
#        s += 'static double __%s%s%s[][%d] = {\n' % ( 
#            self._options.prefix and ('%s_' % self._options.prefix) or '',
#            re.sub(r'([A-Z]+)', r'_\1', self._dbc[1]).lower(), 
#            self._options.suffix and ('_%s' % self._options.suffix) or '',
#            len(self._class_names))
#        for i in xrange(0, len(CombatRatingsDataGenerator._combat_rating_ids)):
#            id = CombatRatingsDataGenerator._combat_rating_ids[i]
#            s += '  // %s rating class scalar multipliers\n' % CombatRatingsDataGenerator._combat_ratings[i]
#            s += '  { \n'
#            s += '    %20.15f, %20.15f, %20.15f, %20.15f, %20.15f,\n' % (
#                0.0, db[id * 10 + 1].gt_value, db[id * 10 + 2].gt_value, db[id * 10 + 3].gt_value,
#                db[id * 10 + 4].gt_value)
#
#            s += '    %20.15f, %20.15f, %20.15f, %20.15f, %20.15f,\n' % (
#                db[id * 10 + 5].gt_value, db[id * 10 + 6].gt_value, db[id * 10 + 7].gt_value,
#                db[id * 10 + 8].gt_value, db[id * 10 + 9].gt_value )
#
#            s += '    %20.15f, %20.15f\n' % ( db[i * 10 + 10].gt_value, db[i * 10 + 11].gt_value )
#
#            s += '  },\n'
#        
#       s += '};\n\n'

        return s

class ClassScalingDataGenerator(DataGenerator):
    def __init__(self, options, scaling_data):
        if isinstance(scaling_data, str):
            self._dbc = [ scaling_data ]
        else:
            self._dbc = scaling_data

        DataGenerator.__init__(self, options)

    def generate(self, ids = None):
        s = ''
        for i in self._dbc:
            db = getattr(self, '_%s_db' % i.lower())
            s += '// Class based scaling multipliers for levels 1 - %d, wow build %d\n' % ( 
                self._options.level, self._options.build )
            s += 'static double __%s%s%s[][%d] = {\n' % ( 
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                re.sub(r'([A-Z]+)', r'_\1', i).lower(), 
                self._options.suffix and ('_%s' % self._options.suffix) or '',
                self._options.level )
            
            for j in xrange(0, len(self._class_names)):
                # Last entry is the fixed data
                if j < len(self._class_names) and self._class_names[j] != None:
                    s += '  // %s\n' % self._class_names[j]
            
                s += '  {\n'
                for k in xrange((j - 1) * 123, (j - 1) * 123 + self._options.level, 5):
                    s += '    %20.15f, %20.15f, %20.15f, %20.15f, %20.15f,\n' % (
                        db[k].gt_value, db[k + 1].gt_value, db[k + 2].gt_value, 
                        db[k + 3].gt_value, db[k + 4].gt_value
                    )
                s += '  },\n'
            
            s += '};\n\n'

        return s

class SpellScalingDataGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'gtSpellScaling' ]

        DataGenerator.__init__(self, options)

    def generate(self, ids = None):
        db = self._gtspellscaling_db

        s = ''
        s += '// Spell scaling multipliers for levels 1 - %d, wow build %d\n' % ( 
            self._options.level, self._options.build )
        s += 'static double __%s%s%s[][%d] = {\n' % ( 
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            re.sub(r'([A-Z]+)', r'_\1', self._dbc[0]).lower(), 
            self._options.suffix and ('_%s' % self._options.suffix) or '',
            self._options.level )
        
        for j in xrange(0, len(self._class_names) + 5):
            # Last entry is the fixed data
            if j < len(self._class_names) and self._class_names[j] != None:
                s += '  // %s\n' % self._class_names[j]
            else:
                s += '  // Constant scaling\n'
        
            s += '  {\n'
            for k in xrange((j - 1) * 123, (j - 1) * 123 + self._options.level, 5):
                s += '    %20.15f, %20.15f, %20.15f, %20.15f, %20.15f,\n' % (
                    db[k].gt_value, db[k + 1].gt_value, db[k + 2].gt_value, 
                    db[k + 3].gt_value, db[k + 4].gt_value
                )
            s += '  },\n'
        
        s += '};\n\n'

        return s

class TalentDataGenerator(DataGenerator):
    def __init__(self, options):
        DataGenerator.__init__(self, options)

        self._dbc = [ 'Spell', 'Talent' ]
        self._dbc.append( 'ChrSpecialization' )


    def filter(self):
        ids = [ ]

        for talent_id, talent_data in self._talent_db.iteritems():
            # Make sure at least one spell id is defined
            if talent_data.id_spell == 0: 
                continue

            # Make sure the "base spell" exists
            if not self._spell_db.get(talent_data.id_spell):
                continue

            ids.append(talent_id)

        return ids

    def generate(self, ids = None):
        # Sort keys
        ids.sort()
        
        s = '#define %sTALENT%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        )
        s += '// %d talents, wow build %d\n' % ( len(ids), self._options.build )
        s += 'static struct talent_data_t __%stalent%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' )

        index = 0
        for id in ids + [ 0 ]:
            talent     = self._talent_db[id]
            spell      = self._spell_db[talent.id_spell]
            if not spell.id and talent.id_spell > 0:
                continue

            if( index % 20 == 0 ):
                s += '//{ Name                                ,    Id, Flgs,  Class, Spc, Col, Row, SpellID, ReplaceID, S1 },\n'

            fields = spell.field('name')
            fields += talent.field('id')
            fields += [ '%#.2x' % 0 ]
            fields += [ '%#.04x' % (DataGenerator._class_masks[talent.class_id] or 0) ]
            fields += talent.field('spec_id')
                           
            fields += talent.field('col','row', 'id_spell', 'id_replace' )
            # Pad struct with empty pointers for direct rank based spell data access
            fields += [ ' 0' ]
        
            s += '  { %s },\n' % (', '.join(fields))

            index += 1

        s += '};'

        return s

class RulesetItemUpgradeGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'RulesetItemUpgrade' ]

        DataGenerator.__init__(self, options)

    def generate(self, ids = None):
        s = 'static item_upgrade_rule_t __%s_data[] = {\n' % (
            self.format_str( 'item_upgrade_rule' ),
        )

        for id_ in sorted(self._rulesetitemupgrade_db.keys()) + [ 0 ]:
            rule = self._rulesetitemupgrade_db[id_]
            s += '  { %s },\n' % (', '.join(rule.field('id', 'upgrade_level', 'id_upgrade_base', 'id_item')))
        
        s += '};\n\n'
        
        return s

class ItemUpgradeDataGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'ItemUpgrade' ]
        
        DataGenerator.__init__(self, options)

    def generate(self, ids = None):
        s = 'static item_upgrade_t __%s_data[] = {\n' % (
            self.format_str( 'item_upgrade' ),
        )

        for id_ in sorted(self._itemupgrade_db.keys()) + [ 0 ]:
            upgrade = self._itemupgrade_db[id_]
            s += '  { %s },\n' % (', '.join(upgrade.field('id', 'upgrade_ilevel')))
        
        s += '};\n\n'
        
        return s

class ItemDataGenerator(DataGenerator):
    _item_blacklist = [
        17,    138,   11671, 11672,                 # Various non-existing items
        27863, 27864, 37301, 38498,
        40948, 41605, 41606, 43336,
        43337, 43362, 43384, 55156,
        55159, 65015, 65104, 51951,
        51953, 52313, 52315, 
        68711, 68713, 68710, 68709,                 # Tol Barad trinkets have three versions, only need two, blacklist the "non faction" one
        62343, 62345, 62346, 62333,                 # Therazane enchants that have faction requirements
        50338, 50337, 50336, 50335,                 # Sons of Hodir enchants that have faction requirements
        50370, 50368, 50367, 50369, 50373, 50372,   # Wotlk head enchants that have faction requirements
        62367, 68719, 62368, 68763, 68718, 62366,   # Obsolete Cataclysm head enchants 
        68721, 62369, 62422, 68722,                 #
        43097,                                      #
    ]
    
    _item_name_blacklist = [
        "^(Lesser |)Arcanum of",
        "^Scroll of Enchant",
        "^Enchant ",
        "Deprecated",
        "DEPRECATED",
        "QA",
        "zzOLD",
        "NYI",
    ]
    
    _type_flags = {
        "Raid Finder"   : 0x01,
        "Heroic"        : 0x02,
        "Flexible"      : 0x04,
        "Elite"         : 0x10, # Meta type
        "Timeless"      : 0x10,
        "Thunderforged" : 0x10,
        "Warforged"     : 0x10,

        # Combinations
        "Heroic Thunderforged" : 0x12,
        "Heroic Warforged"     : 0x12,
    }

    def __init__(self, options):
        self._dbc = [ 'Item-sparse', 'Item', 'ItemEffect', 'SpellEffect', 'Spell', 'JournalEncounterItem', 'ItemNameDescription' ]

        DataGenerator.__init__(self, options)

    def initialize(self):
        DataGenerator.initialize(self)

        # Map Spell effects to Spell IDs so we can do filtering based on them
        for spell_effect_id, spell_effect_data in self._spelleffect_db.iteritems():
            if not spell_effect_data.id_spell:
                continue

            spell = self._spell_db[spell_effect_data.id_spell]
            if not spell.id:
                continue

            spell.add_effect(spell_effect_data)
        
        # Map JournalEncounterItem.dbc data to items.
        for id, journal_item_data in self._journalencounteritem_db.iteritems():
            if self._item_sparse_db[journal_item_data.id_item]:
                self._item_sparse_db[journal_item_data.id_item].journal = journal_item_data

        # For WoD, map ItemEffect to Item-sparse
        for is_id,data in self._itemeffect_db.iteritems():
            item = self._item_sparse_db[data.id_item]
            if not item.id:
                continue
            
            item.spells.append(data)

        return True
    
    def filter(self):
        ids = []
        for item_id, data in self._item_sparse_db.iteritems():
            blacklist_item = False
            
            classdata = self._item_db[item_id]
            if item_id in self._item_blacklist:
                continue
                
            for pat in self._item_name_blacklist:
                if data.name and re.search(pat, data.name):
                    blacklist_item = True
            
            if blacklist_item:
                continue

            filter_ilevel = True

            # Item no longer in game
            if data.flags & 0x10:
                continue

            # Gems
            if classdata.classs == 3:
                if data.gem_props == 0:
                    continue
                else:
                    filter_ilevel = False
            # Consumables
            elif classdata.classs == 0:
                # Potions, Elixirs, Flasks. Simple spells only.
                if classdata.has_value('subclass', [1, 2, 3]):
                    for item_effect in data.spells:
                        spell = self._spell_db[item_effect.id_spell]
                        if not spell.has_effect('type', 6):
                            continue

                        # Grants armor, stats, or rating
                        if not spell.has_effect('sub_type', [13, 22, 29, 99, 189, 465]):
                            continue

                        filter_ilevel = False
                # Food
                elif classdata.has_value('subclass', 5):
                    for item_effect in data.spells:
                        spell = self._spell_db[item_effect.id_spell]
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.sub_type == 23:
                                filter_ilevel = False
                elif classdata.subclass == 3:
                    filter_ilevel = False
                # Permanent Item Enchants (not strictly needed for simc, but 
                # paperdoll will like them)
                elif classdata.subclass == 6:
                   filter_ilevel = False 
                else:
                    continue
            # Hunter scopes and whatnot
            elif classdata.classs == 7:
                if classdata.has_value('subclass', 3):
                    for item_effect in data.spells:
                        spell = self._spell_db[item_effect.id_spell]
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.type == 53:
                                filter_ilevel = False
            # Only very select quest-item permanent item enchantments
            elif classdata.classs == 12:
                valid = False
                for spell in data.spells:
                    spell_id = spell.id_spell
                    if spell_id == 0:
                        continue

                    spell = self._spell_db[spell_id]
                    for effect in spell._effects:
                        if not effect or effect.type != 53:
                            continue
                        
                        valid = True
                        break

                if valid:
                    filter_ilevel = False
                else:
                    continue
            # All glyphs
            elif classdata.classs == 16:
                filter_ilevel = False
            # All tabards
            elif data.inv_type == 19:
                filter_ilevel = False
            # All shirts
            elif data.inv_type == 4:
                filter_ilevel = False

            # Item-level based non-equippable items
            if filter_ilevel and data.inv_type == 0:
                continue
            # All else is filtered based on item level
            elif filter_ilevel and (data.ilevel < self._options.min_ilevel or data.ilevel > self._options.max_ilevel):
                continue
            
            ids.append(item_id)
        
        return ids
    
    def generate(self, ids = None):
        ids.sort()
        
        s = '#include "data_definitions.hh"\n\n'
        s += '#define %sITEM%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        )
        s += '// %d items, ilevel %d-%d, wow build level %d\n' % ( len(ids), 
            self._options.min_ilevel, self._options.max_ilevel, self._options.build )
        s += 'static struct item_data_t __%sitem%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''
        )

        index = 0
        for id in ids + [ 0 ]:
            item = self._item_sparse_db[id]
            item2 = self._item_db[id]
            
            if not item.id and id > 0:
                sys.stderr.write('Item id %d not found\n' % id) 
                continue

            # Aand, hack classs 12 (quest item) to be 0, 6 
            # so we get item enchants clumped in the same category, sigh ..
            if item2.classs == 12:
                item2.classs = 0
                item2.subclass = 6

            if(index % 20 == 0):
                s += '//{    Id, Name                                                   ,     Flags1,     Flags2, Type,Level,ReqL,ReqSk, RSkL,Qua,Inv,Cla,SCl,Bnd, Delay, DmgRange, Modifier,  ClassMask,   RaceMask, { ST1, ST2, ST3, ST4, ST5, ST6, ST7, ST8, ST9, ST10}, {  SV1,  SV2,  SV3,  SV4,  SV5,  SV6,  SV7,  SV8,  SV9, SV10 }, {  SId1,  SId2,  SId3,  SId4,  SId5 }, {Soc1,Soc2,Soc3 }, GemP,IdSBon,IdSet,IdSuf },\n'

            fields = item.field('id', 'name')
            fields += item.field('flags', 'flags_2')

            flag_types = 0x00

            if hasattr(item, 'journal'):
                if item.journal.flags_1 == 0x10:
                    flag_types |= self._type_flags['Raid Finder']
                elif item.journal.flags_1 == 0xC:
                    flag_types |= self._type_flags['Heroic']

            desc = self._itemnamedescription_db[item.id_name_desc]
            flag_types |= self._type_flags.get(desc.desc, 0)

            fields += [ '%#.2x' % flag_types ]
            fields += item.field('ilevel', 'req_level', 'req_skill', 'req_skill_rank', 'quality', 'inv_type')
            fields += item2.field('classs', 'subclass')
            fields += item.field( 'bonding', 'delay', 'weapon_damage_range', 'item_damage_modifier', 'race_mask', 'class_mask') 
            fields += [ '{ %s }' % ', '.join(item.field('stat_type_1', 'stat_type_2', 'stat_type_3', 'stat_type_4', 'stat_type_5', 'stat_type_6', 'stat_type_7', 'stat_type_8', 'stat_type_9', 'stat_type_10')) ]
            fields += [ '{ %s }' % ', '.join(item.field('stat_val_1', 'stat_val_2', 'stat_val_3', 'stat_val_4', 'stat_val_5', 'stat_val_6', 'stat_val_7', 'stat_val_8', 'stat_val_9', 'stat_val_10')) ]
            fields += [ '{ %s }' % ', '.join(item.field('stat_alloc_1', 'stat_alloc_2', 'stat_alloc_3', 'stat_alloc_4', 'stat_alloc_5', 'stat_alloc_6', 'stat_alloc_7', 'stat_alloc_8', 'stat_alloc_9', 'stat_alloc_10')) ]
            fields += [ '{ %s }' % ', '.join(item.field('stat_socket_mul_1', 'stat_socket_mul_2', 'stat_socket_mul_3', 'stat_socket_mul_4', 'stat_socket_mul_5', 'stat_socket_mul_6', 'stat_socket_mul_7', 'stat_socket_mul_8', 'stat_socket_mul_9', 'stat_socket_mul_10')) ]

            spells = self._itemeffect_db[0].field('id_spell') * 5
            trigger_types = self._itemeffect_db[0].field('trigger_type') * 5
            cooldown_category = self._itemeffect_db[0].field('cooldown_category') * 5
            cooldown_value = self._itemeffect_db[0].field('cooldown_category_duration') * 5
            cooldown_group = self._itemeffect_db[0].field('cooldown_group') * 5
            cooldown_shared = self._itemeffect_db[0].field('cooldown_group_duration') * 5
            for spell in item.spells:
                spells[ spell.index ] = spell.field('id_spell')[ 0 ]
                trigger_types[ spell.index ] = spell.field('trigger_type')[ 0 ]
                cooldown_category[ spell.index ] = spell.field('cooldown_category')[ 0 ]
                cooldown_value[ spell.index ] = spell.field('cooldown_category_duration')[ 0 ]
                cooldown_group[ spell.index ] = spell.field('cooldown_group')[ 0 ]
                cooldown_shared[ spell.index ] = spell.field('cooldown_group_duration')[ 0 ]

            fields += [ '{ %s }' % ', '.join(trigger_types) ]
            fields += [ '{ %s }' % ', '.join(spells) ]
            fields += [ '{ %s }' % ', '.join(cooldown_category) ]
            fields += [ '{ %s }' % ', '.join(cooldown_value) ]
            fields += [ '{ %s }' % ', '.join(cooldown_group) ]
            fields += [ '{ %s }' % ', '.join(cooldown_shared) ]

            fields += [ '{ %s }' % ', '.join(item.field('socket_color_1', 'socket_color_2', 'socket_color_3')) ]
            fields += item.field('gem_props', 'socket_bonus', 'item_set', 'rand_suffix' )

            s += '  { %s },\n' % (', '.join(fields))
            
            index += 1
        s += '};\n\n'

        return s

class RandomPropertyHTMLParser(HTMLParser.HTMLParser):
    def __init__(self, suffix_map):
        #self.__indent = 0
        self.__getSuffix = False
        self.__suffixName = None
        self.__suffix_map = suffix_map
        HTMLParser.HTMLParser.__init__(self)

    # Really stupid way to parse things, but it should work
    def handle_starttag(self, tag, attrs):
        #print '%s%s: %s' % ( ' ' * self.__indent, tag, attrs )
        #self.__indent += 2
        if tag == 'td' and not self.__suffixName:
            for attr in attrs:
                if attr[0] == 'class' and 'color-q' in attr[1]:
                    self.__getSuffix = True
                    break
                

    def handle_data(self, data):
        if self.__getSuffix:
            self.__suffixName = data.translate(None, '.')
            self.__suffix_map[self.__suffixName] = None
        elif self.__suffixName:
            self.__suffix_map[self.__suffixName] = data.translate(None, '\r\n\t')

    def handle_endtag(self, tag):
        #self.__indent -= 2
        #print '%s%s' % ( ' ' * self.__indent, tag )
        if tag == 'td' and self.__getSuffix:
            self.__getSuffix = False
        elif tag == 'td' and self.__suffixName:
            self.__suffixName = None

class RandomSuffixGroupGenerator(ItemDataGenerator):
    _stat_map = {
        'agility'           : 3,
        'strength'          : 4,
        'intellect'         : 5,
        'spirit'            : 6,
        'stamina'           : 7,
        'dodge rating'      : 13,
        'parry rating'      : 14,
        'hit rating'        : 31,
        'crit rating'       : 32,
        'haste rating'      : 36,
        'expertise rating'  : 37,
        'holy resistance'   : 53,
        'mastery rating'    : 49,
        'frost resistance'  : 52,
        'shadow resistance' : 54,
        'nature resistance' : 55,
        'arcane resistance' : 56,
    }
    
    _quality_str = [ '', '', 'uncm', 'rare', 'epic' ]

    def random_suffix_type(self, id):
        item = self._item_sparse_db[id]
        item2 = self._item_db[id]
      
        f = -1

        if item2.classs == 2:
            if item2.subclass == 1 or item2.subclass == 5  or item2.subclass == 6 or item2.subclass == 8 or item2.subclass == 10:
                f = 0
            elif item2.subclass == 2  or item2.subclass == 3  or item2.subclass == 18 or item2.subclass == 16 or item2.subclass == 19:
                f = 4
            else:
                f = 3
        else:
            if item.inv_type == 1 or item.inv_type == 5 or item.inv_type == 7:
                f = 0
            elif item.inv_type == 3 or item.inv_type == 6 or item.inv_type == 8 or item.inv_type == 10 or item.inv_type == 12:
                f = 1
            elif item.inv_type == 2 or item.inv_type == 9 or item.inv_type == 11 or item.inv_type == 14 or item.inv_type == 23 or item.inv_type == 16:
                f = 2

        return f

    def __init__(self, options):
        ItemDataGenerator.__init__(self, options)
        
    def initialize(self):
        self._dbc += [ 'SpellItemEnchantment', 'ItemRandomSuffix', 'RandPropPoints' ]

        return ItemDataGenerator.initialize(self)
        
    def filter(self):
        item_ids = ItemDataGenerator.filter(self)

        ids = []
        # Generate an ID list of random suffix ids, to which we need to figure 
        # out the random suffix grouping, based on web crawling of battle.net
        for id in item_ids:
            if self._item_sparse_db[id].rand_suffix > 0:
                ids.append(id)
                
        return ids
        
    def generate(self, ids = None):
        rsuffix_groups = { }
        parsed_rsuffix_groups = { }
        for id in ids:
            item = self._item_sparse_db[id]
            if item.rand_suffix not in rsuffix_groups.keys():
                rsuffix_groups[item.rand_suffix] = [ ]
                
            rsuffix_groups[item.rand_suffix].append(id)
            
        for rsuffix_group, rsuffix_items in rsuffix_groups.iteritems():
            # Take first item of the group, we could revert to more items here
            # if the url returns 404 or such
            item = self._item_sparse_db[rsuffix_items[0]]
            smap = { }
            sys.stderr.write('.. Fetching group %d with item id %d (%s)\n' % (rsuffix_group, item.id, item.name))
            try:
                url = urllib2.urlopen(r'http://us.battle.net/wow/en/item/%d/randomProperties' % item.id)
            except urllib2.HTTPError as err:
                sys.stderr.write('.. HTTP Error %d: %s\n' % (err.code, err.msg))
                continue

            html = RandomPropertyHTMLParser(smap)
            html.feed(url.read())
            html.close()
    
            for suffix, stats in smap.iteritems():
                html_stats = [ ]
                splits = stats.split(',')
                # Parse html stats
                for stat_str in splits:
                    stat_re = re.match(r'^[\+\-]([0-9]+) ([a-z ]+)', stat_str.lower().strip())
                    if not stat_re:
                        continue
                    
                    stat_val = int(stat_re.group(1))
                    stat_id = self._stat_map.get(stat_re.group(2))
                    
                    if stat_id == None:
                        #sys.stderr.write('Unknown stat %s\n' % stat_str.lower().strip())
                        continue
                        
                    html_stats.append((stat_id, stat_val))
                
                for suffix_id, suffix_data in self._itemrandomsuffix_db.iteritems():
                    if suffix_data.name_sfx != suffix:
                        continue

                    # Name matches, we need to check the stats
                    rsuffix_stats = [ ]
                        
                    # Then, scan through the suffix properties, 
                    for sp_id in xrange(1, 6):
                        item_ench_id = getattr(suffix_data, 'id_property_%d' % sp_id)
                        item_ench_alloc = getattr(suffix_data, 'property_pct_%d' % sp_id)
                        if item_ench_id == 0:
                            continue
                            
                        rprop = self._randproppoints_db[item.ilevel]
                        f = self.random_suffix_type(item.id)
                            
                        points = getattr(rprop, '%s_points_%d' % (self._quality_str[item.quality], f + 1))
                        amount = points * item_ench_alloc / 10000.0

                        item_ench = self._spellitemenchantment_db[item_ench_id]
                        for ie_id in xrange(1, 4):
                            ie_stat_type = getattr(item_ench, 'type_%d' % ie_id)
                            ie_stat_prop = getattr(item_ench, 'id_property_%d' % ie_id)
                            if ie_stat_type != 5:
                                continue
                            
                            rsuffix_stats.append((ie_stat_prop, int(amount)))
                    
                    # Compare lists, need at least as many matches as html stats
                    match = 0
                    for i in xrange(0, len(html_stats)):
                        if html_stats[i] in rsuffix_stats:
                            match += 1
                        
                    if match == len(html_stats):
                        if not rsuffix_group in parsed_rsuffix_groups.keys():
                            parsed_rsuffix_groups[rsuffix_group] = [ ]
                        
                        parsed_rsuffix_groups[rsuffix_group].append(suffix_data.id)
                        break
        
        s = '#define %sRAND_SUFFIX_GROUP%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(parsed_rsuffix_groups.keys())
        )
        s += '// Random suffix groups\n';
        s += 'static struct random_suffix_group_t __%srand_suffix_group%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' )

        for group_id in sorted(parsed_rsuffix_groups.keys()):
            data = parsed_rsuffix_groups[group_id]
            s += '  { %4u, { %s, 0 } },\n' % (group_id, ', '.join(['%3u' % d for d in sorted(data)]))
        
        s += '  {    0, {   0 } }\n'
        s += '};\n\n'
        
        return s

class SpellDataGenerator(DataGenerator):
    _spell_ref_rx = r'\$(?:\?[A-z]|@spell(?:name|desc|icon|tooltip))?([0-9]+)(?:\[|[A-z][0-9]?|)'
    

    # Pattern based whitelist, these will always be added
    _spell_name_whitelist = [
        re.compile(r'^Item\s+-\s+(.+)\s+T([0-9]+)\s+([A-z\s]*)\s*([0-9]+)P')
    ]

    # Explicitly included list of spells per class, that cannot be 
    # found from talents, or based on a SkillLine category
    # The list contains a tuple ( spell_id, category[, activated ] ), 
    # where category is an entry in the _class_categories class-specific
    # tuples, e.g. general = 0, specialization0..3 = 1..4 and pets as a whole
    # are 5. The optional activated parameter forces the spell to appear (if set 
    # to True) or disappear (if set to False) from the class activated spell list, 
    # regardless of the automated activated check.
    # Manually entered general spells ("tree" 0) do not appear in class activated lists, even if
    # they pass the "activated" check, but will appear there if the optional activated parameter 
    # exists, and is set to True.
    # The first tuple in the list is for non-class related, generic spells that are whitelisted,
    # without a category
    _spell_id_list = [
        (
         109871, 109869,            # No'Kaled the Elements of Death - LFR
         107785, 107789,            # No'Kaled the Elements of Death - Normal
         109872, 109870,            # No'Kaled the Elements of Death - Heroic
         52586,  68043,  68044,     # Gurthalak, Voice of the Deeps - LFR, N, H
         109959, 109955, 109939,    # Rogue Legendary buffs for P1, 2, 3
         84745,  84746,             # Shallow Insight, Moderate Insight
         138537,                    # Death Knight Tier15 2PC melee pet special attack
         137597,                    # Legendary meta gem Lightning Strike
         137323, 137288, 137247,    # Healer legendary meta
         137331, 137326, 
         146137,                    # Cleave
         146071,                    # Multistrike
         120032, 142530,            # Dancing Steel
         104993, 142535,            # Jade Spirit
         116631,                    # Colossus
         105617,                    # Alchemist's Flask
         137596,                    # Capacitance
         104510, 104423,            # Windsong Mastery / Haste buffs
         156060, 156055, 173287,    # Hunter Scopes
         177172, 177175, 177176,    # WoD Legendary ring, phase 1(?)
         177161, 177159, 177160,    # WoD Legendary ring, phase 1(?)
        ),
        
        # Warrior:
        (
            ( 118340, 0 ),          # Impending Victory Heal
            ( 58385,  0 ),          # Glyph of Hamstring
            ( 118779, 0, False ),   # Victory Rush heal is not directly activatable
            ( 144500, 0 ),          # T16 tank 4pc proc
            ( 156321, 0 ),          # Shield Charge for Gladiator Stance
            ( 163558, 0 ),          # Execute off-hand
            ( 165336, 0 ),          # Arms T17 2 piece
            ( 165345, 0 ),          # Arms T17 4 piece
            ( 165337, 0 ),          # Fury T17 2 piece
            ( 165349, 0 ),          # Fury T17 4 piece
            ( 165338, 0 ),          # Prot T17 2 piece
            ( 165351, 0 ),          # Prot T17 4 piece
            
        ),
        
        # Paladin:
        (
            ( 86700, 5 ),           # Ancient Power
            ( 144581, 0 ),          # Blessing of the Guardians (prot T16 2-piece bonus)
            ( 144595, 0 ),          # Divine Crusader (ret T16 4-piece bonus)
            ( 122287, 0, True ),    # Symbiosis Wrath
            ( 96172, 0 ),           # Hand of Light damage spell
            ( 42463, 0, False ),    # Seal of Truth damage id not directly activatable
            ( 114852, 0, False ),   # Holy Prism false positives for activated
            ( 114871, 0, False ),
            ( 65148, 0, False ),    # Sacred Shield absorb tick
            ( 136494, 0, False ),   # World of Glory false positive for activated
            ( 113075, 0, False ),   # Barkskin (from Symbiosis)
            ( 144569, 0, False ),   # Bastion of Power (prot T16 4-piece bonus)
        ),  
        
        # Hunter:
        ( ( 83381, 0 ), # Kill Command base dmg
          ( 82928, 0 ), # Aimed Shot Master Marksman
          ( 131900, 0 ), # Murder of Crows damage spell
          ( 138374, 5 ), # T15 2pc nuke
          ( 168811, 0 ), # Sniper Training
          ( 171454, 0 ), # Chimaera Shot - Frost
          ( 171457, 0 ), # Chimaera Shot - Nature
        ),
          
        # Rogue:
        (
            ( 121474, 0 ),          # Shadow Blades off hand
            ( 57841, 0 ),           # Killing Spree assault
            ( 57842, 0 ),           # Killing Spree Off-Hand assault
            ( 22482, 0 ),           # Blade Flurry damage spell
            ( 113780, 0, False ),   # Deadly Poison damage is not directly activatable
            ( 89775, 0, False ),    # Hemorrhage damage is not directy activatable
            ( 86392, 0, False ),    # Main Gauche false positive for activatable
            ( 145211, 0 ),          # Subtlety Tier16 4PC proc
            ( 168908, 0 ),          # Sinister Calling: Hemorrhage
            ( 168952, 0 ),          # Sinister Calling: Crimson Tempest
            ( 168971, 0 ),          # Sinister Calling: Garrote
            ( 168963, 0 ),          # Sinister Calling: Rupture
            ( 115189, 0 ),          # Anticipation buff
        ),  
        
        # Priest:
        (   (  63619, 5 ), # Shadowfiend "Shadowcrawl"
            (  94472, 0 ), # Atonement Crit
            (  65081, 0 ), # Body and Soul
            ( 114908, 0, False ), # Spirit Shell absorb
            ( 124464, 0, False ), ( 124465, 0, False ), ( 124467, 0, False ), ( 124468, 0, False ), ( 124469, 0, False ), # Shadow Mastery "duplicate" ticks
            ( 127627, 3 ), ( 127628, 3 ), # Shadow Cascade
            ( 127626, 0, False ), # Devouring Plague heal (deactive)
            ( 129197, 3 ), # Mind Flay (Insanity)
            ( 165628, 0 ), # Item - Priest T17 Shadow 2P Bonus
            ( 165623, 0 ), # Item - Priest T17 Shadow 2P Bonus - dot spell
            
            ( 165621, 0 ), # Item - Priest T17 Holy 2P Bonus
            ( 165614, 0 ), # Item - Priest T17 Discipline 2P Bonus
            ( 167694, 0 ), # Item - Priest T17 Discipline 4P Bonus
            ( 167684, 0 ), # Item - Priest T17 Holy 4P Bonus
            ( 165629, 0 ), # Item - Priest T17 Shadow 4P Bonus
        ), 
        
        # Death Knight:
        ( ( 70890, 0 ), # weird Scourge Strike secondary effect
          ( 51963, 5 ), # gargoyle strike
          ( 66198, 0 ), # Obliterate off-hand
          ( 66196, 0 ), # Frost Strike off-hand
          ( 66216, 0 ), # Plague Strike off-hand
          ( 66188, 0 ), # Death Strike off-hand
          ( 113516, 0, True ), # Symbiosis Wild Mushroom: Plague
          ( 52212, 0, False ), # Death and Decay false positive for activatable
          ( 81277, 5 ), ( 81280, 5 ), ( 50453, 5 ), # Bloodworms heal / burst
          ( 45470, 0 ), # Death Strike heal
          ( 77535, 0 ), # Blood Shield
          ( 116783, 0 ), # Death Siphon heal
          ( 96171, 0 ), # Will of the Necropolish Rune Tap buff
          ( 144948, 0 ), # T16 tank 4PC Bone Shield charge proc
          ( 144953, 0 ), # T16 tank 2PC Death Strike proc
          ( 144909, 0 ), # T16 dps 4PC frost driver spell
          ( 57330, 0, True ), # Horn of Winter needs to be explicitly put in the general tree, as our (over)zealous filtering thinks it's not an active ability
          ( 47568, 0, True ), # Same goes for Empower Rune Weapon
        ),  
        
        # Shaman:
        ( (  77451, 0 ), (  45284, 0 ), (  45297, 0 ),  #  Overloads
          ( 114093, 0 ),                                # Ascendance: Stormblast, offhand melee swing,
          ( 114074, 0 ), ( 114738, 0 ),                 # Ascendance: Lava Beam, Lava Beam overload
          ( 120687, 0 ), ( 120588, 0 ),                 # Stormlash, Elemental Blast overload
          ( 121617, 0 ),                                # Ancestral Swiftness 5% haste passive
          ( 25504, 0, False ), ( 33750, 0, False ),     # Windfury passives are not directly activatable
          ( 8034, 0, False ),                           # Frostbrand false positive for activatable
          ( 145002, 0, False ),                         # Lightning Elemental nuke
          ( 157348, 5 ), ( 157331, 5 ),                 # Storm elemental spells
          ( 159101, 0 ), ( 159105, 0 ), ( 159103, 0 ),  # Echo of the Elements spec buffs
          ( 157766, 0 ),                                # Improved Chain Lightning visible buff
          ( 173184, 0 ), ( 173185, 0 ), ( 173186, 0 ),  # Elemental Blast buffs
          ( 173183, 0 ),                                # Elemental Blast buffs
          ( 170512, 0 ), ( 170523, 0 )                  # Feral Spirit windfury (t17 enhance 4pc set bonus)
        ),
        
        # Mage:
        ( 
          ( 48107, 0, False ), ( 48108, 0, False ), # Heating Up and Pyroblast! buffs
          ( 79058, 0 ), # Mage: Mana Gem, Frostfire Orb x2, Arcane Brilliance
          ( 88084, 5 ), ( 59637, 5 ), ( 88082, 5 ), ( 59638, 5 ), # Mirror Image spells.
          ( 80354, 0 ),                                           # Temporal Displacement
          ( 131079, 0 ), ( 131080, 0 ), ( 131081, 0 ),		      # Mini-bolt spells
          ( 131581, 0 ),                                          # Waterbolt
          ( 7268, 0, False ),                                     # Arcane missiles trigger
          ( 115757, 0, False ),                                   # Frost Nova false positive for activatable
          ( 145264, 0 ),                                          # T16 Frigid Blast
          ( 148022, 0 ),                                          # Icicle
          ( 155152, 5 ),                                          # Prismatic Crystal nuke
          ( 157978, 0 ), ( 157979, 0 ), ( 157977, 0 ),            # Unstable magic aoe
        ), 
        
        # Warlock:
        (   (  85692, 5 ),       # doomguard doom bolt
            ( 115746, 5 ),       # fel imp felbolt
            ( 115778, 5 ),       # observer tongue lash
            ( 115748, 5 ),       # shivarra bladedance
            ( 115770, 5 ),       # shivarra fellash
            ( 115625, 5 ),       # wrathguard mortal cleave
            ( 115831, 5 ),       # wrathguard wrathstorm
            ( 112092, 0 ),       # glyphed shadow bolt
            ( 112866, 0, True ), # fel imp summon
            ( 112867, 0, True ), # voidlord summon
            ( 112868, 0, True ), # shivarra summon
            ( 112869, 0, True ), # observer summon
            ( 112870, 2, True ), # wrathguard summon
            ( 112921, 0, True ), # abyssal summon
            ( 112927, 0, True ), # terrorguard summon
            ( 115422, 2, True ), # void ray
            ( 104025, 2, True ), # immolation aura
            ( 104027, 2, True ), # meta soul fire
            ( 124916, 2, True ), # meta chaos wave
            ( 103964, 2, True ), # meta touch of chaos
            ( 104232, 3, True ), # destruction rain of fire
            ( 114790, 1 ), ( 87385, 1 ), # soulburn seed of corruption
            ( 131737, 0, False ), ( 131740, 0, False ), ( 132566, 0, False ), ( 131736, 0, False ), # Duplicated Warlock dots
            ( 111859, 0, True ), ( 111895, 0, True ), ( 111897, 0, True ), ( 111896, 0, True ), ( 111898, 2, True ), # Grimoire of Service summons
            ( 103988, 0, False ), # Demo melee
            ( 145075, 0 ),        # T16 2pc destro
            ( 145085, 0 ),        # T16 2pc demo
            ( 145159, 0 ),        # T16 4pc affli soul shard gain
            ( 145164, 0 ),        # T16 4pc destro ember fire gain
            ( 114654, 0 ),        # Fire and Brimstone nukes
            ( 108686, 0 ),
            ( 108685, 0 ),
            ( 109468, 0 ),
            ( 104225, 0 ),
            ( 89653, 0 ),         # Drain Life heal
            ( 157698, 1 ),        # Soulburn: Haunt buff
        ),
        
        # Monk:
        ( ( 118864, 3 ), # Combo Breaker: Tiger Palm
          ( 116768, 3 ), # Combo Breaker: Blackout Kick
          ( 159407, 3 ), # Combo Breaker: Chi Explosion
          ( 108557, 1, False ), # Brewmaster Jab
          ( 126890, 0, False ), # Eminence false positives for activatable abilities
          ( 117895, 0, False ),
        ),
        
        # Druid:
        ( (  93402, 1, True ), # Sunfire
          ( 106996, 1, True ), # Astral Storm
          ( 112071, 1, True ), # Celestial Alignment
          ( 110621, 0, True ), # Symbiosis spells
          ( 122114, 1, True ), # Chosen of Elune
          ( 122283, 0, True ),
          ( 110807, 0, True ),
          ( 112997, 0, True ),
          ( 113769, 5 ),       # Wrath for Treants
          ( 110691, 5 ),       # Wrath for Mirror Images
          ( 108291, 0 ), ( 108292, 0 ), ( 108293, 0 ), ( 108294, 0 ), # Heart of the Wild
          ( 144770, 1, False ), ( 144772, 1, False ), # Balance Tier 16 2pc spells
          ( 150017, 5 ),       # Rake for Treants
          ( 146874, 0 ),       # Feral Rage (T16 4pc feral bonus)
          ( 124991, 0 ), ( 124988, 0 ), # Nature's Vigil
	  ( 155580, 0 ),       # Lunar Inspiration
	  ( 155627, 0 ),       # Lunar Inspiration
	  ( 155625, 0 ),       # Lunar Inspiration Moonfire
	  ( 145152, 0 ),       # Bloodtalons buff
	  ( 135597, 0 ),       # Tooth and Claw absorb buff
	  ( 155784, 0 ),       # Primal Tenacity buff
	  ( 165431, 0 ),       # tier17_2pc_melee
	  ( 165432, 0 ),       # tier17_4pc_melee
	  ( 166638, 0 ),       # tier17_4pc_melee debuff
	  ( 166639, 0 ),       # tier17_4pc_melee proc driver
	  ( 177969, 0 ),       # tier17_4pc_tank buff
        ), 
    ]

    # Class specific item sets, T13, T14, T15
    _item_set_list = [
        (),
        # Tier13,                Tier14,                      Tier15                      Tier16
        ( ( 1073, 1074, ),       ( 1144, 1145, ),             ( 1172, 1173 ),             ( 1179, 1180 ),             ), # Warrior
        ( ( 1063, 1065, 1064, ), ( 1134, 1135, 1136, ),       ( 1162, 1163, 1164 ),       ( 1188, 1189, 1190 ),       ), # Paladin
        ( ( 1061, ),             ( 1129, ),                   ( 1157, ),                  ( 1195, ),                  ), # Hunter
        ( ( 1068, ),             ( 1139, ),                   ( 1167, ),                  ( 1185, ),                  ), # Rogue
        ( ( 1066, 1067, ),       ( 1137, 1138, ),             ( 1165, 1166 ),             ( 1186, 1187 ),             ), # Priest
        ( ( 1056, 1057, ),       ( 1123, 1124, ),             ( 1151, 1152 ),             ( 1200, 1201 ),             ), # Death Knight
        ( ( 1070, 1071, 1069, ), ( 1140, 1141, 1142, ),       ( 1168, 1169, 1170 ),       ( 1182, 1183, 1184 ),       ), # Shaman
        ( ( 1062, ),             ( 1130, ),                   ( 1158, ),                  ( 1194, )                   ), # Mage
        ( ( 1072, ),             ( 1143, ),                   ( 1171, ),                  ( 1181, )                   ), # Warlock
        ( ( ),                   ( 1131, 1132, 1133, ),       ( 1159, 1160, 1161 ),       ( 1191, 1192, 1193 ),       ), # Monk
        ( ( 1059, 1058, 1060 ),  ( 1125, 1126, 1127, 1128, ), ( 1153, 1154, 1155, 1156 ), ( 1196, 1197, 1198, 1199 ), ), # Druid
    ]
  
    _profession_enchant_categories = [
        165,  # Leatherworking
        171,  # Alchemy
        197,  # Tailoring
        202,  # Engineering
        333,  # Enchanting
        773,  # Inscription
    ]
    
    # General
    _skill_categories = [
          0,
        840,    # Warrior
        800,    # Paladin
        795,    # Hunter
        921,    # Rogue
        804,    # Priest
        796,    # Death Knight
        924,    # Shaman
        904,    # Mage
        849,    # Warlock
        829,    # Monk
        798,    # Druid
    ]
    
    _pet_skill_categories = [
        ( ),
        ( ),         # Warrior
        ( ),         # Paladin
        ( 203, 208, 209, 210, 211, 212, 213, 214, 215, 217, 218, 236, 251, 270, 653, 654, 655, 656, 763, 764, 765, 766, 767, 768, 775, 780, 781, 783, 784, 785, 786, 787, 788, 808, 811 ),       # Hunter
        ( ),         # Rogue
        ( ),         # Priest
        ( 782, ),    # Death Knight
        ( 962, 963 ),         # Shaman
        ( 805, ),    # Mage
        ( 188, 189, 204, 205, 206, 207, 761 ),  # Warlock
        ( ),         # Monk
        ( ),         # Druid
    ]
    
    # Specialization categories, Spec0 | Spec1 | Spec2 
    # Note, these are reset for MoP
    _spec_skill_categories = [
        (),
        (  71,  72,  73,   0 ), # Warrior
        (  65,  66,  70,   0 ), # Paladin
        ( 254, 255, 256,   0 ), # Hunter
        ( 259, 260, 261,   0 ), # Rogue
        ( 256, 257, 258,   0 ), # Priest
        ( 250, 251, 252,   0 ), # Death Knight
        ( 262, 263, 264,   0 ), # Shaman
        (  62,  63,  64,   0 ), # Mage
        ( 265, 266, 267,   0 ), # Warlock
        ( 268, 270, 269,   0 ), # Monk
        ( 102, 103, 104, 105 ), # Druid
    ]
    
    _race_categories = [
        (),
        ( 754, ),                # Human     0x0001
        ( 125, ),                # Orc       0x0002   
        ( 101, ),                # Dwarf     0x0004
        ( 126, ),                # Night-elf 0x0008
        ( 220, ),                # Undead    0x0010
        ( 124, ),                # Tauren    0x0020
        ( 753, ),                # Gnome     0x0040
        ( 733, ),                # Troll     0x0080
        ( 790, ),                # Goblin    0x0100? not defined yet
        ( 756, ),                # Blood elf 0x0200
        ( 760, ),                # Draenei   0x0400
        (),                      # Fel Orc
        (),                      # Naga
        (),                      # Broken
        (),                      # Skeleton
        (),                      # Vrykul
        (),                      # Tuskarr
        (),                      # Forest Troll
        (),                      # Taunka
        (),                      # Northrend Skeleton
        (),                      # Ice Troll
        ( 789, ),                # Worgen   0x200000
        (),                      # Gilnean
    ]
    
    _skill_category_blacklist = [
        148,                # Horse Riding
        762,                # Riding
        183,                # Generic (DND)
    ]

    # Any spell with this effect type, will be automatically 
    # blacklisted
    # http://github.com/mangos/mangos/blob/400/src/game/SharedDefines.h
    _effect_type_blacklist = [
        5,      # SPELL_EFFECT_TELEPORT_UNITS
        #10,     # SPELL_EFFECT_HEAL
        18,     # SPELL_EFFECT_RESURRECT
        25,     # SPELL_EFFECT_WEAPONS
        39,     # SPELL_EFFECT_LANGUAGE
        47,     # SPELL_EFFECT_TRADESKILL
        50,     # SPELL_EFFECT_TRANS_DOOR
        60,     # SPELL_EFFECT_PROFICIENCY
        71,     # SPELL_EFFECT_PICKPOCKET
        94,     # SPELL_EFFECT_SELF_RESURRECT
        97,     # SPELL_EFFECT_SUMMON_ALL_TOTEMS
        109,    # SPELL_EFFECT_SUMMON_DEAD_PET 
        110,    # SPELL_EFFECT_DESTROY_ALL_TOTEMS
        118,    # SPELL_EFFECT_SKILL
        126,    # SPELL_STEAL_BENEFICIAL_BUFF
    ]
    
    # http://github.com/mangos/mangos/blob/400/src/game/SpellAuraDefines.h 
    _aura_type_blacklist = [
        1,      # SPELL_AURA_BIND_SIGHT
        2,      # SPELL_AURA_MOD_POSSESS  
        5,      # SPELL_AURA_MOD_CONFUSE
        6,      # SPELL_AURA_MOD_CHARM 
        7,      # SPELL_AURA_MOD_FEAR
        #8,      # SPELL_AURA_PERIODIC_HEAL
        17,     # SPELL_AURA_MOD_STEALTH_DETECT
        25,     # SPELL_AURA_MOD_PACIFY
        30,     # SPELL_AURA_MOD_SKILL (various skills?)
        #31,     # SPELL_AURA_MOD_INCREASE_SPEED
        44,     # SPELL_AURA_TRACK_CREATURES
        45,     # SPELL_AURA_TRACK_RESOURCES
        56,     # SPELL_AURA_TRANSFORM
        58,     # SPELL_AURA_MOD_INCREASE_SWIM_SPEED 
        75,     # SPELL_AURA_MOD_LANGUAGE
        78,     # SPELL_AURA_MOUNTED
        82,     # SPELL_AURA_WATER_BREATHING
        91,     # SPELL_AURA_MOD_DETECT_RANGE
        98,     # SPELL_AURA_MOD_SKILL (trade skills?)
        104,    # SPELL_AURA_WATER_WALK,
        105,    # SPELL_AURA_FEATHER_FALL
        151,    # SPELL_AURA_TRACK_STEALTHED
        154,    # SPELL_AURA_MOD_STEALTH_LEVEL 
        156,    # SPELL_AURA_MOD_REPUTATION_GAIN 
        206,    # SPELL_AURA_MOD_FLIGHT_SPEED_xx begin
        207, 
        208,
        209, 
        210,
        211,
        212     # SPELL_AURA_MOD_FLIGHT_SPEED_xx ends
    ]

    _mechanic_blacklist = [
        21,     # MECHANIC_MOUNT
    ]

    _spell_blacklist = [
        3561,   # Teleports --
        3562,
        3563,
        3565,
        3566,
        3567,   # -- Teleports
        20585,  # Wisp spirit (night elf racial)
        42955,  # Conjure Refreshment
        43987,  # Ritual of Refreshment
        48018,  # Demonic Circle: Summon
        48020,  # Demonic Circle: Teleport
        69044,  # Best deals anywhere (goblin racial)
        69046,  # Pack hobgoblin (woe hogger)
        68978,  # Flayer (worgen racial)
        68996,  # Two forms (worgen racial)
    ]
    
    _spell_name_blacklist = [
        "^Languages",
        "^Teleport:",
        "^Weapon Skills",
        "^Armor Skills",
        "^Tamed Pet Passive",
    ]

    _spell_families = {
        'mage': 3,
        'warrior': 4,
        'warlock': 5,
        'priest': 6,
        'druid': 7,
        'rogue': 8,
        'hunter': 9,
        'paladin': 10,
        'shaman': 11,
        'deathknight': 15,
        'monk': 53
    }
    
    def __init__(self, options):
        DataGenerator.__init__(self, options)

        self._dbc = [
            'Spell', 'SpellEffect', 'SpellScaling', 'SpellCooldowns', 'SpellRange', 'SpellClassOptions',
            'SpellDuration', 'SpellPower', 'SpellLevels', 'SpellCategories', 'Talent', 
            'SkillLineAbility', 'SpellAuraOptions', 'SpellRuneCost', 'SpellRadius', 'GlyphProperties',
            'SpellCastTimes', 'ItemSet', 'SpellDescriptionVariables', 'SpellItemEnchantment', 'Item-sparse',
            'Item', 'SpellEquippedItems', 'SpellIcon', 'SpecializationSpells', 'ChrSpecialization', 'SpellEffectScaling',
            'SpellMisc', 'SpellProcsPerMinute', 'ItemSetSpell', 'ItemEffect', 'MinorTalent' ]

    def initialize(self):
        DataGenerator.initialize(self)

        # Map Spell effects to Spell IDs so we can do filtering based on them
        for spell_effect_id, spell_effect_data in self._spelleffect_db.iteritems():
            if not spell_effect_data.id_spell:
                continue

            spell = self._spell_db[spell_effect_data.id_spell]
            if not spell.id:
                continue

            spell.add_effect(spell_effect_data)
        
        # Map Spell effect scaling to Spell Effects
        for ses_id,ses_data in self._spelleffectscaling_db.iteritems():
            if not ses_data.id_effect:
                continue

            effect = self._spelleffect_db[ses_data.id_effect]
            if not effect.id:
                continue

            effect.scaling = ses_data

        # Map Spell powers to spell ids
        for spell_power_id, spell_power_data in self._spellpower_db.iteritems():
            if not spell_power_data.id_spell:
                continue
        
            spell = self._spell_db[spell_power_data.id_spell]
            if not spell.id:
                continue
        
            spell.add_power(spell_power_data)
        
        # For builds 15589+, map SpellMisc.dbc to spell ids
        #for spell_misc_id, spell_misc_data in self._spellmisc_db.iteritems():
        #    if not spell_misc_data.id_spell:
        #        continue
        #    
        #    spell = self._spell_db[spell_misc_data.id_spell]
        #    if not spell.id:
        #        continue
        #    
        #    spell.add_misc(spell_misc_data)

        # For WoD, map ItemSetSpell.dbc to ItemSet.dbc
        for isb_id, data in self._itemsetspell_db.iteritems():
            item_set = self._itemset_db[data.id_item_set]
            if not item_set.id:
                continue
            
            item_set.bonus.append(data)

        # For WoD, map ItemEffect to Item-sparse
        for is_id,data in self._itemeffect_db.iteritems():
            item = self._item_sparse_db[data.id_item]
            if not item.id:
                continue
            
            item.spells.append(data)
        
        return True
    
    def class_mask_by_skill(self, skill):
        for i in xrange(0, len(self._skill_categories)):
            if self._skill_categories[i] == skill:
                return DataGenerator._class_masks[i]
        
        return 0
    
    def class_mask_by_spec_skill(self, spec_skill):
        for i in xrange(0, len(self._spec_skill_categories)):
            if spec_skill in self._spec_skill_categories[i]:
                return DataGenerator._class_masks[i]
        
        return 0
    
    def class_mask_by_pet_skill(self, pet_skill):
        for i in xrange(0, len(self._pet_skill_categories)):
            if pet_skill in self._pet_skill_categories[i]:
                return DataGenerator._class_masks[i]

        return 0

    def race_mask_by_skill(self, skill):
        for i in xrange(0, len(self._race_categories)):
            if skill in self._race_categories[i]:
                return DataGenerator._race_masks[i]
        
        return 0
    
    def process_spell(self, spell_id, result_dict, mask_class = 0, mask_race = 0, state = True):
        filter_list = { }
        lst = self.generate_spell_filter_list(spell_id, mask_class, mask_race, filter_list, state)
        if not lst:
            return
            
        for k, v in lst.iteritems():
            if result_dict.get(k):
                result_dict[k]['mask_class'] |= v['mask_class']
                result_dict[k]['mask_race'] |= v['mask_race']
            else:
                result_dict[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }

    def spell_state(self, spell, enabled_effects = None):
        # Check for blacklisted spells
        if spell.id in SpellDataGenerator._spell_blacklist:
            self.debug("Spell id %u (%s) is blacklisted" % ( spell.id, spell.name ) )
            return False

        # Check for spell name blacklist
        for p in SpellDataGenerator._spell_name_blacklist:
            if spell.name and re.search(p, spell.name):
                self.debug("Spell id %u (%s) matches name blacklist pattern %s" % ( spell.id, spell.name, p ) )
                return False

        # Check for blacklisted spell category mechanism
        if spell.id_categories > 0:
            c = self._spellcategories_db[spell.id_categories]
            if c.mechanic in SpellDataGenerator._mechanic_blacklist:
                self.debug("Spell id %u (%s) matches mechanic blacklist %u" % ( spell.id, spell.name, c.mechanic ))
                return False

        # Make sure we can filter based on effects even if there's no map of relevant effects
        if enabled_effects == None:
            enabled_effects = [ True ] * ( spell.max_effect_index + 1 )
        
        # Effect blacklist processing
        for effect_index in xrange(0, len(spell._effects)):
            if not spell._effects[effect_index]:
                enabled_effects[effect_index] = False
                continue
        
            effect = spell._effects[effect_index]
        
            # Blacklist by effect type
            if effect.type in SpellDataGenerator._effect_type_blacklist:
                enabled_effects[effect.index] = False

            # Blacklist by apply aura (party, raid)
            if effect.type in [ 6, 35, 65 ] and effect.sub_type in SpellDataGenerator._aura_type_blacklist:
                enabled_effects[effect.index] = False
    
        # If we do not find a true value in enabled effects, this spell is completely
        # blacklisted, as it has no effects enabled that interest us
        if True not in enabled_effects:
            self.debug("Spell id %u (%s) has no enabled effects" % ( spell.id, spell.name ) )
            return False

        return True
    
    def generate_spell_filter_list(self, spell_id, mask_class, mask_race, filter_list = { }, state = True):
        spell = self._spell_db[spell_id]
        enabled_effects = [ True ] * ( spell.max_effect_index + 1 )

        if not spell.id:
            return None
            
        if state and not self.spell_state(spell, enabled_effects):
            return None
        
        filter_list[spell.id] = { 'mask_class': mask_class, 'mask_race': mask_race, 'effect_list': enabled_effects }

        # Add spell triggers to the filter list recursively
        for effect in spell._effects:
            if not effect or spell.id == effect.trigger_spell:
                continue
            
            # Regardless of trigger_spell or not, if the effect is not enabled,
            # we do not process it
            if not enabled_effects[effect.index]:
                continue
                
            if effect.trigger_spell > 0:
                if effect.trigger_spell in filter_list.keys():
                    continue

                lst = self.generate_spell_filter_list(effect.trigger_spell, mask_class, mask_race, filter_list)
                if not lst:
                    continue
                
                for k, v in lst.iteritems():
                    if filter_list.get(k):
                        filter_list[k]['mask_class'] |= v['mask_class']
                        filter_list[k]['mask_race'] |= v['mask_race']
                    else:
                        filter_list[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }
        
        spell_refs = re.findall(SpellDataGenerator._spell_ref_rx, spell.desc or '')
        spell_refs += re.findall(SpellDataGenerator._spell_ref_rx, spell.tt or '')
        if spell.id_desc_var and self._spelldescriptionvariables_db.get(spell.id_desc_var):
            spell_refs += re.findall(SpellDataGenerator._spell_ref_rx, self._spelldescriptionvariables_db.get(spell.id_desc_var).var)
        spell_refs = list(set(spell_refs))

        for ref_spell_id in spell_refs:
            rsid = int(ref_spell_id)
            if rsid == spell.id:
                continue
            
            if rsid in filter_list.keys():
                continue
            
            lst = self.generate_spell_filter_list(rsid, mask_class, mask_race, filter_list)
            if not lst:
                continue
            
            for k, v in lst.iteritems():
                if filter_list.get(k):
                    filter_list[k]['mask_class'] |= v['mask_class']
                    filter_list[k]['mask_race'] |= v['mask_race']
                else:
                    filter_list[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }

        return filter_list

    def filter(self):
        ids = { }

        # First, get spells from talents. Pet and character class alike
        for talent_id, talent_data in self._talent_db.iteritems():
            mask_class = 0
            
            # These may now be pet talents
            if talent_data.class_id > 0:
                mask_class = DataGenerator._class_masks[talent_data.class_id]
                self.process_spell(getattr(talent_data, 'id_spell'), ids, mask_class, 0, False)

        # Get all perks
        for perk_id, perk_data in self._minortalent_db.iteritems():
            if perk_data.id_spell == 0:
                continue

            spec_data = self._chrspecialization_db[perk_data.id_spec]
            if spec_data.id == 0:
                continue

            self.process_spell(perk_data.id_spell, ids, DataGenerator._class_masks[spec_data.class_id], 0, False)

        # Get base skills from SkillLineAbility
        for ability_id, ability_data in self._skilllineability_db.iteritems():
            mask_class_category = 0
            mask_race_category  = 0

            if ability_data.id_skill in SpellDataGenerator._skill_category_blacklist:
                continue
            
            # Guess class based on skill category identifier
            mask_class_category = self.class_mask_by_skill(ability_data.id_skill)
            if mask_class_category == 0:
                mask_class_category = self.class_mask_by_spec_skill(ability_data.id_skill)
            
            if mask_class_category == 0:
                mask_class_category = self.class_mask_by_pet_skill(ability_data.id_skill)

            # Guess race based on skill category identifier
            mask_race_category = self.race_mask_by_skill(ability_data.id_skill)

            # Make sure there's a class or a race for an ability we are using
            if not ability_data.mask_class and not ability_data.mask_race and not mask_class_category and not mask_race_category:
                continue
                
            spell = self._spell_db[ability_data.id_spell]
            if not spell.id:
                continue

            self.process_spell(spell.id, ids, ability_data.mask_class or mask_class_category, ability_data.mask_race or mask_race_category)

        # Get specialization skills from SpecializationSpells and masteries from ChrSpecializations
        for spec_id, spec_spell_data in self._specializationspells_db.iteritems():
            # Guess class based on specialization category identifier
            spec_data = self._chrspecialization_db[spec_spell_data.spec_id]
            if spec_data.id == 0:
                continue

            spell = self._spell_db[spec_spell_data.spell_id]
            if not spell.id:
                continue
            
            mask_class = 0
            if spec_data.class_id > 0:
                mask_class = DataGenerator._class_masks[spec_data.class_id]
            # Hunter pet classes have a class id of 0, tag them as "hunter spells" like before
            else:
                mask_class = DataGenerator._class_masks[3]
            
            self.process_spell(spell.id, ids, mask_class, 0, False)
            if ids.has_key(spell.id):
                ids[spell.id]['replace_spell_id'] = spec_spell_data.replace_spell_id
        
        for spec_id, spec_data in self._chrspecialization_db.iteritems():
            s = self._spell_db[spec_data.id_mastery]
            if s.id == 0:
                continue
            
            if self._spellmisc_db[s.id_misc].flags_12694 & 0x20000000:
                self.process_spell(s.id, ids, DataGenerator._class_masks[spec_data.class_id], 0, False)
        

        # Get spells relating to item enchants, so we can populate a (nice?) list
        for enchant_id, enchant_data in self._spellitemenchantment_db.iteritems():
            for i in xrange(1, 4):
                type_field_str = 'type_%d' % i
                id_field_str = 'id_property_%d' % i

                # "combat spell", "equip spell", "use spell"
                if getattr(enchant_data, type_field_str) not in [ 1, 3, 7 ]:
                    continue
                
                spell_id = getattr(enchant_data, id_field_str)
                if not spell_id:
                    continue
                
                self.process_spell(spell_id, ids, 0, 0)

        # Get spells that create item enchants
        for ability_id, ability_data in self._skilllineability_db.iteritems():
            if ability_data.id_skill not in self._profession_enchant_categories:
                continue;
            
            spell = self._spell_db[ability_data.id_spell]
            if not spell.id:
                continue

            enchant_spell_id = 0
            for effect in spell._effects:
                # Grab Enchant Items and Create Items (create item will be filtered further)
                if not effect or (effect.type != 53 and effect.type != 24):
                    continue

                # Create Item, see if the created item has a spell that enchants an item, if so
                # add the enchant spell. Also grab all gem spells
                if effect.type == 24:
                    item = self._item_sparse_db[effect.item_type]
                    if not item.id or item.gem_props == 0:
                        continue

                    for spell in item.spells:
                        id_spell = spell.id_spell
                        enchant_spell = self._spell_db[id_spell]
                        for enchant_effect in enchant_spell._effects:
                            if not enchant_effect or (enchant_effect.type != 53 and enchant_effect.type != 6):
                                continue

                            enchant_spell_id = id_spell
                            break

                        if enchant_spell_id > 0:
                            break
                elif effect.type == 53:
                    spell_item_ench = self._spellitemenchantment_db[effect.misc_value]
                    #if (spell_item_ench.req_skill == 0 and self._spelllevels_db[spell.id_levels].base_level < 60) or \
                    #   (spell_item_ench.req_skill > 0 and spell_item_ench.req_skill_value <= 375):
                    #    continue
                    enchant_spell_id = spell.id
                
                if enchant_spell_id > 0:
                    break

            # Valid enchant, process it
            if enchant_spell_id > 0:
                self.process_spell(enchant_spell_id, ids, 0, 0)

        # Rest of the Item enchants relevant to us, such as Shoulder / Head enchants
        for item_id, data in self._item_sparse_db.iteritems():
            blacklist_item = False
            
            classdata = self._item_db[item_id]
            if item_id in ItemDataGenerator._item_blacklist:
                continue
                
            for pat in ItemDataGenerator._item_name_blacklist:
                if data.name and re.search(pat, data.name):
                    blacklist_item = True
            
            if blacklist_item:
                continue

            # Consumables, Flasks, Elixirs, Potions, Food & Drink, Permanent Enchants
            if classdata.classs != 12 and \
               (classdata.classs != 7 or classdata.subclass not in [3]) and \
               (classdata.classs != 0 or classdata.subclass not in [1, 2, 3, 5, 6]):
                continue
            
            # Grab relevant spells from quest items, this in essence only 
            # includes certain permanent enchants
            if classdata.classs == 12:
                for spell in data.spells:
                    spell_id = spell.id_spell
                    if spell_id == 0:
                        continue

                    spell = self._spell_db[spell_id]
                    for effect in spell._effects:
                        if not effect or effect.type != 53:
                            continue
                        
                        self.process_spell(spell_id, ids, 0, 0)
            # Grab relevant spells from consumables as well
            elif classdata.classs == 0:
                for item_effect in data.spells:
                    spell = self._spell_db[item_effect.id_spell]
                    if not spell.id:
                        continue

                    # Potions and Elixirs need to apply attributes, rating or
                    # armor
                    if classdata.has_value('subclass', [1, 2, 3]) and spell.has_effect('sub_type', [13, 22, 29, 99, 189, 465]):
                        self.process_spell(spell.id, ids, 0, 0)
                    # Food needs to have a periodically triggering effect
                    # (presumed to be always a stat giving effect)
                    elif classdata.has_value('subclass', 5) and spell.has_effect('sub_type', 23):
                        self.process_spell(spell.id, ids, 0, 0)
                    # Permanent enchants
                    elif classdata.has_value('subclass', 6):
                        self.process_spell(spell.id, ids, 0, 0)
            # Hunter scopes and whatnot
            elif classdata.classs == 7:
                if classdata.has_value('subclass', 3):
                    for item_effect in data.spells:
                        spell = self._spell_db[item_effect.id_spell]
                        for effect in spell._effects:
                            if not effect:
                                continue

                            if effect.type == 53:
                                self.process_spell(spell.id, ids, 0, 0)
        
        # Item sets, loop through ItemSet.dbc getting class-specific tier sets and add 
        # their bonuses to the spell list
        for itemset_id, itemset_data in self._itemset_db.iteritems():
            mask_class_category = 0
            for cls in xrange(0, len(SpellDataGenerator._item_set_list)):
                for tier in SpellDataGenerator._item_set_list[cls]:
                    if itemset_id in tier:
                        mask_class_category = DataGenerator._class_masks[cls]
                        break

                if mask_class_category:
                    break

            if not mask_class_category:
                continue

            if not itemset_data.bonus:
                continue

            # Item set is a tier set, we want informations.
            for item_set_bonus in itemset_data.bonus:
                if not item_set_bonus.id_spell:
                    continue

                self.process_spell(item_set_bonus.id_spell, ids, mask_class_category, 0)

        # PVP Set bonuses, and then some
        for id, set_spell_data in self._itemsetspell_db.iteritems():
            itemset = self._itemset_db[set_spell_data.id_item_set]
            if 'Gladiator' not in itemset.name:
                continue

            # Only wod pvp spells
            if set_spell_data.unk_wod_1 == 0:
                continue

            self.process_spell(set_spell_data.id_spell, ids, 0, 0)

        # Glyph effects, need to do trickery here to get actual effect from spellbook data
        for ability_id, ability_data in self._skilllineability_db.iteritems():
            if ability_data.id_skill != 810 or not ability_data.mask_class:
                continue

            use_glyph_spell = self._spell_db[ability_data.id_spell]
            if not use_glyph_spell.id:
                continue

            # Find the on-use for glyph then, misc value will contain the correct GlyphProperties.dbc id
            for effect in use_glyph_spell._effects:
                if not effect or effect.type != 74: # Use glyph
                    continue

                # Filter some erroneous glyph data out
                glyph_data = self._glyphproperties_db[effect.misc_value]
                if not glyph_data.id or not glyph_data.id_spell:
                    continue
                
                self.process_spell(glyph_data.id_spell, ids, ability_data.mask_class, 0)

        # Item enchantments that use a spell
        for eid, data in self._spellitemenchantment_db.iteritems():
            for attr_id in xrange(1, 4):
                attr_type = getattr(data, 'type_%d' % attr_id)
                if attr_type == 1 or attr_type == 3 or attr_type == 7:
                    sid = getattr(data, 'id_property_%d' % attr_id)
                    self.process_spell(sid, ids, 0, 0)
        
        # Items with a spell identifier as "stats"
        for iid, data in self._item_sparse_db.iteritems():
            # Allow trinkets, weapons, 2hweapons to bypass ilevel checking
            if data.inv_type not in [ 12, 13, 15, 17, 21, 22, 26 ] and \
               (data.ilevel < self._options.min_ilevel or data.ilevel > self._options.max_ilevel):
                continue
            
            for spell in data.spells:
                if spell.id_spell == 0:
                    continue

                self.process_spell(spell.id_spell, ids, 0, 0)
        
        # Last, get the explicitly defined spells in _spell_id_list on a class basis and the 
        # generic spells from SpellDataGenerator._spell_id_list[0]
        for generic_spell_id in SpellDataGenerator._spell_id_list[0]:
            if generic_spell_id in ids.keys():
                sys.stderr.write('Whitelisted spell id %u (%s) already in the list of spells to be extracted.\n' % (
                    generic_spell_id, self._spell_db[generic_spell_id].name) )
            self.process_spell(generic_spell_id, ids, 0, 0)

        for cls in xrange(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls]:
                if len(spell_tuple) == 2 and spell_tuple[0] in ids.keys():
                    sys.stderr.write('Whitelisted spell id %u (%s) already in the list of spells to be extracted.\n' % (
                        spell_tuple[0], self._spell_db[spell_tuple[0]].name) )
                self.process_spell(spell_tuple[0], ids, self._class_masks[cls], 0)

        for spell_id, spell_data in self._spell_db.iteritems():
            for pattern in SpellDataGenerator._spell_name_whitelist:
                if pattern.match(spell_data.name):
                    self.process_spell(spell_id, ids, 0, 0)

        
        # After normal spells have been fetched, go through all spell ids, 
        # and get all the relevant aura_ids for selected spells
        more_ids = { }
        for spell_id, spell_data in ids.iteritems():
            spell = self._spell_db[spell_id]
            for power in spell._powers:
                if not power or power.aura_id == 0:
                    continue
                
                self.process_spell(power.aura_id, more_ids, spell_data['mask_class'], spell_data['mask_race'])
        
        for id, data in more_ids.iteritems():
            if not ids.has_key(id):
                ids[ id ] = data
            else:
                ids[id]['mask_class'] |= data['mask_class']
                ids[id]['mask_race'] |= data['mask_race']
        
        return ids

    def generate(self, ids = None):
        # Sort keys
        id_keys = ids.keys()
        id_keys.sort()
        effects = set()
        powers = set()

        s = '#include "data_definitions.hh"\n\n'
        s += '#define %sSPELL%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        )
        s += '// %d spells, wow build level %d\n' % ( len(ids), self._options.build )
        s += 'static struct spell_data_t __%sspell%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''
        )

        index = 0
        for id in id_keys + [ 0 ]:
            spell = self._spell_db[id]

            if not spell.id and id > 0:
                sys.stderr.write('Spell id %d not found\n') % id
                continue
            
            #if len(spell._misc) > 1:
            #    sys.stderr.write('Spell id %u (%s) has more than one SpellMisc.dbc entry\n' % ( spell.id, spell.name ) )
            #    continue

            for power in spell._powers:
                if power == None:
                    continue
                
                powers.add( power )

            if index % 20 == 0:
              s += '//{ Name                                ,     Id,Flags,PrjSp,  Sch, Class,  Race,Sca,MSL,SpLv,MxL,MinRange,MaxRange,Cooldown,  GCD,  Cat,  Duration,  RCost, RPG,Stac, PCh,PCr, ProcFlags,EqpCl, EqpInvType,EqpSubclass,CastMn,CastMx,Div,       Scaling,SLv, RplcId, {      Attr1,      Attr2,      Attr3,      Attr4,      Attr5,      Attr6,      Attr7,      Attr8,      Attr9,     Attr10,     Attr11,     Attr12 }, {     Flags1,     Flags2,     Flags3,     Flags4 }, Family, Description, Tooltip, Description Variable, Icon, ActiveIcon, Effect1, Effect2, Effect3 },\n'
            
            fields = spell.field('name', 'id') 
            fields += [ '%#.2x' % 0 ]
            fields += self._spellmisc_db[spell.id_misc].field('prj_speed', 'mask_school')

            # Hack in the combined class from the id_tuples dict
            fields += [ '%#.3x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_class'] ]
            fields += [ '%#.3x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_race'] ]

            # Set the scaling index for the spell
            fields += self._spellscaling_db[spell.id_scaling].field('id_class', 'max_scaling_level')

            #fields += spell.field('extra_coeff')

            fields += self._spelllevels_db[spell.id_levels].field('base_level', 'max_level')
            fields += self._spellrange_db[self._spellmisc_db[spell.id_misc].id_range].field('min_range')
            fields += self._spellrange_db[self._spellmisc_db[spell.id_misc].id_range].field('max_range')
            fields += self._spellcooldowns_db[spell.id_cooldowns].field('cooldown_duration', 'gcd_cooldown')

            fields += self._spellcategories_db[spell.id_categories].field('category')
            fields += self._spellduration_db[self._spellmisc_db[spell.id_misc].id_duration].field('duration_1')
            fields += _rune_cost(self, None, self._spellrunecost_db[spell.id_rune_cost], '%#.4x'),
            fields += self._spellrunecost_db[spell.id_rune_cost].field('rune_power_gain')
            fields += self._spellauraoptions_db[spell.id_aura_opt].field(
                'stack_amount', 'proc_chance', 'proc_charges', 'proc_flags'
            )

            if self.dbc_version(50400, 17093):
                fields += self._spellauraoptions_db[spell.id_aura_opt].field('internal_cooldown')
            else:
                fields += [ '%7u' % 0 ]

            if self._options.build >= 17227:
                aura = self._spellauraoptions_db[spell.id_aura_opt]
                fields += self._spellprocsperminute_db[aura.id_ppm].field('ppm')
            else:
                fields += [ '%5.3f' % 0 ]
            
            fields += self._spellequippeditems_db[spell.id_equip_items].field('item_class', 'mask_inv_type', 'mask_sub_class')
            
            if spell.id_scaling:
                fields += self._spellscaling_db[spell.id_scaling].field('cast_min', 'cast_max', 'cast_div')
                fields += self._spellscaling_db[spell.id_scaling].field('c_scaling', 'c_scaling_threshold')
            else:
                fields += self._spellcasttimes_db[self._spellmisc_db[spell.id_misc].id_cast_time].field('min_cast_time', 'cast_time')
                # Use default values, i.e., zeros
                fields += self._spellscaling_db[0].field('cast_div', 'c_scaling', 'c_scaling_threshold' )

            if ids.has_key(id) and ids[id].has_key('replace_spell_id'):
                fields += [ '%6u' % ids[id]['replace_spell_id'] ]
            else:
                fields += [ '%6u' % 0 ]
            
            s_effect = []
            effect_ids = []
            for effect in spell._effects:
                if effect and ids.get(id, { 'effect_list': [ False ] })['effect_list'][effect.index]:
                    effects.add( ( effect.id, spell.id_scaling ) )
                    effect_ids.append( '%u' % effect.id )

            # Add spell flags
            fields += [ '{ %s }' % ', '.join(self._spellmisc_db[spell.id_misc].field('flags', 'flags_1', 'flags_2', 'flags_3', 'flags_4', 'flags_5', 'flags_6', 'flags_7', 'flags_12694', 'flags_8', 'unk_2', 'flags_15668')) ]
            fields += [ '{ %s }' % ', '.join(self._spellclassoptions_db[spell.id_class_opts].field('spell_family_flags_1', 'spell_family_flags_2', 'spell_family_flags_3', 'spell_family_flags_4')) ]
            fields += self._spellclassoptions_db[spell.id_class_opts].field('spell_family_name')
            fields += spell.field('desc', 'tt')
            if spell.id_desc_var and self._spelldescriptionvariables_db.get(spell.id_desc_var):
                fields += self._spelldescriptionvariables_db[spell.id_desc_var].field('var')
            else:
                fields += [ '0' ]
            if spell.id_misc and self._spellicon_db.get(self._spellmisc_db[spell.id_misc].id_icon):
                fields += self._spellicon_db[self._spellmisc_db[spell.id_misc].id_icon].field('name')
            else:
                fields += [ '0' ]
            if spell.id_misc and self._spellicon_db.get(self._spellmisc_db[spell.id_misc].id_active_icon):
                fields += self._spellicon_db[self._spellmisc_db[spell.id_misc].id_active_icon].field('name')
            else:
                fields += [ '0' ]
            fields += spell.field('rank')
            # Pad struct with empty pointers for direct access to spell effect data
            fields += [ '0', '0' ]

            s += '  { %s }, /* %s */\n' % (', '.join(fields), ', '.join(effect_ids))
            
            index += 1
        
        s += '};\n\n'
        
        s += '#define __%sSPELLEFFECT%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(effects)
        )
        s += '// %d effects, wow build level %d\n' % ( len(effects), self._options.build )
        s += 'static struct spelleffect_data_t __%sspelleffect%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or ''
        )

        index = 0
        for effect_data in sorted(effects) + [ ( 0, 0 ) ]:
            effect = self._spelleffect_db[effect_data[0]]
            if not effect.id and effect_data[ 0 ] > 0:
                sys.stderr.write('Spell Effect id %d not found\n') % effect_data[0]
                continue

            if index % 20 == 0:
                s += '//{     Id,Flags,   SpId,Idx, EffectType                  , EffectSubType                              ,       Average,         Delta,       Unknown,   Coefficient, APCoefficient,  Ampl,  Radius,  RadMax,   BaseV,   MiscV,  MiscV2, {     Flags1,     Flags2,     Flags3,     Flags4 }, Trigg,   DmgMul,  CboP, RealP,Die, 0, 0 },\n'

            fields = effect.field('id')
            fields += [ '%#.2x' % 0 ] 
            fields += effect.field('id_spell', 'index')
            tmp_fields = []
            if constants.effect_type.get(effect.type):
                tmp_fields += [ '%-*s' % ( constants.effect_type_maxlen, constants.effect_type.get(effect.type) ) ]
            else:
                #print "Type %d missing" % effect.type
                tmp_fields += [ '%-*s' % ( constants.effect_type_maxlen, 'E_%d' % effect.type ) ]

            if constants.effect_subtype.get(effect.sub_type):
                tmp_fields += [ '%-*s' % ( constants.effect_subtype_maxlen, constants.effect_subtype.get(effect.sub_type) ) ]
            else:
                #stm.add(effect.sub_type)
                tmp_fields += [ '%-*s' % ( constants.effect_subtype_maxlen, 'A_%d' % effect.sub_type ) ]

            fields += tmp_fields
            if effect.scaling == None:
                fields += self._spelleffectscaling_db[0].field('average', 'delta', 'bonus')
            else:
                fields += effect.scaling.field('average', 'delta', 'bonus')
            fields += effect.field('coefficient', 'ap_coefficient', 'amplitude')
            fields += self._spellradius_db[effect.id_radius].field('radius_1')
            fields += self._spellradius_db[effect.id_radius_max].field('radius_1')
            fields += effect.field('base_value', 'misc_value', 'misc_value_2')
            fields += [ '{ %s }' % ', '.join( effect.field('class_mask_1', 'class_mask_2', 'class_mask_3', 'class_mask_4' ) ) ]
            fields += effect.field('trigger_spell', 'dmg_multiplier', 'points_per_combo_points', 'real_ppl', 'die_sides')
            # Pad struct with empty pointers for direct spell data access
            fields += [ '0', '0' ]

            s += '  { %s },\n' % (', '.join(fields))

            index += 1

        s += '};\n\n'
        
        index = 0
        def sortf( a, b ):
            if a.id > b.id:
                return 1
            elif a.id < b.id:
                return -1
            
            return 0
        
        powers = list(powers)
        powers.sort(sortf)
        
        s += '#define __%s_SIZE (%d)\n\n' % ( self.format_str( "spellpower" ).upper(), len(powers) )
        s += '// %d effects, wow build level %d\n' % ( len(powers), self._options.build )
        s += 'static struct spellpower_data_t __%s_data[] = {\n' % ( self.format_str( "spellpower" ) )

        for power in powers + [ self._spellpower_db[0] ]:
            fields = power.field('id', 'id_spell', 'aura_id', 'type_power', 'cost', 'cost_2', 'cost_per_second', 'cost_per_second2' )

            s += '  { %s },\n' % (', '.join(fields))
            
        s += '};\n\n'

        return s

class MasteryAbilityGenerator(DataGenerator):
    def __init__(self, options):
        DataGenerator.__init__(self, options)
        
        self._dbc = [ 'Spell', 'ChrSpecialization' ]

        if options.build >= 15589:
            self._dbc.append( 'SpellMisc' )

    def filter(self):
        ids = {}
        for k, v in self._chrspecialization_db.iteritems():
            if v.class_id == 0:
                continue
            
            s = self._spell_db[v.id_mastery]
            if s.id == 0:
                continue
            
            if self._spellmisc_db[s.id_misc].flags_12694 & 0x20000000:
                ids[v.id_mastery] = { 'mask_class' : v.class_id, 'category' : v.spec_id, 'spec_name' : v.name }
            
        return ids

    def generate(self, ids = None):
        max_ids = 0
        mastery_class = 0
        keys    = [ 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
        ]
        
        for k, v in ids.iteritems():
            keys[v['mask_class']][v['category']].append( ( self._spell_db[k].name, k, v['spec_name'] ) )
        
        
        # Find out the maximum size of a key array
        for cls in keys:
            for spec in cls:
                if len(spec) > max_ids:
                    max_ids = len(spec)

        data_str = "%sclass_mastery_ability%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )
        
        s = '#define %s_SIZE (%d)\n\n' % (
            data_str.upper(),
            max_ids
        )
        s += '// Class mastery abilities, wow build %d\n' % self._options.build
        s += 'static unsigned __%s_data[MAX_CLASS][MAX_SPECS_PER_CLASS][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        )
        
        for cls in xrange(0, len(keys)):
            if SpellDataGenerator._class_names[cls]:
                s += '  // Class mastery abilities for %s\n' % ( SpellDataGenerator._class_names[cls] )
                
            s += '  {\n'
            for spec in xrange(0, len(keys[cls])):
                if len(keys[cls][spec]) > 0:
                    s += '    // Masteries for %s specialization\n' % keys[cls][spec][0][2]
                s += '    {\n'
                for ability in sorted(keys[cls][spec], key = lambda i: i[0]):
                    s += '     %6u, // %s\n' % ( ability[1], ability[0] )

                if len(keys[cls][spec]) < max_ids:
                    s += '     %6u,\n' % 0

                s += '    },\n'
            s += '  },\n'
        
        s += '};\n'
        
        return s
    
class RacialSpellGenerator(SpellDataGenerator):
    def __init__(self, options):
        SpellDataGenerator.__init__(self, options)
        
        SpellDataGenerator._class_categories = []
        
    def filter(self):
        ids = { }
        
        for ability_id, ability_data in self._skilllineability_db.iteritems():
            racial_spell = 0
            
            # Take only racial spells to this
            for j in xrange(0, len(SpellDataGenerator._race_categories)):
                if ability_data.id_skill in SpellDataGenerator._race_categories[j]:
                    racial_spell = j
                    break
            
            if not racial_spell:
                continue
            
            spell = self._spell_db[ability_data.id_spell]
            if not self.spell_state(spell):
                continue
                
            if ids.get(ability_data.id_spell):
                ids[ability_data.id_spell]['mask_class'] |= ability_data.mask_class
                ids[ability_data.id_spell]['mask_race'] |= (ability_data.mask_race or (1 << (racial_spell - 1)))
            else:
                ids[ability_data.id_spell] = { 'mask_class': ability_data.mask_class, 'mask_race' : ability_data.mask_race or (1 << (racial_spell - 1)) }

        return ids
    
    def generate(self, ids = None):
        keys = [ ]
        max_ids = 0

        for i in xrange(0, len(DataGenerator._race_names)):
            keys.insert(i, [])
            for j in xrange(0, len(DataGenerator._class_names)):
                keys[i].insert(j, [])

        for k, v in ids.iteritems():
            # Add this for all races and classes that have a mask in v['mask_race']
            for race_bit in xrange(0, len(DataGenerator._race_names)):
                if not DataGenerator._race_names[race_bit]:
                    continue
                
                if v['mask_race'] & (1 << (race_bit - 1)):
                    if v['mask_class']:
                        for class_bit in xrange(0, len(DataGenerator._class_names)):
                            if not DataGenerator._class_names[class_bit]:
                                continue
                                
                            if v['mask_class'] & (1 << (class_bit - 1)):
                                spell = self._spell_db[k]
                                keys[race_bit][class_bit].append( ( spell.name, k ) )
                    # Generic racial spell, goes to "class 0"
                    else:
                        spell = self._spell_db[k]
                        keys[race_bit][0].append( ( spell.name, k ) )
        
        # Figure out tree with most abilities
        for race in xrange(0, len(keys)):
            for cls in xrange(0, len(keys[race])):
                if len(keys[race][cls]) > max_ids:
                    max_ids = len(keys[race][cls])

        data_str = "%srace_ability%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        # Then, output the stuffs
        s = '#define %s_SIZE (%d)\n\n' % (
            data_str.upper(),
            max_ids
        )

        s += "#ifndef %s\n#define %s (%d)\n#endif\n\n" % (
            self.format_str( 'MAX_RACE' ),
            self.format_str( 'MAX_RACE' ),
            len(DataGenerator._race_names) )
        s += '// Racial abilities, wow build %d\n' % self._options.build
        s += 'static unsigned __%s_data[%s][%s][%s_SIZE] = {\n' % (
            self.format_str( 'race_ability' ),
            self.format_str( 'MAX_RACE' ),
            self.format_str( 'MAX_CLASS' ),
            data_str.upper()
        )

        for race in xrange(0, len(keys)):
            if DataGenerator._race_names[race]:
                s += '  // Racial abilities for %s\n' % DataGenerator._race_names[race]
            
            s += '  {\n'
            
            for cls in xrange(0, len(keys[race])):
                if len(keys[race][cls]) > 0:
                    if cls == 0:
                        s += '    // Generic racial abilities\n'
                    else:
                        s += '    // Racial abilities for %s class\n' % DataGenerator._class_names[cls]
                    s += '    {\n'
                else:
                    s += '    { %5d, },\n' % 0
                    continue
                
                for ability in sorted(keys[race][cls], key = lambda i: i[0]):
                    s += '      %5d, // %s\n' % ( ability[1], ability[0] )
                    
                if len(keys[race][cls]) < max_ids:
                    s += '      %5d,\n' % 0
                    
                s += '    },\n'
            s += '  },\n'
        s += '};\n'
        
        return s

class SpecializationSpellGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'Spell', 'SpecializationSpells', 'ChrSpecialization' ]
        
        DataGenerator.__init__(self, options)
    
    def generate(self, ids = None):
        max_ids = 0
        keys = [ 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ] 
        ]
        
        for ssid, data in self._specializationspells_db.iteritems():
            chrspec = self._chrspecialization_db[data.spec_id]
            if chrspec.id == 0:
                continue

            spell = self._spell_db[data.spell_id]
            if spell.id == 0:
                continue
            
            keys[chrspec.class_id][chrspec.spec_id].append( ( self._spell_db[data.spell_id].name, data.spell_id, chrspec.name ) )

        # Figure out tree with most abilities
        for cls in xrange(0, len(keys)):
            for tree in xrange(0, len(keys[cls])):
                if len(keys[cls][tree]) > max_ids:
                    max_ids = len(keys[cls][tree])

        data_str = "%stree_specialization%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        s = '#define %s_SIZE (%d)\n\n' % (
            data_str.upper(),
            max_ids
        )

        s += '// Talent tree specialization abilities, wow build %d\n' % self._options.build 
        s += 'static unsigned __%s_data[][MAX_SPECS_PER_CLASS][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        )

        for cls in xrange(0, len(keys)):
            s += '  // Specialization abilities for %s\n' % (cls > 0 and DataGenerator._class_names[cls] or 'Hunter pets')
            s += '  {\n'

            for tree in xrange(0, len(keys[cls])):
                if len(keys[cls][tree]) > 0:
                    s += '    // Specialization abilities for %s\n' % keys[cls][tree][0][2]
                s += '    {\n'
                for ability in sorted(keys[cls][tree], key = lambda i: i[0]):
                    s += '      %6u, // %s\n' % ( ability[1], ability[0] )

                if len(keys[cls][tree]) < max_ids:
                    s += '      %6u,\n' % 0

                s += '    },\n'
            s += '  },\n'
        s += '};\n'

        return s

class PerkSpellGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'ChrSpecialization', 'MinorTalent', 'Spell' ]
        
        DataGenerator.__init__(self, options)
    
    def generate(self, ids = None):
        max_ids = 0
        keys = [ 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ], 
            [ [], [], [], [] ] 
        ]

        spec_map = { }
        
        for ssid, data in self._chrspecialization_db.iteritems():
            spec_map[ssid] = (data.class_id, data.spec_id, data.name)

        for mtid, data in self._minortalent_db.iteritems():
            spec = data.id_spec

            pos_data = spec_map[spec]
            spell = self._spell_db[data.id_spell]

            keys[pos_data[0]][pos_data[1]].append((data.index, data.id_spell, pos_data[2], spell.name))

        # Figure out tree with most abilities
        for cls in xrange(0, len(keys)):
            for tree in xrange(0, len(keys[cls])):
                if len(keys[cls][tree]) > max_ids:
                    max_ids = len(keys[cls][tree])

        data_str = "%sperk%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        s = '#define %s_SIZE (%d)\n\n' % (
            data_str.upper(),
            max_ids
        )

        s += '// Perk specialization abilities, wow build %d\n' % self._options.build 
        s += 'static unsigned __%s_data[][MAX_SPECS_PER_CLASS][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        )

        for cls in xrange(0, len(keys)):
            s += '  {\n'

            for tree in xrange(0, len(keys[cls])):
                if len(keys[cls][tree]) > 0:
                    s += '    // %s\n' % keys[cls][tree][0][2]
                s += '    {\n'
                for ability in sorted(keys[cls][tree], key = lambda i: i[0]):
                    s += '      %6u, // %d: %s\n' % ( ability[1], ability[0], ability[3] )

                if len(keys[cls][tree]) < max_ids:
                    s += '      %6u,\n' % 0

                s += '    },\n'
            s += '  },\n'
        s += '};\n'

        return s

class SpellListGenerator(SpellDataGenerator):
    def __init__(self, options):
        SpellDataGenerator.__init__(self, options)

    def spell_state(self, spell, enabled_effects = None):
        if not SpellDataGenerator.spell_state(self, spell, None):
            return False

        # Skip passive spells
        if self._spellmisc_db[spell.id_misc].flags & 0x40:
            self.debug( "Spell id %u (%s) marked as passive" % ( spell.id, spell.name ) )
            return False

        if self._spellmisc_db[spell.id_misc].flags & 0x80:
            self.debug( "Spell id %u (%s) marked as hidden" % ( spell.id, spell.name ) )
            return False

        # Skip by possible indicator for spellbook visibility
        if self._spellmisc_db[spell.id_misc].flags_4 & 0x8000:
            self.debug( "Spell id %u (%s) marked as hidden in spellbook" % ( spell.id, spell.name ) )
            return False;

        # Skip spells without any resource cost and category
        found_power = False
        for power in spell._powers:
            if not power:
                continue

            if power.cost > 0 or power.cost_2 > 0 or power.cost_per_second > 0:
                found_power = True
                break

        if not found_power and spell.id_rune_cost == 0 and spell.id_categories == 0:
            self.debug( "Spell id %u (%s) has no power requirements" % ( spell.id, spell.name ) )
            return False

        # Make sure rune cost makes sense, even if the rune cost id is valid
        if spell.id_rune_cost > 0:
            src = self._spellrunecost_db[spell.id_rune_cost]
            if src.rune_cost_1 == 0 and src.rune_cost_2 == 0 and src.rune_cost_3 == 0 and src.rune_cost_4 == 0:
                self.debug( "Spell id %u (%s) has no sensible rune cost" % ( spell.id, spell.name ) )
                return False

        # Filter out any "Rank x" string, as there should no longer be such things. This should filter out 
        # some silly left over? things, or things not shown to player anyhow, so should be all good.
        if spell.ofs_rank > 0 and 'Rank ' in spell.rank:
            self.debug( "Spell id %u (%s) has a rank defined" % ( spell.id, spell.name ) )
            return False

        # Let's not accept spells that have over 100y range, as they cannot really be base abilities then
        if self._spellmisc_db[spell.id_misc].id_range > 0:
            range = self._spellrange_db[self._spellmisc_db[spell.id_misc].id_range]
            if range.max_range > 100.0 or range.max_range_2 > 100.0:
                self.debug( "Spell id %u (%s) has a high range (%f, %f)" % ( spell.id, spell.name, range.max_range, range.max_range_2 ) )
                return False
        
        # And finally, spells that are forcibly activated/disabled in whitelisting for 
        for cls in xrange(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls]:
                if  spell_tuple[0] == spell.id and len(spell_tuple) == 2 and spell_tuple[1] == 0:
                    return False
                elif spell_tuple[0] == spell.id and len(spell_tuple) == 3:
                    return spell_tuple[2]
        
        return True

    def filter(self):
        triggered_spell_ids = []
        ids = { }
        spell_tree = -1
        spell_tree_name = ''
        for ability_id, ability_data in self._skilllineability_db.iteritems():
            if ability_data.id_skill in SpellDataGenerator._skill_category_blacklist:
                continue

            mask_class_skill = self.class_mask_by_skill(ability_data.id_skill)
            mask_class_pet_skill = self.class_mask_by_pet_skill(ability_data.id_skill)
            mask_class = 0

            # Generic Class Ability
            if mask_class_skill > 0:
                spell_tree_name = "General"
                spell_tree = 0
                mask_class = mask_class_skill
            elif mask_class_pet_skill > 0:
                spell_tree_name = "Pet"
                spell_tree = 5
                mask_class = mask_class_pet_skill

            # We only want abilities that belong to a class
            if mask_class == 0:
                continue

            spell = self._spell_db[ability_data.id_spell]
            if not spell.id:
                continue

            # Blacklist all triggered spells for this
            for effect in spell._effects:
                if not effect:
                    continue

                if effect.trigger_spell > 0:
                    triggered_spell_ids.append(effect.trigger_spell)

            # Check generic SpellDataGenerator spell state filtering before anything else
            if not self.spell_state(spell):
                continue

            if ids.get(ability_data.id_spell):
                ids[ability_data.id_spell]['mask_class'] |= ability_data.mask_class or mask_class
            else:
                ids[ability_data.id_spell] = { 
                    'mask_class': ability_data.mask_class or mask_class,
                    'tree'      : [ spell_tree ]
                }

        # Specialization spells
        for ss_id, ss_data in self._specializationspells_db.iteritems():
            chrspec = self._chrspecialization_db[ss_data.spec_id]

            if chrspec.class_id == 0:
                continue
            
            spell = self._spell_db[ss_data.spell_id]
            if not spell.id:
                continue
            
            # Check generic SpellDataGenerator spell state filtering before anything else
            if not self.spell_state(spell):
                continue
        
            if ids.get(ss_data.spell_id):
                ids[ss_data.spell_id]['mask_class'] |= DataGenerator._class_masks[chrspec.class_id]
                if chrspec.spec_id + 1 not in ids[ss_data.spell_id]['tree']:
                    ids[ss_data.spell_id]['tree'].append( chrspec.spec_id + 1 )
            else:
                ids[ss_data.spell_id] = { 
                    'mask_class': DataGenerator._class_masks[chrspec.class_id],
                    'tree'      : [ chrspec.spec_id + 1 ]
                }

        for cls in xrange(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls]:
                # Skip spells with zero tree, as they dont exist
                #if spell_tuple[1] == 0:
                #    continue

                spell = self._spell_db[spell_tuple[0]]
                if not spell.id:
                    continue

                if len(spell_tuple) == 2 and (spell_tuple[1] == 0 or not self.spell_state(spell)):
                    continue
                elif len(spell_tuple) == 3 and spell_tuple[2] == False:
                    continue

                if ids.get(spell_tuple[0]):
                    ids[spell_tuple[0]]['mask_class'] |= self._class_masks[cls]
                    if spell_tuple[1] not in ids[spell_tuple[0]]['tree']:
                        ids[spell_tuple[0]]['tree'].append( spell_tuple[1] )
                else:
                    ids[spell_tuple[0]] = { 
                        'mask_class': self._class_masks[cls], 
                        'tree'      : [ spell_tuple[1] ],
                    }

        # Finally, go through the spells and remove any triggered spell
        # as the order in dbcs is arbitrary
        for id in ids.keys():
            if id in triggered_spell_ids:
                self.debug("Spell id %u (%s) is a triggered spell" % (id, self._spell_db[id].name))
                del ids[id]

        return ids

    def generate(self, ids = None):
        keys = [ 
            # General | Spec0 | Spec1 | Spec2 | Spec3 | Pet
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
            [ [], [], [], [], [], [] ], 
        ]

        # Sort a suitable list for us
        for k, v in ids.iteritems():
            if v['mask_class'] not in DataGenerator._class_masks:
                continue

            spell = self._spell_db[k]
            for tree in v['tree']:
                keys[self._class_map[v['mask_class']]][tree].append(( spell.name, spell.id ))

        # Find out the maximum size of a key array
        max_ids = 0
        for cls_list in keys:
            for tree_list in cls_list:
                if len(tree_list) > max_ids:
                    max_ids = len(tree_list)

        data_str = "%sclass_ability%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        s = '#define %s_SIZE (%d)\n' % (
            data_str.upper(),
            max_ids
        )
        s += '#define %s_TREE_SIZE (%d)\n\n' % ( data_str.upper(), len( keys[0] ) )
        s += "#ifndef %s\n#define %s (%d)\n#endif\n" % (
            self.format_str( 'MAX_CLASS' ),
            self.format_str( 'MAX_CLASS' ),
            len(DataGenerator._class_names) )
        s += '// Class based active abilities, wow build %d\n' % self._options.build
        s += 'static unsigned __%s_data[][%s_TREE_SIZE][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
            data_str.upper(),
        )

        for i in xrange(0, len(keys)):
            if SpellDataGenerator._class_names[i]:
                s += '  // Class active abilities for %s\n' % ( SpellDataGenerator._class_names[i] )

            s += '  {\n'

            for j in xrange(0, len(keys[i])):
                # See if we can describe the tree
                for t in keys[i][j]:
                    tree_name = ''
                    if j == 0:
                        tree_name = 'General'
                    elif j == 5:
                        tree_name = 'Pet'
                    else:
                        for chrspec_id, chrspec_data in self._chrspecialization_db.iteritems():
                            if chrspec_data.class_id == i and chrspec_data.spec_id == j - 1:
                                tree_name = chrspec_data.name
                                break
                    
                    s += '    // %s tree, %d abilities\n' % ( tree_name, len(keys[i][j]) )
                    break
                s += '    {\n'
                for spell_id in sorted(keys[i][j], key = lambda k_: k_[0]):
                    r = ''
                    if self._spell_db[spell_id[1]].rank:
                        r = ' (%s)' % self._spell_db[spell_id[1]].rank
                    s += '      %6u, // %s%s\n' % ( spell_id[1], spell_id[0], r )

                # Append zero if a short struct
                if max_ids - len(keys[i][j]) > 0:
                    s += '      %6u,\n' % 0

                s += '    },\n'

            s += '  },\n'

        s += '};\n'

        return s

class ClassFlagGenerator(SpellDataGenerator):
    _masks = {
            'mage': 3,
            'warrior': 4,
            'warlock': 5,
            'priest': 6,
            'druid': 7,
            'rogue': 8,
            'hunter': 9,
            'paladin': 10,
            'shaman': 11,
            'deathknight': 15
    }
    def __init__(self, options):
        SpellDataGenerator.__init__(self, options)

    def filter(self, class_name):
        ids = { }
        mask = ClassFlagGenerator._masks.get(class_name.lower(), -1)
        if mask == -1:
            return ids

        for id, data in self._spell_db.iteritems():
            if data.id_class_opts == 0:
                continue

            opts = self._spellclassoptions_db[data.id_class_opts]

            if opts.spell_family_name != mask:
                continue

            ids[id] = { }
        return ids

    def generate(self, ids):
        s = ''

        spell_data = []
        effect_data = { }
        for i in xrange(0, 128):
            spell_data.append({ 'spells' : [ ], 'effects': [ ] })

        for spell_id, data in ids.iteritems():
            spell = self._spell_db[spell_id]
            if not spell.id_class_opts:
                continue

            copts = self._spellclassoptions_db[spell.id_class_opts]

            # Assign this spell to bitfield entries
            for i in xrange(1, 5):
                f = getattr(copts, 'spell_family_flags_%u' % i)
                
                for bit in xrange(0, 32):
                    if not (f & (1 << bit)):
                        continue

                    bfield = ((i - 1) * 32) + bit
                    spell_data[bfield]['spells'].append( spell )

            # Loop through spell effects, assigning them to effects
            for effect in spell._effects:
                if not effect:
                    continue

                for i in xrange(1, 5):
                    f = getattr(effect, 'class_mask_%u' % i)
                    for bit in xrange(0, 32):
                        if not (f & (1 << bit)):
                            continue

                        bfield = ((i - 1) * 32) + bit
                        spell_data[bfield]['effects'].append( effect )

        # Build effect data
        
        for bit_data in spell_data:
            for effect in bit_data['effects']:
                if not effect_data.has_key(effect.id_spell):
                    effect_data[effect.id_spell] = {
                        'effects': { },
                        'spell': self._spell_db[effect.id_spell]
                    }

                if not effect_data[effect.id_spell]['effects'].has_key(effect.index):
                    effect_data[effect.id_spell]['effects'][effect.index] = []

                effect_data[effect.id_spell]['effects'][effect.index] += bit_data['spells']
        
        field = 0
        for bit_field in spell_data:
            field += 1
            if not len(bit_field['spells']):
                continue

            if not len(bit_field['effects']):
                continue

            s += '  [%-3d] ===================================================\n' % field
            for spell in sorted(bit_field['spells'], key = lambda s: s.name):
                s += '       %s (%u)\n' % ( spell.name, spell.id )
            
            for effect in sorted(bit_field['effects'], key = lambda e: e.id_spell):
                rstr = ''
                if self._spell_db[effect.id_spell].rank:
                    rstr = ' (%s)' % self._spell_db[effect.id_spell].rank
                s += '         [%u] {%u} %s%s\n' % ( effect.index, effect.id_spell, self._spell_db[effect.id_spell].name, rstr)

            s += '\n'
        
        for spell_id in sorted(effect_data.keys()):
            spell = effect_data[spell_id]['spell']
            s += 'Spell: %s (%u)' % (spell.name, spell.id)
            if spell.rank:
                s += ' %s' % spell.rank
            s += '\n'

            effects = effect_data[spell_id]['effects']
            for effect_index in sorted(effects.keys()):
                s += '  Effect#%u:\n' % effect_index
                for spell in sorted(effects[effect_index], key = lambda s: s.id):
                    s += '    %s (%u)' % (spell.name, spell.id)
                    if spell.rank:
                        s += ' %s' % spell.rank
                    s += '\n'
                s += '\n'
            s += '\n'
        
        return s

class GlyphPropertyGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'GlyphProperties.dbc', 'Spell.dbc' ]

        DataGenerator.__init__(self, options)

    def filter(self):
        return None
        
    def generate(self, ids = None):
        data_str = "%sglyph_property_data%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        content_str = ''
        properties = 0
        for id, data in self._glyphproperties_db.iteritems():
            if data.id_spell == 0:
                continue

            if self._spell_db[data.id_spell].id == 0:
                continue

            content_str += '  { %5u, %6u },\n' % (data.id, data.id_spell)
            properties += 1


        s = '// Glyph properties, wow build %d\n' % self._options.build
        s += 'static glyph_property_data_t __%s[%d] = {\n' % (data_str, properties + 1)
        s += content_str
        s += '  { %5u, %6u }\n' % (0, 0)
        s += '};\n'
        
        return s

class GlyphListGenerator(SpellDataGenerator):
    def __init__(self, options):

        SpellDataGenerator.__init__(self, options)

    def filter(self):
        ids = { }

        for ability_id, ability_data in self._skilllineability_db.iteritems():
            if ability_data.id_skill != 810 or not ability_data.mask_class:
                continue

            use_glyph_spell = self._spell_db[ability_data.id_spell]
            if not use_glyph_spell.id:
                continue

            # Find the on-use for glyph then, misc value will contain the correct GlyphProperties.dbc id
            for effect in use_glyph_spell._effects:
                if not effect or effect.type != 74: # Use glyph
                    continue

                # Filter some erroneous glyph data out
                glyph_data = self._glyphproperties_db[effect.misc_value]
                if not glyph_data.id or not glyph_data.id_spell:
                    continue

                if ids.get(glyph_data.id_spell):
                    ids[glyph_data.id_spell]['mask_class'] |= ability_data.mask_class
                else:
                    ids[glyph_data.id_spell] = { 'mask_class': ability_data.mask_class, 'glyph_slot' : glyph_data.flags }

        return ids
        
    def generate(self, ids = None):
        max_ids = 0
        keys = [ 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ], 
            [ [], [], [] ] 
        ]

        glyph_slot_names = [ 'Major', 'Minor', 'Prime' ]

        for k, v in ids.iteritems():
            keys[self._class_map[v['mask_class']]][v['glyph_slot']].append( ( self._spell_db[k].name, k ) )
        
        # Figure out tree with most abilities
        for cls in xrange(0, len(keys)):
            for glyph_slot in xrange(0, len(keys[cls])):
                if len(keys[cls][glyph_slot]) > max_ids:
                    max_ids = len(keys[cls][glyph_slot])

        data_str = "%sglyph_abilities%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        s = '#define %s_SIZE (%d)\n\n' % (
            data_str.upper(),
            max_ids
        )

        s += '// Glyph spells for classes, wow build %d\n' % self._options.build
        s += 'static unsigned __%s_data[][3][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        )
        
        for cls in xrange(0, len(keys)):
            if DataGenerator._class_names[cls]:
                s += '  // Glyph spells for %s\n' % DataGenerator._class_names[cls]
            s += '  {\n'
            
            for glyph_slot in xrange(0, len(keys[cls])):
                if len(keys[cls][glyph_slot]) > 0:
                    s += '    // %s Glyphs (%d spells)\n' % (glyph_slot_names[glyph_slot], len(keys[cls][glyph_slot]))
                s += '    {\n'
                for glyph in sorted(keys[cls][glyph_slot], key = lambda i: i[0]):
                    s += '      %6u, // %s\n' % ( glyph[1], glyph[0] )
                    
                if len(keys[cls][glyph_slot]) < max_ids:
                    s += '      %6u,\n' % 0
                    
                s += '    },\n'
            s += '  },\n'
        s += '};\n'
        
        return s

class SetBonusListGenerator(DataGenerator):
    # A tier spell map that maps ItemSetSpell.dbc IDs to tiers. This is needed
    # to ensure that certain re-used tier spells (Ret paladin T10 set bonus for
    # example) get the correct tier number, as blizzard does not provide tier
    # information for us, outside of their spell name template for the bonus
    # spells
    tier_spell_map = {
            # Ret T10 bonus is re-used in T14
            2980: 14
    }

    # Older set bonuses (T16 and before) need to be mapped to "roles" in
    # simulationcraft (for backwards compatibility reasons, and because set
    # bonuses were not "spec specific" before Warlords of Draenor). We can use
    # the specialization roles for healer and tank, however simulationcraft
    # makes a distinction between "caster" and "melee" roles, so the DPS
    # specialization role in the client has to be split into two, by spec.
    #
    # Use Blizzard's "dps" spec type (2) as "melee", and add 3 as the "caster"
    # version. All of this is only relevant to backwards support our
    # "tierxx_ypc_zzzz" options, where zzzz is the role. Tier17 onwards, set
    # bonuses are spec specific, and we can simply enable/disable bonuses.
    role_map = {
            # Balance Druid
            102: 3,
            # Mage specs
            62 : 3, 63: 3, 64: 3,
            # Shadow Priest
            258: 3,
            # Elemental Shaman
            262: 3,
            # Warlock specs
            265: 3, 266: 3, 267: 3
    }

    def __init__(self, options):
        self._dbc = [ 'ItemSet', 'ItemSetSpell', 'Spell', 'ChrSpecialization', 'Item-sparse', 'ItemNameDescription' ]

        self._regex = re.compile(r'^Item\s+-\s+(.+)\s+T([0-9]+)\s+([A-z\s]*)\s*([0-9]+)P\s+Bonus')

        DataGenerator.__init__(self, options)

    def initialize(self):
        DataGenerator.initialize(self)

        self.spec_type_map = {}
        for spec_id, spec_data in self._chrspecialization_db.iteritems():
            if spec_data.class_id not in self.spec_type_map:
                self.spec_type_map[spec_data.class_id] = { }

            if spec_data.spec_type not in self.spec_type_map[spec_data.class_id]:
                self.spec_type_map[spec_data.class_id][spec_data.spec_type] = []

            self.spec_type_map[spec_data.class_id][spec_data.spec_type].append(spec_id)

        return True

    def derive_specs(self, class_, name):
        specs = []
        spec_added = False
        roles = set()
        for spec_id, spec_data in self._chrspecialization_db.iteritems():
            if class_ == spec_data.class_id and spec_data.name in name:
                spec_added = True
                specs.append(spec_id)
                roles.add(SetBonusListGenerator.role_map.get(spec_id, spec_data.spec_type))

        # If we could not figure out specs from spec specific words, use
        # some generic things and blizzard typoes .. sigh .. to massage
        # things a bit
        if not spec_added:
            if name == 'Tank':
                specs = self.spec_type_map[class_][0]
                roles.add(0)
            elif name in ['DPS', 'Melee']:
                specs = self.spec_type_map[class_][2]
                roles.add(SetBonusListGenerator.role_map.get(specs[0], 2))
            elif name in ['Healer', 'Healing']:
                specs = self.spec_type_map[class_][1]
                roles.add(1)
            # Pure DPS classes can have empty string, in which case just
            # slap in all specs for the class
            elif len(name) == 0:
                specs = self.spec_type_map[class_][2]
                roles.add(SetBonusListGenerator.role_map.get(specs[0], 2))
            # Sigh ...
            elif name == 'Enhancment':
                specs = [ 263, ]
                roles.add(2)
            else:
                print >>sys.stderr, "Found set bonus spell '%s' that does not match Blizzard set bonus spell template" % name

        return specs, list(roles)[0]

    def filter(self):
        data = []
        for id, set_spell_data in self._itemsetspell_db.iteritems():
            spell_data = self._spell_db[set_spell_data.id_spell]

            mobj = self._regex.match(spell_data.name)
            if not mobj:
                continue

            set_spell_id = set_spell_data.id_spell
            set_class = self._class_map[mobj.group(1)]
            set_tier_derived = int(mobj.group(2))
            set_tier_spec_string = mobj.group(3).strip()
            set_tier_bonus_derived = int(mobj.group(4))
            set_spec = 0
            set_role = -1

            if id in SetBonusListGenerator.tier_spell_map:
                set_tier_derived = SetBonusListGenerator.tier_spell_map[id]

            set_spec_arr_derived = []

            if set_spell_data.unk_wod_1 == 0:
                set_spec_arr_derived, set_role = self.derive_specs(set_class, set_tier_spec_string)
            # We have a spec. Figure out class from it too and stop guessing by
            # name.
            else:
                set_spec = set_spell_data.unk_wod_1
                for spec_id, spec_data in self._chrspecialization_db.iteritems():
                    if spec_id == set_spec:
                        set_class = spec_data.class_id
                        set_role = SetBonusListGenerator.role_map.get(spec_id, spec_data.spec_type)
                        break
                set_spec_arr_derived.append(0)

            data.append({
                'derived_class': set_class,
                'derived_tier' : set_tier_derived,
                'derived_spell': set_spell_id,
                'derived_bonus': set_tier_bonus_derived,
                'derived_specs': set_spec_arr_derived,
                'spec'         : set_spec,
                'role'         : set_role,
                'set_id'       : set_spell_data.id_item_set,
                'set_bonus_id' : id })

        # Check for new set bonuses that have no defined ItemSetSpell/ItemSet
        # entries yet. These are wild guesses and will _never_ overwrite data
        # that already exists in the results
        for id, spell_data in self._spell_db.iteritems():
            mobj = self._regex.match(spell_data.name)
            if not mobj:
                continue

            set_spell_id = id
            set_class = self._class_map[mobj.group(1)]
            set_tier_derived = int(mobj.group(2))
            set_tier_spec_string = mobj.group(3).strip()
            set_tier_bonus_derived = int(mobj.group(4))
            set_role = -1

            if id in SetBonusListGenerator.tier_spell_map:
                set_tier_derived = SetBonusListGenerator.tier_spell_map[id]

            set_spec_arr_derived, set_role = self.derive_specs(set_class, set_tier_spec_string)

            found = False
            for entry in data:
                if entry['derived_class'] == set_class and \
                   entry['derived_tier'] == set_tier_derived and \
                   entry['derived_bonus'] == set_tier_bonus_derived:
                    found = True

                    # Do the spec check spearately. It will need to flip the found to
                    # False if the specs dont match reasonably.
                    if found:
                        if entry['spec'] > 0 and set_spec_arr_derived[0] != entry['spec']:
                            found = False
                        elif entry['spec'] == 0 and set_spec_arr_derived != entry['derived_specs']:
                            found = False

                    if found:
                        break

            if found:
                continue

            data.append({
                'derived_class': set_class,
                'derived_tier' : set_tier_derived,
                'derived_spell': set_spell_id,
                'derived_bonus': set_tier_bonus_derived,
                'derived_specs': set_spec_arr_derived,
                'spec'         : 0,
                'role'         : set_role,
                'set_id'       : 0,
                'set_bonus_id' : 0 })


        # PvP stuff very unfinished
        pvp_bonuses = {}
        for id, set_spell_data in self._itemsetspell_db.iteritems():
            itemset = self._itemset_db[set_spell_data.id_item_set]
            if 'Gladiator' not in itemset.name:
                continue

            if set_spell_data.unk_wod_1 == 0:
                continue

            spec_data = self._chrspecialization_db[set_spell_data.unk_wod_1]
            pvp_bonuses[(set_spell_data.unk_wod_1, set_spell_data.n_req_items)] = {
                'derived_class' : spec_data.class_id,
                'derived_tier'  : 0,
                'derived_spell' : set_spell_data.id_spell,
                'derived_bonus' : set_spell_data.n_req_items,
                'derived_specs' : [ 0 ],
                'spec'          : set_spell_data.unk_wod_1,
                'role'          : SetBonusListGenerator.role_map.get(spec_id, spec_data.spec_type),
                'set_id'        : itemset.id,
                'set_bonus_id'  : id
            }

        for spec, pvp_data in pvp_bonuses.iteritems():
            data.append(pvp_data)

        return data

    def generate(self, ids):
        def sorter(a, b):
            if a['derived_tier'] != b['derived_tier']:
                return a['derived_tier'] - b['derived_tier']
            else:
                if a['derived_class'] != b['derived_class']:
                    return a['derived_class'] - b['derived_class']
                else:
                    if a['spec'] != b['spec']:
                        return a['spec'] - b['spec']
                    else:
                        if a['derived_specs'][0] != b['derived_specs'][0]:
                            return a['derived_specs'][0] - b['derived_specs'][0]
                        else:
                            if a['derived_bonus'] != b['derived_bonus']:
                                return a['derived_bonus'] - b['derived_bonus']
                            else:
                                return 0

        ids.sort(cmp = sorter)

        data_str = "%sset_bonus_data%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        s = '#define %s_SIZE (%d)\n\n' % (
            data_str.upper(),
            len(ids)
        )

        s += '// Set bonus data, wow build %d\n' % self._options.build
        s += 'static item_set_bonus_t __%s[%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        )

        for data_idx in xrange(0, len(ids)):
            entry = ids[data_idx]

            if data_idx % 25 == 0:
                s += '  //%45s, SetID, Tier, Bns, Cls, %20s, Role, Spec,  Spell, Items\n' % ('Tier name', 'Derived Spec')

            item_set_spell = None
            if entry['set_bonus_id'] > 0 :
                item_set_spell = self._itemsetspell_db[entry['set_bonus_id']]

            spec = 0
            set_bonus = entry['derived_bonus']
            spell_id = entry['derived_spell']
            item_set_str = ""
            items = []
            role = entry['role']
            if item_set_spell:
                spec = item_set_spell.unk_wod_1
                item_set = self._itemset_db[item_set_spell.id_item_set]
                item_set_str = item_set.name
                if item_set_spell.n_req_items != set_bonus:
                    print >>sys.stderr, 'Discrepancy between set bonuses for %s, derived %u, data %u' % (
                            item_set.name, set_bonus, item_set_spell.n_req_items )
                    set_bonus = item_set_spell.n_req_items

                for item_n in xrange(1, 17):
                    item_id = getattr(item_set, 'id_item_%d' % item_n)
                    if item_id > 0:
                        items.append('%6u' % item_id)
            else:
                spec_str = ''
                if len(entry['derived_specs']) == 1:
                    spec_str = ' %s' % self._chrspecialization_db[entry['derived_specs'][0]].name
                item_set_str = '[PH] %s%s T%d %dP Bonus' % (
                        DataGenerator._class_names[entry['derived_class']],
                        spec_str,
                        entry['derived_tier'], entry['derived_bonus'] )

            if len(items) < 17:
                items.append(' 0')

            spec_str = ''
            if entry['derived_specs'][0] > 0:
                for idx in xrange(0, len(entry['derived_specs'])):
                    spec_str += '%3u, ' % entry['derived_specs'][idx]

            spec_str += '0'

            s += '  { %-45s, %5u, %4u, %3u, %3u, %20s, %4u, %4u, %6u, %s },\n' % (
                '"%s"' % item_set_str.replace('"', '\\"'),
                entry['set_id'],
                entry['derived_tier'],
                set_bonus,
                entry['derived_class'],
                '{ %s }' % spec_str,
                role,
                spec,
                spell_id,
                '{ %s }' % (', '.join(items))
            )

        s += '};\n'
        return s

class ItemSetListGenerator(SpellDataGenerator):
    _tier_base = 13

    def __init__(self, options):

        SpellDataGenerator.__init__(self, options)

    def filter(self):
        ids = { }
        mask_class_category = 0
        tier_id = 0

        # Item sets, loop through ItemSet.dbc getting class-specific tier sets and add 
        # their bonuses to the spell list
        for itemset_id, itemset_data in self._itemset_db.iteritems():
            mask_class_category = 0
            tier_id = 0
            for cls in xrange(0, len(SpellDataGenerator._item_set_list)):
                for tier in xrange(0, len(SpellDataGenerator._item_set_list[cls])):
                    if itemset_id in SpellDataGenerator._item_set_list[cls][tier]:
                        mask_class_category = DataGenerator._class_masks[cls]
                        tier_id = tier
                        break

                if mask_class_category:
                    break

            if not mask_class_category:
                continue

            # Item set is a tier set, we want informations.
            for bonus in itemset_data.bonus:
                spell_id = bonus.id_spell
                if spell_id:
                    f = { }
                    self.process_spell(spell_id, f, mask_class_category, 0)
                    
                    if spell_id not in f:
                        continue

                    ids[spell_id] = {
                        'mask_class': mask_class_category,
                        'set'       : itemset_data.name,
                        'tier'      : tier_id,
                        'n_bonus'   : bonus.n_req_items
                    }

        return ids
        
    def generate(self, ids = None):
        max_ids = 0
        s = ''
        keys = [ ]

        for cls in xrange(0, len(SpellDataGenerator._item_set_list)):
            keys.append([])
            for tier in SpellDataGenerator._item_set_list[cls]:
                keys[cls].append([])

        for spell_id, sdata in ids.iteritems():
            keys[self._class_map[sdata['mask_class']]][sdata['tier']].append( (
                spell_id,
                sdata['set'],
                sdata['n_bonus'] )
            )

        # Figure out tree with most abilities
        for cls in xrange(0, len(keys)):
            for tier_id in xrange(0, len(keys[cls])):
                if len(keys[cls][tier_id]) > max_ids:
                    max_ids = len(keys[cls][tier_id])

        data_str = "%stier_bonuses%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        s = '#define %s_SIZE (%d)\n' % (
            data_str.upper(),
            max_ids
        )
        
        s += '#define %s_MAX_TIER (%d)\n\n' % (
            data_str.upper(),
            len(keys[cls]) + 1
        )

        s += '#define %s_TIER_BASE (%d)\n\n' % (
            data_str.upper(),
            self._tier_base
        )

        s += '// Tier item set bonuses for class, wow build %d\n' % self._options.build
        s += 'static unsigned __%s_data[][%s_MAX_TIER][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
            data_str.upper(),            
        )

        for cls in xrange(0, len(keys)):
            if DataGenerator._class_names[cls]:
                s += '  // Tier bonuses for %s\n' % DataGenerator._class_names[cls]
            s += '  {\n'
            
            for tier_id in xrange(0, len(keys[cls])):
                if len(keys[cls][tier_id]) > 0:
                    s += '    // Tier %d bonuses (%d spells)\n' % (self._tier_base + tier_id, len(keys[cls][tier_id]))
                s += '    {\n'
                for tier_bonus in sorted(keys[cls][tier_id], key = lambda i: i[0]):
                    s += '      %5d, // %s - %d Piece Bonus (%s)\n' % ( 
                        tier_bonus[0],
                        tier_bonus[1],
                        tier_bonus[2],
                        self._spell_db[tier_bonus[0]].name)
                    
                if len(keys[cls][tier_id]) < max_ids:
                    s += '      %5d,\n' % 0
                    
                s += '    },\n'
            s += '    {\n'	
            s += '      %5d,\n' % 0
            s += '    },\n'
            s += '  },\n'
        s += '};\n'

        return s

class RandomSuffixGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'ItemRandomSuffix', 'SpellItemEnchantment' ]

        DataGenerator.__init__(self, options)

    def filter(self):
        ids = set()
        # Let's do some modest filtering here, take only "stat" enchants, 
        # and take out the test items as well
        for id, data in self._itemrandomsuffix_db.iteritems():
            # of the Test, of the Paladin Testing
            if id == 46 or id == 48:
                continue
            
            has_non_stat_enchant = False
            # For now, naively presume type_1 of SpellItemEnchantment will tell us
            # if it's a relevant enchantment for us (ie. a stat one )
            for i in xrange(1,4):
                item_ench = self._spellitemenchantment_db.get( getattr(data, 'id_property_%d' % i) )
                if not item_ench:
                    self.debug( "No item enchantment found for %s (%s)" % (data.name_sfx, data.name_int) )
                    continue
                
                if item_ench.type_1 != 5:
                    has_non_stat_enchant = True
                    break
                
            if has_non_stat_enchant:
                continue
            
            ids.add( id )
            
        return list(ids)

    def generate(self, ids = None):
        # Sort keys
        ids.sort()

        s = '#define %sRAND_SUFFIX%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        )
        s += '// Random "cataclysm" item suffixes, wow build %d\n' % self._options.build
        s += 'static struct random_suffix_data_t __%srand_suffix%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' )
        
        for id in ids + [ 0 ]:
            rs = self._itemrandomsuffix_db[id]
            
            fields  = rs.field('id', 'suffix')
            fields += [ '{ %s }' % ', '.join(rs.field('id_property_1', 'id_property_2', 'id_property_3', 'id_property_4', 'id_property_5')) ]
            fields += [ '{ %s }' % ', '.join(rs.field('property_pct_1', 'property_pct_2', 'property_pct_3', 'property_pct_4', 'property_pct_5')) ]
            s += '  { %s },' % (', '.join(fields))
            if rs.name_int:
                s += ' // %s' % rs.name_int
            s += '\n'

        s += '};\n'
    
        return s

class SpellItemEnchantmentGenerator(RandomSuffixGenerator):
    def __init__(self, options):
        RandomSuffixGenerator.__init__(self, options)

        self._dbc += ['Spell', 'SpellEffect']

    def initialize(self):
        RandomSuffixGenerator.initialize(self)

        for spell_effect_id, spell_effect_data in self._spelleffect_db.iteritems():
            if not spell_effect_data.id_spell:
                continue

            spell = self._spell_db[spell_effect_data.id_spell]
            if not spell.id:
                continue

            spell.add_effect(spell_effect_data)
        
        # Map spell ids to spellitemenchantments, as there's no direct 
        # link between them, and 5.4+, we need/want to scale enchants properly
        for id, data in self._spell_db.iteritems():
            enchant = False

            for effect in data._effects:
                # Skip all effects that are not of type enchant item
                if not effect or effect.type != 53:
                    continue

                item_ench = self._spellitemenchantment_db[effect.misc_value]
                if item_ench.id == 0:
                    continue

                if hasattr(item_ench, '_spells'):
                    item_ench._spells.append(data)
                else:
                    item_ench._spells = [ data ]

        return True

    def filter(self):
        return self._spellitemenchantment_db.keys()

    def generate(self, ids = None):
        ids.sort()

        s = '#define %sSPELL_ITEM_ENCH%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        )
        s += '// Item enchantment data, wow build %d\n' % self._options.build
        s += 'static struct item_enchantment_data_t __%sspell_item_ench%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' )

        for i in ids + [ 0 ]:
            ench_data = self._spellitemenchantment_db[i]

            fields = ench_data.field('id', 'slot', 'id_gem', 'id_scaling', 'min_scaling_level', 'max_scaling_level', 'req_skill', 'req_skill_value')
            fields += [ '{ %s }' % ', '.join(ench_data.field('type_1', 'type_2', 'type_3')) ]
            fields += [ '{ %s }' % ', '.join(ench_data.field('amount_1', 'amount_2', 'amount_3')) ]
            fields += [ '{ %s }' % ', '.join(ench_data.field('id_property_1', 'id_property_2', 'id_property_3')) ]
            fields += [ '{ %s }' % ', '.join(ench_data.field('coeff_1', 'coeff_2', 'coeff_3')) ]
            if hasattr(ench_data, '_spells'):
                fields += ench_data._spells[ 0 ].field('id')
            else:
                fields += self._spell_db[0].field('id')
            fields += ench_data.field('desc')
            s += '  { %s },\n' % (', '.join(fields))

        s += '};\n'

        return s

class RandomPropertyPointsGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'RandPropPoints' ]

        DataGenerator.__init__(self, options)

    def filter(self):
        ids = [ ]

        for ilevel, data in self._randproppoints_db.iteritems():
            if ilevel >= 1 and ilevel <= self._options.scale_ilevel:
                ids.append(ilevel)

        return ids

    def generate(self, ids = None):
        # Sort keys
        ids.sort()
        s = '#define %sRAND_PROP_POINTS%s_SIZE (%d)\n\n' % (
            (self._options.prefix and ('%s_' % self._options.prefix) or '').upper(),
            (self._options.suffix and ('_%s' % self._options.suffix) or '').upper(),
            len(ids)
        )
        s += '// Random property points for item levels 1-%d, wow build %d\n' % (
            self._options.scale_ilevel, self._options.build )
        s += 'static struct random_prop_data_t __%srand_prop_points%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' )

        for id in ids + [ 0 ]:
            rpp = self._randproppoints_db[id]

            fields = rpp.field('id')
            fields += [ '{ %s }' % ', '.join(rpp.field('epic_points_1', 'epic_points_2', 'epic_points_3', 'epic_points_4', 'epic_points_5')) ]
            fields += [ '{ %s }' % ', '.join(rpp.field('rare_points_1', 'rare_points_2', 'rare_points_3', 'rare_points_4', 'rare_points_5')) ]
            fields += [ '{ %s }' % ', '.join(rpp.field('uncm_points_1', 'uncm_points_2', 'uncm_points_3', 'uncm_points_4', 'uncm_points_5')) ]

            s += '  { %s },\n' % (', '.join(fields))

        s += '};\n'

        return s

class WeaponDamageDataGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'ItemDamageOneHand', 'ItemDamageOneHandCaster',
                      'ItemDamageRanged',  'ItemDamageThrown',        'ItemDamageWand',
                      'ItemDamageTwoHand', 'ItemDamageTwoHandCaster',  ]

        DataGenerator.__init__(self, options)

    def filter(self):

        return None

    def generate(self, ids = None):
        s = ''
        for dbname in self._dbc:
            db = getattr(self, '_%s_db' % dbname.lower() )
            
            s += '// Item damage data from %s.dbc, ilevels 1-%d, wow build %d\n' % ( 
                dbname, self._options.scale_ilevel, self._options.build )
            s += 'static struct item_scale_data_t __%s%s%s_data[] = {\n' % (
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                dbname.lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '' )

            for ilevel, data in db.iteritems():
                if ilevel > self._options.scale_ilevel:
                    continue

                fields = data.field('ilevel')
                fields += [ '{ %s }' % ', '.join(data.field('v_1', 'v_2', 'v_3', 'v_4', 'v_5', 'v_6', 'v_7')) ]

                s += '  { %s },\n' % (', '.join(fields))

            s += '  { %s }\n' % ( ', '.join([ '0' ] + [ '{ 0, 0, 0, 0, 0, 0, 0 }' ]) )
            s += '};\n\n'

        return s

class ArmorValueDataGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'ItemArmorQuality', 'ItemArmorShield', 'ItemArmorTotal' ]
        DataGenerator.__init__(self, options)

    def filter(self):

        return None

    def generate(self, ids = None):
        s = ''
        for dbname in self._dbc:
            db = getattr(self, '_%s_db' % dbname.lower() )

            s += '// Item armor values data from %s.dbc, ilevels 1-%d, wow build %d\n' % ( 
                dbname, self._options.scale_ilevel, self._options.build )
            
            s += 'static struct %s __%s%s%s_data[] = {\n' % (
                (dbname != 'ItemArmorTotal') and 'item_scale_data_t' or 'item_armor_type_data_t',
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                dbname.lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '' )

            for ilevel, data in db.iteritems():
                if ilevel > self._options.scale_ilevel:
                    continue

                fields = data.field('ilevel')
                if dbname != 'ItemArmorTotal':
                    fields += [ '{ %s }' % ', '.join(data.field('v_1', 'v_2', 'v_3', 'v_4', 'v_5', 'v_6', 'v_7')) ]
                else:
                    fields += [ '{ %s }' % ', '.join(data.field('v_1', 'v_2', 'v_3', 'v_4')) ]
                s += '  { %s },\n' % (', '.join(fields))

            if dbname != 'ItemArmorTotal':
                s += '  { %s }\n' % ( ', '.join([ '0' ] + [ '{ 0, 0, 0, 0, 0, 0, 0 }' ]) )
            else:
                s += '  { %s }\n' % ( ', '.join([ '0' ] + [ '{ 0, 0, 0, 0 }' ]) )
            s += '};\n\n'

        return s

class ArmorSlotDataGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'ArmorLocation' ]
        DataGenerator.__init__(self, options)

    def filter(self):
        return None

    def generate(self, ids = None):
        s = '// Inventory type based armor multipliers, wow build %d\n' % ( self._options.build )
        
        s += 'static struct item_armor_type_data_t __%sarmor_slot%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' )

        for inv_type in sorted(self._armorlocation_db.keys()):
            data = self._armorlocation_db[inv_type]
            fields = data.field('id')
            fields += [ '{ %s }' % ', '.join(data.field('v_1', 'v_2', 'v_3', 'v_4')) ]
            s += '  { %s },\n' % (', '.join(fields))

        s += '  { %s }\n' % ( ', '.join([ '0' ] + [ '{ 0, 0, 0, 0 }' ]) )
        s += '};\n\n'

        return s

class GemPropertyDataGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'GemProperties' ]
        DataGenerator.__init__(self, options)

    def filter(self):
        ids = []
        for id, data in self._gemproperties_db.iteritems():
            if not data.color or not data.id_enchant:
                continue
                
            ids.append(id)
        return ids

    def generate(self, ids = None):
        ids.sort()
        s = '// Gem properties, wow build %d\n' % ( self._options.build )
        
        s += 'static struct gem_property_data_t __%sgem_property%s_data[] = {\n' % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '' )

        for id in ids + [ 0 ]:
            data = self._gemproperties_db[id]
            if data.color == 0:
                continue;

            fields = data.field('id', 'id_enchant', 'color', 'min_ilevel')
            s += '  { %s },\n' % (', '.join(fields))

        s += '};\n\n'

        return s

class ItemBonusDataGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'ItemBonus', 'ItemBonusTreeNode', 'ItemXBonusTree' ]
        DataGenerator.__init__(self, options)

    def generate(self, ids):
        # Bonus trees
        
        data_str = "%sitem_bonus_tree%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        s = '#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._itembonustreenode_db.keys()) + 1)

        s += '// Item bonus trees, wow build %d\n' % ( self._options.build )

        s += 'static struct item_bonus_tree_entry_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper())

        for key in sorted(self._itembonustreenode_db.keys()) + [0,]:
            data = self._itembonustreenode_db[key]

            fields = data.field('id', 'id_tree', 'index', 'id_child', 'id_node')
            s += '  { %s },\n' % (', '.join(fields))

        s += '};\n\n'
        
        # Bonus definitions

        data_str = "%sitem_bonus%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        s += '#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._itembonus_db.keys()) + 1)
        s += '// Item bonuses, wow build %d\n' % ( self._options.build )

        s += 'static struct item_bonus_entry_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper())

        for key in sorted(self._itembonus_db.keys()) + [0,]:
            data = self._itembonus_db[key]
            fields = data.field('id', 'id_node', 'type', 'val1', 'val2', 'index')
            s += '  { %s },\n' % (', '.join(fields))

        s += '};\n\n'
        
        # Item bonuses (unsure as of yet if we need this, depends on how
        # Blizzard exports the bonus id to third parties)
        data_str = "%sitem_bonus_map%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )

        s += '#define %s_SIZE (%d)\n\n' % (data_str.upper(), len(self._itemxbonustree_db.keys()) + 1)
        s += '// Item bonus map, wow build %d\n' % ( self._options.build )

        s += 'static struct item_bonus_node_entry_t __%s_data[%s_SIZE] = {\n' % (data_str, data_str.upper())

        for key in sorted(self._itemxbonustree_db.keys()) + [0,]:
            data = self._itemxbonustree_db[key]
            fields = data.field('id', 'id_item', 'id_tree')
            s += '  { %s },\n' % (', '.join(fields))

        s += '};\n\n'
        
        return s
