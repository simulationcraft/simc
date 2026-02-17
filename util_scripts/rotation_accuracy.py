#!/usr/bin/env python3
"""Standalone rotation accuracy comparison tool for SimulationCraft.

Compares per-spell damage distributions between a SimC JSON output and
WarcraftLogs real combat data using cosine similarity.

Usage:
    python rotation_accuracy.py \\
        --simc-json path/to/simc_output.json \\
        --class DeathKnight --spec Unholy \\
        --wcl-client-id YOUR_ID --wcl-client-secret YOUR_SECRET

Requirements: Python 3.10+ (stdlib only, no external dependencies)
"""

import argparse
import base64
import json
import math
import os
import statistics
import sys
import time
import urllib.error
import urllib.parse
import urllib.request


# ─────────────────────────────────────────────────────────────────────
# Spell Filtering
# ─────────────────────────────────────────────────────────────────────

FILTER_PATTERNS = {
    "melee", "auto_attack", "auto_shot", "shoot",
    "touch_of_the_magi",
    "leech",
}

FILTER_PREFIXES = (
    "enchant_",
    "trinket_",
    "potion_",
    "flask_",
)

FILTER_EXACT_NORMALIZED = {
    "etherealenergy",
    "tomeoflightsdevotion",
    "eradicatingarcanocore",
    "shardsofthevoid",
    "unstablegoods",
    "unyieldingnetherprism",
    "sigilofthecosmichunt",
    "suffocatingdarkness",
    "screamsofaforgottensky",
    "hyperpyrexia",
}


def normalize_spell_name(name: str) -> str:
    """Normalize spell name for matching between SimC and WCL."""
    return name.lower().replace("_", "").replace(" ", "").replace("-", "").replace("'", "")


def should_filter(name: str) -> bool:
    """Check if a spell should be excluded from distribution comparison."""
    if not name or not name.strip():
        return True
    norm = name.lower().replace(" ", "_")
    if norm in FILTER_PATTERNS:
        return True
    if any(norm.startswith(p) for p in FILTER_PREFIXES):
        return True
    if norm.startswith("pet_") or norm.startswith("pet:"):
        return True
    if "(" in name and ")" in name:
        return True
    collapsed = normalize_spell_name(name)
    if collapsed in FILTER_EXACT_NORMALIZED:
        return True
    return False


# ─────────────────────────────────────────────────────────────────────
# Cosine Similarity
# ─────────────────────────────────────────────────────────────────────

def cosine_similarity(vec_a: list, vec_b: list) -> float:
    """Compute cosine similarity between two vectors."""
    if len(vec_a) != len(vec_b) or len(vec_a) == 0:
        return 0.0
    dot = sum(a * b for a, b in zip(vec_a, vec_b))
    mag_a = math.sqrt(sum(a * a for a in vec_a))
    mag_b = math.sqrt(sum(b * b for b in vec_b))
    if mag_a == 0 or mag_b == 0:
        return 0.0
    return dot / (mag_a * mag_b)


# ─────────────────────────────────────────────────────────────────────
# SimC JSON Parser
# ─────────────────────────────────────────────────────────────────────

def parse_simc_json(json_path: str) -> dict:
    """Parse SimC JSON2 output to extract per-spell damage distribution.

    SimC JSON structure: sim.players[0].stats[] with each entry containing:
      - spell_name: display name
      - portion_amount: damage fraction (0-1)
      - compound_amount: total damage dealt
      - num_executes.mean: average cast count

    Returns dict of {spell_name: damage_pct}.
    """
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    players = data["sim"]["players"]
    if not players:
        raise ValueError("No players found in SimC JSON output")

    stats = players[0].get("stats", [])
    if not stats:
        raise ValueError("No stats found for first player in SimC JSON")

    entries = {}
    for stat in stats:
        compound_amount = stat.get("compound_amount", 0.0)
        if compound_amount <= 0:
            continue
        spell_name = stat.get("spell_name", stat.get("name", "")).strip()
        if not spell_name:
            continue
        portion_amount = stat.get("portion_amount", 0.0)
        damage_pct = round(portion_amount * 100, 2)
        # Accumulate if multiple entries share a name
        entries[spell_name] = entries.get(spell_name, 0.0) + damage_pct

    return entries


