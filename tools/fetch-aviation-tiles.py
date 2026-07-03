#!/usr/bin/env python3
"""
fetch-aviation-tiles.py — Download OpenAIP aviation overlay tiles for AeroScan.

OpenAIP tiles show airspace boundaries, airports, NAVAIDs, and restricted areas
on a transparent background. The AeroScan map scope composites them over its dark
instrument background (#111111), giving the monochrome aviation look.

SETUP
-----
1. Register for a free API key at https://www.openaip.net  (Account → API Keys)
2. Run:
     python3 tools/fetch-aviation-tiles.py --key YOUR_API_KEY --bbox LAT_MIN LON_MIN LAT_MAX LON_MAX
3. Copy output directory to device:
     rsync -avz ./maps/ root@aeroscan:/opt/winglet-gui/maps/
   or put on SD card:
     rsync -avz ./maps/ /media/sdcard/maps/

TILE LIMITS (free tier)
-----------------------
  50 000 tile requests / month. Zoom 7-11 over a US state is roughly 2 000-8 000 tiles.

USAGE EXAMPLES
--------------
  # Arizona + Nevada, zoom 7-11 (~3 000 tiles)
  python3 tools/fetch-aviation-tiles.py --key abc123 \\
      --bbox 31.0 -115.0 37.5 -109.0 --zooms 7-11

  # Full CONUS, zoom 7-9 only (~1 500 tiles — safe for monthly limit)
  python3 tools/fetch-aviation-tiles.py --key abc123 \\
      --bbox 24.0 -125.0 49.5 -66.0 --zooms 7-9

  # Quick test: single zoom level, small area
  python3 tools/fetch-aviation-tiles.py --key abc123 \\
      --bbox 33.0 -112.5 33.8 -111.5 --zooms 9

  # SD card output instead of local
  python3 tools/fetch-aviation-tiles.py --key abc123 \\
      --bbox 31.0 -115.0 37.5 -109.0 --zooms 7-11 --out /media/sdcard/maps
"""

import argparse
import math
import os
import subprocess
import sys
import time

TILE_URL = "https://api.tiles.openaip.net/api/data/openaip/{z}/{x}/{y}.png?apiKey={key}"

# OpenAIP rate limit: 2 req/s on the free tier is safe
REQUEST_DELAY = 0.55  # seconds between requests


def lat_lon_to_tile(lat: float, lon: float, zoom: int) -> tuple[int, int]:
    n = 2 ** zoom
    x = int((lon + 180.0) / 360.0 * n)
    lat_rad = math.radians(lat)
    y = int((1.0 - math.log(math.tan(lat_rad) + 1.0 / math.cos(lat_rad)) / math.pi) / 2.0 * n)
    return x, y


def tile_range(lat_min, lon_min, lat_max, lon_max, zoom):
    # Note: lat_max → smaller y tile index (north is up)
    x_min, y_max = lat_lon_to_tile(lat_min, lon_min, zoom)
    x_max, y_min = lat_lon_to_tile(lat_max, lon_max, zoom)
    return range(x_min, x_max + 1), range(y_min, y_max + 1)


def count_tiles(lat_min, lon_min, lat_max, lon_max, zooms):
    total = 0
    for z in zooms:
        xs, ys = tile_range(lat_min, lon_min, lat_max, lon_max, z)
        total += len(xs) * len(ys)
    return total


def parse_zooms(spec: str) -> list[int]:
    if "-" in spec:
        lo, hi = spec.split("-", 1)
        return list(range(int(lo), int(hi) + 1))
    return [int(spec)]


