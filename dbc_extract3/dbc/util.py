import re

from dbc import constants

def class_id(**kwargs):
    pet_skill = int(kwargs.get('pet_skill', 0))
    player_skill = int(kwargs.get('player_skill', 0))
    spell_family = int(kwargs.get('family', 0))
    mask = int(kwargs.get('mask', 0))

    if player_skill > 0:
        if not hasattr(class_id, '_byskill'):
            class_id._byskill = dict()
            for v in constants.CLASS_INFO:
                class_id._byskill[v['skill']] = v['id']

        return class_id._byskill.get(player_skill, -1)

    if pet_skill > 0:
        if not hasattr(class_id, '_bypetskill'):
            class_id._bypetskill = dict()

            for id, v in enumerate(constants.PET_SKILL_CATEGORIES):
                for skill in v:
                    class_id._bypetskill[skill] = id

        return class_id._bypetskill.get(pet_skill, -1)

    if mask > 0:
        if not hasattr(class_id, '_mask_cache'):
            class_id._mask_cache = {}

        if mask in class_id._mask_cache:
            return class_id._mask_cache[mask]

        set_ = [
            info['id'] for info in constants.CLASS_INFO if mask & (1 << info['bit']) != 0
        ]

        if len(set_) == 1:
            set_ = set_[0]
        elif len(set_) == 0:
            set_ = 0

        class_id._mask_cache[mask] = set_

        return set_

    if spell_family > 0:
        if not hasattr(class_id, '_byfamily'):
            class_id._byfamily = dict()

            for v in constants.CLASS_INFO:
                class_id._byfamily[v['family']] = v['id']

        return class_id._byfamily.get(spell_family, -1)

    return -1

def class_name(**kwargs):
    class_id = int(kwargs.get('class_id', -1))

    if class_id > -1:
        if not hasattr(class_name, '_byclassid'):
            class_name._byclassid = dict()

            for v in constants.CLASS_INFO:
                class_name._byclassid[v['id']] = v['name']

        return class_name._byclassid.get(class_id, 'Unknown')

    return 'Unknown'

def is_blacklisted(**kwargs):
    spell_name = kwargs.get('spell_name', None)

    if spell_name:
        for entry in constants.SPELL_NAME_BLACKLIST:
            if entry.match(spell_name):
                return True

    return False

def race_mask(**kwargs):
    skill = kwargs.get('skill', 0)

    if skill != 0:
        if not hasattr(race_mask, '_byskill'):
            race_mask._byskill = dict()
            for v in constants.RACE_INFO:
                if v['skill'] not in race_mask._byskill:
                    race_mask._byskill[v['skill']] = 0

                race_mask._byskill[v['skill']] |= (1 << v['bit'])

        return race_mask._byskill.get(skill, 0)

    return 0

def race_id(**kwargs):
    mask = kwargs.get('mask', 0)

    if mask != 0:
        if not hasattr(race_id, '_mask_cache'):
            race_id._mask_cache = {}

        if mask in race_id._mask_cache:
            return race_id._mask_cache[mask]

        set_ = [
            info['id'] for info in constants.RACE_INFO if mask & (1 << info['bit']) != 0
        ]

        if len(set_) == 1:
            set_ = set_[0]
        elif len(set_) == 0:
            set_ = 0

        race_id._mask_cache[mask] = set_

        return set_

    return 0

def class_skills():
    if not hasattr(class_skills, '_map'):
        class_skills._map = [info['skill'] for info in constants.CLASS_INFO]

    return class_skills._map

def race_skills():
    if not hasattr(race_skills, '_map'):
        race_skills._map = [info['skill'] for info in constants.RACE_INFO]

    return race_skills._map

def tokenize(str_):
    while len(str_) > 0 and str_[0] in ['_', '+']:
        str_ = str_[1:]

    str_ = re.sub('[^A-z0-9_\+\.% ]', '', str_)
    str_ = str_.lower()
    str_ = str_.replace(' ', '_')

    return str_
