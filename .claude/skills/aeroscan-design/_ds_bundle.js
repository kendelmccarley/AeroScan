/* @ds-bundle: {"format":3,"namespace":"AeroScanDesignSystem_2d719c","components":[{"name":"Badge","sourcePath":"components/core/Badge.jsx"},{"name":"Button","sourcePath":"components/core/Button.jsx"},{"name":"MenuItem","sourcePath":"components/core/MenuItem.jsx"},{"name":"Panel","sourcePath":"components/core/Panel.jsx"},{"name":"DataTable","sourcePath":"components/data/DataTable.jsx"},{"name":"MessageBox","sourcePath":"components/feedback/MessageBox.jsx"},{"name":"StatusBar","sourcePath":"components/feedback/StatusBar.jsx"},{"name":"Checkbox","sourcePath":"components/forms/Checkbox.jsx"},{"name":"Toggle","sourcePath":"components/forms/Toggle.jsx"}],"sourceHashes":{"components/core/Badge.jsx":"4964122e58ff","components/core/Button.jsx":"8cb423acd830","components/core/MenuItem.jsx":"a5bb2a88cd75","components/core/Panel.jsx":"b919ba7f622b","components/data/DataTable.jsx":"434f7da652a1","components/feedback/MessageBox.jsx":"1a4b03c3605c","components/feedback/StatusBar.jsx":"2ce3aad65f60","components/forms/Checkbox.jsx":"abc4e7f58f21","components/forms/Toggle.jsx":"913d9c314576","ui_kits/aeroscan-device/app.jsx":"5e3eb2e095b8","ui_kits/aeroscan-device/screens/FlightScreen.jsx":"3af78d5fe457","ui_kits/aeroscan-device/screens/MainMenuScreen.jsx":"a279e6506479","ui_kits/aeroscan-device/screens/RadarScreen.jsx":"f0df8ed344ec","ui_kits/aeroscan-device/screens/RadioScreen.jsx":"4f9c898aacae","ui_kits/aeroscan-device/screens/SettingsScreen.jsx":"b6b0d6f9f7d4"},"inlinedExternals":[],"unexposedExports":[]} */

(() => {

const __ds_ns = (window.AeroScanDesignSystem_2d719c = window.AeroScanDesignSystem_2d719c || {});

const __ds_scope = {};

(__ds_ns.__errors = __ds_ns.__errors || []);

// components/core/Badge.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/**
 * AeroScan Badge
 * Compact status capsule for telemetry states — LIVE, BLE, WIFI, GND, NO LOCK, drone tags.
 * Two fills: "solid" (filled, dark text) reads as an active signal; "outline" reads as a
 * quiet label. Uppercase Neuropol X, tight pill. Color carries the meaning.
 */
const TONES = {
  green: {
    c: "var(--radar-green)",
    t: "#06210a"
  },
  amber: {
    c: "var(--text-amber)",
    t: "#241500"
  },
  teal: {
    c: "var(--text-teal)",
    t: "#001012"
  },
  magenta: {
    c: "var(--link-magenta)",
    t: "#ffffff"
  },
  red: {
    c: "var(--status-alert)",
    t: "#ffffff"
  },
  cyan: {
    c: "var(--av-cyan)",
    t: "#001722"
  },
  ble: {
    c: "var(--drone-ble)",
    t: "#ffffff"
  },
  wifi: {
    c: "var(--drone-wifi)",
    t: "#221200"
  },
  muted: {
    c: "var(--text-muted)",
    t: "#111111"
  }
};
function Badge({
  children,
  tone = "green",
  variant = "solid",
  // "solid" | "outline"
  dot = false,
  // leading status dot
  glow = false,
  style,
  ...rest
}) {
  const {
    c,
    t
  } = TONES[tone] || TONES.green;
  const solid = variant === "solid";
  return /*#__PURE__*/React.createElement("span", _extends({
    style: {
      display: "inline-flex",
      alignItems: "center",
      gap: "6px",
      fontFamily: "var(--font-title)",
      fontSize: "10px",
      letterSpacing: "var(--tracking-caps)",
      textTransform: "uppercase",
      lineHeight: 1,
      padding: "4px 9px",
      borderRadius: "var(--radius-pill)",
      color: solid ? t : c,
      background: solid ? c : "transparent",
      boxShadow: `${solid ? "none" : `inset 0 0 0 1px ${c}`}${glow ? `, 0 0 9px ${c}88` : ""}`,
      whiteSpace: "nowrap",
      ...style
    }
  }, rest), dot && /*#__PURE__*/React.createElement("span", {
    style: {
      width: "6px",
      height: "6px",
      borderRadius: "50%",
      background: solid ? t : c,
      flex: "0 0 auto"
    }
  }), children);
}
Object.assign(__ds_scope, { Badge });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/core/Badge.jsx", error: String((e && e.message) || e) }); }

// components/core/Button.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/**
 * AeroScan Button
 * Mirrors the device's selectable text-button pattern (winglet-ui MessageBox / menus):
 * a label that is teal at rest, gains an underline + ">" selection caret when focused,
 * and flashes magenta when committed. A "solid" variant gives a filled instrument key
 * for richer touch UIs. All color comes from CSS custom properties.
 */
function Button({
  children,
  variant = "menu",
  // "menu" | "solid" | "ghost"
  tone = "teal",
  // "teal" | "amber" | "magenta"
  selected = false,
  committed = false,
  disabled = false,
  icon = null,
  // optional leading node (img/svg/mask span)
  onClick,
  style,
  ...rest
}) {
  const toneColor = {
    teal: "var(--text-teal)",
    amber: "var(--text-amber)",
    magenta: "var(--link-magenta)"
  }[tone];
  const activeColor = committed ? "var(--link-magenta)" : toneColor;
  const base = {
    display: "inline-flex",
    alignItems: "center",
    gap: "8px",
    fontFamily: "var(--font-body)",
    fontSize: "var(--size-label)",
    letterSpacing: "var(--tracking-label)",
    lineHeight: 1.1,
    cursor: disabled ? "default" : "pointer",
    opacity: disabled ? 0.4 : 1,
    border: "none",
    background: "none",
    transition: "color var(--motion-base) var(--ease-out-quart), background var(--motion-base) var(--ease-standard), box-shadow var(--motion-base)",
    userSelect: "none",
    WebkitTapHighlightColor: "transparent"
  };
  const variants = {
    menu: {
      color: activeColor,
      textDecoration: selected ? "underline" : "none",
      textUnderlineOffset: "4px",
      padding: "2px 0"
    },
    solid: {
      color: "var(--bg-shadow)",
      background: committed ? "var(--link-magenta)" : selected ? toneColor : "var(--surface-raised)",
      padding: "8px 18px",
      borderRadius: "var(--radius-md)",
      fontFamily: "var(--font-title)",
      fontSize: "13px",
      textTransform: "uppercase",
      letterSpacing: "var(--tracking-label)",
      boxShadow: selected ? "var(--glow-amber)" : "var(--shadow-panel)"
    },
    ghost: {
      color: activeColor,
      background: "transparent",
      padding: "7px 16px",
      borderRadius: "var(--radius-md)",
      boxShadow: `inset 0 0 0 1px ${selected ? toneColor : "var(--border-hair)"}`
    }
  };
  const showCaret = variant === "menu" && selected;
  return /*#__PURE__*/React.createElement("button", _extends({
    type: "button",
    onClick: disabled ? undefined : onClick,
    disabled: disabled,
    style: {
      ...base,
      ...variants[variant],
      ...style
    }
  }, rest), showCaret && /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-title)",
      fontWeight: 700,
      color: activeColor
    }
  }, "\u203A"), icon, /*#__PURE__*/React.createElement("span", null, children));
}
Object.assign(__ds_scope, { Button });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/core/Button.jsx", error: String((e && e.message) || e) }); }

