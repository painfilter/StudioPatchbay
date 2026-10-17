#include "PhysicalDevice.h"

#include <algorithm>
#include <array>

namespace StudioPatchbay
{

// ============================================================================
// Channels
// ============================================================================

bool PhysicalDevice::supports (ChannelDirection direction) const
{
    if (direction == ChannelDirection::send)
        return directionality != DeviceDirectionality::recvOnly;
    return directionality != DeviceDirectionality::sendOnly;
}

PhysicalDevice::Range PhysicalDevice::range (ChannelDirection direction) const
{
    const auto start = direction == ChannelDirection::send ? sendChannelStart : recvChannelStart;
    return { start, start + physicalChannelCount - 1 };
}

PhysicalDevice::Range PhysicalDevice::channelRange() const
{
    switch (directionality)
    {
        case DeviceDirectionality::sendOnly: return range (ChannelDirection::send);
        case DeviceDirectionality::recvOnly: return range (ChannelDirection::recv);
        case DeviceDirectionality::bidirectional: break;
    }
    const auto s = range (ChannelDirection::send), r = range (ChannelDirection::recv);
    return { std::min (s.first, r.first), std::max (s.last, r.last) };
}

bool PhysicalDevice::contains (int channel, ChannelDirection direction) const
{
    return supports (direction) && range (direction).contains (channel);
}

std::optional<int> PhysicalDevice::physicalIO (int channel, ChannelDirection direction) const
{
    if (! contains (channel, direction))
        return std::nullopt;
    return channel - range (direction).first + 1;
}

juce::String PhysicalDevice::physicalRangeLabel (const std::vector<int>& channels, ChannelDirection direction) const
{
    std::vector<int> ios;
    for (auto ch : channels)
        if (auto io = physicalIO (ch, direction))
            ios.push_back (*io);
    if (ios.empty())
        return {};
    std::sort (ios.begin(), ios.end());
    if (ios.size() == 1)
        return "I/O " + juce::String (ios.front());

    const auto contiguous = ios.back() - ios.front() + 1 == static_cast<int> (ios.size())
                            && std::adjacent_find (ios.begin(), ios.end()) == ios.end();
    if (contiguous)
        return "I/O " + juce::String (ios.front()) + juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x93"))
               + juce::String (ios.back());

    juce::StringArray parts;
    for (auto io : ios)
        parts.add (juce::String (io));
    return "I/O " + parts.joinIntoString ("+");
}

PhysicalDevice PhysicalDevice::withNewId() const
{
    auto copy = *this;
    copy.id = juce::Uuid();
    return copy;
}

// ============================================================================
// Raw values
// ============================================================================

namespace PhysicalDevices
{

namespace
{
constexpr std::array<std::pair<ConnectorType, const char*>, 10> connectorNames { {
    { ConnectorType::madi, "madi" }, { ConnectorType::adat, "adat" }, { ConnectorType::aes, "aes" },
    { ConnectorType::spdif, "spdif" }, { ConnectorType::dante, "dante" }, { ConnectorType::usb, "usb" },
    { ConnectorType::thunderbolt, "thunderbolt" }, { ConnectorType::analogLine, "analogLine" },
    { ConnectorType::analogMic, "analogMic" }, { ConnectorType::other, "other" } } };
constexpr std::array<std::pair<DeviceType, const char*>, 3> deviceTypeNames { {
    { DeviceType::converter, "converter" }, { DeviceType::audioInterface, "audioInterface" },
    { DeviceType::other, "other" } } };
constexpr std::array<std::pair<DeviceDirectionality, const char*>, 3> directionalityNames { {
    { DeviceDirectionality::bidirectional, "bidirectional" }, { DeviceDirectionality::sendOnly, "sendOnly" },
    { DeviceDirectionality::recvOnly, "recvOnly" } } };

template <typename Enum, size_t N>
juce::String nameOf (const std::array<std::pair<Enum, const char*>, N>& table, Enum value)
{
    for (auto& [e, n] : table)
        if (e == value)
            return n;
    return {};
}

template <typename Enum, size_t N>
std::optional<Enum> valueOf (const std::array<std::pair<Enum, const char*>, N>& table, const juce::String& name)
{
    for (auto& [e, n] : table)
        if (name == n)
            return e;
    return std::nullopt;
}

juce::String quoted (const juce::String& s)
{
    return juce::JSON::toString (juce::var (s));
}
} // namespace

juce::String toString (ConnectorType v)        { return nameOf (connectorNames, v); }
juce::String toString (DeviceType v)           { return nameOf (deviceTypeNames, v); }
juce::String toString (DeviceDirectionality v) { return nameOf (directionalityNames, v); }
std::optional<ConnectorType> connectorFromString (const juce::String& s)               { return valueOf (connectorNames, s); }
std::optional<DeviceType> deviceTypeFromString (const juce::String& s)                 { return valueOf (deviceTypeNames, s); }
std::optional<DeviceDirectionality> directionalityFromString (const juce::String& s)   { return valueOf (directionalityNames, s); }

// ============================================================================
// JSON
// ============================================================================

juce::Result fromJson (const juce::String& json, std::vector<PhysicalDevice>& out)
{
    juce::var parsed;
    if (const auto result = juce::JSON::parse (json, parsed); result.failed())
        return result;
    const auto* array = parsed.getArray();
    if (array == nullptr)
        return juce::Result::fail ("Expected a list of devices.");

    std::vector<PhysicalDevice> devices;
    for (int index = 0; index < array->size(); ++index)
    {
        const auto& entry = array->getReference (index);
        const auto* object = entry.getDynamicObject();
        const auto where = "Device " + juce::String (index + 1) + ": ";
        if (object == nullptr)
            return juce::Result::fail (where + "not an object.");

        const auto text = [object] (const char* key) { return object->getProperty (key); };
        const auto requireString = [&] (const char* key, juce::String& target)
        {
            if (! text (key).isString())
                return false;
            target = text (key).toString();
            return true;
        };
        const auto requireInt = [&] (const char* key, int& target)
        {
            const auto v = text (key);
            if (! (v.isInt() || v.isInt64() || v.isDouble()))
                return false;
            target = static_cast<int> (v);
            return true;
        };

        PhysicalDevice d;
        // An id that is no UUID (older files used numbers) gets a new one.
        if (const auto id = text ("id"); id.isString() && juce::Uuid (id.toString()).toDashedString().equalsIgnoreCase (id.toString()))
            d.id = juce::Uuid (id.toString());

        juce::String raw;
        if (! requireString ("name", d.name))
            return juce::Result::fail (where + "\"name\" is missing.");
        if (! requireString ("shortName", d.shortName))
            return juce::Result::fail (where + "\"shortName\" is missing.");
        if (! requireString ("portColor", raw) || ! connectorFromString (raw))
            return juce::Result::fail (where + "\"portColor\" is missing or unknown.");
        d.portColor = *connectorFromString (raw);
        if (! requireInt ("physicalChannelCount", d.physicalChannelCount)
            || ! requireInt ("sendChannelStart", d.sendChannelStart)
            || ! requireInt ("recvChannelStart", d.recvChannelStart))
            return juce::Result::fail (where + "a channel number is missing.");

        if (object->hasProperty ("deviceType") && ! text ("deviceType").isVoid())
        {
            const auto type = deviceTypeFromString (text ("deviceType").toString());
            if (! type)
                return juce::Result::fail (where + "\"deviceType\" is unknown.");
            d.deviceType = *type;
        }
        if (object->hasProperty ("directionality") && ! text ("directionality").isVoid())
        {
            const auto dir = directionalityFromString (text ("directionality").toString());
            if (! dir)
                return juce::Result::fail (where + "\"directionality\" is unknown.");
            d.directionality = *dir;
        }
        if (text ("customColorHex").isString())
            d.customColorHex = text ("customColorHex").toString();

        devices.push_back (std::move (d));
    }
    out = std::move (devices);
    return juce::Result::ok();
}

juce::String toJson (const std::vector<PhysicalDevice>& devices)
{
    if (devices.empty())
        return "[\n\n]";

    juce::StringArray objects;
    for (const auto& d : devices)
    {
        // Sorted keys, as the Swift encoder writes them.
        juce::StringArray lines;
        if (d.customColorHex.isNotEmpty())
            lines.add ("\"customColorHex\" : " + quoted (d.customColorHex));
        lines.add ("\"deviceType\" : " + quoted (toString (d.deviceType)));
        lines.add ("\"directionality\" : " + quoted (toString (d.directionality)));
        lines.add ("\"id\" : " + quoted (d.id.toDashedString().toUpperCase()));
        lines.add ("\"name\" : " + quoted (d.name));
        lines.add ("\"physicalChannelCount\" : " + juce::String (d.physicalChannelCount));
        lines.add ("\"portColor\" : " + quoted (toString (d.portColor)));
        lines.add ("\"recvChannelStart\" : " + juce::String (d.recvChannelStart));
        lines.add ("\"sendChannelStart\" : " + juce::String (d.sendChannelStart));
        lines.add ("\"shortName\" : " + quoted (d.shortName));
        objects.add ("  {\n    " + lines.joinIntoString (",\n    ") + "\n  }");
    }
    return "[\n" + objects.joinIntoString (",\n") + "\n]";
}

// ============================================================================
// Ownership and conflicts
// ============================================================================

namespace
{
void overlapsWith (const PhysicalDevice& candidate, const PhysicalDevice& other, std::vector<ChannelConflict>& out)
{
    for (auto direction : { ChannelDirection::send, ChannelDirection::recv })
    {
        if (! candidate.supports (direction) || ! other.supports (direction))
            continue;
        const auto mine = candidate.range (direction), theirs = other.range (direction);
        if (mine.overlaps (theirs))
            out.push_back ({ direction, std::max (mine.first, theirs.first), std::min (mine.last, theirs.last), other.name });
    }
}
} // namespace

std::vector<ChannelConflict> conflicts (const PhysicalDevice& candidate, const std::vector<PhysicalDevice>& all)
{
    std::vector<ChannelConflict> result;
    for (auto direction : { ChannelDirection::send, ChannelDirection::recv })
        for (const auto& other : all)
            if (other.id != candidate.id)
            {
                // Per direction first, as the Swift app lists them.
                std::vector<ChannelConflict> found;
                overlapsWith (candidate, other, found);
                for (auto& c : found)
                    if (c.direction == direction)
                        result.push_back (c);
            }
    return result;
}

std::vector<ImportCheck> checkImport (const std::vector<PhysicalDevice>& batch,
                                      const std::vector<PhysicalDevice>& existing,
                                      const std::vector<DetectedPort>& detected)
{
    std::vector<ImportCheck> result (batch.size());
    for (size_t i = 0; i < batch.size(); ++i)
    {
        auto& check = result[i];
        for (const auto& other : existing)
            overlapsWith (batch[i], other, check.addConflicts);
        for (size_t j = 0; j < batch.size(); ++j)
            if (j != i)
                overlapsWith (batch[i], batch[j], check.replaceConflicts);
        check.addConflicts.insert (check.addConflicts.end(), check.replaceConflicts.begin(), check.replaceConflicts.end());

        for (auto direction : { ChannelDirection::send, ChannelDirection::recv })
        {
            if (! batch[i].supports (direction))
                continue;
            const auto range = batch[i].range (direction);
            const auto matches = std::any_of (detected.begin(), detected.end(), [&] (const DetectedPort& p)
            {
                return p.direction == direction && p.channelStart <= range.last && range.first <= p.channelEnd;
            });
            if (! matches)
                check.unmatchedDirections.push_back (direction);
        }
    }
    return result;
}

std::vector<int> freeStartChannels (const PhysicalDevice& draft, const std::vector<PhysicalDevice>& all,
                                    ChannelDirection direction)
{
    std::vector<PhysicalDevice::Range> taken;
    int highest = 0;
    for (const auto& other : all)
        if (other.id != draft.id && other.supports (direction))
        {
            taken.push_back (other.range (direction));
            highest = std::max (highest, other.range (direction).last);
        }

    const auto count = std::max (draft.physicalChannelCount, 1);
    const auto ceiling = std::max (highest + count + 32, 256);
    std::vector<int> result;
    for (int start = 1; start <= ceiling; ++start)
    {
        const PhysicalDevice::Range candidate { start, start + count - 1 };
        if (std::none_of (taken.begin(), taken.end(), [&] (auto r) { return r.overlaps (candidate); }))
            result.push_back (start);
    }
    const auto current = direction == ChannelDirection::send ? draft.sendChannelStart : draft.recvChannelStart;
    if (std::find (result.begin(), result.end(), current) == result.end())
        result.insert (std::upper_bound (result.begin(), result.end(), current), current);
    return result;
}

const PhysicalDevice* find (const std::vector<PhysicalDevice>& all, int channel, ChannelDirection direction,
                            std::optional<DeviceType> excluding)
{
    for (const auto& d : all)
        if (d.contains (channel, direction) && (! excluding || d.deviceType != *excluding))
            return &d;
    return nullptr;
}

bool isChannelOwned (const std::vector<PhysicalDevice>& all, int channel, DeviceType type)
{
    return std::any_of (all.begin(), all.end(), [&] (const PhysicalDevice& d)
    {
        return d.deviceType == type
               && (d.contains (channel, ChannelDirection::send) || d.contains (channel, ChannelDirection::recv));
    });
}

} // namespace PhysicalDevices
} // namespace StudioPatchbay
