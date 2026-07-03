# AeroScan Design System

A design system for **AeroScan** — a handheld aircraft-and-drone scanner built by
**Parhelia Technology**. AeroScan is a port/extension of the avBadge 2024
("Winglet") project onto Raspberry Pi hardware: an embedded Linux instrument with an
800×480 touchscreen that tracks aircraft over ADS-B (1090 MHz / dump1090), receives a
software radio (airband AM, FM, 2m amateur), holds a GPS fix, and — in later phases —
detects nearby drones via Remote ID. It is part scientific instrument, part DEF CON
electronic badge: a dark radar-scope aesthetic with sci-fi display type and a playful
hacker-aerospace personality.

This system captures **two layers**:
1. **Parhelia Technology brand** — the amber "parhelion" sundog (an arcing sun-halo with a
   bright spark) over a graphite serif wordmark; the brand amber is the instrument's hero amber.
2. **The AeroScan instrument theme** — the dark scope palette the device actually renders
   in (black chassis, amber readouts, neon-green radar), lifted verbatim from the firmware.

## Sources
- **Codebase (read-only, mounted):** `AeroScan/` — a Qt5/C++ embedded app + Buildroot tree.
  - GUI app: `AeroScan/aeroscan-gui/` (Qt5 widgets).
  - Theme of record: `AeroScan/aeroscan-gui/winglet-ui/theme.cpp` — QPalette roles, scope
    colors, the three font families, 150ms animation duration.
  - Screens: `winglet-ui/window/` (radarscope, flightboard, mapscope, compass, clock,
    gpstracker, oscope, droneboard, settingsmenu, radiotuner per the plan).
  - Widgets/cores: `winglet-ui/widget/` + `winglet-ui/windowcore/` (statusbar, menuitem,
    scrollablemenu, messagebox, mainmenu, circularkeyboard).
  - Assets: `winglet-ui/assets/` (fonts, icons, logos, mascot art, map style JSON).
  - Product plan: `AeroScan/DEVELOPMENT_PLAN.md` (hardware, phases, screen specs).
- No Figma or slide decks were provided.

All real fonts, icons, logos and mascot art used here were copied out of that codebase into
`assets/`. Fonts are the originals (Lato, Neuropol X, Sofachrome) — no substitutions needed.

---

## CONTENT FUNDAMENTALS

The voice is **terse instrument labelling**, not marketing copy. Think cockpit panel and
ham-radio gear, with an undercurrent of hacker-badge humor.

- **Casing & length.** Screen titles and menu entries are **Title Case, two words max**:
  "Radar Scope", "Flight List", "Map Scope", "Radio Tuner", "GPS Tracker", "Flight List".
  Status and chrome labels are terse and often UPPERCASE ("ADS-B 1090", "Total Aircraft: 14",
  "Zoom Level: 9", "No Time").
- **Data-first.** Most on-screen text is a value, not a sentence: `121.500 MHz`, `34000`,
  `4.2 nm`, `−52 dBm`, `Total Aircraft: 14`. Units are lowercase and spelled the radio/aviation
  way (`nm`, `MHz`, `kHz`, `dBm`). Numbers are never decorated.
