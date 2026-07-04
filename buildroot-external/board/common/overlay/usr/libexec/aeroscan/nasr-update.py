#!/usr/bin/env python3
"""
Download and parse FAA NASR 28-day subscription data.

Extracts airband communication frequencies (118–137 MHz) for all US airports
and writes a compact CSV used by AeroScan's radio tuner preset system.

Downloads the CSV-only extract (~22 MB) rather than the full subscription
ZIP (~250 MB, whose CSVs are buried in a nested CSV_Data/*.zip anyway).
Frequencies come from FRQ.csv, which carries the serviced facility ident,
name, decimal coordinates, frequency, and use code in a single file — no
join against APT_BASE.csv is needed.

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
import io
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

# APRA edition API (the old soa.smext.faa.gov host no longer resolves)
NASR_API_URL     = ("https://external-api.faa.gov/apra/nfdc/nasr/chart"
                    "?edition=current")

# CSV-only extract; date is zero-padded DD_Mon_YYYY, e.g. 11_Jun_2026
NASR_CSV_ZIP_PATTERN = ("https://nfdc.faa.gov/webContent/28DaySub/extra/"
                        "{date}_CSV.zip")

# 28-day cycle epoch: known NASR effective date
NASR_EPOCH = datetime.date(2025, 1, 23)
NASR_CYCLE_DAYS = 28

# Airband frequency bounds (MHz)
AIRBAND_MIN = 118.0
AIRBAND_MAX = 137.0

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


def csv_zip_url(d):
    return NASR_CSV_ZIP_PATTERN.format(date=d.strftime("%d_%b_%Y"))


# ── Download ──────────────────────────────────────────────────────────────────

def fetch_edition_date_via_api():
    """Ask the FAA APRA API for the current edition date, or None."""
    try:
        req = urllib.request.Request(NASR_API_URL, headers={
            "User-Agent": USER_AGENT,
            "Accept": "application/json",
        })
        with urllib.request.urlopen(req, timeout=15) as resp:
            data = json.loads(resp.read())
        # {"status": {...}, "edition": [{"editionDate": "06/11/2026",
        #   "product": {"url": ".../28DaySubscription_Effective_2026-06-11.zip"}}]}
        editions = data.get("edition", [])
        for item in (editions if isinstance(editions, list) else [editions]):
            url = (item.get("product") or {}).get("url", "")
            m = re.search(r'(\d{4})-(\d{2})-(\d{2})', url)
            if m:
                return datetime.date(*map(int, m.groups()))
            m = re.match(r'(\d{2})/(\d{2})/(\d{4})', item.get("editionDate", ""))
            if m:
                mm, dd, yyyy = map(int, m.groups())
                return datetime.date(yyyy, mm, dd)
    except Exception as e:
        print(f"  API query failed: {e}", file=sys.stderr)
    return None


def url_exists(url):
    """Probe url with a 1-byte ranged GET (nfdc.faa.gov rejects HEAD with 503)."""
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT,
                                               "Range": "bytes=0-0"})
    try:
        with urllib.request.urlopen(req, timeout=10):
            return True
    except urllib.error.HTTPError as e:
        if e.code in (403, 404):
            return False
        raise


def resolve_download_url():
    """Return (url, edition_date_str) for the current NASR CSV extract."""
    dates = []
    api_date = fetch_edition_date_via_api()
    if api_date:
        dates.append(api_date)
    dates += [d for d in candidate_dates() if d not in dates]

    for d in dates:
        url = csv_zip_url(d)
        if url_exists(url):
            return url, d.strftime("%Y-%m-%d")

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


# ── Parsing ───────────────────────────────────────────────────────────────────

def to_float(val):
    try:
        return float(val.strip())
    except (ValueError, AttributeError):
        return None


def parse_freq_mhz(val):
    """Return frequency in MHz as float, or None. Handles MHz and kHz strings."""
    val = val.strip().rstrip('T').rstrip('B')  # strip TX/RX suffixes sometimes present
    if not val:
        return None
    f = to_float(val)
    if f is None:
        return None
    # Frequencies > 1000 are in kHz
    if f > 1000:
        f /= 1000.0
    return f


def parse_frequencies(frq_file):
    """Parse FRQ.csv (text file object).

    Return list of dicts: ident, name, lat, lon, freq_mhz, freq_type.
    """
    reader = csv.DictReader(frq_file)
    h = reader.fieldnames or []
    for col in ("FACILITY", "LAT_DECIMAL", "LONG_DECIMAL", "FREQ", "FREQ_USE"):
        if col not in h:
            raise RuntimeError(f"Cannot find {col} column in FRQ.csv header.\n"
                               f"  Available: {h}")

    records = []
    seen = set()
    for row in reader:
        # Prefer the airport being serviced; fall back to the servicing facility
        ident = (row.get("SERVICED_FACILITY") or "").strip() \
                or row["FACILITY"].strip()
        if not ident:
            continue

        lat = to_float(row["LAT_DECIMAL"])
        lon = to_float(row["LONG_DECIMAL"])
        if lat is None or lon is None:
            continue

        freq = parse_freq_mhz(row["FREQ"])
        if freq is None or freq < AIRBAND_MIN or freq > AIRBAND_MAX:
            continue

        ftype = row["FREQ_USE"].strip().upper() or "COMM"
        # AWOS rows repeat the facility ident in FREQ_USE ("00U AWOS-3")
        if ftype.startswith(ident + " "):
            ftype = ftype[len(ident) + 1:].strip() or "COMM"
        name = (row.get("SERVICED_FAC_NAME") or "").strip() \
               or (row.get("FAC_NAME") or "").strip()

        key = (ident, f"{freq:.3f}", ftype)
        if key in seen:
            continue
        seen.add(key)

        # The GUI reader splits lines on bare commas and does not unquote,
        # so keep commas and double quotes out of the text fields.
        records.append({
            "ident":    ident.replace(",", " "),
            "name":     name.replace(",", " ").replace('"', "'"),
            "lat":      lat,
            "lon":      lon,
            "freq_mhz": f"{freq:.3f}",
            "freq_type": ftype.replace(",", " ").replace('"', "'"),
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
        print("Downloading NASR CSV data…")
        download_zip(url, tmp_path)

        print("Parsing frequencies…")
        with zipfile.ZipFile(tmp_path, 'r') as zf:
            frq_member = next((n for n in zf.namelist()
                               if re.fullmatch(r'FRQ\.csv', n, re.IGNORECASE)), None)
            if not frq_member:
                raise RuntimeError(
                    f"FRQ.csv not found in ZIP.\nContents: {zf.namelist()[:20]}")

            with zf.open(frq_member) as raw:
                frq_file = io.TextIOWrapper(raw, encoding='utf-8', errors='replace',
                                            newline='')
                records = parse_frequencies(frq_file)
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
