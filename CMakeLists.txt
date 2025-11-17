cmake_minimum_required(VERSION 3.22)

project(OrchestraSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS deployment / arch model
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum OS X deployment version" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary" FORCE)
endif()

# JUCE as subdirectory (user must clone JUCE 7.0.12 to external/JUCE)
add_subdirectory(external/JUCE)

# Shared sources used by both standalone and plugin
set(ORCHESTRASYNTH_SHARED_SOURCES

    src/Engine/OrchestraSynthEngine.h

    src/DSP/Oversampler.h
    src/DSP/ConvolutionEngine.h
    src/DSP/ImpulseResponseLoader.h

    src/Systems/PresetManager.h
    src/Systems/PerformanceMonitor.h
    src/Systems/Logger.h
    src/Systems/CrashReporter.h

    src/Platform/AVAudioEngineManager.h
    src/Platform/AVAudioEngineManager.mm

    src/UI/SectionStripComponent.h
    src/UI/SectionStripComponent.cpp
    src/UI/PresetBar.h
    src/UI/PresetBar.cpp
    src/UI/MixerComponent.h
    src/UI/MixerComponent.cpp
)

# ===========================
# Standalone GUI application
# ===========================

juce_add_gui_app(OrchestraSynth
    PRODUCT_NAME "OrchestraSynth"
    COMPANY_NAME "YourCompany"
    BUNDLE_ID    com.yourcompany.OrchestraSynth
)

# Ensure JuceHeader.h is generated before any JUCE-dependent compilation
juce_generate_juce_header(OrchestraSynth)

target_sources(OrchestraSynth PRIVATE
    src/App/Main.cpp
    src/App/OrchestraSynthApplication.h
    src/App/OrchestraSynthApplication.cpp

    ${ORCHESTRASYNTH_SHARED_SOURCES}
)

target_link_libraries(OrchestraSynth
    PRIVATE
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
)

if(APPLE)
    target_link_libraries(OrchestraSynth
        PRIVATE
            "-framework AVFoundation"
            "-framework AudioToolbox"
            "-framework CoreAudio"
    )
endif()

# ===========================
# Plugin target (VST3 + AU)
# ===========================

juce_add_plugin(OrchestraSynthPlugin
    COMPANY_NAME        "YourCompany"
    BUNDLE_ID           com.yourcompany.OrchestraSynthPlugin
    IS_SYNTH            TRUE
    NEEDS_MIDI_INPUT    TRUE
    NEEDS_MIDI_OUTPUT   FALSE
    IS_MIDI_EFFECT      FALSE
    VST3                TRUE
    AU                  TRUE
    AU_SANDBOX_SAFE     TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS             VST3 AU
    PRODUCT_NAME        "OrchestraSynth"
)

juce_generate_juce_header(OrchestraSynthPlugin)

target_sources(OrchestraSynthPlugin PRIVATE
    src/Plugin/PluginProcessor.h
    src/Plugin/PluginProcessor.cpp
    src/Plugin/PluginEditor.h
    src/Plugin/PluginEditor.cpp

    ${ORCHESTRASYNTH_SHARED_SOURCES}
)

target_link_libraries(OrchestraSynthPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
)

if(APPLE)
    target_link_libraries(OrchestraSynthPlugin
        PRIVATE
            "-framework AVFoundation"
            "-framework AudioToolbox"
            "-framework CoreAudio"
    )
endif()

# ===========================
# Common configuration
# ===========================

foreach(target OrchestraSynth OrchestraSynthPlugin)
    target_compile_definitions(${target}
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        target_compile_options(${target}
            PRIVATE
                -Wall -Wextra -Wpedantic
        )
    endif()

    # LTO / IPO for enterprise-grade performance builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endforeach()
