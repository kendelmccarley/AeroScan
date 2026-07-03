export interface ToggleProps {
  checked?: boolean;
  disabled?: boolean;
  onChange?: (next: boolean) => void;
  /** optional trailing label (Lato) */
  label?: React.ReactNode;
  /** on-state track color */
  tone?: "amber" | "teal" | "green";
  style?: React.CSSProperties;
}

export function Toggle(props: ToggleProps): JSX.Element;
