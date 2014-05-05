
:: Necessary Qt dlls are packaged with every release.
:: These dlls are not included in the SVN.
:: They need to be copied into the dev area from the Qt install.
:: Qt-Framework is simply the Qt runtime dlls built against the MSVC 2008 compiler
:: It can be found at: http://qt.nokia.com/downloads
:: If you build SimC with MSVC 2008, then you need to use dlls from Qt-Framework
:: As of this writing, the default locations from which to gather the dlls are:
:: Qt-Framework: C:\Qt\Qt5.3.0\

:: Update the qt_dir as necessary
set qt_dir=C:\Qt\5.3.0\5.3\msvc2013_64
set install=simc-win64-release

:: IMPORTANT NOTE FOR DEBUGGING
:: This script will ONLY copy the optimized Qt dlls
:: The MSVC 2008 simcqt.vcproj file is setup to use optimized dlls for both Debug/Release builds
:: This script needs to be smarter if you wish to use the interactive debugger in the Qt SDK
:: The debug Qt dlls are named: Qt___d4.dll

:: Copying new dlls

xcopy /I %qt_dir%\plugins\imageformats simc-release\imageformats
xcopy %qt_dir%\bin\phonon5.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5Core.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5OpenGL.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5PrintSupport.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5Quick.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5Qml.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5V8.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5Sql.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5Gui.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5Widgets.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5Network.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5WebKit.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5WebKitWidgets.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5Multimedia.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5MultimediaWidgets.dll simc-release\qtdll\
xcopy %qt_dir%\bin\Qt5Sensors.dll simc-release\qtdll\

xcopy %qt_dir%\bin\libGLESv2.dll simc-release\qtdll\
xcopy %qt_dir%\bin\icudt52.dll simc-release\qtdll\
xcopy %qt_dir%\bin\icuin52.dll simc-release\qtdll\
xcopy %qt_dir%\bin\icuuc52.dll simc-release\qtdll\
xcopy %qt_dir%\bin\libEGL.dll simc-release\qtdll\
xcopy %qt_dir%\bin\D3DCompiler_47.dll simc-release\qtdll\

xcopy %qt_dir%\plugins\platforms\qminimal.dll simc-release\platforms\
xcopy %qt_dir%\plugins\platforms\qwindows.dll simc-release\platforms\


:: Copy other relevant files for windows release
xcopy Welcome.html simc-release\
xcopy Welcome.png simc-release\
xcopy Simulationcraft.exe simc-release\
xcopy simc64.exe simc-release\
xcopy READ_ME_FIRST.txt simc-release\
xcopy Examples.simc simc-release\
xcopy Error.html simc-release\
xcopy COPYING simc-release\
xcopy Profiles simc-release\profiles\ /s /e