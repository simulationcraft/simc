call python casc_extract.py -m batch --cdn -o wow
cd wow
dir /b /a:D /O:-D>hi.txt
set /p wowdir=<hi.txt
set wowdir=%wowdir:~4,-17%
del hi.txt
cd ..
cd ..
set curr=%cd%
cd dbc_extract
call generate.bat %wowdir% %curr%\casc_extract\wow
pause