- **Person.** Imperative and system-voice. The device addresses the operator directly and
  briefly in dialogs ("Controls are what you'd expect…", "To exit emulator, press both
  knob and power buttons."). No "we", rare "you", never first-person product voice.
- **Dialogs** are two parts: a **Neuropol X title** (the action — "Starting Emulator",
  "Power Off") and a short **Lato body** sentence, then text buttons ("Okay", "Cancel").
- **Empty & error states** are blunt: "No drones detected", "No Time", "No Map".
- **Humor lives in the easter eggs, not the chrome.** Menu has a "GB Emulator", a Starman
  roadster on the oscilloscope, a tuxedo-wearing "FrogFly" mascot on the splash. The
  instrument screens themselves stay deadpan and functional.
- **No emoji.** The device predates and disdains them. Iconography is the device's own
  monochrome glyph set; status is shown with shape + color, never emoji.
- **Vibe:** precise, low-ceremony, slightly retro-military radar. If a word can be cut, cut it.

---

## VISUAL FOUNDATIONS

### Color
Two palettes. The **brand** is solar amber `#f8af1c` + graphite `#414143` (the parhelion
logo) — the amber is the same hero amber the instrument runs on, so brand and product share
one accent. Cyan `#00b0f0` survives as a secondary data accent (ADS-B chrome). The
**instrument** is a dark scope theme taken straight from `theme.cpp` (dark mode is default):

- **Surfaces:** Window `#111111`, true-black Shadow `#000000` (bezel/vignette), and a single
  charcoal-slate `#36454f` for panels and table headers. There is essentially **one surface
  color** — depth comes from glow and black, not from many greys.
- **Text & accent:** amber `#ffac11` is the signature — every screen title and primary
  readout. Teal `#0a7985` is selection/secondary. Muted grey `#999999` is body and data.
  Magenta `#db1675` is the committed-action / link color (a button the instant it fires).
- **Scope signals:** neon green `#39ff14` (radar sweep + rings), soft green `#8bf688`,
  light-grey cursor `#dcdcdc`, oscilloscope yellow `#ffff00`.
- **Status semantics:** green `#22cc44` lock/OK, red `#dd2222` alert/no-lock, amber warn.
- **Identity color:** aircraft callsigns are tinted by carrier (SWA orange, DAL/UAL blue,
  AAL red, UPS brown, FDX purple, unknown grey); drones by detection method (BLE red,
  WiFi orange). Color carries meaning everywhere — almost nothing is decorative.

There is also a light instrument mode in the firmware (`#eeeeee` window, darker amber/teal,
dark-green sweep) kept as `--*-light` reference tokens, but **dark is the default and the
star of the system.**

### Type
Three families, all shipped from the device (`tokens/fonts.css`):
- **Neuropol X** (titleFont) — the techno display face and the AeroScan voice. Screen titles
  (22–26px), labels, all UI chrome. Set with slightly open tracking (0.02–0.12em), often
  uppercase for small labels. Never tight.
- **Sofachrome** (fancyFont) — chrome/aero display face. **Reserved** for the big wordmark and
  splash/credits lockups. Don't use it for UI.
- **Lato** (standardFont) — neutral humanist sans for everything readable: dialog body (14px),
  telemetry/data (12px), table cells (≈11px / firmware 9pt). Regular + Bold only.

The panel is a literal 480px wide, so the type scale is in **px, not rem** — sizes are
device-native. Smallest readable text is ~9px (ring labels); body is 14px.

### Space, shape, elevation
- **Geometry is literal-pixel.** 800×480 framebuffer = a centered 480×480 round-cornered
  content area + two 160px side columns. Circular screens (radar, compass, oscilloscope,
  GPS tracker) center at (240,240) with radius ~240. Spacing follows a 4px base ramp.
- **Corners are mostly hard.** This is an instrument. Small chips/inputs round to 3–6px;
  capsules and toggles go pill; only the outer **device bezel** (≈28px) and screen glass
  (≈18px) round meaningfully. Avoid the web-app "rounded card with a colored left border"
  trope — the device uses a left **inset rule** (2px amber/teal bar) to mark selection, not
  a rounded outline.
- **Elevation is glow, not soft grey shadow.** Active elements emit a colored halo
  (`--glow-amber`, `--glow-green`, `--glow-cyan`). Panels use a 1px inset hairline. The only
  true drop shadow in the system is the device bezel against its backdrop. Screens carry a
  black **vignette** (radial darkening to the corners) to read as glass under a bezel.
- **Backgrounds** are flat black or charcoal — **no gradients, no photos, no textures** inside
  the UI. The one "image" treatment is a faintly ghosted Parhelia sundog mark behind list
  screens (~10–16% opacity, masked). Map Scope is the exception: it renders OpenMapTiles in a
  custom near-black "Dark Matter" style (`assets/style.json`) — black land, `#1b1b1d` water,
  grey hairline roads.

### Motion & states
- **One duration: 150ms** (`theme.cpp` animationDuration) for every menu/dialog transition.
  The signature easing is **OutQuart** (`cubic-bezier(0.165,0.84,0.44,1)`) — used for the
  selection pointer glide. Fades are opacity 0→1. The radar sweep ticks every ~50ms.
- **No bounce, no spring, no decorative loops.** The only perpetual motion is the radar sweep
  and the oscilloscope trace — both functional.
- **Hover/focus** (selection): the focused row brightens to full opacity, shifts amber, grows
  slightly, gains a left inset rule and the "›" caret. Unfocused rows dim to ~0.6–0.7 and
  shrink (the arc-menu falloff).
- **Press/commit:** the firing element flips to **magenta** (`Link` role) and underlines —
  this is the "selected & committed" state, distinct from mere focus.
- **Transparency & blur** are used sparingly: selection backgrounds are ~6–8% amber washes;
  table bodies are transparent over the screen; the vignette is a black radial. No glass-morphism.

---

## ICONOGRAPHY

The device ships its **own monochrome PNG glyph set** (40×40, white/black silhouettes on
transparency) under `assets/icons/`, organized by subsystem:
`adsb/`, `batt/`, `wifi/`, `location/` (GPS), `settings/` (toggle, checkbox, secured-wifi).

- **Recolored at runtime.** The firmware tints each glyph by compositing a palette color over
  the silhouette (`loadMonochromeIcon` / `loadColoredIcon`, SourceIn). We reproduce this in
  HTML/CSS with `mask-image` + a `background` color — see `components/feedback/StatusBar.jsx`
  and `guidelines/brand-iconography.card.html`. GPS greens on lock, reds on no-lock; the
  rest tint to muted chrome; charging/active glyphs glow.
- **Stateful, not decorative.** Icons are battery bars (0–6 + full, charge/discharge), wifi
  signal bars (0–4, connecting, disconnected, off, plus open/secured variants), GPS lock
  states, and ADS-B on/off. They communicate live system state in the 160px status rail.
- **No icon font, no emoji, no unicode-as-icon** in the chrome. (Quadrant nav buttons in this
  kit use a couple of geometric arrows ▲▼●◄ as affordance labels only.)
- **Vector marks:** a few SVGs exist (`assets/icons/airplane.svg` for radar plane icons,
  `CompasPath.svg`). Logos and mascots are PNG. **Never hand-draw replacement icons** — copy
  the device's own glyphs (they're all in `assets/`).
