export interface MessageBoxButton {
  label: string;
  tone?: "teal" | "amber" | "magenta";
  onClick?: () => void;
}

export interface MessageBoxProps {
  /** Neuropol X amber title */
  title?: React.ReactNode;
  /** Lato muted body copy */
  message?: React.ReactNode;
  buttons?: MessageBoxButton[];
  selectedIndex?: number;
  onSelect?: (index: number) => void;
  width?: number;
  style?: React.CSSProperties;
}

/**
 * @startingPoint section="Feedback" subtitle="Device modal dialog" viewport="700x320"
 */
export function MessageBox(props: MessageBoxProps): JSX.Element;
