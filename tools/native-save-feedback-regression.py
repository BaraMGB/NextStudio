#!/usr/bin/env python3
"""#87: real Save/Save As dispatch and transient header pixels in private X11.

Uses the cursor fixture's isolated debug-shell sandbox; additionally needs Pillow.
Fixed default-layout crops are native evidence, not a second feedback state model.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import time

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cursor_fixture", ROOT / "tools/native-cursor-regression.py")
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)


def run(args, output):
    checks = []
    with fixture.Session(args, output) as s:
        def check(name, condition):
            if not condition:
                raise AssertionError(name)
            checks.append(name)
            print(name + ": PASS", flush=True)

        def capture(name, crop=(1110, 18, 1290, 62)):
            s.shell("screenshot 1600", name + ".png")
            with Image.open(output / (name + ".png")) as image:
                return image.convert("RGB").crop(crop).tobytes()

        def state(name):
            s.shell("state-dump", name + ".json")
            return json.loads((output / (name + ".json")).read_text())["edit"]

        def save_key():
            s.x("key", "ctrl+s")

        track = s.cmd("ensure-track", type="midi", name="Save feedback")['trackId']
        clip = s.cmd("ensure-midi-clip", trackId=track, name="Fixture", startSeconds=2, lengthSeconds=4)['clipId']
        s.cmd("ensure-midi-note", clipId=clip, noteNumber=60, startBeats=1, lengthBeats=1, velocity=96)
        musical_before = s.model("before-musical-model")
        s.click(1450, 400) # Give the private X11 window real keyboard focus.
        initial_info = capture("initial")
        # First Save must enter the target form, not claim success yet.
        save_key()
        s.shell("screenshot 1600", "first-save-form.png")
        s.x("type", "--clearmodifiers", "Feedback")
        s.x("key", "Return")
        first_saved_info = capture("first-save")
        check("first-save-success", first_saved_info != initial_info)
        saved = state("first-saved-state")
        s.x("key", "ctrl+l")
        check("shortcut-remains-active", state("shortcut-state")["transport"]["looping"] != saved["transport"]["looping"])
        s.x("key", "ctrl+l")
        check("shortcut-does-not-dismiss-feedback", capture("after-shortcut") == first_saved_info)
        project = Path(saved["file"])
        if "debug-shell" not in project.parts:
            raise RuntimeError("Refusing to modify a non-sandbox project")
        check("persistent-file-written", project.is_file() and project.suffix == ".tracktionedit")
        time.sleep(3.2)
        clean_info = capture("expired")
        check("three-second-expiry", clean_info != first_saved_info)

        # The sidebar Save button uses the same confirmed-success path.
        s.click(171, 76)
        check("sidebar-save-success", capture("sidebar-save") != clean_info)
        time.sleep(1.8)
        save_key()
        time.sleep(1.4)
        check("repeated-save-restarts-lifetime", capture("repeated-save") != clean_info)
        time.sleep(1.8)
        check("repeated-save-expires", capture("repeat-expired") == clean_info)

        # Explicit Save As and cancellation must clear previous success.
        s.shell("project-save-as")
        s.x("type", "--clearmodifiers", "Second")
        s.x("key", "Return")
        save_as_info = capture("save-as-success")
        check("save-as-success", save_as_info != clean_info)
        second = Path(state("second-saved-state")["file"])
        check("save-as-preserves-original", second.is_file() and project.is_file() and second != project)
        s.shell("project-save-as")
        s.x("key", "Escape")
        second_clean_info = capture("cancelled")
        check("cancel-does-not-retain-success", second_clean_info != save_as_info)
        check("cancel-preserves-current-path", Path(state("cancelled-state")["file"]) == second)

        # Fail a direct save deterministically without changing permissions or
        # touching any real user project: put a directory at this sandbox target.
        save_key()
        check("success-before-failure", capture("before-failure") != second_clean_info)
        backup = second.with_suffix(".backup")
        second.rename(backup)
        second.mkdir()
        try:
            save_key()
            s.shell("screenshot 1600", "save-error.png")
            s.x("key", "Escape")
            check("failed-save-clears-success", capture("failed-save") == second_clean_info)
            check("failed-save-keeps-path", Path(state("failed-state")["file"]) == second)
        finally:
            second.rmdir()
            backup.rename(second)
        save_key()
        check("save-retry-success", capture("retry") != second_clean_info)

        # No sidebar dependency: switch browser, then collapse the sidebar.
        s.click(44, 302)  # Home
        save_key()
        check("other-browser-success", capture("home-save") != second_clean_info)
        s.x("mousemove", 415, 400, "mousedown", 1)
        s.x("mousemove", 30, 400)
        s.x("mouseup", 1)
        time.sleep(3.2)
        collapsed_crop = (945, 18, 1125, 62)
        collapsed_blank = capture("collapsed-blank", collapsed_crop)
        save_key()
        check("collapsed-sidebar-success", capture("collapsed-save", collapsed_crop) != collapsed_blank)
        time.sleep(3.2)
        check("collapsed-sidebar-expiry", capture("collapsed-expired", collapsed_crop) == collapsed_blank)

        # Restore Projects and use a narrower window. Button/text geometry is
        # tested exhaustively in HeaderPresentationTests; images retain appearance.
        s.x("mousemove", 85, 400, "mousedown", 1)
        s.x("mousemove", 420, 400)
        s.x("mouseup", 1)
        s.x("mousemove", 265, 400, "mousedown", 1)
        s.x("mousemove", 415, 400)
        s.x("mouseup", 1)
        s.click(44, 46)
        windows = fixture.subprocess.check_output(["xdotool", "search", "--onlyvisible", "--name", "Second"], text=True).splitlines()
        if not windows:
            raise RuntimeError("Saved project window not found")
        s.x("windowsize", windows[0], 1200, 1000)
        narrow_crop = (855, 18, 1035, 62)
        narrow_blank = capture("narrow-blank", narrow_crop)
        save_key()
        check("narrow-header-success", capture("narrow-save", narrow_crop) != narrow_blank)
        time.sleep(3.2)
        check("narrow-header-expiry", capture("narrow-expired", narrow_crop) == narrow_blank)

        # Minimal mode retains position/time and save state while yielding the
        # tempo/signature and loop columns at still smaller supported widths.
        s.x("windowsize", windows[0], 900, 1000)
        minimal_crop = (655, 18, 735, 62)
        minimal_blank = capture("minimal-blank", minimal_crop)
        save_key()
        check("minimal-header-success", capture("minimal-save", minimal_crop) != minimal_blank)
        time.sleep(3.2)
        check("minimal-header-expiry", capture("minimal-expired", minimal_crop) == minimal_blank)
        s.x("windowsize", windows[0], 1600, 1000)

        s.shell("project-save-as")
        long_name = "A very long project name to exercise header ellipsis"
        s.x("type", "--clearmodifiers", long_name)
        s.x("key", "Return")
        long_saved_info = capture("long-name")
        check("long-name-feedback", long_saved_info != second_clean_info)
        check("long-name-file-written", (project.parent / (long_name + ".tracktionedit")).is_file())
        time.sleep(3.2)
        check("long-name-feedback-expires", capture("long-name-expired") != long_saved_info)

        check("saves-preserve-musical-model", s.model("after-musical-model") == musical_before)

        # New does not inherit the old saved project's feedback. Autosave remains
        # silent while recovery snapshots are written for the new edit.
        save_key()
        s.click(120, 76)
        check("new-project-clears-feedback", capture("new-project") == initial_info)
        new_state = state("new-state")
        check("new-project-is-temporary", Path(new_state["file"]).suffix == ".nextTemp")
        # Construction writes an initial snapshot immediately. A fresh edit
        # proves the periodic worker also completes without success feedback.
        recovery = Path(new_state["file"])
        previous_write = recovery.stat().st_mtime_ns
        s.cmd("ensure-track", type="midi", name="Recovery autosave fixture")
        check("recovery-autosave-queued", state("autosave-queued-state")["needsAutosave"])
        time.sleep(10.5)
        check("recovery-autosave-completed", not state("autosave-state")["needsAutosave"] and recovery.stat().st_mtime_ns > previous_write)
        check("recovery-autosave-silent", capture("autosave") == initial_info)

    return checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--display", default=os.environ.get("DISPLAY", ""))
    parser.add_argument("--delay", type=float, default=0.1)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    result = {"binary": str(args.binary.resolve()), "binaryHash": fixture.file_hash(args.binary), "checks": []}
    try:
        result["checks"] = run(args, output)
        result["passed"] = True
    except Exception as error:
        result["passed"] = False
        result["error"] = str(error)
        raise
    finally:
        (output / "results.json").write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
