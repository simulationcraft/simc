
:: Necessary Qt dlls are packaged with every release.
:: These dlls are not included in the SVN.
:: They need to be copied into the dev area from the Qt install.
:: Qt-Framework is simply the Qt runtime dlls built against the MSVC 2008 compiler
:: It can be found at: http://qt.nokia.com/downloads
:: If you build SimC with MSVC 2008, then you need to use dlls from Qt-Framework
:: As of this writing, the default locations from which to gather the dlls are:
:: Qt-Framework: C:\Qt\4.7.4

:: Update the qt_dir as necessary
set qt_dir=C:\Qt\Qt5.0.2\5.0.2\mingw47_32

:: IMPORTANT NOTE FOR DEBUGGING
:: This script will ONLY copy the optimized Qt dlls
:: The MSVC 2008 simcqt.vcproj file is setup to use optimized dlls for both Debug/Release builds
:: This script needs to be smarter if you wish to use the interactive debugger in the Qt SDK
:: The debug Qt dlls are named: Qt___d4.dll

:: Removing existing dlls
del /q imageformats
del /q phonon5.dll
del /q Qt5Core.dll
del /q Qt5OpenGL.dll
del /q Qt5PrintSupport.dll
del /q Qt5Quick.dll
del /q Qt5Qml.dll
del /q Qt5V8.dll
del /q Qt5Sql.dll
del /q Qt5Gui.dll
del /q Qt5Widgets.dll
del /q Qt5Network.dll
del /q Qt5WebKit.dll
del /q Qt5WebKitWidgets.dll
del /q Qt5Multimedia.dll
del /q Qt5MultimediaWidgets.dll
del /q D3DCompiler_43.dll
del /q libGLESv2.dll
del /q icudt49.dll
del /q icuin49.dll
del /q icuuc49.dll
del /q libEGL.dll
del /q mingw*.dll
del /q libgcc*.dll
del /q libstd*.dll
del /q libwinpthread-1.dll
del /q platforms

:: Copying new dlls

xcopy /I %qt_dir%\plugins\imageformats imageformats
xcopy %qt_dir%\bin\phonon5.dll
xcopy %qt_dir%\bin\Qt5Core.dll
xcopy %qt_dir%\bin\Qt5OpenGL.dll
xcopy %qt_dir%\bin\Qt5PrintSupport.dll
xcopy %qt_dir%\bin\Qt5Quick.dll
xcopy %qt_dir%\bin\Qt5Qml.dll
xcopy %qt_dir%\bin\Qt5V8.dll
xcopy %qt_dir%\bin\Qt5Sql.dll
xcopy %qt_dir%\bin\Qt5Gui.dll
xcopy %qt_dir%\bin\Qt5Widgets.dll
xcopy %qt_dir%\bin\Qt5Network.dll
xcopy %qt_dir%\bin\Qt5WebKit.dll
xcopy %qt_dir%\bin\Qt5WebKitWidgets.dll
xcopy %qt_dir%\bin\Qt5Multimedia.dll
xcopy %qt_dir%\bin\Qt5MultimediaWidgets.dll

xcopy %qt_dir%\bin\libGLESv2.dll
xcopy %qt_dir%\bin\icudt49.dll
xcopy %qt_dir%\bin\icuin49.dll
xcopy %qt_dir%\bin\icuuc49.dll
xcopy %qt_dir%\bin\libEGL.dll
xcopy %qt_dir%\bin\D3DCompiler_43.dll

xcopy %qt_dir%\bin\libstdc++-6.dll
xcopy %qt_dir%\bin\libgcc_s_sjlj-1.dll
xcopy %qt_dir%\bin\libwinpthread-1.dll

xcopy %qt_dir%\plugins\platforms\qminimal.dll platforms\
xcopy %qt_dir%\plugins\platforms\qwindows.dll platforms\