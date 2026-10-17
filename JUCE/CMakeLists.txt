cmake_minimum_required(VERSION 3.24)

# Universal binary (arm64 + x86_64) on macOS unless explicitly overridden.
# Must be set before project().
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

# Minimum macOS 11.0 (Big Sur) – first Apple-Silicon OS.
# Must be set before project().
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

project(StudioPatchbay VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.15.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE Catch2)


# ---------------------------------------------------------------------------
# StudioPatchbay – standalone GUI application (no plugin wrapper).
juce_add_gui_app(StudioPatchbay
    COMPANY_NAME "Jan Nordus"
    BUNDLE_ID "com.jannordus.studiopatchbay"
    PRODUCT_NAME "StudioPatchbay"
)

# Bundled UI typeface: IBM Plex Sans (SIL Open Font License 1.1).
# Identical rendering on macOS + Windows; OFL permits embedding.
juce_add_binary_data(StudioPatchbayAssets
    SOURCES
        Resources/Fonts/IBMPlexSans-Regular.ttf
        Resources/Fonts/IBMPlexSans-Medium.ttf
        Resources/Fonts/IBMPlexSans-SemiBold.ttf
        Resources/Fonts/IBMPlexSans-Bold.ttf
        THIRD_PARTY_LICENSES.txt
)

# ---------------------------------------------------------------------------
# Source files.
# Non-UI core sources shared between the app and unit tests.
set(SP_CORE_SOURCES
    Source/Model/CubaseConfigParser.cpp
    Source/Model/PlatformPaths.cpp
    Source/Export/PdfWriter.cpp
)

set(SP_SOURCES
    Source/Main.cpp
    Source/MainWindow.cpp
    Source/AppContentComponent.cpp
    Source/AppState.cpp
    Source/UI/Fonts.cpp
    Source/UI/OS9MenuBar.cpp
    Source/UI/SidebarPanel.cpp
    Source/UI/ViewSwitcher.cpp
    Source/UI/DeviceCardView.cpp
    Source/UI/CardsView.cpp
    Source/UI/PatchbayGridView.cpp
    Source/UI/ControlRoomView.cpp
    Source/UI/SignalFlowView.cpp
    Source/UI/DeviceDetailPanel.cpp
    Source/UI/AboutDialog.cpp
    Source/Export/ViewExporter.cpp
    Source/Model/AppSettings.cpp
    Source/Design/PlatinumLookAndFeel.cpp
    ${SP_CORE_SOURCES}
)

target_sources(StudioPatchbay PRIVATE ${SP_SOURCES})

target_include_directories(StudioPatchbay PRIVATE
    Source
)

target_compile_definitions(StudioPatchbay PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    SP_VERSION_STRING="${PROJECT_VERSION}"
)

target_link_libraries(StudioPatchbay
    PRIVATE
        StudioPatchbayAssets
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

# ---------------------------------------------------------------------------
# Unit tests (ctest): parser logic, later state/migrations.
enable_testing()

juce_add_console_app(StudioPatchbayTests PRODUCT_NAME "StudioPatchbayTests")
target_sources(StudioPatchbayTests PRIVATE
    Tests/PlaceholderTest.cpp
    Tests/ParserTests.cpp
    Tests/PlatformPathsTests.cpp
    Tests/PdfWriterTests.cpp
    ${SP_CORE_SOURCES}
)
target_include_directories(StudioPatchbayTests PRIVATE Source)
target_compile_definitions(StudioPatchbayTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)
target_link_libraries(StudioPatchbayTests
    PRIVATE
        Catch2::Catch2WithMain
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(StudioPatchbayTests)

# ---------------------------------------------------------------------------
# Warnings as errors on our own sources only (JUCE module sources stay untouched).
if(MSVC)
    set(SP_WARNING_FLAGS "/W4")
else()
    set(SP_WARNING_FLAGS "-Wall;-Wextra;-Wpedantic;-Werror")
endif()
set_source_files_properties(
    ${SP_SOURCES}
    Tests/PlaceholderTest.cpp
    Tests/ParserTests.cpp
    Tests/PlatformPathsTests.cpp
    Tests/PdfWriterTests.cpp
    PROPERTIES COMPILE_OPTIONS "${SP_WARNING_FLAGS}"
)
