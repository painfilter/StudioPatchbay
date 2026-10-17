#include <catch2/catch_test_macros.hpp>
#include "Model/PhysicalDevice.h"
#include "Model/AudioPorts.h"
#include "Model/DeviceStore.h"

using namespace StudioPatchbay;

namespace
{
PhysicalDevice device (const juce::String& name, int count, int send, int recv,
                       DeviceDirectionality dir = DeviceDirectionality::bidirectional)
{
    PhysicalDevice d;
    d.name = name;
    d.shortName = name;
    d.portColor = ConnectorType::madi;
    d.physicalChannelCount = count;
    d.sendChannelStart = send;
    d.recvChannelStart = recv;
    d.directionality = dir;
    return d;
}

const juce::String dash (juce::CharPointer_UTF8 ("\xe2\x80\x93"));
} // namespace

TEST_CASE ("PhysicalDevice: ownership is per direction", "[devices]")
{
    const auto d = device ("ADA8", 8, 1, 9, DeviceDirectionality::bidirectional);
    CHECK (d.contains (1, ChannelDirection::send));
    CHECK_FALSE (d.contains (1, ChannelDirection::recv));
    CHECK (d.contains (16, ChannelDirection::recv));
    CHECK (*d.physicalIO (11, ChannelDirection::recv) == 3);
    CHECK (d.channelRange().first == 1);
    CHECK (d.channelRange().last == 16);

    const auto outOnly = device ("Monitors", 2, 1, 1, DeviceDirectionality::sendOnly);
    CHECK_FALSE (outOnly.contains (1, ChannelDirection::recv));
    CHECK (outOnly.channelRange().last == 2);
}

TEST_CASE ("PhysicalDevice: range labels", "[devices]")
{
    const auto d = device ("ADA8", 8, 1, 1);
    CHECK (d.physicalRangeLabel ({ 3 }, ChannelDirection::send) == "I/O 3");
    CHECK (d.physicalRangeLabel ({ 2, 1 }, ChannelDirection::send) == "I/O 1" + dash + "2");
    CHECK (d.physicalRangeLabel ({ 1, 4 }, ChannelDirection::send) == "I/O 1+4");
    CHECK (d.physicalRangeLabel ({ 20 }, ChannelDirection::send).isEmpty());
}

TEST_CASE ("PhysicalDevice: conflicts only in shared directions", "[devices]")
{
    auto a = device ("A", 8, 1, 1);
    auto b = device ("B", 8, 5, 20);
    const auto issues = PhysicalDevices::conflicts (b, { a, b });
    REQUIRE (issues.size() == 1);
    CHECK (issues[0].direction == ChannelDirection::send);
    CHECK (issues[0].first == 5);
    CHECK (issues[0].last == 8);
    CHECK (issues[0].otherName == "A");

    // A's receive range and B's send range share numbers: no conflict.
    auto c = device ("C", 8, 9, 1, DeviceDirectionality::sendOnly);
    CHECK (PhysicalDevices::conflicts (c, { a, c }).empty());
}

TEST_CASE ("PhysicalDevice: import follows the same rule", "[devices]")
{
    const auto saved = device ("Saved", 8, 1, 1);
    // Same ids in a file must not hide a conflict inside it.
    auto x = device ("X", 8, 100, 100);
    auto y = x;
    y.name = "Y";
    const auto checks = PhysicalDevices::checkImport ({ x, y, device ("Z", 4, 5, 300) }, { saved },
                                                     { { "MADI", ChannelDirection::send, 1, 64 } });
    REQUIRE (checks.size() == 3);
    CHECK (checks[0].replaceConflicts.size() == 2);   // with Y, both directions
    CHECK (checks[2].replaceConflicts.empty());
    CHECK (checks[2].addConflicts.size() == 1);       // OUT 5-8 with Saved
    CHECK (checks[0].unmatchedDirections.size() == 2);
    CHECK (checks[2].unmatchedDirections == std::vector<ChannelDirection> { ChannelDirection::recv });
}

TEST_CASE ("PhysicalDevice: free start channels", "[devices]")
{
    auto a = device ("A", 8, 1, 1);
    auto draft = device ("New", 4, 1, 1);
    const auto starts = PhysicalDevices::freeStartChannels (draft, { a }, ChannelDirection::send);
    CHECK (starts.front() == 1);                 // the current value stays listed
    CHECK (starts[1] == 9);
    CHECK (starts.back() == 256);
    a.directionality = DeviceDirectionality::recvOnly;
    CHECK (PhysicalDevices::freeStartChannels (draft, { a }, ChannelDirection::send)[1] == 2);
}

