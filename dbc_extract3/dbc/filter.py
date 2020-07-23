from dbc import db, util, constants

class DataSet:
    def __init__(self, opts):
        self._options = opts

    def db(self, dbname):
        return db.datastore(self._options).get(dbname)

    def ids(self):
        raise NotImplementedError

    def filter(self, **kwargs):
        raise NotImplementedError

class RacialSpellSet(DataSet):
    def filter(self, **kwargs):
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

    def ids(self):
        return list(set(v.id_spell for v in self.filter()))

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

    def filter(self, **kwargs):
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
            if data.id_skill == 0 or data.id_skill not in constants.CLASS_SKILL_CATEGORIES:
                continue

            if data.unk_13 in [3]:
                continue

            if not self.valid_spell(data, data.ref('id_spell')):
                continue

            for effect in data.ref('id_spell').children('SpellEffect'):
                trigger_spells = self.get_trigger_spells(effect, trigger_spells)

            if data.id_spell in trigger_spells:
                continue

            class_id = constants.CLASS_SKILL_CATEGORIES.index(data.id_skill)
            entry = (class_id, data.ref('id_spell'), data.ref('id_replace'))

            if entry not in _data:
                _data.append(entry)

        return _data

    def ids(self):
        ids = set()
        for v in self.filter():
            ids.add(v[1].id)
            if v[1].id != 0:
                ids.add(v[2].id)

        return list(ids)

class PetActiveSpellSet(ActiveClassSpellSet):
    def filter(self, **kwargs):
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

    def ids(self):
        return list(set(v[1] for v in self.filter()))

