---
title: Stats Scaling
---
_This documentation is a part of the [TCI](TextualConfigurationInterface.md) reference._

**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**

# Introduction
Simulationcraft can evaluate scale factors for stats, as shown in our sample reports (see [simulationcraft.org](http://www.simulationcraft.org/)). Scale factors are computed by adding a given number of points (deltas) to a character's stat, and then comparing the resulting dps/heal/deaths count with the reference one : the scale factor is equal to the dps/heal/deaths count difference, divided by the delta.

Note that, because of the fighting variance, in order to have accurate enough results, you need to use large enough deltas or reduce the fighting variance through the statistical settings (see the [statistical behaviour](#Statistical_behaviour) section but be warned that **seed** won't help: the compared runs will always both start with the same seed) or through **smooth\_scale\_factors**. A good way to check your results are stable enough is to run Simulationcraft twice (with **deterministic\_rng** and **seed** being disabled, their default values) and ensure you get similar scale factors.

# Basics
  * **calculate\_scale\_factors** (scope: global; default: 0), when different from zero, forces the application to evaluate scale factors. The application will automatically decide which scale factors are worth to evaluate for every class.
```
 calculate_scale_factors=1
```

  * **scale\_[stat name]** (scope: global; default: 0) allow you to specify the amount of points to add to the corresponding stat (deltas). When left to 0, default deltas shown in the next section will be used.
```
 calculate_scale_factors=1
 scale_haste_rating=200
```
  Increasing this setting does not directly change the resulting scale factor since those factors are proportional to the benefit _per point_. However, since many ratings suffer diminishing returns, you may actually observe a lowered scale factor as you increase this setting.

  * **positive\_scale\_delta** (scope: global; default: 0), when different from zero, will force the default deltas shown in the next version to always be positive.
> If you explicitly set a non-default delta, **positive\_scale\_delta** will not affect those deltas.
```
 calculate_scale_factors=1
 positive_scale_delta=1
```

  * **scale\_over** (scope: global; default: "") is the value over which the scale factors are evaluated. Acceptable values are:
    1. default: the dps of the player alone (or hps for a healer, see **role**).
    1. _tmi_: the theck meloree index, a measure of how smooth damage intake is for tanks.aid.
    1. _deaths_: the number of times the player died throughout all simulations.
    1. _avg\_death\_time_: the average lifespan of the player, in seconds.
    1. _min\_death\_time_: the minimum lifespan of the player, in seconds.
    1. _dmg\_taken_: the damages taken by the player alone.
```
 # Scale factors over the total raid dps rather than the player dps alone.
 scale_over=tmi
```
  Please note that _deaths_, _avg\_death\_time_, _min\_death\_time_ and _dmg\_taken_ can be used to rate a stat over a player's survivability, rather than its dps. They're primarily aimed at tanks.

# Default deltas

The default delta is determined by how much rating it takes to increase haste by 3.5%.

For Shadowlands this will scale stats by **116** by default (SimC will test agility + 116, haste + 116, crit + 116, etc).

Use **positive\_scale\_delta** to use the absolute values instead.

Note: those values will be doubled if you use **smooth\_scale\_factors**

# Advanced

  * **scale\_delta\_multiplier** (scope: global; default: 1.0) is a multiplier affecting all default deltas. For example, when set to 2.0, the application will use a default value of 600 for **scale\_strength** instead of 300. Only default deltas (settings left to 0) are affected, not your custom settings.
```
 calculate_scale_factors=1
 scale_delta_multiplier=2.0
```

  * **scale\_only** (scope: global; default: "") allows you to specify the exact list of stats the simulation may compute scale factors for. This list is exclusive : it won't force the simulation to compute scale factors for the stats you specified but it will never compute scale factors for stats absent from this list.
```
 calculate_scale_factors=1
 scale_only=spell_power,intellect,spirit
```

  * **normalize\_scale\_factors** (scope: global; default: 0), when different from zero, will force the application to normalize scale factors according to your primary stat (strength, agility or intellect, depending on your class): your primary stat will have a scale factor of 1.0 and the other stats will be scaled accordingly.
```
 calculate_scale_factors=1
 normalize_scale_factors=1
```

  * **scale\_factor\_noise** (scope: global; default: -1.0) is used to detect problems with scale factors computations caused by insufficient iterations. When a problem is detected, the scale factors won't be changed but a warning will be reported on the standard output..
    1. If this setting is equal to zero, it is ignored and no warning will be reported.
    1. If this setting is lesser than zero, a warning will be printed out when a scale factor is lesser than or equal to its error margin, computed through: `(error(base_stat) + error(base_stat + delta)) / delta`.
    1. If this setting is greater then zero, any scale factor below this absolute threshold will report a warning.
```
 # Scale factors lesser than 0.5 will be forced to zero.
 calculate_scale_factors=1
 scale_factor_noise=0.5
```

  * **scale\_haste\_iterations** (scope: global; default: 1.0) is a multiplier applied to the number of iterations to use for computing the scale factors for haste.
```
 #This example will use ten times more iterations for computing the scale factors for haste.
 calculate_scale_factors=1
 scale_haste_iterations=10.0
```

  * **smooth\_scale\_factors** (scope: global; default: 0), when different from zero, will force the **deterministic\_rng**, **separated\_rng** and **average\_range** to 1, whatever values you may have explicitly specified. It will also cause the default deltas to be halved.
```
 calculate_scale_factors=1
 smooth_scale_factors=1
```

  * **scale\_lag** (scope: global; default: 0): When different from zero and when **calculate\_scale\_factors** is enabled, the application will compute the scale factor per millisecond for in-game world latency. A delta of 100ms is used, represented by 100ms **gcd\_lag** and 200ms **channel\_lag**. **queue\_lag** is unaffected, since it doesn't depend on your latency.
```
 scale_lag=1
```

# Centering the scale delta
Basically, the scale factors computed by default are the benefit per stat point when your character has a stat value equal to `current_stat_value + delta/2` : indeed, scale factors are actually the derivative of dps relative to a stat value and Simulationcraft, by default, compute this approximation with two points : `dps(current_stat_value)`and `dps(current_stat_value + delta)`. It may be more interesting to get the benefit per point at the stat value you're currently sitting on and two settings allow you to achieve that.

  * **center\_scale\_delta** (scope: global; default: 0), when different from zero, will force the application to use the same linear approximation than the one used by default, with two different points: `dps(current_stat_value - delta/2)`and `dps(current_stat_value + delta/2)`. Since two runs instead of one have to be done (we cannot reuse the baseline run), computation time is doubled with this option.
```
 calculate_scale_factors=1
 center_scale_delta=1
```

# Plotting
Simulationcraft can produce plots showing the dps versus the stats of your choice: all stats will be displayed on the same plot, their deltas on the horizontal axis and the dps gain on the vertical axis.

  * **dps\_plot\_stat** (scope: global; default: "") is a list of stats to plot. You can indifferently use _haste_ or _haste\_rating_, _crit_ or _crit\_rating_, etc.
```
 dps_plot_stat=haste,crit_rating,mastery
```
  Each stat can optionally be followed by `:<value` to override the starting stat amount. Override will only apply during plots of that stat, and plots of any other stat will use the baseline amount of the previously overridden stat.
```
# Start haste plot at 0 rating with rest of stats baseline.
# Then plot crit starting at 1000 with rest of stats (including haste) at baseline.
 dps_plot_stat=haste:0,crit:1000
```

  * **dps\_plot\_points** (scope: global; default: 20) is the number of points to use to create the graph. Simulationcraft will have to do a run for every point, so the computation time will directly increase with this setting. If you specify an even number, it will be automatically incremented so that a point is always displayed for the baseline run (current stat value, delta being zero).

  * **dps\_plot\_step** (scope: global; default: variable) is the delta between two points of the graph. The deltas on the horizontal axis will be within the `[-points * steps / 2 ; +points * steps / 2]` interval. When a single stat is specified in `dps_plot_stat`, the default value will be 0.5% worth of that stat's ratings at maximum level. When multiple stats are specified, or a non-rating stat is specified, the default value will be 0.5% worth of haste rating at maximum level.
```
 #The following example will compute 20 points for crit deltas between -300 and +300.
 dps_plot_stat=crit
 dps_plot_points=20
 dps_plot_step=30
```

  * **dps\_plot\_positive** (scope: global; default: 0), when different from zero, will produce plots between `[0 ; points * steps]` (rather than `[-points * steps / 2 ; +points * steps / 2]`).
```
 #The following example will compute 20 points for haste deltas between 0 and 600.
 dps_plot_positive=1
 dps_plot_stat=haste
 dps_plot_points=20
 dps_plot_step=30
```

  * **dps\_plot\_iterations** (scope: global; default: 10,000) is the number of iterations to use for the computations done for the graph.
```
 dps_plot_stat=haste
 dps_plot_iterations=20000
```

  * **dps_plot_target_error** (scope: global; default: variable) is the target error to run each plot step at. By default it will match the target error of the baseline profile.

* **dps_plot_display_delta** (scope: global; default: 0) when set will display the difference between current step's DPS vs the last step's DPS.

# Reforge plots
Simulationcraft can produce csv data you can use with Office or GNUPlot to study the relations between two or more stats. It also produces 2d charts for computations with two stats.

For example, on the plot below, we can observe, for an affliction warlock, that it is usually better to reforge crit into haste but also that, at some points, reforging crit into mastery is more interesting. Note this chart has NOT been generated by Simulationcraft, it will only produce 2d charts. However, it produced the raw data used to generate the plot.

![http://www.simulationcraft.org/images/wiki/reforge3d.png](http://www.simulationcraft.org/images/wiki/reforge3d.png)

  * **reforge\_plot\_stat** (scope: global; default: "") allows you to enumerate the stats you want to reforge between. The stats must be separated with comma ",". There must be at least 2 stats. You can add more but, beyond 3 stats, the generated data are harder to analyze. For a list of available stats, see [Equipment#Stats\_abbreviations](Equipment.md#Appendix:_Stats_abbreviations).
You can run multiple reforge plots in a single simulation, by separating them with a slash "/".
```
 # This example will produce a 2D plot that shows how the dps evolve between (+200 crit ; -200 haste) on
 # the left side and (-200 crit ; +200 haste) on the right side.
 reforge_plot_stat=crit,haste

 # This example will output CSV data for a 3D plot to examine how the dps
 # evolve according to mastery, haste and crit. The four corners will be : 
 # (-200 crit; +200 mastery; 0 haste) ; (-200 crit; 0 mastery; +200 haste)
 # (+200 crit; -200 mastery; 0 haste) ; (+200 crit; 0 mastery; -200 haste)
 reforge_plot_stat=crit,mastery,haste

 # This example will produce a 2D plot with crit/haste and a 3D plot with mastery/versatility/haste
 reforge_plot_stat=crit,haste/mastery,versatility,haste
```
  * **reforge\_plot\_amount** (scope: global; default: 200) is the maximum amount to reforge per stat.
```
 # This example will produce a 2D plot with (+500 crit ; -500 mastery) on
 # the left side and (-500 crit ; +500 mastery) on the right side.
 reforge_plot_stat=crit,mastery
 reforge_plot_amount=500
```
  * **reforge\_plot\_step** (scope: global; default: 20) is the stat difference between two points. It's NOT the number of steps: a lower value will generate more points! Beware of the computation when studying three or more stats: for K stats, the data generation has an o((N/2)^K) complexity, where N is the numbers of steps.
```
 # This example will produce a 2D plot with (+500 crit ; -500 haste) on
 # the left side and (-500 crit ; +500 haste) on the right side.
 # 11 points will be printed, it will require 10 additional simulations
 # since we already have the baseline one.
 reforge_plot_stat=crit,haste
 reforge_plot_amount=500
 reforge_plot_step=200

 # This example will produce data for a 3D plot. 55 points will be 
 # generated. It will require 54 additional simulations since we 
 # already have the baseline one.
 reforge_plot_stat=crit,mastery,haste
 reforge_plot_amount=500
 reforge_plot_step=200
```

  * **reforge\_plot\_iterations** (scope: global; default: -1), is the number of iterations to use for every point. Value lesser or equal than zero will make Simulationcraft use the baseline number of iterations.  It is advised you first try to reduce the number of steps before you reduce these settings: a graph will a few accurate points is usually better than a graph with many, inaccurate, points.
```
 reforge_plot_iterations=5000
```

  * **reforge\_plot\_output\_file** (scope: global; default: "") is the name of the csv file to write. An empty string will make the application print them on the standard output (the "console" output). For simulations with many characters, a single file will be written and the player name will be printed as a header before his csv data.
```
 # Saves the csv data under "reforge.csv".
 reforge_plot_output_file=reforge.csv

 # The generated data will look like this:
 # John
 # mastery, crit, haste, dps
 # -200, 0, 200, 27399 
```
