@echo off

echo Generating Havoc
py "..\ConvertAPL.py" -i havoc.simc -o "..\apl_demon_hunter.cpp" -s havoc

echo Generating Vengeance
py "..\ConvertAPL.py" -i vengeance.simc -o "..\apl_demon_hunter.cpp" -s vengeance

echo Done!
pause >nul