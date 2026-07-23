import XCTest
@testable import PatchbayCore

/// Control Room is optional in Cubase, so absence and half-configured states are
/// normal inputs here rather than error cases.
final class ControlRoomParserTests: XCTestCase {

    private func bus(name: String, type: Int?, channels: [Int]?) -> String {
        var xml = "<item><member name=\"Name\"><string name=\"String\" value=\"\(name)\"/></member>"
        if let type {
            xml += "<int name=\"Type\" value=\"\(type)\"/>"
        }
        if let channels {
            let items = channels
                .map { "<item value=\"O|{GUID}|HDSPe Device \($0)\"/>" }
                .joined()
            xml += "<member name=\"OutputBus\"><list name=\"Connections\">\(items)</list></member>"
        }
        return xml + "</item>"
    }

    private func document(_ items: String) -> Data {
        Data("""
        <?xml version="1.0" encoding="utf-8"?>
        <ControlRoom>
          <member name="Preset"><list name="Channel">\(items)</list></member>
        </ControlRoom>
        """.utf8)
    }

    // MARK: - Kinds

    func testTypeFifteenIsPhones() throws {
        let data = document(bus(name: "Phones", type: 15, channels: [195, 196]))
        let buses = try ControlRoomParser.parse(data: data)

        XCTAssertEqual(buses.first?.kind, .phones)
        XCTAssertEqual(buses.first?.channels, [195, 196])
    }

    func testTypeSixteenIsSpeakerOut() throws {
        let data = document(bus(name: "Genelec (AES)", type: 16, channels: [193, 194]))
        XCTAssertEqual(try ControlRoomParser.parse(data: data).first?.kind, .speaker)
    }

    func testOtherTypesFallBackToStudio() throws {
        let data = document(bus(name: "Phones Amp", type: 12, channels: [209, 210]))
        XCTAssertEqual(try ControlRoomParser.parse(data: data).first?.kind, .studio)
    }

    func testMissingTypeFallsBackToStudio() throws {
        let data = document(bus(name: "Unlabelled", type: nil, channels: [1]))
        XCTAssertEqual(try ControlRoomParser.parse(data: data).first?.kind, .studio)
    }

    // MARK: - Rejection rules

    /// The main Control Room bus is internal summing and carries no OutputBus at
    /// all, so it drops out naturally rather than appearing as a wired output.
    func testBusWithoutAnOutputBusIsSkipped() throws {
        let data = document(bus(name: "Control Room", type: 14, channels: nil))
        XCTAssertTrue(try ControlRoomParser.parse(data: data).isEmpty)
    }

    func testBusWithNoConnectionsIsSkipped() throws {
        let data = document(bus(name: "Unwired Speaker", type: 16, channels: []))
        XCTAssertTrue(try ControlRoomParser.parse(data: data).isEmpty)
    }

    func testUnnamedBusIsSkipped() throws {
        let data = document(bus(name: "", type: 16, channels: [1, 2]))
        XCTAssertTrue(try ControlRoomParser.parse(data: data).isEmpty)
    }

    // MARK: - Document level

    func testMissingPresetMemberYieldsNoBuses() throws {
        let data = Data("<?xml version=\"1.0\"?><ControlRoom/>".utf8)
        XCTAssertTrue(try ControlRoomParser.parse(data: data).isEmpty)
    }

    func testMalformedXMLThrows() {
        XCTAssertThrowsError(try ControlRoomParser.parse(data: Data("<ControlRoom>".utf8)))
    }

    // MARK: - Aggregation

    func testChannelMapIndexesEveryBusByChannel() throws {
        let data = document(
            bus(name: "Speakers", type: 16, channels: [193, 194])
            + bus(name: "Phones Amp", type: 12, channels: [209, 210])
        )
        let map = try ControlRoomParser.parse(data: data).channelMap

        XCTAssertEqual(map[193]?.first?.name, "Speakers")
        XCTAssertEqual(map[210]?.first?.name, "Phones Amp")
        XCTAssertEqual(map.keys.count, 4)
        XCTAssertNil(map[1])
    }

    func testBusesGetDistinctIdentifiers() throws {
        let data = document(
            bus(name: "One", type: 16, channels: [1])
            + bus(name: "Two", type: 16, channels: [2])
        )
        let buses = try ControlRoomParser.parse(data: data)

        XCTAssertEqual(Set(buses.map(\.id)).count, 2)
    }
}
