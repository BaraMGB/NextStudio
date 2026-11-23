#!/usr/bin/env python3
"""MP3 import, device-output capture and waveform repaint on private X11 1600x1000.

Requires the native-cursor fixture dependencies, FFmpeg, Pillow and a running
PipeWire server. Routes the application to a temporary virtual sink, never to
speakers. Captures the actual display without a debug screenshot (which would
itself repaint the UI). All edits belong to an isolated debug-shell session.
"""
import argparse
from array import array
import importlib.util
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import time
import xml.etree.ElementTree as ET

from PIL import ImageGrab

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cursor_fixture", ROOT / "tools/native-cursor-regression.py")
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)


def pipewire_objects():
    return json.loads(subprocess.check_output(["pw-dump"], text=True))


class VirtualOutput:
    def __enter__(self):
        self.name = f"NextStudioImportTest-{os.getpid()}-{time.monotonic_ns()}"
        self.previous_props = os.environ.get("PIPEWIRE_PROPS")
        subprocess.run(["pw-cli", "create-node", "adapter", "{ "
                        "factory.name = support.null-audio-sink "
                        f"node.name = {self.name} media.class = Audio/Sink "
                        "object.linger = true priority.session = 0 audio.position = [ FL FR ] }"], check=True)
        assert any(obj.get("info", {}).get("props", {}).get("node.name") == self.name
                   for obj in pipewire_objects()), "Virtual audio sink was not created"
        os.environ["PIPEWIRE_PROPS"] = f'{{ target.object = "{self.name}" node.dont-fallback = true }}'
        return self

    def __exit__(self, *_):
        if self.previous_props is None:
            os.environ.pop("PIPEWIRE_PROPS", None)
        else:
            os.environ["PIPEWIRE_PROPS"] = self.previous_props
        for obj in pipewire_objects():
            if obj.get("info", {}).get("props", {}).get("node.name") == self.name:
                subprocess.run(["pw-cli", "destroy", str(obj["id"])], check=True)


def decoded_samples(path):
    decoded = subprocess.check_output([
        "ffmpeg", "-v", "error", "-i", str(path), "-ac", "1", "-ar", "48000",
        "-f", "f32le", "-acodec", "pcm_f32le", "-",
    ])
    samples = array("f")
    samples.frombytes(decoded)
    return samples


def rms(samples):
    return math.sqrt(sum(v * v for v in samples) / len(samples)) if samples else 0.0


def measure_output(s, output, sink, preview=False):
    output.mkdir(parents=True, exist_ok=True)
    capture = output / "device-output.wav"
    with (output / "record.log").open("w") as log:
        recorder = subprocess.Popen([
            "pw-record", "--target", sink, "--properties", "stream.capture.sink=true",
            "--rate", "48000", "--channels", "2", "--format", "f32", str(capture),
        ], stderr=log)
        try:
            time.sleep(0.7)
            assert recorder.poll() is None, "Device-output recorder failed to start"
            if preview:
                s.click(110, 570)
            else:
                s.shell("play")
            time.sleep(5)
            transport = s.shell("transport-state")
            if preview:
                assert transport["playing"] == "false", transport
                s.click(168, 570)
            else:
                assert transport["playing"] == "true" and float(transport["positionSeconds"]) > 4, transport
                s.shell("stop")
            (output / "pipewire.json").write_text(json.dumps(pipewire_objects(), indent=2) + "\n")
        finally:
            recorder.send_signal(signal.SIGINT)
            try:
                recorder.wait(timeout=10)
            except subprocess.TimeoutExpired:
                recorder.kill()
                recorder.wait()
    samples = decoded_samples(capture)
    level = rms(samples)
    assert level > 0.05, f"Silent device output: RMS={level}"
    # The last complete second before stopping must contain the generated
    # 440 Hz tone, not merely an unrelated transient or device noise.
    tone = samples[-62400:-14400]
    assert len(tone) == 48000, "Device-output recording is too short"
    real = sum(v * math.cos(2 * math.pi * 440 * i / 48000) for i, v in enumerate(tone))
    imag = sum(v * math.sin(2 * math.pi * 440 * i / 48000) for i, v in enumerate(tone))
    tone_rms = math.sqrt(2 * (real * real + imag * imag)) / len(tone)
    assert tone_rms > 0.05, f"Missing 440 Hz tone at device output: RMS={tone_rms}"
    return {"rms": level, "tone440Rms": tone_rms, "transport": transport}


