---
title: Action Lists
---
_This documentation is a part of the [TCI](TextualConfigurationInterface) reference._

**Is there an error? Something missing? Funky grammar? Do not hesitate to create an Issue or report it in the SimCMinMax Discord.**

# Default Action Lists

Most specializations provide two built-in action lists, a default action list maintained by the community for SimC and the Blizzard Assisted Combat (Single-Button Assist, One-Button Rotation).

## Default

Default APLs are provided if no actions are explicitly provided and Assisted Combat options are not provided. They are maintained as a part of SimC. No specific options to enable or disable this action list exist.

Implementations may be found [here](https://github.com/simulationcraft/simc/tree/thewarwithin/engine/class_modules/apl).
Current APLs may be found [here](https://github.com/simulationcraft/simc/tree/thewarwithin/ActionPriorityLists/default).

## Assisted Combat

Starting in 11.1.7, Blizzard provides Assisted Combat rotations, otherwise known as Single-Button Assist or One-Button Rotation. Note that each of these options are actor-scope, thus must occur after an actor is created (a line containing a class name e.g. `monk=Credger`).

Do note that the conditions used are not all compatible with the SimC expression syntax, and require modification. Most changes are annotated as comments in saved SimC input files, HTML output, etc.

Current APLs may be found [here](https://github.com/simulationcraft/simc/tree/thewarwithin/ActionPriorityLists/assisted_combat).

### Enable Basic Assisted Combat APL
```
use_blizzard_action_list=1
```

### Enable Basic Cooldowns
Not all cooldowns are used by default, thus we will provide additional cooldowns as necessary.
```
use_cds_with_blizzard_action_list=1
```

### One-Button Mode
Enables the Global Cooldown penalty (GCD) inherent when using Assisted Combat in One-Button mode.
```
one_button_mode=1
```

### Enable Spell Queue
Provides spell queuing behaviour to make action list a little more consistent with in game behaviour.

May exhibit bugs or crashes.
```
enable_spell_queue=1
```

### Spell Queue Window
Spell queue window used for `enable_spell_queue` option.
```
spell_queue_window=400
```

# Additional Action Lists
Multiple action lists can be defined with `actions.<list_name>`. This mostly helps with readability by creating sub-action lists. A sample 'cooldown' action list is below:
```
actions.cooldown=metamorphosis
actions.cooldown+=/potion
actions.cooldown+=/use_item,name=algethar_puzzle_box
actions.cooldown+=/the_hunt
```

## Using Additional Action Lists
Once additional action lists are defined, there are two ways to utilize them from the default action list.

### call_action_list
_call\_action\_list_ takes one argument (name=) of the action list to call.  If a usable action is not found in the sub-action list, the APL will go back to the place _call\_action\_list_ was used and continue going down until it finds a usable action.  _call\_action\_list_ effectively inserts the action list into the default APL where it was called. For example, using the cooldown action list from above:
```
actions=auto_attack
actions+=/call_action_list,name=cooldown
actions+=/immolation_aura
```

Would be the same as:
```
actions=auto_attack
actions+=/metamorphosis
actions+=/potion
actions+=/use_item,name=algethar_puzzle_box
actions+=/the_hunt
actions+=/immolation_aura
```

### run_action_list
_run\_action\_list_ also requires the name= parameter of the action list to run.  The main difference is if _run\_action\_list_ is called and there are no usable actions in the sub-APL, it will return to the top of the default action list with a delay.

# Behaviour
Actions lists are priorities lists: periodically, Simulationcraft scans your character's actions list, starting with the first action (the highest priority ) and continuing until an available action is found, or to the end otherwise. Actions that are not possible at the moment (cooldown not ready, execute phase only, conditions not met) are just considered as not available and the applications jumps to the next one.

Here is a simplified warrior actions list:
```
# If the character has no flask already, use a greater draenic strength one
actions=flask,type=greater_draenic_strength_flask
# Otherwise, use a draenic strength potion if the targets health is below 20 percent and recklessness is up, or if the target will die in 25 seconds.
actions+=/potion,name=draenic_strength,if=(target.health.pct<20&buff.recklessness.up)|target.time_to_die<=25
# Otherwise, starts autoattack if not done already.
actions+=/auto_attack
# Otherwise, pops up the "recklessness" 3 min cooldown if ready.
actions+=/recklessness
# Otherwise, uses bloodthirst if ready.
actions+=/bloodthirst
# Otherwise, use colossus smash if ready.
actions+=/colossus_smash
# Otherwise, use execute (only available when target health is below 20% health but it is always implicit)
actions+=/execute
# Otherwise, use raging blow, but only if target is above 20% health
actions+=/raging_blow,if=target.health_pct>=20
```

A couple pieces of advice for writing actions lists:

1. Do not forget, it is priority-based, it's as simple as that!
1. Do not try to optimize your actions lists for computations performances. Just focus on correctly modeling the gameplay you want.
1. Have doubts? Make Simulationcraft write a combat log for you, with the **log** option, see [Output](Output).

# Syntax
  * **actions** (scope: current character; default: _depends on class and spec_) is the list of actions your character will follow. It is a multi-line string and a sequence of commands using the "/" separator.

```
 # Note how we use the "/" separator and the "+=" appending operator.
 actions=dosomething
 actions+=/dosomethingelse
```

# Available actions

## Basics
  * _auto\_attack_ triggers the auto-attack when it's not already activated. It cannot be used to stopping or resetting auto attacks. Options are:
    1. _sync\_weapons_ (optional, default: 0), when different from zero, will synchronize weapons swings for ambidextrous classes with two weapons with the same swing speed. When zero, the offhand will be delayed by half of its swing time. In game, you always start with your weapons synchronized but target switching and parry rushes often lead you to go unsynchronized. Some mechanics are pretty sensible to weapons being synchronized or note, such as [flurry](http://www.wowhead.com/spell=12972/flurry).
```
 # Ensure the player will always auto attack and will start with synched weapons.
 actions+=/auto_attack,sync_weapons=1
```
  * _snapshot\_stats_ forces simulationcraft to capture your buffed stats values just before the combat actually begins. It has no influence on the simulation itself is it's totally optional. However you need to include it if you want the reports and output to display the correct (non-zero) values for raid-buffed stats. It should be located after all other out-of-combat actions (food, flasks, etc), and BEFORE the first potion.
```
 # Ensure the reports will display correct values for "raid-buffed" stats.
 actions+=/snapshot_stats
```
  * _cancel\_buff_ cancels a buff, just like "/cancelaura" would do in game.
    1. _name_ is the name of the buff to cancel.
```
 # Cancels raging blow buff if 2 stacks are up, which has never been used before in game, but hey, it's an example.
 actions+=/cancel_buff,name=raging_blow,if=buff.raging_blow.stack=2
```
  * _use\_item_ triggers the use of an item.
    1. _name_ is the item's name
    1. Alternatively, you can also specify an item by the _slot_ or the on-use _effect_name_
    1. **Since Simulationcraft 6.1.2-01** In addition, you can also evaluate the type of stat buff the item triggers with the `use_buff.<stat_type>` expression. It evaluates to 1 if the item triggers the expressed stat, 0 otherwise.
```
 # Uses the "shard of woe" trinket
 actions+=/use_item,name=shard_of_woe,if=cooldown.evocation.remains>86
```
  * _restore\_mana_ forcefully and instantly restores mana.
    1. _mana_ (default: 0) is the amount of mana to restore. When left to zero, mana will be restored to its maximum. The purpose of this action is to let you test out infinite mana scenarios or fights with special mana regeneration mechanics. Since this action has no cooldown, it can be performed many times every second.
```
 # This will restore 500 mana anytime it is triggered.
 actions+=/restore_mana,mana=500
```

## Spells
See the relevant page for each class for more information on non-trivial spells.

Spells are added on a per-class basis. Those keywords are the spells' names, where white spaces are replaced with underscores (`_`) and non-alphanumeric characters are ignored. The list would obviously be too long to write and boring to maintain but you can check your class' source code file (sc\_mage.cpp for mages for example) for the "create\_action" function.
```
 # This will make a feral druid cast Tiger's fury.
 actions+=/tigers_fury
```

## Racials
  * _arcane\_torrent_ triggers arcane torrent (blood elf racial)
```
 actions+=/arcane_torrent
```
  * _berserking_ triggers berserking (troll tracial)
```
 actions+=/berserking
```
  * _blood\_fury_ triggers blood fury (orc racial)
```
 actions+=/blood_fury
```
  * _stoneform_ triggers stoneform (dwarf racial)
```
 actions+=/stoneform
```

## Pets
Pets' actions can be used through a specific syntax: `<petname>:<petaction>`. Relevant options are the ones for the specified pet action.
```
 actions+=/spider:wolverine_bite
```
However most pets actions actually have shortcut keywords you will probably prefer:
```
 actions+=/wolverine_bite
```
Note that pets come with their own default actions lists! You can modify them as you do with any regular character.

## External Buffs
External buffs like Power Infusion (haste buff from Priest) can be called and set up for an APL/profile.
Implemented buffs:
- Power Infusion
- Symbol of Hope

A character scoped option `external_buffs.pool` declares the availability of an external buff. Syntax:
```
external_buffs.pool=buff_name1:cooldown:quantity/buff_name2:cooldown:quantity
```
Minimum two arguments per buff, quantity is assumed to be 1 if not present.
E.g. `external_buffs.pool=power_infusion:120` will add a single Power Infusion as an available external buff.

To use available external buffs the APL needs to have actions called `invoke_external_buff`. Schema similar to normal actions:
```
  actions+=/invoke_external_buff,name=buff_name1,if=condition
```
Example
```
  actions+=/invoke_external_buff,name=power_infusion,if=buff.dancing_rune_weapon.up|!talent.dancing_rune_weapon
```

For debugging the option `use_pool` with default 1 (on!) exists. Turning
this off stops it from using the cooldown pool and therefore must be
controlled by a line cd or fancy apl logic. This is for testing
purposes! **Do not include lines with `use_pool=0` in default APLs!**


## Consumables
  * Note that post-profession revamp from Dragonflight, the tokenized name used below will need to be followed by the numerical tier of the consumable.
```
 actions+=/potion,type=elemental_potion_of_power_3
```

  * _food_ can trigger the use of a food.
    1. _type_ is the name of the food to use.
```
 actions+=/food,type=blackrock_barbecue
```
  * _flask_ can trigger the use of a flask.
    1. _type_ is the name of the flask to use.
```
 actions+=/flask,type=greater_draenic_strength_flask
```
  * _health\_stone_ can trigger the use of a health stone.
    1. _health_ or _trigger_ is the absolute hp deficit you must suffer to allow the use of the stone.
```
 # If a character has his health 60k below his maximum, he will use the stone
 actions+=/health_stone,trigger=60000
```
  * _potion_ can trigger the use of a potion. Only 1 potion may be used in combat.
```
 # Triggers the potion use when out of combat or when bloodlust has just started.
 actions+=/potion,name=draenic_strength,if=!in_combat|buff.bloodlust.react
```
  * _augmentation_ can trigger the use of a Augmentation Rune.
    1. _type_ is either focus, hyper or stout.
```
 # Triggers Focus Augmentation Rune.
 actions+=/augmentation,type=focus
```
  * _oralius\_whispering\_crystal_ and _crystal\_of\_infinity_ can trigger the use of the special consumables found in WoW. (**Since Simulationcraft 6.0.3-26**)

## Movement
  * _start\_moving_ triggers a movement phase, it will end only with _stop\_moving_.
  * _stop\_moving_ ends the movement phase.
```
 # Here are fragments of the shadowpriest rotation. When the target health is below 25%, sw:d deals three times more damages and has a chance to make apparitions spawn. This chance is increased while the sp moves. 

 # Do not move when sw:d is on cooldown 
 actions+=/stop_moving,health_percentage<=25,if=cooldown.shadow_word_death.remains>=0.2
 # Start moving when the sw:d cooldown is going to end (we need to stand still to fire it)
 actions+=/start_moving,health_percentage<=25,if=cooldown.shadow_word_death.remains<=0.1
```

## Sequences
Actions sequences are sub-actions chains to execute in a given order. Use the following keyword:

  * _sequence_ declares and triggers a sequence of actions to use in the specified order. Actions are separated with ":".

    1. _name_ (optional, default: "default") is used to name the sequence.

Once one of the sub-actions has been performed, Simulationcraft does not immediately perform the next sub-action in the chain. Instead, it restarts at the beginning of the whole actions list (not the sequence). If the sequence is executed again, then it will trigger the actions which have not been performed yet.
```
# So, some class has three spells: yellow, blue and red. They all share the global cooldown.
actions+=/yellow
actions+=/sequence,red:blue

# On the first gcd, yellow is not ready, red and blue are: the application will execute "red"
# On the second gcd, all spells are ready: the application will execute "yellow"
# On the third gcd, yellow is not ready, red and blue are: the application will execute "blue"
# From now on, the application will only perform "yellow".
```

When all spells have been performed, the sequence is not automatically reinitialized and it will be skipped from now on! You need to use the following keyword:

* _restart\_sequence_ will restart the specified sequence.

  1. _name_ is the name of the sequence to restart.
```
# Here are fragments of the 4.0.6 mage rotation
actions+=/sequence,name=attack:fire_melee:fire_nova:fire_blast
actions+=/restart_sequence,name=attack,moving=0
```

Finally, you can use  _wait\_on\_ready_ (default: -1) on a sequence or one of its sub-actions. When equal to 1, it will force the the application to restart at the beginning of the actions list processing if this spell is not ready. Practically, actions below this one will never be executed. However, things are slightly different for sequences:

  1. If the sequence itself is flagged as wait\_on\_ready, all spells with _wait\_on\_ready=-1_ will be flagged with the value you specified whenever the sequence is restarted.
  1. The sequence will be considered as flagged whenever the next remaining spell is flagged with _wait\_on\_ready=1_.
```
# Here is a sample of an old death knight sequence. Spells have been replaced with their abbreviations to make things easier.
actions+=/sequence,wait_on_ready=1:PS:IT:BS:BS:SS
actions+=/DC
actions+=/restart_sequence,name=default

# At first, because of wait_on_ready, the application will flag all spells with wait_on_ready=1. It means the application will have to wait on every step and until the sequence has been completed. It will never reach the "DC" line even if one of the spells is not ready yet.
# So, the application will perform PS-IT-BS-BS-SS. Then, as long as it can do DC, it will. Once there is not enough runic power left for DC, the sequence will be restarted.
```
Do you have headaches already ? Yes, sequences are tricky, they are rarely used. If you do use them, do it with caution.

## Strict Sequences
Strict Sequences are a breed of sequence, except for they do not need to be reset, and when they are started, they cannot be stopped under normal circumstances. A strict sequence requires all actions in the sequence to be ready for the duration of the sequence.  

Unlike normal sequences, strict sequences must have a _name_, there is no default.
```
# Arms Warrior will perform Recklessness, bloodbath, colossus smash, mortal strike, whirlwind, whirlwind when all are available.
# The name allows you to call this sequence from other parts of the action list. 
actions+=/strict_sequence,name=swifty:recklessness:bloodbath:colossus_smash:mortal_strike:whirlwind
```

## Waits

  * _wait_ orders the application to stop processing the actions list for a given time. Auto-attacks and such will still be performed.

    1. _sec_ (default: 1) is the number of seconds to wait. It can be a constant or an expression (see the [conditional expressions](Action-List-Conditional-Expressions.md) page).
```
# This orders Simulationcraft to stop processing the actions list for 5s
actions+=/wait,sec=5

# This orders Simulationcraft to stop processing the actions list until only 2s remains before somebuff expires (if 5s remaining, wait 3s).
actions+=/wait,sec=buff.somebuff.remains-2
```

  * _wait\_until\_ready_ orders the player to stop processing the actions list until some cooldown or dot expires. Its only purpose is to improve performances but beware: conditions such as a buff expiration, reaction to heroism/bloodlust, etc, won't be checked.

    1. _sec_ (default: 1) is the maximum time, in seconds, the player will wait. One second is enough, it reduces the actions list processing cost by an order of magnitude. And since some conditions won't automatically wake up the application, it is advised to keep it low. Just as for _wait_, this option can be an expression as well as a simple number.
```
actions+=/wait_until_ready,sec=0.5
```

## Resources
* _pool\_resource_ will force the application to stop processing the actions list while the resource is restored. By default, the primary resource of the spec is pooled. 
    1. _wait_ (default: 0.251) is the time, in seconds, to wait. It is advised to keep this value low so that the resource pooling will only occur as long as the application reaches this very action and as its conditions are satisfied.
    1. _for\_next_ (default: 0), when different from 0, will force the application to wait until the player has enough resources for the following action in list. If the following action already satisfies its resource criteria or if it is made unavailable for other reasons than resource starvation (cooldown for example), then _pool\_resource_ will be ignored.
    1. _extra\_amount_ (default: 0), **must** be used with _for\_next_ parameter. When different from 0, it will require an additional amount of resource to be generated (in addition to the cost of the next action).

```
# First example, without using for_next: the application will pool energy while the player has less than 60 energy and slice and dice must soon be refreshed (within 5s).
actions+=/pool_resource,if=energy<60&buff.slice_and_dice.remains<5
actions+=/slice_and_dice,if=combo_points>=3&buff.slice_and_dice.remains<2

# Second example, with for_next: if the player is not stealthed and has less than 5 combo points but the player has less than 85 energy (the only non-satisfied criteria for shadow dance), then the application will pool energy until the player has 85 or more energy. If the player is stealthed or has 5 combo points, both lines will be skipped.
actions+=/pool_resource,for_next=1,extra_amount=85
actions+=/shadow_dance,if=energy>=85&combo_points<5&buff.stealthed.down
```
## APL Variables
APL variables take the general form of _variable,name=,<default=>,<value=>,<op=>,<delay=>,<condition=>,<if=>_ If all optional values are omitted the variable will default to the _set_ operation.
* _name_ is the user assigned name for the variable. This can be used to reference that variable later or to perform additional operations on it.
* _default_ is the initial value of the variable. If not given as an option, 0 will be used.
* _value_ is the value which is to be used for the operation. This supports any string which can be evaluated to a value as well.
* _op_ is the operation to perform on the variable. Possible values are:
    1. _print_ prints the current value of the variable to the log. Requires either _log=1_ or _debug=1_ to generate a log. a 1 second _delay_ value will be used as default to prevent spooling. 
    1. _reset_ current value is reset to _default_.
    1. _floor_ performs the floor operation on _value_ and sets current value to the result.
    1. _ceil_ performs the ceil operation on _value_ and sets current value to the result.
The following operations also require the _value_ to be set:
    1. _set_ sets the value in the variable to the value in the _value_ parameter
    1. _add_ adds _value_ to the current value and sets current value to the result.
    1. _sub_ subtracts _value_ from the current value and sets current value to the result.
    1. _mul_ multiplies _value_ by the current value and sets current value to the result.
    1. _div_ divides the current value by _value_ and sets current value to the result. If _value_ is 0 then current value will be set to 0.
    1. _pow_ raises current value to the power of _value_ and sets current value to the result.
    1. _mod_ performs the modulo operation on current value with _value_ and sets current value to the result.
    1. _min_ performs the min operation on current value and _value_ and sets the current value to the result.
    1. _max_ performs the max operation on current value and _value_ and sets the current value to the result.
    1. _setif_ Requires the additional parameter _value\_else_. If _condition_ evaluates to a non-zero value then sets current value to _value_. Else if _value_ evaluates to 0 then sets current value to _value\_else_
    1. _report_ report changes to the value of the variable as an entry in the Sample Sequence Table in the APL section of the HTML report.
* _delay_ is the delay (simulation time) before the variable action can be executed again.
* _if_ allows [Conditional expressions](Action-List-Conditional-Expressions.md) to control execution of the variable action.
* **apl_variable** (scope: current character) is an option that can be used to override the _default_ value of an APL variable.
```
# override the default value of an APL variable called "aoe_threshold" to 5.
apl_variable.aoe_threshold=5
```

For multi-target sims, _cycling\_variable_ can be used to perform a variable operation on every target. For example:

```
# Count the number of targets that have Agony at more than 5 seconds remaining
actions+=/variable,name=agony_over_5_count,op=reset
actions+=/cycling_variable,name=agony_over_5_count,op=add,value=dot.agony.remains>=5
```

# Actions modifiers
All actions have additional options, we're listing them here.

## Selecting the target

* _target_ (default: "") is the action's target.  When empty, it will be the default target (the player himself for healers, the main target for damage dealers and tanks). To force a spell to target yourself, use the syntax `target=self`.
```
# cast power_infusion on actor named John
actions+=/power_infusion,target=John
# cast holy prism on yourself
actions+=/holy_prism,target=self
```
* _cycle\_targets_ will cycle the action through all available targets when set to 1.
```
# Use moonfire on any target that does not currently have it.
actions+=/moonfire,cycle_targets=1,if=!ticking
```
* _max\_cycle\_targets_ will set a maximum amount of targets to cycle through.
```
# Cycles through only 3 targets.
actions+=/moonfire,cycle_targets=1,max_cycle_targets=3,if=!ticking
```
* _target\_if_ selects a target for the action based on the expression value. In `first` mode (default if unspecified), the action selects the first target for which the expression value evaluates to nonzero, or skips the action if no such target is found. In `min` or `max` mode, the action evaluates the expression value for all possible targets and selects the target for which the expression value is minimal/maximal respectively. Note that the _if_ expression is performed after the _target\_if_ expression has selected a target. So if the _target\_if_ is used in `min` mode and the _if_ expression fails on that target, it won't try the next minimum target but will instead just skip the action.
```
# Cast Agony on any target in pandemic range
actions+=/agony,target_if=refreshable
# Cast Agony on the target with the lowest remaining Agony duration, if it is in pandemic range
actions+=/agony,target_if=min:remains,if=refreshable
```

**Note**: if you are looking for how to check specific buffs/debuffs on a target check the [Conditional Expressions page](Action-List-Conditional-Expressions.md#buffs-and-debuffs#buffs-and-debuffs).

## Usage on specific events only

* _buff.bloodlust.react_ (default: 0), when different from zero, will flag the action as usable only when bloodlust (heroism, time warp, etc) is active. When left to zero, the action will be usable anytime.
```
# Let's use recklessness only under bloodlust.
actions+=/recklessness,if=buff.bloodlust.react=1
```
* _target.debuff.invulnerable.react_ (default: 0), when different from zero, will flag the action as usable only when the target is invulnerable (happens only when you specified an invulnerability raid event). When left to zero, the action will be usable anytime.
```
# Using this at the top of the actions list will force the player to wait (through 0.5s steps) and do nothing while the target is invulnerable.
actions+=/wait,sec=0.5,target.debuff.invulnerable.react=1
```
* _target.debuff.vulnerable.react_ (default: 0), when different from zero, will flag the action as usable only when the target is vulnerable (suffers twice more damages, it happens only when you specified a vulnerability raid event). When left to zero, the action will be usable anytime.
```
# Let's use recklessness only when the target is vulnerable.
actions+=/recklessness,if=target.debuff.vulnerable.react=1
```
* _target.debuff.flying.react_ (default: 0), when different from zero, will flag the action as usable only when the target is flying.
```
# Let's use black arrow only when the target is flying.
actions+=/black_arrow,if=target.debuff.flying.react=1
# Let's use explosive trap only when the target is on the ground.
actions+=/explosive_trap,if=target.debuff.flying.react=0
```
* _moving_ (default: -1), when different from -1, will flag the action as usable only when the players are moving (_moving=1_) or not moving (_moving=0_). When left to -1, the action will be usable anytime. The players happen to move either because of a "movement" raid event, or because of "start\_moving" actions. Note that actions which are not usable while moving do not need to be flagged with "_move=0_", Simulationcraft is already aware of those restrictions.
```
# Let's use typhoon only when the player is moving.
actions+=/typhoon,moving=1
```
* _prev_ returns the previous foreground action executed. This will include gcd and non-gcd actions, such as fireball and bloodbath.
```
# Only use pyroblast when the previous spell used was fireball
actions+=/pyroblast,if=prev.fireball
```
* _prev\_gcd_ returns only the previous action that used a GCD. This will only include actions such as fireball, but not bloodbath. Use integers to change how many GCDs in the past you are looking at.
```
# Only use whirlwind after mortal strike.
actions+=/whirlwind,if=prev_gcd.1.mortal_strike
```
* _prev\_off\_gcd_ returns all off gcd actions that occurred since the previous gcd was executed. So after a warrior uses raging blow, it will track every off-gcd action until another gcd action is executed, then it is reset.
```
# Only use recklessness if bloodbath was just executed.
actions+=/recklessness,if=prev_off_gcd.bloodbath
```

## Time-based usages
* _time_ can be used to make an action usable only when the elapsed time, in seconds, since the beginning of the fight is between specified bounds. It has to be used with the "<=" or ">=" operators. You can specify both an upper and a lower bound. It is especially useful when you want to time an action in respect to your raid events.
```
# Cast bloodlust 20s after the beginning of the fight
actions+=/bloodlust,if=time>=20
```
* _time\_to\_Xpct_ can be used to make an action usable only when the estimated remaining time, in seconds, is between specified bounds. It has to be used with the "<=" or ">=" operators. You can specify both an upper and a lower bound, and you can also set the percent. time\_to\_die will be converted into time\_to\_0pct.
```
# Cast bloodlust 60s before the estimated end the of the fight.
actions+=/bloodlust,if=time_to_die<60
# Cast recklessness if it will be available again for execute range
actions+=/recklessness,if=time_to_20pct>180
```
* _line\_cd_ can be used to force a length of time, in seconds, to pass after executing an action before it can be executed again. In the example below, the second line can execute even while the first line is being delayed because of _line\_cd_.
```
# Cast soulburn exactly once during dark soul (which has a 20s duration)
actions+=/soulburn,line_cd=20,if=buff.dark_soul.up
 
# Cast soulburn during the execute phase when UA is on its last tick
actions+=/soulburn,if=target.health.pct<=20&dot.unstable_affliction.ticks_remain<=1
```

## Cooldowns synchronization
* _sync_ (default: "") can be used to flag an action as unusable while another specified action is not ready. The given value must be the name of the synchronized action. **This line will be executed as soon as the specific action is READY, not when the action is necessarily used.**
```
# Warriors tend to pop their cooldowns at the same time. Recklessness has a 4 mins cooldown, death wish has a 2.4 mins cooldown. Let's force recklessness to wait for dw to be ready.
actions+=/recklessness,sync=death_wish
```

## Health restrictions
* _target.health.pct_ can be used to make an action usable only when the target's health percentage is between specified bounds. It has to be used with the "<=" or ">=" operators. You can specify both an upper and a lower bound.
```
# Starts bloodlust when the target's health is below 25%.
actions+=/bloodlust,target.health.pct<25
```

## Channeling
* _interrupt_ can be used on channeled spells, when set to a non-zero value, to interrupt the channeling when another action with a higher priority is ready. The interrupt will only occur immediately following a tick.
```
# Stop channeling mind flay when any other action with a higher priority is made available.
actions+=/mind_flay,interrupt=1
```
* _interrupt\_if_ can be used on channeled spells to interrupt the channeling if a higher priority action is ready (the same as _interrupt_), the global cooldown has elapsed, **and** the specified conditions are met. The interrupt will only occur immediately following a tick. The conditions are provided using the syntax for conditional expressions.
```
# Stop channeling when there is less than 1s remaining on the mind blast cooldown.
actions+=/mind_flay,interrupt_if=cooldown.mind_blast.remains<1
```
* _interrupt\_immediate_ can be used on a channeled spell that has an _interrupt\_if_ expression to instruct the actor to immediately interrupt the channeled action, even if the global cooldown has not elapsed yet. **Added in Simulationcraft 7.0.3, release 1**
* _chain_ can be used to re-cast a channeled spell at the beginning of its last tick.  This has two advantages over waiting for the channel to complete before re-casting: 1) the gcd finishes sooner, and 2) it avoids the roughly 1/4 second delay between the end of a channel and the beginning of the next cast.
```
# Chain-cast Mind Flay until a higher priority action is ready
actions+=/mind_flay,chain=1
```
* _early\_chain\_if_ has the same effect as _chain_, but with three differences: 1) it only chains the spell if the given expression is true, 2) it can chain the spell at the beginning of any tick, not just the last, and 3) it will not execute during the gcd.
```
# Chain-cast Mind Flay Insanity, restarting the cast early if Devouring Plague is about to fall off
actions+=/mind_flay_insanity,interrupt=1,chain=1,early_chain_if=dot.devouring_plague_tick.remains<=tick_time
```
* _interrupt\_global_ When set to 1 (default 0), forces an actor to look for the higher priority action in the global action list. This option alters the behavior so that the lookup begins from the actor's currently active action list. In the case of `run_action_list`, it is the action list being run. In other cases, it is the primary action list (i.e., the list defined by `actions` options).

