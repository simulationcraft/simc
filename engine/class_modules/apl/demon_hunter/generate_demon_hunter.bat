@echo off

echo Generating Havoc
py "..\ConvertAPL.py" -i havoc.simc -o "..\apl_demon_hunter.cpp" -s havoc

echo Generating Vengeance
py "..\ConvertAPL.py" -i vengeance.simc -o "..\apl_demon_hunter.cpp" -s vengeance
py "..\ConvertAPL.py" -i vengeance_aldrachi_reaver.simc -o "..\apl_demon_hunter.cpp" -s vengeance_aldrachi_reaver
py "..\ConvertAPL.py" -i vengeance_felscarred.simc -o "..\apl_demon_hunter.cpp" -s vengeance_felscarred

echo Done!
pause >nul