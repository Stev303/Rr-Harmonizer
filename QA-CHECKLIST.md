# QA checklist

- [ ] GitHub repository root contains `CMakeLists.txt`.
- [ ] `Source/PluginProcessor.cpp`, `Source/PluginProcessor.h`, `Source/PluginEditor.cpp`, and `Source/PluginEditor.h` are in the `Source` folder.
- [ ] GitHub Actions configures CMake without errors.
- [ ] GitHub Actions builds `RrHarmonizer_VST3` without errors.
- [ ] The workflow uploads a `.vst3` artifact.
- [ ] The plugin loads in a VST3 host.
- [ ] Audio passes through and bypass works.
- [ ] Test every control; some UI controls are prototype-only and need further DSP/UI wiring.
- [ ] Compare UI dimensions and spacing against the supplied reference screenshot.
