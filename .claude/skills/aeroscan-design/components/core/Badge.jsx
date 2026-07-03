import React from "react";

/**
 * AeroScan Badge
 * Compact status capsule for telemetry states — LIVE, BLE, WIFI, GND, NO LOCK, drone tags.
 * Two fills: "solid" (filled, dark text) reads as an active signal; "outline" reads as a
 * quiet label. Uppercase Neuropol X, tight pill. Color carries the meaning.
 */
const TONES = {
  green:   { c: "var(--radar-green)",  t: "#06210a" },
  amber:   { c: "var(--text-amber)",   t: "#241500" },
  teal:    { c: "var(--text-teal)",    t: "#001012" },
  magenta: { c: "var(--link-magenta)", t: "#ffffff" },
  red:     { c: "var(--status-alert)", t: "#ffffff" },
  cyan:    { c: "var(--av-cyan)",      t: "#001722" },
  ble:     { c: "var(--drone-ble)",    t: "#ffffff" },
  wifi:    { c: "var(--drone-wifi)",   t: "#221200" },
  muted:   { c: "var(--text-muted)",   t: "#111111" },
};

export function Badge({
  children,
  tone = "green",
  variant = "solid",      // "solid" | "outline"
  dot = false,            // leading status dot
  glow = false,
  style,
  ...rest
}) {
  const { c, t } = TONES[tone] || TONES.green;
  const solid = variant === "solid";
  return (
    <span
      style={{
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
        ...style,
      }}
      {...rest}
    >
      {dot && (
        <span style={{ width: "6px", height: "6px", borderRadius: "50%", background: solid ? t : c, flex: "0 0 auto" }} />
      )}
      {children}
    </span>
  );
}
