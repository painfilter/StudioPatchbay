#include "Browsing.h"

#include <algorithm>

namespace StudioPatchbay
{

namespace Browsing
{

bool matchesSearch (const ParsedDevice& d, const juce::String& query, const std::vector<PhysicalDevice>& physical)
{
    const auto q = query.toLowerCase();
    if (d.name.toLowerCase().contains (q) || d.midiDevice.toLowerCase().contains (q))
        return true;

    const auto channels = d.allChannels();
    for (auto ch : channels)
        if (juce::String (ch).contains (q))
            return true;

    // Physical device names, by the device's whole span (as the Swift app does).
    for (auto ch : channels)
        for (const auto& p : physical)
            if (p.channelRange().contains (ch))
            {
                if (p.name.toLowerCase().contains (q))
                    return true;
                break;
            }
    return false;
}

std::vector<ParsedDevice> filter (const ParsedDevices& devices, FilterMode mode, const juce::String& query,
                                  const std::vector<PhysicalDevice>& physical)
{
    std::vector<ParsedDevice> result;
    for (const auto& d : devices.all())
    {
        if (query.isNotEmpty() && ! matchesSearch (d, query, physical))
            continue;
        bool keep = true;
        switch (mode)
        {
            case FilterMode::all:         break;
            case FilterMode::instruments: keep = d.type == ParsedDevice::Type::instrument; break;
            case FilterMode::fx:          keep = d.type == ParsedDevice::Type::fx; break;
            case FilterMode::sendOnly:    keep = d.type == ParsedDevice::Type::sendOnly; break;
            case FilterMode::stereo:      keep = d.isStereo(); break;
            case FilterMode::mono:        keep = d.isMono(); break;
            case FilterMode::midi:        keep = d.midiDevice.isNotEmpty(); break;
        }
        if (keep)
            result.push_back (d);
    }
    return result;
}

std::vector<ParsedDevice> inPatchbayOrder (std::vector<ParsedDevice> devices)
{
    std::stable_sort (devices.begin(), devices.end(), [] (const ParsedDevice& a, const ParsedDevice& b)
    {
        const auto ca = a.allChannels(), cb = b.allChannels();
        const auto fa = ca.empty() ? std::numeric_limits<int>::max() : ca.front();
        const auto fb = cb.empty() ? std::numeric_limits<int>::max() : cb.front();
        if (fa != fb)
            return fa < fb;
        return a.name < b.name;
    });
    return devices;
}

} // namespace Browsing

void NavigationHistory::jumped (Point from)
{
    backStack.push_back (from);
    forwardStack.clear();
}

std::optional<NavigationHistory::Point> NavigationHistory::back (Point current)
{
    if (backStack.empty())
        return std::nullopt;
    auto point = backStack.back();
    backStack.pop_back();
    forwardStack.push_back (current);
    return point;
}

std::optional<NavigationHistory::Point> NavigationHistory::forward (Point current)
{
    if (forwardStack.empty())
        return std::nullopt;
    auto point = forwardStack.back();
    forwardStack.pop_back();
    backStack.push_back (current);
    return point;
}

} // namespace StudioPatchbay
