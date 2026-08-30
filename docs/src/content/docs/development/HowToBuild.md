---
title: How to Build
---
##### Table of Contents
- [Downloading the Source](#downloading-the-source)
- [Building SimulationCraft](#building-simulationcraft)
- [Building SimulationCraft on Windows](#building-simulationcraft-on-windows)
- [Building SimulationCraft on Linux](#building-simulationcraft-on-linux)
- [Building SimulationCraft on OSX](#building-simulationcraft-on-osx)
- [Tips and Tricks](#tips-and-tricks)


# Downloading the Source

  * Downloading via Git
    * The repository address can be found on the [SimulationCraft Github page](https://github.com/simulationcraft/simc). Click the green **Clone or Download** button on the right.
      * OSX and most Linux installs should already include **git**, if not, try to install it through your package manager.
      * You can download Git for all platforms directly from [here](https://git-scm.com/downloads).

# Building SimulationCraft

  * **Starting from Simulationcraft 8.1.0 release 2, libcurl (https://curl.haxx.se) is required for armory imports on non-windows platforms**
  * Building the command line interface (CLI) is very easy on all platforms
  * Building the graphical user interface (GUI) is considerably harder
    * The GUI is built using [Qt](https://www.qt.io/)
    * Building the GUI requires that the Qt libraries be downloaded and installed, **including the Webengine component**
    * Qt DLLs are used at runtime, so they need to be in your PATH; to create a release package, a subset must be shipped with it.
    * Refer to platform-specific directions below

# Building SimulationCraft on Windows using Microsoft Visual Studio

* Download and install  [Microsoft Visual Studio Community 2022](https://visualstudio.microsoft.com). Under the `Workloads` tab select `Desktop development with C++` and additionally select `C++ Clang tools for Windows` if you'd like that as well.

* Download and install [the latest Qt binaries for Windows](https://www.qt.io/download) (6.3.1 or newer). 
    * Look for open source downloads and then either use the online installer or look for offline installers. You can [skip account creation for the offline installer](https://superuser.com/a/1524989).
    * Select the latest Qt version during "Select Components", i.e. `Qt 6.3.1` for `MSVC 2019 64-bit` **as well as the Additional Libraries** `Qt Positioning`, `Qt WebChannel` and `Qt WebEngine`
    * Add C:\Qt\6.3.1\msvc2019_64\bin to your [PATH](#adding-directory-to-path) (or where-ever the Qt is installed).

## Using CMake Visual Studio integration
  * Start Visual Studio and select 'Open a local folder', where you choose the root simc directory.
    This will automatically pick up the CMake configuration and build both the gui and cli.
  * To only build the cli (eg. if you do not want to install Qt) go to `Project -> Cmake Settings for simc` and under CMake variables deselect `BUILD_GUI`

## Alternative using Qmake
  * Open a command prompt and run `"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"` to get MS C++ compiler and MSBuild added to your path. ([Source](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170#developer_command_file_locations))
  * In the command prompt, navigate to `your_simc_source_dir`.
  * In `your_simc_source_dir`, issue the command `qmake -r -tp vc -spec win32-msvc simulationcraft.pro`
    * Note that for older Qt versions the spec parameter may require your visual studio version (e.g., `win32-msvc2017`)
    * Output should look something like `Reading <your_simc_source_dir>/lib/lib.pro` (similarly for `gui` and `cli`).
    * If you have upgraded your Qt version, you should delete `.qmake.stash` file before issuing the qmake command
  * Open the generated `simulationcraft.sln` in `your_simc_source_dir` with Visual Studio.
    * Three solutions are available, `Simulationcraft Engine`, which is the core library, `Simulationcraft CLI`, which is the command line client (i.e., simc.exe), and `Simulationcraft GUI`, which is the graphical user interface (i.e., Simulationcraft.exe).

## Alternate using QtCreator
  * Once your Qt version is installed, open `your_simc_source_dir`/simulationcraft.pro file with QtCreator.
  * Build Simulationcraft CLI/GUI

## Advanced Settings
  * If you want to deploy SimulationCraft.exe without having QT installed and added to PATH, execute windeployqt.exe from your QT installation on SimulationCraft.exe. This will copy over the necessary DLL's which you need to send along with the executable.


# Building SimulationCraft on Linux

  * Linux compilation using various g++ versions (at least 5, 6, and 8 series) may fail at the linking stage if Link Time Optimization (LTO) is used in conjunction with the new curl-based networking interface. Error message output will have something akin to `lto1: internal compiler error: in odr_types_equivalent_p, at ipa-devirt.c`. In this case, either switch `llvm-clang` compiler, use `SC_NO_NETWORKING` if you don't need the HTTP interfaces in Simulationcraft (to use `armory` or `guild` options), or stop using LTO in compilation.

## Command Line Interface & Graphical User Interface using Cmake (preferred)
 * If not already installed, install `cmake`, `build-essential`, `libcurl-dev`, and `pkg-config` (e.g., a compilation toolchain, libcurl include headers, and library metainformation tool).
  * Install qt5-qmake & qt5-webengine if you want to build the GUI. On Ubuntu, the required packages are called `qt5-default` and `qtwebengine5-dev`.
  * `cd your_simc_source_dir`
  * `cmake -B build .`
  * `cmake --build build`
  * This builds target `simc` (CLI) and `qt/SimulationCraft` (GUI)
  * Additional cmake options:
    * `cmake ../ -DCMAKE_BUILD_TYPE=Release` for Release build
    * `cmake ../ -DCMAKE_BUILD_TYPE=Debug` for Debug build
    * `cmake ../ -DBUILD_GUI=OFF` for CLI only, no need to have Qt installed
    * `cmake ../ -DSC_NO_NETWORKING=ON` to build without network support for Blizzard Community Platform API (armory), no need to have libcurl installed

## Command Line Interface (classic build)

  * If not already installed, install `build-essential`, `libcurl-dev` (e.g., a compilation toolchain and libcurl include headers).
  * `cd your_simc_source_dir/engine`
  * `make optimized`
  * This builds an optimized executable named `simc`
  * Additional options:
    * Build with clang: Add CXX=clang++, eg. `make optimized CXX=clang++`
  * Note that if you issue the build with `SC_NO_NETWORKING=1`, `libcurl-dev` package is not necessary

## Graphical User Interface (using qmake)

  * Install qt5-qmake & qt5-webengine. On Ubuntu, the required packages are called `qt5-default` and `qtwebengine5-dev`.
  * `cd your_simc_source_dir`
  * `qmake simulationcraft.pro`
  * `make`
  * Optional: Install to `~/SimulationCraft`:
    * `make install`

## Building Debian/Ubuntu packages

If you wish to have SimulationCraft installed via debian packages rather than make install, and packages for your distribution are not (yet) available, you can build your own.
Mind that debian package data is not included in the core sources, and is only present on git.
The debian data structure is maintained following the guidelines from git-buildpackage: http://honk.sigxcpu.org/projects/git-buildpackage/manual-html/gbp.html

### Accessing the debian/ data

According to **gbp** documentation (see above), every supported debian/ubuntu distribution has its own debian/ data directory on a separate branch on git. So, if you were to access debian/data for ubuntu/precise:

  * `git clone https://code.google.com/p/simulationcraft/`
  * `cd simulationcraft`
  * `git checkout ubuntu/precise`

### Requirements

A basic understanding of debian/ contents and package building is not strictly required for package building per se, but comes very helpful.

To build debian source packages you will need the following tools at the very least:

  * `sudo aptitude install devscripts dpkg-dev git git-buildpackage debhelper autotools-dev`

If your plan is to upload the packages to the PPA this is sufficient. In order to build binary packages, you will need a derivate of either **debuild** or **pbuilder**.
While debuild is simpler to use and requires almost zero knowledge (you just run **debuild -us -uc** to build a package), i would recommend **pbuilder** or better yet **pbuilder-dist**, as they have the great benefit of not tainting your running distribution with building dependencies.
For more information see: https://wiki.ubuntu.com/PbuilderHowto

### Building the source package

If your distribution is supported on the source tree (you can check by typing **git branch**), then building the source package is quite straightforward. An example for ubuntu/precise, release 5.4.8-4 (check releases with **git tag**):

  * `git checkout ubuntu/precise`
  * `./debian/_gbp_build release-548-4`

This will leave you with debian source data in ~/tmp/gbp/precise.

### Building the binary package

If you're not using the PPA builder and wish to manually build the package, you can either do it using `debuild`, from the directory where your debian source data is in:

  * `dpkg-source -x simulationcraft-x.y.z_foo.dsc`
  * `cd simulationcraft-x.y.z`
  * `debuild -us -uc`

or use pbuilder, after having configured it (see Requirements section), e.g.:

  * `pbuilder-dist trusty amd64 build simulationcraft-x.y.z_foo.dsc`

### Releasing a new simulationcraft version on a supported release

If a new simc version needs debian support, you can build a new source release by following a few simple steps. Example for ubuntu/precise, updating to release-548-4:

  * `git checkout ubuntu/precise`
  * `git merge release-548-4`
  * `dch -i` - Make sure you follow the version numbering trend, and explicitly type the release name on the right of the version (in this case, `precise`).
  * `./debian/_gbp_build release-548-4`
  * git commit and push your changes

Please be mindful, if you're considering to upload the source packages to a PPA, that Launchpad only accepts one source tarball for every given source, so if you are publishing more than a release (which is likely the case), subsequent builds should avoid adding the source tarball. This is done by changing the last command to this:

  * `./debian/_gbp_build release-548-4 -sd`

### Adding support for more Debian/Ubuntu releases

Only a few releases are supported currently, so if you wish to add support for other releases, follow these steps:

  * Identify the closest supported release
  * Duplicate the branch and name it according to the desired release, e.g.:
    * `git checkout ubuntu/trusty`
    * `git checkout -b ubuntu/utopic`
  * Adjust debian metadata to reflect the change, including but not necessarily limited to:
    * **debian/changelog** - Add a new entry or edit the last one with the right release name
    * **debian/control** - If dependency package names differ on your release, or even if you're just building against a different library version, make sure you edit the proper data (for example, check the diff between ubuntu/precise and ubuntu/trusty, which are built against a different QT major release)
    * **`debian/_gbp_build`** - Update it to reflect your change in the current branch name
    * **debian/gbp.conf** - Same as above
  * git commit and push your changes

# Building SimulationCraft on OSX

Building SimulationCraft on OS X requires you to install the XCode development environment, and in most cases, the command line tools.

## Command Line Interface / Makefile builds

  * `cd your_simc_source_dir/engine`
  * `make optimized`
  * This builds an optimized executable named `simc`

## Graphical User Interface / Qmake builds
  * Install Qt 5
    * If you use Qt 5.1, you should fix the install names for Qt frameworks by pointing the `qt/fix_qt51_osx_paths.sh` to your qt install directory (the `clang_64` directory). This is only relevant if you are building a release, though, or intend to use the `macdeployqt` binary to create a framework independent bundle of `SimulationCraft.app`.
  * In terminal, issue "qmake simulationcraft.pro". This will create a `Makefile` in your simc source directory.
  * If you receive an error from Qmake such as `Project ERROR: Could not resolve SDK path for 'macosx10.9'`, you will need to explicitly tell make what OS X SDK to use, for example `qmake QMAKE_MAC_SDK=macosx<version> simulationcraft.pro`, where <version> is your OS X version (e.g., 10.10, 10.11). Note that the "OS X" version here is at least partly determined by your Xcode version, and may not match 1:1 with your operating system version.
  * Then, issuing "make" in the terminal will build SimulationCraft.app to your source directory.

## Graphical User Interface / XCode builds

XCode builds no longer supported

## Release package

The easiest way to build a release package for OS X is to use the qmake system. Open a terminal and issue "qmake simcqt.pro" in the simc source directory, followed by "make create\_release" will build an optimized GUI and command line clients, and package them in a disk image file. You can also build the release package through the XCode project, by building the `Create Release` target. Note that in this case, you will need to specify the `release` configuration to get optimized versions of the command line and the GUI client.

# Tips and Tricks
## Adding directory to PATH
  * On Windows7 go to Control Panel -> System -> Advanced system settings -> Environment Variables
    * Under System Variables search for PATH. Click Edit and append it with a semicolon `;` , followed by your desired directory path.
    * For other versions of windows, see [http://www.computerhope.com/issues/ch000549.htm](http://www.computerhope.com/issues/ch000549.htm).
## GNU Make Options
    * Add -j n  where n is the number of threads used for compiling
    * The GCC flag -dM will stop the compiler after the preprocessing pass and make it dump all #define

