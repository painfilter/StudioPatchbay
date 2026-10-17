#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace StudioPatchbay::Tokens
{

// Product identity
inline constexpr int baseWidth = 960;
inline constexpr int baseHeight = 700;

// Connector type accent tints (PRODUCT_EXTENSION)
// These are subtle tints applied to controlSurface for channel cells,
// NOT the bright colors from the original Swift app.
// Values derived from the original channel colors but heavily desaturated
// to fit the OS9 Platinum aesthetic.
namespace ConnectorTint
{
    // Applied as a subtle overlay on the base controlSurface.
    // The ~8% alpha lets the underlying Platinum panel show through.
    inline juce::Colour madi()        { return juce::Colour (0x15597AFF); } // sky blue, ~8% alpha
    inline juce::Colour adat()        { return juce::Colour (0x1566D98C); } // mint green, ~8%
    inline juce::Colour aes()         { return juce::Colour (0x15FFB84D); } // amber, ~8%
    inline juce::Colour spdif()       { return juce::Colour (0x15FF8CD9); } // pink, ~8%
    inline juce::Colour dante()       { return juce::Colour (0x158CD9FF); } // cyan, ~8%
    inline juce::Colour usb()         { return juce::Colour (0x10B3B3BF); } // neutral gray, ~6%
    inline juce::Colour thunderbolt() { return juce::Colour (0x15808CFF); } // periwinkle, ~8%
    inline juce::Colour analogLine()  { return juce::Colour (0x15F29940); } // orange, ~8%
    inline juce::Colour analogMic()   { return juce::Colour (0x15D95959); } // coral, ~8%
}

// Device type accent tints for cards
namespace DeviceTint
{
    inline juce::Colour instrument() { return juce::Colour (0x123366CC); } // blue, ~7%
    inline juce::Colour fx()         { return juce::Colour (0x12CC8833); } // orange, ~7%
    inline juce::Colour sendOnly()   { return juce::Colour (0x129966CC); } // purple, ~7%
}

} // namespace StudioPatchbay::Tokens