# ─────────────────────────────────────────────────────────────────────
# WarcraftLogs API Client (embedded, stdlib-only)
# ─────────────────────────────────────────────────────────────────────

class WCLClient:
    """Minimal WarcraftLogs API v2 client using stdlib only."""

    TOKEN_URL = "https://www.warcraftlogs.com/oauth/token"
    API_URL = "https://www.warcraftlogs.com/api/v2/client"

    def __init__(self, client_id: str, client_secret: str, timeout: int = 30):
        self.client_id = client_id
        self.client_secret = client_secret
        self.timeout = timeout
        self._token = None
        self._token_expiry = 0.0
        self._last_call = 0.0
        self._authenticate()

    def _authenticate(self):
        data = urllib.parse.urlencode({"grant_type": "client_credentials"}).encode()
        credentials = base64.b64encode(
            f"{self.client_id}:{self.client_secret}".encode()
        ).decode()
        req = urllib.request.Request(
            self.TOKEN_URL,
            data=data,
            headers={
                "Authorization": f"Basic {credentials}",
                "Content-Type": "application/x-www-form-urlencoded",
            },
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=self.timeout) as resp:
            body = json.loads(resp.read().decode())
            self._token = body["access_token"]
            self._token_expiry = time.time() + body.get("expires_in", 86400) - 60

    def graphql(self, query: str, variables: dict | None = None) -> dict | None:
        if time.time() >= self._token_expiry:
            self._authenticate()
        elapsed = time.time() - self._last_call
        if elapsed < 0.25:
            time.sleep(0.25 - elapsed)

        payload = json.dumps({"query": query, "variables": variables or {}}).encode()
        req = urllib.request.Request(
            self.API_URL,
            data=payload,
            headers={
                "Authorization": f"Bearer {self._token}",
                "Content-Type": "application/json",
            },
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=self.timeout) as resp:
            self._last_call = time.time()
            body = json.loads(resp.read().decode())
            if "errors" in body:
                for err in body["errors"][:2]:
                    print(f"  WCL error: {err.get('message', err)}", file=sys.stderr)
                return None
            return body.get("data")

    def get_current_raid_encounters(self, expansion_id: int = 6) -> list:
        """Auto-detect current raid and return encounter list."""
        query = """
        { worldData { expansion(id: %d) { zones { id name
              encounters { id name } } } } }
        """ % expansion_id
        data = self.graphql(query)
        if not data:
            return []
        zones = data.get("worldData", {}).get("expansion", {}).get("zones", [])
        skip = ("Mythic+", "Complete Raids", "Delves")
        raid_zones = [
            z for z in zones
            if not any(z["name"].startswith(p) for p in skip)
            and len(z.get("encounters", [])) >= 4
        ]
        if not raid_zones:
            return []
        target = max(raid_zones, key=lambda z: z["id"])
        return [
            {"id": e["id"], "name": e["name"],
             "zone_id": target["id"], "zone_name": target["name"]}
            for e in target.get("encounters", [])
        ]

    def get_rankings_with_reports(
        self, class_name: str, spec_name: str,
        encounter_ids: list, difficulty: int = 4,
    ) -> list:
        """Get top rankings with report metadata."""
        wcl_class = class_name.replace(" ", "")
        wcl_spec = spec_name.replace(" ", "")
        eid = encounter_ids[0] if encounter_ids else None
        if not eid:
            return []
        query = """
        { worldData { encounter(id: %d) {
            characterRankings(
              className: "%s" specName: "%s"
              metric: dps difficulty: %d page: 1
            ) } } }
        """ % (eid, wcl_class, wcl_spec, difficulty)
        data = self.graphql(query)
        if not data:
            return []
        rankings_data = (
            data.get("worldData", {})
            .get("encounter", {})
            .get("characterRankings", {})
        )
        results = []
        for entry in (rankings_data or {}).get("rankings", [])[:10]:
            report = entry.get("report", {})
            if report and report.get("code"):
                results.append({
                    "amount": entry.get("amount", 0),
                    "reportCode": report["code"],
                    "fightID": report.get("fightID", 0),
                    "characterName": entry.get("name", ""),
                })
        return results

    def find_source_id(self, report_code: str, character_name: str) -> int | None:
        """Find a player's sourceID in a WCL report."""
        query = """
        query($code: String!) { reportData { report(code: $code) {
            masterData { actors(type: "Player") { id name } } } } }
        """
        data = self.graphql(query, variables={"code": report_code})
        if not data:
            return None
        actors = (
            data.get("reportData", {}).get("report", {})
            .get("masterData", {}).get("actors", [])
        )
        for actor in actors:
            if actor.get("name", "").lower() == character_name.lower():
                return actor.get("id")
        return None

    def get_damage_breakdown(
        self, report_code: str, fight_id: int, source_id: int,
    ) -> list | None:
        """Fetch per-spell damage table from one fight."""
        query = """
        query($code: String!) { reportData { report(code: $code) {
            table(dataType: DamageDone fightIDs: [%d] sourceID: %d) } } }
        """ % (fight_id, source_id)
        data = self.graphql(query, variables={"code": report_code})
        if not data:
            return None
        table = (
            data.get("reportData", {}).get("report", {}).get("table", {})
        )
        entries = (table or {}).get("data", {}).get("entries", [])
        return [
            {"name": e["name"], "total": e["total"]}
            for e in entries if e.get("total", 0) > 0 and e.get("name")
        ]

    def get_spell_distribution(
        self, class_name: str, spec_name: str,
        encounter_ids: list, difficulty: int = 4,
        max_reports: int = 5, cache_dir: str | None = None,
    ) -> tuple:
        """Get aggregated spell damage distribution from WCL top reports.

        Returns (spell_pcts: dict, report_count: int).
        """
        spec_key = f"{class_name.replace(' ', '')}_{spec_name.replace(' ', '')}"

        # Check cache
        if cache_dir:
            cache_file = os.path.join(cache_dir, f"{spec_key}_spell_dist.json")
            if os.path.exists(cache_file):
                try:
                    age_hours = (time.time() - os.path.getmtime(cache_file)) / 3600
                    if age_hours < 24:
                        with open(cache_file, "r") as f:
                            cached = json.load(f)
                            return cached, -1  # -1 = from cache
                except Exception:
                    pass

        rankings = self.get_rankings_with_reports(
            class_name, spec_name, encounter_ids, difficulty,
        )
        if not rankings:
            return None, 0

        all_spell_totals: dict = {}
        reports_sampled = 0

        for ranking in rankings[:max_reports]:
            code = ranking.get("reportCode", "")
            fight_id = ranking.get("fightID", 0)
            char_name = ranking.get("characterName", "")
            if not code or not fight_id or not char_name:
                continue

            source_id = self.find_source_id(code, char_name)
            if source_id is None:
                continue

            breakdown = self.get_damage_breakdown(code, fight_id, source_id)
            if not breakdown:
                continue

            grand_total = sum(e["total"] for e in breakdown)
            if grand_total <= 0:
                continue

            reports_sampled += 1
            for entry in breakdown:
                pct = (entry["total"] / grand_total) * 100
                name = entry["name"]
                all_spell_totals.setdefault(name, []).append(pct)

        if reports_sampled == 0:
            return None, 0

        result = {name: round(statistics.median(pcts), 2)
                  for name, pcts in all_spell_totals.items()}

        # Write cache
        if cache_dir and result:
            os.makedirs(cache_dir, exist_ok=True)
            try:
                with open(os.path.join(cache_dir, f"{spec_key}_spell_dist.json"), "w") as f:
                    json.dump(result, f, indent=2)
            except Exception:
                pass

        return result, reports_sampled


