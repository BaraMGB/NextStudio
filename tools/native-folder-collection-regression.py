#!/usr/bin/env python3
"""Folder CollectionClip input regression on a private 1600x1000 X11 display.

Uses an isolated debug-shell session; creates folder hierarchy via real header
controls and drags. Requires the native-cursor-regression.py prerequisites.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cursor_fixture", ROOT / "tools/native-cursor-regression.py")
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)


def capture_appearance(s):
    """Same model and viewport in both built-in themes, via real Settings controls."""
    def theme(light):
        s.click(44, 375)  # Settings
        s.click(295, 54)  # General
        s.click(300 if light else 165, 284)  # built-in Light / Dark
        s.click(44, 45)  # Projects

    for name, light in [("dark", False), ("light", True)]:
        theme(light)
        s.click(520, 225)  # track-only selection
        s.click(430, 225)  # expand
        s.x("mousemove", 1300, 420)
        s.shell("screenshot 1600", f"appearance-{name}-expanded.png")
        s.click(430, 225)  # collapse
        s.x("mousemove", 1300, 420)
        s.shell("screenshot 1600", f"appearance-{name}-collapsed.png")
        s.x("mousemove", 790, 225)
        s.shell("screenshot 1600", f"appearance-{name}-hover.png")
        s.click(790, 225)
        s.x("mousemove", 1300, 420)
        s.shell("screenshot 1600", f"appearance-{name}-selected.png")
        s.x("mousemove", 790, 225, "mousedown", 1)
        s.x("mousemove", 910, 225)
        s.shell("screenshot 1600", f"appearance-{name}-drag.png")
        s.x("key", "Escape")
        s.x("mouseup", 1)
    theme(False)
    s.click(520, 225)


def run(args):
    checks = []
    with fixture.Session(args, args.output) as s:
        window = subprocess.check_output(["xdotool", "search", "--onlyvisible", "--name", "NextStudio"], text=True).splitlines()[0]
        s.x("windowfocus", window)
        tracks = {}
        for name, start in [("A", 2), ("B", 4)]:
            track = s.cmd("ensure-track", type="midi", name=name)["trackId"]
            tracks[name] = track
            clip = s.cmd("ensure-midi-clip", trackId=track, name=name, startSeconds=start, lengthSeconds=4)["clipId"]
            s.cmd("ensure-midi-note", clipId=clip, noteNumber=60, startBeats=1, lengthBeats=1, velocity=80)

        serial = 0
        def state():
            nonlocal serial
            serial += 1
            name = f"state-{serial:02d}.json"
            s.shell("state-dump", name)
            return json.loads((args.output / name).read_text())["edit"]

        def starts():
            data = state()
            return {name: sorted(c["startSeconds"] for t in data["tracks"] if t["id"] == track for c in t["clips"])
                    for name, track in tracks.items()}

        def check(name, expected):
            actual = starts()
            passed = actual == expected
            checks.append({"name": name, "passed": passed, "expected": expected, "actual": actual})
            (args.output / "results.json").write_text(json.dumps({
                "checks": checks, "binarySha256": fixture.file_hash(args.binary),
                "fixture": "private X11 1600x1000, collapsed two-track MIDI folder",
                "untested": "physical displays, audio playback, Windows/macOS, fractional scaling",
            }, indent=2) + "\n")
            print(f"{name}: {'PASS' if passed else 'FAIL'} {actual}", flush=True)
            if not passed:
                raise AssertionError(name)

        def drag(start, end, modifier=None):
            if modifier: s.x("keydown", modifier)
            s.x("mousemove", *start, "mousedown", 1)
            for step in range(1, 5):
                s.x("mousemove", round(start[0] + (end[0] - start[0]) * step / 4),
                    round(start[1] + (end[1] - start[1]) * step / 4))
            s.x("mouseup", 1)
            if modifier: s.x("keyup", modifier)

        s.click(675, 184)  # append folder
        # Move both tracks as one selected block. Moving them separately changes
        # row coordinates after the first drop and can accidentally pull it back out.
        s.click(520, 235)
        s.x("keydown", "ctrl")
        s.click(520, 280)
        s.x("keyup", "ctrl")
        drag((520, 280), (520, 325))
        s.shell("screenshot 1600", "expanded.png")
        s.click(430, 225)  # collapse folder; the only visible content is virtual
        s.shell("screenshot 1600", "collapsed.png")
        if args.appearance:
            capture_appearance(s)
        initial = {"A": [2.0], "B": [4.0]}
        check("collapse-preserves-content", initial)
        s.click(790, 225)
        s.x("mousemove", 790, 225, "mousedown", 1)
        s.x("mousemove", 910, 225)
        check("move-preview-is-not-model-edit", initial)
        s.shell("screenshot 1600", "move-preview.png")
        s.x("mouseup", 1)
        moved = {"A": [8.0], "B": [10.0]}
        check("collapsed-folder-move", moved)
        s.click(1495, 40)
        check("move-single-undo", initial)
        s.click(1560, 40)
        check("move-single-redo", moved)
        s.click(910, 225)
        s.x("mousemove", 910, 225, "mousedown", 1)
        s.x("mousemove", 950, 225)
        s.x("key", "Escape")
        s.x("mousemove", 990, 225)
        s.x("mouseup", 1)
        check("escape-and-late-release", moved)
        drag((910, 225), (1030, 225), "ctrl")
        copied = {"A": [8.0, 14.0], "B": [10.0, 16.0]}
        check("collapsed-folder-copy", copied)
        s.click(1495, 40)
        check("copy-single-undo", moved)
        s.click(1560, 40)
        check("copy-single-redo", copied)
        s.click(1030, 225)
        s.x("key", "ctrl+d")
        duplicated = {"A": [8.0, 14.0, 20.0], "B": [10.0, 16.0, 22.0]}
        check("collection-keyboard-duplicate", duplicated)
        s.click(1150, 225)
        s.x("key", "Delete")
        check("collection-keyboard-delete", copied)
        s.click(1495, 40)
        check("delete-single-undo", duplicated)
        s.click(1560, 40)
        check("delete-single-redo", copied)
        # A folder lasso resolves to unique real clips, even while collapsed.
        s.click(878, 104)
        drag((995, 212), (1105, 239))
        s.x("key", "Delete")
        check("collapsed-folder-lasso-delete", moved)
        s.click(1495, 40)
        check("lasso-delete-single-undo", copied)
        s.click(1030, 225)
        s.x("key", "ctrl+a")
        drag((1030, 225), (1150, 225))
        check("expanded-multiselection-move", {"A": [14.0, 20.0], "B": [16.0, 22.0]})
        s.click(1495, 40)
        check("expanded-multiselection-undo", copied)
        s.click(1030, 225)
        s.x("key", "ctrl+a")
        s.x("keydown", "ctrl")
        s.click(1030, 225)
        s.x("keyup", "ctrl")
        s.x("key", "Delete")
        check("toggle-expanded-collection", {"A": [14.0], "B": [16.0]})
        s.click(1495, 40)
        check("toggle-delete-undo", copied)
        s.click(960, 104)  # Knife
        s.x("keydown", "shift")  # bypass snapping and split both overlapping clips at 11s
        s.click(930, 225)
        s.x("keyup", "shift")
        split = {"A": [8.0, 11.0, 14.0], "B": [10.0, 11.0, 16.0]}
        check("collapsed-folder-knife", split)
        s.click(1495, 40)
        check("knife-single-undo", copied)
        s.click(1560, 40)
        check("knife-single-redo", split)
        s.shell("screenshot 1600", "final.png")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--display", default=os.environ.get("DISPLAY", ""))
    parser.add_argument("--delay", type=float, default=0.2)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--appearance", action="store_true", help="Also capture expanded/collapsed/hover/selection/drag in Dark and Light")
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    os.environ["DISPLAY"] = args.display
    raise SystemExit(run(args))


if __name__ == "__main__":
    main()
