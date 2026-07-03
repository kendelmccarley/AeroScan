/* AeroScan device shell — column chrome, screen routing, and the four-quadrant nav bus.
   Touch on the real badge maps to four quadrant keys (UP / DOWN / SELECT / BACK); here the
   left column exposes them as buttons and the keyboard mirrors them. */
const { useState, useEffect, useRef, useCallback } = React;

// --- tiny pub/sub nav bus ---
function makeBus() {
  const subs = new Set();
  return {
    emit: (action) => subs.forEach((fn) => fn(action)),
    subscribe: (fn) => { subs.add(fn); return () => subs.delete(fn); },
  };
}
const bus = makeBus();

// Subscribe helper for screens
window.useNav = function useNav(handler) {
  const ref = useRef(handler);
  ref.current = handler;
  useEffect(() => bus.subscribe((a) => ref.current && ref.current(a)), []);
};

function Clock() {
  const [now, setNow] = useState(new Date());
  useEffect(() => { const t = setInterval(() => setNow(new Date()), 1000); return () => clearInterval(t); }, []);
  const day = now.toLocaleDateString("en-US", { weekday: "short" }).toUpperCase();
  const date = now.toLocaleDateString("en-US", { month: "short", day: "numeric" }).toUpperCase();
  const clock = now.toLocaleTimeString("en-US", { hour: "2-digit", minute: "2-digit", hour12: false });
  return { day, date, clock };
}

const SCREENS = {
  radar: { comp: () => window.RadarScreen, title: "Radar Scope" },
  flight: { comp: () => window.FlightScreen, title: "Flight List" },
  radio: { comp: () => window.RadioScreen, title: "Radio Tuner" },
  settings: { comp: () => window.SettingsScreen, title: "Settings" },
};

function LeftColumn() {
  const fire = (a) => () => bus.emit(a);
  return (
    <div className="brandcol">
      <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 16 }}>
        <div className="av" />
        <div className="wm">AEROSCAN</div>
      </div>
      <div style={{ display: "flex", flexDirection: "column", gap: 12, alignItems: "center" }}>
        <div className="quad">
          <div className="qbtn" onClick={fire("up")}>▲ Up</div>
          <div className="qbtn" onClick={fire("select")}>● Sel</div>
          <div className="qbtn" onClick={fire("down")}>▼ Dn</div>
          <div className="qbtn" onClick={fire("back")}>◄ Back</div>
        </div>
        <div className="qhint">touch quadrants</div>
      </div>
    </div>
  );
}

function RightColumn({ gps, adsb }) {
  const { StatusBar } = window.AeroScanDesignSystem_2d719c;
  const time = Clock();
  return (
    <div style={{ height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }}>
      <StatusBar iconBase="../../assets/icons" orientation="vertical"
        gps={gps} adsb={adsb} wifi="conn_4bar" battery="full" time={time} />
    </div>
  );
}

function App() {
  const [screen, setScreen] = useState(() => localStorage.getItem("aeroscan.screen") || "menu");
  const open = useCallback((key) => { setScreen(key); localStorage.setItem("aeroscan.screen", key); }, []);
  const exit = useCallback(() => { setScreen("menu"); localStorage.setItem("aeroscan.screen", "menu"); }, []);

  // keyboard mirrors the quad buttons
  useEffect(() => {
    const onKey = (e) => {
      const map = { ArrowUp: "up", ArrowDown: "down", Enter: "select", " ": "select", Backspace: "back", Escape: "back" };
      if (map[e.key]) { e.preventDefault(); bus.emit(map[e.key]); }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, []);

  let body;
  if (screen === "menu") {
    const Menu = window.MainMenuScreen;
    body = Menu ? <Menu onOpen={open} /> : null;
  } else {
    const Comp = SCREENS[screen] && SCREENS[screen].comp();
    body = Comp ? <Comp onExit={exit} /> : null;
  }

  // status reflects current screen for a touch of life
  const adsb = screen === "radio" ? "off" : "on";

  return (
    <React.Fragment>
      <div className="col-side"><LeftColumn /></div>
      <div className="screen">
        <div className="screen-inner fade-enter" key={screen}>{body}</div>
      </div>
      <div className="col-side"><RightColumn gps="okay" adsb={adsb} /></div>
    </React.Fragment>
  );
}

// Expose the app; the page mounts it once after all scripts load (app.jsx is also bundled
// into _ds_bundle.js, so it must NOT self-mount or it double-roots #app).
window.__AeroScanApp = App;