# ─────────────────────────────────────────────────────────────────────
# Core Comparison Logic
# ─────────────────────────────────────────────────────────────────────

def compare_distributions(
    simc_spells: dict, wcl_spells: dict,
) -> dict:
    """Compare SimC and WCL spell distributions using cosine similarity.

    Args:
        simc_spells: {spell_name: damage_pct} from SimC JSON
        wcl_spells: {spell_name: damage_pct} from WCL reports

    Returns dict with cosine_similarity, matched/missing spells, top-5 lists.
    """
    # Build normalized lookups (excluding filtered spells)
    simc_lookup = {}
    for name, pct in simc_spells.items():
        if should_filter(name):
            continue
        norm = normalize_spell_name(name)
        simc_lookup[norm] = simc_lookup.get(norm, 0.0) + pct

    wcl_lookup = {}
    wcl_display = {}
    for name, pct in wcl_spells.items():
        if should_filter(name):
            continue
        norm = normalize_spell_name(name)
        wcl_lookup[norm] = wcl_lookup.get(norm, 0.0) + pct
        wcl_display[norm] = name

    # Find matches
    matched = set(simc_lookup) & set(wcl_lookup)
    simc_only = set(simc_lookup) - set(wcl_lookup)
    wcl_only = set(wcl_lookup) - set(simc_lookup)

    # Top-5 from each source
    simc_sorted = sorted(simc_lookup.items(), key=lambda x: x[1], reverse=True)
    wcl_sorted = sorted(wcl_lookup.items(), key=lambda x: x[1], reverse=True)

    result = {
        "matched_spell_count": len(matched),
        "simc_unique_count": len(simc_only),
        "wcl_unique_count": len(wcl_only),
        "simc_top_5": [{"name": n, "pct": round(v, 2)} for n, v in simc_sorted[:5]],
        "wcl_top_5": [{"name": wcl_display.get(n, n), "pct": round(v, 2)} for n, v in wcl_sorted[:5]],
        "missing_in_wcl": sorted(simc_only, key=lambda n: simc_lookup.get(n, 0), reverse=True)[:10],
        "missing_in_simc": [wcl_display.get(n, n) for n in
                            sorted(wcl_only, key=lambda n: wcl_lookup.get(n, 0), reverse=True)[:10]],
    }

    if len(matched) < 3:
        result["cosine_similarity"] = None
        result["confidence"] = "insufficient_data"
        result["top_10_similarity"] = None
        return result

    # Build matched vectors
    matched_list = sorted(matched)
    simc_vec = [simc_lookup[n] for n in matched_list]
    wcl_vec = [wcl_lookup[n] for n in matched_list]

    cos_sim = cosine_similarity(simc_vec, wcl_vec)
    result["cosine_similarity"] = round(cos_sim, 4)

    # Top-10 similarity (highest-damage spells)
    top_10 = sorted(matched, key=lambda n: max(simc_lookup.get(n, 0), wcl_lookup.get(n, 0)), reverse=True)[:10]
    if len(top_10) >= 3:
        result["top_10_similarity"] = round(
            cosine_similarity(
                [simc_lookup[n] for n in top_10],
                [wcl_lookup[n] for n in top_10],
            ), 4
        )
    else:
        result["top_10_similarity"] = None

    # Confidence rating
    if cos_sim >= 0.85:
        result["confidence"] = "high"
    elif cos_sim >= 0.65:
        result["confidence"] = "medium"
    else:
        result["confidence"] = "low"

    # Per-spell comparison for matched
    spell_comparison = []
    for n in sorted(matched, key=lambda n: max(simc_lookup.get(n, 0), wcl_lookup.get(n, 0)), reverse=True)[:15]:
        spell_comparison.append({
            "spell": wcl_display.get(n, n),
            "simc_pct": round(simc_lookup[n], 2),
            "wcl_pct": round(wcl_lookup[n], 2),
            "delta": round(simc_lookup[n] - wcl_lookup[n], 2),
        })
    result["spell_comparison"] = spell_comparison

    return result


