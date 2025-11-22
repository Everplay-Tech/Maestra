cmake_minimum_required(VERSION 3.22)

project(OrchestraSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===========================
# Production Build Configuration
# ===========================

# Allow environment variables to override company/product settings
set(ORCHESTRASYNTH_COMPANY_NAME "$ENV{COMPANY_NAME}" CACHE STRING "Company name")
if(NOT ORCHESTRASYNTH_COMPANY_NAME)
    set(ORCHESTRASYNTH_COMPANY_NAME "YourCompany")
endif()

set(ORCHESTRASYNTH_PRODUCT_NAME "$ENV{PRODUCT_NAME}" CACHE STRING "Product name")
if(NOT ORCHESTRASYNTH_PRODUCT_NAME)
    set(ORCHESTRASYNTH_PRODUCT_NAME "OrchestraSynth")
endif()

set(ORCHESTRASYNTH_BUNDLE_ID_PREFIX "$ENV{BUNDLE_ID_PREFIX}" CACHE STRING "Bundle ID prefix")
if(NOT ORCHESTRASYNTH_BUNDLE_ID_PREFIX)
    set(ORCHESTRASYNTH_BUNDLE_ID_PREFIX "com.yourcompany.orchestrasynth")
endif()

# Build number support (useful for CI/CD)
set(ORCHESTRASYNTH_BUILD_NUMBER "$ENV{BUILD_NUMBER}" CACHE STRING "Build number")
if(NOT ORCHESTRASYNTH_BUILD_NUMBER)
    set(ORCHESTRASYNTH_BUILD_NUMBER "0")
endif()

# Full version string with build number
set(ORCHESTRASYNTH_VERSION_FULL "${PROJECT_VERSION}.${ORCHESTRASYNTH_BUILD_NUMBER}")

message(STATUS "OrchestraSynth Configuration:")
message(STATUS "  Company:      ${ORCHESTRASYNTH_COMPANY_NAME}")
message(STATUS "  Product:      ${ORCHESTRASYNTH_PRODUCT_NAME}")
message(STATUS "  Version:      ${ORCHESTRASYNTH_VERSION_FULL}")
message(STATUS "  Bundle ID:    ${ORCHESTRASYNTH_BUNDLE_ID_PREFIX}")

# macOS deployment / arch model
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum OS X deployment version" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary" FORCE)
    set(ORCHESTRASYNTH_APP_BUNDLE_DESTINATION "." CACHE STRING "Where to place the .app bundle when installing")
    set(ORCHESTRASYNTH_PLUGIN_VST3_DESTINATION "Plug-Ins/VST3" CACHE STRING "Where to place VST3 bundle when installing")
    set(ORCHESTRASYNTH_PLUGIN_AU_DESTINATION "Plug-Ins/AU" CACHE STRING "Where to place AU bundle when installing")
endif()

