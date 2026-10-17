#include "CubaseConfigParser.h"

#include <algorithm>
#include <map>
#include <set>

namespace StudioPatchbay
{

// ============================================================================
// Shared helpers
// ============================================================================

namespace
{
// Find a child element matching: <TAG name="NAME" ...>
const juce::XmlElement* findNamedChild (const juce::XmlElement* parent,
                                         const juce::String& tag,
                                         const juce::String& name)
{
    if (parent == nullptr)
        return nullptr;

    for (auto* child = parent->getFirstChildElement(); child != nullptr;
         child = child->getNextElement())
    {
        if (child->hasTagName (tag) && child->getStringAttribute ("name") == name)
            return child;
    }
    return nullptr;
}

// Read the value attribute from: <string name="NAME" value="VALUE" />
juce::String readStringValue (const juce::XmlElement* parent, const juce::String& name)
{
    auto* el = findNamedChild (parent, "string", name);
    return el != nullptr ? el->getStringAttribute ("value") : juce::String();
}

// Read the value attribute from: <int name="NAME" value="VALUE" />
int readIntValue (const juce::XmlElement* parent, const juce::String& name, int defaultVal = 0)
{
    auto* el = findNamedChild (parent, "int", name);
    return el != nullptr ? el->getIntAttribute ("value", defaultVal) : defaultVal;
}
} // namespace

// ============================================================================
// Channel parsing – shared between ExternalPlugins and ControlRoom
// ============================================================================

int CubaseConfigParser::parseChannelFromConnectionValue (const juce::String& value)
{
    // Format: "prefix|...|HDSPe Device NN" or similar pipe-delimited strings.
    // Take the last component after splitting by '|'.
    auto parts = juce::StringArray::fromTokens (value, "|", "");
    if (parts.isEmpty())
        return -1;

    auto lastPart = parts[parts.size() - 1].trim();

    // Strip known prefixes like "HDSPe Device "
    if (lastPart.startsWith ("HDSPe Device "))
        lastPart = lastPart.fromFirstOccurrenceOf ("HDSPe Device ", false, false).trim();

    return lastPart.getIntValue();
}

std::vector<int> CubaseConfigParser::parseChannelsFromBusList (const juce::XmlElement* busList)
{
    std::vector<int> channels;
    if (busList == nullptr)
        return channels;

    // busList is e.g. <list name="Dispatcher Busses">
    //   <item> <list name="Connections"> <item value="..."> ...
    for (auto* busItem = busList->getFirstChildElement(); busItem != nullptr;
         busItem = busItem->getNextElement())
    {
        if (! busItem->hasTagName ("item"))
            continue;

        auto* connections = findNamedChild (busItem, "list", "Connections");
        if (connections == nullptr)
            continue;

        for (auto* connItem = connections->getFirstChildElement(); connItem != nullptr;
             connItem = connItem->getNextElement())
        {
            if (! connItem->hasTagName ("item"))
                continue;

            auto value = connItem->getStringAttribute ("value");
            int ch = parseChannelFromConnectionValue (value);
            if (ch > 0)
                channels.push_back (ch);
        }
    }

    std::sort (channels.begin(), channels.end());
    return channels;
}

// ============================================================================
// External Plugins parser
// ============================================================================

ParsedDevices CubaseConfigParser::parseExternalPlugins (const juce::File& xmlFile)
{
    ParsedDevices result;

    auto xml = juce::XmlDocument::parse (xmlFile);
    if (xml == nullptr || ! xml->hasTagName ("ExternalPlugins"))
        return result;

    // Find <list name="External Effects">
    auto* effectsList = findNamedChild (xml.get(), "list", "External Effects");
    if (effectsList == nullptr)
        return result;

    for (auto* item = effectsList->getFirstChildElement(); item != nullptr;
         item = item->getNextElement())
    {
        if (! item->hasTagName ("item"))
            continue;

        // Read device properties
        auto name = readStringValue (item, "External Effect Name");
        if (name.isEmpty())
            continue;

        int isInstrument = readIntValue (item, "External Instrument", 0);

        // MIDI device
        juce::String midiDevice;
        auto* midiLink = findNamedChild (item, "member", "Midi Link");
        if (midiLink != nullptr)
        {
            auto nodePath = readStringValue (midiLink, "NodePathID");
            if (nodePath.startsWith ("Midi Device Manager\\"))
                midiDevice = nodePath.fromFirstOccurrenceOf ("Midi Device Manager\\", false, false);
            else
                midiDevice = nodePath;
        }

        // Parse channels
        auto* dispatcherBusses = findNamedChild (item, "list", "Dispatcher Busses");
        auto* receiverBusses   = findNamedChild (item, "list", "Receiver Busses");

        auto sendChannels = parseChannelsFromBusList (dispatcherBusses);
        auto recvChannels = parseChannelsFromBusList (receiverBusses);

        // Filter out empty devices
        if (sendChannels.empty() && recvChannels.empty() && isInstrument == 0)
            continue;

        // Determine device type
        ParsedDevice::Type type;
        if (isInstrument == 1)
            type = ParsedDevice::Type::instrument;
        else if (recvChannels.empty())
            type = ParsedDevice::Type::sendOnly;
        else
            type = ParsedDevice::Type::fx;

        ParsedDevice device { name, type, std::move (sendChannels),
                              std::move (recvChannels), midiDevice };

        if (type == ParsedDevice::Type::instrument)
            result.instruments.push_back (std::move (device));
        else
            result.fxDevices.push_back (std::move (device));
    }

    return result;
}

// ============================================================================
// Port Setup parser
// ============================================================================

juce::String CubaseConfigParser::cleanFamilyName (const juce::String& raw)
{
    auto trimmed = raw.trim();

    // Strip trailing " L", " R" (stereo markers)
    if (trimmed.endsWith (" L") || trimmed.endsWith (" R"))
        trimmed = trimmed.dropLastCharacters (2).trim();

    // Strip trailing digits + space (e.g. "MADI 1" -> "MADI")
    while (trimmed.isNotEmpty() && juce::CharacterFunctions::isDigit (trimmed.getLastCharacter()))
        trimmed = trimmed.dropLastCharacters (1);

    return trimmed.trim();
}

std::vector<DetectedPort> CubaseConfigParser::parseAggregateBusList (const juce::XmlElement* root)
{
    std::vector<DetectedPort> result;

    auto* portDescriptors = findNamedChild (root, "list", "PortDescriptors");
    if (portDescriptors == nullptr)
        return result;

    // Collect (direction, family) -> sorted list of channel numbers
    struct PortEntry
    {
        ChannelDirection direction;
        juce::String family;
        int channel;
    };

    std::vector<PortEntry> entries;

    for (auto* item = portDescriptors->getFirstChildElement(); item != nullptr;
         item = item->getNextElement())
    {
        if (! item->hasTagName ("item"))
            continue;

        auto id = readStringValue (item, "ID");
        auto sysName = readStringValue (item, "SysName");

        if (id.isEmpty() || sysName.isEmpty())
            continue;

        // Direction from ID prefix
        auto direction = id.startsWith ("O|") ? ChannelDirection::send : ChannelDirection::recv;

        // Absolute channel from trailing integer in ID
        auto idParts = juce::StringArray::fromTokens (id, "|", "");
        if (idParts.isEmpty())
            continue;

        int absChannel = idParts[idParts.size() - 1].getIntValue();
        if (absChannel <= 0 || absChannel > 2048)
            continue;

        // Family from SysName: "MADI 1 - Ch 01" -> "MADI"
        auto familyRaw = sysName.upToFirstOccurrenceOf (" - ", false, false);
        auto family = cleanFamilyName (familyRaw);

        if (family.isNotEmpty())
            entries.push_back ({ direction, family, absChannel });
    }

    // Group by (direction, family) and split into contiguous runs
    std::map<juce::String, std::vector<int>> grouped; // key = "D-Family"

    for (auto& e : entries)
    {
        auto key = juce::String (e.direction == ChannelDirection::send ? "O" : "I") + "-" + e.family;
        grouped[key].push_back (e.channel);
    }

    for (auto& [key, channels] : grouped)
    {
        std::sort (channels.begin(), channels.end());

        auto direction = key.startsWith ("O") ? ChannelDirection::send : ChannelDirection::recv;
        auto family = key.fromFirstOccurrenceOf ("-", false, false);

        // Split into contiguous runs
        int runStart = channels[0];
        int prev = channels[0];

        for (size_t i = 1; i < channels.size(); ++i)
        {
            if (channels[i] != prev + 1)
            {
                result.push_back ({ family, direction, runStart, prev, false });
                runStart = channels[i];
            }
            prev = channels[i];
        }
        result.push_back ({ family, direction, runStart, prev, false });
    }

    return result;
}

std::vector<DetectedPort> CubaseConfigParser::parseSettingsListFallback (const juce::XmlElement* root)
{
    std::vector<DetectedPort> result;

    auto* settings = findNamedChild (root, "list", "Settings");
    if (settings == nullptr)
        return result;

    // Group entries by (direction, family) in file order -> count channels per group
    std::map<juce::String, int> groupCounts;

    for (auto* item = settings->getFirstChildElement(); item != nullptr;
         item = item->getNextElement())
    {
        if (! item->hasTagName ("item"))
            continue;

        auto id = readStringValue (item, "ID");
        if (id.isEmpty())
            continue;

        auto direction = id.startsWith ("O|") ? ChannelDirection::send : ChannelDirection::recv;

        auto parts = juce::StringArray::fromTokens (id, "|", "");
        if (parts.size() < 3)
            continue;

        // Skip MIDI
        if (parts[1] == "MIDI System")
            continue;

        auto family = cleanFamilyName (parts[2]);
        if (family.isEmpty())
            continue;

        auto key = juce::String (direction == ChannelDirection::send ? "O" : "I") + "-" + family;
        groupCounts[key]++;
    }

    // Generate sequential 1..N numbering (approximate)
    for (auto& [key, count] : groupCounts)
    {
        auto direction = key.startsWith ("O") ? ChannelDirection::send : ChannelDirection::recv;
        auto family = key.fromFirstOccurrenceOf ("-", false, false);
        result.push_back ({ family, direction, 1, count, true });
    }

    return result;
}

std::vector<DetectedPort> CubaseConfigParser::parsePortSetup (const juce::File& xmlFile)
{
    auto xml = juce::XmlDocument::parse (xmlFile);
    if (xml == nullptr)
        return {};

    // Try rich format first (ASIO with PortDescriptors)
    auto ports = parseAggregateBusList (xml.get());
    if (! ports.empty())
        return ports;

    // Fallback to flat Settings format (CoreAudio)
    return parseSettingsListFallback (xml.get());
}

// ============================================================================
// Control Room parser
// ============================================================================

std::vector<ControlRoomBus> CubaseConfigParser::parseControlRoom (const juce::File& xmlFile)
{
    std::vector<ControlRoomBus> result;

    auto xml = juce::XmlDocument::parse (xmlFile);
    if (xml == nullptr)
        return result;

    // Navigate to <member name="Preset"> -> <list name="Channel">
    auto* preset = findNamedChild (xml.get(), "member", "Preset");
    if (preset == nullptr)
    {
        // Try root level if structure differs
        for (auto* child = xml->getFirstChildElement(); child != nullptr;
             child = child->getNextElement())
        {
            preset = findNamedChild (child, "member", "Preset");
            if (preset != nullptr)
                break;
        }
    }
    if (preset == nullptr)
        return result;

    auto* channelList = findNamedChild (preset, "list", "Channel");
    if (channelList == nullptr)
        return result;

    int index = 0;
    for (auto* item = channelList->getFirstChildElement(); item != nullptr;
         item = item->getNextElement())
    {
        if (! item->hasTagName ("item"))
            continue;

        // Read name: <member name="Name"> -> <string name="String" value="...">
        juce::String name;
        auto* nameMember = findNamedChild (item, "member", "Name");
        if (nameMember != nullptr)
            name = readStringValue (nameMember, "String");

        // Must have OutputBus (main "Control Room" bus lacks it and is skipped)
        auto* outputBus = findNamedChild (item, "member", "OutputBus");
        if (outputBus == nullptr)
        {
            ++index;
            continue;
        }

        // Parse channels from OutputBus -> Connections
        auto* connections = findNamedChild (outputBus, "list", "Connections");
        auto channels = parseChannelsFromBusList (outputBus);

        // If no channels from the bus list approach, try connections directly
        if (channels.empty() && connections != nullptr)
        {
            for (auto* connItem = connections->getFirstChildElement(); connItem != nullptr;
                 connItem = connItem->getNextElement())
            {
                if (! connItem->hasTagName ("item"))
                    continue;

                int ch = parseChannelFromConnectionValue (connItem->getStringAttribute ("value"));
                if (ch > 0)
                    channels.push_back (ch);
            }
            std::sort (channels.begin(), channels.end());
        }

        // Determine kind from Type value
        int typeValue = readIntValue (item, "Type", 0);
        ControlRoomBus::Kind kind;
        switch (typeValue)
        {
            case 15: kind = ControlRoomBus::Kind::phones;  break;
            case 16: kind = ControlRoomBus::Kind::speaker; break;
            default: kind = ControlRoomBus::Kind::studio;  break;
        }

        if (! channels.empty())
            result.push_back ({ index, name, kind, std::move (channels) });

        ++index;
    }

    return result;
}

} // namespace StudioPatchbay
