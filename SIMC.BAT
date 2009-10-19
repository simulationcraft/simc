cd /d "%~d1%~p1"
simc.exe max_time=300 optimal_raid=1 input=%~n1%~x1 output=%~n1.txt html=%~n1.html iterations=1000 threads=2
notepad %~n1.txt