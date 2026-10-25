cmake_minimum_required(VERSION 3.24)

project(Librairy
    VERSION 0.1.0
    DESCRIPTION "Professional Audio Library Manager"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Options
# =============================================================================
option(LIBRAIRY_ENABLE_AI     "Build with ONNX Runtime AI inference (Stage 2)" OFF)
option(LIBRAIRY_BUILD_TESTS   "Build unit tests"                               ON)

# =============================================================================
# Dependencies — JUCE (fetched automatically)
# =============================================================================
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# =============================================================================
# SQLite (bundled single-file amalgamation)
# =============================================================================
add_library(sqlite3 STATIC
    third_party/sqlite/sqlite3.c
)
target_include_directories(sqlite3 PUBLIC third_party/sqlite)
target_compile_definitions(sqlite3 PUBLIC
    SQLITE_ENABLE_FTS5=1
    SQLITE_ENABLE_RTREE=1
    SQLITE_ENABLE_JSON1=1
)
# Silence warnings in third-party code
target_compile_options(sqlite3 PRIVATE -w)

# =============================================================================
# Core Library (static lib for testability)
# =============================================================================
set(LIBRAIRY_CORE_SOURCES
    # Core
    src/core/Library.cpp
    src/core/Scanner.cpp
    src/core/Database.cpp
    src/core/SearchEngine.cpp

    # Metadata
    src/metadata/MetadataReader.cpp
    src/metadata/MetadataWriter.cpp
    src/metadata/MetadataModel.cpp

    # Audio
    src/audio/AudioEngine.cpp
    src/audio/WaveformGenerator.cpp

    # Transfer
    src/transfer/FileExporter.cpp
)

add_library(librairy_core STATIC ${LIBRAIRY_CORE_SOURCES})

target_include_directories(librairy_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(librairy_core
    PUBLIC
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_audio_devices
        juce::juce_audio_utils
        sqlite3
)

target_compile_definitions(librairy_core
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_MODAL_LOOPS_PERMITTED=1
)

# =============================================================================
# GUI Application
# =============================================================================
juce_add_gui_app(Librairy
    PRODUCT_NAME       "Librairy"
    COMPANY_NAME       "Librairy"
    BUNDLE_ID          "com.librairy.app"
    ICON_BIG           ""
    VERSION            "${PROJECT_VERSION}"
)

target_sources(Librairy PRIVATE
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/ResultsTable.cpp
    src/ui/MetadataPanel.cpp
    src/ui/WaveformView.cpp
    src/ui/SearchBar.cpp
    src/ui/LibrarySidebar.cpp
)

target_link_libraries(Librairy
    PRIVATE
        librairy_core
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(Librairy PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Librairy,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Librairy,JUCE_VERSION>"
)

# =============================================================================
# AI Module (Stage 2 — optional)
# =============================================================================
if(LIBRAIRY_ENABLE_AI)
    find_package(onnxruntime REQUIRED)

    target_sources(librairy_core PRIVATE
        src/ai/InferenceEngine.cpp
        src/ai/EmbeddingIndex.cpp
        src/ai/AutoTagger.cpp
    )

    target_link_libraries(librairy_core PUBLIC onnxruntime::onnxruntime)
    target_compile_definitions(librairy_core PUBLIC LIBRAIRY_AI_ENABLED=1)
endif()

# =============================================================================
# Tests
# =============================================================================
if(LIBRAIRY_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(librairy_tests
        tests/core/DatabaseTest.cpp
        tests/core/ScannerTest.cpp
        tests/core/SearchEngineTest.cpp
        tests/metadata/MetadataReaderTest.cpp
    )

    target_link_libraries(librairy_tests
        PRIVATE
            librairy_core
            Catch2::Catch2WithMain
    )

    include(CTest)
    include(Catch)
    catch_discover_tests(librairy_tests)
endif()

