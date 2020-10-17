__max_tiers = 7
__talents_per_tier = 3
__class_map = {
    'Death_Knight': [
        (0, -1),        # 56: All
        (1, -1),        # 57: All
        (2, -1),        # 58: All
                        # 60: None
                        # 75: None
        (5, -1),        # 90: All
        (6, -1),        # 100: All
    ],
    'Demon_Hunter': [
        (0, -1),        # 99: All
        (1, -1),        # 100: All
        (2, -1),        # 102: All
                        # 104: None
        (4, -1),        # 106: All
        (5, -1),        # 108: All
        (6, -1),        # 110: All
    ],
    'Druid': [
        (0, -1),        # 15: All
                        # 30: None
        (2, -1),        # 45: All
                        # 60: None
        (4, -1),        # 75: All
        (5, -1),        # 90: All
        (6, -1),        # 100: All
    ],
    'Hunter': [
        (0, -1),        # 15: All
        (1, -1),        # 30: All
                        # 45: None
        (3, -1),        # 60: All
                        # 75: None
        (5, -1),        # 90: All
        (6, -1),        # 100: All
    ],
    'Mage': [
        (0, -1),        # 15: All
                        # 30: None
        (2, -1),        # 45: All
        (3, -1),        # 60: All
                        # 75: None
        (5, -1),        # 90: All
        (6, -1),        # 100: All
    ],
    'Monk': [
        (0, -1),        # 15: All
                        # 30: None
        (2, -1),        # 45: All
        (3, 1),         # 60: Just Good Karma
                        # 75: None
        (5, -1),        # 90: All
        (6, -1),        # 100: All
    ],
    'Paladin': [
        (0, -1),        # 15: All
        (1, -1),        # 30: All
                        # 45: None
        (3, -1),        # 60: All
        (4, -1),        # 75: All
        (5, 1),         # 90: Justicar's Vengeance
        (6, -1),        # 100: All
    ],
    'Priest': [
        (0, -1),        # 15: All
        (1,  1),        # 30: San'layn
        (2, -1),        # 45: All
                        # 60: None
        (4, -1),        # 75: All
        (5, -1),        # 90: All
        (6, -1),        # 100: All
    ],
    'Rogue': [
        (0, -1),        # 15: All
        (1, -1),        # 30: All
        (2, -1),        # 45: All
                        # 60: None
                        # 75: None
        (5, -1),        # 90: All
        (6, -1),        # 100: All
    ],
    'Shaman': [
        (0, -1),        # 15: All
        (1, -1),        # 30: All
                        # 45: None
        (3, -1),        # 60: All
                        # 75: None
        (5, -1),        # 90: All
        (6, -1),        # 100: All
    ],
    'Warlock': [
        (0, -1),        # 15: All
        (1, -1),        # 30: All
                        # 45: None
        (3, -1),        # 60: All
                        # 75: None
        (5, -1),        # 90: All
        (6, -1),        # 100: All
    ],
    'Warrior': [
        (0, -1),        # 15: All
        (1, -1),        # 30: All
        (2, -1),        # 45: All
                        # 60: None
        (4, -1),        # 75: All
        (5, -1),        # 90: All
        (6, -1),        # 90: All
    ]
}

def talent_combinations(klass):
    if klass not in __class_map:
        klass_split = klass.split('_')
        for i in range(len(klass_split)):
            klass = '_'.join(klass_split[0:i])
            if klass in __class_map:
                break

    # Check untalented
    combinations = [ '0' * __max_tiers ]

    for talents in __class_map[klass]:
        talent_arr = [ '0' ] * __max_tiers
        if talents[1] == -1:
            for talent in range(0, __talents_per_tier):
                talent_arr[talents[0]] = str(talent + 1)
                talent_str = ''.join(talent_arr)

                if talent_str not in combinations:
                    combinations.append(talent_str)
        else:
            talent_arr[talents[0]] = str(talents[1])
            combinations.append(''.join(talent_arr))

    return combinations
