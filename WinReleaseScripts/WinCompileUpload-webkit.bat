:: Used to automate everything for Alex so he can be lazy.
cd ..
git clean -f -x -d
:: Clean the directory up, otherwise it'll zip up all sorts of stuff.
For /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%a-%%b)
:: Get the date, because I guess that's important.
git log --no-merges -1 --pretty="%%h">bla.txt
:: Gives the git revision
set /p revision=<bla.txt
:: Hacky hack because windows command prompt is annoying.
del bla.txt

set simcversion=610-07
set install=simc-%simcversion%-source
cd>bla.txt
set /p download=<bla.txt
del bla.txt

robocopy . %install% /s *.* /xd .git %install% /xf *.pgd /xn
:: Don't zip up the PGO file, that's a large file.
set filename=%install%-%mydate%-%revision%
7z a -r -tzip %filename% %install% -mx9
call start winscp /command "open downloads" "put %download%\%filename%.zip -nopreservetime -nopermissions -transfer=binary" "exit"
:: Zips source code, calls winscp to upload it. If anyone else is attempting this 'downloads' is the nickname of whatever server you are trying to upload to in winscp.

::Webkit compilation.
set install=simc-%simcversion%-win64
set qt_dir=C:\Qt\Qt5.4.1\5.4\msvc2013_64
set redist="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\redist\x64\Microsoft.VC120.CRT"

rd %install% /s /q

for /f "skip=2 tokens=2,*" %%A in ('reg.exe query "HKLM\SOFTWARE\Microsoft\MSBuild\ToolsVersions\12.0" /v MSBuildToolsPath') do SET MSBUILDDIR=%%B

"%MSBUILDDIR%msbuild.exe" E:\simulationcraft\simc_vs2013.sln /p:configuration=WebKit-PGO /p:platform=x64 /nr:true

robocopy %redist%\ %install%\ msvcp120.dll msvcr120.dll vccorlib120.dll
robocopy locale\ %install%\locale sc_de.qm sc_zh.qm
robocopy %qt_dir%\bin\ %install%\ Qt5Core.dll Qt5Quick.dll Qt5Qml.dll Qt5Svg.dll Qt5Gui.dll Qt5Widgets.dll Qt5Network.dll Qt5WebKit.dll Qt5WebKitWidgets.dll libGLESv2.dll icudt53.dll icuin53.dll icuuc53.dll libEGL.dll D3DCompiler_47.dll Qt5WebChannel.dll Qt5Multimedia.dll Qt5MultimediaWidgets.dll Qt5Sensors.dll Qt5PrintSupport.dll Qt5Qml.dll Qt5Sql.dll Qt5Svg.dll Qt5Positioning.dll Qt5OpenGl.dll
robocopy winreleasescripts\ %install%\ qt.conf
robocopy %qt_dir%\ %install%\ icudtl.dat
robocopy %qt_dir%\plugins\platforms %install%\platforms\ qwindows.dll
robocopy %qt_dir%\plugins\imageformats %install%\imageformats qdds.dll qgif.dll qicns.dll qico.dll qjp2.dll qjpeg.dll qmng.dll qsvg.dll qtga.dll qtiff.dll qwbmp.dll qwebp.dll
robocopy . %install%\ Welcome.html Welcome.png Simulationcraft64.exe simc64.exe readme.txt Error.html COPYING
robocopy C:\OpenSSL-Win64\bin %install%\ libeay32.dll ssleay32.dll 
robocopy Profiles\ %install%\profiles\ *.* /S

cd winreleasescripts
iscc.exe "setup64.iss"
cd ..
call start winscp /command "open downloads" "put %download%\SimcSetup-%simcversion%-win64.exe -nopreservetime -nopermissions -transfer=binary" "exit"
7z a -r %install% %install% -mx9 -md=32m
call start winscp /command "open downloads" "put %download%\%install%.7z -nopreservetime -nopermissions -transfer=binary" "exit"

set redist="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\redist\x86\Microsoft.VC120.CRT"
set install=simc-%simcversion%-win32
set qt_dir=C:\Qt\Qt5.4.1\5.4\msvc2013
"%MSBUILDDIR%msbuild.exe" E:\simulationcraft\simc_vs2013.sln /p:configuration=WebKit /p:platform=win32 /nr:true /m:8

robocopy %redist%\ %install%\ msvcp120.dll msvcr120.dll vccorlib120.dll
robocopy locale\ %install%\locale sc_de.qm sc_zh.qm
robocopy %qt_dir%\bin\ %install%\ Qt5Core.dll Qt5Quick.dll Qt5Qml.dll Qt5Svg.dll Qt5Gui.dll Qt5Widgets.dll Qt5Network.dll Qt5WebKit.dll Qt5WebKitWidgets.dll libGLESv2.dll icudt53.dll icuin53.dll icuuc53.dll libEGL.dll D3DCompiler_47.dll Qt5WebChannel.dll Qt5Multimedia.dll Qt5MultimediaWidgets.dll Qt5Sensors.dll Qt5PrintSupport.dll Qt5Qml.dll Qt5Sql.dll Qt5Svg.dll Qt5Positioning.dll Qt5OpenGl.dll
robocopy winreleasescripts\ %install%\ qt.conf
robocopy %qt_dir%\ %install%\ icudtl.dat
robocopy %qt_dir%\plugins\platforms %install%\platforms\ qwindows.dll
robocopy %qt_dir%\plugins\imageformats %install%\imageformats qdds.dll qgif.dll qicns.dll qico.dll qjp2.dll qjpeg.dll qmng.dll qsvg.dll qtga.dll qtiff.dll qwbmp.dll qwebp.dll
robocopy . %install%\ Welcome.html Welcome.png Simulationcraft.exe simc.exe readme.txt Error.html COPYING
robocopy C:\OpenSSL-Win32\bin %install%\ libeay32.dll ssleay32.dll 
robocopy Profiles\ %install%\profiles\ *.* /S

cd winreleasescripts
iscc.exe "setup32.iss"
cd ..
call start winscp /command "open downloads" "put %download%\SimcSetup-%simcversion%-win32.exe -nopreservetime -nopermissions -transfer=binary" "exit"
7z a -r %install% %install% -mx9 -md=32m
winscp /command "open downloads" "put %download%\%install%.7z -nopreservetime -nopermissions -transfer=binary" "exit"
pause