# ─────────────────────────────────────────────────────────────────────
# CLI Entry Point
# ─────────────────────────────────────────────────────────────────────

# Mapping of WoW class display names
CLASS_NAMES = {
    "deathknight": "Death Knight", "demonhunter": "Demon Hunter",
    "druid": "Druid", "evoker": "Evoker", "hunter": "Hunter",
    "mage": "Mage", "monk": "Monk", "paladin": "Paladin",
    "priest": "Priest", "rogue": "Rogue", "shaman": "Shaman",
    "warlock": "Warlock", "warrior": "Warrior",
}

SPEC_NAMES = {
    "beastmastery": "Beast Mastery",
}


def resolve_name(raw: str, mapping: dict) -> str:
    """Resolve a raw input name to its proper display form."""
    key = raw.lower().replace(" ", "").replace("_", "")
    if key in mapping:
        return mapping[key]
    # Title-case fallback
    return raw.replace("_", " ").title()


def main():
    parser = argparse.ArgumentParser(
        description="Compare SimC spell distributions vs WarcraftLogs real combat data",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Compare a SimC JSON output against WCL data
  python rotation_accuracy.py \\
      --simc-json dk_unholy.json \\
      --class DeathKnight --spec Unholy \\
      --wcl-client-id ABC123 --wcl-client-secret XYZ789

  # Use environment variables for WCL credentials
  export WCL_CLIENT_ID=ABC123
  export WCL_CLIENT_SECRET=XYZ789
  python rotation_accuracy.py --simc-json output.json --class Mage --spec Arcane

  # Output as JSON for piping
  python rotation_accuracy.py --simc-json output.json --class Mage --spec Arcane --json
        """,
    )
    parser.add_argument("--simc-json", required=True,
                        help="Path to SimC JSON2 output file")
    parser.add_argument("--class", dest="class_name", required=True,
                        help="WoW class name (e.g., DeathKnight, Mage)")
    parser.add_argument("--spec", required=True,
                        help="WoW spec name (e.g., Unholy, Arcane)")
    parser.add_argument("--wcl-client-id",
                        default=os.environ.get("WCL_CLIENT_ID", ""),
                        help="WCL API client ID (or set WCL_CLIENT_ID env var)")
    parser.add_argument("--wcl-client-secret",
                        default=os.environ.get("WCL_CLIENT_SECRET", ""),
                        help="WCL API client secret (or set WCL_CLIENT_SECRET env var)")
    parser.add_argument("--difficulty", type=int, default=4,
                        help="Raid difficulty (3=Normal, 4=Heroic, 5=Mythic)")
    parser.add_argument("--max-reports", type=int, default=5,
                        help="Max WCL reports to sample (default: 5)")
    parser.add_argument("--cache-dir",
                        help="Directory for caching WCL spell distributions (24h TTL)")
    parser.add_argument("--json", action="store_true",
                        help="Output results as JSON instead of human-readable")
    parser.add_argument("--wcl-json",
                        help="Path to pre-fetched WCL spell distribution JSON "
                             "(skip WCL API calls)")

    args = parser.parse_args()

    # ── Parse SimC JSON ──────────────────────────────────────────────
    if not os.path.exists(args.simc_json):
        print(f"Error: SimC JSON file not found: {args.simc_json}", file=sys.stderr)
        sys.exit(1)

    try:
        simc_spells = parse_simc_json(args.simc_json)
    except (json.JSONDecodeError, KeyError, ValueError) as e:
        print(f"Error: Failed to parse SimC JSON: {e}", file=sys.stderr)
        sys.exit(1)

    if not simc_spells:
        print("Error: No spell data found in SimC JSON output", file=sys.stderr)
        sys.exit(1)

    class_name = resolve_name(args.class_name, CLASS_NAMES)
    spec_name = resolve_name(args.spec, SPEC_NAMES)

    # ── Get WCL Data ─────────────────────────────────────────────────
    wcl_spells = None
    wcl_report_count = 0

    if args.wcl_json:
        # Use pre-fetched WCL data
        try:
            with open(args.wcl_json, "r") as f:
                wcl_spells = json.load(f)
                wcl_report_count = -1  # from file
        except (json.JSONDecodeError, OSError) as e:
            print(f"Error: Failed to load WCL JSON: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        # Fetch from WCL API
        if not args.wcl_client_id or not args.wcl_client_secret:
            print("Error: WCL credentials required. Provide --wcl-client-id/--wcl-client-secret "
                  "or set WCL_CLIENT_ID/WCL_CLIENT_SECRET env vars, "
                  "or use --wcl-json for pre-fetched data.", file=sys.stderr)
            sys.exit(1)

        try:
            wcl = WCLClient(args.wcl_client_id, args.wcl_client_secret)
        except Exception as e:
            print(f"Error: WCL authentication failed: {e}", file=sys.stderr)
            sys.exit(1)

        print(f"Fetching WCL data for {class_name} {spec_name}...", file=sys.stderr)
        encounters = wcl.get_current_raid_encounters()
        if not encounters:
            print("Error: Could not find current raid encounters", file=sys.stderr)
            sys.exit(1)

        encounter_ids = [e["id"] for e in encounters]
        print(f"  Raid: {encounters[0]['zone_name']} ({len(encounter_ids)} bosses)",
              file=sys.stderr)

        wcl_spells, wcl_report_count = wcl.get_spell_distribution(
            class_name, spec_name, encounter_ids,
            difficulty=args.difficulty,
            max_reports=args.max_reports,
            cache_dir=args.cache_dir,
        )

    if not wcl_spells:
        print("Error: No WCL spell distribution data available", file=sys.stderr)
        sys.exit(1)

    if wcl_report_count == -1:
        print(f"  Using cached/pre-fetched WCL data ({len(wcl_spells)} spells)",
              file=sys.stderr)
    else:
        print(f"  Sampled {wcl_report_count} WCL reports ({len(wcl_spells)} spells)",
              file=sys.stderr)

    # ── Compare ──────────────────────────────────────────────────────
    result = compare_distributions(simc_spells, wcl_spells)
    result["class"] = class_name
    result["spec"] = spec_name
    result["simc_spell_count"] = len(simc_spells)
    result["wcl_spell_count"] = len(wcl_spells)
    result["wcl_reports_sampled"] = wcl_report_count

    # ── Output ───────────────────────────────────────────────────────
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        _print_human_readable(result)


def _print_human_readable(result: dict):
    """Pretty-print comparison results."""
    print(f"\n{'='*60}")
    print(f"  Rotation Accuracy: {result['class']} {result['spec']}")
    print(f"{'='*60}")

    cos_sim = result.get("cosine_similarity")
    if cos_sim is not None:
        confidence = result.get("confidence", "unknown")
        conf_marker = {"high": "+", "medium": "~", "low": "-"}.get(confidence, "?")
        print(f"\n  Cosine Similarity:  {cos_sim:.4f}  [{conf_marker} {confidence}]")
        top10 = result.get("top_10_similarity")
        if top10 is not None:
            print(f"  Top-10 Similarity:  {top10:.4f}")
    else:
        print(f"\n  Cosine Similarity:  N/A ({result.get('confidence', 'unknown')})")

    matched = result.get("matched_spell_count", 0)
    simc_unique = result.get("simc_unique_count", 0)
    wcl_unique = result.get("wcl_unique_count", 0)
    print(f"\n  Matched spells:     {matched}")
    print(f"  SimC-only spells:   {simc_unique}")
    print(f"  WCL-only spells:    {wcl_unique}")

    # Spell comparison table
    comparison = result.get("spell_comparison", [])
    if comparison:
        print(f"\n  {'Spell':<30} {'SimC%':>7} {'WCL%':>7} {'Delta':>7}")
        print(f"  {'-'*30} {'-'*7} {'-'*7} {'-'*7}")
        for row in comparison:
            delta_str = f"{row['delta']:+.1f}" if row['delta'] != 0 else "0.0"
            print(f"  {row['spell']:<30} {row['simc_pct']:>6.1f}% {row['wcl_pct']:>6.1f}% {delta_str:>6}%")

    # Missing spells
    missing_simc = result.get("missing_in_simc", [])
    missing_wcl = result.get("missing_in_wcl", [])
    if missing_simc:
        print(f"\n  WCL spells missing from SimC: {', '.join(missing_simc[:5])}")
    if missing_wcl:
        print(f"  SimC spells missing from WCL: {', '.join(missing_wcl[:5])}")

    print()


if __name__ == "__main__":
    main()
