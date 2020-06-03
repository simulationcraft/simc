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

