# PPO training of action-selection policies

This directory contains a trainer that learns an action-selection policy for an actor
with reinforcement learning (PPO) The policy is a small neural network: it reads a set
of sim expressions describing the current state and scores each of a fixed list of
actions; the actor then uses the highest-scoring action that is currently usable.

The trainer runs in rounds. Each round it:

1. writes the current network to a policy file,
2. runs simc with that policy, which records every decision the actor made into a
   trajectory file,
3. updates the network from the trajectory.

simc does all the simulating and runs the network itself; Python only does the
learning. Nothing is exchanged between the two during a sim.

## Requirements

- Python 3 with NumPy and PyTorch (`pip install numpy torch`). A GPU is optional.

The trainer consists of `train_ppo.py`, `ppo_common.py` and `read_trajectory.py`; keep
the three files together. `train_ppo.py` can be run from any directory.

## Quick start

```
python ppo_training/train_ppo.py \
    --simc ./simc \
    --profile my_actor.simc \
    --template my_actor_template.txt \
    --workdir runs/my_actor \
    --rounds 200 --iterations 2000
```

When it finishes, the trained policy is in `runs/my_actor/policy_final.txt`. See
[Using the trained policy](#using-the-trained-policy) for how to sim with it.

## Preparing the policy template

The template tells the trainer which inputs the network sees and which actions it chooses
from. It uses the policy file format without the network layers:

```
# Comments and blank lines are ignored.
version 1
inputs 5
buff.fingers_of_frost.stack 0 1
buff.brain_freeze.react_remains 0 1
debuff.freezing.stack 0 1
cooldown.frozen_orb.remains 0 1
fight_remains 0 1
actions 5
frostbolt
glacial_spike
ice_lance
flurry
frozen_orb
```

### Inputs

Each input line is `<expression> <offset> <scale>`. In a template the offset and scale
are placeholders: write `0 1`. The trainer measures every input in the first round and
computes the normalization itself, so you never need to know an expression's range.

Choosing inputs is the most important part of the setup. The network can only react to
what it can see, so include everything a good player would look at:

- stacks and remaining duration of relevant buffs and debuffs,
- cooldown remaining and charges of each relevant action,
- resources,
- remaining fight time (`fight_remains`), so the network can learn end-of-fight behaviour,
- for actors with cast-while-casting or off-GCD actions: whether the actor is currently
  casting, and how much of the cast is left.

For random procs, `buff.<name>.react_remains` is often a better input than `remains`. It
is the remaining duration once the proc can be reacted to, and 0 before that. It gives
the network the same information a player has, at the same time.

Rules for expressions:

- Expressions that need a target, such as `debuff.*` or `dot.*`, are evaluated against
  the target of the **first action in the action list**. Make the first action a plain
  single-target spell without `target_if=` or other targeting options.
- An expression that evaluates to NaN stops the sim with an error. Infinite values, such
  as time-to-die on an immortal target, are clamped to ±1,000,000.

### Actions

Each action line is an ordinary APL action, exactly as it would appear in an action
priority list. Action options such as `use_item,slot=trinket1` work.

- **Use one plain action per line.** Each line must create exactly one action. Do not
  use meta-actions such as `call_action_list`, `run_action_list` and `variable`.
- **Conditions are hard constraints.** An `if=` condition on an action line is part of
  that action's usability check. The network can never choose the action while the
  condition is false. This is useful for rules you are certain of, but every condition
  narrows what the trainer can discover, so leave them out when in doubt.
- Leave out actions the actor can never use; they only add noise.

### Waiting: the `nothing` action

The special action `nothing` lets the network deliberately do nothing. What that means
depends on when the network is asked:

- At a **regular decision point**, the actor waits for a short moment, about 0.1 s, and
  then decides again.
- During **off-GCD or cast-while-casting checks**, which simc runs repeatedly while those
  actions are possible, it means "don't use any of them now".

Options are added after a comma, like action options:

| Option | Meaning |
|---|---|
| `ctx=<mask>` | Where this `nothing` can be chosen, as a sum of: `1` regular decisions, `2` off-GCD checks, `4` cast-while-casting checks. For example, `ctx=6` allows it only during off-GCD and cast-while-casting checks. Default: everywhere. |
| `avail=1` | After choosing it during an off-GCD or cast-while-casting check, skip the following checks until the set of usable actions changes, or until the next regular decision. This turns "not now" into "not until something changes", one decision instead of a series of repeated ones. At regular decision points it behaves like a plain `nothing`. |

Rules:

- A template can contain several `nothing` lines with different options. Each one is a
  separate choice for the network. All `nothing` lines must come after all other actions.
- Include `nothing` only when waiting can be the right choice, such as holding an
  off-GCD spell for a better moment. Choosing it at regular decision points wastes time
  early in training, while the policy is still mostly random, so restrict it with
  `ctx=6` unless waiting at regular decision points is genuinely useful.

A typical setup for an actor with off-GCD or cast-while-casting spells:

```
nothing,ctx=6
nothing,ctx=6,avail=1
```

The first skips a single check, for precise timing within a cast. The second holds until
something changes.

### Reusing a policy file as a template

Any policy file, such as a previous run's `policy_final.txt`, also works as a template.
Its offsets, scales and layers are ignored.

## Running the training

### Options

| Option | Default | Meaning |
|---|---|---|
| `--simc` | required | Command that runs simc. It is split on spaces, so the path itself must not contain spaces. |
| `--profile` | required | The actor's profile. |
| `--template` | required | The policy template (see above). |
| `--workdir` | required | Output directory for this run. |
| `--rounds` | 100 | Total number of rounds, including round 0. |
| `--iterations` | 2000 | simc iterations per round. See [Batch size](#batch-size). |
| `--simc-arg` | | Extra simc argument, such as `threads=8`. Repeatable. |
| `--hidden` | `64,64` | Hidden layer sizes. `64,64,64` gives three hidden layers. |
| `--device` | `cpu` | `cpu` or `cuda`. |
| `--dtype` | `float64` | `float64` or `float32`. Use `float32` on a GPU. |
| `--seed` | random | Random seed. Without it, every new run gets a fresh seed, which is printed at startup. |
| `--lam` | 0.95 | GAE λ per second of fight time. Lower values reduce noise in the learning signal; see [Noisy actors](#noisy-actors). |
| `--lr` | 3e-4 | Learning rate of the policy network. |
| `--value-lr` | 1e-3 | Learning rate of the value network. |
| `--epochs` | 6 | Passes over each batch. |
| `--minibatch` | 4096 | Minibatch size, in decisions. |
| `--entropy` | 0.01 | Bonus for keeping the policy random; higher values mean more exploration. |
| `--clip` | 0.2 | PPO clipping range. |
| `--target-kl` | 0.03 | Stop a round's epochs early if the policy changes more than this. `0` disables it. |
| `--max-grad-norm` | 0.5 | Gradient clipping. |
| `--rescale-factor` | 2.0 | When returns drift by more than this factor, the reward scale is re-estimated. |
| `--keep-trajectories` | off | Keep each round's trajectory file instead of deleting it after use. |

The defaults work well for most actors. Usually only `--iterations`, `--rounds`,
`--hidden` and the hardware options need setting.

### What happens in round 0

Round 0 runs a policy that picks uniformly at random among the usable actions. The
trainer uses this data to compute the input normalization and the reward scale, then
keeps both fixed for the rest of the run. Nothing needs tuning per profile, and
expected damage per iteration in round 0 is far below normal.

### Output files

| File | Contents |
|---|---|
| `checkpoint.pt` | Complete training state, saved after every round. |
| `log.csv` | One row of statistics per round. |
| `policy_NNNN.txt` | The policy simc used in round `NNNN`. |
| `simc_NNNN.log` | simc's output for round `NNNN`. Look here first if simc fails. |
| `policy_final.txt` | The trained policy, written after the last round. |

### Resuming and extending a run

Run the same command again and training continues from the last completed round. To
train longer, raise `--rounds`. You can change `--device` and `--dtype` when resuming.

The inputs and actions of a run are fixed. To change them, start a new run in a new
`--workdir`.

### Reading the log

Each round prints a line like:

```
round 57: return 5.66e+07 ± 1.5e+05  entropy 0.091  kl 0.0007  clip 0.008  vloss 0.0000  ev 1.000  parity 4.6e-07  epochs 6  7.7s [sim 0.3 load 0.1 prep 0.8 update 6.3 save 0.0]
```

- **return**: mean damage per iteration in this round, with its standard error. This is
  the number that should go up. It comes from a slightly random version of the policy,
  so it is a little below what the final policy achieves. Judge progress from the trend
  over 10–20 rounds rather than from single rounds.
- **entropy**: how random the policy still is. It should fall gradually. A collapse to
  near 0 within a few rounds means the policy committed to a strategy too early; raise
  `--entropy`.
- **kl, clip, epochs**: how much each update changed the policy. If `epochs` is often
  below the `--epochs` setting, updates are too large; lower `--lr`.
- **vloss, ev**: value-network loss and explained variance. `ev` is normally close to 1
  and is not a useful health indicator on its own.
- **parity**: the largest difference between the probabilities simc logged and the ones
  Python recomputes from the same data. Expect about 1e-7 with `float64` and 1e-6 with
  `float32`. A larger value means simc and the trainer disagree about the network's
  input or the usable actions, and training results should not be trusted.
- **The bracketed times**: seconds spent per phase. `sim` is simc and `update` is the
  learning step.

### Batch size

What matters for learning is the number of **decisions** per round, shown in the `steps`
column of `log.csv`, not the number of iterations. Aim for roughly 500,000 decisions per
round and set `--iterations` accordingly. A typical actor makes 200–300 decisions per
iteration, but some make many more, so check the first rounds and adjust.

### Speed

- The update step usually dominates the round time. `--dtype float32` makes it roughly
  2× faster on the CPU.
- `--device cuda --dtype float32` helps further, especially with larger networks or
  minibatches. Do not use `float64` on a consumer GPU; it is very slow there.

### Network size

`64,64` is a good default. Actors with complex rotations can benefit from a third layer,
such as `64,64,64`. Prefer adding layers over widening them: wider layers make every
simc decision more expensive, and make the update step slower. To check whether a
larger network really helps, train both sizes with several seeds and compare the ranges.
A single run per size is not enough.

### Noisy actors

Actors whose damage depends heavily on random effects learn more slowly. Two changes
help:

- lower `--lam` to about 0.9,
- roughly double both `--iterations` and `--minibatch`.

Before doing that, check that every random effect that changes the right decision, such
as a proc, is visible in the inputs.

## Using the trained policy

`policy=` and `trajectory=` are actor options: they apply to the actor defined last
before them. Putting them after the profile on the command line therefore applies them
to that profile's actor.

### Simming with the policy

```
./simc my_actor.simc policy=runs/my_actor/policy_final.txt
```

All the usual options, such as `target_error=`, `html=` and `threads=`, work as normal.
Any `policy_NNNN.txt` from the work directory can be used the same way, for example to
check a run in progress.

**Do not pass `trajectory=` when evaluating.** With `trajectory=`, simc assumes it is
training and picks actions at random according to the network's scores. Without it,
simc always picks the highest-scoring usable action, which is the policy you want to
evaluate.

### Comparing with the priority list

To compare the policy with the profile's hand-written priority list in a single sim,
copy the actor and attach the policy only to the copy:

```
./simc my_actor.simc copy=my_actor_ppo policy=runs/my_actor/policy_final.txt target_error=0.05
```

Evaluate with the same fight settings (fight style, length and length variance, target
count) that were used for training.

## Caveats

- **The reward is total damage**, including pet damage. Fight setups where the goal is
  something else are not supported yet. In particular, with a fixed-health target every
  iteration deals the same total damage, so there is nothing to learn. Train with
  time-based fights.
- **The policy replaces every action list except `precombat`.** The profile must have a
  `default` action list; its contents are discarded. `precombat` still runs as written
  and is not learned.
- **One actor per profile.** The trainer appends `policy=` and `trajectory=` after the
  profile, so they apply to the last actor the profile defines. Train with a profile that
  contains only the actor being trained.
- **Input expressions are evaluated against the first action's target**; see [Inputs](#inputs).
- **Training uses a fixed number of iterations per round.** If the profile sets
  `target_error`, disable it for training, for example with `--simc-arg target_error=0`.
  Otherwise simc decides the number of iterations itself and the batch size varies.
- **Outputs set in the profile are rewritten every round.** If the profile sets `html=`,
  `json2=` or similar, simc writes them in every training round. Remove them from the
  training profile; they only cost time.
- **A trained policy is specific to its setup.** It is trained for the profile's gear,
  talents and fight settings, and may perform worse under different ones. Retrain when
  these change significantly.
- **Inputs that never change in round 0** get a default normalization. That is usually
  harmless, but if an important input only starts changing once the policy improves,
  consider whether something in the profile prevents it from varying under random play.
- **Learned behaviour can exploit the sim.** When a policy beats the priority list,
  examine where they disagree before concluding the policy found a better strategy. The
  difference may come from a modelling quirk instead.
