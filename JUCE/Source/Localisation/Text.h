#pragma once

#include <juce_core/juce_core.h>

namespace StudioPatchbay::Text
{

// The interface language: "system", "en" or "de". "system" follows the
// operating system and falls back to English.
void setLanguagePreference (const juce::String&);
juce::String languagePreference();
bool german();

// The text for a key, from the product catalogue first and the shared one
// second, in the chosen language, then in English. A missing key returns the
// key itself, so it shows up instead of an empty label.
juce::String tr (const juce::String& key);

} // namespace StudioPatchbay::Text
