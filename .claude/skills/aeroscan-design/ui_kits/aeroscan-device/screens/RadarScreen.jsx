/* AeroScan Radar Scope — canvas recreation of radarscope.cpp: center (240,240), range rings,
   a rotating neon-green sweep with a fading wedge trail, and aircraft drawn as heading
   chevrons tinted by carrier. Up/Down change the zoom preset (50 / 25 / 10 nm); Back exits. */
const { useRef, useEffect, useState } = React;

const ZOOM_NM = [50, 25, 10];
const ZOOM_RINGS = [[10, 25, 50], [5, 10, 25], [2, 5, 10]];
const AIR = {
  SWA: "#ffa500", DAL: "#3a7bff", UAL: "#3a7bff", AAL: "#ff3b3b", FDX: "#a219ff", UPS: "#b9712e",
};
// Simulated traffic: bearing (deg), distance (nm), heading (deg), callsign
const TRAFFIC = [
  { cs: "SWA241", br: 35, di: 8, hd: 250, al: "SWA" },
  { cs: "DAL1180", br: 110, di: 19, hd: 95, al: "DAL" },
  { cs: "AAL55", br: 200, di: 5, hd: 20, al: "AAL" },
  { cs: "FDX19", br: 285, di: 31, hd: 180, al: "FDX" },
  { cs: "UAL902", br: 150, di: 14, hd: 320, al: "UAL" },
  { cs: "N172SP", br: 320, di: 11, hd: 75, al: null },
  { cs: "UPS28", br: 60, di: 42, hd: 210, al: "UPS" },
];

