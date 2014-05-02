call simc.exe Raid_T15H.simc threads=4 iterations=10 > log.txt
call simc.exe Raid_T15N.simc threads=4 iterations=10 > log.txt
call simc.exe Raid_T15H.simc optimal_raid=0 threads=4 iterations=10 > log.txt
call simc.exe Raid_T15N.simc optimal_raid=0 threads=4 iterations=10 > log.txt

call simc.exe ptr=1 Raid_T15H.simc optimal_raid=0 threads=4 iterations=10 > log.txt
call simc.exe ptr=1 Raid_T15N.simc optimal_raid=0 threads=4 iterations=10 > log.txt
call simc.exe ptr=1 Raid_T15H.simc optimal_raid=0 threads=4 iterations=10 > log.txt
call simc.exe ptr=1 Raid_T15N.simc optimal_raid=0 threads=4 iterations=10 > log.txt