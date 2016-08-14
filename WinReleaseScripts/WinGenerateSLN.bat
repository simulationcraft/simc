# Example script that will work on a computer with windows 10 + qt 5.7 + vs2015
# See: https://github.com/simulationcraft/simc/wiki/HowToBuild#alternate-way-for-microsoft-windows
# These are default paths, if installed to non-default paths enter those in instead.
# Change 5.7.0\5.7 to 5.x.y\5.x if you are using qt 5.x.y instead.
# Change win32-msvc2015 to win32-msvc2013 and 14.0 to 12.0 for vs2013
# qmake must be in your path
cd ..
git clean -f -x -d
set currdir=%cd%
call C:\Qt\Qt5.7.0\5.7\msvc2015_64\bin\qtenv2.bat
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
cd %currdir%
qmake -r -tp vc -spec win32-msvc2015 simulationcraft.pro