function RadarScreen({ onExit }) {
  const canvasRef = useRef(null);
  const [zoom, setZoom] = useState(0);
  const zoomRef = useRef(0);
  zoomRef.current = zoom;

  window.useNav((a) => {
    if (a === "back") onExit();
    else if (a === "up") setZoom((z) => Math.max(0, z - 1));
    else if (a === "down") setZoom((z) => Math.min(2, z + 1));
  });

  useEffect(() => {
    const cv = canvasRef.current;
    const ctx = cv.getContext("2d");
    const CX = 240, CY = 240, R = 218;
    let angle = 0, raf;

    function draw() {
      const z = zoomRef.current;
      const rings = ZOOM_RINGS[z];
      const maxNm = ZOOM_NM[z];
      const pxPerNm = R / maxNm;
      ctx.clearRect(0, 0, 480, 480);
      ctx.fillStyle = "#0b0d0b";
      ctx.beginPath(); ctx.arc(CX, CY, R + 8, 0, Math.PI * 2); ctx.fill();

      // range rings
      ctx.lineWidth = 1;
      rings.forEach((nm) => {
        const rr = nm * pxPerNm;
        ctx.strokeStyle = "rgba(57,255,20,0.30)";
        ctx.beginPath(); ctx.arc(CX, CY, rr, 0, Math.PI * 2); ctx.stroke();
        ctx.fillStyle = "rgba(57,255,20,0.5)";
        ctx.font = "10px Lato, sans-serif";
        ctx.fillText(nm + "nm", CX + 3, CY - rr + 12);
      });
      // crosshair graticule
      ctx.strokeStyle = "rgba(57,255,20,0.16)";
      ctx.beginPath(); ctx.moveTo(CX - R, CY); ctx.lineTo(CX + R, CY);
      ctx.moveTo(CX, CY - R); ctx.lineTo(CX, CY + R); ctx.stroke();
      // outer ring
      ctx.strokeStyle = "rgba(57,255,20,0.55)"; ctx.lineWidth = 2;
      ctx.beginPath(); ctx.arc(CX, CY, R, 0, Math.PI * 2); ctx.stroke();

      // sweep trail wedge
      const grad = ctx.createConicGradient ? ctx.createConicGradient(angle - Math.PI / 2, CX, CY) : null;
      ctx.save();
      ctx.beginPath(); ctx.moveTo(CX, CY);
      ctx.arc(CX, CY, R, angle - 0.55, angle); ctx.closePath();
      const g = ctx.createRadialGradient(CX, CY, 0, CX, CY, R);
      g.addColorStop(0, "rgba(57,255,20,0.22)"); g.addColorStop(1, "rgba(57,255,20,0)");
      ctx.fillStyle = g; ctx.fill();
      ctx.restore();
      // sweep line
      ctx.strokeStyle = "#39ff14"; ctx.lineWidth = 2; ctx.shadowColor = "#39ff14"; ctx.shadowBlur = 8;
      ctx.beginPath(); ctx.moveTo(CX, CY);
      ctx.lineTo(CX + Math.cos(angle) * R, CY + Math.sin(angle) * R); ctx.stroke();
      ctx.shadowBlur = 0;

      // aircraft
      TRAFFIC.forEach((t) => {
        if (t.di > maxNm) return;
        const a = (t.br - 90) * Math.PI / 180; // bearing 0 = north(up)
        const x = CX + Math.cos(a) * t.di * pxPerNm;
        const y = CY + Math.sin(a) * t.di * pxPerNm;
        // brighten when sweep recently passed
        let da = ((angle - a) % (Math.PI * 2) + Math.PI * 2) % (Math.PI * 2);
        const fresh = da < 1.1 ? 1 - da / 1.1 : 0;
        const base = t.al ? AIR[t.al] : "#cfd6cf";
        ctx.save();
        ctx.translate(x, y); ctx.rotate((t.hd) * Math.PI / 180);
        ctx.fillStyle = base;
        ctx.globalAlpha = 0.55 + 0.45 * fresh;
        if (fresh > 0.2) { ctx.shadowColor = base; ctx.shadowBlur = 8 * fresh; }
        ctx.beginPath(); ctx.moveTo(0, -6); ctx.lineTo(4, 5); ctx.lineTo(0, 2.5); ctx.lineTo(-4, 5); ctx.closePath();
        ctx.fill();
        ctx.restore();
        // label
        ctx.globalAlpha = 0.5 + 0.5 * fresh;
        ctx.fillStyle = t.al ? base : "rgba(200,206,200,0.8)";
        ctx.font = "9px Lato, sans-serif";
        ctx.fillText(t.cs, x + 7, y - 6);
        ctx.globalAlpha = 1;
      });

      // center dot
      ctx.fillStyle = "#dcdcdc";
      ctx.beginPath(); ctx.arc(CX, CY, 3, 0, Math.PI * 2); ctx.fill();

      angle += 0.025;
      if (angle > Math.PI * 2) angle -= Math.PI * 2;
      raf = requestAnimationFrame(draw);
    }
    draw();
    return () => cancelAnimationFrame(raf);
  }, []);

  const { Badge } = window.AeroScanDesignSystem_2d719c;
  return (
    <div style={{ position: "absolute", inset: 0 }}>
      <canvas ref={canvasRef} width="480" height="480" style={{ position: "absolute", inset: 0 }} />
      <div style={{ position: "absolute", top: 16, left: 0, right: 0, textAlign: "center",
        fontFamily: "var(--font-title)", fontSize: 18, color: "var(--text-amber)",
        letterSpacing: "var(--tracking-title)", textShadow: "var(--glow-amber)", zIndex: 2 }}>
        Radar Scope
      </div>
      <div style={{ position: "absolute", top: 14, right: 14, zIndex: 2 }}>
        <Badge tone="green" dot glow>Live</Badge>
      </div>
      <div style={{ position: "absolute", bottom: 16, right: 18, zIndex: 2,
        fontFamily: "var(--font-body)", fontSize: 13, color: "var(--radar-green)" }}>
        {ZOOM_NM[zoom]} nm
      </div>
      <div style={{ position: "absolute", bottom: 16, left: 18, zIndex: 2,
        fontFamily: "var(--font-body)", fontSize: 11, color: "var(--text-muted)" }}>
        ▲▼ zoom · ◄ back
      </div>
    </div>
  );
}
window.RadarScreen = RadarScreen;
