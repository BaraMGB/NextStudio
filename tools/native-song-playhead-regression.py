#!/usr/bin/env python3
"""Verify Song Editor canvas editing never seeks transport, including double-clicks.

Uses the private 1600x1000, 100%-scale X11/debug-shell fixture from
native-cursor-regression.py. Run only on an isolated display, not a user session.
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
    binary_hash = fixture.file_hash(args.binary)
    with fixture.Session(args, args.output) as s:
        track = s.cmd("ensure-track", type="midi", name="Playhead regression")["trackId"]
        clip = s.cmd("ensure-midi-clip", trackId=track, name="Original", startSeconds=2, lengthSeconds=4)["clipId"]
        s.cmd("ensure-midi-note", clipId=clip, noteNumber=60, startBeats=1, lengthBeats=1, velocity=80)
        audio = s.cmd("ensure-track", type="audio", name="Empty audio")["trackId"]

        def check(name, condition, **details):
            checks.append({"name": name, "passed": condition, **details})
            (args.output / "results.json").write_text(json.dumps({
                "binary": str(args.binary), "binarySha256": binary_hash,
                "fixture": "Linux/X11 1600x1000, isolated debug shell, 100% scale",
                "checks": checks,
                "untested": "active playback/audio clock, physical/fractional displays, Windows/macOS",
            }, indent=2) + "\n")
            print(f"{name}: {'PASS' if condition else 'FAIL'} {details}", flush=True)
            if not condition:
                raise AssertionError(name)

        def state(name):
            s.shell("state-dump", name + ".json")
            return json.loads((args.output / (name + ".json")).read_text())["edit"]

        def clips(data, track_id=track):
            return next(t for t in data["tracks"] if t["id"] == track_id).get("clips", [])

        def held(name):
            position = float(s.shell("transport-state")["positionSeconds"])
            check(name, abs(position - anchor) < 1.0e-6, expected=anchor, actual=position)

        def drag(start, end, button=1):
            s.x("mousemove", *start, "mousedown", button)
            for step in range(1, 5):
                s.x("mousemove", round(start[0] + (end[0] - start[0]) * step / 4),
                    round(start[1] + (end[1] - start[1]) * step / 4))
            s.x("mouseup", button)

        # Seed a nonzero transport position through the real ruler, not a debug setter.
        s.click(790, 174)
        anchor = float(s.shell("transport-state")["positionSeconds"])
        check("ruler-seeks", abs(anchor - 4.0) < 1.0e-6, actual=anchor)
        original = clips(state("before"))

        # Split a real double-click so the first release and second press are
        # independently observed. Keep the round trip below JUCE's double-click
        # timeout; no sleeps, screenshots or large state dumps between presses.
        s.x("mousemove", 950, 235, "click", 1)
        held("double-first-release-no-seek")
        s.x("mousedown", 1)
        held("double-second-press-no-seek")
        created = state("double-second-press")
        inserted = [c for c in clips(created) if c["id"] != clip]
        check("double-inserts-immediately", len(inserted) == 1
              and inserted[0]["startSeconds"] == 12.0 and inserted[0]["lengthSeconds"] == 2.0)
        s.x("mouseup", 1)
        held("double-second-release-no-seek")
        s.shell("screenshot 1600", "double-click.png")
        after = state("double-release")
        check("original-clip-unchanged", next(c for c in clips(after) if c["id"] == clip) == original[0])
        s.click(1495, 40)  # Toolbar Undo: one insertion transaction.
        check("insert-single-undo", clips(state("undo")) == original)
        held("undo-no-seek")
        s.click(1560, 40)
        check("insert-single-redo", len(clips(state("redo"))) == 2)
        held("redo-no-seek")
        before_interactions = s.model("before-interactions")

        s.click(775, 235)
        selected = state("clip-selection")["selection"]
        check("clip-selection-preserved", selected["selectedClipCount"] == 1)
        held("clip-click-no-seek")
        s.click(1050, 235)
        held("empty-midi-click-no-seek")
        for x, name in [(837, "pointer"), (878, "lasso"), (919, "range"),
                        (960, "knife"), (1000, "stretch")]:
            s.click(x, 104)
            s.click(1150, 235)
            held(name + "-empty-click-no-seek")
        for x, name in [(878, "lasso"), (919, "range")]:
            s.click(x, 104)
            drag((740, 212), (840, 259))
            held(name + "-drag-no-seek")
            s.x("key", "Escape")
        s.click(837, 104)
        s.click(930, 285)
        held("empty-audio-click-no-seek")
        s.x("mousemove", 1100, 285, "click", "--repeat", 2, "--delay", 100, 1)
        held("empty-audio-double-click-no-seek")
        check("audio-double-no-clip", not clips(state("audio-double"), audio))
        s.click(930, 400)
        held("canvas-background-click-no-seek")
        s.click(930, 565)
        held("master-click-no-seek")
        before_pan = state("before-pan")["timelines"]
        drag((1200, 400), (1120, 400), button=2)
        held("middle-pan-no-seek")
        check("middle-pan-still-moves-view", state("after-pan")["timelines"] != before_pan)
        check("selection-and-navigation-preserve-model", s.model("after-interactions") == before_interactions)
        s.click(1000, 174)
        position = float(s.shell("transport-state")["positionSeconds"])
        check("ruler-still-seeks-after-editing", abs(position - anchor) > 1.0e-6, actual=position)
        s.shell("screenshot 1600", "final.png")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--display", default=os.environ.get("DISPLAY", ""))
    parser.add_argument("--delay", type=float, default=0.08)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    raise SystemExit(run(args))


if __name__ == "__main__":
    main()