// components/core/MenuItem.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/**
 * AeroScan MenuItem
 * A row in the arc/scroll menus and settings lists. Carries an optional leading icon,
 * a primary label (Neuropol X) and an optional second line (Lato). The focused row is
 * brighter, slightly larger and amber; unfocused rows dim down — the firmware's
 * opacity-falloff arc menu, flattened to a list.
 */
function MenuItem({
  label,
  secondLine = null,
  icon = null,
  selected = false,
  disabled = false,
  trailing = null,
  // optional right-side node (value, chevron, toggle)
  onClick,
  style,
  ...rest
}) {
  const color = disabled ? "var(--text-muted)" : selected ? "var(--text-amber)" : "var(--text-teal)";
  return /*#__PURE__*/React.createElement("div", _extends({
    onClick: disabled ? undefined : onClick,
    style: {
      display: "flex",
      alignItems: "center",
      gap: "12px",
      padding: "9px 14px",
      cursor: disabled ? "default" : "pointer",
      opacity: disabled ? 0.45 : selected ? 1 : 0.7,
      background: selected ? "rgba(255,172,17,0.06)" : "transparent",
      boxShadow: selected ? "inset 2px 0 0 var(--text-amber)" : "inset 2px 0 0 transparent",
      transition: "opacity var(--motion-base) var(--ease-out-quart), background var(--motion-base), box-shadow var(--motion-base)",
      WebkitTapHighlightColor: "transparent",
      ...style
    }
  }, rest), selected && /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-title)",
      fontWeight: 700,
      color: "var(--text-amber)",
      fontSize: "15px",
      lineHeight: 1
    }
  }, "\u203A"), icon && /*#__PURE__*/React.createElement("span", {
    style: {
      display: "inline-flex",
      width: "22px",
      justifyContent: "center",
      flex: "0 0 auto"
    }
  }, icon), /*#__PURE__*/React.createElement("span", {
    style: {
      display: "flex",
      flexDirection: "column",
      gap: "2px",
      minWidth: 0,
      flex: 1
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-title)",
      fontSize: "var(--size-label)",
      letterSpacing: "var(--tracking-label)",
      color,
      whiteSpace: "nowrap",
      overflow: "hidden",
      textOverflow: "ellipsis"
    }
  }, label), secondLine && /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-body)",
      fontSize: "var(--size-data)",
      color: "var(--text-muted)"
    }
  }, secondLine)), trailing && /*#__PURE__*/React.createElement("span", {
    style: {
      flex: "0 0 auto",
      color: "var(--text-muted)"
    }
  }, trailing));
}
Object.assign(__ds_scope, { MenuItem });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/core/MenuItem.jsx", error: String((e && e.message) || e) }); }

// components/core/Panel.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/**
 * AeroScan Panel
 * The instrument's container surface: charcoal-slate fill, hairline inset, optional
 * Neuropol X title bar in amber. Hard corners by default (it's an instrument). Use it
 * to frame telemetry blocks, settings groups, scope readouts.
 */
function Panel({
  title = null,
  trailing = null,
  // node aligned right in the title bar (e.g. a Badge)
  children,
  scope = false,
  // green scope outline instead of charcoal hairline
  padded = true,
  style,
  ...rest
}) {
  return /*#__PURE__*/React.createElement("section", _extends({
    style: {
      background: "var(--surface-card)",
      borderRadius: "var(--radius-md)",
      boxShadow: scope ? "inset 0 0 0 1px var(--radar-green), 0 0 14px rgba(57,255,20,0.18)" : "var(--shadow-panel)",
      color: "var(--text-body)",
      overflow: "hidden",
      ...style
    }
  }, rest), title && /*#__PURE__*/React.createElement("header", {
    style: {
      display: "flex",
      alignItems: "center",
      justifyContent: "space-between",
      gap: "12px",
      padding: "9px 14px",
      background: "rgba(0,0,0,0.25)",
      borderBottom: "1px solid var(--border-hair)"
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-title)",
      fontSize: "13px",
      letterSpacing: "var(--tracking-label)",
      textTransform: "uppercase",
      color: "var(--text-amber)"
    }
  }, title), trailing), /*#__PURE__*/React.createElement("div", {
    style: {
      padding: padded ? "14px" : 0
    }
  }, children));
}
Object.assign(__ds_scope, { Panel });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/core/Panel.jsx", error: String((e && e.message) || e) }); }

// components/data/DataTable.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/**
 * AeroScan DataTable
 * The FlightBoard / DroneBoard telemetry list. Charcoal-slate header row (Neuropol X-ish
 * labels), transparent body rows, Lato data cells. Right-aligned numeric columns, a sort
 * caret on the active column, and an optional per-cell color (airline / drone identity).
 *
 * columns: [{ key, label, align?, width? }]
 * rows:    [{ ...cellValues, _color?: { colKey: cssColor } }]
 */
function DataTable({
  columns = [],
  rows = [],
  sortKey = null,
  sortDir = "desc",
  // "asc" | "desc"
  selectedIndex = -1,
  onSort,
  onSelectRow,
  emptyText = "No contacts",
  style,
  ...rest
}) {
  return /*#__PURE__*/React.createElement("div", _extends({
    style: {
      fontFamily: "var(--font-body)",
      color: "var(--text-body)",
      ...style
    }
  }, rest), /*#__PURE__*/React.createElement("table", {
    style: {
      width: "100%",
      borderCollapse: "collapse",
      tableLayout: "fixed"
    }
  }, /*#__PURE__*/React.createElement("thead", null, /*#__PURE__*/React.createElement("tr", null, columns.map(col => {
    const active = col.key === sortKey;
    return /*#__PURE__*/React.createElement("th", {
      key: col.key,
      onClick: onSort ? () => onSort(col.key) : undefined,
      style: {
        textAlign: col.align || "left",
        width: col.width,
        padding: "6px 8px",
        background: "var(--surface)",
        color: active ? "var(--text-amber)" : "var(--text-muted)",
        fontFamily: "var(--font-title)",
        fontSize: "10px",
        letterSpacing: "var(--tracking-label)",
        textTransform: "uppercase",
        cursor: onSort ? "pointer" : "default",
        whiteSpace: "nowrap",
        userSelect: "none"
      }
    }, col.label, active && /*#__PURE__*/React.createElement("span", {
      style: {
        marginLeft: "4px"
      }
    }, sortDir === "desc" ? "▾" : "▴"));
  }))), /*#__PURE__*/React.createElement("tbody", null, rows.length === 0 && /*#__PURE__*/React.createElement("tr", null, /*#__PURE__*/React.createElement("td", {
    colSpan: columns.length,
    style: {
      padding: "22px 8px",
      textAlign: "center",
      color: "var(--text-muted)",
      fontStyle: "italic",
      fontSize: "var(--size-body)"
    }
  }, emptyText)), rows.map((row, i) => {
    const sel = i === selectedIndex;
    return /*#__PURE__*/React.createElement("tr", {
      key: i,
      onClick: onSelectRow ? () => onSelectRow(i) : undefined,
      style: {
        background: sel ? "rgba(255,172,17,0.08)" : "transparent",
        boxShadow: sel ? "inset 2px 0 0 var(--text-amber)" : "none",
        cursor: onSelectRow ? "pointer" : "default"
      }
    }, columns.map(col => /*#__PURE__*/React.createElement("td", {
      key: col.key,
      style: {
        textAlign: col.align || "left",
        padding: "6px 8px",
        fontSize: "var(--size-cell)",
        color: row._color && row._color[col.key] || "var(--text-body)",
        borderBottom: "1px solid rgba(255,255,255,0.05)",
        whiteSpace: "nowrap",
        overflow: "hidden",
        textOverflow: "ellipsis"
      }
    }, row[col.key])));
  }))));
}
Object.assign(__ds_scope, { DataTable });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/data/DataTable.jsx", error: String((e && e.message) || e) }); }

