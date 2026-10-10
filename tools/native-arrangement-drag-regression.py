#!/usr/bin/env python3
"""Verify Song Editor move/copy commits, not just drag previews or selection.

Uses the private 1600x1000 X11 session from native-cursor-regression.py.
--existing-overlaps reproduces the reported snap-back with unrelated legacy
clip overlaps on each edited track; success is still the expected result.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cursor_fixture", ROOT / "tools/native-cursor-regression.py")
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)


def run(args):
    checks = []
    with fixture.Session(args, args.output) as s:
        def create_track(name, pitch):
            track = s.cmd("ensure-track", type="midi", name=name)["trackId"]
            clip = s.cmd("ensure-midi-clip", trackId=track, name=name, startSeconds=2, lengthSeconds=4)["clipId"]
            s.cmd("ensure-midi-note", clipId=clip, noteNumber=pitch, startBeats=1, lengthBeats=1, velocity=80)
            if args.existing_overlaps:
                s.cmd("ensure-midi-clip", trackId=track, name="Legacy A", startSeconds=20, lengthSeconds=4)
                s.cmd("ensure-midi-clip", trackId=track, name="Legacy B", startSeconds=22, lengthSeconds=4)
            return track, clip

        def read_clips(track, name):
            s.shell("state-dump", name + ".json")
            data = json.loads((args.output / (name + ".json")).read_text())
            return next(t for t in data["edit"]["tracks"] if t["id"] == track).get("clips", [])

        def check(name, track, before, expected_starts, preserve_id):
            after = read_clips(track, name)
            targets = [c for c in after if c["name"] == before[0]["name"]]
            actual = sorted(c["startSeconds"] for c in targets)
            original_id = before[0]["id"]
            passed = (actual == expected_starts
                      and all(c["lengthSeconds"] == 4 and c["notes"] == before[0]["notes"] for c in targets)
                      and (not preserve_id or any(c["id"] == original_id for c in targets))
                      and [c for c in before if c["name"].startswith("Legacy")]
                          == [c for c in after if c["name"].startswith("Legacy")])
            checks.append({"name": name, "passed": passed, "expectedStarts": expected_starts, "actualStarts": actual})
            print(f"{name}: {'PASS' if passed else 'FAIL'} (expected {expected_starts}, got {actual})", flush=True)
            return targets

        def drag(start, end, modifier=None):
            if modifier:
                s.x("keydown", modifier)
            s.x("mousemove", *start, "mousedown", 1)
            # Real motion over several message-loop turns; don't certify only a
            # single synthetic jump or a painted ghost.
            for step in range(1, 5):
                x = round(start[0] + (end[0] - start[0]) * step / 4)
                y = round(start[1] + (end[1] - start[1]) * step / 4)
                s.x("mousemove", x, y)
            s.shell("screenshot 1600", "drag-preview-" + str(len(checks)) + ".png")
            s.x("mouseup", 1)
            if modifier:
                s.x("keyup", modifier)

        track, clip = create_track("Clip drag", 60)
        before = read_clips(track, "clip-before")
        # A canceled object selection must not poison the next edit's release.
        s.click(878, 104)
        s.x("mousemove", 740, 212, "mousedown", 1)
        s.x("mousemove", 840, 259)
        s.x("key", "Escape")
        s.x("mouseup", 1)
        s.click(837, 104)
        drag((780, 235), (860, 235))
        moved = check("clip-move", track, before, [6.0], True)
        # Derive the next down position from actual membership, so a failed
        # preceding commit cannot make the copy test miss its source entirely.
        actual_start = moved[0]["startSeconds"]
        down = round(710 + actual_start * 20 + 30)
        drag((down, 235), (down + 80, 235), "ctrl")
        check("clip-copy", track, before, sorted([actual_start, actual_start + 4]), True)
        s.shell("screenshot 1600", "clip-after.png")

        range_track, _ = create_track("Range drag", 64)
        before = read_clips(range_track, "range-before")
        s.click(919, 104)
        drag((750, 262), (830, 309), "shift")
        s.shell("screenshot 1600", "range-created.png")
        # Cancel a replacement range, restore the old one, then edit that range.
        s.click(919, 104)
        s.x("mousemove", 750, 262, "mousedown", 1)
        s.x("mousemove", 870, 309)
        s.x("key", "Escape")
        s.x("mouseup", 1)
        s.click(837, 104)
        drag((790, 285), (910, 285))
        moved = check("range-move", range_track, before, [8.0], False)
        actual_start = moved[0]["startSeconds"]
        down = round(710 + actual_start * 20 + 40)
        drag((down, 285), (down + 120, 285), "ctrl")
        check("range-copy", range_track, before, sorted([actual_start, actual_start + 6]), False)
        s.shell("screenshot 1600", "range-after.png")

    result = {"binary": str(args.binary), "binarySha256": fixture.file_hash(args.binary),
              "fixture": "X11 1600x1000, isolated debug shell, default 100% scale",
              "existingOverlaps": args.existing_overlaps, "checks": checks,
              "untested": "audio clips, vertical/group drags, native fractional scaling, physical displays, Windows/macOS"}
    (args.output / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    passed = all(c["passed"] for c in checks)
    print(f"{'PASS' if passed else 'FAIL'}: evidence in {args.output}", flush=True)
    return 0 if passed else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--display", default=os.environ.get("DISPLAY", ""))
    parser.add_argument("--delay", type=float, default=0.2)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--existing-overlaps", action="store_true")
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    raise SystemExit(run(args))


if __name__ == "__main__":
    main()
