export interface CheckboxProps {
  checked?: boolean;
  /** tri-state mixed mark (firmware ships a checkbox_indeterminate glyph) */
  indeterminate?: boolean;
  disabled?: boolean;
  onChange?: (next: boolean) => void;
  /** optional trailing label (Lato) */
  label?: React.ReactNode;
  style?: React.CSSProperties;
}

export function Checkbox(props: CheckboxProps): JSX.Element;
