cmake_minimum_required(VERSION 3.22)
project(SUZGEC VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(SZ_BUILD_PLUGIN "Build the JUCE plugin" ON)
option(SZ_BUILD_TESTS "Build the headless core tests" ON)
set(SZ_JUCE_DIR "" CACHE PATH "Local JUCE checkout (fetched automatically when empty)")

# ---------------------------------------------------------------- core (framework independent)
add_library(sz_core STATIC core/FilterBank.cpp)
target_include_directories(sz_core PUBLIC core)

if(SZ_BUILD_TESTS)
    enable_testing()
    add_executable(sz_tests tests/test_core.cpp)
    target_link_libraries(sz_tests PRIVATE sz_core)
    add_test(NAME sz_tests COMMAND sz_tests)
endif()

# ---------------------------------------------------------------- plugin
if(SZ_BUILD_PLUGIN)
    if(SZ_JUCE_DIR)
        add_subdirectory(${SZ_JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW ON)
        FetchContent_MakeAvailable(JUCE)
    endif()

    if(APPLE)
        set(SZ_FORMATS VST3 AU Standalone)
    elseif(WIN32)
        set(SZ_FORMATS VST3 Standalone)
    else()
        set(SZ_FORMATS VST3)
    endif()

    juce_add_plugin(Suzgec
        COMPANY_NAME "Suzgec"
        BUNDLE_ID "com.suzgec.spectralfilter"
        PLUGIN_MANUFACTURER_CODE Szgc
        PLUGIN_CODE Szg1
        FORMATS ${SZ_FORMATS}
        PRODUCT_NAME "Suzgec"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx Filter
        AU_MAIN_TYPE kAudioUnitType_MusicEffect)

    target_sources(Suzgec PRIVATE
        plugin/PluginProcessor.cpp
        plugin/PluginEditor.cpp)

    target_compile_definitions(Suzgec PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    if(UNIX AND NOT APPLE)
        target_compile_definitions(Suzgec PUBLIC JUCE_ALSA=0 JUCE_JACK=0)
    endif()

    target_link_libraries(Suzgec
        PRIVATE
            sz_core
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_gui_basics
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endif()

# ---------------------------------------------------------------- arayüz ekran görüntüsü aracı (CI)
option(SZ_BUILD_SNAPSHOT "Build the UI snapshot tool" OFF)
if(SZ_BUILD_PLUGIN AND SZ_BUILD_SNAPSHOT)
    juce_add_console_app(SuzgecSnapshot PRODUCT_NAME "SuzgecSnapshot")
    target_sources(SuzgecSnapshot PRIVATE
        tools/snapshot.cpp
        plugin/PluginProcessor.cpp
        plugin/PluginEditor.cpp)
    target_include_directories(SuzgecSnapshot PRIVATE plugin)
    target_compile_definitions(SuzgecSnapshot PRIVATE
        JucePlugin_Name="Suzgec"
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(SuzgecSnapshot PRIVATE
        sz_core
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_recommended_config_flags)
endif()
