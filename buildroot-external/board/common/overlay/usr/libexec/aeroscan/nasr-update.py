#!/usr/bin/env python3
"""
Download and parse FAA NASR 28-day subscription data.

Extracts airband communication frequencies (118–137 MHz) for all US airports
and writes a compact CSV used by AeroScan's radio tuner preset system.

Output: /var/lib/aeroscan/apt_freq.csv
        /var/lib/aeroscan/nasr_edition.txt  (effective date of data)

Requires WiFi connectivity. Designed to run on the RPi4 AeroScan target,
but also works on a dev machine to pre-generate the file.

Usage:
    python3 nasr-update.py [--output-dir DIR]
"""

import argparse
import csv
import datetime
import json
import os
import re
import sys
import tempfile
import urllib.request
import urllib.error
import zipfile

# ── Constants ─────────────────────────────────────────────────────────────────

OUTPUT_DIR       = "/var/lib/aeroscan"
APT_FREQ_CSV     = "apt_freq.csv"
EDITION_FILE     = "nasr_edition.txt"
NASR_API_URL     = ("https://soa.smext.faa.gov/apra/nfdc/nasr/chart"
                    "?edition=current")
NASR_ZIP_PATTERN = ("https://aeronav.faa.gov/Upload_313-d/supplements/"
                    "NASR_Subscription_{date}.zip")

# 28-day cycle epoch: known NASR effective date
NASR_EPOCH = datetime.date(2025, 1, 23)
NASR_CYCLE_DAYS = 28

# Airband frequency bounds (MHz)
AIRBAND_MIN = 118.0
AIRBAND_MAX = 137.0

# Frequency types to include (FREQ_USE_CODE values in NASR)
USEFUL_TYPES = {
    "CTAF", "UNICOM", "TWR", "GND", "APP", "DEP",
    "ATIS", "AWOS", "ASOS", "TRACON", "MULTICOM",
    "CLNC DEL", "CLNC", "RAMP"
}

USER_AGENT = "AeroScan/1.0 (+https://github.com/your-org/aeroscan)"


# ── Cycle date computation ────────────────────────────────────────────────────

def current_nasr_date():
    """Return the effective date of the current NASR 28-day cycle."""
    today = datetime.date.today()
    days_since_epoch = (today - NASR_EPOCH).days
    if days_since_epoch < 0:
        return NASR_EPOCH
    cycle = days_since_epoch // NASR_CYCLE_DAYS
    return NASR_EPOCH + datetime.timedelta(days=cycle * NASR_CYCLE_DAYS)


def candidate_dates():
    """Current cycle date, then the previous two, as fallbacks."""
    base = current_nasr_date()
    return [
        base,
        base - datetime.timedelta(days=NASR_CYCLE_DAYS),
        base - datetime.timedelta(days=NASR_CYCLE_DAYS * 2),
    ]


# ── Download ──────────────────────────────────────────────────────────────────

def fetch_url_via_api():
    """Ask FAA API for the current edition URL. Returns URL string or None."""
    try:
        req = urllib.request.Request(NASR_API_URL,
                                     headers={"User-Agent": USER_AGENT})
        with urllib.request.urlopen(req, timeout=15) as resp:
            data = json.loads(resp.read())
        # Response is a list; each item may have 'url' or nested 'edition'
        for item in (data if isinstance(data, list) else [data]):
            url = item.get("url") or item.get("download_url")
            if url and "aeronav.faa.gov" in url and url.endswith(".zip"):
                return url
    except Exception as e:
        print(f"  API query failed: {e}", file=sys.stderr)
    return None


def resolve_download_url():
    """Return (url, edition_date_str) for the current NASR subscription."""
    # Try FAA API first
    url = fetch_url_via_api()
    if url:
        m = re.search(r'(\d{4}-\d{2}-\d{2})', url)
        date_str = m.group(1) if m else "unknown"
        return url, date_str

    # Fall back to computed cycle dates
    for d in candidate_dates():
        date_str = d.strftime("%Y-%m-%d")
        url = NASR_ZIP_PATTERN.format(date=date_str)
        try:
            req = urllib.request.Request(url, method="HEAD",
                                         headers={"User-Agent": USER_AGENT})
            with urllib.request.urlopen(req, timeout=10):
                pass
            return url, date_str
        except urllib.error.HTTPError as e:
            if e.code == 404:
                continue
            raise

    raise RuntimeError("Could not resolve current NASR edition URL.")


