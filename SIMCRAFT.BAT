cd /d "%~d1%~p1"
simcraft.exe max_time=300 optimal_raid=1 input=%~n1%~x1 output=%~n1.txt html=%~n1.html
notepad %~n1.txt
