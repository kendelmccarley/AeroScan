export interface ButtonProps {
  children: React.ReactNode;
  /** "menu" = device text-button with selection caret (default); "solid" = filled key; "ghost" = outlined */
  variant?: "menu" | "solid" | "ghost";
  /** label/key color at rest */
  tone?: "teal" | "amber" | "magenta";
  /** focused/highlighted state — adds underline (menu) or glow (solid) and the ">" caret */
  selected?: boolean;
  /** momentary committed state — flashes magenta (the device's "selected & firing" color) */
  committed?: boolean;
  disabled?: boolean;
  /** optional leading icon node */
  icon?: React.ReactNode;
  onClick?: () => void;
  style?: React.CSSProperties;
}

/**
 * @startingPoint section="Core" subtitle="Selectable instrument button with caret" viewport="700x150"
 */
export function Button(props: ButtonProps): JSX.Element;