def fetch(key, lat_min, lon_min, lat_max, lon_max, zooms, out_dir, resume):
    total = count_tiles(lat_min, lon_min, lat_max, lon_max, zooms)
    print(f"Tiles to fetch: {total}  (zooms {zooms[0]}-{zooms[-1]})")
    print(f"Output: {os.path.abspath(out_dir)}")
    if total > 10_000:
        print(f"WARNING: {total} tiles will use {total/50000*100:.0f}% of your monthly free quota.")
    print()

    done = skipped = errors = 0
    t0 = time.time()

    for z in zooms:
        xs, ys = tile_range(lat_min, lon_min, lat_max, lon_max, z)
        for x in xs:
            for y in ys:
                dest = os.path.join(out_dir, str(z), str(x), f"{y}.png")
                if resume and os.path.exists(dest):
                    skipped += 1
                    continue

                os.makedirs(os.path.dirname(dest), exist_ok=True)
                url = TILE_URL.format(z=z, x=x, y=y, key=key)

                for attempt in range(3):
                    try:
                        result = subprocess.run(
                            [
                                "curl", "--silent", "--show-error", "--fail",
                                "--max-time", "15",
                                "--write-out", "%{http_code}",
                                "--output", dest,
                                "--user-agent", "AeroScan/1.0",
                                url,
                            ],
                            capture_output=True, text=True,
                        )
                        http_code = result.returncode == 0 and result.stdout.strip() or "0"
                        if result.returncode == 0:
                            done += 1
                            break
                        elif "22" in str(result.returncode) or "404" in result.stderr:
                            # curl exit 22 = HTTP 4xx; treat as missing tile (ocean/no coverage)
                            os.makedirs(os.path.dirname(dest), exist_ok=True)
                            with open(dest, "wb") as f:
                                # minimal 1×1 transparent PNG (68 bytes)
                                f.write(bytes([
                                    0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,
                                    0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
                                    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,
                                    0x08,0x06,0x00,0x00,0x00,0x1f,0x15,0xc4,
                                    0x89,0x00,0x00,0x00,0x0a,0x49,0x44,0x41,
                                    0x54,0x78,0x9c,0x62,0x00,0x00,0x00,0x02,
                                    0x00,0x01,0xe2,0x21,0xbc,0x33,0x00,0x00,
                                    0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,
                                    0x60,0x82,
                                ]))
                            done += 1
                            break
                        elif "429" in result.stderr:
                            print(f"\n  Rate limited — waiting 60s...")
                            time.sleep(60)
                        else:
                            print(f"\n  curl error z={z} x={x} y={y}: {result.stderr.strip()} (attempt {attempt+1})")
                            time.sleep(2)
                    except Exception as e:
                        print(f"\n  Error z={z} x={x} y={y}: {e} (attempt {attempt+1})")
                        time.sleep(2)
                else:
                    errors += 1

                time.sleep(REQUEST_DELAY)

                elapsed = time.time() - t0
                pct = (done + skipped) / total * 100 if total else 0
                rate = done / elapsed if elapsed > 0 else 0
                eta = (total - done - skipped) / rate if rate > 0 else 0
                print(f"\r  {pct:5.1f}%  {done+skipped}/{total}  "
                      f"{rate:.1f} tile/s  ETA {eta/60:.0f}m  errors={errors}   ",
                      end="", flush=True)

    print(f"\n\nDone. {done} downloaded, {skipped} skipped (resume), {errors} errors.")
    print(f"Total time: {(time.time()-t0)/60:.1f} min")


def main():
    ap = argparse.ArgumentParser(
        description="Download OpenAIP aviation overlay tiles for AeroScan map scope.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__.split("SETUP")[0].strip(),
    )
    ap.add_argument("--key",   required=True, help="OpenAIP API key (from openaip.net account)")
    ap.add_argument("--bbox",  required=True, nargs=4, type=float,
                    metavar=("LAT_MIN","LON_MIN","LAT_MAX","LON_MAX"),
                    help="Bounding box in decimal degrees")
    ap.add_argument("--zooms", default="7-11",
                    help="Zoom range, e.g. '7-11' or single level '9' (default: 7-11)")
    ap.add_argument("--out",   default="./maps",
                    help="Output directory (default: ./maps)")
    ap.add_argument("--resume", action="store_true",
                    help="Skip tiles that already exist on disk")
    ap.add_argument("--dry-run", action="store_true",
                    help="Print tile count and exit without downloading")
    args = ap.parse_args()

    lat_min, lon_min, lat_max, lon_max = args.bbox
    if lat_min > lat_max or lon_min > lon_max:
        ap.error("bbox: LAT_MIN must be < LAT_MAX and LON_MIN < LON_MAX")

    zooms = parse_zooms(args.zooms)
    if not all(7 <= z <= 13 for z in zooms):
        ap.error("AeroScan map scope supports zoom levels 7-13 only")

    if args.dry_run:
        total = count_tiles(lat_min, lon_min, lat_max, lon_max, zooms)
        print(f"Dry run: {total} tiles across zooms {zooms}")
        return

    fetch(args.key, lat_min, lon_min, lat_max, lon_max, zooms, args.out, args.resume)


if __name__ == "__main__":
    main()
