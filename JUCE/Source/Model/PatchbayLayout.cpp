#include "PatchbayLayout.h"

#include <algorithm>
#include <set>

namespace StudioPatchbay::Patchbay
{

namespace
{
const juce::String dash (juce::CharPointer_UTF8 ("\xe2\x80\x93"));

std::vector<int> span (int first, int last)
{
    std::vector<int> result;
    for (int ch = first; ch <= last; ++ch)
        result.push_back (ch);
    return result;
}

bool ownedByInterface (const std::vector<PhysicalDevice>& physical, int ch)
{
    return PhysicalDevices::isChannelOwned (physical, ch, DeviceType::audioInterface);
}
} // namespace

std::vector<int> Block::rowChannels (ChannelDirection direction) const
{
    if (! device || ! device->supports (direction))
        return span (first, last);
    const auto r = device->range (direction);
    return span (r.first, r.last);
}

BusMap busesByChannel (const std::vector<ControlRoomBus>& buses)
{
    BusMap map;
    for (const auto& bus : buses)
        for (auto ch : bus.channels)
            map[ch].push_back (bus);
    return map;
}

std::vector<AudioPort> activePorts (const ParsedDevices& devices, const std::vector<ControlRoomBus>& buses,
                                    const std::vector<AudioPort>& ports)
{
    std::set<int> used;   // ids of ports something touches
    for (const auto& d : devices.all())
        for (auto ch : d.allChannels())
            if (auto* p = AudioPorts::find (ports, ch))
                used.insert (p->id);
    for (const auto& bus : buses)
        for (auto ch : bus.channels)
            if (auto* p = AudioPorts::find (ports, ch))
                used.insert (p->id);

    std::vector<AudioPort> candidates;
    for (const auto& p : ports)
        if (used.count (p.id) > 0)
            candidates.push_back (p);
    std::stable_sort (candidates.begin(), candidates.end(),
                      [] (auto& a, auto& b) { return a.channelStart < b.channelStart; });

    std::set<int> claimed;
    std::vector<AudioPort> result;
    for (const auto& p : candidates)
    {
        bool fullyClaimed = true;
        for (int ch = p.channelStart; ch <= p.channelEnd && fullyClaimed; ++ch)
            fullyClaimed = claimed.count (ch) > 0;
        if (fullyClaimed)
            continue;
        for (int ch = p.channelStart; ch <= p.channelEnd; ++ch)
            claimed.insert (ch);
        result.push_back (p);
    }
    return result;
}

Layout build (const ParsedDevices& devices, const std::vector<ControlRoomBus>& buses,
              const std::vector<AudioPort>& ports, const std::vector<PhysicalDevice>& physical)
{
    const auto send = devices.devicesByChannel (ChannelDirection::send);
    const auto recv = devices.devicesByChannel (ChannelDirection::recv);
    const auto crMap = busesByChannel (buses);
    const auto has = [] (const auto& map, int ch) { return map.find (ch) != map.end(); };

    Layout layout;
    for (const auto& port : activePorts (devices, buses, ports))
    {
        PortSection section { port, {}, 0 };

        int usedSendMax = port.channelStart, usedRecvMax = port.channelStart, usedBusMax = port.channelStart;
        for (int ch = port.channelStart; ch <= port.channelEnd; ++ch)
        {
            if (has (send, ch) && ! ownedByInterface (physical, ch)) usedSendMax = ch;
            if (has (recv, ch) && ! ownedByInterface (physical, ch)) usedRecvMax = ch;
            if (has (crMap, ch)) usedBusMax = ch;
            if (has (send, ch) || has (recv, ch)) ++section.usedCount;
        }
        // Shown up to the next multiple of 16 past the last used channel.
        const auto raw = std::max ({ usedSendMax, usedRecvMax, usedBusMax });
        const auto displayEnd = std::min (((raw + 15) / 16) * 16, port.channelEnd);

        std::set<juce::String> seen;   // device ids that already have a block
        for (int i = port.channelStart; i <= displayEnd;)
        {
            if (ownedByInterface (physical, i))
            {
                ++i;
                continue;
            }
            const auto* dev = PhysicalDevices::find (physical, i, ChannelDirection::recv, DeviceType::audioInterface);
            auto direction = ChannelDirection::recv;
            if (dev == nullptr)
            {
                dev = PhysicalDevices::find (physical, i, ChannelDirection::send, DeviceType::audioInterface);
                direction = ChannelDirection::send;
            }
            if (dev != nullptr)
            {
                const auto end = std::min (dev->range (direction).last, displayEnd);
                if (seen.insert (dev->id.toString()).second)
                    section.blocks.push_back ({ *dev, i, end });
                i = end + 1;
            }
            else
            {
                if (has (send, i) || has (recv, i) || has (crMap, i))
                    section.blocks.push_back ({ std::nullopt, i, i });
                ++i;
            }
        }
        layout.ports.push_back (std::move (section));
    }

    for (const auto& d : physical)
        if (d.deviceType == DeviceType::audioInterface)
            layout.audioInterfaces.push_back (d);
    std::stable_sort (layout.audioInterfaces.begin(), layout.audioInterfaces.end(),
                      [] (auto& a, auto& b) { return a.sendChannelStart < b.sendChannelStart; });
    return layout;
}

juce::String headerChannelLabel (const PhysicalDevice& d)
{
    const auto s = d.range (ChannelDirection::send), r = d.range (ChannelDirection::recv);
    const auto text = [] (PhysicalDevice::Range x) { return juce::String (x.first) + dash + juce::String (x.last); };
    switch (d.directionality)
    {
        case DeviceDirectionality::sendOnly: return "OUT ch " + text (s);
        case DeviceDirectionality::recvOnly: return "IN ch " + text (r);
        case DeviceDirectionality::bidirectional: break;
    }
    if (s.first == r.first && s.last == r.last)
        return "ch " + text (r);
    return "OUT ch " + text (s) + juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  ")) + "IN ch " + text (r);
}

juce::String rangeString (std::vector<int> channels)
{
    if (channels.empty())
        return {};
    std::sort (channels.begin(), channels.end());
    const auto contiguous = channels.back() - channels.front() + 1 == static_cast<int> (channels.size())
                            && std::adjacent_find (channels.begin(), channels.end()) == channels.end();
    if (contiguous && channels.size() > 1)
        return juce::String (channels.front()) + dash + juce::String (channels.back());
    juce::StringArray parts;
    for (auto ch : channels)
        parts.add (juce::String (ch));
    return parts.joinIntoString ("+");
}

std::vector<Connection> connections (const Block& block, const DeviceMap& send, const DeviceMap& recv)
{
    const auto blockSend = block.rowChannels (ChannelDirection::send);
    const auto blockRecv = block.rowChannels (ChannelDirection::recv);

    std::vector<ParsedDevice> found;
    const auto collect = [&] (const DeviceMap& map, int ch)
    {
        if (auto it = map.find (ch); it != map.end())
            for (const auto& d : it->second)
                if (std::find (found.begin(), found.end(), d) == found.end())
                    found.push_back (d);
    };
    for (const auto& channels : { blockSend, blockRecv })
        for (auto ch : channels)
        {
            collect (send, ch);
            collect (recv, ch);
        }
    std::stable_sort (found.begin(), found.end(), [] (auto& a, auto& b)
    {
        const auto ca = a.allChannels(), cb = b.allChannels();
        return (ca.empty() ? 0 : ca.front()) < (cb.empty() ? 0 : cb.front());
    });

    std::vector<Connection> result;
    for (const auto& d : found)
    {
        std::vector<int> out, in;
        for (auto ch : d.sendChannels)
            if (std::find (blockSend.begin(), blockSend.end(), ch) != blockSend.end())
                out.push_back (ch);
        for (auto ch : d.recvChannels)
            if (std::find (blockRecv.begin(), blockRecv.end(), ch) != blockRecv.end())
                in.push_back (ch);
        result.push_back ({ d, rangeString (out), rangeString (in) });
    }
    return result;
}

std::vector<std::pair<ControlRoomBus, juce::String>> busConnections (const Block& block, const BusMap& buses)
{
    const auto blockSend = block.rowChannels (ChannelDirection::send);
    std::vector<ControlRoomBus> found;
    for (auto ch : blockSend)
        if (auto it = buses.find (ch); it != buses.end())
            for (const auto& bus : it->second)
                if (std::none_of (found.begin(), found.end(), [&] (auto& b) { return b.index == bus.index; }))
                    found.push_back (bus);
    std::stable_sort (found.begin(), found.end(), [] (auto& a, auto& b)
    {
        return (a.channels.empty() ? 0 : a.channels.front()) < (b.channels.empty() ? 0 : b.channels.front());
    });

    std::vector<std::pair<ControlRoomBus, juce::String>> result;
    for (const auto& bus : found)
    {
        std::vector<int> relevant;
        for (auto ch : bus.channels)
            if (std::find (blockSend.begin(), blockSend.end(), ch) != blockSend.end())
                relevant.push_back (ch);
        result.emplace_back (bus, rangeString (relevant));
    }
    return result;
}

} // namespace StudioPatchbay::Patchbay
