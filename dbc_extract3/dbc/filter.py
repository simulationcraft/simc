from collections import defaultdict
import logging

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

class RenownRewardSet(DataSet):
    def _filter(self, **kwargs):
        _renown_rewards = list()

        for entry in self.db('RenownRewards').values():
            if entry.id_spell == 0:
                continue

            if entry.ref('id_spell').id != entry.id_spell:
                continue

            _renown_rewards.append(entry)

        return _renown_rewards

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
            item = effect.child_ref('ItemXItemEffect').parent_record()
            item2 = self.db('Item')[item.id]
            if item.id == 0:
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

            items.append((item, effect.ref('id_spell'), enchant_id, item2.ref('id_crafting_quality').tier))

        return items

    def ids(self):
        return list(set(v[0] for v in self.get()))

class TraitSet(DataSet):
    # NOTE: assumption is that sub tree ID is at least 5, so there is no conflict with tree enum
    def _coord_key(self, entry, spec):
        key_1 = entry['tree']

        if key_1 == 3:
            key_1 = entry['node'].id_trait_sub_tree

        return (key_1, entry['class_'], spec)

    def _filter(self, **kwargs):
        _spec_map = dict(
            (entry.id_parent, entry.id_spec) for entry in self.db('SpecSetMember').values()
        )

        # List of SkillLineXTraitTree entries related to player skills
        _trait_skills = [
            entry for entry in self.db('SkillLineXTraitTree').values()
                if util.class_id(player_skill=entry.id_skill_line) != -1
        ]

        # Map of TraitTree entries
        _trait_trees = dict(
            (data.id_trait_tree, (data.ref('id_trait_tree'), data.id_skill_line))
                for data in _trait_skills
        )

        # Map TraitTreeNodeGroups to "tree indices" based on the trait tree currency used
        _trait_node_group_map = dict()
        for entry in self.db('TraitTreeXTraitCurrency').values():
            if entry.id_trait_tree not in _trait_trees:
                continue

            currency = entry.ref('id_trait_currency')
            if currency.id == 0:
                continue

            costs = currency.child_refs('TraitCost')
            if len(costs) == 0:
                continue

            for cost in costs:
                node_groups = cost.child_ref('TraitNodeGroupXTraitCost')
                if node_groups.id == 0:
                    continue

                index = 0
                if currency.flags == 0x4:
                    index = 1
                elif currency.flags == 0x8:
                    index = 2
                _trait_node_group_map[node_groups.id_trait_node_group] = index

        # Map of trait_node_id, node_data
        _trait_nodes = dict()

        # Map of trait_node_group_id, group_data
        _trait_node_groups = dict(
            (entry.id, {'group': entry, 'nodes': {}, 'cond': set()})
                for tree, id_skill in _trait_trees.values()
                    for entry in tree.child_refs('TraitNodeGroup')
        )
        _trait_node_groups[0] = {'nodes': {}, 'cond': set()}

        # Map TraitNode entries to TraitNodeGroups
        for data in self.db('TraitNodeGroupXTraitNode').values():
            group_id = data.id_trait_node_group
            node_id = data.id_trait_node

            if group_id not in _trait_node_groups:
                continue

            if node_id not in _trait_nodes:
                _trait_nodes[node_id] = {
                    'node': data.ref('id_trait_node'),
                    'index': data.index,
                    'cond': set(),
                    'entries': set()
                }

            _trait_node_groups[group_id]['nodes'][node_id] = _trait_nodes[node_id]

        # Add in nodes with no group
        for data in self.db('TraitNode').values():
            if data.id_trait_tree not in _trait_trees:
                continue

            node_id = data.id

            if node_id not in _trait_nodes:
                _trait_nodes[node_id] = {
                    'node': data,
                    'cond': set(),
                    'entries': set()
                }

                # Use group 0 to hold the nodes with no group
                _trait_node_groups[0]['nodes'][node_id] = _trait_nodes[node_id]

        # Collect TraitCond entries for each used TraitNodeGroup
        for data in self.db('TraitNodeGroupXTraitCond').values():
            group_id = data.id_trait_node_group
            if group_id not in _trait_node_groups:
                continue

            _trait_node_groups[group_id]['cond'].add(data.ref('id_trait_cond'))

        # Collect TraitCond entries for each used TraitNode
        for data in self.db('TraitNodeXTraitCond').values():
            node_id = data.id_trait_node

            if node_id not in _trait_nodes:
                continue

            _trait_nodes[node_id]['cond'].add(data.ref('id_trait_cond'))

        # Collect TraitNodeEntry entries for each used TraitNode
        for data in self.db('TraitNodeXTraitNodeEntry').values():
            node_id = data.id_trait_node

            if node_id not in _trait_nodes:
                continue

            # TraitNodeXTraitNodeEntry.id is needed in order to resolve any selection index clashes
            entry = (data.ref('id_trait_node_entry'), data.id)
            _trait_nodes[node_id]['entries'].add(entry)

        # A map of trait_node_entry_id, trait_data
        _traits = defaultdict(lambda: {
            'spell': None,
            'specs': set(),
            'starter': set(),
            'class_': 0,
            'node': None,
            'definition': None,
            'entry': None,
            'groups': set(),
            'tree': 0,
            'row': -1,
            'col': -1,
            'selection_index': -1,
            'req_points': 0
        })

        for group in _trait_node_groups.values():
            class_id = 0
            tree_index = 1 # If a node has no groups, assume it is in the class tree.
            if 'group' in group:
                class_id = util.class_id(player_skill=_trait_trees[group['group'].id_parent][1])
                tree_index = _trait_node_group_map.get(group['group'].id, 0)

            group_specs = set(_spec_map.get(cond.id_spec_set, 0)
                for cond in group['cond'] if cond.type == 1
            )

            group_starter = set(_spec_map.get(cond.id_spec_set, 0)
                for cond in group['cond'] if cond.type == 2
            )

            for node in group['nodes'].values():
                node_class_id = util.class_id(player_skill=_trait_trees[node['node'].id_parent][1])

                node_specs = set(_spec_map.get(cond.id_spec_set, 0)
                    for cond in node['cond'] if cond.type == 1
                )

                node_starter = set(_spec_map.get(cond.id_spec_set, 0)
                    for cond in node['cond'] if cond.type == 2
                )

                # tree selection nodes are type 3
                if node['node'].type == 3:
                    tree_index = 4
                # hero tree nodes have a non-zero TraitNode.id_trait_sub_tree
                elif node['node'].id_trait_sub_tree != 0:
                    tree_index = 3

                for entry, db2_id in node['entries']:
                    key = entry.id
                    definition = entry.ref('id_trait_definition')

                    if 'group' in group:
                        _traits[key]['groups'].add(group['group'])
                    _traits[key]['node'] = node['node']
                    _traits[key]['entry'] = entry
                    _traits[key]['definition'] = definition
                    _traits[key]['spell'] = definition.ref('id_spell')
                    _traits[key]['class_'] = class_id if class_id else node_class_id
                    _traits[key]['specs'] |= group_specs | node_specs
                    _traits[key]['starter'] |= group_starter | node_starter

                    if tree_index != 0 and _traits[key]['tree'] == 0:
                        _traits[key]['tree'] = tree_index

                    _traits[key]['req_points'] = max([_traits[key]['req_points']] + [cond.req_points for cond in (node['cond'] | group['cond'])])

                    if node['node'].type == 2:
                        # Note that it's possible to have nodes with entries that have the same selection index.
                        # In such cases it seems random which entry the game assigns as the first entry, and this
                        # can vary build by build with no changes to the underlying data. 
                        _traits[key]['selection_index'] = entry.child_ref('TraitNodeXTraitNodeEntry').index

        _coords = {}
        for entry in _traits.values():
            if entry['tree'] == 0:
                continue

            for spec in entry['specs'] | set((0,)):
                if entry['tree'] == 2 and spec == 0:
                    continue

                if entry['tree'] == 1 and spec != 0:
                    continue

                key = self._coord_key(entry, spec)

                if key not in _coords:
                    _coords[key] = {
                        0: set()
                    }

                pos_x = round(entry['node'].pos_x, -2)
                pos_y = round(entry['node'].pos_y, -2)

                # Some trees have unused nodes with negative coordinates.
                if pos_y < 0:
                    continue

                _coords[key][0].add(pos_y)
                if pos_y not in _coords[key]:
                    _coords[key][pos_y] = set()

                _coords[key][pos_y].add(pos_x)

        for v in _coords.values():
            for key, data in v.items():
                v[key] = sorted(list(data))

        """
        for tree in sorted(_coords.keys()):
            print(tree)
            treedata = _coords[tree]
            for row in sorted(treedata.keys()):
                rowdata = treedata[row]
                print(f'  {row}: {rowdata}')
        """

        for entry in _traits.values():
            for spec in entry['specs'] | set((0,)):
                if entry['tree'] == 2 and spec == 0:
                    continue

                if entry['tree'] == 1 and spec != 0:
                    continue

                key = self._coord_key(entry, spec)

                if key not in _coords:
                    continue

                pos_x = round(entry['node'].pos_x, -2)
                pos_y = round(entry['node'].pos_y, -2)

                # Some trees have unused nodes with negative coordinates.
                if pos_y < 0:
                    continue

                entry['row'] = _coords[key][0].index(pos_y) + 1
                entry['col'] = _coords[key][pos_y].index(pos_x) + 1

        return _traits

    def ids(self):
        return [ v['spell'].id for v in self.get().values() ]