def download_zip(url, dest_path):
    """Stream-download url to dest_path, printing progress."""
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=60) as resp:
        total = int(resp.headers.get("Content-Length", 0))
        downloaded = 0
        with open(dest_path, "wb") as f:
            while True:
                chunk = resp.read(65536)
                if not chunk:
                    break
                f.write(chunk)
                downloaded += len(chunk)
                if total:
                    pct = downloaded / total * 100
                    print(f"\r  {downloaded/1e6:.1f}/{total/1e6:.1f} MB "
                          f"({pct:.0f}%)", end="", flush=True)
    print()


# ── Column detection ──────────────────────────────────────────────────────────

def find_col(header, candidates):
    """Return the first candidate found in header list, or None."""
    for c in candidates:
        if c in header:
            return c
    return None


def require_col(header, candidates, context):
    col = find_col(header, candidates)
    if col is None:
        raise RuntimeError(
            f"Cannot find {context} column in header.\n"
            f"  Tried: {candidates}\n"
            f"  Available: {header}"
        )
    return col


# ── Coordinate parsing ────────────────────────────────────────────────────────

_DMS_RE = re.compile(r'^(\d+)-(\d+)-([\d.]+)([NSns])$')
_DML_RE = re.compile(r'^(\d+)-(\d+)-([\d.]+)([EWew])$')


def parse_lat(val):
    val = val.strip()
    if not val:
        return None
    try:
        return float(val)
    except ValueError:
        m = _DMS_RE.match(val)
        if m:
            d, mn, s, h = m.groups()
            deg = int(d) + int(mn) / 60.0 + float(s) / 3600.0
            return deg if h.upper() == 'N' else -deg
    return None


def parse_lon(val):
    val = val.strip()
    if not val:
        return None
    try:
        f = float(val)
        return f
    except ValueError:
        m = _DML_RE.match(val)
        if m:
            d, mn, s, h = m.groups()
            deg = int(d) + int(mn) / 60.0 + float(s) / 3600.0
            return deg if h.upper() == 'E' else -deg
    return None


# ── Frequency parsing ─────────────────────────────────────────────────────────

def parse_freq_mhz(val):
    """Return frequency in MHz as float, or None. Handles MHz and kHz strings."""
    val = val.strip().rstrip('T').rstrip('B')  # strip TX/RX suffixes sometimes present
    if not val:
        return None
    try:
        f = float(val)
        # Frequencies > 1000 are in kHz
        if f > 1000:
            f /= 1000.0
        return f
    except ValueError:
        return None


# ── Parsing ───────────────────────────────────────────────────────────────────

def parse_airports(apt_base_path):
    """Return dict: site_no -> {ident, name, lat, lon}"""
    airports = {}
    with open(apt_base_path, newline='', encoding='utf-8', errors='replace') as f:
        reader = csv.DictReader(f)
        h = reader.fieldnames or []

        site_col  = require_col(h, ["SITE_NO"],                              "site number")
        ident_col = find_col(h,    ["ICAO_ID", "ICAO", "ARPT_ID", "FAA_IDENT"])
        lid_col   = find_col(h,    ["ARPT_ID", "FAA_IDENT", "FAA_LID", "IDENTIFIER"])
        name_col  = require_col(h, ["ARPT_NAME", "NAME", "AIRPORT_NAME"],    "airport name")
        lat_col   = require_col(h, ["LATITUDE_DECIMAL", "LAT_DECIMAL",
                                    "LATITUDE", "LAT",  "LAT_DEG_DEC"],      "latitude")
        lon_col   = require_col(h, ["LONGITUDE_DECIMAL", "LONG_DECIMAL",
                                    "LONGITUDE", "LON", "LON_DEG_DEC"],      "longitude")

        for row in reader:
            site = row[site_col].strip()
            if not site:
                continue
            lat = parse_lat(row[lat_col])
            lon = parse_lon(row[lon_col])
            if lat is None or lon is None:
                continue

            # Prefer ICAO 4-letter ID; fall back to FAA 3-letter LID
            ident = ""
            if ident_col:
                ident = row[ident_col].strip()
            if not ident and lid_col:
                ident = row[lid_col].strip()
            if not ident:
                continue

            airports[site] = {
                "ident": ident,
                "name":  row[name_col].strip(),
                "lat":   lat,
                "lon":   lon,
            }
    return airports


