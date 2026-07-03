import React from "react";

/**
 * AeroScan Toggle
 * The settings pill switch. Off = hollow charcoal track with a muted knob; On = filled
 * amber track with a dark knob and a faint glow. 150ms ease, matching the firmware.
 */
export function Toggle({
  checked = false,
  disabled = false,
  onChange,
  label = null,
  tone = "amber",         // "amber" | "teal" | "green"
  style,
  ...rest
}) {
  const onColor = {
    amber: "var(--text-amber)",
    teal: "var(--text-teal)",
    green: "var(--radar-green)",
  }[tone];

  const sw = (
    <span
      role="switch"
      aria-checked={checked}
      onClick={disabled ? undefined : () => onChange && onChange(!checked)}
      style={{
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
        verticalAlign: "middle",
      }}
    >
      <span
        style={{
          position: "absolute",
          top: "3px",
          left: checked ? "23px" : "3px",
          width: "18px",
          height: "18px",
          borderRadius: "50%",
          background: checked ? "var(--bg-shadow)" : "var(--text-muted)",
          transition: "left var(--motion-base) var(--ease-out-quart), background var(--motion-base)",
        }}
      />
    </span>
  );

  if (!label) return <span style={style} {...rest}>{sw}</span>;

  return (
    <label
      style={{
        display: "inline-flex",
        alignItems: "center",
        gap: "12px",
        cursor: disabled ? "default" : "pointer",
        fontFamily: "var(--font-body)",
        fontSize: "var(--size-body)",
        color: "var(--text-body)",
        ...style,
      }}
      {...rest}
    >
      {sw}
      <span>{label}</span>
    </label>
  );
}
