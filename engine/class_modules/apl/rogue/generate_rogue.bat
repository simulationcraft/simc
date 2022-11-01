@echo off

echo Generating Assassination
py "..\ConvertAPL.py" -i assassination.simc -o "..\apl_rogue.cpp" -s assassination
echo Generating Outlaw
py "..\ConvertAPL.py" -i outlaw.simc -o "..\apl_rogue.cpp" -s outlaw
echo Generating Subtlety
py "..\ConvertAPL.py" -i subtlety.simc -o "..\apl_rogue.cpp" -s subtlety

echo Generating Assassination DF
py "..\ConvertAPL.py" -i assassination_df.simc -o "..\apl_rogue.cpp" -s assassination_df
echo Generating Outlaw DF
py "..\ConvertAPL.py" -i outlaw_df.simc -o "..\apl_rogue.cpp" -s outlaw_df
echo Generating Subtlety DF
py "..\ConvertAPL.py" -i subtlety_df.simc -o "..\apl_rogue.cpp" -s subtlety_df

echo Done!
pause >nul