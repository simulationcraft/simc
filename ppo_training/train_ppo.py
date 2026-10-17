"""PPO trainer driving simc in rounds.

Each round: export the current policy to a policy file, run simc with it to collect a
trajectory batch, run PPO updates on that batch. Python holds the authoritative weights
(checkpoint.pt); policy files are exports for simc.

Round 0 collects data with an exactly uniform policy (all-zero output layer). From that
batch the trainer fixes the input normalization (per-feature mean/std) and the reward
scale (magnitude of the mean episode return), so nothing has to be tuned per profile.

Example:
    python train_ppo.py --simc ./simc --profile profile.simc --template inputs.txt \
        --workdir runs/frost --rounds 200 --iterations 2000 --simc-arg threads=8

GPU training: add --device cuda --dtype float32 (consumer GPUs are very slow in float64).
--device and --dtype can be changed when resuming from a checkpoint.
"""

import argparse
import csv
import os
import secrets
import time

import numpy as np
import torch
import torch.nn as nn

from ppo_common import (episode_returns, explained_variance, feature_normalization, gae,
                        read_policy_header, run_simc, write_policy)
from read_trajectory import read_trajectory

MASKED_LOGIT = -1e9  # finite stand-in for -inf: exp() underflows to exactly 0, no NaN gradients

LOG_HEADER = ["round", "steps", "episodes", "mean_return", "return_stderr", "reward_scale",
              "entropy", "approx_kl", "clip_frac", "value_loss", "explained_var",
              "logprob_parity", "epochs", "seconds",
              "t_sim", "t_load", "t_prep", "t_update", "t_save"]


# ---------------------------------------------------------------------------
# Networks
# ---------------------------------------------------------------------------

def make_mlp(n_in, hidden, n_out, out_std):
    layers, prev = [], n_in
    for h in hidden:
        lin = nn.Linear(prev, h)
        nn.init.normal_(lin.weight, 0.0, np.sqrt(2.0 / prev))  # He init for ReLU
        nn.init.zeros_(lin.bias)
        layers += [lin, nn.ReLU()]
        prev = h
    out = nn.Linear(prev, n_out)
    if out_std > 0:
        nn.init.normal_(out.weight, 0.0, out_std)
    else:
        nn.init.zeros_(out.weight)
    nn.init.zeros_(out.bias)
    layers.append(out)
    return nn.Sequential(*layers)


def export_layers(net):
    linears = [m for m in net if isinstance(m, nn.Linear)]
    return [(m.weight.detach().cpu().double().numpy(), m.bias.detach().cpu().double().numpy(),
             "linear" if i == len(linears) - 1 else "relu")
            for i, m in enumerate(linears)]


def masked_log_probs(policy, x, mask):
    logits = policy(x).masked_fill(~mask, MASKED_LOGIT)
    return torch.log_softmax(logits, dim=-1)


@torch.no_grad()
def chunked(fn, n, chunk=65536):
    """Evaluates fn(slice) over [0, n) in chunks and concatenates, to bound peak memory."""
    return torch.cat([fn(slice(s, min(s + chunk, n))) for s in range(0, n, chunk)])


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def save_checkpoint(path, state):
    tmp = path + ".tmp"
    torch.save(state, tmp)
    os.replace(tmp, path)


class PhaseTimer:
    """Wall-clock time per phase; synchronizes CUDA so GPU work is attributed correctly."""

    def __init__(self, device):
        self.cuda = device.type == "cuda"
        self.times = {}
        self._last = time.perf_counter()

    def mark(self, phase):
        if self.cuda:
            torch.cuda.synchronize()
        now = time.perf_counter()
        self.times[phase] = self.times.get(phase, 0.0) + now - self._last
        self._last = now


