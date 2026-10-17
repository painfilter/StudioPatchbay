#include <catch2/catch_test_macros.hpp>
#include "Model/Browsing.h"
#include "Model/PatchbayLayout.h"
#include "Model/SignalFlow.h"

using namespace StudioPatchbay;

namespace
{
const juce::String dash (juce::CharPointer_UTF8 ("\xe2\x80\x93"));

ParsedDevice parsed (const juce::String& name, ParsedDevice::Type type, std::vector<int> send, std::vector<int> recv,
                     const juce::String& midi = {})
{
    return { name, type, std::move (send), std::move (recv), midi };
}

PhysicalDevice physical (const juce::String& name, int count, int send, int recv,
                         DeviceDirectionality dir = DeviceDirectionality::bidirectional,
                         DeviceType type = DeviceType::converter)
{
    PhysicalDevice d;
    d.name = name;
    d.shortName = name;
    d.physicalChannelCount = count;
    d.sendChannelStart = send;
    d.recvChannelStart = recv;
    d.directionality = dir;
    d.deviceType = type;
    return d;
}

ParsedDevices studio()
{
    ParsedDevices devices;
    devices.instruments = { parsed ("Juno", ParsedDevice::Type::instrument, {}, { 195, 196 }, "Juno MIDI") };
    devices.fxDevices = { parsed ("1176", ParsedDevice::Type::fx, { 3 }, { 3 }),
                          parsed ("Reverb Send", ParsedDevice::Type::sendOnly, { 9, 10 }, {}) };
    return devices;
}
} // namespace

TEST_CASE ("Browsing: search and filter as in the Swift app", "[views]")
{
    const auto devices = studio();
    const std::vector<PhysicalDevice> hardware { physical ("Ferrofish", 16, 193, 193) };
    const auto names = [] (const std::vector<ParsedDevice>& list)
    {
        juce::StringArray n;
        for (auto& d : list) n.add (d.name);
        return n;
    };

    CHECK (names (Browsing::filter (devices, FilterMode::all, "juno midi", hardware)) == juce::StringArray { "Juno" });
    CHECK (names (Browsing::filter (devices, FilterMode::all, "19", hardware)) == juce::StringArray { "Juno" });
    CHECK (names (Browsing::filter (devices, FilterMode::all, "ferro", hardware)) == juce::StringArray { "Juno" });
    CHECK (names (Browsing::filter (devices, FilterMode::sendOnly, {}, hardware)) == juce::StringArray { "Reverb Send" });
    CHECK (names (Browsing::filter (devices, FilterMode::stereo, {}, hardware)) == juce::StringArray { "Juno", "Reverb Send" });
    CHECK (names (Browsing::filter (devices, FilterMode::mono, {}, hardware)) == juce::StringArray { "1176" });
    CHECK (names (Browsing::filter (devices, FilterMode::midi, {}, hardware)) == juce::StringArray { "Juno" });
    CHECK (names (Browsing::inPatchbayOrder (devices.all())) == juce::StringArray { "1176", "Reverb Send", "Juno" });
}

TEST_CASE ("NavigationHistory: back and forward like a browser", "[views]")
{
    NavigationHistory history;
    CHECK_FALSE (history.canGoBack());
    history.jumped ({ 0, std::nullopt });          // from Cards to ch 5
    history.jumped ({ 1, 5 });                     // from ch 5 to ch 9
    auto back = history.back ({ 1, 9 });
    REQUIRE (back.has_value());
    CHECK (*back == NavigationHistory::Point { 1, 5 });
    CHECK (history.canGoForward());
    auto forward = history.forward (*back);
    CHECK (*forward == NavigationHistory::Point { 1, 9 });
    history.back ({ 1, 9 });
    history.jumped ({ 1, 5 });                     // a new jump drops Forward
    CHECK_FALSE (history.canGoForward());
}

