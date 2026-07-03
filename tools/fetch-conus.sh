#!/usr/bin/env bash
# Downloads OpenAIP aviation overlay tiles for CONUS, zoom 7-10.
# ~21 000 tiles, ~210 MB, completes in one API free-tier month.
# Output: ./maps/{z}/{x}/{y}.png  (rsync to device when done)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Key comes from the OPENAIP_KEY env var or the .openaip-key file at the
# project root (free key: https://www.openaip.net → Account → API Keys).
API_KEY="${OPENAIP_KEY:-$(cat "$SCRIPT_DIR/../.openaip-key" 2>/dev/null || true)}"
if [ -z "$API_KEY" ]; then
    echo "ERROR: no OpenAIP API key — set OPENAIP_KEY or create .openaip-key at the project root" >&2
    exit 1
fi
OUT="${1:-"$SCRIPT_DIR/../maps"}"

echo "Output directory: $OUT"
echo

python3 "$SCRIPT_DIR/fetch-aviation-tiles.py" \
    --key  "$API_KEY"         \
    --bbox 24.0 -125.0 49.5 -66.0 \
    --zooms 7-10              \
    --out  "$OUT"             \
    --resume
