import React from "react";
import { Button } from "../core/Button.jsx";

/**
 * AeroScan MessageBox
 * The device modal (winglet-ui MessageBox): Neuropol X amber title, Lato muted body, and
 * a vertical stack of selectable text buttons with the ">" caret on the focused one.
 * Fades in over 150ms. Rendered over the vignette of the current screen.
 *
 * buttons: [{ label, tone?, onClick }]
 */
export function MessageBox({
  title = null,
  message = "",
  buttons = [{ label: "Okay" }],
  selectedIndex = 0,
  onSelect,
  width = 408,
  style,
  ...rest
}) {
  return (
    <div
      style={{
        width,
        background: "var(--bg)",
        borderRadius: "var(--radius-md)",
        boxShadow: "inset 0 0 0 1px var(--border-hair), 0 18px 50px rgba(0,0,0,0.7)",
        padding: "26px 28px 22px",
        textAlign: "center",
        ...style,
      }}
      {...rest}
    >
      {title && (
        <div
          style={{
            fontFamily: "var(--font-title)",
            fontSize: "var(--size-title)",
            color: "var(--text-amber)",
            letterSpacing: "var(--tracking-title)",
            marginBottom: "14px",
            lineHeight: 1.1,
          }}
        >
          {title}
        </div>
      )}
      <div
        style={{
          fontFamily: "var(--font-body)",
          fontSize: "var(--size-body)",
          color: "var(--text-muted)",
          lineHeight: "var(--leading-body)",
          marginBottom: "22px",
          textWrap: "pretty",
        }}
      >
        {message}
      </div>
      <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: "8px" }}>
        {buttons.map((b, i) => (
          <Button
            key={i}
            variant="menu"
            tone={b.tone || "teal"}
            selected={i === selectedIndex}
            onClick={() => (b.onClick ? b.onClick() : onSelect && onSelect(i))}
          >
            {b.label}
          </Button>
        ))}
      </div>
    </div>
  );
}
