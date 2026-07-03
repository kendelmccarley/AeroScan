import React from "react";

/**
 * AeroScan MenuItem
 * A row in the arc/scroll menus and settings lists. Carries an optional leading icon,
 * a primary label (Neuropol X) and an optional second line (Lato). The focused row is
 * brighter, slightly larger and amber; unfocused rows dim down — the firmware's
 * opacity-falloff arc menu, flattened to a list.
 */
export function MenuItem({
  label,
  secondLine = null,
  icon = null,
  selected = false,
  disabled = false,
  trailing = null,        // optional right-side node (value, chevron, toggle)
  onClick,
  style,
  ...rest
}) {
  const color = disabled
    ? "var(--text-muted)"
    : selected
    ? "var(--text-amber)"
    : "var(--text-teal)";

  return (
    <div
      onClick={disabled ? undefined : onClick}
      style={{
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
        ...style,
      }}
      {...rest}
    >
      {selected && (
        <span style={{ fontFamily: "var(--font-title)", fontWeight: 700, color: "var(--text-amber)", fontSize: "15px", lineHeight: 1 }}>›</span>
      )}
      {icon && (
        <span style={{ display: "inline-flex", width: "22px", justifyContent: "center", flex: "0 0 auto" }}>{icon}</span>
      )}
      <span style={{ display: "flex", flexDirection: "column", gap: "2px", minWidth: 0, flex: 1 }}>
        <span
          style={{
            fontFamily: "var(--font-title)",
            fontSize: "var(--size-label)",
            letterSpacing: "var(--tracking-label)",
            color,
            whiteSpace: "nowrap",
            overflow: "hidden",
            textOverflow: "ellipsis",
          }}
        >
          {label}
        </span>
        {secondLine && (
          <span style={{ fontFamily: "var(--font-body)", fontSize: "var(--size-data)", color: "var(--text-muted)" }}>
            {secondLine}
          </span>
        )}
      </span>
      {trailing && <span style={{ flex: "0 0 auto", color: "var(--text-muted)" }}>{trailing}</span>}
    </div>
  );
}
