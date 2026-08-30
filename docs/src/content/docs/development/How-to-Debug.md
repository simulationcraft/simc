---
title: How to Debug
---
# Debug instructions for VS2019 using the CLI
This assumes you have already installed and setup Visual Studio based on the instructions [here](https://github.com/simulationcraft/simc/wiki/HowToBuild).

If you are able to build SimC as normal, you can continue forward to debug something. First thing you want to do is change the build type from the dropdown from `Release` to `Debug` and then rebuild. After this is rebuilt you can start debugging, however you probably want to change some arguments passed to the simc executable for testing purposes. 

Navigate to `simc_vs2019 debug properties` under the Debug menu in Visual Studio Code. Here you can adjust several settings including `Command Arguments` under `Configuration Properties -> Debugging`. You can then set where your file is you want to test, i.e. `test.simc`

Now you can Start Debugging (F5 keybind by default) to figure out why things are crashing/not working correctly. Use the [Visual Studio Docs](https://docs.microsoft.com/en-us/visualstudio/debugger/navigating-through-code-with-the-debugger?view=vs-2019) to help navigate other features of the debugger, you are ready to debug!
