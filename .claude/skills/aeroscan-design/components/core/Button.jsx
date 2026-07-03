import React from "react";

/**
 * AeroScan Button
 * Mirrors the device's selectable text-button pattern (winglet-ui MessageBox / menus):
 * a label that is teal at rest, gains an underline + ">" selection caret when focused,
 * and flashes magenta when committed. A "solid" variant gives a filled instrument key
 * for richer touch UIs. All color comes from CSS custom properties.
 */
export function Button({
  children,
  variant = "menu",        // "menu" | "solid" | "ghost"
  tone = "teal",           // "teal" | "amber" | "magenta"
  selected = false,
  committed = false,
  disabled = false,
  icon = null,             // optional leading node (img/svg/mask span)
  onClick,
  style,
  ...rest
}) {
  const toneColor = {
    teal: "var(--text-teal)",
    amber: "var(--text-amber)",
    magenta: "var(--link-magenta)",
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
    WebkitTapHighlightColor: "transparent",
  };

  const variants = {
    menu: {
      color: activeColor,
      textDecoration: selected ? "underline" : "none",
      textUnderlineOffset: "4px",
      padding: "2px 0",
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
      boxShadow: selected ? "var(--glow-amber)" : "var(--shadow-panel)",
    },
    ghost: {
      color: activeColor,
      background: "transparent",
      padding: "7px 16px",
      borderRadius: "var(--radius-md)",
      boxShadow: `inset 0 0 0 1px ${selected ? toneColor : "var(--border-hair)"}`,
    },
  };

  const showCaret = variant === "menu" && selected;

  return (
    <button
      type="button"
      onClick={disabled ? undefined : onClick}
      disabled={disabled}
      style={{ ...base, ...variants[variant], ...style }}
      {...rest}
    >
      {showCaret && (
        <span style={{ fontFamily: "var(--font-title)", fontWeight: 700, color: activeColor }}>›</span>
      )}
      {icon}
      <span>{children}</span>
    </button>
  );
}
