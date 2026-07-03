One-sentence: the device's selectable text button — teal at rest, underline + "›" caret when focused, magenta when committed — plus filled/ghost variants for richer touch UIs.

```jsx
<Button variant="menu" selected>Radar Scope</Button>
<Button variant="solid" tone="amber">Tune</Button>
<Button variant="ghost" tone="teal">Back</Button>
```

Variants: `menu` (default, firmware text-button with caret), `solid` (filled instrument key, Neuropol X uppercase), `ghost` (hairline-outlined).
Props: `tone` teal|amber|magenta, `selected`, `committed` (momentary magenta flash), `disabled`, `icon`.
Use `menu` inside arc/list menus; `solid` for primary touch actions; reserve `committed` for the instant an action fires.
