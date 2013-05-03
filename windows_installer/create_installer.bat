:: Create a Windows Installer File ( .msi ) using WiX Toolset

:: Wix Toolset: http://wixtoolset.org/
:: Documentation: http://wix.tramontana.co.hu/

IF (%1) == () exit /b

set version=%1

:: create automated file list for profiles
call heat dir ./profiles PROFILES -generate container -template fragment -projectname SimulationCraft -gg -cg Profiles -var var.profiles -dr INSTALLDIR -out profiles.wxs

:: compile Installer configuration & profiles
call candle -dprofiles=profiles -dprod_ver=%version% SimulationCraft.wxs profiles.wxs

:: link everything together into SimulationCraft.msi
call light -ext WixUIExtension -out SimulationCraft-%version%.msi SimulationCraft.wixobj profiles.wixobj