## Non-standard timing

By default, the sim will only try to perform actions after GCD has elapsed and the actor isn't casting or channeling. Sometimes, it is desirable to use actions during GCD. This only works for actions that do not trigger GCD (such as interrupts) and can be enabled with `use_off_gcd` (default: 0).

```
# Use Water Elemental's Freeze while Ice Lance is in flight
actions+=/freeze,use_off_gcd=1,if=action.ice_lance.in_flight
```

Some actions can also be used while casting or channeling. This only works for actions that support cast while casting and can be enabled with `use_while_casting` (default: 0).

```
# Use Combustion right before Pyroblast finishes casting
actions+=/combustion,use_while_casting=1,if=action.pyroblast.executing&action.pyroblast.execute_remains<0.5
```

Note that `use_while_casting=1` does not imply `use_off_gcd=1`.

Actions following an off-GCD action are assumed to happen simultaneous with no delay, as if they were all macro'd in-game. It's possible to force a queue lag similar to normal on-GCD actions with `add_queue_lag (default: 0). This **ONLY** works for off-GCD actions; it has no effect on normal on-GCD actions.

```
# Wait for the standard queue lag after casting combustion before casting the next spell
actions+=/combustion,add_queue_lag=1
```

## Tweaking out the flight speed
* _travel\_speed_ (default: _ingame flight speed_) is the flight speed, in yards per second, of the spell (a fireball for example).
```
# Let's make our fireballs instant.
actions+=/fireball,travel_speed=0
```

## Sequences behaviour
* _wait\_on\_ready_ (default: -1), when equal to 1, will force the the application to restart at the beginning of the actions list processing if this spell is not ready. Practically, actions below this one will never be executed. You can use it to quickly make the end of the list inactive but the main purpose of this option is for sequences, see the [related section](#Sequences).
```
# Let's put wait_on_ready=1 on this line near the end of the balance druid's actions list.
actions+=/wrath,wait_on_ready=1,if=eclipse_dir=-1

