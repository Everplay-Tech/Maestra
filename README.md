# OrchestraSynth

OrchestraSynth is a macOS orchestral synthesizer featuring:

- Five orchestral sections: **Strings, Brass, Woodwinds, Percussion, Choir**
- Shared audio engine with deterministic MIDI handling and centralized DSP
- Convolution reverb and oversampling / anti-aliasing utilities
- Preset serialization via JUCE `ValueTree` / `DynamicObject`
- Enterprise-oriented logging, crash reporting, and performance monitoring
- A JUCE standalone app and a JUCE-based plugin (VST3, AU) sharing the same core

This repository is organized for modern JUCE + CMake builds on current macOS toolchains (macOS 14, AppleClang 17+).

---

## 1. Cloning and JUCE setup

```bash
git clone <your-orchestra-synth-repo> OrchestraSynth
cd OrchestraSynth

# Clone JUCE 7.0.12 into external/JUCE
mkdir -p external
cd external
git clone --branch 7.0.12 --depth 1 https://github.com/juce-framework/JUCE.git JUCE
cd ..
