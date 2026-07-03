export interface StatusBarProps {
  /** path to the /assets/icons folder from the consuming page */
  iconBase?: string;
  battery?: "full" | "mid" | "charging" | "unknown";
  wifi?: "conn_4bar" | "conn_2bar" | "connecting" | "disconnected" | "off";
  gps?: "okay" | "nolock" | "off";
  adsb?: "on" | "off";
  /** optional clock block: a string or { day, date, clock } */
  time?: string | { day: string; date: string; clock: string } | null;
  orientation?: "vertical" | "horizontal";
  style?: React.CSSProperties;
}

export function StatusBar(props: StatusBarProps): JSX.Element;
