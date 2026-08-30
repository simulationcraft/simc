---
title: Cache Control
---
SimulationCraft caches character profiles and item descriptions from various Internet sources locally to avoid retrieving the same information repeatedly. You can control how and when the cached information is used with the cache control options `cache_items` and `cache_players`.

## `cache_items` (scope: global, default: 1)

If non-zero, SimulationCraft will prefer to use cached copies of item descriptions over fetching fresh data from network sources. If set to `only` SimulationCraft will never fetch from network sources, resulting in failure if an item is not found in the cache. Defaults to `1` since item descriptions rarely change.

## `cache_players` (scope: global, default: 0)

If non-zero, SimulationCraft will prefer to use cached copies of character profiles over fetching fresh data from network sources. If set to `only` SimulationCraft will never fetch from network sources, resulting in failure if a profile is not found in the cache. Defaults to `0` since typical usage of network character profiles is to simulate, edit, and repeat.

### Why `only`?
Setting `cache_items` and/or `cache_players` to `only` is useful for SimulationCraft performance testing.
