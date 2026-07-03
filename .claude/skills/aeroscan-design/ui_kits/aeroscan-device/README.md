# AeroScan Device — UI Kit

A high-fidelity, click-through recreation of the AeroScan badge: an 800×480 instrument with
a centered 480×480 round content screen flanked by two 160px columns (brand + four-quadrant
touch nav on the left, status rail on the right). It composes the design-system primitives —
it does not re-implement them.

## Run
Open `index.html`. The device scales to fit the viewport.

## Navigation (mirrors the badge's four touch quadrants)
- **Left column buttons** — ▲ Up · ● Sel · ▼ Dn · ◄ Back
- **Keyboard** — Arrow Up/Down, Enter/Space = Select, Esc/Backspace = Back

## Screens
| File | Screen | Notes |
|---|---|---|
| `screens/MainMenuScreen.jsx` | Arc main menu | Centered focus list, ghosted AV globe, "›" caret. Map Scope & Power open a demo MessageBox. |
| `screens/RadarScreen.jsx` | Radar Scope | Canvas: range rings, rotating green sweep + trail, carrier-tinted aircraft chevrons, zoom presets (50/25/10 nm). |
| `screens/FlightScreen.jsx` | Flight List | `DataTable` of live aircraft, carrier-tinted callsigns, running total. |
| `screens/RadioScreen.jsx` | Radio Tuner | Band selector (AM/FM/2m), large frequency readout, preset list from frequencies.json. |
| `screens/SettingsScreen.jsx` | Settings | `MenuItem` rows with `Toggle`/`Checkbox` controls. |

`app.jsx` is the device shell: column chrome, the nav bus, screen routing, and the live clock.
Active screen persists to `localStorage` (`aeroscan.screen`).

## Components used
`StatusBar`, `DataTable`, `MenuItem`, `Toggle`, `Checkbox`, `Button`, `Badge`, `MessageBox`
from `window.AeroScanDesignSystem_2d719c` (loaded via `../../_ds_bundle.js`).

## Fidelity notes
- Traffic, GPS lock, and radio state are simulated for the demo. The real badge sources ADS-B
  from dump1090 (TCP 30003), GPS from gpsd, and radio audio from rtl_fm.
- Map Scope, Compass, Oscilloscope, GPS Tracker and DroneBoard exist on the device but are
  out of scope for this kit.
