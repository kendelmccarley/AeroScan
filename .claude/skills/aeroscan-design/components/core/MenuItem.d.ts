export interface MenuItemProps {
  label: string;
  /** optional secondary line under the label (Lato, muted) */
  secondLine?: React.ReactNode;
  /** optional leading icon node */
  icon?: React.ReactNode;
  /** focused row — brighter, amber, gains the "›" caret and left rule */
  selected?: boolean;
  disabled?: boolean;
  /** optional right-aligned node (value, chevron, Toggle) */
  trailing?: React.ReactNode;
  onClick?: () => void;
  style?: React.CSSProperties;
}

/**
 * @startingPoint section="Core" subtitle="Arc/list menu row with selection state" viewport="700x150"
 */
export function MenuItem(props: MenuItemProps): JSX.Element;
