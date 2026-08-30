---
title: Features
---
What is it?
=======================
Simulationcraft is a free and open-source simulation (see [FormulationVsSimulation](appendixes/FormulationVsSimulation.md)) of WoW combat mechanics: it can simulate a fight with one or many players fighting one or many targets and produce detailed informations. It is usable either from a graphical user interface or from a command-line client (typically through configuration files).

Since Simulationcraft is a simulation, it will never give you the same numbers (the rng matters!): they will always vary but should remain close to the exact values you can observe in game (precision can be controlled and increased at the expense of computations times). You may also face intensive computations times and memory needs in some circumstances. Finally, at its core it was a command-line tool based on configurations files and the GUI was added lately and, while efficient, it is still rudimentary and dry.

However, because Simulationcraft is a simulation, it also has advantages over traditional, formulation-based, tools, such as spreadsheets or Rawr. Developing a formulation is like solving a vast and complex problem while developing is a simulation is like solving many simple problems. And it does matter for you, the end-user, especially in a world where tools are developed by a few benevolent people. Which ones? See below.
 * **Reliability:** "easy" to develop, easy to verify. And we never have to do mathematical approximations or to ignore some interactions: all mechanics and interactions are spontaneously and genuinely reproduced. Bugs may arise of course but they tend to be quickly spotted and easily fixed. The most carefully formulation-based tools can still reach the same accuracy level but, too often, you have no idea about the level of accuracy of your tools and which approximations they did: you're left with a black box and rely on trust. And since the source code of those tools is hard to understand and to verify, no one may be brave enough to investigate them.

 * **Quickly updated:** A simulation is easier to update: most of the times, you will be able to make simulations taking into account the changes made on the PTR long before they reach the live servers. Simulationcraft was able to do a decent job a couple of weeks after Cataclysm was out and was considered mature after 9 weeks. At this time,  many class/spec combinations did not have yet have any robust and renowned formulation-based tool.

 * **Features-rich and highly configurable:** The nature of Simulationcraft allows us to provide rich informations and offer you a broader range of features, to let you change your rotations, to simulate fights the way you want, to synchronize your cooldowns with certain events, etc... Simulationcraft has tons of settings you can play with.

Rich reports
====================
Simulationcraft can generate reports such as those displayed on [simulationcraft.org](http://www.simulationcraft.org/)). Most notable informations are:
 * Decomposition of the dps, such as tools like Recount or World of logs can provide.
 * Dps distribution: to understand how your dps can vary across fights because of the rng.
 * Resource gains: how much a talent or buff is important for your resources?
 * Dps timeline: to measure the importance of certain cooldowns.
 * Spells details: damage per resources, per execution time, minimum and maximum output, etc.
 * Buffs details: uptimes, real benefit, etc.
 * Etc...

![http://www.simulationcraft.org/images/wiki/samplegraphs.png](http://www.simulationcraft.org/images/wiki/samplegraphs.png)

Stats weighting to compare items
====================================
Simulationcraft can be used to compute scale factors: those are weights given to every stat so that one can compare them. The higher a scale factor, the better a stat!

How can you use them? Our reports will provide you with link towards [Wowhead (sample)](http://www.wowhead.com/?items&filter=ub=1;gm=3;gb=1;rf=1;minle=346;wt=20:21:77:117:119:96:103:170:32;wtv=2.6373:1.2115:1.1952:1.5543:1.4658:1.5130:1.0993:1.3631:5.0913;) and [GuildOx (sample)](http://www.guildox.com/wr.asp?usr=&ser=&grp=www&Cla=1024&F=H&Int=3.2213&Spi=2.2387&spd=2.4038&mhit=2.2286&mcr=0.9007&mh=1.7434&Mr=1.1686&Ver=6) to compare your items.

![http://www.simulationcraft.org/images/wiki/wowhead.png](http://www.simulationcraft.org/images/wiki/wowhead.png)

Connected to the rest of the world
====================================
Simulationcraft can import character profiles from [Battle.net](http://us.battle.net/wow/en/), [Wowhead profiles](http://www.wowhead.com/profiles)

It provides you with link towards [Wowhead](http://www.wowhead.com/?items&filter=ub=1;gm=3;gb=1;rf=1;minle=346;wt=20:21:77:117:119:96:103:170:32;wtv=2.6373:1.2115:1.1952:1.5543:1.4658:1.5130:1.0993:1.3631:5.0913;) for items comparison, or a string to paste in [Pawn](http://wow.curse.com/downloads/wow-addons/details/pawn.aspx). Simulationcraft can also export JSON, CSV, and XML files for your custom tools, or combat logs in a custom format.

Finally, class Discords, class websites, and others theorycrafters' lairs are frequently monitored to grab the latest research and discoveries, and their feedback about Simulationcraft.

Have your simulated avatar play the way you want
=======================================================
Do you want to test out another dps rotation? Check whether you should pop your cooldowns as soon as possible or try to synchronize them with bloodlust/heroism or a certain phase of the combat? You can do it, check out the documentation (see [ActionLists](reference/ActionLists.md)). Here is a sample actions list:
```
 actions+=/slice_and_dice,if=buff.slice_and_dice.down&time<4
 actions+=/slice_and_dice,if=buff.slice_and_dice.remains<2&combo_points>=3
 actions+=/killing_spree,if=energy<20&buff.slice_and_dice.remains>5
 actions+=/rupture,if=!ticking&combo_points=5&target.time_to_die>10
 actions+=/eviscerate,if=combo_points=5&buff.slice_and_dice.remains>7&dot.rupture.remains>6
 actions+=/eviscerate,if=combo_points>=4&buff.slice_and_dice.remains>4&energy>40&dot.rupture.remains>5
 actions+=/eviscerate,if=combo_points=5&target.time_to_die<10
 actions+=/revealing_strike,if=combo_points=4&buff.slice_and_dice.remains>8
 actions+=/sinister_strike,if=combo_points<5
 actions+=/adrenaline_rush,if=energy<20
```

Simulate real conditions
=============================
Do most fights look like a tank's n spank? No. Sometimes there are adds spawning, sometimes you suffer heavy damages, sometimes the boss may take twice much more damages, sometimes he become invulnerable, sometimes you have to kick his incantation, sometimes you have to move, sometimes you are distracted by what's happening on your screen, etc... Simulationcraft can simulate that.
```
 # This is currently not functioning as we transition to true multi-target support.
```
Besides, Simulationcraft offers you tools to simulate human behaviours: your players may have a high latency, they may have brain lag and take time to notice a spell miss or a new buff/debuff, they may do mistakes and hit the wrong button.

Internationalization
========================
Simulationcraft is only fully available in English.

However, it can work with all versions of the wow armory and battle.net websites (notably: Russian, Chinese, Korean and Taiwanese characters and guilds can be imported). If you use the command-line client, your files must be encoded as latin1 or utf-8, please take time to read [TextualConfigurationInterface#Characters\_encoding](reference/TextualConfigurationInterface.md#Characters_encoding).


There are German and Chinese translations available for the Graphical User Interface, but not for Input Configuration and Output reports. If you want to contribute in translating the GUI, have a look at [Developers Corner/Localization](development/Localization.md)

Limitations
=====================
At first, Simulationcraft was made for damage dealers, with just a dummy target. It evolved since then but there are still some limitations although we hope to see some of them be removed.

 * **Tanking:** No tanking support currently.
 * **Healing:** No healing support currently.
