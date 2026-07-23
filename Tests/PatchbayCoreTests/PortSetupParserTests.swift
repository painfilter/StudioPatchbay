import XCTest
@testable import PatchbayCore

/// Port Setup.xml comes in two structurally different shapes depending on the
/// driver: a rich "PortDescriptors" list carrying absolute Cubase channel
/// numbers (ASIO), and a flat "Settings" list with no channel numbers at all
/// (CoreAudio). Both are covered here, including the rule that the rich format
/// always wins when a file happens to contain both.
final class PortSetupParserTests: XCTestCase {

    // MARK: - Fixtures

    private func descriptor(id: String, sysName: String) -> String {
        """
        <item><string name="ID" value="\(id)"/><string name="SysName" value="\(sysName)"/></item>
        """
    }

    private func aggregateDocument(_ items: String) -> Data {
        Data("""
        <?xml version="1.0" encoding="utf-8"?>
        <PortSetup><list name="PortDescriptors">\(items)</list></PortSetup>
        """.utf8)
    }

    private func settingsDocument(_ ids: [String]) -> Data {
        let items = ids.map { "<item><string name=\"ID\" value=\"\($0)\"/></item>" }.joined()
        return Data("""
        <?xml version="1.0" encoding="utf-8"?>
        <PortSetup><list name="Settings">\(items)</list></PortSetup>
        """.utf8)
    }

    // MARK: - Rich format

    func testContiguousChannelsBecomeOnePort() throws {
        let data = aggregateDocument(
            descriptor(id: "O|{G}|1", sysName: "MADI 1 - 1")
            + descriptor(id: "O|{G}|2", sysName: "MADI 2 - 2")
            + descriptor(id: "O|{G}|3", sysName: "MADI 3 - 3")
        )
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(ports.count, 1)
        XCTAssertEqual(ports.first?.family, "MADI")
        XCTAssertEqual(ports.first?.channelRange, 1...3)
        XCTAssertEqual(ports.first?.direction, .send)
        XCTAssertFalse(ports.first?.isApproximate ?? true)
    }

    /// The same label legitimately appears twice on different hardware, e.g. AES
    /// on a MADI FX and again on a RayDAT. A gap in channel numbers is what
    /// separates them, so they must not be merged into one impossible range.
    func testSameFamilyWithAGapSplitsIntoTwoPorts() throws {
        let data = aggregateDocument(
            descriptor(id: "I|{G}|193", sysName: "AES L - 193")
            + descriptor(id: "I|{G}|194", sysName: "AES R - 194")
            + descriptor(id: "I|{G}|227", sysName: "AES L - 227")
            + descriptor(id: "I|{G}|228", sysName: "AES R - 228")
        )
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(ports.count, 2)
        XCTAssertEqual(ports.map(\.channelRange), [193...194, 227...228])
        XCTAssertEqual(Set(ports.map(\.family)), ["AES"])
    }

    func testDirectionComesFromTheIdPrefix() throws {
        let data = aggregateDocument(
            descriptor(id: "O|{G}|1", sysName: "MADI 1 - 1")
            + descriptor(id: "I|{G}|50", sysName: "ADAT 1 - 50")
        )
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(ports.first(where: { $0.family == "MADI" })?.direction, .send)
        XCTAssertEqual(ports.first(where: { $0.family == "ADAT" })?.direction, .recv)
    }

    func testTrailingIndexAndLeftRightAreStrippedFromFamily() throws {
        let data = aggregateDocument(
            descriptor(id: "I|{G}|10", sysName: "ADAT 12 - 10")
            + descriptor(id: "I|{G}|40", sysName: "Phones L - 40")
        )
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(Set(ports.map(\.family)), ["ADAT", "Phones"])
    }

    /// Some non-audio entries share the same shape but use huge internal ids that
    /// are not real channel numbers. They must not pollute the port list.
    func testImplausibleChannelNumbersAreRejected() throws {
        let data = aggregateDocument(
            descriptor(id: "O|{G}|1", sysName: "MADI 1 - 1")
            + descriptor(id: "O|{G}|99999", sysName: "Bogus 1 - 99999")
        )
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(ports.count, 1)
        XCTAssertEqual(ports.first?.family, "MADI")
    }

    func testEntriesWithoutASysNameAreIgnored() throws {
        let data = aggregateDocument(
            "<item><string name=\"ID\" value=\"O|{G}|7\"/></item>"
            + descriptor(id: "O|{G}|1", sysName: "MADI 1 - 1")
        )
        XCTAssertEqual(try PortSetupParser.parse(data: data).count, 1)
    }

    func testPortsAreSortedByFirstChannel() throws {
        let data = aggregateDocument(
            descriptor(id: "I|{G}|200", sysName: "ADAT 1 - 200")
            + descriptor(id: "I|{G}|5", sysName: "MADI 1 - 5")
        )
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(ports.map(\.channelRange.lowerBound), [5, 200])
    }

    // MARK: - Flat fallback format

    func testSettingsFallbackNumbersChannelsSequentiallyAndFlagsThem() throws {
        let data = settingsDocument([
            "O|{G}|MOTU MicroBook 1",
            "O|{G}|MOTU MicroBook 2",
            "O|{G}|MOTU MicroBook 3",
        ])
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(ports.count, 1)
        XCTAssertEqual(ports.first?.family, "MOTU MicroBook")
        XCTAssertEqual(ports.first?.channelRange, 1...3)
        XCTAssertTrue(ports.first?.isApproximate ?? false,
                      "Synthesised numbering must be marked, so the UI never presents it as real Cubase data")
    }

    func testSettingsFallbackSeparatesDirections() throws {
        let data = settingsDocument([
            "O|{G}|Built-in Audio L",
            "O|{G}|Built-in Audio R",
            "I|{G}|Built-in Audio L",
        ])
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(ports.count, 2)
        XCTAssertEqual(ports.first(where: { $0.direction == .send })?.channelRange, 1...2)
        XCTAssertEqual(ports.first(where: { $0.direction == .recv })?.channelRange, 1...1)
    }

    func testSettingsFallbackSkipsMidiSystemEntries() throws {
        let data = settingsDocument([
            "O|MIDI System|Some MIDI Port",
            "O|{G}|Built-in Audio L",
        ])
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(ports.count, 1)
        XCTAssertEqual(ports.first?.family, "Built-in Audio")
    }

    // MARK: - Format precedence and edges

    func testRichFormatWinsWhenBothListsArePresent() throws {
        let data = Data("""
        <?xml version="1.0" encoding="utf-8"?>
        <PortSetup>
          <list name="PortDescriptors">\(descriptor(id: "O|{G}|1", sysName: "MADI 1 - 1"))</list>
          <list name="Settings"><item><string name="ID" value="O|{G}|Built-in Audio L"/></item></list>
        </PortSetup>
        """.utf8)
        let ports = try PortSetupParser.parse(data: data)

        XCTAssertEqual(ports.count, 1)
        XCTAssertEqual(ports.first?.family, "MADI")
        XCTAssertFalse(ports.first?.isApproximate ?? true)
    }

    func testDocumentWithNeitherListYieldsNoPorts() throws {
        let data = Data("<?xml version=\"1.0\"?><PortSetup><list name=\"Other\"/></PortSetup>".utf8)
        XCTAssertTrue(try PortSetupParser.parse(data: data).isEmpty)
    }

    func testMalformedXMLThrows() {
        XCTAssertThrowsError(try PortSetupParser.parse(data: Data("<PortSetup>".utf8)))
    }
}