// components/feedback/MessageBox.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/**
 * AeroScan MessageBox
 * The device modal (winglet-ui MessageBox): Neuropol X amber title, Lato muted body, and
 * a vertical stack of selectable text buttons with the ">" caret on the focused one.
 * Fades in over 150ms. Rendered over the vignette of the current screen.
 *
 * buttons: [{ label, tone?, onClick }]
 */
function MessageBox({
  title = null,
  message = "",
  buttons = [{
    label: "Okay"
  }],
  selectedIndex = 0,
  onSelect,
  width = 408,
  style,
  ...rest
}) {
  return /*#__PURE__*/React.createElement("div", _extends({
    style: {
      width,
      background: "var(--bg)",
      borderRadius: "var(--radius-md)",
      boxShadow: "inset 0 0 0 1px var(--border-hair), 0 18px 50px rgba(0,0,0,0.7)",
      padding: "26px 28px 22px",
      textAlign: "center",
      ...style
    }
  }, rest), title && /*#__PURE__*/React.createElement("div", {
    style: {
      fontFamily: "var(--font-title)",
      fontSize: "var(--size-title)",
      color: "var(--text-amber)",
      letterSpacing: "var(--tracking-title)",
      marginBottom: "14px",
      lineHeight: 1.1
    }
  }, title), /*#__PURE__*/React.createElement("div", {
    style: {
      fontFamily: "var(--font-body)",
      fontSize: "var(--size-body)",
      color: "var(--text-muted)",
      lineHeight: "var(--leading-body)",
      marginBottom: "22px",
      textWrap: "pretty"
    }
  }, message), /*#__PURE__*/React.createElement("div", {
    style: {
      display: "flex",
      flexDirection: "column",
      alignItems: "center",
      gap: "8px"
    }
  }, buttons.map((b, i) => /*#__PURE__*/React.createElement(__ds_scope.Button, {
    key: i,
    variant: "menu",
    tone: b.tone || "teal",
    selected: i === selectedIndex,
    onClick: () => b.onClick ? b.onClick() : onSelect && onSelect(i)
  }, b.label))));
}
Object.assign(__ds_scope, { MessageBox });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/MessageBox.jsx", error: String((e && e.message) || e) }); }

// components/feedback/StatusBar.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/**
 * AeroScan StatusBar
 * The instrument's status rail (battery · wifi · gps · adsb), rendered from the device's
 * own monochrome PNG glyphs recolored via CSS mask — exactly how the firmware tints them
 * (loadMonochromeIcon / loadColoredIcon, SourceIn composite). GPS goes green on lock,
 * red on no-lock; the rest tint to the muted chrome color.
 *
 * iconBase: path to the /assets/icons folder from the consuming page.
 * orientation: "vertical" (device side-rail) | "horizontal" (compact header).
 */
function MaskIcon({
  base,
  path,
  color,
  size = 28,
  glow = false
}) {
  return /*#__PURE__*/React.createElement("span", {
    style: {
      display: "inline-block",
      width: size,
      height: size,
      background: color,
      WebkitMaskImage: `url(${base}/${path})`,
      maskImage: `url(${base}/${path})`,
      WebkitMaskRepeat: "no-repeat",
      maskRepeat: "no-repeat",
      WebkitMaskPosition: "center",
      maskPosition: "center",
      WebkitMaskSize: "contain",
      maskSize: "contain",
      filter: glow ? `drop-shadow(0 0 5px ${color})` : "none",
      flex: "0 0 auto"
    }
  });
}
function StatusBar({
  iconBase = "assets/icons",
  battery = "full",
  // "full" | "mid" | "charging" | "unknown"
  wifi = "conn_4bar",
  // conn_4bar | conn_2bar | connecting | disconnected | off
  gps = "okay",
  // "okay" | "nolock" | "off"
  adsb = "on",
  // "on" | "off"
  time = null,
  // optional {day,date,clock} or string
  orientation = "vertical",
  style,
  ...rest
}) {
  const chrome = "var(--text-muted)";
  const gpsColor = gps === "okay" ? "var(--status-ok)" : "var(--status-alert)";
  const adsbColor = adsb === "on" ? "var(--radar-green)" : "var(--text-muted)";
  const battPath = battery === "charging" ? "batt/chg_full.png" : battery === "mid" ? "batt/dsg_4bar.png" : "batt/dsg_full.png";
  const wifiPath = `wifi/${wifi}.png`;
  const gpsPath = `location/${gps === "okay" ? "okay" : gps === "nolock" ? "nolock" : "off"}.png`;
  const adsbPath = `adsb/${adsb === "on" ? "on" : "off"}.png`;
  const vertical = orientation === "vertical";
  return /*#__PURE__*/React.createElement("div", _extends({
    style: {
      display: "flex",
      flexDirection: vertical ? "column" : "row",
      alignItems: "center",
      justifyContent: vertical ? "flex-start" : "center",
      gap: vertical ? "26px" : "18px",
      padding: vertical ? "20px 0" : "8px 14px",
      ...style
    }
  }, rest), time && /*#__PURE__*/React.createElement("div", {
    style: {
      fontFamily: "var(--font-body)",
      fontSize: "var(--size-body)",
      color: chrome,
      textAlign: "center",
      lineHeight: 1.25,
      whiteSpace: "pre-line"
    }
  }, typeof time === "string" ? time : `${time.day}\n${time.date}\n${time.clock}`), /*#__PURE__*/React.createElement(MaskIcon, {
    base: iconBase,
    path: battPath,
    color: battery === "charging" ? "var(--status-ok)" : chrome
  }), /*#__PURE__*/React.createElement(MaskIcon, {
    base: iconBase,
    path: wifiPath,
    color: wifi === "disconnected" || wifi === "off" ? "var(--text-muted)" : chrome
  }), /*#__PURE__*/React.createElement(MaskIcon, {
    base: iconBase,
    path: gpsPath,
    color: gpsColor,
    glow: gps === "okay"
  }), /*#__PURE__*/React.createElement(MaskIcon, {
    base: iconBase,
    path: adsbPath,
    color: adsbColor,
    glow: adsb === "on"
  }));
}
Object.assign(__ds_scope, { StatusBar });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/StatusBar.jsx", error: String((e && e.message) || e) }); }

// components/forms/Checkbox.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/**
 * AeroScan Checkbox
 * Square instrument check used in settings lists. Supports an indeterminate state
 * (the firmware ships a tri-state checkbox icon). Checked = amber fill + dark glyph.
 */
