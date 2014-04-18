# This script is intended to easily allow checking of many sim setups in debug to catch bugs.

import subprocess as sb
import time

simc_bin = "simc.exe"
iterations=4
threads=1
output_dir="d:/dev/simc/debug/"
classes = [
           #("priest", ["shadow","discipline","holy"]),
           ("priest", ["shadow",]),
           ("shaman", ["elemental", "enhancement", "restoration"]),
           #("druid",["guardian","feral","balance","restoration"]),
           ("druid",["guardian","feral","balance"]),
           ("warrior",["fury","arms","protection"]),
           ("paladin",["holy","retribution","protection"]),
           ("rogue",["subtlety","combat","assassination"]),
           ("warlock",["demonology","affliction","destruction"]),
           #("monk",["brewmaster","windwalker","mistweaver"]),
           ("monk",["brewmaster","windwalker"]),
           ]

for wowclass in classes:
    for spec in wowclass[1]:
        try:
            command = "{bin} {wclass}=foo spec={spec} iterations={iterations} threads={threads} output={output}" \
            .format( bin=simc_bin, wclass=wowclass[0], spec=spec, iterations=iterations, threads=threads, output=output_dir+ wowclass[0] + "_" + spec + ".txt" )
            print( "Simulating {wclass} spec {spec}".format(wclass=wowclass[0], spec=spec) )
            print( command )
            time.sleep(0.1)
            sb.check_call( command )
            time.sleep(2)
        except sb.CalledProcessError as e:
            print ("Exited with non-zero return code: " + str(e.returncode) )
