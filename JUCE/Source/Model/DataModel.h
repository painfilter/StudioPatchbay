#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <map>
#include <set>
#include <optional>

namespace StudioPatchbay
{

// ============================================================================
// Enums – mirror the Swift originals exactly for data compatibility
// ============================================================================

enum class ConnectorType
{
    madi, adat, aes, spdif, dante, usb, thunderbolt, analogLine, analogMic, other
};

enum class DeviceDirectionality
{
    bidirectional, sendOnly, recvOnly
};

enum class DeviceType
{
    converter, audioInterface, other
};

enum class ChannelDirection
{
    send, recv
};

// ============================================================================
// ParsedDevice – one logical Cubase External Effect / Instrument
// ============================================================================

struct ParsedDevice
{
    enum class Type
    {
        instrument,
        fx,
        sendOnly
    };

    juce::String name;
    Type type;
    std::vector<int> sendChannels; // sorted ascending
    std::vector<int> recvChannels; // sorted ascending
    juce::String midiDevice;

    // --- Computed helpers ---

    std::vector<int> allChannels() const
    {
        std::set<int> merged (sendChannels.begin(), sendChannels.end());
        merged.insert (recvChannels.begin(), recvChannels.end());
        return { merged.begin(), merged.end() };
    }

    int channelCount() const { return static_cast<int> (allChannels().size()); }
    bool isStereo()    const { return channelCount() == 2; }
    bool isMono()      const { return channelCount() == 1; }

    bool hasMismatchedChannels() const
    {
        return ! sendChannels.empty() && ! recvChannels.empty()
               && sendChannels != recvChannels;
    }
};

// ============================================================================
// ParsedDevices – collection result from ExternalPlugins parsing
// ============================================================================

struct ParsedDevices
{
    std::vector<ParsedDevice> instruments;
    std::vector<ParsedDevice> fxDevices;

    std::vector<ParsedDevice> all() const
    {
        std::vector<ParsedDevice> result;
        result.reserve (instruments.size() + fxDevices.size());
        result.insert (result.end(), instruments.begin(), instruments.end());
        result.insert (result.end(), fxDevices.begin(), fxDevices.end());
        return result;
    }

    // Map channel number -> device name for send or recv direction.
    std::map<int, juce::String> buildChannelMap (ChannelDirection dir) const
    {
        std::map<int, juce::String> result;
        for (auto& d : all())
        {
            auto& channels = (dir == ChannelDirection::send) ? d.sendChannels : d.recvChannels;
            for (auto ch : channels)
                result[ch] = d.name;
        }
        return result;
    }

    int usedChannelCount() const
    {
        std::set<int> channels;
        for (auto& d : all())
        {
            channels.insert (d.sendChannels.begin(), d.sendChannels.end());
            channels.insert (d.recvChannels.begin(), d.recvChannels.end());
        }
        return static_cast<int> (channels.size());
    }
};

// ============================================================================
// DetectedPort – one contiguous port range from Port Setup
// ============================================================================

struct DetectedPort
{
    juce::String family;
    ChannelDirection direction;
    int channelStart;
    int channelEnd; // inclusive
    bool isApproximate = false;

    juce::String id() const
    {
        return juce::String (direction == ChannelDirection::send ? "O" : "I")
               + "-" + family + "-" + juce::String (channelStart);
    }

    int channelCount() const { return channelEnd - channelStart + 1; }
};

// ============================================================================
// PhysicalDevice – user-configured hardware converter/interface
// ============================================================================

struct PhysicalDevice
{
    juce::Uuid id;
    juce::String name;
    juce::String shortName;
    ConnectorType portColor = ConnectorType::other;
    DeviceType deviceType = DeviceType::other;
    int physicalChannelCount = 0;
    int sendChannelStart = 0;
    int recvChannelStart = 0;
    DeviceDirectionality directionality = DeviceDirectionality::bidirectional;
    juce::String customColorHex;
};

// ============================================================================
// ControlRoomBus – one Control Room output bus
// ============================================================================

struct ControlRoomBus
{
    enum class Kind
    {
        studio,
        phones,
        speaker
    };

    int index;
    juce::String name;
    Kind kind;
    std::vector<int> channels;
};

// ============================================================================
// CubaseInstallation – one detected Cubase version with preference paths
// ============================================================================

struct CubaseInstallation
{
    int version;
    juce::File prefsDir;

    juce::File externalPluginsXml()  const { return prefsDir.getChildFile ("External Plugins.xml"); }
    juce::File portSetupXml()        const { return prefsDir.getChildFile ("Port Setup.xml"); }
    juce::File controlRoomXml()      const { return prefsDir.getChildFile ("Presets")
                                                            .getChildFile ("ControlRoom")
                                                            .getChildFile ("Default.xml"); }

    juce::String displayName() const { return "Cubase " + juce::String (version); }

    bool xmlExists()            const { return externalPluginsXml().existsAsFile(); }
    bool portSetupXmlExists()   const { return portSetupXml().existsAsFile(); }
    bool controlRoomXmlExists() const { return controlRoomXml().existsAsFile(); }
};

} // namespace StudioPatchbay
