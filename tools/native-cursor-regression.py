#!/usr/bin/env python3
"""Linux/X11 native #90 cursor checks using a fresh debug-shell sandbox.

Requires Python 3.10+, Node.js, xdotool, libX11 and libXfixes, and a private
1600x1000 X display at 100% scale. Uses the existing application's layout as a
fixture, not a duplicate cursor policy. See docs/development/testing.md.
"""

import argparse
import ctypes
import hashlib
import json
import math
import os
from pathlib import Path
import queue
import shutil
import struct
import subprocess
import sys
import threading
import time
import zlib

ROOT = Path(__file__).resolve().parents[1]


def file_hash(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        while block := source.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


class CursorImage(ctypes.Structure):
    _fields_ = [("x", ctypes.c_short), ("y", ctypes.c_short),
                ("width", ctypes.c_ushort), ("height", ctypes.c_ushort),
                ("xhot", ctypes.c_ushort), ("yhot", ctypes.c_ushort),
                ("serial", ctypes.c_ulong), ("pixels", ctypes.POINTER(ctypes.c_ulong)),
                ("atom", ctypes.c_ulong), ("name", ctypes.c_char_p)]


def write_png(path, width, height, rgba):
    def chunk(kind, data):
        return struct.pack("!I", len(data)) + kind + data + struct.pack("!I", zlib.crc32(kind + data))
    rows = b"".join(b"\0" + rgba[y * width * 4:(y + 1) * width * 4] for y in range(height))
    path.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack("!2I5B", width, height, 8, 6, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


class Session:
    def __init__(self, args, output):
        self.args, self.output = args, output
        self.display = None
        self.process = None
        self.log = None
        self.checks = []
        self.x11 = ctypes.CDLL("libX11.so.6")
        self.fixes = ctypes.CDLL("libXfixes.so.3")
        self.x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
        self.x11.XOpenDisplay.restype = ctypes.c_void_p
        self.x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
        self.x11.XFree.argtypes = [ctypes.c_void_p]
        for name in ("XDefaultScreen", "XDisplayWidth", "XDisplayHeight"):
            getattr(self.x11, name).restype = ctypes.c_int
        self.x11.XDefaultScreen.argtypes = [ctypes.c_void_p]
        self.x11.XDisplayWidth.argtypes = self.x11.XDisplayHeight.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.fixes.XFixesGetCursorImage.argtypes = [ctypes.c_void_p]
        self.fixes.XFixesGetCursorImage.restype = ctypes.POINTER(CursorImage)

    def __enter__(self):
        try:
            self.display = self.x11.XOpenDisplay(self.args.display.encode())
            if not self.display:
                raise RuntimeError(f"Cannot open X display {self.args.display}")
            screen = self.x11.XDefaultScreen(self.display)
            size = (self.x11.XDisplayWidth(self.display, screen), self.x11.XDisplayHeight(self.display, screen))
            if size != (1600, 1000):
                raise RuntimeError(f"Fixture requires 1600x1000, got {size}; use private Xvfb")
            self.log = (self.output / "bridge.log").open("w")
            self.process = subprocess.Popen(["node", str(ROOT / "tools/native-cursor-bridge.js"), str(self.args.binary)], cwd=ROOT, text=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.log)
            self.responses = queue.Queue()
            threading.Thread(target=self._read, daemon=True).start()
            ready = self.response()
            fields = ready["fields"]
            settings = Path(fields["settingsPath"])
            if "debug-shell" not in settings.parts:
                raise RuntimeError(f"Refusing non-sandbox settings: {settings}")
            windows = subprocess.check_output(["xdotool", "search", "--onlyvisible", "--pid", str(ready["pid"])], text=True).splitlines()
            if not windows:
                raise RuntimeError("No visible application window")
            self.x("windowsize", windows[0], 1600, 1000, "windowmove", windows[0], 0, 0)
            time.sleep(0.5)
            self.shell("screenshot 1600", "fixture.png")
            return self
        except BaseException:
            self.close()
            raise

    def _read(self):
        for line in self.process.stdout:
            self.responses.put(line)
        self.responses.put(None)

    def response(self):
        try:
            line = self.responses.get(timeout=40)
        except queue.Empty as error:
            raise RuntimeError("Bridge response timed out; inspect bridge.log") from error
        if line is None:
            raise RuntimeError("Bridge exited; inspect bridge.log")
        result = json.loads(line)
        if result.get("status") != "ok":
            raise RuntimeError(result.get("message", str(result)))
        return result

    def shell(self, command, artifact=None):
        self.process.stdin.write(json.dumps(command) + "\n")
        self.process.stdin.flush()
        result = self.response()["fields"]
        if artifact:
            shutil.copyfile(result["path"], self.output / artifact)
        return result

    def cmd(self, command, **arguments):
        return self.shell({"command": command, "arguments": arguments})

    def model(self, name):
        self.shell("state-dump", name + ".json")
        state = json.loads((self.output / (name + ".json")).read_text())
        # Preserve complete track/clip/note records, excluding selection/view state.
        return [(track["id"], track.get("clips", [])) for track in state["edit"]["tracks"]]

    def x(self, *args):
        subprocess.run(["xdotool", *map(str, args)], check=True, timeout=10)
        time.sleep(self.args.delay)

    def click(self, x, y):
        self.x("mousemove", x, y, "click", 1)

    def cursor(self, name):
        ptr = self.fixes.XFixesGetCursorImage(self.display)
        if not ptr:
            raise RuntimeError("XFixes cursor capture failed")
        try:
            c = ptr.contents
            pixels = [c.pixels[i] & 0xffffffff for i in range(c.width * c.height)]
            rgba = bytes(value for pixel in pixels for value in ((pixel >> 16) & 255, (pixel >> 8) & 255, pixel & 255, (pixel >> 24) & 255))
            result = {"position": [c.x, c.y], "size": [c.width, c.height], "hotspot": [c.xhot, c.yhot], "visiblePixels": sum(pixel >> 24 > 0 for pixel in pixels), "hash": hashlib.sha256(rgba).hexdigest()}
            if not result["visiblePixels"]:
                raise RuntimeError(f"Invisible cursor: {name}")
            (self.output / (name + ".json")).write_text(json.dumps(result, indent=2) + "\n")
            write_png(self.output / (name + ".png"), c.width, c.height, rgba)
            return result["hash"]
        finally:
            self.x11.XFree(ptr)

    def check(self, name, expected):
        actual = self.cursor(name)
        passed = actual == expected
        self.checks.append({"name": name, "passed": passed, "expected": expected, "actual": actual})
        print(f"{name}: {'PASS' if passed else 'FAIL'}", flush=True)

    def close(self):
        if self.process:
            if self.process.poll() is None:
                try:
                    self.shell("quit")
                except (RuntimeError, BrokenPipeError):
                    self.process.terminate()
                try:
                    self.process.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait()
            self.process.stdin.close()
            self.process.stdout.close()
        if self.log:
            self.log.close()
        if self.display:
            self.x11.XCloseDisplay(self.display)
            self.display = None

    def __exit__(self, *_):
        # Never leave buttons/modifiers held after a failed gesture.
        try:
            subprocess.run(["xdotool", "mouseup", "1", "keyup", "ctrl", "shift", "alt"], check=False, timeout=10)
        finally:
            self.close()


def transitions(s):
    track = s.cmd("ensure-track", type="midi", name="Cursor transitions")["trackId"]
    s.cmd("ensure-midi-clip", trackId=track, name="Clip", startSeconds=2, lengthSeconds=2)
    s.cmd("select-track", trackId=track)
    before = s.model("before-model")
    references = {}
    areas = [("gap", 827), ("body", 770), ("left", 751), ("right", 788)]
    for tool, button in [("pointer", 837), ("stretch", 960)]:
        for area, x in areas:
            s.click(button, 104)
            s.x("mousemove", x, 236)
            references[tool, area] = s.cursor(f"reference-{tool}-{area}")
    if len({references["pointer", area] for area, _ in areas}) != 4:
        raise RuntimeError("Fixture invalid: gap/body/both-edge reference cursors must differ")
    for tool, keys in [("pointer", ["shift+Tab", "shift+Tab", "Return"]), ("stretch", ["Tab", "Return"])]:
        for area, x in areas:
            s.click(919, 104)
            s.x("mousemove", x, 236)
            s.x("key", *keys)
            s.check(f"range-to-{tool}-{area}", references[tool, area])
    s.click(919, 104)
    s.x("mousemove", 810, 236, "mousedown", 1)
    s.x("mousemove", 890, 236)
    s.x("mouseup", 1)
    for area, x in [("body", 850), ("left", 810), ("right", 890)]:
        s.click(837, 104)
        s.x("mousemove", x, 236)
        expected = s.cursor("reference-selected-" + area)
        s.click(919, 104)
        s.x("mousemove", x, 236)
        s.x("key", "shift+Tab", "shift+Tab", "Return")
        s.check("range-to-pointer-selected-" + area, expected)
        s.x("key", "Tab", "Tab", "Tab", "Tab", "Return")
        s.x("key", "shift+Tab", "shift+Tab", "shift+Tab", "shift+Tab", "Return")
        s.check("knife-to-pointer-selected-" + area, expected)
    s.click(837, 104)
    s.x("mousemove", 827, 566)
    normal = s.cursor("reference-master")
    for tool, keys in [("pointer", ["shift+Tab", "shift+Tab", "Return"]), ("knife", ["Tab", "Tab", "Return"])]:
        s.click(919, 104)
        s.x("mousemove", 827, 566)
        s.x("key", *keys)
        s.check("master-range-to-" + tool, normal)
    return s.model("after-model") == before


def working_areas(s):
    track = s.cmd("ensure-track", type="midi", name="Cursor working areas")["trackId"]
    for name, start in [("Left", 2), ("Right", 8)]:
        clip = s.cmd("ensure-midi-clip", trackId=track, name=name, startSeconds=start, lengthSeconds=2)["clipId"]
        s.cmd("ensure-midi-note", clipId=clip, noteNumber=60, startBeats=1, lengthBeats=1, velocity=96)
    s.cmd("select-track", trackId=track)
    s.x("mousemove", 766, 232, "click", "--repeat", 2, "--delay", 100, 1)
    time.sleep(0.5)
    before = s.model("before-model")
    s.x("mousemove", 700, 800, "keydown", "ctrl", "click", "--repeat", 15, "--delay", 60, 5, "keyup", "ctrl")
    time.sleep(0.5)
    s.shell("screenshot 1600", "wide-fixture.png")
    s.x("mousemove", 700, 600)
    normal = s.cursor("reference-normal")
    modes = {}
    for tool, button in [("draw", 788), ("eraser", 870), ("knife", 911), ("range", 829), ("lasso", 951)]:
        s.click(button, 625)
        s.x("mousemove", 650, 800)
        mode = s.cursor("reference-" + tool)
        modes[tool] = mode
        if mode == normal:
            raise RuntimeError(f"Fixture invalid: {tool} reference must differ from pointer")
        for area, x in [("left-empty", 260), ("left-clip", 650), ("gap", 1000), ("right-clip", 1350), ("right-empty", 1550)]:
            expected = normal if tool in ("draw", "eraser", "knife") and "clip" not in area else mode
            s.x("mousemove", x, 800)
            s.check(tool + "-" + area, expected)
            s.x("mousemove", 700, 600)
            s.x("mousemove", x, 800)
            s.check(tool + "-" + area + "-reentry", expected)
        if s.model(tool + "-model") != before:
            return False
    if len(set(modes.values())) != 5:
        raise RuntimeError("Fixture invalid: tool reference cursor images must differ")
    s.click(1000, 104)
    for area, x in [("clip", 769), ("gap", 827)]:
        s.x("mousemove", x, 236)
        s.check("song-knife-" + area, modes["knife"])
    s.x("mousemove", 827, 516)
    s.check("master-normal", normal)
    return s.model("after-model") == before


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--display", default=os.environ.get("DISPLAY"))
    parser.add_argument("--output", type=Path, required=True, help="New directory for retained results; never overwritten")
    parser.add_argument("--suite", choices=("all", "transitions", "working-areas"), default="all")
    parser.add_argument("--delay", type=float, default=0.18, help="UI settling delay after native input")
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
    results = {"binary": str(args.binary), "binarySha256": file_hash(args.binary), "display": args.display, "fixture": "Linux X11 1600x1000, 100% sandbox application/cursor scale", "suites": {}}
    try:
        for name, procedure in [("transitions", transitions), ("working-areas", working_areas)]:
            if args.suite not in ("all", name):
                continue
            output = args.output / name
            output.mkdir()
            with Session(args, output) as session:
                unchanged = procedure(session)
                results["suites"][name] = {"checks": session.checks, "modelUnchanged": unchanged}
                if not unchanged:
                    raise RuntimeError(f"{name} changed musical model")
        failed = any(not check["passed"] for suite in results["suites"].values() for check in suite["checks"])
        results["passed"] = not failed
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
