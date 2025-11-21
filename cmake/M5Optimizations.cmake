# M5 Chip Optimizations for OrchestraSynth
# Automatically included during build configuration

if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    message(STATUS "Applying M5 Apple Silicon optimizations")

    # Target M5 microarchitecture
    set(M5_COMPILE_FLAGS
        -mcpu=apple-m5          # Target M5 specifically
        -mtune=apple-m5         # Tune for M5
        -march=armv9-a          # ARMv9 instruction set
    )

    # Check if compiler supports M5 flags
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-mcpu=apple-m5" COMPILER_SUPPORTS_M5)

    if(COMPILER_SUPPORTS_M5)
        message(STATUS "  [OK] M5-specific optimizations enabled")
        foreach(target OrchestraSynth OrchestraSynthPlugin)
            if(TARGET ${target})
                target_compile_options(${target} PRIVATE ${M5_COMPILE_FLAGS})
            endif()
        endforeach()
    else()
        message(STATUS "  [WARN] Compiler doesn't support M5-specific flags, using general ARM64 optimizations")
        foreach(target OrchestraSynth OrchestraSynthPlugin)
            if(TARGET ${target})
                target_compile_options(${target} PRIVATE -mcpu=apple-latest)
            endif()
        endforeach()
    endif()

    # Enable SVE/NEON optimizations
    foreach(target OrchestraSynth OrchestraSynthPlugin)
        if(TARGET ${target})
            target_compile_definitions(${target} PRIVATE
                JUCE_USE_ARM_NEON=1
                JUCE_ARM_NEON_OPTIMIZATIONS=1
            )
        endif()
    endforeach()

    # Link-time optimizations
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

    message(STATUS "  [OK] NEON optimizations enabled")
    message(STATUS "  [OK] Link-time optimization enabled")
endif()
