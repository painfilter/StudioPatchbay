#pragma once

#include "DataModel.h"

#include <optional>

namespace StudioPatchbay
{

// A converter, a self-contained audio interface or other hardware, defined by
// the user. Stored in physicalDevices.json in the same format as the Swift
// app, so both read the same file.
//
// Cubase numbers send and receive channels independently: a device owns a
// channel only in a direction it supports and within that direction's range.
struct PhysicalDevice
{
    juce::Uuid id;
    juce::String name;
    juce::String shortName;
    ConnectorType portColor = ConnectorType::other;
    DeviceType deviceType = DeviceType::converter;
    int physicalChannelCount = 0;
    int sendChannelStart = 0;   // first channel Cubase -> hardware (OUT)
    int recvChannelStart = 0;   // first channel hardware -> Cubase (IN)
    DeviceDirectionality directionality = DeviceDirectionality::bidirectional;
    juce::String customColorHex;   // "#RRGGBB", empty = the connector's colour

    struct Range
    {
        int first = 0, last = -1;   // inclusive
        bool contains (int ch) const { return ch >= first && ch <= last; }
        bool overlaps (Range o) const { return first <= o.last && o.first <= last; }
    };

    bool supports (ChannelDirection) const;
    Range range (ChannelDirection) const;
    Range channelRange() const;   // union of the supported directions, for headers
    bool contains (int channel, ChannelDirection) const;

    // 1-based physical input/output number of a channel, if this device has it.
    std::optional<int> physicalIO (int channel, ChannelDirection) const;
    // "I/O 3", "I/O 1–2" or "I/O 1+4" for a set of channels; empty if none.
    juce::String physicalRangeLabel (const std::vector<int>& channels, ChannelDirection) const;

    // A copy with a new id, as Duplicate and Import do.
    PhysicalDevice withNewId() const;

    bool operator== (const PhysicalDevice&) const = default;
};

// Two devices claiming the same channels in the same direction.
struct ChannelConflict
{
    ChannelDirection direction;
    int first, last;         // the shared channels
    juce::String otherName;
};

// What importing a file would do, device by device.
struct ImportCheck
{
    std::vector<ChannelConflict> addConflicts;      // with saved devices and the rest of the file
    std::vector<ChannelConflict> replaceConflicts;  // within the file only
    std::vector<ChannelDirection> unmatchedDirections; // no detected port overlaps that range
};

namespace PhysicalDevices
{
// Raw values as the Swift app writes them ("madi", "audioInterface", "sendOnly", ...).
juce::String toString (ConnectorType);
juce::String toString (DeviceType);
juce::String toString (DeviceDirectionality);
std::optional<ConnectorType> connectorFromString (const juce::String&);
std::optional<DeviceType> deviceTypeFromString (const juce::String&);
std::optional<DeviceDirectionality> directionalityFromString (const juce::String&);

// Reads a JSON array of devices. Missing optional keys take the Swift
// defaults; an old numeric id gets a new UUID. Anything else wrong fails the
// whole file, with a message, rather than dropping devices silently.
juce::Result fromJson (const juce::String& json, std::vector<PhysicalDevice>& out);
// Pretty-printed with sorted keys, like the Swift encoder.
juce::String toJson (const std::vector<PhysicalDevice>&);

// Every overlap with another device (by id) in the same direction; empty
// means the candidate can be saved.
std::vector<ChannelConflict> conflicts (const PhysicalDevice& candidate, const std::vector<PhysicalDevice>& all);

// The same rule for a file being imported: against the saved devices and,
// by position (ids in a file are not trusted), against the rest of the file.
// Port matching is informational: a setup may be for hardware not connected.
std::vector<ImportCheck> checkImport (const std::vector<PhysicalDevice>& batch,
                                      const std::vector<PhysicalDevice>& existing,
                                      const std::vector<DetectedPort>& detected);

// Start channels at which a device of this size overlaps no other device in
// the direction (the device's own range never counts), up to past the highest
// used channel; the current start is always included.
std::vector<int> freeStartChannels (const PhysicalDevice& draft, const std::vector<PhysicalDevice>& all,
                                    ChannelDirection);

// The first device that owns a channel in a direction, optionally ignoring a type.
const PhysicalDevice* find (const std::vector<PhysicalDevice>&, int channel, ChannelDirection,
                            std::optional<DeviceType> excluding = std::nullopt);
bool isChannelOwned (const std::vector<PhysicalDevice>&, int channel, DeviceType);
} // namespace PhysicalDevices

} // namespace StudioPatchbay