class PermanentEnchantItemSet(DataSet):
    def _get_enchant_effect(self, spell_id, effect_type):
        _spell = self.db('SpellName')[spell_id]
        if _spell.id == 0:
            return None

        _spell_effects = _spell.children('SpellEffect')
        _effects = [e.type == effect_type for e in _spell_effects]

        if True not in _effects:
            return None

        return _spell_effects[_effects.index(True)]

    def _get_item_enchant(self, item_id):
        _item = self.db('ItemSparse')[item_id]
        if _item.id == 0:
            return None, None

        for effect_map_ref in _item.children('ItemXItemEffect'):
            _effect = self.get_unranked_enchant_effect(effect_map_ref.ref('id_item_effect').id_spell)
            if _effect is None:
                continue

            _enchant = self.db('SpellItemEnchantment')[_effect.misc_value_1]
            if _enchant.id == 0:
                continue

            _enchant_sei = _spell.child_ref('SpellEquippedItems')
            if _enchant_sei.id == 0:
                continue

            return _enchant, _enchant_sei

        return None, None

    def _filter(self, **kwargs):
        items = set()

        # Professions + "Runeforging"
        _skill_line_ids = [ 164, 165, 171, 197, 202, 333, 755, 773, 960 ]
        _spells = []

        enchants = {}

        for skill in self.db('SkillLineAbility').values():
            if skill.id_skill not in _skill_line_ids:
                continue

            enchant_spell = skill.ref('id_spell')
            if enchant_spell.id == 0:
                continue

            tokenized_name = util.tokenize(enchant_spell.name)

            unranked_effect = self._get_enchant_effect(enchant_spell.id, 53)
            unranked_item = self._get_enchant_effect(enchant_spell.id, 24)
            ranked_effect = self._get_enchant_effect(enchant_spell.id, 301)
            ranked_item = self._get_enchant_effect(enchant_spell.id, 288)

            if unranked_effect is not None:
                _enchant = self.db('SpellItemEnchantment')[unranked_effect.misc_value_1]
                if _enchant.id == 0:
                    continue

                _enchant_sei = enchant_spell.child_ref('SpellEquippedItems')
                if _enchant_sei.id == 0:
                    continue

                _key = (tokenized_name, 0, _enchant.id, _enchant_sei.item_class,
                        _enchant_sei.mask_inv_type, _enchant_sei.mask_sub_class)

                if _key not in enchants:
                    enchants[_key] = (_enchant, _enchant_sei)

            elif unranked_item is not None:
                for item_effect_map_ref in unranked_item.ref('item_type').children('ItemXItemEffect'):
                    _spell = item_effect_map_ref.ref('id_item_effect').ref('id_spell')
                    if _spell.id == 0:
                        continue

                    _effect = self._get_enchant_effect(_spell.id, 53)
                    if _effect is None:
                        continue

                    _enchant = self.db('SpellItemEnchantment')[_effect.misc_value_1]
                    if _enchant.id == 0:
                        continue

                    _enchant_sei = _spell.child_ref('SpellEquippedItems')
                    if _enchant_sei.id == 0:
                        continue

                    _key = (tokenized_name, 0, _enchant.id, _enchant_sei.item_class,
                            _enchant_sei.mask_inv_type, _enchant_sei.mask_sub_class)

                    if _key not in enchants:
                        enchants[_key] = (_enchant, _enchant_sei)

            elif ranked_effect is not None:
                _crafting_data = self.db('CraftingData')[ranked_effect.misc_value_1]
                if _crafting_data.id == 0:
                    continue

                for _entry in _crafting_data.children('CraftingDataEnchantQuality'):
                    _enchant = _entry.ref('id_spell_item_enchantment')
                    if _enchant.id == 0:
                        continue

                    _enchant_sei = enchant_spell.child_ref('SpellEquippedItems')
                    if _enchant_sei.id == 0:
                        continue

                    _key = (tokenized_name, _entry.rank, _enchant.id, _enchant_sei.item_class,
                            _enchant_sei.mask_inv_type, _enchant_sei.mask_sub_class)

                    if _key not in enchants:
                        enchants[_key] = (_enchant, _enchant_sei)

            elif ranked_item is not None:
                _crafting_data = self.db('CraftingData')[ranked_item.misc_value_1]
                if _crafting_data.id == 0:
                    continue

                for entry in _crafting_data.children('CraftingDataItemQuality'):
                    _item2 = self.db('Item')[entry.id_item]
                    if _item2.id == 0:
                        continue

                    for item_effect_map_ref in entry.ref('id_item').children('ItemXItemEffect'):
                        _spell = item_effect_map_ref.ref('id_item_effect').ref('id_spell')
                        _spell_effects = _spell.children('SpellEffect')
                        _effects = [e.type == 53 for e in _spell_effects]

                        if True not in _effects:
                            continue

                        _id_value = _spell_effects[_effects.index(True)].misc_value_1
                        _enchant = self.db('SpellItemEnchantment')[_id_value]
                        if _enchant.id == 0:
                            continue

                        _enchant_sei = _spell.child_ref('SpellEquippedItems')
                        if _enchant_sei.id == 0:
                            continue

                        _key = (tokenized_name, _item2.id_crafting_quality, _enchant.id, _enchant_sei.item_class,
                                _enchant_sei.mask_inv_type, _enchant_sei.mask_sub_class)

                        if _key not in enchants:
                            enchants[_key] = (_enchant, _enchant_sei)

        return [k + v for k, v in enchants.items()]

    def ids(self):
        return list(set(v[0] for v in self.get()))

