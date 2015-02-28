:: Necessary Qt dlls are packaged with every release.
:: These dlls are not included in the GIT.
:: They need to be copied into the dev area from the Qt install.
:: Qt-Framework is simply the Qt runtime dlls built against the MSVC 2013 compiler
:: It can be found at: http://qt-project.org/downloads
:: As of this writing, the default locations from which to gather the dlls are:
:: Qt-Framework: C:\Qt\Qt5.4.1\
cd ..
:: Update the qt_dir as necessary
set qt_dir=C:\Qt\Qt5.4.1\5.4\msvc2013
set redist="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\redist\x86\Microsoft.VC120.CRT"

:: IMPORTANT NOTE FOR DEBUGGING
:: This script will ONLY copy the optimized Qt dlls
:: The simcqt_vs2013.vcproj file is setup to use optimized dlls for both Debug/Release builds
:: This script needs to be smarter if you wish to use the interactive debugger in the Qt SDK
:: The debug Qt dlls are named: Qt___d5.dll

rd %install% /s /q

for /f "skip=2 tokens=2,*" %%A in ('reg.exe query "HKLM\SOFTWARE\Microsoft\MSBuild\ToolsVersions\12.0" /v MSBuildToolsPath') do SET MSBUILDDIR=%%B

"%MSBUILDDIR%msbuild.exe" E:\simulationcraft\simc_vs2013.sln /p:configuration=WebEngine /p:platform=win32 /nr:true /m:8

forfiles -s -m generate_????.simc -c "cmd /c echo Running @path && %~dp0simc.exe @file"

robocopy %redist%\ %install%\ msvcp120.dll msvcr120.dll vccorlib120.dll
robocopy locale\ %install%\locale sc_de.qm sc_zh.qm
robocopy %qt_dir%\bin\ %install%\ Qt5Core.dll Qt5Quick.dll Qt5Qml.dll Qt5Svg.dll Qt5Gui.dll Qt5Widgets.dll Qt5Network.dll Qt5WebEngineCore.dll Qt5WebEngine.dll Qt5WebEngineWidgets.dll libGLESv2.dll icudt53.dll icuin53.dll icuuc53.dll libEGL.dll D3DCompiler_47.dll QtWebEngineProcess.exe Qt5OpenGl.dll
robocopy winreleasescripts\ %install%\ qt.conf
robocopy %qt_dir%\ %install%\ icudtl.dat qtwebengine_resources.pak
robocopy %qt_dir%\plugins\platforms %install%\platforms\ qwindows.dll
robocopy %qt_dir%\plugins\imageformats %install%\imageformats qdds.dll qgif.dll qicns.dll qico.dll qjp2.dll qjpeg.dll qmng.dll qsvg.dll qtga.dll qtiff.dll qwbmp.dll qwebp.dll
robocopy . %install%\ Welcome.html Welcome.png Simulationcraft.exe simc.exe readme.txt Error.html COPYING
robocopy C:\OpenSSL-Win32\bin %install%\ libeay32.dll ssleay32.dll 
robocopy Profiles\ %install%\profiles\ *.* /S
cd winreleasescripts