# Audio fixtures

`tone-440hz.mp3` is a generated 0.25-second stereo sine wave, not sampled music.
Regenerate from the repository root with:

```sh
ffmpeg -hide_banner -loglevel error -f lavfi -i 'sine=frequency=440:duration=0.25' -ac 2 -b:a 128k -map_metadata -1 -y App/tests/fixtures/tone-440hz.mp3
```

`ClipOverwriteCommandTests` uses it to exercise the actual MP3 source reference,
direct-playback policy, legacy cached-proxy bypass, undo/redo and saved state.
The native audio-import regression separately records real device output.