class ExpectedStatModSet(DataSet):
    def _filter(self, **kwargs):
        mods = list()

        # grab all raid maps reverse sorted by expansion id
        maps = [
            m for m in sorted(self.db('Map').values(), key = lambda e: e.id_expansion, reverse = True)
                if m.instance_type == 2 # raid instance type
        ]

        # assume the latest expansion has the highest expansion id
        xpac = maps[0].id_expansion
        maps = [m for m in maps if m.id_expansion == xpac]

        # assume the latest raid has the highest order index in JournalTierXInstance for the map's JournalInstan ce
        maps.sort(key = lambda e: e.child_ref('JournalInstance').child_refs('JournalTierXInstance')[0].order, reverse = True)
        map_id = maps[0].id

        # find all the difficulties for the map & m+
        content_ids = []
        dungeon_id = []

        for c in self.db('MapDifficulty').values():
            if c.id_parent == map_id:
                id_ct = c.id_content_tuning

                # check ConditionalContentTuning.db2 for fated redirects
                for cct in self.db('ConditionalContentTuning').values():
                    if cct.flag == 8 and cct.id_parent == id_ct:
                        id_ct = cct.id_content_tuning
                        break

                content_ids.append([id_ct, c.difficulty])

            # NOTE: so far M+ content tuning ID has always been 1279, but we calculate it out just in case
            # item context 16 is for M+ dungeons
            elif len(dungeon_id) == 0 and c.item_context == 16 and c.parent_record().id_expansion == xpac:
                dungeon_id = [c.id_content_tuning, c.difficulty]

        # find all mod_ids that match the content tuning ids
        mod_ids = []
        mod_d_ids = []

        for e in self.db('ContentTuningXExpected').values():
            for c in content_ids:
                if e.id_parent == c[0]:
                    mod_ids.append([e, c[1]])
                    break
            else:
                # check if it's m+ season for the current expansion
                if e.id_parent == dungeon_id[0] and e.ref('id_mythic_plus_season').id_expansion == xpac:
                    mod_d_ids.append([e, dungeon_id[1]])

        # assume highest mythic plus season id is the current season
        mod_d_ids.sort(key = lambda e: e[0].id_mythic_plus_season, reverse = True)
        mod_d_ids = [
            e for e in mod_d_ids
                if e[0].id_mythic_plus_season == mod_d_ids[0][0].id_mythic_plus_season
        ]

        mod_ids += mod_d_ids

        # fill in results
        for entry in self.db('ExpectedStatMod').values():
            for m in mod_ids:
                if entry.id == m[0].id_expected_stat_mod:
                    mods.append([entry, m[1]])

        return mods

    def ids(self, **kwargs):
        return list(set(v[0].id for v in self.get()))

