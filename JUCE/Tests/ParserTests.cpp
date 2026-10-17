#include <catch2/catch_test_macros.hpp>
#include "Model/CubaseConfigParser.h"
#include "Model/PlatformPaths.h"

using namespace StudioPatchbay;

// ============================================================================
// XML fixture helpers (mirror the Swift test fixtures exactly)
// ============================================================================

namespace
{
juce::String connections (const std::vector<int>& channels, const juce::String& prefix)
{
    juce::String items;
    for (auto ch : channels)
        items += "<item value=\"" + prefix + "|{GUID}|HDSPe Device " + juce::String (ch) + "\"/>";
    return "<item><list name=\"Connections\">" + items + "</list></item>";
}

juce::String device (const juce::String& name,
                     bool instrument = false,
                     const std::vector<int>& send = {},
                     const std::vector<int>& recv = {},
                     const juce::String& midi = "")
{
    juce::String xml = "<item><string name=\"External Effect Name\" value=\"" + name + "\"/>";
    xml += "<int name=\"External Instrument\" value=\"" + juce::String (instrument ? 1 : 0) + "\"/>";

    if (midi.isNotEmpty())
        xml += "<member name=\"Midi Link\"><string name=\"NodePathID\" value=\"" + midi + "\"/></member>";

    if (! send.empty())
        xml += "<list name=\"Dispatcher Busses\">" + connections (send, "O") + "</list>";

    if (! recv.empty())
        xml += "<list name=\"Receiver Busses\">" + connections (recv, "I") + "</list>";

    return xml + "</item>";
}

juce::File writeTestXml (const juce::String& items, const juce::String& rootTag = "ExternalPlugins",
                          const juce::String& listName = "External Effects")
{
    juce::String xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    xml += "<" + rootTag + ">";
    xml += "<list name=\"" + listName + "\">" + items + "</list>";
    xml += "</" + rootTag + ">";

    auto tempFile = juce::File::createTempFile (".xml");
    tempFile.replaceWithText (xml);
    return tempFile;
}
} // namespace

// ============================================================================
// ExternalPlugins parser tests
// ============================================================================

TEST_CASE ("Instrument is classified as instrument", "[parser]")
{
    auto f = writeTestXml (device ("OB6", true, {69, 70}, {69, 70}));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.instruments.size() == 1);
    REQUIRE (parsed.fxDevices.empty());
    REQUIRE (parsed.instruments[0].name == "OB6");
    REQUIRE (parsed.instruments[0].type == ParsedDevice::Type::instrument);
}

TEST_CASE ("Device with return channels is External FX", "[parser]")
{
    auto f = writeTestXml (device ("Creme", false, {5, 6}, {5, 6}));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.fxDevices.size() == 1);
    REQUIRE (parsed.fxDevices[0].type == ParsedDevice::Type::fx);
    REQUIRE (parsed.instruments.empty());
}

TEST_CASE ("Device without return channels is sendOnly", "[parser]")
{
    auto f = writeTestXml (device ("Headphone Feed", false, {211}));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.fxDevices.size() == 1);
    REQUIRE (parsed.fxDevices[0].type == ParsedDevice::Type::sendOnly);
    REQUIRE (parsed.fxDevices[0].recvChannels.empty());
}

TEST_CASE ("Channels are parsed and sorted", "[parser]")
{
    auto f = writeTestXml (device ("Pulse", false, {96, 95}, {96, 95}));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    auto all = parsed.all();
    REQUIRE (all.size() == 1);
    REQUIRE (all[0].sendChannels == std::vector<int>{95, 96});
    REQUIRE (all[0].recvChannels == std::vector<int>{95, 96});
    REQUIRE (all[0].isStereo());
}

TEST_CASE ("Mono device reports single channel", "[parser]")
{
    auto f = writeTestXml (device ("dbx 160", false, {38}, {38}));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    auto all = parsed.all();
    REQUIRE (all.size() == 1);
    REQUIRE (all[0].channelCount() == 1);
    REQUIRE (all[0].isMono());
    REQUIRE_FALSE (all[0].isStereo());
}

TEST_CASE ("Mismatched send and return is detected", "[parser]")
{
    auto f = writeTestXml (device ("Analog Drive", false, {212}, {210}));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    auto all = parsed.all();
    REQUIRE (all.size() == 1);
    REQUIRE (all[0].hasMismatchedChannels());
    auto allCh = all[0].allChannels();
    REQUIRE (allCh == std::vector<int>{210, 212});
}

TEST_CASE ("Matching send and return is not flagged", "[parser]")
{
    auto f = writeTestXml (device ("SA-4000", false, {1, 2}, {1, 2}));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE_FALSE (parsed.all()[0].hasMismatchedChannels());
}

TEST_CASE ("MIDI Device Manager prefix is stripped", "[parser]")
{
    auto f = writeTestXml (device ("Matriarch", true, {79, 80}, {79, 80},
                                   "Midi Device Manager\\Matriarch"));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.instruments[0].midiDevice == "Matriarch");
}

TEST_CASE ("Missing MIDI link yields empty string", "[parser]")
{
    auto f = writeTestXml (device ("XFilter", false, {3, 4}, {3, 4}));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.all()[0].midiDevice.isEmpty());
}

TEST_CASE ("Unnamed items are skipped", "[parser]")
{
    auto f = writeTestXml (device ("", false, {1}, {1}));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.all().empty());
}

TEST_CASE ("Unwired non-instrument is skipped", "[parser]")
{
    auto f = writeTestXml (device ("Retired Compressor"));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.all().empty());
}

TEST_CASE ("Unwired instrument is kept", "[parser]")
{
    auto f = writeTestXml (device ("iMac Tiger", true));
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.instruments.size() == 1);
}

TEST_CASE ("Missing External Effects list yields empty result", "[parser]")
{
    auto f = writeTestXml ("", "ExternalPlugins", "Something Else");
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.all().empty());
}

TEST_CASE ("Empty file yields empty result", "[parser]")
{
    auto f = juce::File::createTempFile (".xml");
    f.replaceWithText ("");
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    REQUIRE (parsed.all().empty());
}

TEST_CASE ("Channel maps split by direction", "[parser]")
{
    auto items = device ("Instrument", true, {10}, {11})
               + device ("FX", false, {20, 21}, {20, 21});
    auto f = writeTestXml (items);
    auto parsed = CubaseConfigParser::parseExternalPlugins (f);
    f.deleteFile();

    auto sendMap = parsed.buildChannelMap (ChannelDirection::send);
    auto recvMap = parsed.buildChannelMap (ChannelDirection::recv);

    REQUIRE (sendMap[10] == "Instrument");
    REQUIRE (sendMap.find (11) == sendMap.end());
    REQUIRE (recvMap[11] == "Instrument");
    REQUIRE (parsed.usedChannelCount() == 4);
}

// ============================================================================
// PlatformPaths tests
// ============================================================================

TEST_CASE ("App data directory is valid", "[paths]")
{
    auto dir = PlatformPaths::getAppDataDirectory();
    REQUIRE (dir.isDirectory());
    REQUIRE (dir.getFileName() == "StudioPatchbay");
}
