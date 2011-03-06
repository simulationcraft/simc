
setlocal

set NAME=%1
set SCDIR=d:\data\code\simcraft\simulationcraft
set QTDIR=c:\qt
set QTLIBDIR=%QTDIR%\lib
set QTDLLDIR=%QTDIR%\bin
set OUTDIR=%SCDIR%\..\releases\simc-%NAME%
set VDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin

rd /Q /S %OUTDIR%

svn copy https://simulationcraft.googlecode.com/svn/trunk https://simulationcraft.googlecode.com/svn/tags/release-%NAME% -m "Tagging as the %NAME% release."

svn export %SCDIR% %OUTDIR%

rd /Q /S %OUTDIR%\dbc_extract
rd /Q /S %OUTDIR%\html

cd %OUTDIR%
cd ..

"C:\Program Files (x86)\7-Zip\7z.exe" a -y -r simc-%NAME%-src.zip simc-%NAME%

xcopy %QTDLLDIR%\phonon4*.dll %OUTDIR%
xcopy %QTDLLDIR%\QtCore4*.dll %OUTDIR%
xcopy %QTDLLDIR%\QtGui4*.dll %OUTDIR%
xcopy %QTDLLDIR%\QtNetwork4*.dll %OUTDIR%
xcopy %QTDLLDIR%\QtWebKit4*.dll %OUTDIR%
xcopy %QTDLLDIR%\QtXmlPatterns4*.dll %OUTDIR%
xcopy %SCDIR%\..\extrafiles\*.* %OUTDIR%
xcopy /e /I %QTDIR%\plugins\imageformats %OUTDIR%\imageformats




rem vcvars32.bat

call "%VDIR%\vcvars32.bat"

DevEnv %OUTDIR%\simc.sln /Build "Release|Win32"
DevEnv %OUTDIR%\simcqt.sln /Build "Release|Win32"

copy %OUTDIR%\simc.exe %OUTDIR%\..\simc.exe
copy %OUTDIR%\SimulationCraft.exe %OUTDIR%\..\SimulationCraft.exe

DevEnv %OUTDIR%\simc.sln /Clean "Release|Win32"
DevEnv %OUTDIR%\simcqt.sln /Clean "Release|Win32"

rd /Q /S %OUTDIR%\Release

copy %OUTDIR%\..\simc.exe %OUTDIR%\simc.exe 
copy %OUTDIR%\..\SimulationCraft.exe %OUTDIR%\SimulationCraft.exe

cd %OUTDIR%
cd ..

"C:\Program Files (x86)\7-Zip\7z.exe" a -y -r simc-%NAME%-win32.zip simc-%NAME%

:end

endlocal