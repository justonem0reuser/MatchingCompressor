cmake_minimum_required(VERSION 3.22)
project("MatchingCompressor")

include(FetchContent)

# Fetch JUCE
set(JUCE_VERSION "8.0.4")
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${JUCE_VERSION}
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Fetch AlgLib
set(ALGLIB_VERSION "4.00.0")
FetchContent_Declare(
    AlgLib
    URL https://www.alglib.net/translator/re/alglib-${ALGLIB_VERSION}.cpp.gpl.zip
)
FetchContent_MakeAvailable(AlgLib)
set(ALGLIB "${alglib_SOURCE_DIR}/src")

set(BUILD_STANDALONE_DEFAULT ON)
if(DEFINED ENV{CI})
    set(BUILD_STANDALONE_DEFAULT OFF)
endif()
option(BUILD_STANDALONE "Build Standalone" ${BUILD_STANDALONE_DEFAULT})

include_directories(${ALGLIB})

set(JUCE_PLUGIN_FORMATS VST3 AU AUv3 AAX)
if(BUILD_STANDALONE)
    list(APPEND JUCE_PLUGIN_FORMATS Standalone)
endif()

set(PLUGIN_VERSION_DEFAULT "0.1.0")
if(NOT DEFINED VERSION)
    set(VERSION ${PLUGIN_VERSION_DEFAULT})
endif()

juce_add_plugin(MatchingCompressor
  VERSION ${VERSION}
  PRODUCT_NAME "MatchingCompressor"
  COMPANY_NAME "Just1m0reApp"
  COMPANY_WEBSITE "https://github.com/justonem0reuser"

  PLUGIN_MANUFACTURER_CODE Jmor
  PLUGIN_CODE Mtcp

  FORMATS ${JUCE_PLUGIN_FORMATS}

  AU_MAIN_TYPE kAudioUnitType_Effect

  VST3_CATEGORIES
    "Analyzer"
    "Dynamics"
    "Fx"
    "Mastering"
  VST2_CATEGORY "kPlugCategEffect"
)

juce_generate_juce_header(MatchingCompressor)

target_compile_definitions(MatchingCompressor
  PUBLIC
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
)

if (MSVC)
    target_compile_options(MatchingCompressor PRIVATE
        $<$<CONFIG:Release>:/O2 /DNDEBUG /GL>
    )

    target_link_options(MatchingCompressor PRIVATE
        $<$<CONFIG:Release>:/OPT:REF /OPT:ICF /INCREMENTAL:NO /LTCG>
    )
endif()

if(UNIX AND NOT APPLE)
    target_link_options(MatchingCompressor PRIVATE
        -static-libstdc++
        -static-libgcc
)
endif()

target_sources(MatchingCompressor
  PRIVATE
    "${ALGLIB}/alglibinternal.cpp"
    "${ALGLIB}/alglibmisc.cpp"
    "${ALGLIB}/ap.cpp"
    "${ALGLIB}/dataanalysis.cpp"
    "${ALGLIB}/diffequations.cpp"
    "${ALGLIB}/fasttransforms.cpp"
    "${ALGLIB}/integration.cpp"
    "${ALGLIB}/interpolation.cpp"
    "${ALGLIB}/kernels_avx2.cpp"
    "${ALGLIB}/kernels_fma.cpp"
    "${ALGLIB}/kernels_sse2.cpp"
    "${ALGLIB}/linalg.cpp"
    "${ALGLIB}/optimization.cpp"
    "${ALGLIB}/solvers.cpp"
    "${ALGLIB}/specialfunctions.cpp"
    "${ALGLIB}/statistics.cpp"
    "Source/Components/ComboBoxWithAttachment.cpp"
    "Source/Components/ComponentInitializerHelper.cpp"
    "Source/Components/CurvePlotComponent.cpp"
    "Source/Components/DataReceiver.cpp"
    "Source/Components/PlotWithCoordinateSystemComponent.cpp"
    "Source/Components/SettingsComponent.cpp"
    "Source/Components/SettingsWithDataReceiver.cpp"
    "Source/Components/SliderWithAttachment.cpp"
    "Source/Components/ToggleButtonWithAttachment.cpp"
    "Source/Controllers/DataReceiverController.cpp"
    "Source/Controllers/MainController.cpp"
    "Source/Controllers/MatchController.cpp"
    "Source/DSP/DataCollector.cpp"
    "Source/DSP/DynamicShaper.cpp"
    "Source/Data/MatchingData.cpp"
    "Source/MatchWindow.cpp"
    "Source/ParamsCalculator/CompParamsCalculator.cpp"
    "Source/ParamsCalculator/CompParamsCalculatorEnv.cpp"
    "Source/ParamsCalculator/CompParamsCalculatorEnv1D.cpp"
    "Source/ParamsCalculator/CompParamsCalculatorEnv2D.cpp"
    "Source/ParamsCalculator/CompParamsCalculatorNoEnv.cpp"
    "Source/ParamsCalculator/QuantilesCalculator.cpp"
    "Source/PluginEditor.cpp"
    "Source/PluginProcessor.cpp"
    "Source/MCLookAndFeel.cpp"
    "Source/Utilities/AudioFileReader.cpp"
    "Source/Utilities/CurveCalculator.cpp"
)

juce_add_binary_data(MatchingCompressor_BinaryData
  SOURCES
    "Assets/background.jpg"
    "Assets/plotBackground.jpg"
    "Assets/slider.png"
    "Assets/toggle-off.png"
    "Assets/toggle-on.png"
    "Assets/tool-hover.png"
    "Assets/tool.png"
)

target_link_libraries(MatchingCompressor
  PRIVATE
    MatchingCompressor_BinaryData
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Installation targets
# Usage: cmake --install build

if(APPLE)
    install(CODE "
        file(INSTALL
            DESTINATION \"\$ENV{HOME}/Library/Audio/Plug-Ins/Components\"
            TYPE DIRECTORY
            FILES \"${CMAKE_BINARY_DIR}/MatchingCompressor_artefacts/\$<CONFIG>/AU/MatchingCompressor.component\"
        )
        file(INSTALL
            DESTINATION \"\$ENV{HOME}/Library/Audio/Plug-Ins/VST3\"
            TYPE DIRECTORY
            FILES \"${CMAKE_BINARY_DIR}/MatchingCompressor_artefacts/\$<CONFIG>/VST3/MatchingCompressor.vst3\"
        )
    ")
elseif(UNIX)
    install(CODE "
        file(INSTALL
            DESTINATION \"\$ENV{HOME}/.vst3\"
            TYPE DIRECTORY
            FILES \"${CMAKE_BINARY_DIR}/MatchingCompressor_artefacts/\$<CONFIG>/VST3/MatchingCompressor.vst3\"
        )
    ")
elseif(WIN32)
    install(CODE "
        file(INSTALL
            DESTINATION \"\$ENV{PROGRAMFILES}/Common Files/VST3\"
            TYPE DIRECTORY
            FILES \"${CMAKE_BINARY_DIR}/MatchingCompressor_artefacts/\$<CONFIG>/VST3/MatchingCompressor.vst3\"
        )
    ")
endif()
