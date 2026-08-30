---
title: Statistical Behaviour
---
_This documentation is a part of the [TCI](TextualConfigurationInterface.md) reference._

**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Introduction
Because of the very nature of simulations (see [FormulationVsSimulation](FormulationVsSimulation)), Simulationcraft produces slightly different results on every run. This is why multiple iterations of the same fight are performed, to produce average and more stable results, and smoothen out the randomness. The following options can help you to tweak out those problems.

# Default Behavior

If the user does not specify any settings, SimC will use 0.2% target error and a maximum of 1000000 (1M) iterations.

The GUI defaults to to the same settings as above.

If `iterations` is specified and `target_error` is empty, SimC will run that exact number of iterations.

# Target Error
  * **target\_error** (scope:global, default:0.2), when different from zero, will potentially end the simulation before all iterations complete.  The simulator tracks metrics based upon player role (dps, heal, tank) each iteration.  Using this growing sample of metrics it can examine the distribution of values to determine statistical error.  As iterations increase, the error decreases.  Once the error reaches the specified target level, the simulator stops iterating and generates reports.  Examples of error values are in current reports for values like player DPS.  Note that the maximum number of iterations when target\_error is specified is 1000000 unless set explicitly.
```
 target_error=0.2
```

# Iterations
  * **iterations** (scope: global; default: 1000000 (1M)) is the number of simulated fights per run. Increasing this setting is the most obvious way to improve the accuracy and stability of the simulations but it also increases computations times.  The normal default is 1000000 (1M) with target_error=0.2
```
 iterations=10000
```
> You can give a look at [FightingVariance](../appendixes/FightingVariance.md) if you want more information on the relationship between the number of iterations and the variance of the results.



# Constant seed
Pseudo-random number generators (rng) can be seen as pseudo-random numbers sequences. By default, Simulationcraft will use a different sequence on every run, which is why two consecutive runs using the same inputs still produce different outputs. A constant seed forces Simulationcraft to always use the same sequence : if Simulationcraft was only using rolls between 1 and 6, a constant seed would ensure the sequence would always be, for example, 1-5-4-4-3-6-..., whatever those numbers would be used for.

  * **seed** (scope: global; default:0) is the seed of the pseudo-random sequence. When set to 0, Simulationcraft will use a different seed on every run, based on the execution time. When different from zero, the given seed will be used to generate the same pseudo-random sequence across all runs. Incremented by 1 for every thread.
```
 seed=1247695
```

Note: there are some misconceptions about constant seeds. You may think it is useful to compare slightly different inputs but it is probably not : just switching one piece of gear and increasing haste means the actions order will change : if your highest dpet action was previously favored by good rolls on the first run, those good rolls can now have been consumed by something else. Really, what constant seeds just achieve is to ensure that two different runs with two identical inputs yield identical outputs.

## Deterministic
  * **deterministic** (scope:global, default:0), when different from zero, will seed our rng packages with an arbitrary, hard-coded, value (31459, incremented for every thread). Equal to the option seed=31459.
```
 deterministic=1
```

# Averaging rolls
  * **average\_range** (scope:global, default:1), when different from zero, will force rolls for damages ranges, such as the one for weapon damages, to always return the average value. This setting does not affect rolls for the attack table, chances of procs, etc. It has little to no effect on the accuracy of the simulation but it helps reducing the variance, hence why it is enabled by default.
```
 average_range=0
```

  * **average\_gauss** (scope:global; default:0), when different from zero, will force normal distribution rolls (see [Wikipedia - Normal distribution](http://en.wikipedia.org/wiki/Normal_distribution)), such as the ones for gcd variation, travel time variation on raid movement, etc, to always return the average value. It obviously makes settings such as **gcd\_stddev** useless.
```
 average_gauss=1
```
