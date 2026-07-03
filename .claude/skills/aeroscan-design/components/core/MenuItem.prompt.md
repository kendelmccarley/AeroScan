One-sentence: a menu/list row — leading icon, Neuropol X label, optional second line — that brightens to amber with a "›" caret and left rule when selected.

```jsx
<MenuItem label="Radar Scope" icon={<img src="..." />} selected />
<MenuItem label="WiFi" secondLine="HangarNet · −52 dBm" trailing={<Toggle checked />} />
```

Use inside arc menus, settings lists, preset lists. `trailing` takes a value, chevron, or Toggle. Unselected rows dim to ~0.7 opacity.