TEST_CASE ("Patchbay: blocks per converter, asymmetric and one-way devices", "[views]")
{
    ParsedDevices devices;
    devices.fxDevices = { parsed ("Comp", ParsedDevice::Type::fx, { 3 }, { 3 }),
                          parsed ("Synth", ParsedDevice::Type::fx, { 20 }, { 18 }),
                          parsed ("Loose", ParsedDevice::Type::fx, { 40 }, { 40 }),
                          parsed ("USB Box", ParsedDevice::Type::fx, { 30 }, { 30 }) };
    const std::vector<AudioPort> ports { { 0, "MADI", 1, 64, ConnectorType::madi, "MADI Interface" } };
    const std::vector<PhysicalDevice> hardware {
        physical ("SP8", 8, 1, 1),
        physical ("Asym", 8, 19, 17),                                   // send runs two further
        physical ("Monitors", 2, 27, 27, DeviceDirectionality::sendOnly),
        physical ("USB", 4, 29, 29, DeviceDirectionality::bidirectional, DeviceType::audioInterface) };

    const auto layout = Patchbay::build (devices, {}, ports, hardware);
    REQUIRE (layout.ports.size() == 1);
    const auto& blocks = layout.ports[0].blocks;
    juce::StringArray names;
    for (auto& b : blocks)
        names.add (b.device ? b.device->name : juce::String ("Unknown@") + juce::String (b.first));
    // Asym appears once although its send range reaches past its receive range;
    // the interface's channels are left out; an unused unknown channel is hidden.
    CHECK (names == juce::StringArray { "SP8", "Asym", "Monitors", "Unknown@40" });
    CHECK (blocks[1].rowChannels (ChannelDirection::send).front() == 19);
    CHECK (blocks[1].rowChannels (ChannelDirection::recv).front() == 17);
    CHECK (blocks[2].rowChannels (ChannelDirection::recv) == std::vector<int> { 27, 28 });   // no IN: block channels
    CHECK (layout.audioInterfaces.size() == 1);
    CHECK (layout.ports[0].usedCount == 5);

    const auto connected = Patchbay::connections (blocks[1], devices.devicesByChannel (ChannelDirection::send),
                                                  devices.devicesByChannel (ChannelDirection::recv));
    REQUIRE (connected.size() == 1);
    CHECK (connected[0].device.name == "Synth");
    CHECK (connected[0].out == "20");
    CHECK (connected[0].in == "18");
}

TEST_CASE ("Patchbay: ports, labels and ranges", "[views]")
{
    ParsedDevices devices;
    devices.fxDevices = { parsed ("A", ParsedDevice::Type::fx, { 196 }, { 196 }) };
    const std::vector<AudioPort> ports { { 0, "ADAT", 195, 210, ConnectorType::adat, "x" },
                                         { 1, "Phones", 195, 196, ConnectorType::analogLine, "x" },
                                         { 2, "AES", 300, 301, ConnectorType::aes, "x" } };
    std::vector<ControlRoomBus> buses { { 0, "Phones", ControlRoomBus::Kind::phones, { 300 } } };
    const auto active = Patchbay::activePorts (devices, buses, ports);
    REQUIRE (active.size() == 2);   // Phones lies inside ADAT
    CHECK (active[0].name == "ADAT");
    CHECK (active[1].name == "AES");

    CHECK (Patchbay::rangeString ({ 4, 3, 5 }) == "3" + dash + "5");
    CHECK (Patchbay::rangeString ({ 3, 7 }) == "3+7");
    CHECK (Patchbay::rangeString ({ 3 }) == "3");
    CHECK (Patchbay::headerChannelLabel (physical ("x", 8, 1, 1)) == "ch 1" + dash + "8");
    CHECK (Patchbay::headerChannelLabel (physical ("x", 8, 1, 1, DeviceDirectionality::recvOnly)) == "IN ch 1" + dash + "8");
}

TEST_CASE ("SignalFlow: the path of an instrument and of an effect", "[views]")
{
    const std::vector<PhysicalDevice> hardware { physical ("Ferrofish", 16, 193, 193) };
    const std::vector<AudioPort> ports { { 0, "ADAT", 193, 208, ConnectorType::adat, "RayDAT" } };
    using K = SignalFlow::Node::Kind;
    const auto kinds = [] (const std::vector<SignalFlow::Node>& nodes)
    {
        std::vector<K> k;
        for (auto& n : nodes) k.push_back (n.kind);
        return k;
    };

    const auto instrument = SignalFlow::build (studio().instruments[0], hardware, ports);
    CHECK (kinds (instrument) == std::vector<K> { K::cubaseInstrumentTrack, K::midiOut, K::instrument, K::analogOut,
                                                  K::interfaceReceive, K::cubaseReceived });
    CHECK (instrument[3].text == juce::String (juce::CharPointer_UTF8 ("Ferrofish  \xc2\xb7  I/O 3")) + dash + "4");
    CHECK (instrument[4].text == "RayDAT");
    CHECK (instrument[4].detail == "195 + 196");
    CHECK (*instrument[4].jumpChannel == 195);

    const auto fx = SignalFlow::build (parsed ("Comp", ParsedDevice::Type::fx, { 3 }, { 4 }), {}, {});
    CHECK (kinds (fx) == std::vector<K> { K::cubaseFxSend, K::interfaceSend, K::hardware, K::interfaceReceive,
                                          K::cubaseFxReturn });
    CHECK (fx[1].noConverter);
    CHECK (fx[1].text == "RME HDSPe");
}