function Checkbox({
  checked = false,
  indeterminate = false,
  disabled = false,
  onChange,
  label = null,
  style,
  ...rest
}) {
  const on = checked || indeterminate;
  const box = /*#__PURE__*/React.createElement("span", {
    role: "checkbox",
    "aria-checked": indeterminate ? "mixed" : checked,
    onClick: disabled ? undefined : () => onChange && onChange(!checked),
    style: {
      position: "relative",
      display: "inline-flex",
      alignItems: "center",
      justifyContent: "center",
      width: "20px",
      height: "20px",
      borderRadius: "var(--radius-sm)",
      background: on ? "var(--text-amber)" : "transparent",
      boxShadow: on ? "0 0 8px rgba(255,172,17,0.4)" : "inset 0 0 0 1.5px var(--text-teal)",
      cursor: disabled ? "default" : "pointer",
      opacity: disabled ? 0.4 : 1,
      transition: "background var(--motion-base), box-shadow var(--motion-base)",
      flex: "0 0 auto"
    }
  }, checked && !indeterminate && /*#__PURE__*/React.createElement("svg", {
    width: "12",
    height: "12",
    viewBox: "0 0 12 12",
    fill: "none"
  }, /*#__PURE__*/React.createElement("path", {
    d: "M2 6.2L4.7 9L10 3",
    stroke: "var(--bg-shadow)",
    strokeWidth: "2",
    strokeLinecap: "square"
  })), indeterminate && /*#__PURE__*/React.createElement("span", {
    style: {
      width: "10px",
      height: "2.5px",
      background: "var(--bg-shadow)"
    }
  }));
  if (!label) return /*#__PURE__*/React.createElement("span", _extends({
    style: style
  }, rest), box);
  return /*#__PURE__*/React.createElement("label", _extends({
    style: {
      display: "inline-flex",
      alignItems: "center",
      gap: "12px",
      cursor: disabled ? "default" : "pointer",
      fontFamily: "var(--font-body)",
      fontSize: "var(--size-body)",
      color: "var(--text-body)",
      ...style
    }
  }, rest), box, /*#__PURE__*/React.createElement("span", null, label));
}
Object.assign(__ds_scope, { Checkbox });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/Checkbox.jsx", error: String((e && e.message) || e) }); }

// components/forms/Toggle.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/**
 * AeroScan Toggle
 * The settings pill switch. Off = hollow charcoal track with a muted knob; On = filled
 * amber track with a dark knob and a faint glow. 150ms ease, matching the firmware.
 */
function Toggle({
  checked = false,
  disabled = false,
  onChange,
  label = null,
  tone = "amber",
  // "amber" | "teal" | "green"
  style,
  ...rest
}) {
  const onColor = {
    amber: "var(--text-amber)",
    teal: "var(--text-teal)",
    green: "var(--radar-green)"
  }[tone];
  const sw = /*#__PURE__*/React.createElement("span", {
    role: "switch",
    "aria-checked": checked,
    onClick: disabled ? undefined : () => onChange && onChange(!checked),
    style: {
      position: "relative",
      display: "inline-block",
      width: "44px",
      height: "24px",
      borderRadius: "var(--radius-pill)",
      background: checked ? onColor : "var(--surface)",
      boxShadow: checked ? `0 0 9px ${onColor}66` : "inset 0 0 0 1px var(--border-hair)",
      cursor: disabled ? "default" : "pointer",
      opacity: disabled ? 0.4 : 1,
      transition: "background var(--motion-base) var(--ease-standard), box-shadow var(--motion-base)",
      flex: "0 0 auto",
      verticalAlign: "middle"
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      position: "absolute",
      top: "3px",
      left: checked ? "23px" : "3px",
      width: "18px",
      height: "18px",
      borderRadius: "50%",
      background: checked ? "var(--bg-shadow)" : "var(--text-muted)",
      transition: "left var(--motion-base) var(--ease-out-quart), background var(--motion-base)"
    }
  }));
  if (!label) return /*#__PURE__*/React.createElement("span", _extends({
    style: style
  }, rest), sw);
  return /*#__PURE__*/React.createElement("label", _extends({
    style: {
      display: "inline-flex",
      alignItems: "center",
      gap: "12px",
      cursor: disabled ? "default" : "pointer",
      fontFamily: "var(--font-body)",
      fontSize: "var(--size-body)",
      color: "var(--text-body)",
      ...style
    }
  }, rest), sw, /*#__PURE__*/React.createElement("span", null, label));
}
Object.assign(__ds_scope, { Toggle });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/Toggle.jsx", error: String((e && e.message) || e) }); }

