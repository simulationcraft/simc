@echo off

echo Generating Assassination
py "..\ConvertAPL.py" -i assassination.simc -o "..\apl_rogue.cpp" -s assassination
echo Generating Outlaw
py "..\ConvertAPL.py" -i outlaw.simc -o "..\apl_rogue.cpp" -s outlaw
echo Generating Subtlety
py "..\ConvertAPL.py" -i subtlety.simc -o "..\apl_rogue.cpp" -s subtlety

echo Done!
pause >nul