- **Brand & mascots:** the Parhelia sundog lockup + spark mark (`assets/brand/parhelia-*.png`,
  with a dark-theme relit lockup for black screens), the **Canard** fighter-
  pilot helmet (Radio Tuner mark), the tuxedoed **FrogFly** splash mascot, and the green
  wireframe **Starman roadster** oscilloscope easter egg.

---

## Index / manifest

**Root**
- `styles.css` — the single global entry point (consumers link this). `@import`s tokens only.
- `readme.md` — this guide.
- `SKILL.md` — Agent-Skills-compatible entry for Claude Code.

**Tokens** (`tokens/`, all `@import`ed by `styles.css`)
- `fonts.css` — `@font-face` for Lato, Neuropol X, Sofachrome.
- `colors.css` — brand + instrument palette, status, identity, semantic aliases.
- `typography.css` — families, device-native px scale, tracking.
- `spacing.css` — 4px ramp, device geometry, radius, glow/bezel elevation, motion.

**Components** (`components/`, React primitives → `window.AeroScanDesignSystem_2d719c`)
- `core/` — `Button`, `MenuItem`, `Badge`, `Panel`
- `forms/` — `Toggle`, `Checkbox`
- `data/` — `DataTable`
- `feedback/` — `StatusBar`, `MessageBox`

Each component directory has `<Name>.jsx`, `<Name>.d.ts`, `<Name>.prompt.md`, and a
`@dsCard`-tagged card HTML.

**Foundation cards** (`guidelines/`) — specimen cards for the Design System tab:
colors (brand, surfaces, text, scope, status, airlines, drones), type (display, title,
body, scale), spacing (scale, radius/elevation, device geometry), brand (logo, mascots,
iconography).

**UI kits** (`ui_kits/`)
- `aeroscan-device/` — the full instrument: arc main menu, Radar Scope, Flight List,
  Radio Tuner, Settings. See its `README.md`.

**Assets** (`assets/`)
- `fonts/` — the three TTFs.
- `brand/` — Parhelia logo (light + dark lockups), sundog spark mark + arc, plus legacy
  AV globe, Canard helmet, FrogFly, Starman roadster, dev ring.
- `icons/` — device glyph set (adsb, batt, wifi, location, settings) + airplane/compass SVGs.
