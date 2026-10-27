# Lower Zone

- Type: reference
- Audience: users
- Scope: lower-range container, tab navigation and collapse

## Overview

The Lower Zone is the area at the bottom of the main window. It hosts three views: the Mixer, the MIDI Editor (Piano Roll) and the Track Chain.

## Tab navigation

A vertical tab bar on the left side switches between the three views:

| Tab | View |
|---|---|
| Mixer | Channel strips for all audio tracks, plus the Master channel — see [Mixer](mixer.md) |
| MIDI Editor | Piano Roll for the selected MIDI clip — see [Piano Roll](../user/piano-roll.md). This tab is enabled only while a MIDI clip is selected. |
| Plugins | Track Chain for the selected track — see [Track Chain](track-chain.md) |

Clicking a tab selects its view; the active tab is highlighted.

While a Wave clip is being dragged, hovering a track header selects that track and switches the Lower Zone to its Track Chain. This automatic switch also occurs from the Mixer or MIDI Editor so a track-bound sample target can be revealed without ending the drag.

## Height and collapsed state

The splitter handle at the top edge can collapse the Lower Zone completely and expand it again. For the Mixer and Track Chain, the handle switches between the collapsed state and a fixed expanded height. The MIDI Editor is the only view whose expanded height can also be adjusted continuously by dragging the handle.

While collapsed, three compact buttons (`Mixer`, `MidiEditor`, `PluginChain`) remain available. Clicking one expands the Lower Zone and selects that view directly. The MIDI Editor button is enabled only while a MIDI clip is selected. The collapsed/expanded state is an application-wide preference and is restored in the next NextStudio session.

## Related documents

- [Mixer](mixer.md)
- [Piano Roll](../user/piano-roll.md)
- [Track Chain](track-chain.md)
- [Song Editor](song-editor.md)
- [Getting Started](../user/getting-started.md)
