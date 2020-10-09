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
parser.add_argument('--test-trinkets', default=False, action='store_true',
                    help='Test trinkets. Trinket tests require simc-support library to be installed.')
parser.add_argument('--trinkets-fight-style', default='DungeonSlice', type=str,
                    help='Fight style used for trinket simulations.')
args = parser.parse_args()

def test_trinkets(klass : str, path : str):
    if not args.test_trinkets:
        return
    try:
        from simc_support.game_data.WowSpec import get_wow_spec_from_combined_simc_name
        from simc_support.game_data.Trinket import get_trinkets_for_spec
    except ImportError:
        print("Error importing simc-support library for trinket test. Skipping test.")
        return

    trinkets = get_trinkets_for_spec(
        get_wow_spec_from_combined_simc_name(klass))
    fight_style = args.trinkets_fight_style
    grp = TestGroup('{}/{}/trinkets'.format(profile, fight_style),
                    fight_style=fight_style, profile=path)
    tests.append(grp)
    for trinket in trinkets:
        Test('{} ({})'.format(trinket.name, trinket.item_id), group=grp, args=[
                ('trinket1', '{},id={},ilevel={}'.format(trinket.name, trinket.item_id, trinket.min_itemlevel))])

klass = args.specialization

print(' '.join(klass.split('_')))

tests = []
profiles = list(find_profiles(klass))
if len(profiles) == 0:
    print("No profile found for {}".format(klass))

for profile, path in profiles:
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

    test_trinkets(klass, path)


sys.exit(run(tests))
