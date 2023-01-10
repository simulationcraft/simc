@echo off

echo Generating Affliction
py "..\ConvertAPL.py" -i havoc.simc -o "..\apl_warlock.cpp" -s affliction

echo Generating Demonology
py "..\ConvertAPL.py" -i vengeance.simc -o "..\apl_warlock.cpp" -s demonology

echo Generating Destruction
py "..\ConvertAPL.py" -i vengeance.simc -o "..\apl_warlock.cpp" -s destruction

echo Done!
pause >nul
