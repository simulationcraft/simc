---
title: Examples
---
_This documentation is a part of the [TCI](TextualConfigurationInterface) reference._

**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Import your character
The following script will import your character from Battle.net and save it under _profile.simc_.
```
 armory=us,illidan,john
 save=profile.simc
```
The following script will import your character from Battle.net, using his inactive spec, and save it under _profile\_offspec.simc_.
```
 armory=us,illidan,john,spec=inactive
 save=profile_offspec.simc
```

You can also use a locally saved JSON formatted [Blizzard community API](http://blizzard.github.io/api-wow-docs/#character-profile-api) character object to import a character.
```
 local_json=john.json,john
 save=profile.simc
```

# Compute stats scaling
Run a simulation to compute John's scale factors.
```
 profile.simc
 iterations=10000
 calculate_scale_factors=1
 html=scaling.html
```
Let's make some changes: a positive delta for crit rating.
```
 profile.simc
 iterations=10000
 calculate_scale_factors=1
 html=scaling.html

 scale_crit_rating=300
```

# Compare the PTR with live servers
Run a simulation with both your live character and his evil twin on the PTR. The simulation will have two characters: _John\_Live_ and _John\_PTR_
```
 ptr=0
 profile.simc
 name=John_Live

 ptr=1
 copy=John_PTR

 html=ptr_vs_live.html
```

# Compare trinkets
Run a simulation to compare John with his evil twin using different trinkets.
```
 profile.simc

 copy=john_evil_twin
 trinket1=heart_of_ignacious,id=65110
 trinket2=shard_of_woe,id=60233

 html=trinkets_comparison.html
```

# Stat plotting
Generate a _haste versus dps_ graph, with a haste delta varying within a `[-300 ; +300]` interval.
```
 profile.simc
 iterations=10000
 dps_plot_stat=haste
 dps_plot_points=10
 dps_plot_step=60

 html=haste_plot.html
```

# Play with lag
Let's simulate a 250ms lag.
```
 profile.simc
 channel_lag=0.5
 gcd_lag=0.250
 html=lag.html
```

# Play with buffs
Let's simulate a self-buffed character.
```
 profile.simc
 optimal_raid=0
 html=self_buffed.html
```
Let's keep an optimal raid but disable bloodlust, a shaman's actions list will trigger it.
```
 profile.simc
 override.bloodlust=0
 html=manual_bloodlust.html
```
# Add a fake enchant
Let's add a fake 500 strength enchant on john
```
 profile.simc
 enchant_strength=500
 html=john_500str.html
```
# Make the raid suffer periodic damages
```
 profile.simc
 raid_events+=/damage,cooldown=30,amount=20000
 html=damages.html
```
