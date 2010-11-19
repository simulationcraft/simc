import sys, os, re, types, json
import parser, db, data

#
# Custom formatting functions for some of the stuff
#
def __flags(generator, filter_data, record, raw, *args):
    if raw: return 0
    return args[0] % 0

def __filter(generator, filter_data, record, raw, *args):
    if raw: return filter_data[args[1]]
    return args[0] % filter_data[args[1]]

def __name(generator, filter_data, record, raw, *args):
    if raw: return record.name
    return args[0] % ( '"%s"' % record.name )

def __rune_cost(generator, filter_data, record, raw, *args):
    cost = 0
    for rune_type in xrange(0, 3):
        for i in xrange(0, getattr(record, 'rune_cost_%d' % (rune_type + 1))):
            cost |= 1 << (rune_type * 2 + i)
    
    if raw: return cost
    return args[0] % cost

def __avg(generator, filter_data, record, raw, *args):
    spell = generator._Spell_db.get(record.id_spell)
    if not spell:
        if raw: return 0.0
        return args[0] % 0.0

    if raw: return getattr(generator._SpellScaling_db[spell.id_scaling], 'e%d_average' % (record.index + 1))
    return args[0] % getattr(generator._SpellScaling_db[spell.id_scaling], 'e%d_average' % (record.index + 1))
    
def __delta(generator, filter_data, record, raw, *args):
    spell = generator._Spell_db.get(record.id_spell)
    if not spell:
        if raw: return 0.0
        return args[0] % 0.0
    
    if raw: return getattr(generator._SpellScaling_db[spell.id_scaling], 'e%d_delta' % (record.index + 1))
    return args[0] % getattr(generator._SpellScaling_db[spell.id_scaling], 'e%d_delta' % (record.index + 1))

def __unk(generator, filter_data, record, raw, *args):
    spell = generator._Spell_db.get(record.id_spell)
    if not spell:
        if raw: return 0.0
        return args[0] % 0.0

    if raw: return getattr(generator._SpellScaling_db[spell.id_scaling], 'e%d_unk' % (record.index + 1))
    return args[0] % getattr(generator._SpellScaling_db[spell.id_scaling], 'e%d_unk' % (record.index + 1))

def __min_range(generator, filter_data, record, raw, *args):
	if record.id_range > 0:
		min_range_record = generator._SpellRange_db[record.id_range]
		if raw: return min_range_record.max_range
		return args[0] % min_range_record.max_range
	
	if raw: return min_range
	return args[0] % min_range

