#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace StudioPatchbay
{

// The user's interface choices, kept between launches in
// <app data>/StudioPatchbay.settings (an XML properties file).
struct AppSettings
{
    // Version of the settings file. Raise it when a key changes meaning.
    static constexpr int formatVersion = 1;

    bool darkMode = true;
    double uiScale = 1.0;

    static AppSettings load();
    void save() const;
};

} // namespace StudioPatchbay
