#pragma once

#include "DataModel.h"

namespace StudioPatchbay
{

// Parses Cubase XML preference files into the application data model.
// All methods are pure functions: XML in, data out, no side effects.
// Uses juce::XmlDocument for cross-platform XML parsing.
struct CubaseConfigParser
{
    // Parse External Plugins.xml -> instruments + FX devices with channel mappings.
    static ParsedDevices parseExternalPlugins (const juce::File& xmlFile);

    // Parse Port Setup.xml -> detected port ranges (MADI, ADAT, AES, etc.).
    // Tries the rich ASIO format first, falls back to flat CoreAudio format.
    static std::vector<DetectedPort> parsePortSetup (const juce::File& xmlFile);

    // Parse Presets/ControlRoom/Default.xml -> monitor bus configurations.
    static std::vector<ControlRoomBus> parseControlRoom (const juce::File& xmlFile);

private:
    // Shared helper: extract channel number from a Cubase connection value string.
    // Format: "prefix|...|HDSPe Device NN" -> NN
    static int parseChannelFromConnectionValue (const juce::String& value);

    // Shared helper: parse channels from a bus list element (Dispatcher Busses / Receiver Busses).
    static std::vector<int> parseChannelsFromBusList (const juce::XmlElement* busList);

    // Port Setup: rich format with PortDescriptors (ASIO).
    static std::vector<DetectedPort> parseAggregateBusList (const juce::XmlElement* root);

    // Port Setup: fallback flat format (CoreAudio).
    static std::vector<DetectedPort> parseSettingsListFallback (const juce::XmlElement* root);

    // Port Setup helper: strip trailing indices/LR markers from family name.
    // "MADI 1" -> "MADI", "AES L" -> "AES"
    static juce::String cleanFamilyName (const juce::String& raw);
};

} // namespace StudioPatchbay
