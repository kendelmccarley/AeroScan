import React from "react";

/**
 * AeroScan Panel
 * The instrument's container surface: charcoal-slate fill, hairline inset, optional
 * Neuropol X title bar in amber. Hard corners by default (it's an instrument). Use it
 * to frame telemetry blocks, settings groups, scope readouts.
 */
export function Panel({
  title = null,
  trailing = null,        // node aligned right in the title bar (e.g. a Badge)
  children,
  scope = false,          // green scope outline instead of charcoal hairline
  padded = true,
  style,
  ...rest
}) {
  return (
    <section
      style={{
        background: "var(--surface-card)",
        borderRadius: "var(--radius-md)",
        boxShadow: scope
          ? "inset 0 0 0 1px var(--radar-green), 0 0 14px rgba(57,255,20,0.18)"
          : "var(--shadow-panel)",
        color: "var(--text-body)",
        overflow: "hidden",
        ...style,
      }}
      {...rest}
    >
      {title && (
        <header
          style={{
            display: "flex",
            alignItems: "center",
            justifyContent: "space-between",
            gap: "12px",
            padding: "9px 14px",
            background: "rgba(0,0,0,0.25)",
            borderBottom: "1px solid var(--border-hair)",
          }}
        >
          <span
            style={{
              fontFamily: "var(--font-title)",
              fontSize: "13px",
              letterSpacing: "var(--tracking-label)",
              textTransform: "uppercase",
              color: "var(--text-amber)",
            }}
          >
            {title}
          </span>
          {trailing}
        </header>
      )}
      <div style={{ padding: padded ? "14px" : 0 }}>{children}</div>
    </section>
  );
}
