#!/usr/bin/env python3
"""Selection routing on the private 1600x1000 X11/debug-shell fixture.

Reuses the maintained debug-shell bridge/session from native-cursor-regression.py.
No normal settings/project data are loaded. See docs/development/testing.md.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shutil
import time

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cursor_fixture", ROOT / "tools/native-cursor-regression.py")
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)


def run(args, output):
    checks = []
    with fixture.Session(args, output) as s:
        track = s.cmd("ensure-track", type="midi", name="Selection regression")["trackId"]
        clip = s.cmd("ensure-midi-clip", trackId=track, name="First", startSeconds=2, lengthSeconds=4)["clipId"]
        for pitch, beat, velocity in [(53, 1, 32), (55, 3, 80), (57, 5, 112)]:
            s.cmd("ensure-midi-note", clipId=clip, noteNumber=pitch, startBeats=beat, lengthBeats=1, velocity=velocity)
        s.cmd("select-track", trackId=track)
        s.x("mousemove", 766, 232, "click", "--repeat", 2, "--delay", 100, 1)
        time.sleep(0.5)
        before = s.model("before")

        def selection(name, notes=None, clips=None, automation=None):
            s.shell("state-dump", name + ".json")
            data = json.loads((output / (name + ".json")).read_text())["edit"]["selection"]
            if notes is not None:
                actual = sorted(n["noteNumber"] for n in data.get("selectedMidiNotes", []))
                if actual != sorted(notes):
                    raise AssertionError(f"{name}: expected MIDI pitches {notes}, got {actual}")
            if clips is not None and data["selectedClipCount"] != clips:
                raise AssertionError(f"{name}: expected {clips} clips, got {data['selectedClipCount']}")
            if automation is not None and data["selectedAutomationPointCount"] != automation:
                raise AssertionError(f"{name}: expected {automation} automation points, got {data['selectedAutomationPointCount']}")
            checks.append(name)
            print(name + ": PASS", flush=True)
            return data

        def drag(start, end):
            s.x("mousemove", *start, "mousedown", 1)
            s.x("mousemove", *end)

        def release():
            s.x("mouseup", 1)

        s.click(748, 624)  # Pointer: original down point must survive the drag threshold.
        drag((490, 808), (510, 842))
        selection("pointer-original-anchor", notes=[53])
        release()
        selection("pointer-release", notes=[53])

        s.click(951, 624)
        drag((480, 730), (1195, 870))
        selection("lasso-expanded", notes=[53, 55, 57])
        s.shell("screenshot 1600", "lasso-expanded.png")
        s.x("mousemove", 720, 870)
        selection("lasso-shrunk", notes=[53])
        release()
        selection("lasso-release", notes=[53])

        s.click(800, 782)  # Select G3 in the note grid.
        selection("grid-single", notes=[55])
        s.click(951, 624)
        s.x("keydown", "shift")
        drag((480, 808), (680, 855))
        selection("shift-add", notes=[53, 55])
        s.x("mousemove", 499, 855)
        selection("shift-shrink", notes=[55])
        release()
        s.x("keyup", "shift")

        s.click(951, 624)
        s.x("keydown", "ctrl")
        drag((720, 762), (950, 802))
        selection("ctrl-toggle", notes=[])
        release()
        s.x("keyup", "ctrl")

        s.click(800, 782)
        s.click(951, 624)
        drag((480, 730), (1195, 870))
        selection("before-escape", notes=[53, 55, 57])
        s.x("key", "Escape")
        selection("escape-restores", notes=[55])
        s.x("mousemove", 720, 870)
        release()
        selection("escape-late-events", notes=[55])
        s.shell("screenshot 1600", "escaped.png")
        # A new press on the same strategy must work without a toolbar reset.
        drag((480, 808), (680, 855))
        selection("midi-same-tool-new-press", notes=[53])
        release()
        selection("midi-same-tool-new-release", notes=[53])
        s.click(800, 782)

        s.click(829, 624)  # MIDI range remains a separate time/pitch selection.
        drag((480, 808), (680, 855))
        selection("midi-range", notes=[53])
        s.shell("screenshot 1600", "midi-range.png")
        s.x("key", "Escape")
        selection("midi-range-escape", notes=[55])
        release()
        selection("midi-range-late-release", notes=[55])

        s.click(951, 624)
        drag((480, 808), (680, 855))
        selection("midi-live-replace", notes=[53])
        s.x("keydown", "shift")
        selection("midi-live-shift", notes=[53, 55])
        s.x("keyup", "shift")
        selection("midi-live-shift-release", notes=[53])
        s.x("keydown", "ctrl")
        selection("midi-live-ctrl", notes=[53, 55])
        s.x("keyup", "ctrl")
        selection("midi-live-ctrl-release", notes=[53])
        s.x("key", "Escape")
        release()
        selection("midi-live-cancel", notes=[55])

        # Cancellation must restore the shared manager across editor boundaries.
        shared_before = selection("shared-note-original", notes=[55])
        s.click(878, 104)
        drag((740, 212), (840, 259))
        selection("shared-song-lasso-hits", notes=[], clips=1)
        s.x("key", "Escape")
        release()
        if selection("shared-note-song-lasso-cancel", notes=[55]) != shared_before:
            raise AssertionError("Song lasso cancellation lost shared selection")
        s.click(919, 104)
        drag((810, 212), (890, 259))
        s.x("key", "Escape")
        release()
        if selection("shared-note-song-range-cancel", notes=[55]) != shared_before:
            raise AssertionError("Song range cancellation lost shared selection")

        s.click(837, 104)
        s.click(770, 236)
        shared_before = selection("shared-clip-original", notes=[], clips=1)
        for tool_x, name in [(951, "lasso"), (829, "range")]:
            s.click(tool_x, 624)
            drag((480, 808), (680, 855))
            selection("shared-midi-" + name + "-hits", notes=[53], clips=0)
            s.x("key", "Escape")
            s.x("mousemove", 720, 870)
            release()
            if selection("shared-clip-midi-" + name + "-cancel", notes=[], clips=1) != shared_before:
                raise AssertionError("MIDI " + name + " cancellation lost shared selection")

        s.click(748, 624)
        drag((490, 808), (710, 855))
        selection("midi-new-press-after-cancel", notes=[53])
        selection("before-zoom", notes=[53])
        s.x("keydown", "ctrl", "click", "--repeat", 2, "--delay", 100, 5, "keyup", "ctrl")
        selection("standing-pointer-zoom", notes=[53])
        s.shell("screenshot 1600", "standing-pointer-zoom.png")
        release()
        if s.model("after-midi") != before:
            raise AssertionError("MIDI selection changed musical model")

        # Independent arrangement fixture; do not depend on a MIDI view's zoom.
        s.cmd("ensure-midi-clip", trackId=track, name="Second", startSeconds=8, lengthSeconds=2)
        song_before = s.model("song-before")
        s.click(878, 104)
        drag((740, 212), (920, 259))
        selection("song-lasso-expanded", clips=2)
        s.x("mousemove", 840, 259)
        selection("song-lasso-shrunk", clips=1)
        release()
        selection("song-lasso-release", clips=1)
        s.shell("screenshot 1600", "song-lasso.png")
        s.click(878, 104)
        s.x("keydown", "shift")
        drag((850, 212), (920, 259))
        selection("song-shift-add", clips=2)
        s.x("mousemove", 860, 259)
        selection("song-shift-shrink", clips=1)
        release()
        s.x("keyup", "shift")
        # Press/release modifiers only AFTER a drag, while the pointer stands.
        s.click(878, 104)
        drag((850, 212), (920, 259))
        selection("song-live-replace", clips=1)
        s.x("keydown", "shift")
        selection("song-live-shift", clips=2)
        s.x("keyup", "shift")
        selection("song-live-shift-release", clips=1)
        s.x("keydown", "ctrl")
        selection("song-live-ctrl", clips=2)
        s.x("keyup", "ctrl")
        selection("song-live-ctrl-release", clips=1)
        s.x("key", "Escape")
        release()
        selection("song-live-cancel", clips=1)
        s.click(878, 104)
        s.x("keydown", "ctrl")
        drag((740, 212), (840, 259))
        selection("song-ctrl-toggle", clips=0)
        release()
        s.x("keyup", "ctrl")
        s.click(770, 236)
        selection("song-grid-single", clips=1)
        s.click(878, 104)
        drag((740, 212), (920, 259))
        selection("song-before-escape", clips=2)
        s.x("key", "Escape")
        selection("song-escape", clips=1)
        s.x("mousemove", 840, 259)
        release()
        selection("song-escape-late-events", clips=1)
        s.click(919, 104)
        drag((810, 212), (890, 259))
        selection("song-time-range-not-object-selection", clips=0)
        s.shell("screenshot 1600", "song-range.png")
        s.x("key", "Escape")
        release()
        selection("song-range-escape", clips=1)
        if s.model("song-after") != song_before:
            raise AssertionError("Arrangement selection changed musical model")

        # Add a real volume lane through the production control's context menu.
        s.click(837, 104)
        s.x("mousemove", 677, 228, "click", 3)
        s.click(780, 325)  # Add automation lane (fresh default parameter has no curve).
        time.sleep(0.5)
        for x, y in [(780, 290), (850, 300), (920, 282)]:
            s.x("mousemove", x, y)
            s.x("click", "--repeat", 2, "--delay", 100, 1)
        s.shell("screenshot 1600", "automation-fixture.png")
        selection("automation-fixture-single", automation=1)

        def curve_model(name):
            s.shell("state-dump", name + ".json")
            data = json.loads((output / (name + ".json")).read_text())
            return [(t["id"], p["id"], a["id"], a.get("automationPoints", []))
                    for t in data["edit"]["tracks"] for p in t.get("plugins", []) for a in p.get("parameters", [])]

        curves_before = curve_model("curves-before")
        if sum(len(a[3]) for a in curves_before) != 4:
            raise AssertionError("Automation fixture must contain initial point plus three inserted points")
        s.click(878, 104)
        drag((770, 276), (930, 308))
        selection("automation-expanded", automation=3)
        s.shell("screenshot 1600", "automation-lasso.png")
        s.x("mousemove", 820, 308)
        selection("automation-shrunk", automation=1)
        release()
        selection("automation-release", automation=1)
        s.x("mousemove", 920, 282)
        s.x("click", 1)
        s.click(878, 104)
        s.x("keydown", "shift")
        drag((770, 282), (820, 308))
        selection("automation-shift-add", automation=2)
        s.x("mousemove", 775, 308)
        selection("automation-shift-shrink", automation=1)
        release()
        s.x("keyup", "shift")
        s.click(878, 104)
        drag((770, 282), (820, 308))
        selection("automation-live-replace", automation=1)
        s.x("keydown", "shift")
        selection("automation-live-shift", automation=2)
        s.x("keyup", "shift")
        selection("automation-live-shift-release", automation=1)
        s.x("keydown", "ctrl")
        selection("automation-live-ctrl", automation=2)
        s.x("keyup", "ctrl")
        selection("automation-live-ctrl-release", automation=1)
        s.x("key", "Escape")
        release()
        selection("automation-live-cancel", automation=1)
        s.click(878, 104)
        s.x("keydown", "ctrl")
        drag((910, 270), (935, 295))
        selection("automation-ctrl-toggle", automation=0)
        release()
        s.x("keyup", "ctrl")
        s.x("mousemove", 920, 282)
        s.x("click", 1)
        s.click(878, 104)
        drag((770, 276), (930, 308))
        selection("automation-before-escape", automation=3)
        s.x("key", "Escape")
        selection("automation-escape", automation=1)
        s.x("mousemove", 820, 308)
        release()
        selection("automation-escape-late-events", automation=1)
        # MIDI cancellation must also retain disposable automation proxies even
        # after the lane's asynchronous unselected-proxy cleanup has run.
        shared_before = selection("shared-automation-original", automation=1)
        s.click(951, 624)
        drag((480, 730), (1195, 870))
        selection("shared-midi-replaces-automation", automation=0)
        s.x("key", "Escape")
        s.x("mousemove", 720, 870)
        release()
        if selection("shared-automation-midi-cancel", automation=1) != shared_before:
            raise AssertionError("MIDI cancellation lost automation selection identity")
        if curve_model("curves-after") != curves_before:
            raise AssertionError("Automation selection mutated curve points")

        (output / "results.json").write_text(json.dumps({
            "binary": str(args.binary), "binarySha256": fixture.file_hash(args.binary),
            "fixture": "private Linux/X11 1600x1000, 100% application scale",
            "checks": checks, "untested": ["native non-X11 platforms", "higher UI/display scales", "velocity lasso (#84 follow-up)"],
        }, indent=2) + "\n")
    print(f"PASS: {len(checks)} checks; evidence in {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--display", default=os.environ.get("DISPLAY", ""))
    parser.add_argument("--delay", type=float, default=0.2)
    args = parser.parse_args()
    if not args.display or not shutil.which("xdotool"):
        parser.error("Use a private Xvfb display with xdotool installed")
    args.output.mkdir(parents=True, exist_ok=False)
    run(args, args.output)


if __name__ == "__main__":
    main()
