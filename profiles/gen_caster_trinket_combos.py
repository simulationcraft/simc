trinkets = {"DMC"      : "relic_of_yulon,id=79331",
            "StaticC"  : "staticcasters_medallion,id=93254",
            "LightL"   : "light_of_the_cosmos,id=86792",
            "LightN"   : "light_of_the_cosmos,id=86133",
            "LightH"   : "light_of_the_cosmos,id=87065",
            "EssenceL" : "essence_of_terror,id=86907",
            "EssenceN" : "essence_of_terror,id=86388",
            "EssenceH" : "essence_of_terror,id=87175"}

reforges = ["mastery", "haste", "crit"]

header = """#!./simc

gear_hit_rating=5100
"""

def get_trinket_uniqueness(name):
    return trinkets[name].split(',')[0]

def get_trinket(slot, name, reforge):
    line = "trinket%i=%s,upgrade=2" % (slot, trinkets[name])
    if name.startswith("Light") and reforge != "haste":
        line += ",reforge=haste_%s" % reforge
    return line

for targetreforge in reforges:
    fname = "caster_trinket_combos_%s.simc" % targetreforge
    with open(fname, 'w') as output:

        print >>output, header

        names = list(trinkets.keys())
        names.sort()
        for i, trinket1 in enumerate(names):
            for trinket2 in names[i + 1:]:
                if get_trinket_uniqueness(trinket1) == get_trinket_uniqueness(trinket2):
                    continue
                
                print >>output, "copy=%s_%s" % (trinket1, trinket2)
                print >>output, get_trinket(1, trinket1, targetreforge)
                print >>output, get_trinket(2, trinket2, targetreforge)
                print >>output
