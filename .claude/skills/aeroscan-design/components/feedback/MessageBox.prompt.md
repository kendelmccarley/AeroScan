One-sentence: the device modal — Neuropol X amber title, Lato muted body, and a vertical stack of selectable text buttons with the "›" caret on the focused one.

```jsx
<MessageBox title="Starting Emulator" message="Knob is start & power button is select."
  buttons={[{label:"Okay"},{label:"Cancel",tone:"magenta"}]} selectedIndex={0} onSelect={...} />
```

Render centered over the current screen's vignette. Built on Button (`variant="menu"`).
