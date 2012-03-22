import sys, os, re, types, HTMLParser, urllib2

import parser, db, data, constants

def _rune_cost(generator, filter_data, record, *args):
    cost = 0
    for rune_type in xrange(0, 3):
        for i in xrange(0, getattr(record, 'rune_cost_%d' % (rune_type + 1))):
            cost |= 1 << (rune_type * 2 + i)
    
    return args[0] % cost

class DataGenerator(object):
    _class_names = [ None, 'Warrior', 'Paladin', 'Hunter', 'Rogue',     'Priest', 'Death Knight', 'Shaman', 'Mage',  'Warlock', 'Monk',       'Druid'  ]
    _class_masks = [ None, 0x1,       0x2,       0x4,      0x8,         0x10,     0x20,           0x40,     0x80,    0x100,     0x200,        0x400   ]
    _race_names  = [ None, 'Human',   'Orc',     'Dwarf',  'Night Elf', 'Undead', 'Tauren',       'Gnome',  'Troll', 'Goblin',  'Blood Elf', 'Draenei' ] + [ None ] * 10 + [ 'Worgen', None ]
    _race_masks  = [ None, 0x1,       0x2,       0x4,      0x8,         0x10,     0x20,           0x40,     0x80,    0x100,     0x200,       0x400     ] + [ None ] * 10 + [ 0x200000, None ]
    _pet_names   = [ None, 'Ferocity', 'Tenacity', None, 'Cunning' ]
    _pet_masks   = [ None, 0x1,        0x2,        None, 0x4       ]

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

        return True

    def filter(self):
        return None

    def generate(self, ids = None):
        return ''

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

class CombatRatingsDataGenerator(DataGenerator):
    # From UIParent.lua, seems to match to gtCombatRatings too for lvl80 stuff
    _combat_ratings = [ 'Dodge',        'Parry',       'Block',       'Melee hit',  'Ranged hit', 
                        'Spell hit',    'Melee crit',  'Ranged crit', 'Spell crit', 'Melee haste', 
                        'Ranged haste', 'Spell haste', 'Expertise',   'Mastery' ]
    _combat_rating_ids = [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 17, 18, 19, 23, 25 ] 
    def __init__(self, options):
        # Hardcode these, as we need two different kinds of databases for output, using the same combat rating ids
        self._dbc = [ 'gtCombatRatings', 'gtOCTClassCombatRatingScalar' ]

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
            for k in xrange(m * 100, m * 100 + self._options.level, 5):
                s += '    %20.15f, %20.15f, %20.15f, %20.15f, %20.15f,\n' % (
                    db[k].gt_value, db[k + 1].gt_value, db[k + 2].gt_value,
                    db[k + 3].gt_value, db[k + 4].gt_value )

            s += '  },\n'
        
        s += '};\n\n'

        db = self._gtoctclasscombatratingscalar_db
        s += '// Combat Rating scalar multipliers for classes, wow build %d\n' % self._options.build
        s += 'static double __%s%s%s[][%d] = {\n' % ( 
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            re.sub(r'([A-Z]+)', r'_\1', self._dbc[1]).lower(), 
            self._options.suffix and ('_%s' % self._options.suffix) or '',
            len(self._class_names))
        for i in xrange(0, len(CombatRatingsDataGenerator._combat_rating_ids)):
            id = CombatRatingsDataGenerator._combat_rating_ids[i]
            s += '  // %s rating class scalar multipliers\n' % CombatRatingsDataGenerator._combat_ratings[i]
            s += '  { \n'
            s += '    %20.15f, %20.15f, %20.15f, %20.15f, %20.15f,\n' % (
                0.0, db[id * 10 + 1].gt_value, db[id * 10 + 2].gt_value, db[id * 10 + 3].gt_value,
                db[id * 10 + 4].gt_value)

            s += '    %20.15f, %20.15f, %20.15f, %20.15f, %20.15f,\n' % (
                db[id * 10 + 5].gt_value, db[id * 10 + 6].gt_value, db[id * 10 + 7].gt_value,
                db[id * 10 + 8].gt_value, db[id * 10 + 9].gt_value )

            s += '    %20.15f, %20.15f\n' % ( db[i * 10 + 10].gt_value, db[i * 10 + 11].gt_value )

            s += '  },\n'
        
        s += '};\n\n'

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
                for k in xrange((j - 1) * 100, (j - 1) * 100 + self._options.level, 5):
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
        
        for j in xrange(0, len(self._class_names) + 1):
            # Last entry is the fixed data
            if j < len(self._class_names) and self._class_names[j] != None:
                s += '  // %s\n' % self._class_names[j]
            elif j == len(self._class_names):
                s += '  // Constant scaling\n'
        
            s += '  {\n'
            for k in xrange((j - 1) * 100, (j - 1) * 100 + self._options.level, 5):
                s += '    %20.15f, %20.15f, %20.15f, %20.15f, %20.15f,\n' % (
                    db[k].gt_value, db[k + 1].gt_value, db[k + 2].gt_value, 
                    db[k + 3].gt_value, db[k + 4].gt_value
                )
            s += '  },\n'
        
        s += '};\n\n'

        return s

class TalentDataGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'Spell', 'Talent', 'TalentTab' ]

        DataGenerator.__init__(self, options)

    def filter(self):
        ids = [ ]

        for talent_id, talent_data in self._talent_db.iteritems():
            talent_tab = self._talenttab_db[talent_data.talent_tab]
            if not talent_tab.id:
                continue

            # Make sure the talent is a class  / pet associated one
            if talent_tab.mask_class      not in DataGenerator._class_masks and \
               talent_tab.mask_pet_talent not in DataGenerator._pet_masks:
                continue

            # Make sure at least one spell id is defined
            if talent_data.id_rank_1 == 0: 
                continue

            # Make sure the "base spell" exists
            if not self._spell_db.get(talent_data.id_rank_1):
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
            talent_tab = self._talenttab_db[talent.talent_tab]
            spell      = self._spell_db[talent.id_rank_1]
            if not spell.id and talent.id_rank_1 > 0:
                continue
				
            if( index % 20 == 0 ):
                s += '//{ Name                                ,    Id, Flgs,Tab, Class, Pet,Depend,DR ,Col,Row, { Rank1, Rank2, Rank3 },S1,S2,S3 },\n'

            fields = spell.field('name')
            fields += talent.field('id')
            fields += [ '%#.2x' % 0 ]
            fields += talent_tab.field('tab_page', 'mask_class', 'mask_pet_talent')
            fields += talent.field('talent_depend', 'depend_rank', 'col', 'row')
            fields += [ '{ %s }' % ', '.join(talent.field('id_rank_1', 'id_rank_2', 'id_rank_3')) ]
            # Pad struct with empty pointers for direct rank based spell data access
            fields += [ '0', '0', '0' ]
        
            s += '  { %s },\n' % (', '.join(fields))
			
            index += 1

        s += '};'

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
        "^Scroll of Enchant",
        "^Enchant ",
        "Deprecated",
        "DEPRECATED",
        "QA",
        "zzOLD",
        "NYI",
    ]
    
    def __init__(self, options):
        self._dbc = [ 'Item-sparse', 'Item', 'ItemDisplayInfo', 'SpellEffect', 'Spell' ]

        DataGenerator.__init__(self, options)

    def initialize(self):
        DataGenerator.initialize(self)
        
        # Go through a local client cache file if provided, overriding any data 
        # found in Item-sparse.db2/Item.db2 client files
        if self._options.item_cache_dir != '': 
            for i in [ 'Item-sparse.adb', 'Item.adb' ]:
                if not os.access(os.path.abspath(os.path.join(self._options.item_cache_dir, i)), os.R_OK):
                    continue
                
                dbcname = i[0:i.find('.')].replace('-', '_').lower()
                setattr(self, '_%s_cache' % dbcname, 
                    parser.DBCParser(self._options, os.path.abspath(os.path.join(self._options.item_cache_dir, i))))
                dbc = getattr(self, '_%s_cache' % dbcname)

                if not dbc.open_dbc():
                    return False

                if '_%s_db' % dbc.name() not in dir(self):
                    setattr(self, '_%s_db' % dbc.name(), db.DBCDB(dbc._class))

                dbase = getattr(self, '_%s_db' % dbc.name())
                record = dbc.next_record()
                while record != None:
                    dbase[record.id] = record
                    record = dbc.next_record()
                
        # Map Spell effects to Spell IDs so we can do filtering based on them
        for spell_effect_id, spell_effect_data in self._spelleffect_db.iteritems():
            if not spell_effect_data.id_spell:
                continue

            spell = self._spell_db[spell_effect_data.id_spell]
            if not spell.id:
                continue

            spell.add_effect(spell_effect_data)
        
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
            
            # Gems, Glyphs, Shirts, Tabards and Permanent item enchants are also included to avoid extra internet fetches
            # Quest items are also checked for enchant item information, as apparently the DBC 
            # files handle the various head- and shoulder enchants as class 12 (quest items?)
            if ( classdata.classs != 3 or data.ilevel < 81 or data.gem_props == 0 ) and \
               ( classdata.classs != 0 or classdata.subclass != 6 or data.ilevel < 80 ) and \
               ( classdata.classs != 7 or classdata.subclass != 3 or data.ilevel < 80 ) and \
               ( classdata.classs != 16 ) and \
               ( classdata.classs != 12 or data.ilevel < 80 ) and \
               ( data.inv_type != 19 ) and \
               ( data.inv_type != 4 ) and \
               ( data.ilevel < self._options.min_ilevel or data.ilevel > self._options.max_ilevel or data.inv_type == 0 or data.flags & 0x10 ):
                continue
            
            # Grab various permanent item enchants from item class 12
            if classdata.classs == 12:
                for i in xrange(1, 6):
                    spell_id = getattr(data, 'id_spell_%d' % i)
                    enchant_spell_id = 0
                    if spell_id > 0:
                        spell = self._spell_db[spell_id]
                        for effect in spell._effects:
                            if not effect or effect.type != 53:
                                continue
                            
                            enchant_spell_id = spell_id
                            break
                    
                    if enchant_spell_id > 0:
                        ids.append(item_id)
                        break
            else:
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
            item_display = self._itemdisplayinfo_db[item2.id_display]
            
            if not item.id and id > 0:
                sys.stderr.write('Item id %d not found\n' % id) 
                continue

            # Aand, hack classs 12 (quest item) and tradegood/devices (7, 3) to be 0, 6 
            # so we get item enchants clumped in the same category, sigh ..
            if item2.classs == 12 or ( item2.classs == 7 and item2.subclass == 3 ):
                item2.classs = 0
                item2.subclass = 6

            if(index % 20 == 0):
			    s += '//{    Id, Name                                                   , Icon                                    ,     Flags1,     Flags2,Level,ReqL,ReqSk, RSkL,Qua,Inv,Cla,SCl,Bnd, Delay, DmgRange, Modifier,  ClassMask,   RaceMask, { ST1, ST2, ST3, ST4, ST5, ST6, ST7, ST8, ST9, ST10}, {  SV1,  SV2,  SV3,  SV4,  SV5,  SV6,  SV7,  SV8,  SV9, SV10 }, {  SId1,  SId2,  SId3,  SId4,  SId5 }, {TId1,TId2,TId3,TId4,TId5 }, {    CdS1,    CdS2,    CdS3,    CdS4,    CdS5 }, { CdCat1, CdCat2, CdCat3, CdCat4, CdCat5 }, {Soc1,Soc2,Soc3 }, GemP,IdSBon,IdSet,IdSuf },\n'

            fields = item.field('id', 'name')
            fields += item_display.field('icon')
            fields += item.field('flags', 'flags_2', 'ilevel', 'req_level', 'req_skill', 'req_skill_rank', 'quality', 'inv_type')
            fields += item2.field('classs', 'subclass')
            fields += item.field( 'bonding', 'delay', 'weapon_damage_range', 'item_damage_modifier', 'race_mask', 'class_mask') 
            fields += [ '{ %s }' % ', '.join(item.field('stat_type_1', 'stat_type_2', 'stat_type_3', 'stat_type_4', 'stat_type_5', 'stat_type_6', 'stat_type_7', 'stat_type_8', 'stat_type_9', 'stat_type_10')) ]
            fields += [ '{ %s }' % ', '.join(item.field('stat_val_1', 'stat_val_2', 'stat_val_3', 'stat_val_4', 'stat_val_5', 'stat_val_6', 'stat_val_7', 'stat_val_8', 'stat_val_9', 'stat_val_10')) ]
            fields += [ '{ %s }' % ', '.join(item.field('id_spell_1', 'id_spell_2', 'id_spell_3', 'id_spell_4', 'id_spell_5')) ]
            fields += [ '{ %s }' % ', '.join(item.field('trg_spell_1', 'trg_spell_2', 'trg_spell_3', 'trg_spell_4', 'trg_spell_5')) ]
            fields += [ '{ %s }' % ', '.join(item.field('cd_spell_1', 'cd_spell_2', 'cd_spell_3', 'cd_spell_4', 'cd_spell_5')) ]
            fields += [ '{ %s }' % ', '.join(item.field('cdc_spell_1', 'cdc_spell_2', 'cdc_spell_3', 'cdc_spell_4', 'cdc_spell_5')) ]
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
    _spell_ref_rx = r'\$(?:\?[A-z])?([0-9]+)(?:\[|[A-z][0-9]?)'
    
    # Explicitly included list of spells per class, that cannot be 
    # found from talents, or based on a SkillLine category
    # The list contains a tuple ( spell_id, category ), 
    # where category is an entry in the _class_categories class-specific
    # tuples, e.g. general = 0, talent_tab0..2 = 1..3 and pets as a whole
    # are 4. General spells do not appear in class activated lists, even if
    # they pass the "activated" check
    # The first tuple in the list is for non-class related, generic spells that are whitelisted,
    # without a category
    _spell_id_list = [
        (
         109871, 109869, 109867,    # No'Kaled the Elements of Death - LFR
         107785, 107789, 107787,    # No'Kaled the Elements of Death - Normal
         109872, 109870, 109868,    # No'Kaled the Elements of Death - Heroic
          52586,  68043,  68044,    # Gurthalak, Voice of the Deeps - LFR, N, H
        ),
        (), 
        ( ( 54158, 0 ), ( 90174, 0 ), ( 99092, 0 ), ( 99075, 0 ), ),     # Paladin "Judgement" damage portion on some Seals. Melee Tier 12 2pc effect. Prot T12 2pc spell.
        ( ( 99058, 0 ), ( 83381, 0 ), ), # Hunter T12 2pc, Kill Command base dmg
        ( ( 8680, 1 ), ( 2818, 1 ), ( 13218, 1 ), ( 99173, 0 ), ( 99186, 0 ), ( 99187, 0 ), ( 99188, 0 ), ), # Rogue poison effects, Tier 12 bonus spells
        ( ( 63619, 4 ), ( 95740, 0 ), ( 93683, 0), ( 95799, 0), ( 94472, 0 ), ( 99156, 4 ), ),     # Priest shadowfiend "Shadowcrawl", new Shadow Orbs, Atonement Crit, sfiend tier12 bonus
        ( ( 50401, 0 ), ( 70890, 0 ), ( 51963, 4 ), ( 99000, 0 ), ), # DK Razorice runeforge, weird Scourge Strike secondary effect, gargoyle strike. Tier 12 4pc spell.
        ( ( 12470, 4 ), ( 13376, 4 ), ( 57984, 4 ) ),   # Shaman Greater Fire Elemental abilities
        ( 
            ( 5405, 0 ),  ( 92283, 3 ), ( 84721, 0 ), ( 79058, 0 ), # Mana Gem, Frostfire Orb x2, Arcane Brilliance
            ( 88084, 4 ), ( 59637, 4 ), ( 88082, 4 ), ( 59638, 4 ), ( 99062, 4 ), # Mirror Image spells. incl T12 Fireball
            ( 80354, 0 ),                                           # Temporal Displacement
        ), 
        ( ( 85692, 4 ), ( 99226, 4 ), ),     # Warlock doomguard "Doom Bolt", Tier 12 bonus spell
        (),
        ( ( 81070, 0 ), ( 99002, 0 ), ( 99026, 4 ) ),     # Euphoria mana feed for Balance droods, Tier 12 2pc Feral Fiery Claws. Tier 12 2pc Balance attack.
    ]

    # Class specific item sets, T11, T12, T13
    _item_set_list = [
        (),
        ( ( 942, 943 ), ( 1017, 1018 ), ( 1073, 1074, ), ), # Warrior
        ( ( 932, 933, 934 ), ( 1011, 1012, 1013 ), ( 1063, 1065, 1064, ), ), #Paladin
        ( ( 930, ), ( 1005, ), ( 1061, ), ), # Hunter
        ( ( 937, ), ( 1006, ), ( 1068, ), ), # Rogue
        ( ( 936, 935 ), ( 1009, 1010 ), ( 1066, 1067, ), ), #Priest
        ( ( 925, 926 ), ( 1000, 1001, ), ( 1056, 1057, ), ), # Death Knight
        ( ( 938, 939, 940 ), ( 1014, 1015, 1016 ), ( 1070, 1071, 1069, ), ), #Shaman
        ( ( 931, ), ( 1007, ), ( 1062, ), ), # Mage
        ( ( 941, ), ( 1008, ), ( 1072, ), ), # Warlock
        (),
        ( ( 927, 928, 929 ), ( 1002, 1003, 1004 ), ( 1059, 1058, 1060 ), ), #Druid
    ]
  
    _profession_enchant_categories = [
        165,  # Leatherworking
        197,  # Tailoring
        202,  # Engineering
        333,  # Enchanting
        773,  # Inscription
    ]
    
    # General, TalentTab0, TalentTab1, TalentTab2, PetCategories...
    _class_categories = [
        ( ),
        ( 803,  26, 256, 257 ),         # Warrior
        ( 800, 594, 267, 184 ),         # Paladin
        ( 795,  50, 163,  51, ( 653, 210, 818, 655, 211, 213, 209, 214, 212, 811, 763, 780, 787, 781, 786, 783, 788, 808, 270, 215, 654, 815, 775, 764, 217, 767, 236, 768, 817, 203, 765, 218, 251, 766, 656, 208, 785, 784 ) ),       # Hunter
        ( 797, 253,  38,  39 ),         # Rogue
        ( 804, 613,  56,  78 ),         # Priest
        ( 796, 770, 771, 772, 782 ),    # Death Knight
        ( 801, 375, 373, 374 ),         # Shaman
        ( 799, 237,   8,   6, 805 ),    # Mage
        ( 802, 355, 354, 593, ( 207, 761, 206, 204, 205, 189, 188 ) ),  # Warlock
        ( ), 
        ( 798, 574, 134, 573 ),         # Druid
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
        31,     # SPELL_AURA_MOD_INCREASE_SPEED
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
        "^Mastery",
        "^Tamed Pet Passive",
    ]
    
    def __init__(self, options):
        self._dbc = [
            'Spell', 'SpellEffect', 'SpellScaling', 'SpellCooldowns', 'SpellRange', 'SpellClassOptions',
            'SpellDuration', 'SpellPower', 'SpellLevels', 'SpellCategories', 'Talent', 'TalentTab',
            'SkillLineAbility', 'SpellAuraOptions', 'SpellRuneCost', 'SpellRadius', 'GlyphProperties',
            'SpellCastTimes', 'ItemSet', 'SpellDescriptionVariables', 'SpellItemEnchantment', 'Item-sparse',
            'Item', 'SpellEquippedItems', 'SpellIcon', 'SpellEffectScaling' ]

        DataGenerator.__init__(self, options)

    def initialize(self):
        DataGenerator.initialize(self)

        # Go through a local client cache file if provided, overriding any data 
        # found in Item-sparse.db2 client file
        if self._options.item_cache_dir != '': 
            for i in [ 'Item-sparse.adb' ]:
                if not os.access(os.path.abspath(os.path.join(self._options.item_cache_dir, i)), os.R_OK ):
                    continue

                dbcname = i[0:i.find('.')].replace('-', '_').lower()
                setattr(self, '_%s_cache' % dbcname, 
                        parser.DBCParser(self._options, os.path.abspath(os.path.join(self._options.item_cache_dir, i))))
                dbc = getattr(self, '_%s_cache' % dbcname)
          
                if not dbc.open_dbc():
                    return False
          
                if '_%s_db' % dbc.name() not in dir(self):
                    setattr(self, '_%s_db' % dbc.name(), db.DBCDB(dbc._class))
          
                dbase = getattr(self, '_%s_db' % dbc.name())
                record = dbc.next_record()
                while record != None:
                    dbase[record.id] = record
                    record = dbc.next_record()

        # Map Spell effects to Spell IDs so we can do filtering based on them
        for spell_effect_id, spell_effect_data in self._spelleffect_db.iteritems():
            if not spell_effect_data.id_spell:
                continue

            spell = self._spell_db[spell_effect_data.id_spell]
            if not spell.id:
                continue

            spell.add_effect(spell_effect_data)
        
        return True

    def spell_state(self, spell):
        enabled_effects = [ True, True, True ]

        # Check for blacklisted spells
        if spell.id in SpellDataGenerator._spell_blacklist:
            return False

        # Check for spell name blacklist
        for p in SpellDataGenerator._spell_name_blacklist:
            if spell.name and re.search(p, spell.name):
                return False

        # Check for blacklisted spell category mechanism
        if spell.id_categories > 0:
            c = self._spellcategories_db[spell.id_categories]
            if c.mechanic in SpellDataGenerator._mechanic_blacklist:
                return False

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
            return False

        return True
    
    def generate_spell_filter_list(self, spell_id, mask_class, mask_pet, mask_race, filter_list = { }):
        spell = self._spell_db[spell_id]
        enabled_effects = [ True, True, True ]

        if not spell.id:
            return None
            
        if not self.spell_state(spell):
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

                lst = self.generate_spell_filter_list(effect.trigger_spell, mask_class, mask_pet, mask_race, filter_list)
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
        spell_refs = list(set(spell_refs))

        for ref_spell_id in spell_refs:
            if int(ref_spell_id) == spell.id:
                continue
            
            if int(ref_spell_id) in filter_list.keys():
                continue
            
            lst = self.generate_spell_filter_list(int(ref_spell_id), mask_class, mask_pet, mask_race, filter_list)
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
            if self._options.build < 15464:
                talent_tab = self._talenttab_db[talent_data.talent_tab]
                if not talent_tab.id:
                    continue

                # Make sure the talent is a class  / pet associated one
                if talent_tab.mask_class      not in DataGenerator._class_masks and \
                   talent_tab.mask_pet_talent not in DataGenerator._pet_masks:
                    continue

                # Get all talents that have spell ranks associated with them
                for rank in xrange(1, 4):
                    filter_list = { }
                    id = getattr(talent_data, 'id_rank_%d' % rank)
                    if id:
                        lst = self.generate_spell_filter_list(id, talent_tab.mask_class, talent_tab.mask_pet_talent, 0, filter_list)
                        if not lst:
                            continue
                        
                        for k, v in lst.iteritems():
                            if ids.get(k):
                                ids[k]['mask_class'] |= v['mask_class']
                                ids[k]['mask_race'] |= v['mask_race']
                            else:
                                ids[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }
            else:
                filter_list = { }
                id = getattr(talent_data, 'id_rank_1')
                if id:
                    lst = self.generate_spell_filter_list(id, DataGenerator._class_masks[talent_data['class']], 0, 0, filter_list)
                    if not lst:
                        continue
                    
                    for k, v in lst.iteritems():
                        if ids.get(k):
                            ids[k]['mask_class'] |= v['mask_class']
                            ids[k]['mask_race'] |= v['mask_race']
                        else:
                            ids[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }

        # Get base skills from SkillLineAbility
        for ability_id, ability_data in self._skilllineability_db.iteritems():
            mask_class_category = 0
            mask_race_category  = 0
            if ability_data.max_value > 0 or ability_data.min_value > 0:
                continue

            if ability_data.id_skill in SpellDataGenerator._skill_category_blacklist:
                continue

            # Guess class based on skill category identifier
            for j in xrange(0, len(SpellDataGenerator._class_categories)):
                for k in xrange(0, len(SpellDataGenerator._class_categories[j])):
                    if ( isinstance(SpellDataGenerator._class_categories[j][k], tuple) and \
                         ability_data.id_skill in SpellDataGenerator._class_categories[j][k] ) or \
                         ability_data.id_skill == SpellDataGenerator._class_categories[j][k]:
                        mask_class_category = DataGenerator._class_masks[j]
                        break

            # Guess race based on skill category identifier
            for j in xrange(0, len(SpellDataGenerator._race_categories)):
                if ability_data.id_skill in SpellDataGenerator._race_categories[j]:
                    mask_race_category = DataGenerator._race_masks[j]
                    break

            # Make sure there's a class or a race for an ability we are using
            if not ability_data.mask_class and not ability_data.mask_race and not mask_class_category and not mask_race_category:
                continue
                
            spell = self._spell_db[ability_data.id_spell]
            if not spell.id:
                continue

            filter_list = { }
            lst = self.generate_spell_filter_list(spell.id, ability_data.mask_class or mask_class_category, 0, ability_data.mask_race or mask_race_category, filter_list)
            if not lst:
                continue
                
            for k, v in lst.iteritems():
                if ids.get(k):
                    ids[k]['mask_class'] |= v['mask_class']
                    ids[k]['mask_race'] |= v['mask_race']
                else:
                    ids[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }

        # Get spells relating to item enchants, so we can populate a (nice?) list
        for ability_id, ability_data in self._skilllineability_db.iteritems():
            if ( ability_data.id_skill not in self._profession_enchant_categories ):
                continue;
                        
            spell = self._spell_db[ability_data.id_spell]
            if not spell.id:
                continue

            enchant_spell_id = 0
            for effect in spell._effects:
                # Grab Enchant Items and Create Items (create item will be filtered further )
                if not effect or (effect.type != 53 and effect.type != 24):
                    continue

                # Create Item, see if the created item has a spell that enchants an item, if so
                # add the enchant spell
                if effect.type == 24:
                    item = self._item_sparse_db[effect.item_type]
                    if not item.id or item.ilevel < 80:
                        continue

                    for i in xrange(1, 6):
                        id_spell = getattr(item, 'id_spell_%d' % i)
                        enchant_spell = self._spell_db[id_spell]
                        for enchant_effect in enchant_spell._effects:
                            if not enchant_effect or enchant_effect.type != 53:
                                continue

                            enchant_spell_id = id_spell
                            break

                        if enchant_spell_id > 0:
                            break
                # Skip pre-wotlk enchants
                elif effect.type == 53:
                    spell_item_ench = self._spellitemenchantment_db[effect.misc_value]
                    if (spell_item_ench.req_skill == 0 and self._spelllevels_db[spell.id_levels].base_level < 60) or \
                       (spell_item_ench.req_skill > 0 and spell_item_ench.req_skill_value <= 375):
                        continue
                    enchant_spell_id = spell.id
                
                if enchant_spell_id > 0:
                    break

            if enchant_spell_id > 0:
                filter_list = { }
                lst = self.generate_spell_filter_list(enchant_spell_id, 0, 0, 0, filter_list)
                if not lst:
                    continue
                    
                for k, v in lst.iteritems():
                    if ids.get(k):
                        ids[k]['mask_class'] |= v['mask_class']
                        ids[k]['mask_race'] |= v['mask_race']
                    else:
                        ids[k] = { 
                            'mask_class': v['mask_class'], 
                            'mask_race' : v['mask_race'], 
                            'effect_list': v['effect_list'],
                        }

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
            
            # Quest items, Permanent Item enchants
            if ( classdata.classs != 0 or classdata.subclass != 6 or data.ilevel < 80 ) and \
               ( classdata.classs != 12 or data.ilevel < 80 ):
                continue
            
            # Grab various permanent item enchants from item class 12
            enchant_spell_id = 0
            if classdata.classs == 12:
                for i in xrange(1, 6):
                    spell_id = getattr(data, 'id_spell_%d' % i)
                    if spell_id > 0:
                        spell = self._spell_db[spell_id]
                        for effect in spell._effects:
                            if not effect or effect.type != 53:
                                continue
                            
                            enchant_spell_id = spell_id
                            break
                    
                    if enchant_spell_id > 0:
                        break
            
            if enchant_spell_id > 0:
                filter_list = { }
                lst = self.generate_spell_filter_list(enchant_spell_id, 0, 0, 0, filter_list)
                if not lst:
                    continue
                    
                for k, v in lst.iteritems():
                    if ids.get(k):
                        ids[k]['mask_class'] |= v['mask_class']
                        ids[k]['mask_race'] |= v['mask_race']
                    else:
                        ids[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }
        
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

            # Item set is a tier set, we want informations.
            for id_spell_field in xrange(1, 9):
                spell_id = getattr(itemset_data, 'id_spell_%d' % id_spell_field)
                if spell_id:
                    #print itemset_data.name, id_spell_field, spell_id
                    filter_data = { }
                    lst = self.generate_spell_filter_list(spell_id, mask_class_category, 0, 0, filter_list)
                    if not lst:
                        continue
                        
                    for k, v in lst.iteritems():
                        if ids.get(k):
                            ids[k]['mask_class'] |= v['mask_class']
                            ids[k]['mask_race'] |= v['mask_race']
                        else:
                            ids[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }


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

                # Finally, generate a spell filter list out of the glyph's id spell
                filter_list = { }
                lst = self.generate_spell_filter_list(glyph_data.id_spell, ability_data.mask_class, 0, 0, filter_list)
                if not lst:
                    continue

                for k, v in lst.iteritems():
                    if ids.get(k):
                        ids[k]['mask_class'] |= v['mask_class']
                        ids[k]['mask_race'] |= v['mask_race']
                    else:
                        ids[k] = { 
                            'mask_class' : v['mask_class'], 
                            'mask_race'  : v['mask_race'], 
                            'effect_list': v['effect_list'] 
                        }

        # Item enchantments that use a spell
        for eid, data in self._spellitemenchantment_db.iteritems():
            filter_list = { }
            for attr_id in xrange(1, 4):
                attr_type = getattr(data, 'type_%d' % attr_id)
                if attr_type == 1 or attr_type == 3 or attr_type == 7:
                    sid = getattr(data, 'id_property_%d' % attr_id)
                    lst = self.generate_spell_filter_list(sid, 0, 0, 0, filter_list)
                    if not lst:
                        continue
                    
                    for k, v in lst.iteritems():
                        ids[k] = { 
                            'mask_class' : 0,
                            'mask_race'  : 0,
                            'effect_list': v['effect_list'] 
                        }
        
        # Items with a spell identifier as "stats"
        for iid, data in self._item_sparse_db.iteritems():
            if data.ilevel < self._options.min_ilevel or data.ilevel > self._options.max_ilevel:
                continue
            
            filter_list = { }
            for stat_id in xrange(1, 6):
                sid = getattr(data, 'id_spell_%d' % stat_id)
                if sid > 0:
                    lst = self.generate_spell_filter_list(sid, 0, 0, 0, filter_list)
                    if not lst:
                        continue
                    
                    for k, v in lst.iteritems():
                        ids[k] = { 
                            'mask_class' : 0,
                            'mask_race'  : 0,
                            'effect_list': v['effect_list']
                        }
        
        # Last, get the explicitly defined spells in _spell_id_list on a class basis and the 
        # generic spells from SpellDataGenerator._spell_id_list[0]
        for generic_spell_id in SpellDataGenerator._spell_id_list[0]:
            filter_list = { }
            lst = self.generate_spell_filter_list(generic_spell_id, 0, 0, 0, filter_list)
            if not lst:
                continue
            
            for k, v in lst.iteritems():
                if ids.get(k):
                    ids[k]['mask_class'] |= v['mask_class']
                    ids[k]['mask_race'] |= v['mask_race']
                else:
                    ids[k] = { 
                        'mask_class': v['mask_class'], 
                        'mask_race' : v['mask_race'], 
                        'effect_list': v['effect_list'] 
                }

        for cls in xrange(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls]:
                filter_list = { }
                lst = self.generate_spell_filter_list(spell_tuple[0], self._class_masks[cls], 0, 0, filter_list)
                if not lst:
                    continue
                    
                for k, v in lst.iteritems():
                    if ids.get(k):
                        ids[k]['mask_class'] |= v['mask_class']
                        ids[k]['mask_race'] |= v['mask_race']
                    else:
                        ids[k] = { 
                            'mask_class': v['mask_class'], 
                            'mask_race' : v['mask_race'], 
                            'effect_list': v['effect_list'] 
                        }
                    
        return ids

    def generate(self, ids = None):
        # Sort keys
        id_keys = ids.keys()
        id_keys.sort()
        effects = set()

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

            if(index % 20 == 0):
              s += '//{ Name                                ,    Id,Flags,PrjSp,  Sch, PT, Class,  Race,Sca,ExtraCoeff,SpLv,MxL,MinRange,MaxRange,Cooldown,  GCD,  Cat,  Duration,Cost, RCost, RPG,Stac, PCh,PCr,EqpCl, EqpInvType,EqpSubclass,CastMn,CastMx,Div,       Scaling,SLv, {   Ef1,   Ef2,   Ef3 }, {      Attr1,      Attr2,      Attr3,      Attr4,      Attr5,      Attr6,      Attr7,      Attr8,      Attr9,     Attr10 }, Description, Tooltip, Description Variable, Icon, Effect1, Effect2, Effect3 },\n'
				
            fields = spell.field('name', 'id') 
            fields += [ '%#.2x' % 0 ]
            fields += spell.field('prj_speed', 'mask_school', 'type_power')

            # Hack in the combined class from the id_tuples dict
            fields += [ '%#.3x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_class'] ]
            fields += [ '%#.3x' % ids.get(id, { 'mask_class' : 0, 'mask_race': 0 })['mask_race'] ]

            # Set the scaling index for the spell
            fields += self._spellscaling_db[spell.id_scaling].field('id_class')

            fields += spell.field('extra_coeff')

            fields += self._spelllevels_db[spell.id_levels].field('base_level', 'max_level')
            if self._options.build >= 12694 and self._options.build < 12942:
                range = self._spellrange_db[spell.id_range]
                if range.id_range > 0:
                    range = self._spellrange_db[range.id_range]
                
                fields += range.field('max_range')
            else:
                fields += self._spellrange_db[spell.id_range].field('min_range')
            fields += self._spellrange_db[spell.id_range].field('max_range')
            fields += self._spellcooldowns_db[spell.id_cooldowns].field('cooldown_duration', 'gcd_cooldown')
            fields += self._spellcategories_db[spell.id_categories].field('category')
            fields += self._spellduration_db[spell.id_duration].field('duration_1')
            fields += self._spellpower_db[spell.id_power].field('power_cost')
            fields += _rune_cost(self, None, self._spellrunecost_db[spell.id_rune_cost], '%#.3x'),
            fields += self._spellrunecost_db[spell.id_rune_cost].field('rune_power_gain')
            fields += self._spellauraoptions_db[spell.id_aura_opt].field(
                'stack_amount', 'proc_chance', 'proc_charges'
            )
            
            fields += self._spellequippeditems_db[spell.id_equip_items].field('item_class', 'mask_inv_type', 'mask_sub_class')
            
            if spell.id_scaling:
                fields += self._spellscaling_db[spell.id_scaling].field('cast_min', 'cast_max', 'cast_div')
                fields += self._spellscaling_db[spell.id_scaling].field('c_scaling', 'c_scaling_threshold')
            else:
                fields += self._spellcasttimes_db[spell.id_cast_time].field('min_cast_time', 'cast_time')
                fields += [ ' 0', '%13.10f' % 0, ' 0' ]

            s_effect = []
            for effect in spell._effects:
                if not effect or not ids.get(id, { 'effect_list': [ True, True, True ] })['effect_list'][effect.index]:
                    s_effect += data.SpellEffect.default().field('id')
                else:
                    s_effect += effect.field('id')
                    effects.add( ( effect.id, spell.id_scaling ) )

            fields += [ '{ %s }' % ', '.join(s_effect) ]
            # Add spell flags
            fields += [ '{ %s }' % ', '.join(spell.field('flags', 'flags_1', 'flags_2', 'flags_3', 'flags_4', 'flags_5', 'flags_6', 'flags_7', 'flags_12694', 'flags_8')) ]
            fields += spell.field('desc', 'tt')
            if spell.id_desc_var and self._spelldescriptionvariables_db.get(spell.id_desc_var):
                fields += self._spelldescriptionvariables_db[spell.id_desc_var].field('var')
            else:
                fields += [ '0' ]
            if spell.id_icon and self._spellicon_db.get(spell.id_icon):
                fields += self._spellicon_db[spell.id_icon].field('name')
            else:
                fields += [ '0' ]
            # Pad struct with empty pointers for direct access to spell effect data
            fields += [ '0', '0', '0' ]
            s += '  { %s },\n' % (', '.join(fields))
			
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

        #stm = set()
        # Move spell scaling data and spell effect data to other struct
        index = 0
        for effect_data in sorted(effects) + [ ( 0, 0 ) ]:
            effect = self._spelleffect_db[effect_data[0]]
            if not effect.id and effect_data[ 0 ] > 0:
                sys.stderr.write('Spell Effect id %d not found\n') % effect_data[0]
                continue

            if(index % 20 == 0):
                s += '//{    Id,Flags,  SpId,Ix, EffectType                  , EffectSubType                              ,       Average,         Delta,       Unknown,   Coefficient,  Ampl,  Radius,  RadMax,   BaseV,   MiscV,  MiscV2, Trigg,   DmgMul,  CboP, RealP,Die, 0, 0 },\n'
				
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
            fields += self._spellscaling_db[effect_data[1]].field(
                'e%d_average' % (effect.index + 1), 'e%d_delta' % (effect.index + 1), 'e%d_bcp' % (effect.index + 1))
            fields += effect.field('coefficient', 'amplitude')
            fields += self._spellradius_db[effect.id_radius].field('radius_1')
            fields += self._spellradius_db[effect.id_radius_max].field('radius_1')
            fields += effect.field('base_value', 'misc_value', 'misc_value_2', 'trigger_spell', 'dmg_multiplier', 'points_per_combo_points', 'real_ppl', 'die_sides')
            # Pad struct with empty pointers for direct spell data access
            fields += [ '0', '0' ]

            s += '  { %s },\n' % (', '.join(fields))

            index += 1

        s += '};\n\n'

        return s

class MasteryAbilityGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'Spell', 'SkillLineAbility' ]

        DataGenerator.__init__(self, options)
        
    def filter(self):
        ids = {}
        for k, v in self._skilllineability_db.iteritems():
            s = self._spell_db[v.id_spell]
            if s.flags_12694 & 0x20000000:
                ids[v.id_spell] = { 'mask_class' : v.mask_class, 'category' : v.id_skill }
                
        return ids

    def generate(self, ids = None):
        max_ids = 0
        mastery_class = 0
        keys    = [ [], [], [], [], [], [], [], [], [], [], [], [] ]
        
        for k, v in ids.iteritems():
            if v['mask_class'] == 0:
                for cls in xrange(0, len(SpellDataGenerator._class_categories)):
                    if v['category'] in SpellDataGenerator._class_categories[cls]:
                        mastery_class = cls
                        break
            else:
                mastery_class = self._class_map[v['mask_class']]
            
            keys[mastery_class].append( ( self._spell_db[k].name, k ) )
            
        # Find out the maximum size of a key array
        for i in keys:
            if len(i) > max_ids:
                max_ids = len(i)

        data_str = "%sclass_mastery_ability%s" % (
            self._options.prefix and ('%s_' % self._options.prefix) or '',
            self._options.suffix and ('_%s' % self._options.suffix) or '',
        )
        
        s = '#define %s_SIZE (%d)\n\n' % (
            data_str.upper(),
            max_ids
        )
        s += '// Class mastery abilities, wow build %d\n' % self._options.build
        s += 'static unsigned __%s_data[][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        )
        
        for k in xrange(0, len(keys)):
            if SpellDataGenerator._class_names[k]:
                s += '  // Class mastery abilities for %s\n' % ( SpellDataGenerator._class_names[k] )
                
            s += '  {\n'
            for ability in sorted(keys[k], key = lambda i: i[0]):
                s += '    %5d, // %s\n' % ( ability[1], ability[0] )
                
            if len(keys[k]) < max_ids:
                s += '    %5d,\n' % 0
                
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
        s = '#define %s_SIZE (%d)\n#ifndef CLASS_SIZE\n#define CLASS_SIZE (12)\n#endif\n\n' % (
            data_str.upper(),
            max_ids
        )
        s += '// Racial abilities, wow build %d\n' % self._options.build
        s += 'static unsigned __%s_data[][CLASS_SIZE][%s_SIZE] = {\n' % (
            data_str,
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

class TalentSpecializationGenerator(DataGenerator):
    def __init__(self, options):
        self._dbc = [ 'Spell', 'Talent', 'TalentTab', 'TalentTreePrimarySpells' ]

        DataGenerator.__init__(self, options)
        
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
        
        for k, v in self._talenttreeprimaryspells_db.iteritems():
            talent_tab = self._talenttab_db[v.id_talent_tab]
            
            keys[self._class_map[talent_tab.mask_class]][talent_tab.tab_page].append( ( self._spell_db[v.id_spell].name, v.id_spell, talent_tab.name) )
        
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
        s += 'static unsigned __%s_data[][MAX_TALENT_TABS][%s_SIZE] = {\n' % (
            data_str,
            data_str.upper(),
        )
        
        for cls in xrange(0, len(keys)):
            if DataGenerator._class_names[cls]:
                s += '  // Talent Tree specialization abilities for %s\n' % DataGenerator._class_names[cls]
            s += '  {\n'
            
            for tree in xrange(0, len(keys[cls])):
                if len(keys[cls][tree]) > 0:
                    s += '    // Specialization abilities for %s tree\n' % keys[cls][tree][0][2]
                s += '    {\n'
                for ability in sorted(keys[cls][tree], key = lambda i: i[0]):
                    s += '      %5d, // %s\n' % ( ability[1], ability[0] )
                    
                if len(keys[cls][tree]) < max_ids:
                    s += '      %5d,\n' % 0
                    
                s += '    },\n'
            s += '  },\n'
        s += '};\n'
        
        return s
                
class SpellListGenerator(SpellDataGenerator):
    def __init__(self, options):
        SpellDataGenerator.__init__(self, options)

        # Add SkillLine so we can fancy output
        self._dbc += [ 'SkillLine' ]
        
        # Blacklist glyphs for this
        #SpellDataGenerator._skill_category_blacklist += [ 810 ]
        #SpellDataGenerator._spell_name_blacklist += [ '^Glyph of' ]
        
        # Blacklist some ids, as no idea how to filter them out
        SpellDataGenerator._spell_blacklist += [
            # 54158,  # Judgement
            66198,  # Obliterate (offhand attack)
            66217,  # Rune Strike (offhand attack)
            88767,  # Fulmination
            22959,  # Critical Mass (weird talent debuff)
            86941,  # Molten armor (talented replacement effect)
            92315,  # Pyroblast (unknown)
            89420,  # Drain Life (unknown)
            81283,  # Fungal Growth (talented Treant secondary effect)
            81291,  # Fungal Growth (talented Treant secondary effect)
        ]

    def spell_state(self, spell):
        if not SpellDataGenerator.spell_state(self, spell):
            return False
            
        # Skip passive spells
        if spell.flags & 0x40:
            return False
            
        # Skip by possible indicator for spellbook visibility
        if spell.flags_4 & 0x8000:
            return False;
        
        # Skip spells without any resource cost and category
        if spell.id_power == 0 and spell.id_rune_cost == 0 and spell.id_categories == 0:
            return False
        
        # Make sure rune cost makes sense, even if the rune cost id is valid
        if spell.id_rune_cost > 0:
            src = self._spellrunecost_db[spell.id_rune_cost]
            if src.rune_cost_1 == 0 and src.rune_cost_2 == 0 and src.rune_cost_3 == 0:
                return False
                
        # Filter out any "Rank x" string, as there should no longer be such things. This should filter out 
        # some silly left over? things, or things not shown to player anyhow, so should be all good.
        if spell.ofs_rank > 0 and 'Rank ' in spell.rank:
            return False
        
        # Let's not accept spells that have over 100y range, as they cannot really be base abilities then
        if spell.id_range > 0:
            range = self._spellrange_db[spell.id_range]
            if range.max_range > 100.0 or range.max_range_2 > 100.0:
                return False
        
        return True
        
    def filter(self):
        triggered_spell_ids = []
        ids = { }
        spell_tree = 0
        spell_tree_name = ''
        for ability_id, ability_data in self._skilllineability_db.iteritems():
            mask_class_category = 0
            if ability_data.max_value > 0 or ability_data.min_value > 0:
                continue

            if ability_data.id_skill in SpellDataGenerator._skill_category_blacklist:
                continue

            # Guess class based on skill category identifier
            for j in xrange(0, len(SpellDataGenerator._class_categories)):
                for k in xrange(0, len(SpellDataGenerator._class_categories[j])):
                    if ( isinstance(SpellDataGenerator._class_categories[j][k], tuple) and \
                         ability_data.id_skill in SpellDataGenerator._class_categories[j][k] ) or \
                         ability_data.id_skill == SpellDataGenerator._class_categories[j][k]:
                        mask_class_category = DataGenerator._class_masks[j]
                        # Get spell tree to filter data, clump up pets to same category (fourth)
                        spell_tree = k > 4 and 4 or k
                        spell_tree_name = k > 3 and 'Pet' or self._skillline_db[ability_data.id_skill].name
                        break
                
                if mask_class_category:
                    break;
            
            # We only want abilities that belong to a class
            if not mask_class_category:
                continue
                
            spell = self._spell_db[ability_data.id_spell]
            if not spell.id:
                continue
                
            # Skip triggered spells
            if ability_data.id_spell in triggered_spell_ids:
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
                ids[ability_data.id_spell]['mask_class'] |= ability_data.mask_class or mask_class_category
            else:
                ids[ability_data.id_spell] = { 
                    'mask_class': ability_data.mask_class or mask_class_category,
                    'tree'      : spell_tree,
                    'tree_name' : spell_tree_name,
                }
        """
        for talent_id, talent_data in self._talent_db.iteritems():
            talent_tab = self._talenttab_db[talent_data.talent_tab]
            if not talent_tab.id:
                continue

            # Make sure the talent is a class  / pet associated one
            if talent_tab.mask_class      not in DataGenerator._class_masks and \
               talent_tab.mask_pet_talent not in DataGenerator._pet_masks:
                continue

            # Get all talents that have spell ranks associated with them
            for rank in xrange(1, 4):
                id = getattr(talent_data, 'id_rank_%d' % rank)
                
                spell = self._spell_db[id]
                if not spell.id:
                    continue
                    
                if not self.spell_state(spell):
                    continue

                if ids.get(id):
                    ids[id]['mask_class'] |= talent_tab.mask_class or 0x04 # if class is 0, it's a hunter pet talent, so 0x04 mask
                else:
                    ids[id] = { 'mask_class': talent_tab.mask_class or 0x04 }
        """
        for cls in xrange(1, len(SpellDataGenerator._spell_id_list)):
            for spell_tuple in SpellDataGenerator._spell_id_list[cls]:
                # Skip spells with zero tree, as they dont exist
                if spell_tuple[1] == 0:
                    continue

                spell = self._spell_db[spell_tuple[0]]
                if not spell.id:
                    continue
                
                if not self.spell_state(spell):
                    continue
                
                if ids.get(spell_tuple[0]):
                    ids[spell_tuple[0]]['mask_class'] |= self._class_masks[cls]
                else:
                    ids[spell_tuple[0]] = { 
                        'mask_class': self._class_masks[cls], 
                        'tree'      : spell_tuple[1],
                        'tree_name' : spell_tuple[1] == 4 and 'Pet' or None
                    }

        return ids
    
    def generate(self, ids = None):
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
        
        # Sort a suitable list for us
        for k, v in ids.iteritems():
            if v['mask_class'] not in DataGenerator._class_masks:
                continue

            spell = self._spell_db[k]
            keys[self._class_map[v['mask_class']]][v['tree'] - 1].append(( spell.name, spell.id ))

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
                    if ids[t[1]]['tree_name']:
                        s += '    // %s tree, %d abilities\n' % ( ids[keys[i][j][0][1]]['tree_name'], len(keys[i][j]) )
                        break
                s += '    {\n'
                for spell_id in sorted(keys[i][j], key = lambda k_: k_[0]):
                    r = ''
                    if self._spell_db[spell_id[1]].rank:
                        r = ' (%s)' % self._spell_db[spell_id[1]].rank
                    s += '      %5d, // %s%s\n' % ( spell_id[1], spell_id[0], r )
            
                # Append zero if a short struct
                if max_ids - len(keys[i][j]) > 0:
                    s += '      %5d,\n' % 0

                s += '    },\n'
            
            s += '  },\n'
            
        s += '};\n'
        
        return s

class ClassFlagGenerator(SpellDataGenerator):
    def __init__(self, options):
        SpellDataGenerator.__init__(self, options)
    """
    def filter(self, class_name):
        ids = { }
        mask = DataGenerator._class_masks[self._class_map[class_name.capitalize()]]

        for ability_id, ability_data in self._skilllineability_db.iteritems():
            if not (ability_data.mask_class & mask):
                continue

            ids[ability_data.id_spell] = {}

        return ids
    """
    def generate(self, ids, class_name):
        s = ''
        mask = DataGenerator._class_masks[self._class_map[class_name.capitalize()]]
        spell_family = { }

        for spell_id, data in ids.iteritems():
            spell = self._spell_db[spell_id]
            if not spell.id_class_opts:
                continue

            if not data['mask_class'] & mask:
                continue

            copts = self._spellclassoptions_db[spell.id_class_opts]
            if not copts.spell_family_name in spell_family.keys():
                spell_family[copts.spell_family_name] = [ ]
                for i in xrange(0, 96):
                    spell_family[copts.spell_family_name].append({
                        'spells' : [ ],
                        'effects': [ ]
                    })


            # Assign this spell to bitfield entries
            for i in xrange(1, 4):
                f = getattr(copts, 'spell_family_flags_%u' % i)
                
                for bit in xrange(0, 31):
                    if not (f & (1 << bit)):
                        continue

                    bfield = ((i - 1) * 32) + bit
                    spell_family[copts.spell_family_name][bfield]['spells'].append( spell )

            # Loop through spell effects, assigning them to effects
            for effect in spell._effects:
                if not effect:
                    continue

                for i in xrange(1, 4):
                    f = getattr(effect, 'class_mask_%u' % i)
                    for bit in xrange(0, 31):
                        if not (f & (1 << bit)):
                            continue

                        bfield = ((i - 1) * 32) + bit
                        spell_family[copts.spell_family_name][bfield]['effects'].append( effect )

            """
            spell = self._spell_db[spell_id]
            sname = spell.name
            if spell.rank:
                sname += ' (%s)' % spell.rank

            # Calculate the class specific thing for the spell
            if spell.id_class_opts > 0:
                opts = self._spellclassoptions_db[spell.id_class_opts]

                for i in xrange(1, 4):
                    f = getattr(opts, 'spell_family_flags_%u' % i)
                    
                    for bit in xrange(0, 31):
                        if not (f & (1 << bit)):
                            continue

                        bfield = ((i - 1) * 32) + bit
                        class_opts[bfield].append( ( [ -1 ], sname, spell.id ) )

            # Aand check what affects what for spells too
            for effect in spell._effects:
                if not effect:
                    continue

                # Skip anything but modifier applications
                if effect.type != 6 or effect.sub_type not in [ 13, 79, 107, 108, 308 ]:
                    continue

                # Skip server side scripts and see what happens then
                #if effect.type == 74 or effect.type == 0 or effect.type == 3 or (effect.type == 6 and effect.sub_type == 4):
                #    continue

                for i in xrange(1, 4):
                    f = getattr(effect, 'class_mask_%u' % i)
                    for bit in xrange(0, 31):
                        if not (f & (1 << bit)):
                            continue

                        bfield = ((i - 1) * 32) + bit
                        found = False
                        for t in class_opts[bfield]:
                            if sname == t[1] and spell.id == t[2]:
                                if effect.index not in t[0]:
                                    t[0].append(effect.index)

                                found = True
                                break

                        if not found:
                            class_opts[bfield].append( ( [ effect.index ], sname, spell.id ) )
        
        # Then, loop ALL talents for the class, and adding their effect's options
        for talent_id, talent_data in self._talent_db.iteritems():
            tabinfo = self._talenttab_db[talent_data.talent_tab]
            if tabinfo.mask_class != mask:
                continue

            spell = self._spell_db[talent_data.id_rank_1]
            if not spell.id:
                continue

            sname = spell.name
            if spell.rank:
                sname += ' (%s)' % spell.rank

            for effect in spell._effects:
                if not effect:
                    continue

                # Skip anything but modifier applications
                if effect.type != 6 or effect.sub_type not in [ 13, 79, 107, 108, 308 ]:
                    continue

                # Skip server side scripts and see what happens then
                #if effect.type == 74 or effect.type == 0 or effect.type == 3 or (effect.type == 6 and effect.sub_type == 4):
                #    continue

                for i in xrange(1, 4):
                    f = getattr(effect, 'class_mask_%u' % i)
                    for bit in xrange(0, 31):
                        if not (f & (1 << bit)):
                            continue

                        bfield = ((i - 1) * 32) + bit
                        found = False
                        for t in class_opts[bfield]:
                            if sname == t[1] and spell.id == t[2]:
                                if effect.index not in t[0]:
                                    t[0].append(effect.index)

                                found = True
                                break

                        if not found:
                            class_opts[bfield].append( ( [ effect.index ], sname, spell.id ) )
        
        s = 'Bit Spell name, id\n'
        for i in xrange(0, len(class_opts)):
            if not class_opts[i]:
                s += ' %2d.\n' % (i + 1)
            else:
                s += ' %2d. %s%s (%u)\n' % ( 
                    i + 1, 
                    (-1 not in sorted(class_opts[i])[0][0]) and ('[%s] ' % ', '.join(["%s" % el for el in sorted(class_opts[i])[0][0]])) or '    ', 
                    sorted(class_opts[i])[0][1],
                    sorted(class_opts[i])[0][2]
                )
                for sp in sorted(class_opts[i])[1:]:
                    s += '     %s%s (%u)\n' % ( 
                        (-1 not in sp[0]) and ('[%s] ' % ', '.join(["%s" % el for el in sp[0]])) or '    ', 
                        sp[1], 
                        sp[2])

        """

        for family, d in sorted(spell_family.items()):
            s += 'Spell family %d:\n' % family
            for bit_field in xrange(0, len(d)):
                if not len(d[bit_field]['spells']):
                    continue

                s += '  [%-2d] ===================================================\n' % bit_field
                for spell in sorted(d[bit_field]['spells'], key = lambda s: s.name):
                    s += '       %s (%u)\n' % ( spell.name, spell.id )
                
                for effect in sorted(d[bit_field]['effects'], key = lambda e: e.id_spell):
                    rstr = ''
                    if self._spell_db[effect.id_spell].rank:
                        rstr = ' (%s)' % self._spell_db[effect.id_spell].rank
                    s += '         [%u] {%u} %s%s\n' % ( effect.index, effect.id_spell, self._spell_db[effect.id_spell].name, rstr)

                s += '\n'

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
                    s += '      %5d, // %s\n' % ( glyph[1], glyph[0] )
                    
                if len(keys[cls][glyph_slot]) < max_ids:
                    s += '      %5d,\n' % 0
                    
                s += '    },\n'
            s += '  },\n'
        s += '};\n'
        
        return s

class ItemSetListGenerator(SpellDataGenerator):
    _tier_base = 11
    
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
            for id_spell_field in xrange(1, 9):
                spell_id = getattr(itemset_data, 'id_spell_%d' % id_spell_field)
                if spell_id:
                    filter_list = { }
                    # We will want to filter the tier spell ids, but 
                    # at the same time, only take the base spell
                    lst = self.generate_spell_filter_list(spell_id, mask_class_category, 0, 0, filter_list)
                    if not lst:
                        continue

                    if spell_id not in lst.keys():
                        continue

                    ids[spell_id] = {
                        'mask_class': lst[spell_id]['mask_class'],
                        'set'       : itemset_data.name,
                        'tier'      : tier_id,
                        'n_bonus'   : getattr(itemset_data, 'n_items_%d' % id_spell_field)
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
            s += '  { %s },\n' % (', '.join(fields))

        s += '};\n'
    
        return s

class SpellItemEnchantmentGenerator(RandomSuffixGenerator):
    def __init__(self, options):
        RandomSuffixGenerator.__init__(self, options)

    def filter(self):
        #sfx_ids = set(RandomSuffixGenerator.filter(self))
        #ids = set()
        
        #for sfx_id in sorted(sfx_ids):
        #    data = self._itemrandomsuffix_db[sfx_id]
        #    for j in xrange(1, 6):
        #        val = getattr(data, 'id_property_%d' % j )
        #        if val > 0: ids.add( val )

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

            fields = ench_data.field('id', 'slot', 'desc', 'id_gem', 'req_skill', 'req_skill_value')
            fields += [ '{ %s }' % ', '.join(ench_data.field('type_1', 'type_2', 'type_3')) ]
            fields += [ '{ %s }' % ', '.join(ench_data.field('amount_1', 'amount_2', 'amount_3')) ]
            fields += [ '{ %s }' % ', '.join(ench_data.field('id_property_1', 'id_property_2', 'id_property_3')) ]
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
            if ilevel >= 1 and ilevel <= self._options.max_ilevel:
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
            self._options.max_ilevel, self._options.build )
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
                dbname, self._options.max_ilevel, self._options.build )
            s += 'static struct item_scale_data_t __%s%s%s_data[] = {\n' % (
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                dbname.lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '' )

            for ilevel, data in db.iteritems():
                if ilevel > self._options.max_ilevel:
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
                dbname, self._options.max_ilevel, self._options.build )
            
            s += 'static struct %s __%s%s%s_data[] = {\n' % (
                (dbname != 'ItemArmorTotal') and 'item_scale_data_t' or 'item_armor_type_data_t',
                self._options.prefix and ('%s_' % self._options.prefix) or '',
                dbname.lower(),
                self._options.suffix and ('_%s' % self._options.suffix) or '' )

            for ilevel, data in db.iteritems():
                if ilevel > self._options.max_ilevel:
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

        for id in ids:
            data = self._gemproperties_db[id]
            if data.color == 0:
                continue;

            fields = data.field('id', 'id_enchant', 'color')
            s += '  { %s },\n' % (', '.join(fields))

        s += '  { %s }\n' % ( ', '.join([ '0', '0', '0' ]) )
        s += '};\n\n'

        return s
