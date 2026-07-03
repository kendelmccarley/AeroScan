export interface BadgeProps {
  children: React.ReactNode;
  /** semantic color of the capsule */
  tone?: "green" | "amber" | "teal" | "magenta" | "red" | "cyan" | "ble" | "wifi" | "muted";
  /** "solid" = filled signal; "outline" = quiet label */
  variant?: "solid" | "outline";
  /** leading status dot */
  dot?: boolean;
  /** add a soft colored glow */
  glow?: boolean;
  style?: React.CSSProperties;
}

export function Badge(props: BadgeProps): JSX.Element;
