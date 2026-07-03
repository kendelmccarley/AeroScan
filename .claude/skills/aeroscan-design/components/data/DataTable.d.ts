export interface DataColumn {
  key: string;
  label: string;
  align?: "left" | "center" | "right";
  width?: string | number;
}

export interface DataRow {
  [key: string]: React.ReactNode;
  /** optional per-cell color override, e.g. airline/drone identity: { callsign: "var(--air-swa)" } */
  _color?: { [colKey: string]: string };
}

export interface DataTableProps {
  columns: DataColumn[];
  rows: DataRow[];
  sortKey?: string | null;
  sortDir?: "asc" | "desc";
  selectedIndex?: number;
  onSort?: (key: string) => void;
  onSelectRow?: (index: number) => void;
  emptyText?: string;
  style?: React.CSSProperties;
}

/**
 * @startingPoint section="Data" subtitle="Telemetry list (FlightBoard / DroneBoard)" viewport="700x260"
 */
export function DataTable(props: DataTableProps): JSX.Element;
