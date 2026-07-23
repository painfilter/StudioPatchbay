import XCTest
@testable import PatchbayCore

/// External Plugins.xml is written by Cubase, so its shape is not ours to
/// control. These fixtures mirror the real structure: an "External Effects"
/// list of items, each carrying a name, an instrument flag, an optional MIDI
/// link, and Dispatcher (send) / Receiver (recv) bus lists whose Connections
/// entries end in "…|HDSPe Device <channel>".
final class ExternalPluginsParserTests: XCTestCase {

    // MARK: - Fixture helpers

    private func connections(_ channels: [Int], prefix: String) -> String {
        let items = channels
            .map { "<item value=\"\(prefix)|{GUID}|HDSPe Device \($0)\"/>" }
            .joined()
        return """
        <item><list name="Connections">\(items)</list></item>
        """
    }

    private func device(
        name: String,
        instrument: Bool = false,
        send: [Int] = [],
        recv: [Int] = [],
        midi: String? = nil
    ) -> String {
        var xml = "<item><string name=\"External Effect Name\" value=\"\(name)\"/>"
        xml += "<int name=\"External Instrument\" value=\"\(instrument ? 1 : 0)\"/>"
        if let midi {
            xml += "<member name=\"Midi Link\"><string name=\"NodePathID\" value=\"\(midi)\"/></member>"
        }
        if !send.isEmpty {
            xml += "<list name=\"Dispatcher Busses\">\(connections(send, prefix: "O"))</list>"
        }
        if !recv.isEmpty {
            xml += "<list name=\"Receiver Busses\">\(connections(recv, prefix: "I"))</list>"
        }
        return xml + "</item>"
    }

    private func document(_ items: String) -> Data {
        Data("""
        <?xml version="1.0" encoding="utf-8"?>
        <ExternalPlugins>
          <list name="External Effects">\(items)</list>
        </ExternalPlugins>
        """.utf8)
    }

    // MARK: - Classification

    func testInstrumentIsClassifiedAsInstrument() throws {
        let data = document(device(name: "OB6", instrument: true, send: [69, 70], recv: [69, 70]))
        let parsed = try ExternalPluginsParser.parse(data: data)

        XCTAssertEqual(parsed.instruments.count, 1)
        XCTAssertTrue(parsed.fxDevices.isEmpty)
        XCTAssertEqual(parsed.instruments.first?.name, "OB6")
        XCTAssertEqual(parsed.instruments.first?.type, .instrument)
    }

    func testDeviceWithReturnChannelsIsExternalFX() throws {
        let data = document(device(name: "Creme", send: [5, 6], recv: [5, 6]))
        let parsed = try ExternalPluginsParser.parse(data: data)

        XCTAssertEqual(parsed.fxDevices.first?.type, .fx)
        XCTAssertTrue(parsed.instruments.isEmpty)
    }

    func testDeviceWithoutReturnChannelsIsSendOnly() throws {
        let data = document(device(name: "Headphone Feed", send: [211]))
        let parsed = try ExternalPluginsParser.parse(data: data)

        XCTAssertEqual(parsed.fxDevices.first?.type, .sendOnly)
        XCTAssertEqual(parsed.fxDevices.first?.recvChannels, [])
    }

    // MARK: - Channels

    func testChannelsAreParsedAndSorted() throws {
        let data = document(device(name: "Pulse", send: [96, 95], recv: [96, 95]))
        let device = try XCTUnwrap(ExternalPluginsParser.parse(data: data).all.first)

        XCTAssertEqual(device.sendChannels, [95, 96])
        XCTAssertEqual(device.recvChannels, [95, 96])
        XCTAssertEqual(device.allChannels, [95, 96])
        XCTAssertTrue(device.isStereo)
    }

    func testMonoDeviceReportsSingleChannel() throws {
        let data = document(device(name: "dbx 160", send: [38], recv: [38]))
        let device = try XCTUnwrap(ExternalPluginsParser.parse(data: data).all.first)

        XCTAssertEqual(device.channelCount, 1)
        XCTAssertTrue(device.isMono)
        XCTAssertFalse(device.isStereo)
    }

