---
title: Hunters
---
**Is there an error? Something missing? Funky grammar? Do not hesitate to leave a comment.**



# Textual configuration interface
_This section is a part of the [TCI](TextualConfigurationInterface) reference._

Regular spells are not mentioned here, you just have to follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting).

## Pets
Pets can be declared and edited as any character with the **pet** setting (see [Characters#Pets](Characters#Pets)). A couple of notes:
  * Supported pets are not listed here, there are many of them though. Their names follow the standard [names formatting rules](TextualConfigurationInterface#Names_formatting). In doubt, you can check the `sc_util.cpp` source file and search for `util::pet_type_string` to get the list of pets.

  * The **summon\_pet** setting (scope: character; default: `turtle`) is used to specify the pet this character should use.
```
 # Here are some examples
 summon_pet=wind_serpent
 summon_pet=spirit_beast
 summon_pet=crocolisk
```
  * Finally, you can use the _summon\_pet_ action to summon the pet you previously specified.
```
 # Summon your pet.
 actions+=/summon_pet
```
  * Specifying `summon_pet=disabled` will make the _summon\_pet_ action not summon any pet.

## Primary pet expressions
Primary (main) hunter pet can be referenced in pet expressions as `pet.main` instead of using the name set by the _summon\_pet_ option. This way action lists work properly with any pet type.
```
actions+=/barrage,if=pet.main.buff.frenzy.remains>execute_time
```

## Placing a hunter in front of his target
The **position** setting (scope: character; default: `back`) can be used to specify whether the character should start in front of the target, or at its back. Acceptable values are `front`, `ranged_front`, `back`, and `ranged_back`.
```
 position=front
```

## Focus regen during casting
(New for 6.0) All hunter actions have an additional property, `cast_regen`. The property returns the amount of focus regenerated during the execution of the action (casting, channel, or GCD). This enables easier conditionals to protect against focus capping. For example, in MM where Aimed Shot can be used to consume focus, an ability might be used unless it would focus cap after casting the ability and aimed\_shot.
```
 actions+=/steady_shot,if=cast_regen+action.aimed_shot.cast_regen<focus.deficit
```

## Estimating "real" cooldown durations
Several hunter spells and abilities have mechanics that can reduce their base cooldown durations significantly by the time they are ready again. Two custom expressions exist to give values closer to what the final cooldown durations are after accounting for reductions: `duration_guess` and `remains_guess`, each replacing the generic cooldown expressions `duration` and `remains` respectively.
```
 actions+=/trueshot,if=cooldown.trueshot.duration_guess+buff.trueshot.duration>target.time_to_die
 actions+=/bestial_wrath,if=cooldown.aspect_of_the_wild.remains_guess>5
```

## Bloodseeker remaining duration
Kill Command for Survival hunters has an additional expression to return the remaining duration of the Bloodseeker damage over time effect on the current target: `bloodseeker.remains`.
```
actions+=/kill_command,target_if=min:bloodseeker.remains
```

## Wildfire Infusion
The next (currently "active") Wildfire Infusion bomb type can be checked with a `next_wi_bomb.[shrapnel|pheromone|volatile]` expression. If the talent is not active the expression always return _false_.
```
action+=/wildfire_bomb,if=next_wi_bomb.shrapnel&dot.serpent_sting.remains>5*gcd
```

## Tar Trap
The module supports casting Tar Trap and implements 2 related expressions:
 * `tar_trap.remains` - returns the remaining duration on the Tar Trap slowing effect
 * `tar_trap.up` - returns true if the Tar Trap slowing effect is active (basically a shortcut to `tar_trap.remains>0`)

# Reports
All entries for hunters are fairly obvious and therefore not documented.
