@echo off

echo Generating Assassination
py "..\ConvertAPL.py" -i assassination.simc -o "..\apl_rogue.cpp" -s assassination
echo Generating Outlaw
py "..\ConvertAPL.py" -i outlaw.simc -o "..\apl_rogue.cpp" -s outlaw
echo Generating Subtlety
py "..\ConvertAPL.py" -i subtlety.simc -o "..\apl_rogue.cpp" -s subtlety

echo Generating Assassination PTR
py "..\ConvertAPL.py" -i assassination_ptr.simc -o "..\apl_rogue.cpp" -s assassination_ptr
echo Generating Outlaw PTR
py "..\ConvertAPL.py" -i outlaw_ptr.simc -o "..\apl_rogue.cpp" -s outlaw_ptr
echo Generating Subtlety PTR
py "..\ConvertAPL.py" -i subtlety_ptr.simc -o "..\apl_rogue.cpp" -s subtlety_ptr

echo Done!
pause >nul