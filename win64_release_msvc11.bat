
:: Necessary Qt dlls are packaged with every release.
:: These dlls are not included in the SVN.
:: They need to be copied into the dev area from the Qt install.
:: Qt-Framework is simply the Qt runtime dlls built against the MSVC 2008 compiler
:: It can be found at: http://qt.nokia.com/downloads
:: If you build SimC with MSVC 2008, then you need to use dlls from Qt-Framework
:: As of this writing, the default locations from which to gather the dlls are:
:: Qt-Framework: C:\Qt\Qt5.2.1\

:: Update the qt_dir as necessary
set qt_dir=C:\Qt\5.2.1\5.2.1\msvc2012_64
set install=simc-win64-release

:: IMPORTANT NOTE FOR DEBUGGING
:: This script will ONLY copy the optimized Qt dlls
:: The MSVC 2008 simcqt.vcproj file is setup to use optimized dlls for both Debug/Release builds
:: This script needs to be smarter if you wish to use the interactive debugger in the Qt SDK
:: The debug Qt dlls are named: Qt___d4.dll

:: Copying new dlls

xcopy /I %qt_dir%\plugins\imageformats %install%\imageformats
xcopy %qt_dir%\bin\phonon5.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5Core.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5OpenGL.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5PrintSupport.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5Quick.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5Qml.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5V8.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5Sql.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5Gui.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5Widgets.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5Network.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5WebKit.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5WebKitWidgets.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5Multimedia.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5MultimediaWidgets.dll %install%\qtdll\
xcopy %qt_dir%\bin\Qt5Sensors.dll %install%\qtdll\

xcopy %qt_dir%\bin\libGLESv2.dll %install%\qtdll\
xcopy %qt_dir%\bin\icudt51.dll %install%\qtdll\
xcopy %qt_dir%\bin\icuin51.dll %install%\qtdll\
xcopy %qt_dir%\bin\icuuc51.dll %install%\qtdll\
xcopy %qt_dir%\bin\libEGL.dll %install%\qtdll\
xcopy %qt_dir%\bin\D3DCompiler_46.dll %install%\qtdll\

xcopy %qt_dir%\plugins\platforms\qminimal.dll %install%\platforms\
xcopy %qt_dir%\plugins\platforms\qwindows.dll %install%\platforms\


:: Copy other relevant files for windows release
xcopy Welcome.html %install%\
xcopy Welcome.png %install%\
xcopy Simulationcraft.exe %install%\
xcopy simc64.exe %install%\
xcopy READ_ME_FIRST.txt %install%\
xcopy Examples.simc %install%\
xcopy Error.html %install%\
xcopy COPYING %install%\
xcopy Profiles %install%\profiles\ /s /e