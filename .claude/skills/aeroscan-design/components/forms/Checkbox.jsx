import React from "react";

/**
 * AeroScan Checkbox
 * Square instrument check used in settings lists. Supports an indeterminate state
 * (the firmware ships a tri-state checkbox icon). Checked = amber fill + dark glyph.
 */
export function Checkbox({
  checked = false,
  indeterminate = false,
  disabled = false,
  onChange,
  label = null,
  style,
  ...rest
}) {
  const on = checked || indeterminate;
  const box = (
    <span
      role="checkbox"
      aria-checked={indeterminate ? "mixed" : checked}
      onClick={disabled ? undefined : () => onChange && onChange(!checked)}
      style={{
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
        flex: "0 0 auto",
      }}
    >
      {checked && !indeterminate && (
        <svg width="12" height="12" viewBox="0 0 12 12" fill="none">
          <path d="M2 6.2L4.7 9L10 3" stroke="var(--bg-shadow)" strokeWidth="2" strokeLinecap="square" />
        </svg>
      )}
      {indeterminate && (
        <span style={{ width: "10px", height: "2.5px", background: "var(--bg-shadow)" }} />
      )}
    </span>
  );

  if (!label) return <span style={style} {...rest}>{box}</span>;

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
      {box}
      <span>{label}</span>
    </label>
  );
}
