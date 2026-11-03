#!/usr/bin/env python3
"""Audio clip gain mouse/field regression on a private 1600x1000 X11 display.

Uses the isolated debug-shell fixture and a generated sine wave in its private
sample directory. Never run against a user's normal DAW session.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import math
import struct
import subprocess
import wave

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cursor_fixture", ROOT / "tools/native-cursor-regression.py")
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)


def run(args):
    with fixture.Session(args, args.output) as s:
        window = subprocess.check_output(["xdotool", "search", "--onlyvisible", "--name", "NextStudio"], text=True).splitlines()[0]
        s.x("windowfocus", window)
        s.shell("state-dump", "initial.json")
        work = Path(json.loads((args.output / "initial.json").read_text())["ui"]["workDir"])
        if "debug-shell" not in work.parts:
            raise RuntimeError("Refusing a non-sandbox sample path")
        sample = work / "Samples/707/BassDrum1.wav"
        with wave.open(str(sample), "wb") as out:
            out.setparams((1, 2, 44100, 0, "NONE", "not compressed"))
            out.writeframes(b"".join(struct.pack("<h", round(12000 * math.sin(i * 440 * 2 * math.pi / 44100))) for i in range(44100 * 8)))
        track = s.cmd("ensure-track", type="audio", name="Gain fixture")["trackId"]
        s.click(45, 180)
        s.x("mousemove", 200, 80, "mousedown", 1)
        for x, y in [(250, 100), (400, 150), (600, 220), (750, 235)]:
            s.x("mousemove", x, y)
        s.x("mouseup", 1)
        s.click(810, 220)
        checks = []
        serial = 0

        def state():
            nonlocal serial
            serial += 1
            name = f"state-{serial:02d}.json"
            s.shell("state-dump", name)
            return json.loads((args.output / name).read_text())["edit"]

        def audio():
            return next(t for t in state()["tracks"] if t["id"] == track)["clips"][0]

        def check(name, condition, **details):
            checks.append({"name": name, "passed": bool(condition), **details})
            (args.output / "results.json").write_text(json.dumps({
                "binarySha256": fixture.file_hash(args.binary), "checks": checks,
                "fixture": "Linux/X11 1600x1000, isolated sine-wave sample, 100% scale",
                "untested": "physical audio output, Windows/macOS, fractional native raster scaling",
            }, indent=2) + "\n")
            print(f"{name}: {'PASS' if condition else 'FAIL'} {details}", flush=True)
            if not condition:
                raise AssertionError(name)

        def gain(expected, name):
            actual = audio()["gainDb"]
            check(name, abs(actual - expected) < 0.0001, actual=actual, expected=expected)

        def handle_y(db):
            return round(354 - (db + 100) / 124 * 108)

        def drag(start, end):
            s.x("mousemove", *start, "mousedown", 1)
            s.x("mousemove", *end)
            s.x("mouseup", 1)

        def text(value):
            s.x("mousemove", 955, 145, "click", "--repeat", 2, "--delay", 100, 1)
            s.x("key", "ctrl+a")
            s.x("type", "--", value)
            s.x("key", "Return")

        # The circle must work before the user enlarges the default 50px track.
        s.shell("screenshot 1600", "default-height.png")
        drag((830, 248), (830, 256))
        gain(-1, "default-height-circle-drag")
        s.click(1495, 40)
        gain(0, "default-height-undo")
        s.x("mousemove", 680, 259, "mousedown", 1)
        s.x("mousemove", 680, 359)
        s.x("mouseup", 1)
        s.click(810, 230)
        s.shell("screenshot 1600", "selected.png")
        s.shell("state-dump", "selected.json")
        original = audio()
        gain(0, "initial-unity")
        s.x("mousemove", 830, 267)
        circle_cursor = s.cursor("circle-cursor")
        s.x("mousemove", 795, 267)
        check("line-not-handle", s.cursor("line-cursor") != circle_cursor)
        s.x("mousemove", 833, 270)
        check("circle-corner-not-handle", s.cursor("corner-cursor") != circle_cursor)
        s.x("mousemove", 830, 220)
        check("header-not-handle", s.cursor("header-cursor") != circle_cursor)
        s.x("mousemove", 830, 267, "mousedown", 1)
        s.x("mousemove", 830, 315)
        gain(-6, "circle-live-gain")
        s.shell("screenshot 1600", "live-gain.png")
        s.x("mouseup", 1)
        gain(-6, "circle-commit")
        check("gain-preserves-position", all(audio()[key] == original[key] for key in ("startSeconds", "lengthSeconds")))
        s.click(1495, 40)
        gain(0, "single-undo")
        s.click(1560, 40)
        gain(-6, "single-redo")
        drag((795, handle_y(-6)), (795, 310))
        gain(-6, "line-drag-no-gain")
        drag((830, 220), (830, 240))
        gain(-6, "header-drag-no-gain")

        y = handle_y(-6)
        s.x("mousemove", 830, y, "mousedown", 1)
        s.x("mousemove", 830, y - 40)
        gain(-1, "escape-preview")
        s.x("key", "Escape")
        gain(-6, "escape-restores")
        s.x("mousemove", 830, y - 60)
        s.x("mouseup", 1)
        gain(-6, "cancelled-release-no-gain")
        s.click(1495, 40)
        gain(0, "cancel-no-undo-entry")
        s.click(1560, 40)
        gain(-6, "restore-before-reset")
        s.x("mousemove", 830, y, "click", "--repeat", 2, "--delay", 100, 1)
        gain(0, "double-click-reset")
        s.click(1495, 40)
        gain(-6, "reset-undo")
        text("-12.5")
        gain(-12.5, "typed-absolute")
        text("-101")
        gain(-12.5, "invalid-text-rejected")
        s.x("key", "Escape")
        s.x("mousemove", 955, 145, "click", 4)
        gain(-12, "field-wheel")
        drag((955, 145), (955, 185))
        gain(-17, "field-drag")
        s.click(1495, 40)
        gain(-12, "field-single-undo")
        s.x("mousemove", 955, 145, "mousedown", 1)
        s.x("mousemove", 955, 169)
        gain(-15, "field-live-preview")
        s.x("key", "Escape")
        s.x("mouseup", 1)
        gain(-12, "field-escape")
        s.x("mousemove", 830, handle_y(-12), "mousedown", 1)
        s.x("mousemove", 830, handle_y(-12) + 16)
        gain(-14, "selection-preview")
        s.cmd("select-track", trackId=track)
        gain(-12, "selection-cancels-preview")
        s.x("mouseup", 1)
        gain(-12, "selection-release-no-gain")
        # Add a second real audio clip without modifying the first.
        s.x("mousemove", 200, 80, "mousedown", 1)
        for x, y in [(300, 100), (600, 180), (990, 280)]:
            s.x("mousemove", x, y)
        s.x("mouseup", 1)
        s.click(1050, 220)
        text("-6")
        s.x("keydown", "ctrl")
        s.click(810, 220)
        s.x("keyup", "ctrl")
        check("two-audio-selected", state()["selection"]["selectedClipCount"] == 2)

        def gains():
            return [c["gainDb"] for c in next(t for t in state()["tracks"] if t["id"] == track)["clips"]]

        check("mixed-gains-before", gains() == [-12.0, -6.0])
        drag((830, handle_y(-12)), (830, handle_y(-12) + 16))
        check("group-relative-drag", gains() == [-14.0, -8.0])
        s.click(1495, 40)
        check("group-single-undo", gains() == [-12.0, -6.0])
        text("-3")
        check("group-absolute-text", gains() == [-3.0, -3.0])
        s.click(1495, 40)
        check("group-absolute-undo", gains() == [-12.0, -6.0])
        # Main command routing must cancel a live preview before history changes.
        s.x("mousemove", 830, handle_y(-12), "mousedown", 1)
        s.x("mousemove", 830, handle_y(-12) + 16)
        check("history-preview", gains() == [-14.0, -8.0])
        s.x("key", "ctrl+z")
        s.x("mouseup", 1)
        check("undo-during-drag", gains() == [-12.0, 0.0])
        s.x("key", "ctrl+shift+z")
        check("redo-after-cancel", gains() == [-12.0, -6.0])
        s.x("mousemove", 830, handle_y(-12), "mousedown", 1)
        s.x("mousemove", 830, handle_y(-12) + 16)
        s.shell("project-save-as")
        check("save-cancels-live-preview", gains() == [-12.0, -6.0])
        s.x("mouseup", 1)
        s.x("key", "Escape")
        # Mixed selection disables the field, but a circle still targets audio only.
        midi_track = s.cmd("ensure-track", type="midi", name="MIDI untouched")["trackId"]
        s.cmd("ensure-midi-clip", trackId=midi_track, name="MIDI untouched", startSeconds=2, lengthSeconds=8)
        s.click(810, 220)
        s.x("keydown", "ctrl")
        s.click(1050, 220)
        s.click(810, 380)
        s.x("keyup", "ctrl")
        check("mixed-selection", state()["selection"]["selectedClipCount"] == 3)
        midi_before = next(t for t in state()["tracks"] if t["id"] == midi_track)["clips"]
        s.x("mousemove", 955, 145, "click", 4)
        check("mixed-field-disabled", gains() == [-12.0, -6.0])
        drag((830, handle_y(-12)), (830, handle_y(-12) + 16))
        check("mixed-handle-audio-only", gains() == [-14.0, -8.0]
              and next(t for t in state()["tracks"] if t["id"] == midi_track)["clips"] == midi_before)
        s.shell("screenshot 1600", "final.png")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--display", default=os.environ.get("DISPLAY", ""))
    parser.add_argument("--delay", type=float, default=0.2)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    os.environ["DISPLAY"] = args.display
    raise SystemExit(run(args))


if __name__ == "__main__":
    main()
