#pragma once

#include "PhysicalDevice.h"
#include "AudioPorts.h"

#include <optional>

namespace StudioPatchbay
{

// The path of a Cubase device through the studio, as the Swift app's detail
// panel shows it: Cubase, the interface port, converters, the hardware and
// back. Nodes carry their facts; the view words and draws them.
namespace SignalFlow
{
// The physical devices a device's channels pass through in one direction,
// in channel order, each with the channels it carries.
struct DeviceIO
{
    PhysicalDevice device;
    std::vector<int> channels;
    ChannelDirection direction;
    juce::String fullLabel() const;   // "ADA8  ·  I/O 1–2"
};
std::vector<DeviceIO> deviceIOs (const std::vector<int>& channels, ChannelDirection, const std::vector<PhysicalDevice>&);

struct Node
{
    enum class Kind
    {
        cubaseInstrumentTrack, midiOut, instrument, noChannels,
        analogOut,          // hardware out -> converter (text: device label)
        interfaceReceive,   // the port, receiving (text: interface, detail: channels)
        cubaseReceived,
        cubaseSent, interfaceSend, analogIn, hardware,
        cubaseFxSend, noSendChannels, cubaseFxReturn
    };
    Kind kind;
    juce::String text;
    juce::String detail;
    bool noConverter = false;          // the port's channels pass no converter
    std::optional<int> jumpChannel;    // click shows this channel in the patchbay
};

std::vector<Node> build (const ParsedDevice&, const std::vector<PhysicalDevice>&, const std::vector<AudioPort>&);
} // namespace SignalFlow

} // namespace StudioPatchbay