def run_case(args, name, channels, encoding, extension="mp3", broken_proxy=False):
    output = args.output / name
    output.mkdir(parents=True, exist_ok=True)
    with fixture.Session(args, output) as s:
        s.shell("state-dump", "initial.json")
        work = Path(json.loads((output / "initial.json").read_text())["ui"]["workDir"])
        if "debug-shell" not in work.parts:
            raise RuntimeError("Refusing a non-sandbox audio path")
        source = work / f"probe.{extension}"
        subprocess.run([
            "ffmpeg", "-v", "error", "-f", "lavfi", "-i",
            "sine=frequency=440:duration=8", "-af", "volume=5", "-ac", str(channels),
            *encoding, "-y", str(source),
        ], check=True)
        if broken_proxy:
            # Pre-existing unreadable cache entry, before the engine can map it.
            # JUCE's String::hashCode64 names the ordinary file proxy (101 hash).
            file_hash = 0
            for char in str(source):
                file_hash = (file_hash * 101 + ord(char)) & ((1 << 64) - 1)
            proxy = work.parent / f"proxy_{file_hash:x}.wav"
            proxy.write_bytes(b"")
        track_id = s.cmd("ensure-track", type="audio", name="Import fixture")["trackId"]
        # Expand the lane before import, so a later layout change cannot conceal
        # a missing background-completion repaint.
        s.x("mousemove", 680, 259, "mousedown", 1)
        s.x("mousemove", 680, 359)
        s.x("mouseup", 1)
        s.click(45, 310)  # Home browser
        s.click(395, 85)  # Parent, then Back: rescan the generated file
        s.click(95, 85)
        time.sleep(0.5)
        preview_output = None
        if broken_proxy:
            s.click(200, 210)
            preview_output = measure_output(s, output / "preview", args.sink, preview=True)
        s.x("mousemove", 200, 210, "mousedown", 1)
        for x, y in [(250, 180), (400, 200), (600, 220), (750, 235)]:
            s.x("mousemove", x, y)
        s.x("mouseup", 1)

        # No mouse movement or app commands until the live waveform is checked.
        deadline = time.monotonic() + 15
        blue_pixels = 0
        while time.monotonic() < deadline:
            time.sleep(0.2)
            display = ImageGrab.grab(xdisplay=args.display)
            pixels = display.crop((765, 240, 895, 345)).convert("RGB").getdata()
            blue_pixels = sum(abs(r - 79) <= 3 and abs(g - 129) <= 3 and abs(b - 189) <= 3
                              for r, g, b in pixels)
            if blue_pixels > 500:
                break
        display.save(output / "live-waveform.png")
        if blue_pixels <= 500:
            raise AssertionError(f"{name}: no automatic waveform repaint ({blue_pixels} pixels)")

        s.shell("state-dump", "imported.json")
        state = json.loads((output / "imported.json").read_text())["edit"]
        track = next(t for t in state["tracks"] if t["id"] == track_id)
        assert len(track["clips"]) == 1, track
        clip = track["clips"][0]
        assert clip["type"] == "audio" and 7.9 < clip["lengthSeconds"] < 8.2, clip
        assert abs(clip["startSeconds"] - 2) < 0.001, clip

        assert clip["sourceFile"] == str(source), clip
        assert clip["playbackFile"] == str(source), clip
        assert clip["proxyAllowed"] == (extension != "mp3"), clip
        device_output = measure_output(s, output, args.sink)
        reloaded_output = None
        if broken_proxy:
            assert proxy.read_bytes() == b"", "The corrupt cache fixture was unexpectedly replaced"
            # Save a real project, turn its MP3 state into a legacy proxy-backed
            # clip, and reopen through Projects to exercise MainComponent loading.
            s.click(715, 178)
            s.shell("project-save-as")
            s.x("type", "--clearmodifiers", "Legacy MP3")
            s.x("key", "Return")
            s.shell("state-dump", "saved.json")
            project = Path(json.loads((output / "saved.json").read_text())["edit"]["file"])
            assert "debug-shell" in project.parts and project.suffix == ".tracktionedit", project
            tree = ET.parse(project)
            audio_nodes = list(tree.iter("AUDIOCLIP"))
            assert len(audio_nodes) == 1, audio_nodes
            audio_nodes[0].set("proxyAllowed", "1")
            audio_nodes[0].set("name", "Legacy restored MP3")
            tree.write(project, encoding="utf-8", xml_declaration=True)
            s.x("mousemove", 200, 175, "click", "--repeat", 2, "--delay", 120, 1)
            time.sleep(0.5)
            s.shell("state-dump", "reloaded.json")
            loaded_edit = json.loads((output / "reloaded.json").read_text())["edit"]
            loaded_track = next(t for t in loaded_edit["tracks"] if t["id"] == track_id)
            loaded_clip = loaded_track["clips"][0]
            assert loaded_clip["name"] == "Legacy restored MP3", loaded_clip
            assert loaded_clip["proxyAllowed"] is False and loaded_clip["playbackFile"] == str(source), loaded_clip
            s.click(715, 178)
            reloaded_output = measure_output(s, output / "legacy-reloaded", args.sink)
        result = {"name": name, "passed": True, "waveformPixels": blue_pixels,
                  "deviceOutput": device_output, "previewOutput": preview_output,
                  "legacyReloadedOutput": reloaded_output, "clip": clip}
        (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        print(f"{name}: PASS (device RMS {device_output['rms']}, 440 Hz RMS {device_output['tone440Rms']})", flush=True)
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--display", default=":97")
    parser.add_argument("--delay", type=float, default=0.15)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--broken-proxy-only", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with VirtualOutput() as device:
        args.sink = device.name
        results = []
        if not args.broken_proxy_only:
            results += [
                run_case(args, "mp3-cbr-mono", 1, ["-b:a", "192k"]),
                run_case(args, "mp3-vbr-stereo", 2, ["-q:a", "2"]),
                run_case(args, "mp3-48khz-stereo", 2, ["-ar", "48000", "-b:a", "192k"]),
                run_case(args, "wav-control", 1, ["-c:a", "pcm_s16le"], "wav"),
            ]
        results.append(run_case(args, "mp3-broken-cache", 2, ["-q:a", "2"], broken_proxy=True))
    (args.output / "results.json").write_text(json.dumps({
        "binarySha256": fixture.file_hash(args.binary), "checks": results,
        "untested": "Physical speakers/hardware drivers, Windows/macOS, arbitrary MP3 encoders and damaged files",
    }, indent=2) + "\n")


if __name__ == "__main__":
    main()