def open_log(log_path):
    if os.path.exists(log_path):
        with open(log_path, newline="") as f:
            header = next(csv.reader(f), None)
        if header != LOG_HEADER:
            os.replace(log_path, log_path + ".old")
            print(f"log format changed; previous log moved to {log_path}.old")
    new_log = not os.path.exists(log_path)
    log_file = open(log_path, "a", newline="")
    log = csv.writer(log_file)
    if new_log:
        log.writerow(LOG_HEADER)
    return log_file, log


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--simc", required=True, help="simc command")
    p.add_argument("--profile", required=True)
    p.add_argument("--template", required=True, help="policy file providing the inputs and actions sections")
    p.add_argument("--workdir", required=True)
    p.add_argument("--rounds", type=int, default=100, help="total rounds including round 0")
    p.add_argument("--iterations", type=int, default=2000, help="simc iterations per round")
    p.add_argument("--simc-arg", action="append", default=[], help="extra simc argument, repeatable")
    p.add_argument("--hidden", default="64,64")
    p.add_argument("--lr", type=float, default=3e-4)
    p.add_argument("--value-lr", type=float, default=1e-3)
    p.add_argument("--epochs", type=int, default=6)
    p.add_argument("--minibatch", type=int, default=4096)
    p.add_argument("--clip", type=float, default=0.2)
    p.add_argument("--entropy", type=float, default=0.01)
    p.add_argument("--max-grad-norm", type=float, default=0.5)
    p.add_argument("--target-kl", type=float, default=0.03, help="stop the epochs early above this (<=0 disables)")
    p.add_argument("--lam", type=float, default=0.95, help="GAE lambda per second of fight time")
    p.add_argument("--rescale-factor", type=float, default=2.0,
                   help="re-estimate the reward scale when |mean return| / scale leaves [1/f, f]")
    p.add_argument("--device", default="cpu", help="torch device: cpu, cuda, cuda:1, ...")
    p.add_argument("--dtype", default="float64", choices=["float64", "float32"])
    p.add_argument("--seed", type=int, default=None,
                   help="random seed; default: a fresh random seed for a new run, the stored one when resuming")
    p.add_argument("--keep-trajectories", action="store_true")
    return p.parse_args()


