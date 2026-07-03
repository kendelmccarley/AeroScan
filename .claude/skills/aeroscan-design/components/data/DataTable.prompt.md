One-sentence: the FlightBoard/DroneBoard telemetry list — charcoal header, transparent rows, Lato data cells, a sort caret on the active column and per-cell identity colors.

```jsx
<DataTable
  columns={[{key:"callsign",label:"Callsign"},{key:"alt",label:"Alt",align:"right"},{key:"dist",label:"Dist (nm)",align:"right"}]}
  rows={[{callsign:"SWA241",alt:"34000",dist:"12.4",_color:{callsign:"var(--air-swa)"}}]}
  sortKey="dist" sortDir="desc" selectedIndex={0}
  onSort={...} onSelectRow={...}
/>
```

Right-align numeric columns. Use `_color` for airline/drone identity. `emptyText` shows when rows is empty ("No drones detected").
