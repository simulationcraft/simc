call py -3 casc_extract.py -m batch --cdn -o wow
cd wow
dir /b /a:D /O:-D>hi.txt
set /p wowdir=<hi.txt
set /p oldname=<hi.txt
set wowdir=%wowdir:~6%
ren %oldname% %wowdir%
del hi.txt
cd ..
cd ..
set curr=%cd%
cd dbc_extract3
robocopy "C:\World of Warcraft\Cache\ADB\enUS" %cd%\cache\live DBCache.bin
call generate.bat %wowdir% %curr%\casc_extract\wow
pause