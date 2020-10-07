#!/usr/bin/env python3

import sys
import argparse

from helper import Test, TestGroup, run, find_profiles
from talent_options import talent_combinations

FIGHT_STYLES = ('Patchwerk', 'DungeonSlice', 'HeavyMovement')
COVENANTS = ('Kyrian', 'Venthyr', 'Night_Fae', 'Necrolord')

parser = argparse.ArgumentParser(description='Run simc tests')
parser.add_argument('specialization', metavar='spec', type=str,
                    help='Simc specialization in the form of CLASS_SPEC, eg. Priest_Shadow')
parser.add_argument('--no-trinkets', default=False, action='store_true',
                    help='Disable trinket tests. Trinket tests require simc-support library to be installed.')
parser.add_argument('--trinkets-fight-style', default='DungeonSlice', type=str,
                    help='Fight style used for trinket simulations.')
args = parser.parse_args()

klass = args.specialization

print(' '.join(klass.split('_')))

tests = []
for profile, path in find_profiles(klass):
    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/talents'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for talents in talent_combinations(klass):
            Test(talents, group=grp, args=[('talents', talents)])

    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/covenants'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for covenant in COVENANTS:
            Test(covenant, group=grp,
                 args=[('covenant', covenant.lower()), ('level', 60)])

    if not args.no_trinkets:
        from simc_support.game_data.WowSpec import get_wow_spec_from_combined_simc_name
        from simc_support.game_data.Trinket import get_trinkets_for_spec

        trinkets = get_trinkets_for_spec(
            get_wow_spec_from_combined_simc_name(klass))
        fight_style = args.trinkets_fight_style
        grp = TestGroup('{}/{}/trinkets'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for trinket in trinkets:
            Test('{} ({})'.format(trinket.name, trinket.item_id), group=grp, args=[
                    ('trinket1', '{},id={},ilevel={}'.format(trinket.name, trinket.item_id, trinket.min_itemlevel))])
else:
    print("No profile found for {}".format(klass))

sys.exit(run(tests))