    /// Send and return landing on different channels is a real wiring mistake
    /// the app is meant to surface, so it must survive parsing.
    func testMismatchedSendAndReturnIsDetected() throws {
        let data = document(device(name: "Analog Drive", send: [212], recv: [210]))
        let device = try XCTUnwrap(ExternalPluginsParser.parse(data: data).all.first)

        XCTAssertTrue(device.hasMismatchedChannels)
        XCTAssertEqual(device.allChannels, [210, 212])
    }

    func testMatchingSendAndReturnIsNotFlagged() throws {
        let data = document(device(name: "SA-4000", send: [1, 2], recv: [1, 2]))
        let device = try XCTUnwrap(ExternalPluginsParser.parse(data: data).all.first)

        XCTAssertFalse(device.hasMismatchedChannels)
    }

    // MARK: - MIDI

    func testMidiDeviceManagerPrefixIsStripped() throws {
        let data = document(device(
            name: "Matriarch", instrument: true, send: [79, 80], recv: [79, 80],
            midi: "Midi Device Manager\\Matriarch"
        ))
        let device = try XCTUnwrap(ExternalPluginsParser.parse(data: data).all.first)

        XCTAssertEqual(device.midiDevice, "Matriarch")
    }

    func testMissingMidiLinkYieldsEmptyString() throws {
        let data = document(device(name: "XFilter", send: [3, 4], recv: [3, 4]))
        let device = try XCTUnwrap(ExternalPluginsParser.parse(data: data).all.first)

        XCTAssertEqual(device.midiDevice, "")
    }

    // MARK: - Rejection rules

    func testUnnamedItemsAreSkipped() throws {
        let data = document(device(name: "", send: [1], recv: [1]))
        XCTAssertTrue(try ExternalPluginsParser.parse(data: data).all.isEmpty)
    }

    /// Cubase keeps entries for gear that is no longer wired up. A non-instrument
    /// with no connections at all is noise, not a device.
    func testUnwiredNonInstrumentIsSkipped() throws {
        let data = document(device(name: "Retired Compressor"))
        XCTAssertTrue(try ExternalPluginsParser.parse(data: data).all.isEmpty)
    }

    func testUnwiredInstrumentIsKept() throws {
        let data = document(device(name: "iMac Tiger", instrument: true))
        XCTAssertEqual(try ExternalPluginsParser.parse(data: data).instruments.count, 1)
    }

    // MARK: - Document level

    func testMissingExternalEffectsListYieldsEmptyResult() throws {
        let data = Data("""
        <?xml version="1.0" encoding="utf-8"?>
        <ExternalPlugins><list name="Something Else"/></ExternalPlugins>
        """.utf8)

        let parsed = try ExternalPluginsParser.parse(data: data)
        XCTAssertTrue(parsed.all.isEmpty)
    }

    func testMalformedXMLThrows() {
        let data = Data("<ExternalPlugins><list name=\"External Effects\">".utf8)
        XCTAssertThrowsError(try ExternalPluginsParser.parse(data: data))
    }

    func testEmptyDataThrows() {
        XCTAssertThrowsError(try ExternalPluginsParser.parse(data: Data()))
    }

    // MARK: - Aggregation

    func testChannelMapsSplitByDirection() throws {
        let data = document(
            device(name: "Instrument", instrument: true, send: [10], recv: [11])
            + device(name: "FX", send: [20, 21], recv: [20, 21])
        )
        let parsed = try ExternalPluginsParser.parse(data: data)

        XCTAssertEqual(parsed.sendChannelMap[10]?.first?.name, "Instrument")
        XCTAssertNil(parsed.sendChannelMap[11])
        XCTAssertEqual(parsed.recvChannelMap[11]?.first?.name, "Instrument")
        XCTAssertEqual(parsed.channelMap[20]?.count, 1)
        XCTAssertEqual(parsed.usedChannelCount, 4)
    }

    func testMultipleDevicesOnOneChannelAreAllReported() throws {
        let data = document(
            device(name: "First", send: [65], recv: [65])
            + device(name: "Second", send: [65], recv: [65])
        )
        let parsed = try ExternalPluginsParser.parse(data: data)

        XCTAssertEqual(parsed.channelMap[65]?.count, 2)
    }
}
