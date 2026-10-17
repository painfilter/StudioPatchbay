#pragma once

#include "PhysicalDevice.h"
#include "AudioPorts.h"

#include <map>
#include <optional>

namespace StudioPatchbay
{

// What the patchbay shows, worked out as in the Swift app, without drawing.
//
// Each interface port becomes a section; inside it one block per physical
// device (a converter), found by its receive range, else its send range for
// output-only devices. A channel no device claims shows as a block of its
// own only if something uses it. Audio interfaces are not part of the
// interface's port map and get a section of their own; Control Room buses too.
namespace Patchbay
{
using DeviceMap = std::map<int, std::vector<ParsedDevice>>;
using BusMap = std::map<int, std::vector<ControlRoomBus>>;

struct Block
{
    std::optional<PhysicalDevice> device;   // empty: "Unknown"
    int first = 0, last = 0;                // the channels this block takes in the port

    // The cells of one row: the device's own range in that direction, so OUT
    // and IN always show its physical I/O 1..N; else the block's channels.
    std::vector<int> rowChannels (ChannelDirection) const;
};

struct PortSection
{
    AudioPort port;
    std::vector<Block> blocks;
    int usedCount = 0;   // channels of the port in use in either direction
};

struct Layout
{
    std::vector<PortSection> ports;
    std::vector<PhysicalDevice> audioInterfaces;   // by first send channel
};

BusMap busesByChannel (const std::vector<ControlRoomBus>&);

Layout build (const ParsedDevices&, const std::vector<ControlRoomBus>&, const std::vector<AudioPort>&,
              const std::vector<PhysicalDevice>&);

// Ports touched by a device's channels or a Control Room bus, without ports
// whose whole range an earlier port already covers ("Phones" inside "ADAT").
std::vector<AudioPort> activePorts (const ParsedDevices&, const std::vector<ControlRoomBus>&,
                                    const std::vector<AudioPort>&);

// "ch 1–8" when both directions match, else "OUT ch 1–8  ·  IN ch 3–10";
// one direction for one-way devices.
juce::String headerChannelLabel (const PhysicalDevice&);

// "3–6" for a contiguous run of two or more, else "3+7".
juce::String rangeString (std::vector<int> channels);

// For the export: every device using one of the block's channels, and the
// block's own channels it uses in each direction.
struct Connection
{
    ParsedDevice device;
    juce::String out, in;
};
std::vector<Connection> connections (const Block&, const DeviceMap& send, const DeviceMap& recv);
std::vector<std::pair<ControlRoomBus, juce::String>> busConnections (const Block&, const BusMap&);
} // namespace Patchbay

} // namespace StudioPatchbay
