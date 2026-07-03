/* AeroScan main menu — the firmware's centered "arc" scroll menu, flattened to a focus list.
   Title pinned top, AV globe ghosted behind, the selected entry centered, large and amber
   with a "›" caret; neighbours shrink and dim with distance. */
const { useState } = React;

function MainMenuScreen({ onOpen }) {
  const items = [
    { key: "map", label: "Map Scope", demo: true },
    { key: "radar", label: "Radar Scope" },
    { key: "flight", label: "Flight List" },
    { key: "radio", label: "Radio Tuner" },
    { key: "settings", label: "Settings" },
    { key: "power", label: "Power", demo: true },
  ];
  const [idx, setIdx] = useState(1);
  const [msg, setMsg] = useState(null);
  const { MessageBox } = window.AeroScanDesignSystem_2d719c;

  window.useNav((a) => {
    if (msg) { if (a === "select" || a === "back") setMsg(null); return; }
    if (a === "up") setIdx((i) => (i - 1 + items.length) % items.length);
    else if (a === "down") setIdx((i) => (i + 1) % items.length);
    else if (a === "select") {
      const it = items[idx];
      if (it.demo) setMsg(it.label);
      else onOpen(it.key);
    }
  });

  const VISIBLE = 2; // each side of center
  return (
    <div style={{ position: "absolute", inset: 0, display: "flex", flexDirection: "column", alignItems: "center" }}>
      {/* ghosted Parhelia sundog mark */}
      <div style={{
        position: "absolute", top: "50%", left: "50%", width: 220, height: 300,
        transform: "translate(-50%,-48%)",
        background: "url(../../assets/brand/parhelia-mark.png) center/contain no-repeat",
        opacity: 0.1, pointerEvents: "none",
      }} />

      <div style={{ marginTop: 30, fontFamily: "var(--font-title)", fontSize: 24, color: "var(--text-amber)",
        letterSpacing: "var(--tracking-title)", textShadow: "var(--glow-amber)", zIndex: 1 }}>
        Main Menu
      </div>

      <div style={{ position: "relative", zIndex: 1, flex: 1, width: "100%", display: "flex",
        flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 4 }}>
        {items.map((it, i) => {
          let d = i - idx;
          if (d > items.length / 2) d -= items.length;
          if (d < -items.length / 2) d += items.length;
          const dist = Math.abs(d);
          if (dist > VISIBLE) return null;
          const sel = d === 0;
          const size = sel ? 26 : 22 - (dist - 1) * 4;
          const op = sel ? 1 : 0.62 - (dist - 1) * 0.18;
          const indent = dist * 16; // arc curve
          return (
            <div key={it.key} onClick={() => (it.demo ? setMsg(it.label) : onOpen(it.key))}
              style={{ display: "flex", alignItems: "center", gap: 8, cursor: "pointer",
                transform: `translateX(${indent}px)`,
                transition: "all var(--motion-base) var(--ease-out-quart)" }}>
              {sel && <span style={{ fontFamily: "var(--font-title)", fontWeight: 700, color: "var(--text-teal)", fontSize: 22 }}>›</span>}
              <span style={{ fontFamily: "var(--font-title)", fontSize: size,
                letterSpacing: "var(--tracking-label)",
                color: sel ? "var(--text-amber)" : "var(--text-teal)", opacity: op }}>
                {it.label}
              </span>
            </div>
          );
        })}
      </div>

      {msg && (
        <div style={{ position: "absolute", inset: 0, zIndex: 60, display: "flex", alignItems: "center",
          justifyContent: "center", background: "rgba(0,0,0,0.55)" }}>
          <MessageBox title={msg} width={360}
            message={msg === "Power" ? "Power options are disabled in this demo build." : `${msg} is a live screen on the badge — not wired into this demo.`}
            buttons={[{ label: "Okay" }]} selectedIndex={0} onSelect={() => setMsg(null)} />
        </div>
      )}
    </div>
  );
}
window.MainMenuScreen = MainMenuScreen;
