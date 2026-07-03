# Parhelia Logo — Qt Integration Patch

Drops the Parhelia Technology sundog mark into the AeroScan firmware (Qt5/C++) in place of
the old Aerospace Village globe watermark. ~5 minutes, one resource edit + one code line.

## What's in this folder
| File | What it is | Where it goes |
|---|---|---|
| `parhelia_watermark.png` | 194×256 · amber sundog, **pre-dimmed ~24%** | `aeroscan-gui/assets/` |
| `parhelia_mark.png` | 194×256 · amber sundog, **full opacity** | `aeroscan-gui/assets/` (for tint variants / other UI) |
| `parhelia_logo.png` | 440×218 · full dark-theme lockup | `aeroscan-gui/assets/` (optional splash/credits) |
| `resources.qrc.snippet` | the 3 lines to add to `resources.qrc` | reference |
| `theme.cpp.patch` | the one-line code change (+ variants) | reference |

All three PNGs are transparent and already match the design-system amber (`#f8af1c`).

---

## Step 1 — Copy the assets
Copy the three PNGs into the Qt assets folder:

```bash
cp parhelia_watermark.png parhelia_mark.png parhelia_logo.png \
   /path/to/AeroScan/aeroscan-gui/assets/
```

## Step 2 — Register them in resources.qrc
Open `aeroscan-gui/resources.qrc`. Inside the existing `<qresource prefix="/images">`
block (the one that already lists `av_logo.png`), add the three lines from
`resources.qrc.snippet`:

```xml
        <file alias="parhelia_watermark.png">assets/parhelia_watermark.png</file>
        <file alias="parhelia_mark.png">assets/parhelia_mark.png</file>
        <file alias="parhelia_logo.png">assets/parhelia_logo.png</file>
```

Qt's `rcc` compiles these into the binary at build time, so they ship inside the app — no
runtime file paths needed. They're then reachable as `:/images/parhelia_watermark.png`.

## Step 3 — Point the watermark at the new mark
Open `aeroscan-gui/winglet-ui/theme.cpp`, find `WingletTheme::setColorModePalette(...)`, and
replace the av_logo load line:

```cpp
// before
loadMonochromeIcon(&avLogo, ":/images/av_logo.png", QPalette::Shadow);

// after
avLogo.load(":/images/parhelia_watermark.png");
```

That's it — `avLogo` is the same QPixmap the existing `renderBgAvLogo()` already draws on the
main menu / flight board, centered at (300, 245). The label auto-sizes to the new pixmap.

> Want a near-black subtle watermark (original behavior) or a brighter amber instead? See the
> two variants at the bottom of `theme.cpp.patch`.

## Step 4 — Rebuild
**Desktop / dev build:**
```bash
cd /path/to/AeroScan/aeroscan-gui
qmake aeroscan-gui.pro
make -j$(nproc)
```

**On-device (Buildroot SD image):** rebuild the `aeroscan-gui` package and regenerate the
image, per `DEVELOPMENT_PLAN.md` Phase 14:
```bash
make aeroscan-gui-dirclean   # force a clean rebuild of just this package
make                          # rebuild + repack the SD image
```
(`rcc` re-runs automatically because `resources.qrc` changed.)

## Step 5 — Verify
Launch the app (or flash the image). The main menu and flight list should show the **Parhelia
sundog** behind the menu instead of the globe. If you don't see it, confirm the `<file>`
aliases match the `:/images/...` path exactly and that the PNGs landed in `aeroscan-gui/assets/`.

---

## Optional — splash / credits lockup
`parhelia_logo.png` is the full "PARHELIA TECHNOLOGY" lockup sized for the 480px screen. To use
it (e.g. on the Credits screen `winglet-ui/window/credits.*` or a boot splash):

```cpp
QLabel *brand = new QLabel(this);
brand->setPixmap(QPixmap(":/images/parhelia_logo.png"));
brand->setFixedSize(brand->sizeHint());
moveCenter(brand, 240, 120);   // 240 = content-area center X
```

## Notes
- Colors and fonts already match the firmware (the design-system tokens were derived from
  `theme.cpp`), so no `QPalette`/`QFont` changes are needed for the rebrand — only the logo.
- The old `av_logo.png` can stay in the tree; nothing references it after Step 3.
- These are raster PNGs extracted from the supplied logo. If you have the original vector
  (SVG/AI), regenerate crisper assets from that and re-export at the same sizes.
