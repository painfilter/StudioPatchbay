#include "AudioPorts.h"

#include <algorithm>

namespace StudioPatchbay::AudioPorts
{

ConnectorType colorForFamily (const juce::String& family)
{
    const auto f = family.toUpperCase();
    if (f == "MADI")  return ConnectorType::madi;
    if (f == "ADAT")  return ConnectorType::adat;
    if (f == "AES")   return ConnectorType::aes;
    if (f == "SPDIF") return ConnectorType::spdif;
    if (f == "DANTE") return ConnectorType::dante;
    if (f == "PHONES" || f == "LINE" || f == "INSTRUMENT") return ConnectorType::analogLine;
    if (f == "MIC")   return ConnectorType::analogMic;
    if (f == "USB")   return ConnectorType::usb;
    return ConnectorType::other;
}

std::vector<AudioPort> build (const std::vector<DetectedPort>& detected)
{
    struct Run { int first, last; };
    std::map<juce::String, std::pair<std::vector<Run>, std::vector<Run>>> byFamily;   // sorted by family
    for (const auto& p : detected)
    {
        auto& runs = byFamily[p.family];
        (p.direction == ChannelDirection::send ? runs.first : runs.second).push_back ({ p.channelStart, p.channelEnd });
    }

    std::vector<AudioPort> result;
    int nextId = 0;
    for (auto& [family, runs] : byFamily)
    {
        auto& [sends, recvs] = runs;
        const auto byStart = [] (const Run& a, const Run& b) { return a.first < b.first; };
        std::sort (sends.begin(), sends.end(), byStart);
        std::sort (recvs.begin(), recvs.end(), byStart);
        const auto count = std::max (sends.size(), recvs.size());

        for (size_t i = 0; i < count; ++i)
        {
            const auto* s = i < sends.size() ? &sends[i] : nullptr;
            const auto* r = i < recvs.size() ? &recvs[i] : nullptr;
            const auto first = std::min (s ? s->first : r->first, r ? r->first : s->first);
            const auto last = std::max (s ? s->last : r->last, r ? r->last : s->last);
            const auto name = family + (count > 1 ? " " + juce::String (static_cast<int> (i) + 1) : juce::String());
            result.push_back ({ nextId++, name, first, last, colorForFamily (family), family + " Interface" });
        }
    }

    std::stable_sort (result.begin(), result.end(),
                      [] (const AudioPort& a, const AudioPort& b) { return a.channelStart < b.channelStart; });
    return result;
}

juce::String stableLabelKey (const AudioPort& port)
{
    // Strip a trailing instance number: "AES 2" -> "AES".
    auto family = port.name;
    const auto lastSpace = family.lastIndexOfChar (' ');
    if (lastSpace > 0 && family.substring (lastSpace + 1).containsOnly ("0123456789")
        && family.substring (lastSpace + 1).isNotEmpty())
        family = family.substring (0, lastSpace).trimEnd();
    return family + "@" + juce::String (port.channelStart);
}

void applyLabels (std::vector<AudioPort>& ports, const std::map<juce::String, juce::String>& labels)
{
    const auto lookup = [&labels] (const juce::String& key) -> juce::String
    {
        const auto it = labels.find (key);
        return it != labels.end() ? it->second : juce::String();
    };
    for (auto& port : ports)
    {
        auto label = lookup (stableLabelKey (port));
        if (label.isEmpty()) label = lookup ("ch" + juce::String (port.channelStart));
        if (label.isEmpty()) label = lookup (port.name);
        if (label.trim().isNotEmpty())
            port.interfaceName = label;
    }
}

const AudioPort* find (const std::vector<AudioPort>& ports, int channel)
{
    for (const auto& p : ports)
        if (p.contains (channel))
            return &p;
    return nullptr;
}

} // namespace StudioPatchbay::AudioPorts
