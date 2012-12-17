
:: Necessary Qt dlls are packaged with every release.
:: These dlls are not included in the SVN.
:: They need to be copied into the dev area from the Qt install.
:: It is important to understand that the Qt-SDK is different from the Qt-Framework
:: Qt-SDK is an integrated development environment that is shipped with the MinGW 4.4 compiler
:: Qt-Framework is simply the Qt runtime dlls built against your own MinGW
:: Both can be found at: http://qt.nokia.com/downloads
:: If you build SimC with QT-Framework, then you need to use dlls from Qt-Framework
:: If you build SimC with the Qt SDK, then you need to use dlls from Qt-SDK
:: As of this writing, the default locations from which to gather the dlls are:
:: Qt-SDK: c:\QtSDK\Desktop\Qt\4.8.1\mingw
:: Qt-Framework: C:\Qt\4.8.4

:: Update the qt_dir as necessary
set qt_dir=C:\Qt\4.8.4
set mingw_dir=C:\MinGW

:: IMPORTANT NOTE FOR DEBUGGING
:: This script will ONLY copy the optimized Qt dlls
:: The Qt SDK simcqt.pro file will only use optimize dlls for Release
:: This script needs to be smarter if you wish to use the interactive debugger in the Qt SDK
:: The debug Qt dlls are named: Qt___d4.dll

:: Removing existing dlls
del /q imageformats
del /q phonond4.dll
del /q QtCored4.dll
del /q QtGuid4.dll
del /q QtNetworkd4.dll
del /q QtWebKitd4.dll
del /q QtXmlPatternsd4.dll
del /q mingw*.dll
del /q libgcc*.dll
del /q libstdc++*.dll

:: Copying new dlls
xcopy /I %qt_dir%\plugins\imageformats imageformats
xcopy %qt_dir%\bin\phonond4.dll
xcopy %qt_dir%\bin\QtCored4.dll
xcopy %qt_dir%\bin\QtGuid4.dll
xcopy %qt_dir%\bin\QtNetworkd4.dll
xcopy %qt_dir%\bin\QtWebKitd4.dll

:: Copy MingW runtime DLLs
xcopy %mingw_dir%\bin\mingw*.dll .
xcopy %mingw_dir%\bin\libgcc*.dll .
xcopy %mingw_dir%\bin\libstdc++*.dll .
