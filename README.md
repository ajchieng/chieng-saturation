# Chieng Saturation

A simple **one-knob saturation** audio plugin built with [JUCE 8](https://juce.com).
Builds as an **Audio Unit** (for Logic Pro and other AU hosts) and a **Standalone** app.

The single **Drive** knob drives the signal into a `tanh` soft-clip with output-level
compensation, so the tone thickens and warms as you turn it up while loudness stays roughly
steady.

## Requirements

- macOS with Xcode + command-line tools
- CMake 3.25+

## Build

JUCE is not committed to this repo — clone it next to the sources first:

```sh
git clone --depth 1 --branch 8.0.13 https://github.com/juce-framework/JUCE.git JUCE

cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release
```

The build copies `Chieng Saturation.component` into
`~/Library/Audio/Plug-Ins/Components/` automatically.

## Validate (optional)

```sh
auval -v aufx Sat1 Chng
```

## Use in Logic Pro

Insert it on a track via **Audio Units → Chieng → Chieng Saturation**. If it doesn't show up,
open **Logic Pro → Settings → Plug-in Manager** and **Reset & Rescan Selection**.

A standalone build for quick testing (no DAW) is at
`build/ChiengSaturation_artefacts/Release/Standalone/Chieng Saturation.app`.

## Layout

| Path | Purpose |
|------|---------|
| `CMakeLists.txt` | Plugin definition (AU + Standalone, plugin codes, auto-install) |
| `Source/PluginProcessor.*` | DSP — the `tanh` saturation and the Drive parameter |
| `Source/PluginEditor.*` | UI — the rotary Drive knob |