#
# Output formats for build -> dbc file
# - Last known output format for the dbc will be used, regardless of if it 
#   is defined for a build or not, i.e. as there's now only 1 build defined, 
#   all build versions use the same output formats, defined for 12604
# - Output format is of the form
#   ( path, {format|function}, args... ),
#   * Path is the path to the value to be formatted, paths consist of 
#     DBC_NAME[.attr]* lists and a special '::' string, denoting an 
#     indirection to another DBC_NAME. The last attribute before the 
#     '::' is used as the identifier field for the second DBC_NAME 
#     element.
#   * Format is a printf() string format (Pythonized version), the 
#     value resulting from path traversal will be passed to the 
#     format. If a function is passed, the function parameters will 
#     be generator, filter_data, record, *args, where
#     - generator is the generator instance
#     - filter_data is the associated filter data dictionary for this
#       id value
#     - record is the value resulting from path traversal
#     - *args are the remaining args... from the output format tuple
#
_OUTPUT_FORMATS = {
    12604: {
        'Spell': [
            # Spell.dbc
            ( 'Spell', __name, '%-36s' ), ( 'Spell.id', '%5u' ), ( 'Spell', __flags, '%#.2x' ), ( 'Spell.prj_speed', '%4.1f' ), 
            ( 'Spell.mask_school', '%#.2x' ), ( 'Spell.type_power', '%2d' ),  ( 'Spell', __filter, '%#.3x', 'mask_class' ), ( 'Spell', __filter, '%#.3x', 'mask_race' ),
            # SpellLevels.dbc
            ( 'Spell.id_levels::SpellLevels.base_level', '%3u' ), 
            # SpellRange.dbc
            ( 'Spell.id_range::SpellRange.min_range', '%7.1f' ), ( 'Spell.id_range::SpellRange.max_range', '%7.1f' ), 
            # SpellCooldown.dbc
            ( 'Spell.id_cooldowns::SpellCooldowns.cooldown_duration', '%7u' ), ( 'Spell.id_cooldowns::SpellCooldowns.gcd_cooldown', '%4u' ),
            # SpellCategories.dbc
            ( 'Spell.id_categories::SpellCategories.category', '%4u' ),
            # SpellDuration.dbc
            ( 'Spell.id_duration::SpellDuration.duration_1', '%9.1f' ),
            # SpellPower.dbc
            ( 'Spell.id_power::SpellPower.power_cost', '%3u' ),
            # SpellRuneCost.dbc
            ( 'Spell.id_rune_cost::SpellRuneCost', __rune_cost, '%#.3x' ), ( 'Spell.id_rune_cost::SpellRuneCost.rune_power_gain', '%3u' ),
            # SpellAuraOptions.dbc
            ( 'Spell.id_aura_opt::SpellAuraOptions.stack_amount', '%3u' ), ( 'Spell.id_aura_opt::SpellAuraOptions.proc_chance', '%3u' ), 
            ( 'Spell.id_aura_opt::SpellAuraOptions.proc_charges', '%2u' ),
            # SpellEffect.dbc
            ( 'Spell.effect_1.id', '%5u' ), ( 'Spell.effect_2.id', '%5u' ), ( 'Spell.effect_3.id', '%5u' )
        ],
        'SpellEffect': [
            ( 'SpellEffect.id', '%5u' ), ( 'SpellEffect.id_spell', '%5u' ), ( 'SpellEffect.index', '%u' ), 
            ( 'SpellEffect.type', '%3u' ), ( 'SpellEffect.sub_type', '%3u' ),
            # SpellScaling.dbc
            ( 'SpellEffect', __avg, '%13.10f' ), ( 'SpellEffect', __delta, '%13.10f' ), ( 'SpellEffect', __unk, '%13.10f' ), 
            # more SpellEffect.dbc
            ( 'SpellEffect.coefficient', '%13.10f' ), ( 'SpellEffect.amplitude', '%5u' ), 
            # SpellRadius.dbc
            ( 'SpellEffect.id_radius::SpellRadius.radius_1', '%7.1f' ), ( 'SpellEffect.id_radius_max::SpellRadius.radius_1', '%7.1f' ),
            # more SpellEffect.dbc
            ( 'SpellEffect.base_value', '%7d' ), ( 'SpellEffect.misc_value', '%7d' ), ( 'SpellEffect.misc_value_2', '%7d' ), ( 'SpellEffect.trigger_spell', '%5u' ),
            ( 'SpellEffect.points_per_combo_points', '%5.1f' )
        ],
        'Talent': [
            ( 'Talent.id_rank_1::Spell', __name, '%-36s' ), ( 'Talent.id', '%5u' ), ( 'Talent', __flags, '%#.2x' ),
            # TalentTab.dbc
            ( 'Talent.talent_tab::TalentTab.tab_page', '%2u' ), ( 'Talent.talent_tab::TalentTab.mask_class', '%#.3x' ), 
            ( 'Talent.talent_tab::TalentTab.mask_pet_talent', '%#.1x' ), 
            # more Talent.dbc
            ( 'Talent.talent_depend', '%5u' ), ( 'Talent.depend_rank', '%u' ), 
            ( 'Talent.row', '%u' ), ( 'Talent.col', '%u' ), 
            ( 'Talent.id_rank_1', '%5u' ), ( 'Talent.id_rank_2', '%5u' ), ( 'Talent.id_rank_3' ,'%5u' )
        ],
        'gtCombatRatings': [ ( 'gt_value', '%13.10f' ) ],
        'gtChanceToMeleeCritBase': [ ( 'gt_value', '%13.10f' ) ], 
        'gtChanceToSpellCritBase': [ ( 'gt_value', '%13.10f' ) ],
        'gtSpellScaling': [ ( 'gt_value', '%15.10f' ) ], 
        'gtChanceToMeleeCrit': [ ( 'gt_value', '%15.10f' ) ], 
        'gtChanceToSpellCrit': [ ( 'gt_value', '%15.10f' ) ], 
        'gtRegenMPPerSpt': [ ( 'gt_value', '%13.10f' ) ], 
        'gtOCTRegenMP': [ ( 'gt_value', '%13.10f' ) ],
    },
    12694: {
        'Spell': [
            # Spell.dbc
            ( 'Spell', __name, '%-36s' ), ( 'Spell.id', '%5u' ), ( 'Spell', __flags, '%#.2x' ), ( 'Spell.prj_speed', '%4.1f' ), 
            ( 'Spell.mask_school', '%#.2x' ), ( 'Spell.type_power', '%2d' ), ( 'Spell', __filter, '%#.3x', 'mask_class' ), ( 'Spell', __filter, '%#.3x', 'mask_race' ),
            # SpellLevels.dbc
            ( 'Spell.id_levels::SpellLevels.base_level', '%3u' ), 
            # SpellRange.dbc
            ( 'Spell.id_range::SpellRange', __min_range, '%7.1f' ), ( 'Spell.id_range::SpellRange.max_range', '%7.1f' ), 
            # SpellCooldown.dbc
            ( 'Spell.id_cooldowns::SpellCooldowns.cooldown_duration', '%7u' ), ( 'Spell.id_cooldowns::SpellCooldowns.gcd_cooldown', '%4u' ),
            # SpellCategories.dbc
            ( 'Spell.id_categories::SpellCategories.category', '%4u' ),
            # SpellDuration.dbc
            ( 'Spell.id_duration::SpellDuration.duration_1', '%9.1f' ),
            # SpellPower.dbc
            ( 'Spell.id_power::SpellPower.power_cost', '%3u' ),
            # SpellRuneCost.dbc
            ( 'Spell.id_rune_cost::SpellRuneCost', __rune_cost, '%#.3x' ), ( 'Spell.id_rune_cost::SpellRuneCost.rune_power_gain', '%3u' ),
            # SpellAuraOptions.dbc
            ( 'Spell.id_aura_opt::SpellAuraOptions.stack_amount', '%3u' ), ( 'Spell.id_aura_opt::SpellAuraOptions.proc_chance', '%3u' ), 
            ( 'Spell.id_aura_opt::SpellAuraOptions.proc_charges', '%2u' ),
            # SpellEffect.dbc
            ( 'Spell.effect_1.id', '%5u' ), ( 'Spell.effect_2.id', '%5u' ), ( 'Spell.effect_3.id', '%5u' ), 
            # Spell.dbc flags system
            ( 'Spell.flags', '%#.8x' ), ( 'Spell.flags_1', '%#.8x' ), ( 'Spell.flags_2', '%#.8x' ), ( 'Spell.flags_3', '%#.8x' ), ( 'Spell.flags_4', '%#.8x' ),
            ( 'Spell.flags_5', '%#.8x' ), ( 'Spell.flags_6', '%#.8x' ), ( 'Spell.flags_7', '%#.8x' ), ( 'Spell.flags_12694', '%#.8x' ), ( 'Spell.flags_8', '%#.8x' ),
        ],
    }
}

