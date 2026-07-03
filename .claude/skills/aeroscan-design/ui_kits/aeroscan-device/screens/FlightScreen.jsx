/* AeroScan Flight List — FlightBoard recreation: amber title, ghosted AV globe, the live
   aircraft table (DataTable) with carrier-tinted callsigns and a running total. Up/Down move
   the row cursor; Select cycles the sort column; Back exits. */
const { useState } = React;

function FlightScreen({ onExit }) {
  const { DataTable, Badge } = window.AeroScanDesignSystem_2d719c;
  const columns = [
    { key: "cs", label: "Callsign", width: "27%" },
    { key: "alt", label: "Alt", align: "right" },
    { key: "spd", label: "Speed", align: "right" },
    { key: "hdg", label: "Hdg", align: "right" },
    { key: "dist", label: "Dist", align: "right" },
  ];
  const C = { SWA: "var(--air-swa)", DAL: "var(--air-dal)", UAL: "var(--air-ual)", AAL: "var(--air-aal)", FDX: "var(--air-fdx)", UPS: "var(--air-ups)" };
  const rows = [
    { cs: "AAL55", alt: "GND", spd: "—", hdg: "020", dist: "5.0", _color: { cs: C.AAL } },
    { cs: "SWA241", alt: "8200", spd: "212", hdg: "250", dist: "8.1", _color: { cs: C.SWA } },
    { cs: "N172SP", alt: "4500", spd: "118", hdg: "075", dist: "11.0", _color: { cs: "var(--air-unknown)" } },
    { cs: "UAL902", alt: "15025", spd: "288", hdg: "320", dist: "14.2", _color: { cs: C.UAL } },
    { cs: "DAL1180", alt: "23000", spd: "402", hdg: "095", dist: "19.4", _color: { cs: C.DAL } },
    { cs: "FDX19", alt: "41000", spd: "501", hdg: "180", dist: "31.2", _color: { cs: C.FDX } },
    { cs: "UPS28", alt: "37000", spd: "488", hdg: "210", dist: "42.6", _color: { cs: C.UPS } },
  ];
  const sortKeys = ["dist", "alt", "spd", "cs"];
  const [sel, setSel] = useState(0);
  const [sortI, setSortI] = useState(0);

  window.useNav((a) => {
    if (a === "back") onExit();
    else if (a === "up") setSel((s) => Math.max(0, s - 1));
    else if (a === "down") setSel((s) => Math.min(rows.length - 1, s + 1));
    else if (a === "select") setSortI((i) => (i + 1) % sortKeys.length);
  });

  return (
    <div style={{ position: "absolute", inset: 0, padding: "16px 14px 0", boxSizing: "border-box", display: "flex", flexDirection: "column" }}>
      <div style={{
        position: "absolute", top: "52%", left: "50%", width: 190, height: 260, transform: "translate(-50%,-50%)",
        background: "url(../../assets/brand/parhelia-mark.png) center/contain no-repeat",
        opacity: 0.07, pointerEvents: "none",
      }} />
      <div style={{ display: "flex", alignItems: "center", justifyContent: "center", position: "relative", marginBottom: 12 }}>
        <span style={{ fontFamily: "var(--font-title)", fontSize: 22, color: "var(--text-amber)",
          letterSpacing: "var(--tracking-title)", textShadow: "var(--glow-amber)" }}>Flight List</span>
      </div>
      <div style={{ position: "relative", zIndex: 1, flex: 1, overflow: "hidden" }}>
        <DataTable columns={columns} rows={rows} sortKey={sortKeys[sortI]} sortDir="asc"
          selectedIndex={sel} onSelectRow={setSel} onSort={(k) => setSortI(sortKeys.indexOf(k))} />
      </div>
      <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between",
        padding: "10px 4px 14px", position: "relative", zIndex: 1 }}>
        <span style={{ fontFamily: "var(--font-title)", fontSize: 12, color: "var(--text-amber)", letterSpacing: ".04em" }}>
          Total Aircraft: {rows.length}
        </span>
        <Badge tone="cyan" variant="outline">ADS-B 1090</Badge>
      </div>
    </div>
  );
}
window.FlightScreen = FlightScreen;
