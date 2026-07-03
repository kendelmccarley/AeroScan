import React from "react";

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
function MaskIcon({ base, path, color, size = 28, glow = false }) {
  return (
    <span
      style={{
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
        flex: "0 0 auto",
      }}
    />
  );
}

export function StatusBar({
  iconBase = "assets/icons",
  battery = "full",            // "full" | "mid" | "charging" | "unknown"
  wifi = "conn_4bar",          // conn_4bar | conn_2bar | connecting | disconnected | off
  gps = "okay",                // "okay" | "nolock" | "off"
  adsb = "on",                 // "on" | "off"
  time = null,                 // optional {day,date,clock} or string
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

  return (
    <div
      style={{
        display: "flex",
        flexDirection: vertical ? "column" : "row",
        alignItems: "center",
        justifyContent: vertical ? "flex-start" : "center",
        gap: vertical ? "26px" : "18px",
        padding: vertical ? "20px 0" : "8px 14px",
        ...style,
      }}
      {...rest}
    >
      {time && (
        <div
          style={{
            fontFamily: "var(--font-body)",
            fontSize: "var(--size-body)",
            color: chrome,
            textAlign: "center",
            lineHeight: 1.25,
            whiteSpace: "pre-line",
          }}
        >
          {typeof time === "string" ? time : `${time.day}\n${time.date}\n${time.clock}`}
        </div>
      )}
      <MaskIcon base={iconBase} path={battPath} color={battery === "charging" ? "var(--status-ok)" : chrome} />
      <MaskIcon base={iconBase} path={wifiPath} color={wifi === "disconnected" || wifi === "off" ? "var(--text-muted)" : chrome} />
      <MaskIcon base={iconBase} path={gpsPath} color={gpsColor} glow={gps === "okay"} />
      <MaskIcon base={iconBase} path={adsbPath} color={adsbColor} glow={adsb === "on"} />
    </div>
  );
}
