import sys,subprocess, math
import numpy as np
import xml.etree.ElementTree as ET

def main():
	simc_bin = "../engine/simc.exe"
	profile_dir="../profiles/Tier16H/"
	name = "Priest_Shadow_T16H"
	profile= name + ".simc"
	extra_options = ""
	threads=1
	output_dir = "d:/dev/simc/"

	iterations=10000
	num_repetitions=10
	list_cpu_seconds = np.empty([])
	
	for repetition in range(num_repetitions):
		command = "{bin} {profile} {eo} iterations={iterations} threads={threads} output={output} xml={xml}" \
					.format( bin=simc_bin, profile=profile_dir+profile, eo=extra_options, iterations=iterations, threads=threads, output=output_dir+profile+".txt", xml=output_dir+profile+".xml" )
					
		subprocess.call( command )

		tree = ET.parse(output_dir+profile+".xml")
		root = tree.getroot()
		cpu_seconds = float(root.find("performance").find("cpu_seconds").text)
		print( "Done simulation #{n} of {rep} with iterations={it}: cpu_seconds={t}".format(n=(repetition+1), rep=num_repetitions, it=iterations, t=cpu_seconds ) )
		list_cpu_seconds = np.append( list_cpu_seconds, cpu_seconds )
	
	print "mean: %s"%(np.mean(list_cpu_seconds))
	print "stddev: %s"%(np.std(list_cpu_seconds))
	print "stddev/sqrt(N): %s"%(np.std(list_cpu_seconds)/math.sqrt(num_repetitions))
					
if __name__ == "__main__":
	main()