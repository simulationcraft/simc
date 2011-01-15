set dir=simc-403-22-win32
set rev=r6289

svn checkout https://simulationcraft.googlecode.com/svn/branches/cataclysm/ %dir% --username natehieter --revision %rev%

mkdir %dir%\imageformats

xcopy *.dll %dir%
xcopy /I imageformats %dir%\imageformats
xcopy vcredist_x86.exe %dir%
xcopy Win32OpenSSL_Light-0_9_8m.exe %dir%

xcopy simc.exe %dir%
xcopy SimulationCraft.exe %dir%