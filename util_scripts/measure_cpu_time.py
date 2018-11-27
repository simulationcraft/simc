#!/usr/bin/python
import sys
import subprocess
import math

import numpy as np
import json


def main():
    simc_bin = "../engine/simc"
    name = "T22_Raid"
    profile = name + ".simc"
    extra_options = "deterministic=1"
    threads = 1
    output_dir = "/tmp"

    iterations = 100
    num_repetitions = 100
    list_cpu_seconds = []

    for repetition in range(num_repetitions):
        json_file = output_dir + profile + ".json"
        command = "{bin} {profile} {eo} iterations={iterations} threads={threads} output={output} json={json}".format(bin=simc_bin, profile=profile, eo=extra_options, iterations=iterations, threads=threads, output=output_dir + profile + ".txt", json=json_file)
        command = command.split(" ")
        print("Calling cmd={}".format(command))
        subprocess.call(command)

        with open(json_file) as f:
            data = json.load(f)
            cpu_seconds = float(data["sim"]["statistics"]["elapsed_time_seconds"])
            print("Done simulation #{n} of {rep} with iterations={it}: cpu_seconds={t}".format(n=(repetition + 1), rep=num_repetitions, it=iterations, t=cpu_seconds))
            list_cpu_seconds.append(cpu_seconds)

    print("mean: %s" % (np.mean(list_cpu_seconds)))
    print("stddev: %s" % (np.std(list_cpu_seconds)))
    print("stddev/sqrt(N): %s" % (np.std(list_cpu_seconds) / math.sqrt(num_repetitions)))

if __name__ == "__main__":
    main()
