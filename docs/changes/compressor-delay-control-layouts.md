# Compressor and Delay control layouts

## Problem

The Compressor and Next Delay editors requested the same rack width as simpler effects while placing a large graph above several rows of parameter controls. `AutomatableParameterComponent` reserves 20 pixels for its title and 15 pixels for its value. At the normal Track Chain height, this left some rotary sliders only 14–16 pixels high. The rotary renderer then had almost no drawable radius, making controls difficult to read and operate.

## Layout policy

Both dense editors now request rack width factor 3. Their graphs are placed beside the controls so the full editor height is available to the parameter rows.

The Compressor uses:

- a transfer graph on the left;
- two rows of three knobs on the right;
- a full-width sidechain source and trigger footer.

The Next Delay uses:

- a delay-space graph on the left;
- one weighted row for Mode, Sync, and Division choices, with extra width reserved for Mode;
- one row for Time, Feedback, and Mix;
- one row for Offset, PingPong, HP, and LP.

The legacy Tracktion Delay retains its existing compact editor and width. The Next Delay graph header now contains only `DELAY SPACE`; mode and timing values remain visible in their controls and are not duplicated in the narrower graph header.

## Responsive calculation

`EffectEditorLayout.h` owns the pure integer rectangle calculations. The Compressor graph receives 42% of the available width, clamped between 140 and 210 pixels while preserving control space. The denser Delay reserves 250 pixels for controls before assigning width to its graph and keeps the graph at least 150 pixels wide when the editor permits it. Its Mode, Sync, and Division cells receive 42%, 28%, and 30% of a compact 64–68 pixel choice row. The remaining height is divided equally between the two knob rows, removing unused vertical space and increasing the knob size. Choice boxes use their complete cell width so all values remain visible. Cell insets and integer remainder distribution keep all rectangles bounded and non-overlapping.

This changes presentation only. Existing Tracktion parameters, automation, MIDI Learn, sidechain routing, presets, and undo behavior are unchanged.

## Regression coverage

`EffectEditorLayoutTests` checks narrow, default, and wide editor sizes. It verifies bounded graph widths, in-bounds rectangles, non-overlapping graph/control/footer regions, separate minimum heights for choice and knob rows, and sufficient widths for all three Delay choice fields.
