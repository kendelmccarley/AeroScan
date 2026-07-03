One-sentence: the device status rail (battery · wifi · gps · adsb) drawn from the firmware's own monochrome PNG glyphs recolored via CSS mask — GPS greens on lock, reds on no-lock.

```jsx
<StatusBar iconBase="../../assets/icons" gps="okay" adsb="on" wifi="conn_4bar" battery="full"
  time={{day:"SAT",date:"MAR 14",clock:"21:23"}} />
```

`iconBase` must point at /assets/icons from the consuming page. `orientation="vertical"` for the 160px side column, `"horizontal"` for a compact header.