def main():
    args = parse_args()
    device = torch.device(args.device)
    if device.type == "cuda" and not torch.cuda.is_available():
        raise RuntimeError("--device cuda requested but CUDA is not available")
    dtype = torch.float64 if args.dtype == "float64" else torch.float32
    torch.set_default_dtype(dtype)

    os.makedirs(args.workdir, exist_ok=True)
    ckpt_path = os.path.join(args.workdir, "checkpoint.pt")
    log_path = os.path.join(args.workdir, "log.csv")
    simc = args.simc.split()
    hidden = [int(h) for h in args.hidden.split(",") if h]

    # Seed: stored in the checkpoint so a resumed run keeps it; a new run without --seed
    # draws a fresh one, so runs are independent unless a seed is passed explicitly.
    state = torch.load(ckpt_path, map_location=device, weights_only=False) if os.path.exists(ckpt_path) else None
    if state is not None:
        stored = state.get("seed", 1)  # checkpoints from before this option used seed 1
        if args.seed is not None and args.seed != stored:
            print(f"note: --seed {args.seed} overrides the checkpoint's seed {stored}")
        seed = args.seed if args.seed is not None else stored
    else:
        seed = args.seed if args.seed is not None else secrets.randbelow(2**31 - 1) + 1
    print(f"seed {seed}")
    torch.manual_seed(seed)

    def simc_seed(r):
        # simc's seed is an unsigned 64-bit integer where 0 means "pick a random seed".
        return (seed * 100003 + r) % 2**64 or 1

    expressions, actions = read_policy_header(args.template)
    n_in, n_act = len(expressions), len(actions)

    def paths(r):
        return (os.path.join(args.workdir, f"policy_{r:04d}.txt"),
                os.path.join(args.workdir, f"batch_{r:04d}.bin"),
                os.path.join(args.workdir, f"simc_{r:04d}.log"))

    def collect(r, policy_net, offset, scale, timer):
        policy_path, traj_path, simc_log = paths(r)
        write_policy(policy_path, expressions, offset, scale, actions, export_layers(policy_net))
        run_simc(simc, args.profile, policy_path, traj_path, args.iterations, simc_seed(r),
                 args.simc_arg, simc_log)
        timer.mark("sim")
        traj = read_trajectory(traj_path)
        if [e for e, _, _ in traj.inputs] != expressions or traj.actions != actions:
            raise RuntimeError(f"{traj_path}: inputs/actions do not match the template")
        if not args.keep_trajectories:
            os.remove(traj_path)
        timer.mark("load")
        return traj

    policy = make_mlp(n_in, hidden, n_act, out_std=0.01).to(device)
    value = make_mlp(n_in, hidden, 1, out_std=0.01).to(device)
    policy_opt = torch.optim.Adam(policy.parameters(), lr=args.lr)
    value_opt = torch.optim.Adam(value.parameters(), lr=args.value_lr)

    if state is not None:
        if state["expressions"] != expressions or state["actions"] != actions:
            raise RuntimeError("checkpoint was made with different inputs/actions; use a new --workdir")
        # load_state_dict copies into the existing parameters, converting dtype/device as needed.
        policy.load_state_dict(state["policy"])
        value.load_state_dict(state["value"])
        policy_opt.load_state_dict(state["policy_opt"])
        value_opt.load_state_dict(state["value_opt"])
        offset, scale, reward_scale = state["offset"], state["scale"], state["reward_scale"]
        start_round = state["round"]
        # Fresh shuffling sequence after a resume, rather than replaying round 1's.
        torch.manual_seed(seed + start_round)
        del state
        print(f"resumed at round {start_round}")
    else:
        # Round 0: exactly uniform policy; normalization in this file is irrelevant.
        uniform = make_mlp(n_in, hidden, n_act, out_std=0.0)
        timer = PhaseTimer(device)
        traj = collect(0, uniform, np.zeros(n_in), np.ones(n_in), timer)
        offset, scale = feature_normalization(traj.features)
        returns = episode_returns(traj)
        # Magnitude, so negative-return objectives (e.g. -time to kill) work too.
        reward_scale = float(abs(returns.mean()))
        if reward_scale == 0:
            raise RuntimeError("round 0 mean episode return is zero")
        print(f"round 0: {len(traj.action)} steps, {traj.episodes} episodes, "
              f"mean return {returns.mean():.6g} (uniform policy), "
              f"sim {timer.times['sim']:.1f}s  load {timer.times['load']:.1f}s")
        start_round = 1

    print(f"training on {device} in {args.dtype}")
    log_file, log = open_log(log_path)

    for r in range(start_round, args.rounds):
        timer = PhaseTimer(device)
        traj = collect(r, policy, offset, scale, timer)

        raw_returns = episode_returns(traj)
        mean_return = float(raw_returns.mean())
        stderr = float(raw_returns.std(ddof=1) / np.sqrt(len(raw_returns))) if len(raw_returns) > 1 else float("nan")

        # Scale-only PopArt: keep |value targets| around 1 without changing value predictions.
        ratio = abs(mean_return) / reward_scale
        if ratio > args.rescale_factor or ratio < 1.0 / args.rescale_factor:
            new_scale = abs(mean_return)
            out = [m for m in value if isinstance(m, nn.Linear)][-1]
            with torch.no_grad():
                out.weight.mul_(reward_scale / new_scale)
                out.bias.mul_(reward_scale / new_scale)
            reward_scale = new_scale

        x = torch.from_numpy(traj.normalized_features()).to(device=device, dtype=dtype)
        mask = torch.from_numpy(traj.mask).to(device)
        act = torch.from_numpy(traj.action).to(device)
        old_lp = torch.from_numpy(traj.log_prob).to(device=device, dtype=dtype)
        n = len(traj.action)
        timer.mark("load")

        values = chunked(lambda s: value(x[s]).squeeze(-1), n).cpu().double().numpy()
        lp_now = chunked(lambda s: masked_log_probs(policy, x[s], mask[s]).gather(1, act[s, None]).squeeze(1), n)
        parity = float((lp_now - old_lp).abs().max())

        adv, targets = gae(traj.reward / reward_scale, values, traj.time, traj.terminal, args.lam)
        ev = explained_variance(values, targets)
        adv_t = torch.from_numpy((adv - adv.mean()) / (adv.std() + 1e-8)).to(device=device, dtype=dtype)
        targets_t = torch.from_numpy(targets).to(device=device, dtype=dtype)
        timer.mark("prep")

        # Statistics stay on the device and are synchronized once per epoch, not per minibatch.
        stats = {"entropy": [], "kl": [], "clip": [], "vloss": []}
        epochs_done = 0
        for epoch in range(args.epochs):
            perm = torch.randperm(n, device=device)
            epoch_kl = []
            for start in range(0, n, args.minibatch):
                idx = perm[start:start + args.minibatch]
                mb_mask = mask[idx]

                log_probs = masked_log_probs(policy, x[idx], mb_mask)
                new_lp = log_probs.gather(1, act[idx, None]).squeeze(1)
                log_ratio = new_lp - old_lp[idx]
                ratio_t = log_ratio.exp()
                surr = torch.min(ratio_t * adv_t[idx],
                                 ratio_t.clamp(1 - args.clip, 1 + args.clip) * adv_t[idx])
                probs = log_probs.exp()
                entropy = -(probs * log_probs).sum(-1)  # illegal actions: 0 * -1e9 = 0
                policy_loss = -surr.mean() - args.entropy * entropy.mean()

                policy_opt.zero_grad()
                policy_loss.backward()
                nn.utils.clip_grad_norm_(policy.parameters(), args.max_grad_norm)
                policy_opt.step()

                v_pred = value(x[idx]).squeeze(-1)
                value_loss = nn.functional.huber_loss(v_pred, targets_t[idx], delta=1.0)
                value_opt.zero_grad()
                value_loss.backward()
                nn.utils.clip_grad_norm_(value.parameters(), args.max_grad_norm)
                value_opt.step()

                with torch.no_grad():
                    kl = ((ratio_t - 1) - log_ratio).mean()
                    epoch_kl.append(kl)
                    stats["kl"].append(kl)
                    stats["clip"].append(((ratio_t - 1).abs() > args.clip).to(dtype).mean())
                    stats["entropy"].append(entropy.mean())
                    stats["vloss"].append(value_loss.detach())
            epochs_done += 1
            if args.target_kl > 0 and float(torch.stack(epoch_kl).mean()) > args.target_kl:
                break
        summary = {k: float(torch.stack(v).mean()) for k, v in stats.items()}
        timer.mark("update")

        save_checkpoint(ckpt_path, {
            "round": r + 1, "seed": seed, "expressions": expressions, "actions": actions,
            "offset": offset, "scale": scale, "reward_scale": reward_scale,
            "policy": policy.state_dict(), "value": value.state_dict(),
            "policy_opt": policy_opt.state_dict(), "value_opt": value_opt.state_dict(),
        })
        timer.mark("save")

        t = timer.times
        total = sum(t.values())
        log.writerow([r, n, traj.episodes, mean_return, stderr, reward_scale,
                      summary["entropy"], summary["kl"], summary["clip"], summary["vloss"], ev,
                      parity, epochs_done, total,
                      t["sim"], t["load"], t["prep"], t["update"], t["save"]])
        log_file.flush()
        print(f"round {r}: return {mean_return:.6g} ± {stderr:.3g}  entropy {summary['entropy']:.3f}  "
              f"kl {summary['kl']:.4f}  clip {summary['clip']:.3f}  vloss {summary['vloss']:.4f}  "
              f"ev {ev:.3f}  parity {parity:.1e}  epochs {epochs_done}  {total:.1f}s "
              f"[sim {t['sim']:.1f} load {t['load']:.1f} prep {t['prep']:.1f} "
              f"update {t['update']:.1f} save {t['save']:.1f}]")

    # Final export of the trained policy.
    final = os.path.join(args.workdir, "policy_final.txt")
    write_policy(final, expressions, offset, scale, actions, export_layers(policy))
    print(f"wrote {final}")


if __name__ == "__main__":
    main()
