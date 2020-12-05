from dbc import db, util, constants

# Simple cache for raw filter results
_CACHE = { }

class DataSet:
    def __init__(self, opts):
        self._options = opts

    def _filter(self, **kwargs):
        raise NotImplementedError

    def db(self, dbname):
        return db.datastore(self._options).get(dbname)

    def ids(self, **kwargs):
        raise NotImplementedError

    def get(self, **kwargs):
        if not self.__class__.__name__ in _CACHE:
            _CACHE[self.__class__.__name__] = self._filter(**kwargs)

        return _CACHE[self.__class__.__name__]

class RacialSpellSet(DataSet):
    def _filter(self, **kwargs):
        _data = []

        for v in self.db('SkillLineAbility').values():
            if v.id_parent not in util.race_skills():
                continue

            if v.ref('id_spell').id == 0:
                continue

            if util.is_blacklisted(spell_name = v.ref('id_spell').name):
                continue

            _data.append(v)

        return _data

    def ids(self, **kwargs):
        return list(set(v.id_spell for v in self.get()))

class ActiveClassSpellSet(DataSet):
    def get_trigger_spells(self, effect, set_):
        if effect.trigger_spell == 0:
            return set_

        for e in effect.ref('trigger_spell').children('SpellEffect'):
            if e.trigger_spell not in set_:
                set_ = self.get_trigger_spells(e, set_)

        set_.add(effect.trigger_spell)
        return set_

    def valid_misc(self, skill_line_ability, spell):
        misc = spell.children('SpellMisc')

        if len(misc) == 0:
            return False

        m = misc[0]
        if m.flags_1 & 0x40:
            return False

        if m.flags_1 & 0x80:
            return False

        if skill_line_ability and skill_line_ability.unk_13 not in [2, 4]:
            return False

        return True

    def valid_spell(self, skill_line_ability, spell):
        if util.is_blacklisted(spell_name = spell.name):
            return False

        return self.valid_misc(skill_line_ability, spell)

    def _filter(self, **kwargs):
        trigger_spells = set()
        _data = []

        for data in self.db('SpecializationSpells').values():
            if data.ref('spec_id').class_id == 0:
                continue

            if not self.valid_spell(None, data.ref('spell_id')):
                continue

            for effect in data.ref('spell_id').children('SpellEffect'):
                trigger_spells = self.get_trigger_spells(effect, trigger_spells)

            if data.spell_id in trigger_spells:
                continue

            _data += [(data.ref('spec_id'), data.ref('spell_id'), data.ref('replace_spell_id'))]

        for data in self.db('SkillLineAbility').values():
            if data.id_skill == 0 or data.id_skill not in util.class_skills():
                continue

            if data.unk_13 in [3]:
                continue

            if not self.valid_spell(data, data.ref('id_spell')):
                continue

            for effect in data.ref('id_spell').children('SpellEffect'):
                trigger_spells = self.get_trigger_spells(effect, trigger_spells)

            if data.id_spell in trigger_spells:
                continue

            entry = (
                util.class_id(player_skill=data.id_skill),
                data.ref('id_spell'),
                data.ref('id_replace')
            )

            if entry not in _data:
                _data.append(entry)

        for active_spell in constants.ACTIVE_SPELL_WHITELIST:
            spell = self.db('SpellName')[active_spell]
            if spell.id != active_spell:
                continue

            class_id = util.class_id(family = spell.child_ref('SpellClassOptions').family)
            if class_id == -1:
                continue

            entry = (class_id, spell, self.db('SpellName').default())

            if entry not in _data:
                _data.append(entry)

        return _data

    def ids(self, **kwargs):
        ids = set()
        for v in self.get():
            ids.add(v[1].id)
            if v[1].id != 0:
                ids.add(v[2].id)

        return list(ids)

class PetActiveSpellSet(ActiveClassSpellSet):
    def _filter(self, **kwargs):
        trigger_spells = set()
        _data = []
        specialization_spells = [
            e.spell_id for e in self.db('SpecializationSpells').values() if e.parent_record().id > 0
        ]

        pet_skill_categories = [v for cls_ in constants.PET_SKILL_CATEGORIES for v in cls_]

        for data in self.db('SkillLineAbility').values():
            if data.unk_13 in [3]:
                continue

            class_id = util.class_id(pet_skill = data.id_skill)
            if class_id == -1:
                continue

            if not self.valid_spell(data, data.ref('id_spell')):
                continue

            if data.id_spell in trigger_spells:
                continue

            if data.id_spell in specialization_spells:
                continue

            for effect in data.ref('id_spell').children('SpellEffect'):
                trigger_spells = self.get_trigger_spells(effect, trigger_spells)

            entry = (class_id, data.id_spell)

            if len([v[1] for v in _data if v[0] == class_id and v[1] == data.id_spell]) == 0:
                _data.append(entry)

        return _data

    def ids(self, **kwargs):
        return list(set(v[1] for v in self.get()))