# Those last lines will never be executed.
actions+=/starfire
actions+=/wild_mushroom,moving=1,if=buff.wild_mushroom.stack<3
actions+=/moonfire,moving=1
actions+=/sunfire,moving=1
```

## Enemy-Specific Modifiers
* See the article on [enemies](Enemies#Action_Lists).

# Raid Event expressions
If you are using a sim that has raid events active, you can conditionally check for properties of current or future events to handle actions properly.

Current list of raid event types:
* _adds_
    * In a DungeonRoute sim, _adds_ will return the next pull event. E.g., _raid_event.adds.count_ will return how many adds are in the next pull. _raid_event.adds.in_ and _raid_event.adds.remains_ will estimate how long the pull is by checking damage done against the pull's total HP. _raid_event.adds.duration_ returns very large numbers in DungeonRoute.
* _move_enemy_
* _casting_
* _distraction_
* _invul_ or _invulnerable_
* _interrupt_
* _movement_ or _moving_
* _damage_
* _heal_
* _stun_
* _position_switch_
* _flying_
* _damage_taken_debuff_
* _damage_done_buff_

The following expressions are available for raid events:
* _in_ checks how long until the next raid event
* _duration_ how long in seconds the raid event will last
* _cooldown_ how long in seconds is left on the raid event cooldown
* _distance_ how far away the raid event is
* _max_distance_ max distance the raid event will go while active
* _min_distance_ min distance the raid event will go while active
* _amount_ amount of damage the raid event will deal to the player (only works for damage raid events)
* _to_pct_ healing event expression
* _count_ how many adds are active in the current raid event (only works for add raid events)
* _up_ returns 1 if the raid event is currently active
* _exists_ returns 1 if there is a raid event at some point in the sim
* _remains_ find how long the current raid event will last, or 0 if no events are currently active

## Remains
```
# Check if the adds will live for 15s before casting Unholy Nova
actions+=/unholy_nova,if=!raid_event.adds.up|raid_event.adds.remains>=15
```

This condition also checks if `!raid_event.adds.up` that way this line will also work if there are no adds currently active. Alternative you can also use `raid_event.adds.exists` to see if the sim contains any adds for the entire sim.

## Check when adds/movement will spawn next
Sometimes it would be helpful to use an action only if adds aren't coming soon, otherwise you should hold your ability.

```
# Use Shadow Crash unless adds will be coming in <=10s
actions+=/shadow_crash,if=raid_event.adds.in>10
```

You can use the same check for movement raid events to make sure you don't interrupt a channel mid-way through.

```
# Use Void Torrent unless there is movement coming that will interrupt you
actions+=/void_torrent,if=raid_event.movement.in>3
```

# Conditional expressions
* See the article on [Conditional expressions](Action-List-Conditional-Expressions.md) for a in-depth guide on how to conditionally filter actions in a action priority list.