_NAME_CLASS   = [ None, 'Warrior', 'Paladin', 'Hunter', 'Rogue', 'Priest', 'Death Knight', 'Shaman', 'Mage', 'Warlock', None, 'Druid' ]
_MASK_CLASS   = [ None, 0x1,       0x2,       0x4,      0x8,     0x10,     0x20,           0x40,      0x80,   0x100,    None, 0x400   ]
_NAME_PET     = [ None, 'Ferocity', 'Tenacity', None, 'Cunning' ]
_MASK_PET     = [ None, 0x1,        0x2,        None, 0x4       ]
_NAME_RATINGS = [ 'Dodge',        'Parry',       'Block',       'Melee hit',  'Ranged hit', 
                  'Spell hit',    'Melee crit',  'Ranged crit', 'Spell crit', 'Melee haste', 
                  'Ranged haste', 'Spell haste', 'Expertise',   'Mastery' ]
_ID_RATINGS   = [ 2, 3, 4, 5, 6, 7, 8, 9, 10, 17, 18, 19, 23, 25 ]

class DataGenerator(object):
    def __initialize_database(self, name):
        dbc = parser.DBCParser(self._options, os.path.abspath(os.path.join(self._options.path, '%s.dbc' % name)))
        if not dbc.open_dbc():
            raise sys.exit(1)
        
        #print dbc, dbc._class, os.path.abspath(os.path.join(self._options.path, '%s.dbc' % name))
        dbs = db.DBCDB(dbc._class)

        record = dbc.next_record()
        while record != None:
            dbs[record.id] = record
            record = dbc.next_record()
        
        setattr(self, '_%s_dbc' % name, dbc)
        setattr(self, '_%s_db' % name, dbs)

        return dbs

    # Do on-demand database loading
    def __getattr__(self, name):
        if '_db' in name and name not in dir(self):
            return self.__initialize_database(re.sub(r'_([A-z]+)_db', r'\1', name))
            
        return object.__getattribute__(self, name)
    
    def __init__(self, options):
        self._options = options

        self._class_map = { }
        # Build some maps to help us output things
        for i in xrange(0, len(_NAME_CLASS)):
            if not _NAME_CLASS[i]:
                continue

            self._class_map[_NAME_CLASS[i]] = i
            self._class_map[_NAME_CLASS [i]] = i

    def output_format(self, output_type):
        fields      = [ ]
        build_level = self._options.build or data.current_patch_level()
        # Figure out which output fields we should use
        for build in sorted(_OUTPUT_FORMATS.keys()):
            if build > build_level:
                break

            if not fields or (fields and _OUTPUT_FORMATS[build].get(output_type)):
                fields = _OUTPUT_FORMATS[build].get(output_type)
        
        return fields

    def tokenize_path(self, raw_path):
        path         = [ ]
        dbc_elements = [ raw_path ]
        
        if '::' in raw_path:
            dbc_elements = raw_path.split('::')
            
        for dbc_element in dbc_elements:
            if '.' in dbc_element:
                path.append( tuple( dbc_element.split('.') ) )
            else:
                path.append( ( dbc_element, ) )

        return path

    def initialize(self):
        return True

    def filter(self):
        return None

    def generate(self, ids = None):
        return ''
    
    def generate_fields(self, fields, id, filter_data, raw = False):
        values      = [ ]
        # 1) :: for linkage to another dbc file
        # 2) . for attribute traversal in record
        # 3) if format is a function, pass record, filter data and dbc parser to it          
        for field in fields:
            dbc_elements = [ field[0] ]
            val          = None
            
            if '::' in field[0]:
                dbc_elements = field[0].split('::')
                
            val = id
            # Loop through the dbc path
            # val will have either the ending value or the 
            # record, if the path ends with a DBC type
            for dbc_element in dbc_elements:
                if '.' in dbc_element:
                    operands = dbc_element.split('.')
                else:
                    operands = [ dbc_element ]

                db = getattr(self, '_%s_db' % operands[0])
                record = db[val]
                if len(operands) > 1:
                    for attr in operands[1:-1]:
                        record = getattr(record, attr)
                    
                    val = getattr(record, operands[-1])
                else:
                    val = record
                    break
            
            if isinstance(field[1], types.FunctionType):
                args = []
                if len(field) > 2:
                    args = field[2:]
                
                values.append(field[1](self, filter_data, val, raw, *args))
            else:
                if not raw:
                    values.append(field[1] % val)
                else:
                    values.append(val)
        
        return values
    
    def generate_cpp(self, output_type, ids = { }, columns = 1, comment = '', indent = 2):
        s           = ''
        fields      = self.output_format(output_type)
        data_fmt    = r'{ %s }, '
        if len(fields) == 1:
            data_fmt = r'%s, '

        if comment != '':
            s += '// %s\n' % comment
        else:
            s += '// %s data, wow build %d, %d entries\n' % ( 
                output_type, self._options.build or data.current_patch_level(), len(ids.keys()))

        s += 'static struct %s_data_t %s_data[] = {\n' % ( output_type.lower(), output_type.lower() )
        for id in sorted(ids.keys()):
            s += ' ' * indent
            for col in xrange(0, columns):
                s+= data_fmt % (', '.join(self.generate_fields(fields, id, ids[id])))

            s += '\n'

        s += '};\n'

        return s

    # JSON export probably could use with some mappings, instead of pure 
    # array based stuff. Regardless, the json data is raw values, no formatting
    # done whatsoever
    def generate_json(self, output_type, ids = { }, columns = 1, comment = '', indent = 2):
        s           = ''
        fields      = self.output_format(output_type)
        json_data   = [ ]
        if len(fields) == 1:
            data_fmt = r'%s, '

        if comment != '':
            s += '// %s\n' % comment
        else:
            s += '// %s data, wow build %d, %d entries\n' % ( 
                output_type, self._options.build or data.current_patch_level(), len(ids.keys()))
        
        s += 'var %s_data = ' % output_type.lower()

        for id in sorted(ids.keys()):
            json_data.append( self.generate_fields(fields, id, ids[id], True) )

        return s + json.JSONEncoder().encode(json_data)

