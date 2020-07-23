from dbc import constants

def class_id(**kwargs):
    pet_skill = int(kwargs.get('pet_skill', 0))
    player_skill = int(kwargs.get('player_skill', 0))

    if player_skill > 0:
        try:
            return constants.CLASS_SKILL_CATEGORIES.index(player_skill)
        except:
            return -1
    elif pet_skill > 0:
        for idx in range(0, len(constants.PET_SKILL_CATEGORIES)):
            if pet_skill in constants.PET_SKILL_CATEGORIES[idx]:
                return idx

    return -1

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

def race_skills():
    if not hasattr(race_skills, '_map'):
        race_skills._map = [info['skill'] for info in constants.RACE_INFO]

    return race_skills._map

