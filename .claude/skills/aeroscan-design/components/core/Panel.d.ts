export interface PanelProps {
  /** optional title bar text (Neuropol X, amber, uppercase) */
  title?: React.ReactNode;
  /** optional node aligned right in the title bar */
  trailing?: React.ReactNode;
  children: React.ReactNode;
  /** use the green scope outline + glow instead of charcoal hairline */
  scope?: boolean;
  /** body padding (default true) */
  padded?: boolean;
  style?: React.CSSProperties;
}

/**
 * @startingPoint section="Core" subtitle="Instrument container surface" viewport="700x200"
 */
export function Panel(props: PanelProps): JSX.Element;
