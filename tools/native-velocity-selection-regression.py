#!/usr/bin/env python3
"""#84 velocity source: real X11 mouse routing in an isolated 1600x1000 sandbox."""
import argparse
import copy
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
        track = s.cmd("ensure-track", type="midi", name="Velocity selection")['trackId']
        clip = s.cmd("ensure-midi-clip", trackId=track, name="Velocity", startSeconds=2, lengthSeconds=4)['clipId']
        for pitch, beat, velocity in [(53, 1, 32), (55, 3, 80), (57, 5, 112)]:
            s.cmd("ensure-midi-note", clipId=clip, noteNumber=pitch, startBeats=beat, lengthBeats=1, velocity=velocity)
        s.cmd("select-track", trackId=track)
        s.x("mousemove", 766, 232, "click", "--repeat", 2, "--delay", 100, 1)
        time.sleep(0.5)
        before = s.model("before")

        def selection(name, notes):
            s.shell("state-dump", name + ".json")
            data = json.loads((output / (name + ".json")).read_text())["edit"]["selection"]
            actual = sorted(n["noteNumber"] for n in data.get("selectedMidiNotes", []))
            if actual != sorted(notes):
                raise AssertionError(f"{name}: expected {notes}, got {actual}")
            checks.append(name)
            print(name + ": PASS", flush=True)
            return data

        def drag(start, end):
            s.x("mousemove", *start, "mousedown", 1)
            s.x("mousemove", *end)

        def release():
            s.x("mouseup", 1)

        s.click(951, 624)
        drag((480, 878), (1065, 955))
        selection("velocity-expanded", [53, 55, 57])
        s.shell("screenshot 1600", "velocity-expanded.png")
        s.x("mousemove", 720, 955)
        selection("velocity-shrunk", [53])
        release()
        selection("velocity-release", [53])
        s.x("mousemove", 1200, 955)
        s.shell("screenshot 1600", "velocity-selected-head.png")

        s.click(800, 782)  # Existing grid-selected note is the snapshot.
        selection("grid-original", [55])
        s.click(951, 624)
        drag((480, 918), (680, 950))
        selection("velocity-live-replace", [53])
        s.x("keydown", "shift")
        selection("velocity-live-shift", [53, 55])
        s.x("mousemove", 499, 950)
        selection("velocity-shift-shrink", [55])
        s.x("mousemove", 680, 950, "keyup", "shift")
        selection("velocity-shift-release", [53])
        s.x("keydown", "ctrl")
        selection("velocity-live-ctrl", [53, 55])
        s.x("keyup", "ctrl")
        selection("velocity-ctrl-release", [53])
        s.x("key", "Escape")
        selection("velocity-escape", [55])
        s.x("mousemove", 1065, 955)
        release()
        selection("velocity-late-release", [55])

        # Cancellation must not suppress a new press, even on the same Lasso tool.
        drag((720, 890), (850, 913))
        s.x("keydown", "ctrl")
        selection("velocity-toggle-selected", [])
        s.x("keyup", "ctrl")
        selection("velocity-toggle-release", [55])
        release()

        # Pointer empty space uses the same policy without switching strategies.
        drag((1065, 945), (480, 878))
        selection("velocity-pointer-reverse", [53, 55, 57])
        release()
        s.click(951, 624)
        drag((720, 890), (850, 913))
        release()
        original = selection("velocity-shared-original", [55])
        drag((480, 878), (1065, 955))
        s.x("key", "Escape")
        release()
        if selection("velocity-shared-restore", [55]) != original:
            raise AssertionError("Cancellation changed shared membership")

        s.click(951, 624)
        drag((720, 918), (850, 945))
        selection("velocity-stem-only", [])
        s.x("key", "Escape")
        release()
        selection("velocity-stem-cancel", [55])
        # Explicit Lasso over a marker must select, not edit its velocity.
        drag((774, 902), (850, 913))
        selection("velocity-explicit-marker", [55])
        release()

        # Stationary-pointer geometry replay, unsnapped original beat anchor.
        drag((480, 918), (680, 950))
        selection("velocity-before-zoom", [53])
        s.x("keydown", "ctrl", "click", "--repeat", 2, "--delay", 100, 5, "keyup", "ctrl")
        selection("velocity-standing-zoom", [53])
        s.x("key", "Escape")
        release()
        if s.model("after-selection") != before:
            raise AssertionError("Selection changed musical model")

        # Reopen the fixture to reset horizontal zoom before velocity editing.
        s.cmd("select-track", trackId=track)
        # Zoom back using the inverse wheel operation in the same lane.
        s.x("mousemove", 680, 940, "keydown", "ctrl", "click", "--repeat", 2, "--delay", 100, 4, "keyup", "ctrl")
        s.click(951, 624)
        drag((480, 878), (1065, 955))
        release()
        selection("velocity-group-before-edit", [53, 55, 57])
        drag((774, 902), (774, 892))
        release()
        after = s.model("after-group-edit")
        velocities = [n["velocity"] for _, clips in after for c in clips for n in c.get("notes", [])]
        if sorted(velocities) != [42, 90, 122]:
            raise AssertionError(f"Group velocity drag: {velocities}")
        expected = copy.deepcopy(before)
        for _, clips in expected:
            for c in clips:
                for n in c.get("notes", []):
                    n["velocity"] += 10
        if after != expected:
            raise AssertionError("Group velocity drag changed unrelated musical fields")
        checks.append("velocity-group-edit")
        print("velocity-group-edit: PASS", flush=True)

        s.click(951, 624)
        drag((480, 918), (680, 945))
        release()
        selection("velocity-single-selection", [53])
        drag((774, 896), (774, 886))  # Unselected middle head: single-note edit only.
        release()
        selection("velocity-unselected-drag-membership", [53])
        for _, clips in expected:
            for c in clips:
                for n in c.get("notes", []):
                    if n["noteNumber"] == 55:
                        n["velocity"] += 10
        if s.model("after-single-edit") != expected:
            raise AssertionError("Unselected marker drag did not preserve single-note behavior")
        checks.append("velocity-unselected-single-edit")
        print("velocity-unselected-single-edit: PASS", flush=True)
        # Property text must finish on the old selection before a lane press
        # clears membership or captures a marker's starting velocity.
        def type_velocity(text):
            s.x("mousemove", 730, 665, "click", "--repeat", 2, "--delay", 100, 1)
            s.x("key", "ctrl+a")
            s.x("type", "--clearmodifiers", text)

        def with_middle_velocity(model, velocity):
            result = copy.deepcopy(model)
            for _, clips in result:
                for c in clips:
                    for note in c.get("notes", []):
                        if note["noteNumber"] == 55:
                            note["velocity"] = velocity
            return result

        def musical_model(name, expected_model):
            if s.model(name) != expected_model:
                raise AssertionError(f"{name}: unexpected musical records")
            checks.append(name)
            print(name + ": PASS", flush=True)

        s.click(800, 782)
        selection("property-pointer-original", [55])
        type_velocity("99")  # Deliberately no Enter/Tab.
        s.x("mousemove", 480, 878, "mousedown", 1)
        committed = with_middle_velocity(expected, 99)
        musical_model("property-pointer-focus-commit", committed)
        selection("property-pointer-empty-down", [])
        s.x("mousemove", 1065, 955)
        release()
        musical_model("property-pointer-late-callback", committed)
        selection("property-pointer-release", [53, 55, 57])
        s.x("key", "ctrl+z")
        musical_model("property-pointer-one-undo", expected)
        s.x("key", "ctrl+shift+z")
        musical_model("property-pointer-redo", committed)

        s.click(480, 750)  # Clear the selected group before choosing one note.
        s.click(800, 782)
        selection("property-lasso-original", [55])
        s.click(951, 624)  # Set Lasso before opening the text editor.
        type_velocity("90")
        drag((480, 878), (1065, 955))
        committed = with_middle_velocity(committed, 90)
        musical_model("property-lasso-focus-commit", committed)
        s.x("key", "Escape")
        release()
        selection("property-lasso-cancel-original", [55])
        musical_model("property-lasso-cancel-retains-edit", committed)

        type_velocity("invalid")
        drag((480, 878), (1065, 955))
        musical_model("property-invalid-focus-reject", committed)
        s.x("key", "Escape")
        release()
        selection("property-invalid-cancel-original", [55])

        type_velocity("77")
        s.x("key", "Escape")  # Escape in the field, not in a lane gesture.
        drag((480, 878), (1065, 955))
        musical_model("property-text-escape-discards", committed)
        s.x("key", "Escape")
        release()
        selection("property-text-escape-original", [55])

        s.click(749, 624)  # Pointer: a marker drag must use the committed value.
        type_velocity("80")
        s.x("mousemove", 774, 902, "mousedown", 1)  # Head at its new velocity.
        musical_model("property-marker-focus-commit", with_middle_velocity(committed, 80))
        s.x("mousemove", 774, 892)
        release()
        musical_model("property-marker-drag-origin", committed)  # 80 + 10, not 90 + 10.
        selection("property-marker-drag-membership", [55])

        (output / "results.json").write_text(json.dumps({
            "binary": str(args.binary), "binarySha256": fixture.file_hash(args.binary),
            "fixture": "private Linux/X11 1600x1000, 100% scale", "checks": checks,
            "untested": ["physical displays", "fractional scaling", "Windows/macOS", "multi-clip/source deletion"],
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