class SpellDataGenerator(DataGenerator):
    __skill_categories = [
        ( ),
        ( 26, 256, 27 ),    # Warrior
        ( 594, 267, 184 ),  # Paladin
        ( 50, 163, 51 ),    # Hunter
        ( 253, 38, 39 ),    # Rogue
        ( 613, 56, 78 ),    # Priest
        ( 770, 771, 772 ),  # Death Knight
        ( 375, 373, 374 ),  # Shaman
        ( 237, 8, 6 ),      # Mage
        ( 355, 354, 593 ),  # Warlock
        ( ), 
        ( 574, 134, 573 ),  # Druid
    ]

    # Any spell with this effect type, will be automatically 
    # blacklisted
    # http://github.com/mangos/mangos/blob/400/src/game/SharedDefines.h
    __effect_type_blacklist = [
        5,      # SPELL_EFFECT_TELEPORT_UNITS
        18,     # SPELL_EFFECT_RESURRECT
        25,     # SPELL_EFFECT_WEAPONS
        39,     # SPELL_EFFECT_LANGUAGE
        50,     # SPELL_EFFECT_TRANS_DOOR
        60,     # SPELL_EFFECT_PROFICIENCY
        71,     # SPELL_EFFECT_PICKPOCKET
        94,     # SPELL_EFFECT_SELF_RESURRECT
        97,     # SPELL_EFFECT_SUMMON_ALL_TOTEMS
        109,    # SPELL_EFFECT_SUMMON_DEAD_PET 
        110,    # SPELL_EFFECT_DESTROY_ALL_TOTEMS
        126,    # SPELL_STEAL_BENEFICIAL_BUFF
    ]
    
    # http://github.com/mangos/mangos/blob/400/src/game/SpellAuraDefines.h 
    __aura_type_blacklist = [
        2,      # SPELL_AURA_MOD_POSSESS  
        6,      # SPELL_AURA_MOD_CHARM 
        44,     # SPELL_AURA_TRACK_CREATURES
        78,     # SPELL_AURA_MOUNTED
        91,     # SPELL_AURA_MOD_DETECT_RANGE
        206,    # SPELL_AURA_MOD_FLIGHT_SPEED_xx begin
        207, 
        208,
        209, 
        210,
        211,
        212     # SPELL_AURA_MOD_FLIGHT_SPEED_xx ends
    ]

    __spell_blacklist = [
        3561,   # Teleports --
        3562,
        3563,
        3565,
        3566,
        3567,   # -- Teleports
        42955,  # Conjure Refreshment
        43987,  # Ritual of Refreshment
        46584,  # Raise Dead
        48018,  # Demonic Circle: Summon
        48020,  # Demonic Circle: Teleport
    ]

    def initialize(self):
        DataGenerator.initialize(self)

        # Map Spell effects to Spell IDs so we can do filtering based on them
        for spell_effect_id, spell_effect_data in self._SpellEffect_db.iteritems():
            if not spell_effect_data.id_spell:
                continue

            spell = self._Spell_db[spell_effect_data.id_spell]
            if not spell.id:
                continue

            spell.add_effect(spell_effect_data)
        
        return True
    
    def generate_spell_filter_list(self, spell_id, mask_class, mask_pet, mask_race):
        filter_list = { }
        spell = self._Spell_db[spell_id]
        enabled_effects = [ True, True, True ]

        if not spell.id:
            return None

        # Check for blacklisted spells
        if spell.id in SpellDataGenerator.__spell_blacklist:
            return None

        # Effect blacklist processing
        for effect_index in xrange(0, len(spell._effects)):
            if not spell._effects[effect_index]:
                enabled_effects[effect_index] = False
                continue
            
            effect = spell._effects[effect_index]
            
            if effect.type in SpellDataGenerator.__effect_type_blacklist:
                enabled_effects[effect.index] = False

            # Filter by apply aura (party, raid)
            if effect.type in [ 6, 35, 65 ] and effect.sub_type in SpellDataGenerator.__aura_type_blacklist:
                enabled_effects[effect.index] = False
        
        # If we do not find a true value in enabled effects, this spell is completely
        # blacklisted, and gone
        # // 2764 spells, wow build level 12644
        # // 4195 effects, wow build level 12644
        if True not in enabled_effects:
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
                lst = self.generate_spell_filter_list(effect.trigger_spell, mask_class, mask_pet, mask_race)
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
        for talent_id, talent_data in self._Talent_db.iteritems():
            talent_tab = self._TalentTab_db[talent_data.talent_tab]
            if not talent_tab.id:
                continue

            # Make sure the talent is a class  / pet associated one
            if talent_tab.mask_class      not in _MASK_CLASS and \
               talent_tab.mask_pet_talent not in _MASK_PET:
                continue

            # Get all talents that have spell ranks associated with them
            for rank in xrange(1, 4):
                id = getattr(talent_data, 'id_rank_%d' % rank)
                if id:
                    lst = self.generate_spell_filter_list(id, talent_tab.mask_class, talent_tab.mask_pet_talent, 0)
                    if not lst:
                        continue
                    
                    for k, v in lst.iteritems():
                        if ids.get(k):
                            ids[k]['mask_class'] |= v['mask_class']
                            ids[k]['mask_race'] |= v['mask_race']
                        else:
                            ids[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }

        # Get base skills from SkillLineAbility
        for ability_id, ability_data in self._SkillLineAbility_db.iteritems():
            mask_class_category = 0
            if ability_data.max_value > 0 or ability_data.min_value > 0:
                continue

            # Guess class based on skill category identifier
            for j in xrange(0, len(SpellDataGenerator.__skill_categories)):
                if ability_data.id_skill in SpellDataGenerator.__skill_categories[j]:
                    mask_class_category = _MASK_CLASS[j]
                    break

            spell = self._Spell_db[ability_data.id_spell]
            if not spell.id:
                continue

            # Make sure there's a class or a race
            if not ability_data.mask_class and not ability_data.mask_race and not mask_class_category:
                continue
                
            lst = self.generate_spell_filter_list(spell.id, ability_data.mask_class or mask_class_category, 0, ability_data.mask_race)
            if not lst:
                continue
                
            for k, v in lst.iteritems():
                if ids.get(k):
                    ids[k]['mask_class'] |= v['mask_class']
                    ids[k]['mask_race'] |= v['mask_race']
                else:
                    ids[k] = { 'mask_class': v['mask_class'], 'mask_race' : v['mask_race'], 'effect_list': v['effect_list'] }

        return ids
        
class TalentDataGenerator(DataGenerator):
    def filter(self):
        ids = { }

        for talent_id, talent_data in self._Talent_db.iteritems():
            talent_tab = self._TalentTab_db[talent_data.talent_tab]
            if not talent_tab.id:
                continue

            # Make sure the talent is a class  / pet associated one
            if talent_tab.mask_class      not in _MASK_CLASS and \
               talent_tab.mask_pet_talent not in _MASK_PET:
                continue

            # Make sure at least one spell id is defined
            if talent_data.id_rank_1 == 0: 
                continue

            # Just empty dict, we need nothing in generation
            ids[talent_id] = { }

        return ids
