One-sentence: the settings pill switch — hollow charcoal when off, filled amber/teal/green with a soft glow when on, 150ms knob slide.

```jsx
<Toggle checked={dark} onChange={setDark} label="Dark Maps" />
<Toggle checked tone="green" />
```

Pair with MenuItem via its `trailing` slot for settings rows. Omit `label` to render just the switch.
