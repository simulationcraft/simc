---
title: FAQ
---
# General
## What is the license?
Simulationcraft is licensed under the [GPL v3](http://www.gnu.org/licenses/gpl.html). It is free and open-source. Anyone can contribute (help with the development, check the code, report issues, provide feedback, etc), including you.

## Where are the changelogs / version notes?
See the [list of commits](https://github.com/simulationcraft/simc/commits).

## Where are the sample outputs?
On [simulationcraft.org](http://www.simulationcraft.org/).

## How did you setup those Best-in-Slot profiles and default actions lists?
Most profiles and action lists are submitted by users like you! Please submit an issue if you believe you have improvements for them.

## I cannot import my Russian/Korean/Chinese/... character
If you use configuration files, they must be encoded as latin1 or utf-8, please take time to read [TextualConfigurationInterface#Characters\_encoding](reference/TextualConfigurationInterface.md#Characters_encoding).

## Have you thought about leveraging the GPU for SimC?
The GPU is good at iterating small pieces of code millions of times, in a parallel way. A typical example is matrix operations. For Simcraft, however, roughly half of its code is involved in the iterations (basically: anything found in sc\_CLASSNAME.cpp, along with most of sc\_player.cpp and sc\_sim.cpp). We're not iterating one small algorithm (matrix inversion) but one big algorithm with many sub-components (a couple of algorithms per spell, one for buff application/removal, one for resources management, one for attack table resolution, etc.). Besides, while fights can be ran on parallel, every fight simulation is sequential and therefore not parallelizable.

# Graphical user interface

## Where are my files saved?
 * On Windows, it is in the %appdata% folder.
 * On OSX, when you use the GUI, it is `$HOME/Library/Application Support/simcqt`

## How can I change the region and language for the armory page?
Under the **options** tab and the **globals** sub-tab, there is an "Armory region" setting, it allows you to change the region. As with every option, it is persisted across runs. The default armory language will still be English but you will be able to find your characters even if they're not on an English-speaking realm.

You can also directly change the url. For example, replace the "en" by "fr" if you want to have the armory in French.

![http://www.simulationcraft.org/images/wiki/url_structure.jpg](http://www.simulationcraft.org/images/wiki/url_structure.jpg)

## VCRUNTIME140_1.DLL not found
Install [Microsoft Visual C++ Redistributable for Visual Studio 2015, 2017, 2019 and 2022](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-160) for the architecture matching your downloaded SimulationCraft.

# Results and reports
## My results change across runs!
Of course since Simulationcraft is a [simulation](SimulationVsFormulation). Your results will slightly vary but will always stay close from the truth.

## There is no way my dps can be so high on a target dummy
By default, you're considered to be in an optimal raid, which includes bloodlust/heroism/time warp. There is also no movement/distractions, but these can be added by advanced users!