class MasterySpellSet(DataSet):
    def _filter(self, **kwargs):
        _data = []

        for v in self.db('ChrSpecialization').values():
            if v.ref('id_mastery_1').id == 0:
                continue

            _data.append(v)

        return _data

    def ids(self, **kwargs):
        return list(set(v.id_mastery_1 for v in self.get()))

class RankSpellSet(DataSet):
    def _filter(self, **kwargs):
        _data = []

        for data in self.db('SpecializationSpells').values():
            if data.ref('spec_id').class_id == 0:
                continue

            spell_text = self.db('Spell')[data.spell_id]
            if 'Rank' not in str(spell_text.rank):
                continue

            _data += [(
                data.ref('spec_id').class_id,
                data.spec_id, data.ref('spell_id'),
                data.ref('replace_spell_id')
            )]

        for data in self.db('SkillLineAbility').values():
            if data.id_skill == 0 or data.id_skill not in util.class_skills():
                continue

            if data.unk_13 in [3]:
                continue

            spell_text = self.db('Spell')[data.id_spell]
            if 'Rank' not in str(spell_text.rank):
                continue

            entry = (
                util.class_id(player_skill=data.id_skill),
                0,
                data.ref('id_spell'),
                data.ref('id_replace')
            )

            if entry not in _data:
                _data.append(entry)

        return _data

# Master list of all conduits (spells - used for indexing, ids used for option stuff)
class ConduitSet(DataSet):
    def _filter(self, **kwargs):
        _conduits = set()

        for data in self.db('SoulbindConduitRank').values():
            if data.ref('id_spell').id != 0:
                _conduits.add((data.ref('id_spell'), data.id_parent))

        return list(_conduits)

    def ids(self, **kwargs):
        return list(set(v[0].id for v in self.get()))

class SoulbindAbilitySet(DataSet):
    def _filter(self, **kwargs):
        _soulbinds = set()

        for entry in self.db('Soulbind').values():
            for talent in entry.ref('id_garr_talent_tree').child_refs('GarrTalent'):
                for rank in talent.children('GarrTalentRank'):
                    if rank.id_spell == 0:
                        continue

                    if rank.ref('id_spell').id == rank.id_spell:
                        _soulbinds.add((rank, entry))

        return list(_soulbinds)

    def ids(self):
        return list(set(v[0].id_spell for v in self.get()))

class CovenantAbilitySet(DataSet):
    def _filter(self, **kwargs):
        _covenant_abilities = list()

        for entry in self.db('UICovenantAbility').values():
            if entry.id_spell == 0:
                continue

            if entry.ref('id_spell').id != entry.id_spell:
                continue

            _covenant_abilities.append(entry)

        return _covenant_abilities

    def ids(self):
        return list(set(v.id_spell for v in self.get()))

class TalentSet(DataSet):
    def _filter(self, **kwargs):
        talents = list()

        for entry in self.db('Talent').values():
            if entry.id_spell == 0:
                continue

            if entry.ref('id_spell').id != entry.id_spell:
                continue

            talents.append(entry)

        return talents

class TemporaryEnchantItemSet(DataSet):
    def _filter(self, **kwargs):
        items = list()

        for effect in self.db('ItemEffect').values():
            if effect.parent_record().id == 0:
                continue

            spell_effects = effect.ref('id_spell').children('SpellEffect')

            temporary_enchant_effects = [
                e.type == 54 for e in spell_effects
            ]

            # Skip any temporary enchants that modify skills, they are (for our
            # purposes) fishing temp enchants, and useless
            if True not in temporary_enchant_effects:
                continue

            enchant_id = spell_effects[temporary_enchant_effects.index(True)].misc_value_1
            enchant = self.db('SpellItemEnchantment')[enchant_id]
            if enchant.id == 0:
                continue

            # Collect enchant spells
            enchant_spells = []
            for index in range(1, 4):
                type_ = getattr(enchant, 'type_{}'.format(index))
                prop_ = getattr(enchant, 'id_property_{}'.format(index))
                if type_ in [1, 3, 7] and prop_:
                    spell = self.db('SpellName')[prop_]
                    if spell.id != 0:
                        enchant_spells.append(self.db('SpellName')[prop_])

            # Skip any enchant that has a spell that modifies a skill
            mod_skill_effects = [
                e.sub_type == 30 for s in enchant_spells for e in s.children('SpellEffect')
            ]

            if True in mod_skill_effects:
                continue

            items.append((effect.parent_record(), effect.ref('id_spell'), enchant_id))

        return items

    def ids(self):
        return list(set(v[0] for v in self.get()))