TEST_CASE ("PhysicalDevice: reads the Swift app's file", "[devices]")
{
    // As the Swift app writes it, plus an old entry with a numeric id and
    // without the later keys.
    const juce::String json = R"([
  {
    "customColorHex" : "#FF8800",
    "deviceType" : "audioInterface",
    "directionality" : "sendOnly",
    "id" : "9F3C1E2A-0B4D-4C55-8E7F-112233445566",
    "name" : "Babyface",
    "physicalChannelCount" : 4,
    "portColor" : "usb",
    "recvChannelStart" : 1,
    "sendChannelStart" : 1,
    "shortName" : "BF"
  },
  {
    "id" : 3,
    "name" : "Ferrofish",
    "physicalChannelCount" : 16,
    "portColor" : "adat",
    "recvChannelStart" : 195,
    "sendChannelStart" : 197,
    "shortName" : "FF"
  }
])";
    std::vector<PhysicalDevice> devices;
    REQUIRE (PhysicalDevices::fromJson (json, devices).wasOk());
    REQUIRE (devices.size() == 2);
    CHECK (devices[0].id.toDashedString().toUpperCase() == "9F3C1E2A-0B4D-4C55-8E7F-112233445566");
    CHECK (devices[0].deviceType == DeviceType::audioInterface);
    CHECK (devices[0].directionality == DeviceDirectionality::sendOnly);
    CHECK (devices[0].customColorHex == "#FF8800");
    CHECK (devices[1].deviceType == DeviceType::converter);
    CHECK (devices[1].directionality == DeviceDirectionality::bidirectional);
    CHECK (! devices[1].id.isNull());

    // Written back and read again, nothing changes.
    std::vector<PhysicalDevice> again;
    REQUIRE (PhysicalDevices::fromJson (PhysicalDevices::toJson (devices), again).wasOk());
    REQUIRE (again.size() == 2);
    CHECK (again[1].id == devices[1].id);
    CHECK (PhysicalDevices::toJson (again) == PhysicalDevices::toJson (devices));
    CHECK (PhysicalDevices::toJson (devices).contains ("\"id\" : \"9F3C1E2A-0B4D-4C55-8E7F-112233445566\""));
}

TEST_CASE ("PhysicalDevice: a broken file is an error, not an empty list", "[devices]")
{
    std::vector<PhysicalDevice> devices { device ("Keep", 2, 1, 1) };
    CHECK (PhysicalDevices::fromJson (R"([{ "name" : "No ranges", "shortName" : "x", "portColor" : "madi" }])", devices).failed());
    CHECK (PhysicalDevices::fromJson (R"([{ "name" : "x", "shortName" : "x", "portColor" : "laser",
        "physicalChannelCount" : 2, "sendChannelStart" : 1, "recvChannelStart" : 1 }])", devices).failed());
    CHECK (devices.size() == 1);   // untouched
}

TEST_CASE ("AudioPorts: send and receive runs pair by position", "[ports]")
{
    // RayDAT-like: ADAT receive starts at 195, send at 197; two AES ports far apart.
    std::vector<DetectedPort> detected {
        { "ADAT", ChannelDirection::recv, 195, 210 },
        { "ADAT", ChannelDirection::send, 197, 212 },
        { "AES", ChannelDirection::send, 65, 66 },
        { "AES", ChannelDirection::recv, 65, 66 },
        { "AES", ChannelDirection::send, 225, 226 },
        { "MADI", ChannelDirection::send, 1, 64 },
    };
    const auto ports = AudioPorts::build (detected);
    REQUIRE (ports.size() == 4);
    CHECK (ports[0].name == "MADI");
    CHECK (ports[1].name == "AES 1");
    CHECK (ports[2].name == "ADAT");
    CHECK (ports[2].channelStart == 195);
    CHECK (ports[2].channelEnd == 212);
    CHECK (ports[3].name == "AES 2");
    CHECK (ports[3].interfaceName == "AES Interface");
    CHECK (ports[2].color == ConnectorType::adat);
    CHECK (AudioPorts::stableLabelKey (ports[3]) == "AES@225");
    CHECK (AudioPorts::find (ports, 70) == nullptr);
    CHECK (AudioPorts::find (ports, 200)->name == "ADAT");
}

TEST_CASE ("AudioPorts: labels by stable key, then the older keys", "[ports]")
{
    auto ports = AudioPorts::build ({ { "MADI", ChannelDirection::send, 1, 64 },
                                      { "AES", ChannelDirection::send, 65, 66 },
                                      { "ADAT", ChannelDirection::send, 67, 74 } });
    AudioPorts::applyLabels (ports, { { "MADI@1", "HDSPe MADI FX" }, { "ch65", "Old AES" }, { "ADAT", "Oldest" } });
    CHECK (ports[0].interfaceName == "HDSPe MADI FX");
    CHECK (ports[1].interfaceName == "Old AES");
    CHECK (ports[2].interfaceName == "Oldest");
}

TEST_CASE ("ParsedDevice: stereo and mono as in the Swift app", "[devices]")
{
    ParsedDevice d;
    d.sendChannels = { 1, 2 };
    d.recvChannels = { 5, 6 };
    CHECK (d.isStereo());       // four channels, but two per direction
    CHECK_FALSE (d.isMono());

    ParsedDevice m;
    m.sendChannels = {};
    m.recvChannels = { 3 };
    CHECK (m.isMono());
}

TEST_CASE ("DeviceStore: keeps an unreadable file and reports it", "[devices]")
{
    const auto dir = juce::File::createTempFile ("store");
    dir.createDirectory();
    {
        DeviceStore store (dir);
        store.load();
        CHECK (store.getError().isEmpty());
        CHECK (store.devicesFile().existsAsFile());   // first launch writes an empty list

        store.add (device ("ADA8", 8, 1, 1));
        store.setLabel ("MADI@1", "  MADI FX ");
        DeviceStore reread (dir);
        reread.load();
        REQUIRE (reread.getDevices().size() == 1);
        CHECK (reread.getLabels().at ("MADI@1") == "MADI FX");

        const auto copy = reread.duplicate (reread.getDevices()[0]);
        CHECK (copy.name == "ADA8 copy");
        CHECK (copy.id != reread.getDevices()[0].id);

        store.devicesFile().replaceWithText ("{ not json");
        DeviceStore broken (dir);
        broken.load();
        CHECK (broken.getError().isNotEmpty());
        CHECK (broken.getDevices().empty());
        broken.add (device ("New", 2, 1, 1));
        CHECK (dir.findChildFiles (juce::File::findFiles, false, "physicalDevices.unreadable-*.json").size() == 1);
    }
    dir.deleteRecursively();
}
