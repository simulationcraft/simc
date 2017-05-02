:: Example script that will work on a computer with qt 5.6 + vs2015
:: See: https://github.com/simulationcraft/simc/wiki/HowToBuild#alternate-way-for-microsoft-windows
:: These are default paths, if installed to non-default paths enter those in instead.
:: Change 5.6.2\5.6 to 5.x.y\5.x if you are using qt 5.x.y instead.
:: Change win32-msvc2015 to win32-msvc2013 and 14.0 to 12.0 for vs2013
:: qmake must be in your path
cd ..
set currdir=%cd%
call C:\Qt\Qt5.6.2\5.6\msvc2015_64\bin\qtenv2.bat
call "%VS140COMNTOOLS:~0,-14%VC\vcvarsall.bat" x64
cd /D %currdir%
qmake -r -tp vc -spec win32-msvc2015 simulationcraft.pro