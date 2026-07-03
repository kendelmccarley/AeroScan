/* AeroScan Radio Tuner — the software-radio screen (radiotuner spec): band selector
   (AM airband / FM / 2m), a large frequency readout, and a preset list from frequencies.json.
   Up/Down scroll presets, Select tunes, Back stops the radio and exits. ADS-B drops while
   the dongle is held (the right-rail ADS-B icon goes dark via app state). */
const { useState } = React;

const BANDS = {
  AM: { label: "AM Airband", mode: "AM", presets: [
    { n: "Guard", f: 121.500 }, { n: "Local Tower", f: 118.300 },
    { n: "ATIS", f: 133.000 }, { n: "Approach", f: 119.100 } ] },
  FM: { label: "FM Broadcast", mode: "WFM", presets: [
    { n: "KXCI", f: 91.30 }, { n: "The Mountain", f: 92.90 },
    { n: "NPR News", f: 89.10 }, { n: "Classic Rock", f: 106.30 } ] },
  "2m": { label: "2m Amateur", mode: "NFM", presets: [
    { n: "2m Calling", f: 146.520 }, { n: "APRS", f: 144.390 },
    { n: "Repeater", f: 147.120 } ] },
};
const ORDER = ["AM", "FM", "2m"];

function RadioScreen({ onExit }) {
  const { Button, Badge } = window.AeroScanDesignSystem_2d719c;
  const [bandI, setBandI] = useState(0);
  const [presetI, setPresetI] = useState(0);
  const [tuned, setTuned] = useState({ band: 0, preset: 0 });

  const band = BANDS[ORDER[bandI]];
  const presets = band.presets;

  window.useNav((a) => {
    if (a === "back") onExit();
    else if (a === "up") setPresetI((p) => Math.max(0, p - 1));
    else if (a === "down") setPresetI((p) => Math.min(presets.length - 1, p + 1));
    else if (a === "select") setTuned({ band: bandI, preset: presetI });
  });

  const cycleBand = (i) => { setBandI(i); setPresetI(0); };
  const tunedFreq = BANDS[ORDER[tuned.band]].presets[tuned.preset];

  return (
    <div style={{ position: "absolute", inset: 0, padding: "16px 18px", boxSizing: "border-box",
      display: "flex", flexDirection: "column" }}>
      <div style={{ textAlign: "center", fontFamily: "var(--font-title)", fontSize: 20,
        color: "var(--text-amber)", letterSpacing: "var(--tracking-title)", textShadow: "var(--glow-amber)", marginBottom: 14 }}>
        Radio Tuner
      </div>

      {/* band tabs */}
      <div style={{ display: "flex", gap: 8, justifyContent: "center", marginBottom: 14 }}>
        {ORDER.map((b, i) => (
          <Button key={b} variant="solid" tone="amber" selected={i === bandI} onClick={() => cycleBand(i)}>{b}</Button>
        ))}
      </div>

      {/* big frequency */}
      <div style={{ textAlign: "center", marginBottom: 6 }}>
        <span style={{ fontFamily: "var(--font-title)", fontSize: 46, color: "var(--text-amber)",
          textShadow: "var(--glow-amber)", letterSpacing: "0.01em" }}>
          {tunedFreq.f.toFixed(3)}
        </span>
        <span style={{ fontFamily: "var(--font-body)", fontSize: 16, color: "var(--text-muted)", marginLeft: 8 }}>MHz</span>
      </div>
      <div style={{ textAlign: "center", marginBottom: 14, display: "flex", gap: 8, justifyContent: "center", alignItems: "center" }}>
        <Badge tone="green" dot glow>Live · {band.mode}</Badge>
        <Badge tone="muted" variant="outline">{tunedFreq.n}</Badge>
      </div>

      {/* preset list */}
      <div style={{ flex: 1, overflow: "hidden", borderTop: "1px solid var(--border-hair)" }}>
        {presets.map((p, i) => {
          const sel = i === presetI;
          const isTuned = tuned.band === bandI && tuned.preset === i;
          return (
            <div key={p.n} onClick={() => { setPresetI(i); setTuned({ band: bandI, preset: i }); }}
              style={{ display: "flex", alignItems: "center", justifyContent: "space-between",
                padding: "9px 8px", cursor: "pointer",
                background: sel ? "rgba(255,172,17,0.08)" : "transparent",
                boxShadow: sel ? "inset 2px 0 0 var(--text-amber)" : "none",
                borderBottom: "1px solid rgba(255,255,255,0.05)" }}>
              <span style={{ display: "flex", alignItems: "center", gap: 8 }}>
                {sel && <span style={{ fontFamily: "var(--font-title)", color: "var(--text-teal)", fontWeight: 700 }}>›</span>}
                <span style={{ fontFamily: "var(--font-body)", fontSize: 14,
                  color: sel ? "var(--text-amber)" : "var(--text-teal)" }}>{p.n}</span>
                {isTuned && <span style={{ width: 6, height: 6, borderRadius: "50%", background: "var(--radar-green)", boxShadow: "var(--glow-green)" }} />}
              </span>
              <span style={{ fontFamily: "var(--font-body)", fontSize: 13, color: "var(--text-muted)" }}>{p.f.toFixed(3)}</span>
            </div>
          );
        })}
      </div>
      <div style={{ fontFamily: "var(--font-body)", fontSize: 11, color: "var(--text-muted)", paddingTop: 8 }}>
        ▲▼ presets · ● tune · ◄ back — ADS-B paused while tuned
      </div>
    </div>
  );
}
window.RadioScreen = RadioScreen;
