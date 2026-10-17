#pragma once

#include "DataModel.h"

#include <optional>

namespace StudioPatchbay
{

// Detects Cubase installations by scanning the per-user preference folders
// Cubase writes. Returns installations sorted descending by version number
// (latest first). Only versions >= 12 are included.
//
// macOS:   ~/Library/Preferences/Cubase NN/
// Windows: %APPDATA%\Steinberg\Cubase NN_64
struct PlatformPaths
{
    // Scan this platform's preference folder (empty where Cubase does not run).
    static std::vector<CubaseInstallation> detectInstallations();

    // Scan an explicit base folder. Accepts both folder spellings, so the
    // detection logic is testable on any platform.
    static std::vector<CubaseInstallation> detectInstallationsIn (const juce::File& baseDir);

    // "Cubase 14" (macOS) or "Cubase 14_64" (Windows) -> 14. Anything else,
    // including versions below 12, -> nullopt.
    static std::optional<int> versionFromFolderName (const juce::String& folderName);

    // Return the application data directory for persistent storage
    // (physical devices, port labels, settings).
    // macOS: ~/Library/Application Support/StudioPatchbay/
    // Windows: %APPDATA%\StudioPatchbay\, elsewhere ~/.config/StudioPatchbay/
    static juce::File getAppDataDirectory();

    static constexpr int minimumVersion = 12;

private:
    // The base folder Cubase stores its per-version preference folders in;
    // a non-existent File on platforms Cubase does not run on.
    static juce::File getCubasePreferencesBase();
};

} // namespace StudioPatchbay
