import SwiftUI

// The house style shared with HardwareRecall.
//
// The plug-in renders hardware, so its own language is a faceplate one: light
// neutral panels, dark ink, legends printed directly onto the surface. Only
// part of that can travel to an app whose content is routing data rather than
// physical objects.
//
// What transfers is the lettering idiom and the restraint behind it. Labels are
// printed small, uppercase and letterspaced in the regular sans, so they read as
// annotation on a technical drawing rather than as headlines competing with the
// data. Structure comes from alignment and hairlines instead of fills.
//
// What deliberately does not transfer: faceplate imagery, knob metaphors and
// custom window chrome. This is a native Mac app and should look like one, which
// is also the honest answer rather than dressing a data view in costume.

enum PF {

    /// Painfilter crimson, the same value the website uses for `--accent`.
    ///
    /// Reserved for brand presence. Deliberately NOT used for selection, focus
    /// or state: in audio software red already means record or clipping, and
    /// spending it on decoration would make a real warning ambiguous. Device
    /// type colours (blue, orange, purple) stay as they are, because there the
    /// colour carries information rather than identity.
    static let brand = Color(red: 0.694, green: 0.0, blue: 0.184)  // #b1002f
}

extension View {

    /// Printed-legend treatment for a section label: small, uppercase and
    /// letterspaced, the way the plug-in prints panel lettering.
    ///
    /// Pass a colour only where it carries meaning, such as a device-type
    /// section, otherwise the label stays secondary so the data reads first.
    func panelLabel(_ color: Color? = nil) -> some View {
        self
            .font(.caption.weight(.semibold))
            .textCase(.uppercase)
            .tracking(0.6)
            .foregroundStyle(color ?? .secondary)
    }
}
