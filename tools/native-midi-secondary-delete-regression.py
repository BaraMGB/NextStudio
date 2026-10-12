#!/usr/bin/env python3
"""Pointer/Draw secondary-click MIDI deletion in an isolated X11 sandbox."""

import argparse
import importlib.util
import json
import math
import os
from pathlib import Path
import shutil
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cursor_fixture", ROOT / "tools/native-cursor-regression.py")
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)


def run(args, output):
    checks = []
    with fixture.Session(args, output) as session:
        track_id = session.cmd("ensure-track", type="midi", name="Secondary-click deletion")["trackId"]
        clip_id = session.cmd("ensure-midi-clip", trackId=track_id, name="Notes", startSeconds=2, lengthSeconds=4)["clipId"]
        for pitch, beat in ((53, 1), (55, 3), (57, 5)):
            session.cmd("ensure-midi-note", clipId=clip_id, noteNumber=pitch,
                        startBeats=beat, lengthBeats=1, velocity=96)
        session.cmd("select-track", trackId=track_id)
        session.x("mousemove", 766, 232, "click", "--repeat", 2, "--delay", 100, 1)
        time.sleep(0.5)

        def state(name):
            session.shell("state-dump", name + ".json")
            return json.loads((output / (name + ".json")).read_text())["edit"]

        def pitches(edit):
            for track in edit["tracks"]:
                if track["id"] == track_id:
                    return sorted(note["noteNumber"] for clip in track.get("clips", [])
                                  for note in clip.get("notes", []))
            raise AssertionError("Fixture track disappeared")

        def selected(edit):
            return sorted(note["noteNumber"] for note in edit["selection"].get("selectedMidiNotes", []))

        def check(name, expected_pitches, expected_selection):
            edit = state(name)
            actual_pitches = pitches(edit)
            actual_selection = selected(edit)
            if actual_pitches != sorted(expected_pitches) or actual_selection != sorted(expected_selection):
                raise AssertionError(
                    f"{name}: pitches {actual_pitches}, selection {actual_selection}; "
                    f"expected {sorted(expected_pitches)}, {sorted(expected_selection)}")
            checks.append(name)
            print(name + ": PASS", flush=True)

        # Pointer: retain another selected note, delete only the hit, and keep
        # one distinct undo/redo step.
        session.click(748, 624)
        session.click(502, 822)
        session.x("keydown", "shift", "mousemove", 800, 782, "click", 1, "keyup", "shift")
        check("pointer-selection-before", [53, 55, 57], [53, 55])
        session.x("mousemove", 800, 782, "click", 3)
        check("pointer-delete", [53, 57], [53])
        session.x("key", "ctrl+z")
        check("pointer-undo", [53, 55, 57], [53])
        session.x("key", "ctrl+shift+z")
        check("pointer-redo", [53, 57], [53])
        session.x("key", "ctrl+z")
        check("pointer-restore", [53, 55, 57], [53])

        # Empty-space secondary clicks must not invoke Pointer's deselection.
        session.click(800, 782)
        session.x("mousemove", 650, 700, "click", 3)
        check("pointer-empty-noop", [53, 55, 57], [55])
        session.click(502, 822)
        check("pointer-primary-after-secondary", [53, 55, 57], [53])

        # Draw receives the same hit deletion without beginning note creation.
        session.click(788, 624)
        session.x("mousemove", 1046, 742, "click", 3)
        check("draw-delete", [53, 55], [53])
        session.x("key", "ctrl+z")
        check("draw-undo", [53, 55, 57], [53])
        session.x("mousemove", 650, 700, "click", 3)
        check("draw-empty-noop", [53, 55, 57], [53])
        session.shell("screenshot 1600", "final.png")

    return checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--display", default=os.environ.get("DISPLAY"))
    parser.add_argument("--output", type=Path, required=True, help="New directory for retained results")
    parser.add_argument("--delay", type=float, default=0.18)
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    if sys.platform != "linux" or not args.display or not args.binary.is_file() or not math.isfinite(args.delay) or args.delay <= 0:
        parser.error("Requires Linux, a private DISPLAY, an existing binary and positive --delay")
    for command in ("node", "xdotool"):
        if not shutil.which(command):
            parser.error(f"Missing prerequisite: {command}")
    os.environ["DISPLAY"] = args.display
    args.output = args.output.resolve()
    try:
        args.output.mkdir(parents=True, exist_ok=False)
    except OSError as error:
        parser.error(f"Output directory must be new and writable: {error}")

    results = {
        "binary": str(args.binary),
        "binarySha256": fixture.file_hash(args.binary),
        "display": args.display,
        "fixture": "Linux X11 1600x1000, 100% sandbox application scale",
    }
    try:
        results["checks"] = run(args, args.output)
        results["passed"] = True
    except Exception as error:
        results["passed"] = False
        results["error"] = str(error)
    (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    print(f"{'PASS' if results['passed'] else 'FAIL'}: results in {args.output}")
    if "error" in results:
        print(results["error"], file=sys.stderr)
    return 0 if results["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
