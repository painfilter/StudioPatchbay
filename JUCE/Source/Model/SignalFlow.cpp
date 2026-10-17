#include "SignalFlow.h"

#include <algorithm>

namespace StudioPatchbay::SignalFlow
{

namespace
{
juce::String joined (const std::vector<int>& channels)
{
    juce::StringArray parts;
    for (auto ch : channels)
        parts.add (juce::String (ch));
    return parts.joinIntoString (" + ");
}

juce::String interfaceFor (const std::vector<int>& first, const std::vector<int>& second, const std::vector<AudioPort>& ports)
{
    const auto ch = ! first.empty() ? first.front() : (! second.empty() ? second.front() : 0);
    if (const auto* port = AudioPorts::find (ports, ch))
        return port->interfaceName;
    return "RME HDSPe";
}

std::optional<int> firstOf (const std::vector<int>& v)
{
    return v.empty() ? std::nullopt : std::optional<int> (v.front());
}
} // namespace

juce::String DeviceIO::fullLabel() const
{
    return device.name + juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  ")) + device.physicalRangeLabel (channels, direction);
}

std::vector<DeviceIO> deviceIOs (const std::vector<int>& channels, ChannelDirection direction,
                                 const std::vector<PhysicalDevice>& physical)
{
    std::vector<DeviceIO> result;
    for (auto ch : channels)
    {
        const auto* dev = PhysicalDevices::find (physical, ch, direction);
        if (dev == nullptr)
            continue;
        auto it = std::find_if (result.begin(), result.end(), [dev] (auto& io) { return io.device.id == dev->id; });
        if (it != result.end())
            it->channels.push_back (ch);
        else
            result.push_back ({ *dev, { ch }, direction });
    }
    return result;
}

std::vector<Node> build (const ParsedDevice& d, const std::vector<PhysicalDevice>& physical, const std::vector<AudioPort>& ports)
{
    using K = Node::Kind;
    const auto recvIOs = deviceIOs (d.recvChannels, ChannelDirection::recv, physical);
    const auto sendIOs = deviceIOs (d.sendChannels, ChannelDirection::send, physical);
    const auto recvInterface = interfaceFor (d.recvChannels, d.sendChannels, ports);
    const auto sendInterface = interfaceFor (d.sendChannels, d.recvChannels, ports);

    std::vector<Node> nodes;
    const auto add = [&nodes] (Node n) { nodes.push_back (std::move (n)); };
    const auto receivePath = [&]
    {
        for (const auto& io : recvIOs)
            add ({ K::analogOut, io.fullLabel(), {}, false, firstOf (io.channels) });
        add ({ K::interfaceReceive, recvInterface, joined (d.recvChannels), recvIOs.empty(), firstOf (d.recvChannels) });
    };
    const auto sendPath = [&]
    {
        add ({ K::interfaceSend, sendInterface, joined (d.sendChannels), sendIOs.empty(), firstOf (d.sendChannels) });
        for (const auto& io : sendIOs)
            add ({ K::analogIn, io.fullLabel(), {}, false, firstOf (io.channels) });
    };

    switch (d.type)
    {
        case ParsedDevice::Type::instrument:
            add ({ K::cubaseInstrumentTrack, {}, {}, false, {} });
            if (d.midiDevice.isNotEmpty())
                add ({ K::midiOut, d.midiDevice, {}, false, {} });
            add ({ K::instrument, d.name, {}, false, {} });
            if (d.recvChannels.empty())
                add ({ K::noChannels, {}, {}, false, {} });
            else
            {
                receivePath();
                add ({ K::cubaseReceived, {}, {}, false, {} });
            }
            break;

        case ParsedDevice::Type::sendOnly:
            add ({ K::cubaseSent, {}, {}, false, {} });
            sendPath();
            add ({ K::hardware, d.name, {}, false, {} });
            break;

        case ParsedDevice::Type::fx:
            add ({ K::cubaseFxSend, {}, {}, false, {} });
            if (d.sendChannels.empty())
                add ({ K::noSendChannels, {}, {}, false, {} });
            else
                sendPath();
            add ({ K::hardware, d.name, {}, false, {} });
            receivePath();
            add ({ K::cubaseFxReturn, {}, {}, false, {} });
            break;
    }
    return nodes;
}

} // namespace StudioPatchbay::SignalFlow
