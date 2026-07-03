---
name: aeroscan-design
description: Use this skill to generate well-branded interfaces and assets for AeroScan (the Parhelia Technology aircraft & drone scanner badge), either for production or throwaway prototypes/mocks/etc. Contains essential design guidelines, colors, type, fonts, assets, and UI kit components for prototyping.
user-invocable: true
---

Read the README.md file within this skill, and explore the other available files.
If creating visual artifacts (slides, mocks, throwaway prototypes, etc), copy assets out and create static HTML files for the user to view. If working on production code, you can copy assets and read the rules here to become an expert in designing with this brand.
If the user invokes this skill without any other guidance, ask them what they want to build or design, ask some questions, and act as an expert designer who outputs HTML artifacts _or_ production code, depending on the need.

Quick orientation:
- `styles.css` is the single global entry point — link it and you get all fonts + tokens.
- Brand = Parhelia Technology — solar amber `#f8af1c` + graphite `#414143`, a parhelion
  (sundog) arc + spark mark over a serif wordmark; brand amber == the instrument hero amber.
- Product = AeroScan instrument: dark scope theme (black `#111`, amber `#ffac11` titles,
  neon-green `#39ff14` radar), display type Neuropol X (titles) / Sofachrome (wordmark) /
  Lato (body). Terse cockpit-label copy. No emoji. Glow, not soft shadow. Hard corners.
- Components live under `components/` and bundle to `window.AeroScanDesignSystem_<id>`.
- The full device recreation is `ui_kits/aeroscan-device/`.
- Real fonts, icons, logos and mascots are in `assets/` — copy them, never redraw them.