# Automatically fetch JUCE 7.0.12 from GitHub
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Shared sources used by both standalone and plugin
set(ORCHESTRASYNTH_SHARED_SOURCES

    src/Engine/OrchestraSynthEngine.h

    src/DSP/Oversampler.h
    src/DSP/ConvolutionEngine.h
    src/DSP/ImpulseResponseLoader.h
    src/DSP/QuantumDSPCore.h
    src/DSP/NeuralWaveguideProcessor.h
    src/DSP/StabilityGuardian.h
    src/DSP/MacroModulationMatrix.h
    src/DSP/PresetMorphingEngine.h
    src/DSP/SpectralIntelligence.h
    src/DSP/HarmonicEnhancer.h

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
    PRODUCT_NAME "${ORCHESTRASYNTH_PRODUCT_NAME}"
    COMPANY_NAME "${ORCHESTRASYNTH_COMPANY_NAME}"
    BUNDLE_ID    "${ORCHESTRASYNTH_BUNDLE_ID_PREFIX}"
    VERSION      "${ORCHESTRASYNTH_VERSION_FULL}"

    # macOS specific metadata
    COMPANY_COPYRIGHT "Copyright (c) 2024 ${ORCHESTRASYNTH_COMPANY_NAME}"
    COMPANY_WEBSITE "https://yourcompany.com"

    # Icon file (place icon at resources/icon.icns)
    ICON_BIG "${CMAKE_SOURCE_DIR}/resources/icon.icns"

    # File extensions/types this app can handle (if applicable)
    # DOCUMENT_EXTENSIONS "orchproj"
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
    COMPANY_NAME        "${ORCHESTRASYNTH_COMPANY_NAME}"
    BUNDLE_ID           "${ORCHESTRASYNTH_BUNDLE_ID_PREFIX}.plugin"
    VERSION             "${ORCHESTRASYNTH_VERSION_FULL}"
    COMPANY_COPYRIGHT   "Copyright (c) 2024 ${ORCHESTRASYNTH_COMPANY_NAME}"
    COMPANY_WEBSITE     "https://yourcompany.com"

    IS_SYNTH            TRUE
    NEEDS_MIDI_INPUT    TRUE
    NEEDS_MIDI_OUTPUT   FALSE
    IS_MIDI_EFFECT      FALSE

    VST3                TRUE
    AU                  TRUE
    AU_SANDBOX_SAFE     TRUE

    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS             VST3 AU
    PRODUCT_NAME        "${ORCHESTRASYNTH_PRODUCT_NAME}"

    # Plugin identifiers
    PLUGIN_MANUFACTURER_CODE "Ymco"  # 4-character unique ID (customize this!)
    PLUGIN_CODE             "Orch"  # 4-character unique ID (customize this!)

    # VST3 specific
    VST3_CATEGORIES     "Instrument|Synth"

    # AU specific
    AU_MAIN_TYPE        "kAudioUnitType_MusicDevice"
    AU_SANDBOX_SAFE     TRUE

    # Icon (same as standalone)
    ICON_BIG            "${CMAKE_SOURCE_DIR}/resources/icon.icns"

    # Description
    DESCRIPTION         "Professional orchestral synthesizer with five orchestral sections"
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

# ===========================
# Installation + packaging
# ===========================

if(APPLE)
    include(GNUInstallDirs)

    # App bundle
    install(TARGETS OrchestraSynth
        BUNDLE DESTINATION "${ORCHESTRASYNTH_APP_BUNDLE_DESTINATION}"
    )

    # Plugin bundles (JUCE handles copying to build tree; here we define install destinations for packaging)
    install(TARGETS OrchestraSynthPlugin
        LIBRARY DESTINATION "${ORCHESTRASYNTH_PLUGIN_VST3_DESTINATION}"  # VST3 bundle
        BUNDLE DESTINATION "${ORCHESTRASYNTH_PLUGIN_AU_DESTINATION}"      # AU component bundle
    )

    # CPack DragNDrop DMG configuration
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_NAME "${ORCHESTRASYNTH_PRODUCT_NAME}")
    set(CPACK_PACKAGE_VENDOR "${ORCHESTRASYNTH_COMPANY_NAME}")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "${ORCHESTRASYNTH_PRODUCT_NAME}-${PROJECT_VERSION}-macOS-universal")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional orchestral synthesizer with five orchestral sections")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://yourcompany.com")

    # DMG appearance and format
    set(CPACK_DMG_FORMAT "UDZO")  # Compressed (UDZO) or UDBZ for better compression
    set(CPACK_DMG_VOLUME_NAME "${ORCHESTRASYNTH_PRODUCT_NAME} Installer")
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/macos/dmg_setup.scpt")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/resources/dmg_background.png")
    set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK ON)

    # DMG license (optional - uncomment if you have a license file)
    # set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")

    # Additional DMG metadata
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)

    include(CPack)
endif()

# ===========================
# Development Notes
# ===========================
# Production checklist:
# 1. Customize ORCHESTRASYNTH_COMPANY_NAME and ORCHESTRASYNTH_BUNDLE_ID_PREFIX
# 2. Update PLUGIN_MANUFACTURER_CODE and PLUGIN_CODE to unique 4-char IDs
# 3. Add icon file at resources/icon.icns (1024x1024 PNG converted to ICNS)
# 4. Optional: Add DMG background at resources/dmg_background.png (540x380)
# 5. Update copyright year and company website URL
# 6. Set entitlements in config/entitlements/*.entitlements
# 7. Configure code signing in config/build.env