class EmbellishmentSet(DataSet):
    def _filter(self, **kwargs):
        _data = []

        emb_mcrs = [
            e.id for e in self.db('ModifiedCraftingReagentSlot').values()
                if e.name == "Add Embellishment"
        ]

        emb_mcc = [
            e.id_modified_crafting_category for e in self.db('MCRSlotXMCRCategory').values()
                if e.id_parent in emb_mcrs
        ]

        emb_mcri = [
            e for e in self.db('ModifiedCraftingReagentItem').values()
                if e.id_modified_crafting_category in emb_mcc
        ]

        for mcri in emb_mcri:
            bonus_list = [
                b.ref('id_node').child_ref('ItemBonus') for b in mcri.ref('id_tree').children('ItemBonusTreeNode')
            ]

            if len(bonus_list) == 0:
                continue

            for bonus in bonus_list:
                if bonus.type == 23:
                    _name = mcri.child_refs('ItemSparse')[-1].name
                    _bonus_id = bonus.id_node
                    _effect_id = bonus.val_1

                    _data.append([f'"{_name}"', _bonus_id, _effect_id])

        return _data

    def ids(self, **kwargs):
        return list(set(v[1] for v in self.get()))

class CharacterLoadoutSet(DataSet):
    def _filter(self, **kwargs):
        _data = []

        for cli in self.db('CharacterLoadoutItem').values():
            loadout = cli.ref('id_loadout')
            if loadout.purpose != 14:
                continue

            item = cli.ref('id_item')
            if item.classs == 2 or item.classs == 4:
                _data.append([cli, loadout])

        _data.sort(key = lambda e: (e[1].id_class, e[1].id, e[0].id_item ))

        return _data

    def ids(self, **kwargs):
        return (list(set(v[0].id for v in self.get())))

class TraitLoadoutSet(DataSet):
    def _filter(self, **kwargs):
        _data = []
        loadouts = {}

        for e in self.db('TraitTreeLoadout').values():
            loadouts[e.id_spec] = e.id

        for e in self.db('TraitTreeLoadoutEntry').values():
            if e.id_trait_tree_loadout in loadouts.values():
                _data.append(e)

        return _data

    def ids(self, **kwargs):
        return (list(set(v.id for v in self.get())))
