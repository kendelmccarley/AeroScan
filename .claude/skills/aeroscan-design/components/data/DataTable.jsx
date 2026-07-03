import React from "react";

/**
 * AeroScan DataTable
 * The FlightBoard / DroneBoard telemetry list. Charcoal-slate header row (Neuropol X-ish
 * labels), transparent body rows, Lato data cells. Right-aligned numeric columns, a sort
 * caret on the active column, and an optional per-cell color (airline / drone identity).
 *
 * columns: [{ key, label, align?, width? }]
 * rows:    [{ ...cellValues, _color?: { colKey: cssColor } }]
 */
export function DataTable({
  columns = [],
  rows = [],
  sortKey = null,
  sortDir = "desc",        // "asc" | "desc"
  selectedIndex = -1,
  onSort,
  onSelectRow,
  emptyText = "No contacts",
  style,
  ...rest
}) {
  return (
    <div style={{ fontFamily: "var(--font-body)", color: "var(--text-body)", ...style }} {...rest}>
      <table style={{ width: "100%", borderCollapse: "collapse", tableLayout: "fixed" }}>
        <thead>
          <tr>
            {columns.map((col) => {
              const active = col.key === sortKey;
              return (
                <th
                  key={col.key}
                  onClick={onSort ? () => onSort(col.key) : undefined}
                  style={{
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
                    userSelect: "none",
                  }}
                >
                  {col.label}
                  {active && <span style={{ marginLeft: "4px" }}>{sortDir === "desc" ? "▾" : "▴"}</span>}
                </th>
              );
            })}
          </tr>
        </thead>
        <tbody>
          {rows.length === 0 && (
            <tr>
              <td
                colSpan={columns.length}
                style={{ padding: "22px 8px", textAlign: "center", color: "var(--text-muted)", fontStyle: "italic", fontSize: "var(--size-body)" }}
              >
                {emptyText}
              </td>
            </tr>
          )}
          {rows.map((row, i) => {
            const sel = i === selectedIndex;
            return (
              <tr
                key={i}
                onClick={onSelectRow ? () => onSelectRow(i) : undefined}
                style={{
                  background: sel ? "rgba(255,172,17,0.08)" : "transparent",
                  boxShadow: sel ? "inset 2px 0 0 var(--text-amber)" : "none",
                  cursor: onSelectRow ? "pointer" : "default",
                }}
              >
                {columns.map((col) => (
                  <td
                    key={col.key}
                    style={{
                      textAlign: col.align || "left",
                      padding: "6px 8px",
                      fontSize: "var(--size-cell)",
                      color: (row._color && row._color[col.key]) || "var(--text-body)",
                      borderBottom: "1px solid rgba(255,255,255,0.05)",
                      whiteSpace: "nowrap",
                      overflow: "hidden",
                      textOverflow: "ellipsis",
                    }}
                  >
                    {row[col.key]}
                  </td>
                ))}
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}
