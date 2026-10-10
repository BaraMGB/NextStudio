#!/usr/bin/env python3
"""Piano-key pitch selection after object/range selection, on private X11."""
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
        track = s.cmd("ensure-track", type="midi", name="Keyboard selection")['trackId']
        first = s.cmd("ensure-midi-clip", trackId=track, name="First", startSeconds=2, lengthSeconds=4)['clipId']
        for pitch, beat in [(53, 1), (53, 2), (55, 3), (55, 4), (57, 5)]:
            s.cmd("ensure-midi-note", clipId=first, noteNumber=pitch, startBeats=beat, lengthBeats=0.5, velocity=80)
        s.cmd("select-track", trackId=track)
        s.x("mousemove", 766, 232, "click", "--repeat", 2, "--delay", 100, 1)
        time.sleep(0.5)
        second = s.cmd("ensure-midi-clip", trackId=track, name="Sibling", startSeconds=8, lengthSeconds=2)['clipId']
        s.cmd("ensure-midi-note", clipId=second, noteNumber=53, startBeats=1, lengthBeats=0.5, velocity=80)
        before = s.model("before")
        f_notes = [(first, 53, 1), (first, 53, 2)]
        g_notes = [(first, 55, 3), (first, 55, 4)]
        a_notes = [(first, 57, 5)]
        sibling = [(second, 53, 1)]

        def selection(name, notes, clips=None):
            s.shell("state-dump", name + ".json")
            data = json.loads((output / (name + ".json")).read_text())["edit"]["selection"]
            actual = sorted((n['clipId'], n['noteNumber'], n['startBeats']) for n in data.get('selectedMidiNotes', []))
            if actual != sorted(notes):
                raise AssertionError(f"{name}: expected {notes}, got {actual}")
            if clips is not None and data['selectedClipCount'] != clips:
                raise AssertionError(f"{name}: expected {clips} shared clips, got {data['selectedClipCount']}")
            checks.append(name)
            print(name + ": PASS", flush=True)

        def drag(start, end):
            s.x("mousemove", *start, "mousedown", 1)
            s.x("mousemove", *end)
            s.x("mouseup", 1)

        s.click(205, 822)
        selection("keyboard-initial-pitch", f_notes, clips=1)
        s.click(951, 624)
        drag((1000, 730), (1195, 760))
        selection("keyboard-grid-lasso", a_notes, clips=0)
        s.click(205, 822)
        selection("keyboard-after-grid-lasso", f_notes, clips=0)
        s.click(205, 782)
        selection("keyboard-repeat-other-pitch", g_notes, clips=0)
        s.x("keydown", "shift")
        s.click(205, 822)
        s.x("keyup", "shift")
        selection("keyboard-shift-add-pitch", f_notes + g_notes, clips=0)
        s.x("keydown", "shift")
        s.click(205, 822)
        s.x("keyup", "shift")
        selection("keyboard-shift-toggle-pitch", g_notes, clips=0)

        s.click(480, 750)  # Empty grid click clears note membership, not target context.
        selection("keyboard-empty-membership", [], clips=0)
        s.click(205, 822)
        selection("keyboard-after-empty-selection", f_notes, clips=0)
        s.click(951, 624)
        drag((1000, 890), (1195, 913))
        selection("keyboard-velocity-lasso", a_notes, clips=0)
        s.click(205, 822)
        selection("keyboard-after-velocity-lasso", f_notes, clips=0)
        s.click(829, 624)
        drag((1000, 730), (1195, 760))
        selection("keyboard-range", a_notes, clips=0)
        s.click(205, 822)
        selection("keyboard-after-range", f_notes, clips=0)
        s.click(205, 800)  # F# has no notes in the clip.
        selection("keyboard-unmatched-pitch", [], clips=0)
        s.click(205, 822)
        selection("keyboard-after-unmatched-pitch", f_notes, clips=0)

        # Explicit sibling selection replaces the keyboard target, not all track clips.
        s.click(837, 104)
        s.click(890, 232)
        s.click(205, 822)
        selection("keyboard-new-explicit-clip", sibling, clips=1)
        s.x("keydown", "ctrl")
        s.click(770, 232)
        s.x("keyup", "ctrl")
        s.click(205, 822)
        selection("keyboard-multiple-explicit-clips", f_notes + sibling, clips=2)
        s.click(951, 624)
        drag((1000, 730), (1195, 760))
        selection("keyboard-multi-clip-lasso", a_notes, clips=0)
        s.click(205, 822)
        selection("keyboard-multi-clip-scope-retained", f_notes + sibling, clips=0)
        s.shell("screenshot 1600", "keyboard-after-lasso.png")
        if s.model("after") != before:
            raise AssertionError("Keyboard/rectangle selection changed musical records")
        (output / "results.json").write_text(json.dumps({
            "binary": str(args.binary), "binarySha256": fixture.file_hash(args.binary),
            "fixture": "private Linux/X11 1600x1000, 100% scale", "checks": checks,
            "untested": ["physical displays", "fractional scaling", "Windows/macOS", "native clip deletion/track teardown"],
        }, indent=2) + "\n")
    print(f"PASS: {len(checks)} checks; evidence in {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--display", default=os.environ.get("DISPLAY", ""))
    parser.add_argument("--delay", type=float, default=0.35)
    args = parser.parse_args()
    if not args.display or not shutil.which("xdotool"):
        parser.error("Use a private Xvfb display with xdotool installed")
    args.output.mkdir(parents=True, exist_ok=False)
    run(args, args.output)


if __name__ == "__main__":
    main()
