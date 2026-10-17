"""Reader for trajectory files written by write_nn_trajectory (format version 1)."""

from dataclasses import dataclass

import numpy as np


@dataclass
class Trajectory:
    raw_limit: float
    norm_limit: float
    inputs: list          # [(expression, offset, scale)]
    actions: list         # [action line]
    episodes: int
    features: np.ndarray  # float32 [S, N], raw values
    mask: np.ndarray      # bool    [S, M]
    action: np.ndarray    # int64   [S]
    log_prob: np.ndarray  # float32 [S]
    time: np.ndarray      # float64 [S]
    reward: np.ndarray    # float64 [S]
    episode: np.ndarray   # uint32  [S]
    terminal: np.ndarray  # bool    [S]

    def normalized_features(self) -> np.ndarray:
        """Network input, computed exactly as the C++ side does (float32 raw -> float64)."""
        offset = np.array([o for _, o, _ in self.inputs], dtype=np.float64)
        scale = np.array([s for _, _, s in self.inputs], dtype=np.float64)
        x = (self.features.astype(np.float64) - offset) / scale
        return np.clip(x, -self.norm_limit, self.norm_limit)


def read_trajectory(path: str) -> Trajectory:
    with open(path, "rb") as f:
        blob = f.read()

    pos = 0

    def line() -> str:
        nonlocal pos
        end = blob.index(b"\n", pos)
        s = blob[pos:end].decode("utf-8")
        pos = end + 1
        return s

    def keyed(key: str) -> str:
        parts = line().split(" ", 1)
        if parts[0] != key or len(parts) != 2:
            raise ValueError(f"{path}: expected '{key} <value>'")
        return parts[1]

    if line() != "nn_trajectory 1":
        raise ValueError(f"{path}: not a version 1 trajectory file")
    S = int(keyed("steps"))
    episodes = int(keyed("episodes"))
    raw_limit = float(keyed("raw_limit"))
    norm_limit = float(keyed("norm_limit"))

    inputs = []
    for _ in range(int(keyed("inputs"))):
        expr, offset, scale = line().rsplit(" ", 2)
        inputs.append((expr, float(offset), float(scale)))
    actions = [line() for _ in range(int(keyed("actions")))]
    if line() != "data":
        raise ValueError(f"{path}: expected 'data'")

    N, M = len(inputs), len(actions)

    def take(dtype, count):
        # The file is always little-endian; convert to native byte order so the arrays work
        # everywhere downstream (torch.from_numpy rejects non-native byte order).
        nonlocal pos
        native = np.dtype(dtype)
        arr = np.frombuffer(blob, dtype=native.newbyteorder("<"), count=count, offset=pos)
        pos += arr.nbytes
        return arr.astype(native)

    features = take(np.float32, S * N).reshape(S, N)
    mask = take(np.uint8, S * M).reshape(S, M).astype(bool)
    action = take(np.uint32, S).astype(np.int64)
    log_prob = take(np.float32, S)
    time = take(np.float64, S)
    reward = take(np.float64, S)
    episode = take(np.uint32, S)
    terminal = take(np.uint8, S).astype(bool)
    if pos != len(blob):
        raise ValueError(f"{path}: {len(blob) - pos} trailing bytes")

    # Cheap consistency checks.
    if S:
        if not mask[np.arange(S), action].all():
            raise ValueError(f"{path}: a chosen action was not legal")
        if terminal.sum() != episodes or not terminal[-1]:
            raise ValueError(f"{path}: episode boundaries are inconsistent")
        same_episode = episode[1:] == episode[:-1]
        if (np.diff(time)[same_episode] < 0).any():
            raise ValueError(f"{path}: time goes backwards within an episode (two iterations fused?)")

    return Trajectory(raw_limit, norm_limit, inputs, actions, episodes,
                      features, mask, action, log_prob, time, reward, episode, terminal)