// ui_kits/aeroscan-device/app.jsx
try { (() => {
/* AeroScan device shell — column chrome, screen routing, and the four-quadrant nav bus.
   Touch on the real badge maps to four quadrant keys (UP / DOWN / SELECT / BACK); here the
   left column exposes them as buttons and the keyboard mirrors them. */
const {
  useState,
  useEffect,
  useRef,
  useCallback
} = React;

// --- tiny pub/sub nav bus ---
function makeBus() {
  const subs = new Set();
  return {
    emit: action => subs.forEach(fn => fn(action)),
    subscribe: fn => {
      subs.add(fn);
      return () => subs.delete(fn);
    }
  };
}
const bus = makeBus();

// Subscribe helper for screens
window.useNav = function useNav(handler) {
  const ref = useRef(handler);
  ref.current = handler;
  useEffect(() => bus.subscribe(a => ref.current && ref.current(a)), []);
};
function Clock() {
  const [now, setNow] = useState(new Date());
  useEffect(() => {
    const t = setInterval(() => setNow(new Date()), 1000);
    return () => clearInterval(t);
  }, []);
  const day = now.toLocaleDateString("en-US", {
    weekday: "short"
  }).toUpperCase();
  const date = now.toLocaleDateString("en-US", {
    month: "short",
    day: "numeric"
  }).toUpperCase();
  const clock = now.toLocaleTimeString("en-US", {
    hour: "2-digit",
    minute: "2-digit",
    hour12: false
  });
  return {
    day,
    date,
    clock
  };
}
const SCREENS = {
  radar: {
    comp: () => window.RadarScreen,
    title: "Radar Scope"
  },
  flight: {
    comp: () => window.FlightScreen,
    title: "Flight List"
  },
  radio: {
    comp: () => window.RadioScreen,
    title: "Radio Tuner"
  },
  settings: {
    comp: () => window.SettingsScreen,
    title: "Settings"
  }
};
function LeftColumn() {
  const fire = a => () => bus.emit(a);
  return /*#__PURE__*/React.createElement("div", {
    className: "brandcol"
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: "flex",
      flexDirection: "column",
      alignItems: "center",
      gap: 16
    }
  }, /*#__PURE__*/React.createElement("div", {
    className: "av"
  }), /*#__PURE__*/React.createElement("div", {
    className: "wm"
  }, "AEROSCAN")), /*#__PURE__*/React.createElement("div", {
    style: {
      display: "flex",
      flexDirection: "column",
      gap: 12,
      alignItems: "center"
    }
  }, /*#__PURE__*/React.createElement("div", {
    className: "quad"
  }, /*#__PURE__*/React.createElement("div", {
    className: "qbtn",
    onClick: fire("up")
  }, "\u25B2 Up"), /*#__PURE__*/React.createElement("div", {
    className: "qbtn",
    onClick: fire("select")
  }, "\u25CF Sel"), /*#__PURE__*/React.createElement("div", {
    className: "qbtn",
    onClick: fire("down")
  }, "\u25BC Dn"), /*#__PURE__*/React.createElement("div", {
    className: "qbtn",
    onClick: fire("back")
  }, "\u25C4 Back")), /*#__PURE__*/React.createElement("div", {
    className: "qhint"
  }, "touch quadrants")));
}
function RightColumn({
  gps,
  adsb
}) {
  const {
    StatusBar
  } = window.AeroScanDesignSystem_2d719c;
  const time = Clock();
  return /*#__PURE__*/React.createElement("div", {
    style: {
      height: "100%",
      display: "flex",
      alignItems: "center",
      justifyContent: "center"
    }
  }, /*#__PURE__*/React.createElement(StatusBar, {
    iconBase: "../../assets/icons",
    orientation: "vertical",
    gps: gps,
    adsb: adsb,
    wifi: "conn_4bar",
    battery: "full",
    time: time
  }));
}
function App() {
  const [screen, setScreen] = useState(() => localStorage.getItem("aeroscan.screen") || "menu");
  const open = useCallback(key => {
    setScreen(key);
    localStorage.setItem("aeroscan.screen", key);
  }, []);
  const exit = useCallback(() => {
    setScreen("menu");
    localStorage.setItem("aeroscan.screen", "menu");
  }, []);

  // keyboard mirrors the quad buttons
  useEffect(() => {
    const onKey = e => {
      const map = {
        ArrowUp: "up",
        ArrowDown: "down",
        Enter: "select",
        " ": "select",
        Backspace: "back",
        Escape: "back"
      };
      if (map[e.key]) {
        e.preventDefault();
        bus.emit(map[e.key]);
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, []);
  let body;
  if (screen === "menu") {
    const Menu = window.MainMenuScreen;
    body = Menu ? /*#__PURE__*/React.createElement(Menu, {
      onOpen: open
    }) : null;
  } else {
    const Comp = SCREENS[screen] && SCREENS[screen].comp();
    body = Comp ? /*#__PURE__*/React.createElement(Comp, {
      onExit: exit
    }) : null;
  }

  // status reflects current screen for a touch of life
  const adsb = screen === "radio" ? "off" : "on";
  return /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement("div", {
    className: "col-side"
  }, /*#__PURE__*/React.createElement(LeftColumn, null)), /*#__PURE__*/React.createElement("div", {
    className: "screen"
  }, /*#__PURE__*/React.createElement("div", {
    className: "screen-inner fade-enter",
    key: screen
  }, body)), /*#__PURE__*/React.createElement("div", {
    className: "col-side"
  }, /*#__PURE__*/React.createElement(RightColumn, {
    gps: "okay",
    adsb: adsb
  })));
}

// Expose the app; the page mounts it once after all scripts load (app.jsx is also bundled
// into _ds_bundle.js, so it must NOT self-mount or it double-roots #app).
window.__AeroScanApp = App;
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/aeroscan-device/app.jsx", error: String((e && e.message) || e) }); }

// ui_kits/aeroscan-device/screens/FlightScreen.jsx
try { (() => {
/* AeroScan Flight List — FlightBoard recreation: amber title, ghosted AV globe, the live
   aircraft table (DataTable) with carrier-tinted callsigns and a running total. Up/Down move
   the row cursor; Select cycles the sort column; Back exits. */
const {
  useState
} = React;
function FlightScreen({
  onExit
}) {
  const {
    DataTable,
    Badge
  } = window.AeroScanDesignSystem_2d719c;
  const columns = [{
    key: "cs",
    label: "Callsign",
    width: "27%"
  }, {
    key: "alt",
    label: "Alt",
    align: "right"
  }, {
    key: "spd",
    label: "Speed",
    align: "right"
  }, {
    key: "hdg",
    label: "Hdg",
    align: "right"
  }, {
    key: "dist",
    label: "Dist",
    align: "right"
  }];
  const C = {
    SWA: "var(--air-swa)",
    DAL: "var(--air-dal)",
    UAL: "var(--air-ual)",
    AAL: "var(--air-aal)",
    FDX: "var(--air-fdx)",
    UPS: "var(--air-ups)"
  };
  const rows = [{
    cs: "AAL55",
    alt: "GND",
    spd: "—",
    hdg: "020",
    dist: "5.0",
    _color: {
      cs: C.AAL
    }
  }, {
    cs: "SWA241",
    alt: "8200",
    spd: "212",
    hdg: "250",
    dist: "8.1",
    _color: {
      cs: C.SWA
    }
  }, {
    cs: "N172SP",
    alt: "4500",
    spd: "118",
    hdg: "075",
    dist: "11.0",
    _color: {
      cs: "var(--air-unknown)"
    }
  }, {
    cs: "UAL902",
    alt: "15025",
    spd: "288",
    hdg: "320",
    dist: "14.2",
    _color: {
      cs: C.UAL
    }
  }, {
    cs: "DAL1180",
    alt: "23000",
    spd: "402",
    hdg: "095",
    dist: "19.4",
    _color: {
      cs: C.DAL
    }
  }, {
    cs: "FDX19",
    alt: "41000",
    spd: "501",
    hdg: "180",
    dist: "31.2",
    _color: {
      cs: C.FDX
    }
  }, {
    cs: "UPS28",
    alt: "37000",
    spd: "488",
    hdg: "210",
    dist: "42.6",
    _color: {
      cs: C.UPS
    }
  }];
  const sortKeys = ["dist", "alt", "spd", "cs"];
  const [sel, setSel] = useState(0);
  const [sortI, setSortI] = useState(0);
  window.useNav(a => {
    if (a === "back") onExit();else if (a === "up") setSel(s => Math.max(0, s - 1));else if (a === "down") setSel(s => Math.min(rows.length - 1, s + 1));else if (a === "select") setSortI(i => (i + 1) % sortKeys.length);
  });
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      inset: 0,
      padding: "16px 14px 0",
      boxSizing: "border-box",
      display: "flex",
      flexDirection: "column"
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      top: "52%",
      left: "50%",
      width: 190,
      height: 260,
      transform: "translate(-50%,-50%)",
      background: "url(../../assets/brand/parhelia-mark.png) center/contain no-repeat",
      opacity: 0.07,
      pointerEvents: "none"
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      display: "flex",
      alignItems: "center",
      justifyContent: "center",
      position: "relative",
      marginBottom: 12
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-title)",
      fontSize: 22,
      color: "var(--text-amber)",
      letterSpacing: "var(--tracking-title)",
      textShadow: "var(--glow-amber)"
    }
  }, "Flight List")), /*#__PURE__*/React.createElement("div", {
    style: {
      position: "relative",
      zIndex: 1,
      flex: 1,
      overflow: "hidden"
    }
  }, /*#__PURE__*/React.createElement(DataTable, {
    columns: columns,
    rows: rows,
    sortKey: sortKeys[sortI],
    sortDir: "asc",
    selectedIndex: sel,
    onSelectRow: setSel,
    onSort: k => setSortI(sortKeys.indexOf(k))
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      display: "flex",
      alignItems: "center",
      justifyContent: "space-between",
      padding: "10px 4px 14px",
      position: "relative",
      zIndex: 1
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-title)",
      fontSize: 12,
      color: "var(--text-amber)",
      letterSpacing: ".04em"
    }
  }, "Total Aircraft: ", rows.length), /*#__PURE__*/React.createElement(Badge, {
    tone: "cyan",
    variant: "outline"
  }, "ADS-B 1090")));
}
window.FlightScreen = FlightScreen;
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/aeroscan-device/screens/FlightScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/aeroscan-device/screens/MainMenuScreen.jsx
try { (() => {
/* AeroScan main menu — the firmware's centered "arc" scroll menu, flattened to a focus list.
   Title pinned top, AV globe ghosted behind, the selected entry centered, large and amber
   with a "›" caret; neighbours shrink and dim with distance. */
const {
  useState
} = React;
function MainMenuScreen({
  onOpen
}) {
  const items = [{
    key: "map",
    label: "Map Scope",
    demo: true
  }, {
    key: "radar",
    label: "Radar Scope"
  }, {
    key: "flight",
    label: "Flight List"
  }, {
    key: "radio",
    label: "Radio Tuner"
  }, {
    key: "settings",
    label: "Settings"
  }, {
    key: "power",
    label: "Power",
    demo: true
  }];
  const [idx, setIdx] = useState(1);
  const [msg, setMsg] = useState(null);
  const {
    MessageBox
  } = window.AeroScanDesignSystem_2d719c;
  window.useNav(a => {
    if (msg) {
      if (a === "select" || a === "back") setMsg(null);
      return;
    }
    if (a === "up") setIdx(i => (i - 1 + items.length) % items.length);else if (a === "down") setIdx(i => (i + 1) % items.length);else if (a === "select") {
      const it = items[idx];
      if (it.demo) setMsg(it.label);else onOpen(it.key);
    }
  });
  const VISIBLE = 2; // each side of center
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      inset: 0,
      display: "flex",
      flexDirection: "column",
      alignItems: "center"
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      top: "50%",
      left: "50%",
      width: 220,
      height: 300,
      transform: "translate(-50%,-48%)",
      background: "url(../../assets/brand/parhelia-mark.png) center/contain no-repeat",
      opacity: 0.1,
      pointerEvents: "none"
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      marginTop: 30,
      fontFamily: "var(--font-title)",
      fontSize: 24,
      color: "var(--text-amber)",
      letterSpacing: "var(--tracking-title)",
      textShadow: "var(--glow-amber)",
      zIndex: 1
    }
  }, "Main Menu"), /*#__PURE__*/React.createElement("div", {
    style: {
      position: "relative",
      zIndex: 1,
      flex: 1,
      width: "100%",
      display: "flex",
      flexDirection: "column",
      alignItems: "center",
      justifyContent: "center",
      gap: 4
    }
  }, items.map((it, i) => {
    let d = i - idx;
    if (d > items.length / 2) d -= items.length;
    if (d < -items.length / 2) d += items.length;
    const dist = Math.abs(d);
    if (dist > VISIBLE) return null;
    const sel = d === 0;
    const size = sel ? 26 : 22 - (dist - 1) * 4;
    const op = sel ? 1 : 0.62 - (dist - 1) * 0.18;
    const indent = dist * 16; // arc curve
    return /*#__PURE__*/React.createElement("div", {
      key: it.key,
      onClick: () => it.demo ? setMsg(it.label) : onOpen(it.key),
      style: {
        display: "flex",
        alignItems: "center",
        gap: 8,
        cursor: "pointer",
        transform: `translateX(${indent}px)`,
        transition: "all var(--motion-base) var(--ease-out-quart)"
      }
    }, sel && /*#__PURE__*/React.createElement("span", {
      style: {
        fontFamily: "var(--font-title)",
        fontWeight: 700,
        color: "var(--text-teal)",
        fontSize: 22
      }
    }, "\u203A"), /*#__PURE__*/React.createElement("span", {
      style: {
        fontFamily: "var(--font-title)",
        fontSize: size,
        letterSpacing: "var(--tracking-label)",
        color: sel ? "var(--text-amber)" : "var(--text-teal)",
        opacity: op
      }
    }, it.label));
  })), msg && /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      inset: 0,
      zIndex: 60,
      display: "flex",
      alignItems: "center",
      justifyContent: "center",
      background: "rgba(0,0,0,0.55)"
    }
  }, /*#__PURE__*/React.createElement(MessageBox, {
    title: msg,
    width: 360,
    message: msg === "Power" ? "Power options are disabled in this demo build." : `${msg} is a live screen on the badge — not wired into this demo.`,
    buttons: [{
      label: "Okay"
    }],
    selectedIndex: 0,
    onSelect: () => setMsg(null)
  })));
}
window.MainMenuScreen = MainMenuScreen;
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/aeroscan-device/screens/MainMenuScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/aeroscan-device/screens/RadarScreen.jsx
try { (() => {
/* AeroScan Radar Scope — canvas recreation of radarscope.cpp: center (240,240), range rings,
   a rotating neon-green sweep with a fading wedge trail, and aircraft drawn as heading
   chevrons tinted by carrier. Up/Down change the zoom preset (50 / 25 / 10 nm); Back exits. */
const {
  useRef,
  useEffect,
  useState
} = React;
const ZOOM_NM = [50, 25, 10];
const ZOOM_RINGS = [[10, 25, 50], [5, 10, 25], [2, 5, 10]];
const AIR = {
  SWA: "#ffa500",
  DAL: "#3a7bff",
  UAL: "#3a7bff",
  AAL: "#ff3b3b",
  FDX: "#a219ff",
  UPS: "#b9712e"
};
// Simulated traffic: bearing (deg), distance (nm), heading (deg), callsign
const TRAFFIC = [{
  cs: "SWA241",
  br: 35,
  di: 8,
  hd: 250,
  al: "SWA"
}, {
  cs: "DAL1180",
  br: 110,
  di: 19,
  hd: 95,
  al: "DAL"
}, {
  cs: "AAL55",
  br: 200,
  di: 5,
  hd: 20,
  al: "AAL"
}, {
  cs: "FDX19",
  br: 285,
  di: 31,
  hd: 180,
  al: "FDX"
}, {
  cs: "UAL902",
  br: 150,
  di: 14,
  hd: 320,
  al: "UAL"
}, {
  cs: "N172SP",
  br: 320,
  di: 11,
  hd: 75,
  al: null
}, {
  cs: "UPS28",
  br: 60,
  di: 42,
  hd: 210,
  al: "UPS"
}];
function RadarScreen({
  onExit
}) {
  const canvasRef = useRef(null);
  const [zoom, setZoom] = useState(0);
  const zoomRef = useRef(0);
  zoomRef.current = zoom;
  window.useNav(a => {
    if (a === "back") onExit();else if (a === "up") setZoom(z => Math.max(0, z - 1));else if (a === "down") setZoom(z => Math.min(2, z + 1));
  });
  useEffect(() => {
    const cv = canvasRef.current;
    const ctx = cv.getContext("2d");
    const CX = 240,
      CY = 240,
      R = 218;
    let angle = 0,
      raf;
    function draw() {
      const z = zoomRef.current;
      const rings = ZOOM_RINGS[z];
      const maxNm = ZOOM_NM[z];
      const pxPerNm = R / maxNm;
      ctx.clearRect(0, 0, 480, 480);
      ctx.fillStyle = "#0b0d0b";
      ctx.beginPath();
      ctx.arc(CX, CY, R + 8, 0, Math.PI * 2);
      ctx.fill();

      // range rings
      ctx.lineWidth = 1;
      rings.forEach(nm => {
        const rr = nm * pxPerNm;
        ctx.strokeStyle = "rgba(57,255,20,0.30)";
        ctx.beginPath();
        ctx.arc(CX, CY, rr, 0, Math.PI * 2);
        ctx.stroke();
        ctx.fillStyle = "rgba(57,255,20,0.5)";
        ctx.font = "10px Lato, sans-serif";
        ctx.fillText(nm + "nm", CX + 3, CY - rr + 12);
      });
      // crosshair graticule
      ctx.strokeStyle = "rgba(57,255,20,0.16)";
      ctx.beginPath();
      ctx.moveTo(CX - R, CY);
      ctx.lineTo(CX + R, CY);
      ctx.moveTo(CX, CY - R);
      ctx.lineTo(CX, CY + R);
      ctx.stroke();
      // outer ring
      ctx.strokeStyle = "rgba(57,255,20,0.55)";
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(CX, CY, R, 0, Math.PI * 2);
      ctx.stroke();

      // sweep trail wedge
      const grad = ctx.createConicGradient ? ctx.createConicGradient(angle - Math.PI / 2, CX, CY) : null;
      ctx.save();
      ctx.beginPath();
      ctx.moveTo(CX, CY);
      ctx.arc(CX, CY, R, angle - 0.55, angle);
      ctx.closePath();
      const g = ctx.createRadialGradient(CX, CY, 0, CX, CY, R);
      g.addColorStop(0, "rgba(57,255,20,0.22)");
      g.addColorStop(1, "rgba(57,255,20,0)");
      ctx.fillStyle = g;
      ctx.fill();
      ctx.restore();
      // sweep line
      ctx.strokeStyle = "#39ff14";
      ctx.lineWidth = 2;
      ctx.shadowColor = "#39ff14";
      ctx.shadowBlur = 8;
      ctx.beginPath();
      ctx.moveTo(CX, CY);
      ctx.lineTo(CX + Math.cos(angle) * R, CY + Math.sin(angle) * R);
      ctx.stroke();
      ctx.shadowBlur = 0;

      // aircraft
      TRAFFIC.forEach(t => {
        if (t.di > maxNm) return;
        const a = (t.br - 90) * Math.PI / 180; // bearing 0 = north(up)
        const x = CX + Math.cos(a) * t.di * pxPerNm;
        const y = CY + Math.sin(a) * t.di * pxPerNm;
        // brighten when sweep recently passed
        let da = ((angle - a) % (Math.PI * 2) + Math.PI * 2) % (Math.PI * 2);
        const fresh = da < 1.1 ? 1 - da / 1.1 : 0;
        const base = t.al ? AIR[t.al] : "#cfd6cf";
        ctx.save();
        ctx.translate(x, y);
        ctx.rotate(t.hd * Math.PI / 180);
        ctx.fillStyle = base;
        ctx.globalAlpha = 0.55 + 0.45 * fresh;
        if (fresh > 0.2) {
          ctx.shadowColor = base;
          ctx.shadowBlur = 8 * fresh;
        }
        ctx.beginPath();
        ctx.moveTo(0, -6);
        ctx.lineTo(4, 5);
        ctx.lineTo(0, 2.5);
        ctx.lineTo(-4, 5);
        ctx.closePath();
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
      ctx.beginPath();
      ctx.arc(CX, CY, 3, 0, Math.PI * 2);
      ctx.fill();
      angle += 0.025;
      if (angle > Math.PI * 2) angle -= Math.PI * 2;
      raf = requestAnimationFrame(draw);
    }
    draw();
    return () => cancelAnimationFrame(raf);
  }, []);
  const {
    Badge
  } = window.AeroScanDesignSystem_2d719c;
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      inset: 0
    }
  }, /*#__PURE__*/React.createElement("canvas", {
    ref: canvasRef,
    width: "480",
    height: "480",
    style: {
      position: "absolute",
      inset: 0
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      top: 16,
      left: 0,
      right: 0,
      textAlign: "center",
      fontFamily: "var(--font-title)",
      fontSize: 18,
      color: "var(--text-amber)",
      letterSpacing: "var(--tracking-title)",
      textShadow: "var(--glow-amber)",
      zIndex: 2
    }
  }, "Radar Scope"), /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      top: 14,
      right: 14,
      zIndex: 2
    }
  }, /*#__PURE__*/React.createElement(Badge, {
    tone: "green",
    dot: true,
    glow: true
  }, "Live")), /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      bottom: 16,
      right: 18,
      zIndex: 2,
      fontFamily: "var(--font-body)",
      fontSize: 13,
      color: "var(--radar-green)"
    }
  }, ZOOM_NM[zoom], " nm"), /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      bottom: 16,
      left: 18,
      zIndex: 2,
      fontFamily: "var(--font-body)",
      fontSize: 11,
      color: "var(--text-muted)"
    }
  }, "\u25B2\u25BC zoom \xB7 \u25C4 back"));
}
window.RadarScreen = RadarScreen;
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/aeroscan-device/screens/RadarScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/aeroscan-device/screens/RadioScreen.jsx
try { (() => {
/* AeroScan Radio Tuner — the software-radio screen (radiotuner spec): band selector
   (AM airband / FM / 2m), a large frequency readout, and a preset list from frequencies.json.
   Up/Down scroll presets, Select tunes, Back stops the radio and exits. ADS-B drops while
   the dongle is held (the right-rail ADS-B icon goes dark via app state). */
const {
  useState
} = React;
const BANDS = {
  AM: {
    label: "AM Airband",
    mode: "AM",
    presets: [{
      n: "Guard",
      f: 121.500
    }, {
      n: "Local Tower",
      f: 118.300
    }, {
      n: "ATIS",
      f: 133.000
    }, {
      n: "Approach",
      f: 119.100
    }]
  },
  FM: {
    label: "FM Broadcast",
    mode: "WFM",
    presets: [{
      n: "KXCI",
      f: 91.30
    }, {
      n: "The Mountain",
      f: 92.90
    }, {
      n: "NPR News",
      f: 89.10
    }, {
      n: "Classic Rock",
      f: 106.30
    }]
  },
  "2m": {
    label: "2m Amateur",
    mode: "NFM",
    presets: [{
      n: "2m Calling",
      f: 146.520
    }, {
      n: "APRS",
      f: 144.390
    }, {
      n: "Repeater",
      f: 147.120
    }]
  }
};
const ORDER = ["AM", "FM", "2m"];
function RadioScreen({
  onExit
}) {
  const {
    Button,
    Badge
  } = window.AeroScanDesignSystem_2d719c;
  const [bandI, setBandI] = useState(0);
  const [presetI, setPresetI] = useState(0);
  const [tuned, setTuned] = useState({
    band: 0,
    preset: 0
  });
  const band = BANDS[ORDER[bandI]];
  const presets = band.presets;
  window.useNav(a => {
    if (a === "back") onExit();else if (a === "up") setPresetI(p => Math.max(0, p - 1));else if (a === "down") setPresetI(p => Math.min(presets.length - 1, p + 1));else if (a === "select") setTuned({
      band: bandI,
      preset: presetI
    });
  });
  const cycleBand = i => {
    setBandI(i);
    setPresetI(0);
  };
  const tunedFreq = BANDS[ORDER[tuned.band]].presets[tuned.preset];
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      inset: 0,
      padding: "16px 18px",
      boxSizing: "border-box",
      display: "flex",
      flexDirection: "column"
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      textAlign: "center",
      fontFamily: "var(--font-title)",
      fontSize: 20,
      color: "var(--text-amber)",
      letterSpacing: "var(--tracking-title)",
      textShadow: "var(--glow-amber)",
      marginBottom: 14
    }
  }, "Radio Tuner"), /*#__PURE__*/React.createElement("div", {
    style: {
      display: "flex",
      gap: 8,
      justifyContent: "center",
      marginBottom: 14
    }
  }, ORDER.map((b, i) => /*#__PURE__*/React.createElement(Button, {
    key: b,
    variant: "solid",
    tone: "amber",
    selected: i === bandI,
    onClick: () => cycleBand(i)
  }, b))), /*#__PURE__*/React.createElement("div", {
    style: {
      textAlign: "center",
      marginBottom: 6
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-title)",
      fontSize: 46,
      color: "var(--text-amber)",
      textShadow: "var(--glow-amber)",
      letterSpacing: "0.01em"
    }
  }, tunedFreq.f.toFixed(3)), /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-body)",
      fontSize: 16,
      color: "var(--text-muted)",
      marginLeft: 8
    }
  }, "MHz")), /*#__PURE__*/React.createElement("div", {
    style: {
      textAlign: "center",
      marginBottom: 14,
      display: "flex",
      gap: 8,
      justifyContent: "center",
      alignItems: "center"
    }
  }, /*#__PURE__*/React.createElement(Badge, {
    tone: "green",
    dot: true,
    glow: true
  }, "Live \xB7 ", band.mode), /*#__PURE__*/React.createElement(Badge, {
    tone: "muted",
    variant: "outline"
  }, tunedFreq.n)), /*#__PURE__*/React.createElement("div", {
    style: {
      flex: 1,
      overflow: "hidden",
      borderTop: "1px solid var(--border-hair)"
    }
  }, presets.map((p, i) => {
    const sel = i === presetI;
    const isTuned = tuned.band === bandI && tuned.preset === i;
    return /*#__PURE__*/React.createElement("div", {
      key: p.n,
      onClick: () => {
        setPresetI(i);
        setTuned({
          band: bandI,
          preset: i
        });
      },
      style: {
        display: "flex",
        alignItems: "center",
        justifyContent: "space-between",
        padding: "9px 8px",
        cursor: "pointer",
        background: sel ? "rgba(255,172,17,0.08)" : "transparent",
        boxShadow: sel ? "inset 2px 0 0 var(--text-amber)" : "none",
        borderBottom: "1px solid rgba(255,255,255,0.05)"
      }
    }, /*#__PURE__*/React.createElement("span", {
      style: {
        display: "flex",
        alignItems: "center",
        gap: 8
      }
    }, sel && /*#__PURE__*/React.createElement("span", {
      style: {
        fontFamily: "var(--font-title)",
        color: "var(--text-teal)",
        fontWeight: 700
      }
    }, "\u203A"), /*#__PURE__*/React.createElement("span", {
      style: {
        fontFamily: "var(--font-body)",
        fontSize: 14,
        color: sel ? "var(--text-amber)" : "var(--text-teal)"
      }
    }, p.n), isTuned && /*#__PURE__*/React.createElement("span", {
      style: {
        width: 6,
        height: 6,
        borderRadius: "50%",
        background: "var(--radar-green)",
        boxShadow: "var(--glow-green)"
      }
    })), /*#__PURE__*/React.createElement("span", {
      style: {
        fontFamily: "var(--font-body)",
        fontSize: 13,
        color: "var(--text-muted)"
      }
    }, p.f.toFixed(3)));
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      fontFamily: "var(--font-body)",
      fontSize: 11,
      color: "var(--text-muted)",
      paddingTop: 8
    }
  }, "\u25B2\u25BC presets \xB7 \u25CF tune \xB7 \u25C4 back \u2014 ADS-B paused while tuned"));
}
window.RadioScreen = RadioScreen;
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/aeroscan-device/screens/RadioScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/aeroscan-device/screens/SettingsScreen.jsx
try { (() => {
/* AeroScan Settings — the settings list (settingsmenu): a scroll list of labelled rows with
   Toggle / Checkbox controls. Up/Down move the cursor, Select flips the focused control,
   Back exits. */
const {
  useState
} = React;
function SettingsScreen({
  onExit
}) {
  const {
    MenuItem,
    Toggle,
    Checkbox,
    Badge
  } = window.AeroScanDesignSystem_2d719c;
  const [vals, setVals] = useState({
    darkMaps: true,
    clock12: false,
    invScroll: false,
    persistPos: true,
    fastChg: false,
    adsb: true
  });
  const rows = [{
    key: "darkMaps",
    label: "Dark Maps",
    second: "Dark map tiles on Map Scope",
    type: "toggle"
  }, {
    key: "clock12",
    label: "12-Hour Clock",
    second: "Status bar time format",
    type: "toggle"
  }, {
    key: "invScroll",
    label: "Inverted Scroll",
    second: "Reverse menu wheel direction",
    type: "toggle"
  }, {
    key: "persistPos",
    label: "Persist Position",
    second: "Save last GPS fix to SD",
    type: "check"
  }, {
    key: "fastChg",
    label: "Fast Charge",
    second: "Higher current draw",
    type: "toggle"
  }, {
    key: "adsb",
    label: "ADS-B Receiver",
    second: "dump1090 on port 30003",
    type: "toggle"
  }];
  const [sel, setSel] = useState(0);
  const flip = k => setVals(v => ({
    ...v,
    [k]: !v[k]
  }));
  window.useNav(a => {
    if (a === "back") onExit();else if (a === "up") setSel(s => Math.max(0, s - 1));else if (a === "down") setSel(s => Math.min(rows.length - 1, s + 1));else if (a === "select") flip(rows[sel].key);
  });
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: "absolute",
      inset: 0,
      display: "flex",
      flexDirection: "column"
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      textAlign: "center",
      padding: "16px 0 10px",
      fontFamily: "var(--font-title)",
      fontSize: 22,
      color: "var(--text-amber)",
      letterSpacing: "var(--tracking-title)",
      textShadow: "var(--glow-amber)"
    }
  }, "Settings"), /*#__PURE__*/React.createElement("div", {
    style: {
      flex: 1,
      overflow: "hidden",
      padding: "0 4px"
    }
  }, rows.map((r, i) => {
    const ctrl = r.type === "toggle" ? /*#__PURE__*/React.createElement(Toggle, {
      checked: vals[r.key],
      onChange: () => flip(r.key),
      tone: r.key === "adsb" ? "green" : "amber"
    }) : /*#__PURE__*/React.createElement(Checkbox, {
      checked: vals[r.key],
      onChange: () => flip(r.key)
    });
    return /*#__PURE__*/React.createElement(MenuItem, {
      key: r.key,
      label: r.label,
      secondLine: r.second,
      selected: i === sel,
      trailing: ctrl,
      onClick: () => {
        setSel(i);
        flip(r.key);
      }
    });
  })), /*#__PURE__*/React.createElement("div", {
    style: {
      display: "flex",
      alignItems: "center",
      justifyContent: "space-between",
      padding: "10px 14px 14px"
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      fontFamily: "var(--font-body)",
      fontSize: 11,
      color: "var(--text-muted)"
    }
  }, "\u25B2\u25BC move \xB7 \u25CF toggle \xB7 \u25C4 back"), /*#__PURE__*/React.createElement(Badge, {
    tone: "teal",
    variant: "outline"
  }, "v2.0")));
}
window.SettingsScreen = SettingsScreen;
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/aeroscan-device/screens/SettingsScreen.jsx", error: String((e && e.message) || e) }); }

__ds_ns.Badge = __ds_scope.Badge;

__ds_ns.Button = __ds_scope.Button;

__ds_ns.MenuItem = __ds_scope.MenuItem;

__ds_ns.Panel = __ds_scope.Panel;

__ds_ns.DataTable = __ds_scope.DataTable;

__ds_ns.MessageBox = __ds_scope.MessageBox;

__ds_ns.StatusBar = __ds_scope.StatusBar;

__ds_ns.Checkbox = __ds_scope.Checkbox;

__ds_ns.Toggle = __ds_scope.Toggle;

})();
