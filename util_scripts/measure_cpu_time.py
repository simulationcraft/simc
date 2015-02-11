#!/usr/bin/python
import sys,subprocess, math
import numpy as np
import xml.etree.ElementTree as ET

def main():
	simc_bin = "../engine/simc.exe"
	#profile_dir="../profiles/Tier16H/"
	name = "Raid_T17P"
	profile= name + ".simc"
	extra_options = "deterministic_rng=1"
	threads=1
	output_dir = "d:/dev/simc/"

	iterations=100
	num_repetitions=20
	list_cpu_seconds = []
	
	for repetition in range(num_repetitions):
		command = "{bin} {profile} {eo} iterations={iterations} threads={threads} output={output} xml={xml}" \
					.format( bin=simc_bin, profile=profile, eo=extra_options, iterations=iterations, threads=threads, output=output_dir+profile+".txt", xml=output_dir+profile+".xml" )
					
		subprocess.call( command )

		tree = ET.parse(output_dir+profile+".xml")
		root = tree.getroot()
		cpu_seconds = float(root.find("performance").find("cpu_seconds").text)
		print( "Done simulation #{n} of {rep} with iterations={it}: cpu_seconds={t}".format(n=(repetition+1), rep=num_repetitions, it=iterations, t=cpu_seconds ) )
		list_cpu_seconds.append( cpu_seconds )
	
	print "mean: %s"%(np.mean(list_cpu_seconds))
	print "stddev: %s"%(np.std(list_cpu_seconds))
	print "stddev/sqrt(N): %s"%(np.std(list_cpu_seconds)/math.sqrt(num_repetitions))
					
if __name__ == "__main__":
	main()