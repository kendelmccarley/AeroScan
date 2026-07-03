/* AeroScan Settings — the settings list (settingsmenu): a scroll list of labelled rows with
   Toggle / Checkbox controls. Up/Down move the cursor, Select flips the focused control,
   Back exits. */
const { useState } = React;

function SettingsScreen({ onExit }) {
  const { MenuItem, Toggle, Checkbox, Badge } = window.AeroScanDesignSystem_2d719c;
  const [vals, setVals] = useState({
    darkMaps: true, clock12: false, invScroll: false, persistPos: true, fastChg: false, adsb: true,
  });
  const rows = [
    { key: "darkMaps", label: "Dark Maps", second: "Dark map tiles on Map Scope", type: "toggle" },
    { key: "clock12", label: "12-Hour Clock", second: "Status bar time format", type: "toggle" },
    { key: "invScroll", label: "Inverted Scroll", second: "Reverse menu wheel direction", type: "toggle" },
    { key: "persistPos", label: "Persist Position", second: "Save last GPS fix to SD", type: "check" },
    { key: "fastChg", label: "Fast Charge", second: "Higher current draw", type: "toggle" },
    { key: "adsb", label: "ADS-B Receiver", second: "dump1090 on port 30003", type: "toggle" },
  ];
  const [sel, setSel] = useState(0);
  const flip = (k) => setVals((v) => ({ ...v, [k]: !v[k] }));

  window.useNav((a) => {
    if (a === "back") onExit();
    else if (a === "up") setSel((s) => Math.max(0, s - 1));
    else if (a === "down") setSel((s) => Math.min(rows.length - 1, s + 1));
    else if (a === "select") flip(rows[sel].key);
  });

  return (
    <div style={{ position: "absolute", inset: 0, display: "flex", flexDirection: "column" }}>
      <div style={{ textAlign: "center", padding: "16px 0 10px", fontFamily: "var(--font-title)", fontSize: 22,
        color: "var(--text-amber)", letterSpacing: "var(--tracking-title)", textShadow: "var(--glow-amber)" }}>
        Settings
      </div>
      <div style={{ flex: 1, overflow: "hidden", padding: "0 4px" }}>
        {rows.map((r, i) => {
          const ctrl = r.type === "toggle"
            ? <Toggle checked={vals[r.key]} onChange={() => flip(r.key)} tone={r.key === "adsb" ? "green" : "amber"} />
            : <Checkbox checked={vals[r.key]} onChange={() => flip(r.key)} />;
          return (
            <MenuItem key={r.key} label={r.label} secondLine={r.second}
              selected={i === sel} trailing={ctrl} onClick={() => { setSel(i); flip(r.key); }} />
          );
        })}
      </div>
      <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "10px 14px 14px" }}>
        <span style={{ fontFamily: "var(--font-body)", fontSize: 11, color: "var(--text-muted)" }}>▲▼ move · ● toggle · ◄ back</span>
        <Badge tone="teal" variant="outline">v2.0</Badge>
      </div>
    </div>
  );
}
window.SettingsScreen = SettingsScreen;
