# Rr Harmonizer (VST3)

A JUCE 8 / CMake source project for a Windows VST3 vocal-harmonizer prototype. The editor has been laid out to closely follow the supplied reference screenshot: dark grey panels, lime accents, top transport-style toolbar, Input / Harmony / Voices / Tone / Output sections, voice controls, and output meter.

## Repository layout

```text
CMakeLists.txt
README.md
QA-CHECKLIST.md
.gitignore
Source/
  PluginProcessor.cpp
  PluginProcessor.h
  PluginEditor.cpp
  PluginEditor.h
.github/workflows/build.yml
```

Upload the *contents* of the extracted ZIP to the repository root. Do not upload an extra enclosing folder or only the ZIP file.

## Build on GitHub

The workflow checks out JUCE 8.0.8, configures with CMake and builds the VST3 target on Windows. Open **Actions → Build VST3** and download `Rr-Harmonizer-VST3-Windows` from a successful run.

## Important status

This is source code for a prototype, not a verified production plugin. The interface is based on the supplied screenshot but cannot be pixel-identical to the original proprietary plugin. Some controls are currently presentation/configuration controls rather than fully implemented harmonizer features; Undo/Redo are shown but disabled because no undo history is configured. Run the GitHub Actions build and test the resulting plugin in a VST3 host before relying on it.

## Local build

Install Visual Studio with the C++ desktop workload and CMake, then run:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target RrHarmonizer_VST3
```
