#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace StudioPatchbay
{

// The user's interface choices, kept between launches in
// <app data>/StudioPatchbay.settings (an XML properties file).
struct AppSettings
{
    // Version of the settings file. Raise it when a key changes meaning, and
    // read the old keys in load().
    //   1: darkMode (bool), uiScale, recentExports
    //   2: appearance and language replace darkMode; window size and the last
    //      Cubase version are kept
    static constexpr int formatVersion = 2;

    juce::String appearance { "system" };   // system | light | dark
    juce::String language { "system" };     // system | en | de
    double uiScale = 1.0;
    double contentScale = 1.0;              // the views' own zoom, 0.6 .. 2.0
    int windowWidth = 0;                    // logical pixels; 0 = default size
    int windowHeight = 0;
    int lastCubaseVersion = 0;              // 0 = none chosen yet
    juce::StringArray recentExports;        // full paths, newest first

    static constexpr int maxRecentExports = 3;
    void rememberExport (const juce::File&);

    static AppSettings load();
    void save() const;

    // The same, on a given file (for tests).
    static AppSettings load (const juce::File&);
    void save (const juce::File&) const;
};

} // namespace StudioPatchbay
