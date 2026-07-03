#!/usr/bin/env python3
"""
Download label-free Stamen Toner Background tiles from Stadia Maps.

Fetches the exact same {z}/{x}/{y} coordinates present in the existing tile set
and replaces them in-place. Safe to interrupt and re-run — already-downloaded
tiles are skipped unless --force is passed.

Usage:
    python3 download-tiles.py --api-key YOUR_STADIA_KEY [--force] [--dry-run]

Get a free API key at: https://stadiamaps.com/
"""

import argparse
import os
import sys
import time
import urllib.request
import urllib.error
import glob

TILE_DIR = os.path.join(
    os.path.dirname(__file__),
    "../output/rpi2/target/opt/winglet-gui/maps"
)
TILE_URL = "https://tiles.stadiamaps.com/tiles/stamen_toner_background/{z}/{x}/{y}.png"
ZOOM_LEVELS = [7, 8, 9, 10]

# Stadia free tier: 200k tiles/month, ~2 req/s sustained is safe
REQUEST_DELAY_S = 0.55
MAX_RETRIES = 3
RETRY_DELAY_S = 5.0

USER_AGENT = "AeroScan-tile-downloader/1.0 (+https://github.com/your-org/aeroscan)"


def collect_tiles(tile_dir, zoom_levels):
    """Return list of (z, x, y) tuples matching existing tile files."""
    tiles = []
    for z in zoom_levels:
        for path in glob.glob(f"{tile_dir}/{z}/*/*.png"):
            parts = path.split(os.sep)
            x = int(parts[-2])
            y = int(parts[-1].replace(".png", ""))
            tiles.append((z, x, y))
    tiles.sort()
    return tiles


def download_tile(z, x, y, api_key, out_path, dry_run):
    url = TILE_URL.format(z=z, x=x, y=y) + f"?api_key={api_key}"
    if dry_run:
        print(f"  [dry-run] {z}/{x}/{y}")
        return True

    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    for attempt in range(1, MAX_RETRIES + 1):
        try:
            with urllib.request.urlopen(req, timeout=15) as resp:
                data = resp.read()
            # Verify it looks like a PNG
            if not data.startswith(b"\x89PNG"):
                print(f"  WARN: {z}/{x}/{y} — unexpected content, skipping")
                return False
            os.makedirs(os.path.dirname(out_path), exist_ok=True)
            with open(out_path, "wb") as f:
                f.write(data)
            return True
        except urllib.error.HTTPError as e:
            if e.code == 401:
                print("\nERROR: 401 Unauthorized — check your API key.", file=sys.stderr)
                sys.exit(1)
            if e.code == 429:
                print(f"  rate-limited on {z}/{x}/{y}, waiting {RETRY_DELAY_S}s…")
                time.sleep(RETRY_DELAY_S)
            else:
                print(f"  HTTP {e.code} on {z}/{x}/{y} (attempt {attempt}/{MAX_RETRIES})")
                time.sleep(RETRY_DELAY_S)
        except Exception as e:
            print(f"  error on {z}/{x}/{y} attempt {attempt}: {e}")
            time.sleep(RETRY_DELAY_S)
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--api-key", required=True, help="Stadia Maps API key")
    parser.add_argument("--tile-dir", default=TILE_DIR,
                        help="Root directory of the tile set (default: %(default)s)")
    parser.add_argument("--force", action="store_true",
                        help="Re-download tiles that already exist")
    parser.add_argument("--dry-run", action="store_true",
                        help="List tiles without downloading")
    args = parser.parse_args()

    tile_dir = os.path.realpath(args.tile_dir)
    if not os.path.isdir(tile_dir):
        print(f"ERROR: tile directory not found: {tile_dir}", file=sys.stderr)
        sys.exit(1)

    tiles = collect_tiles(tile_dir, ZOOM_LEVELS)
    total = len(tiles)
    print(f"Found {total} tiles across zoom levels {ZOOM_LEVELS}")
    if args.dry_run:
        print("Dry-run mode — no files written.")

    skipped = downloaded = failed = 0
    for i, (z, x, y) in enumerate(tiles, 1):
        out_path = os.path.join(tile_dir, str(z), str(x), f"{y}.png")

        if not args.force and os.path.exists(out_path):
            skipped += 1
            continue

        pct = i / total * 100
        print(f"[{i}/{total} {pct:.1f}%] {z}/{x}/{y}", end="  ", flush=True)
        ok = download_tile(z, x, y, args.api_key, out_path, args.dry_run)
        if ok:
            downloaded += 1
            print("ok")
        else:
            failed += 1
            print("FAILED")

        if not args.dry_run:
            time.sleep(REQUEST_DELAY_S)

    print(f"\nDone. downloaded={downloaded} skipped={skipped} failed={failed}")
    if failed:
        print(f"Re-run without --force to retry only the {failed} failed tiles.")


if __name__ == "__main__":
    main()