def parse_frequencies(apt_freq_path, airports):
    """Return list of dicts: ident, name, lat, lon, freq_mhz, freq_type."""
    records = []
    with open(apt_freq_path, newline='', encoding='utf-8', errors='replace') as f:
        reader = csv.DictReader(f)
        h = reader.fieldnames or []

        site_col  = require_col(h, ["SITE_NO"],                                  "site number")
        freq_col  = require_col(h, ["FREQ_NBR", "FREQ", "FREQUENCY", "FREQ_MHZ"],"frequency")
        type_col  = require_col(h, ["FREQ_USE_CODE", "FREQ_USE", "FREQ_TYPE",
                                    "USE_CODE", "FREQ_SRV_CD"],                  "frequency type")

        for row in reader:
            site = row[site_col].strip()
            apt = airports.get(site)
            if not apt:
                continue

            freq = parse_freq_mhz(row[freq_col])
            if freq is None or freq < AIRBAND_MIN or freq > AIRBAND_MAX:
                continue

            ftype = row[type_col].strip().upper()
            if not ftype:
                ftype = "COMM"

            records.append({
                "ident":    apt["ident"],
                "name":     apt["name"],
                "lat":      apt["lat"],
                "lon":      apt["lon"],
                "freq_mhz": f"{freq:.3f}",
                "freq_type": ftype,
            })

    return records


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--output-dir", default=OUTPUT_DIR,
                        help="Directory for output files (default: %(default)s)")
    args = parser.parse_args()

    out_dir = args.output_dir
    os.makedirs(out_dir, exist_ok=True)

    print("Resolving current NASR edition…")
    url, edition = resolve_download_url()
    print(f"  Edition: {edition}")
    print(f"  URL:     {url}")

    with tempfile.NamedTemporaryFile(suffix=".zip", delete=False) as tmp:
        tmp_path = tmp.name

    try:
        print(f"Downloading NASR subscription…")
        download_zip(url, tmp_path)

        print("Extracting airport data…")
        with zipfile.ZipFile(tmp_path, 'r') as zf:
            names = zf.namelist()
            # Find APT_BASE.csv and APT_FREQ.csv (may be in a subdirectory)
            def find_member(pattern):
                for n in names:
                    if re.search(pattern, n, re.IGNORECASE):
                        return n
                return None

            base_member = find_member(r'APT.?BASE\.csv$') or find_member(r'APT\.csv$')
            freq_member = find_member(r'APT.?FREQ\.csv$')

            if not base_member:
                raise RuntimeError(f"APT_BASE.csv not found in ZIP.\nContents: {names[:20]}")
            if not freq_member:
                raise RuntimeError(f"APT_FREQ.csv not found in ZIP.\nContents: {names[:20]}")

            print(f"  Airport base: {base_member}")
            print(f"  Frequencies:  {freq_member}")

            with tempfile.TemporaryDirectory() as extract_dir:
                zf.extract(base_member, extract_dir)
                zf.extract(freq_member, extract_dir)
                base_path = os.path.join(extract_dir, base_member)
                freq_path = os.path.join(extract_dir, freq_member)

                print("Parsing airports…")
                airports = parse_airports(base_path)
                print(f"  Loaded {len(airports)} airports")

                print("Parsing frequencies…")
                records = parse_frequencies(freq_path, airports)
                print(f"  Found {len(records)} airband frequencies")

        out_csv = os.path.join(out_dir, APT_FREQ_CSV)
        print(f"Writing {out_csv}…")
        with open(out_csv, 'w', newline='') as f:
            writer = csv.DictWriter(f,
                fieldnames=["ident", "lat", "lon", "freq_mhz", "freq_type", "name"])
            writer.writeheader()
            writer.writerows(records)

        edition_file = os.path.join(out_dir, EDITION_FILE)
        with open(edition_file, 'w') as f:
            f.write(edition + "\n")

        print(f"Done. {len(records)} records written.")
        print(f"Edition: {edition}")

    finally:
        os.unlink(tmp_path)


if __name__ == "__main__":
    main()
