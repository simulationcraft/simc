#!/usr/bin/env python3

import sys
import argparse
import random
try:
    from simc_support.game_data import WowSpec
    from simc_support.game_data import Covenant
    from simc_support.game_data import Conduit
    from simc_support.game_data import Trinket
    from simc_support.game_data import Legendary
    from simc_support.game_data import SoulBind
except ImportError:
    raise Exception(
        "simc-support dependency missing. Please install using 'pip3 install -r requirements.txt'")

from helper import Test, TestGroup, run, find_profiles
from talent_options import talent_combinations

FIGHT_STYLES = ('Patchwerk', 'DungeonSlice', 'HeavyMovement')


def get_talent_name(spec: WowSpec.WowSpec, talents: str):
    try:
        talent_row = next(i for i, x in enumerate(talents) if x != "0")
        talent_choice = int(talents[talent_row])
        talent = spec.talents[talent_row+1][talent_choice]
        return talent["name"]
    except StopIteration:
        return "None"
    except Exception:
        return "Unknown"


def test_talents(klass: str, path: str):
    spec = WowSpec.get_wow_spec_from_combined_simc_name(klass)
    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/talents'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for talents in talent_combinations(klass):
            talent_name = get_talent_name(spec, talents)
            Test('{:<40} ({})'.format(talent_name, talents),
                 group=grp, args=[('talents', talents)])


def test_covenants(klass: str, path: str):
    spec = WowSpec.get_wow_spec_from_combined_simc_name(klass)
    conduits = Conduit.get_conduits_for_spec(spec)
    for fight_style in FIGHT_STYLES:
        for covenant in Covenant.COVENANTS:
            grp = TestGroup('{}/{}/covenants/{}'.format(profile, fight_style, covenant.simc_name),
                            fight_style=fight_style, profile=path)
            tests.append(grp)

            Test(covenant.full_name, group=grp,
                 args=[('covenant', covenant.simc_name), ('level', 60)])
            # Add conduits specific to selected covenant
            for conduit in conduits:
                if covenant in conduit.covenants:
                    rank = random.choice(conduit.ranks)
                    soulbind_argument = '{}:{}'.format(conduit.id, rank)
                    Test('{:<40} {:<10} Rank {:>2}'.format(conduit.full_name, "({})".format(conduit.id), rank), group=grp, args=[
                        ('covenant', covenant.simc_name), ('level', 60), ('soulbind', soulbind_argument)])


def test_trinkets(klass: str, path: str):
    spec = WowSpec.get_wow_spec_from_combined_simc_name(klass)
    trinkets = Trinket.get_trinkets_for_spec(spec)
    fight_style = args.trinkets_fight_style
    grp = TestGroup('{}/{}/trinkets'.format(profile, fight_style),
                    fight_style=fight_style, profile=path)
    tests.append(grp)
    for trinket in trinkets:
        Test('{} ({})'.format(trinket.name, trinket.item_id), group=grp, args=[
            ('trinket1', '{},id={},ilevel={}'.format(trinket.name, trinket.item_id, trinket.min_itemlevel))])


def test_legendaries(klass: str, path: str):
    spec = WowSpec.get_wow_spec_from_combined_simc_name(klass)
    legendaries = Legendary.get_legendaries_for_spec(spec)
    for fight_style in FIGHT_STYLES:
        grp = TestGroup('{}/{}/legendaries'.format(profile, fight_style),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for legendary in legendaries:
            Test('{} ({} / {})'.format(legendary.full_name, legendary.id, legendary.bonus_id), group=grp, args=[
                ('trinket1', '{},bonus_id={}'.format(legendary.full_name, legendary.bonus_id))])


def test_soulbinds(klass: str, path: str):
    fight_style = args.soulbind_fight_style
    for covenant in Covenant.COVENANTS:
        grp = TestGroup('{}/{}/soulbinds/{}'.format(profile, fight_style, covenant.simc_name),
                        fight_style=fight_style, profile=path)
        tests.append(grp)
        for soulbind in SoulBind.SOULBINDS:
            if covenant == soulbind.covenant:
                for soulbind_talent in soulbind.soul_bind_talents:
                    if soulbind_talent.spell_id != 0:
                        soulbind_name = '{} - {}'.format(soulbind.full_name, soulbind_talent.full_name)
                        Test('{:<60} ({})'.format(soulbind_name, soulbind_talent.spell_id), group=grp, args=[
                            ('covenant', covenant.simc_name), ('level', 60), ('soulbind', '{},{}'.format(soulbind.simc_name, soulbind_talent.spell_id))])


available_tests = {
    "talent": test_talents,
    "covenant": test_covenants,
    "trinket": test_trinkets,
    "legendary": test_legendaries,
    "soulbind": test_soulbinds
}

parser = argparse.ArgumentParser(description='Run simc tests.')
parser.add_argument('specialization', metavar='spec', type=str,
                    help='Simc specialization in the form of CLASS_SPEC, eg. Priest_Shadow')
parser.add_argument('-tests', nargs='+', default=["talent"], required=False,
                    help='Tests to run. Available tests: {}'.format([t for t in available_tests.keys()]))
parser.add_argument('--trinkets-fight-style', default='DungeonSlice', type=str,
                    help='Fight style used for trinket simulations.')
parser.add_argument('--soulbind-fight-style', default='DungeonSlice', type=str,
                    help='Fight style used for soulbind simulations.')
parser.add_argument('--max-profiles-to-use', default='0', type=int,
                    help='Maximum number of profiles to use per spec. 0 means use all available profiles')
args = parser.parse_args()


klass = args.specialization

print(' '.join(klass.split('_')))

tests = []
profiles = list(find_profiles(klass))
if args.max_profiles_to_use != 0:
    profiles = profiles[:args.max_profiles_to_use]

if len(profiles) == 0:
    print("No profile found for {}".format(klass))

for profile, path in profiles:
    for test in args.tests:
        if test in available_tests:
            available_tests[test](klass, path)
        else:
            print("Could not find test {}".format(test))


sys.exit(run(tests))
