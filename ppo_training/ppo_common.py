"""NumPy-only helpers for the PPO trainer: policy file I/O, normalization, GAE, running simc."""

import os
import subprocess

import numpy as np


# ---------------------------------------------------------------------------
# Policy files (format version 1, see nn_policy.hpp)
# ---------------------------------------------------------------------------

def _content_lines(path):
    with open(path) as f:
        for raw in f:
            line = raw.strip()
            if line and not line.startswith("#"):
                yield line


def read_policy_header(path):
    """Reads the version/inputs/actions sections of a policy file; layers are ignored.

    Returns (expressions, actions). Input lines must be '<expression> <offset> <scale>';
    the offset and scale are ignored (the trainer computes its own).
    """
    lines = _content_lines(path)

    def keyed(key):
        parts = next(lines).split()
        if len(parts) != 2 or parts[0] != key:
            raise ValueError(f"{path}: expected '{key} <n>'")
        return parts[1]

    if keyed("version") != "1":
        raise ValueError(f"{path}: unsupported version")
    expressions = []
    for _ in range(int(keyed("inputs"))):
        parts = next(lines).rsplit(None, 2)
        if len(parts) != 3:
            raise ValueError(f"{path}: input lines must be '<expression> <offset> <scale>'")
        expressions.append(parts[0])
    actions = [next(lines) for _ in range(int(keyed("actions")))]
    if len(set(actions)) != len(actions):
        raise ValueError(f"{path}: duplicate actions")
    return expressions, actions


def read_policy_layers(path):
    """Reads a full policy file. Returns (inputs [(expr, offset, scale)], actions, layers [(W, b, act)])."""
    lines = list(_content_lines(path))
    pos = 0

    def take():
        nonlocal pos
        pos += 1
        return lines[pos - 1]

    assert take() == "version 1"
    n_in = int(take().split()[1])
    inputs = []
    for _ in range(n_in):
        e, o, s = take().rsplit(None, 2)
        inputs.append((e, float(o), float(s)))
    n_act = int(take().split()[1])
    actions = [take() for _ in range(n_act)]
    layers = []
    while pos < len(lines):
        _, a, b, act = take().split()
        a, b = int(a), int(b)
        W = np.array([[float(x) for x in take().split()] for _ in range(b)]).reshape(b, a)
        bias = np.array([float(x) for x in take().split()])
        layers.append((W, bias, act))
    return inputs, actions, layers


def write_policy(path, expressions, offsets, scales, actions, layers):
    """Writes a policy file. layers: list of (W [n_out, n_in], b [n_out], activation)."""
    def fmt(x):
        return f"{float(x):.17g}"

    out = ["version 1", f"inputs {len(expressions)}"]
    out += [f"{e} {fmt(o)} {fmt(s)}" for e, o, s in zip(expressions, offsets, scales)]
    out += [f"actions {len(actions)}"] + list(actions)
    for W, b, act in layers:
        n_out, n_in = W.shape
        out.append(f"layer {n_in} {n_out} {act}")
        out += [" ".join(fmt(w) for w in row) for row in W]
        out.append(" ".join(fmt(x) for x in b))
    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        f.write("\n".join(out) + "\n")
    os.replace(tmp, path)


# ---------------------------------------------------------------------------
# Statistics
# ---------------------------------------------------------------------------

def feature_normalization(raw_features, min_std=1e-6):
    """Per-feature (offset, scale) = (mean, std); constant features get scale 1."""
    x = raw_features.astype(np.float64)
    offset = x.mean(axis=0)
    scale = x.std(axis=0)
    scale[scale < min_std] = 1.0
    return offset, scale


def episode_returns(traj):
    return np.bincount(traj.episode, weights=traj.reward, minlength=traj.episodes)


def gae(rewards, values, times, terminal, lam_per_sec):
    """Undiscounted (gamma = 1) GAE with a time-based lambda: lambda_t = lam_per_sec ** dt.

    Every episode ends with a true termination (no bootstrapping past the last step).
    Returns (advantages, value_targets).
    """
    n = len(rewards)
    adv = np.empty(n)
    r = rewards.tolist()
    v = values.tolist()
    tm = times.tolist()
    term = terminal.tolist()
    next_adv = 0.0
    for t in range(n - 1, -1, -1):
        if term[t]:
            delta = r[t] - v[t]
            next_adv = delta
        else:
            delta = r[t] + v[t + 1] - v[t]
            next_adv = delta + (lam_per_sec ** (tm[t + 1] - tm[t])) * next_adv
        adv[t] = next_adv
    return adv, adv + values


def explained_variance(pred, target):
    var = np.var(target)
    return float("nan") if var == 0 else float(1.0 - np.var(target - pred) / var)


# ---------------------------------------------------------------------------
# Running the simulator
# ---------------------------------------------------------------------------

def run_simc(simc, profile, policy_path, trajectory_path, iterations, seed, extra_args, log_path):
    cmd = list(simc) + [profile, f"policy={policy_path}", f"trajectory={trajectory_path}",
                        f"iterations={iterations}", f"seed={seed}"] + list(extra_args)
    with open(log_path, "w") as log:
        result = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        raise RuntimeError(f"simc failed with exit code {result.returncode}; see {log_path}")
    if not os.path.exists(trajectory_path):
        raise RuntimeError(f"simc did not write {trajectory_path}; see {log_path}")
