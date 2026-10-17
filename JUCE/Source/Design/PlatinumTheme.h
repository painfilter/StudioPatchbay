// Generated file. Do not edit by hand: changes are overwritten when the
// file is regenerated.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace StudioPatchbay::Design
{

struct Palette
{
    juce::Colour desktop;
    juce::Colour windowPanel;
    juce::Colour controlSurface;
    juce::Colour field;
    juce::Colour ink;
    juce::Colour highlightEdge;
    juce::Colour shadowEdge;
    juce::Colour selection;
    juce::Colour scrollbarTrack;
    juce::Colour scrollbarThumb;
    juce::Colour graphGrid;
    juce::Colour inkSoft;
};

inline Palette lightPalette()
{
    return { juce::Colour (0xff657a78),
             juce::Colour (0xffd8d8d2),
             juce::Colour (0xffdddddd),
             juce::Colour (0xffffffff),
             juce::Colour (0xff000000),
             juce::Colour (0xffffffff),
             juce::Colour (0xff777777),
             juce::Colour (0xff332b9a),
             juce::Colour (0xffaaaaaa),
             juce::Colour (0xff9f9bf7),
             juce::Colour (0xffbbbbbb),
             juce::Colour (0xff6b6f76) };
}

inline Palette darkPalette()
{
    return { juce::Colour (0xff171b1c),
             juce::Colour (0xff303530),
             juce::Colour (0xff343733),
             juce::Colour (0xff171a18),
             juce::Colour (0xfff0ead8),
             juce::Colour (0xff6b7069),
             juce::Colour (0xff111311),
             juce::Colour (0xff5148a8),
             juce::Colour (0xff242824),
             juce::Colour (0xff7b75b5),
             juce::Colour (0xff4d534f),
             juce::Colour (0xff9a9ea4) };
}

namespace Metrics
{
inline constexpr int menuBarHeight = 27;
inline constexpr int menuClockWidth = 84;
}

namespace Interaction
{
inline constexpr double menuClockDateDisplayMs = 3600.0;
}

namespace Scale
{
inline constexpr std::array<double, 8> windowZoomLevels { 0.75, 0.9, 1.0, 1.1, 1.25, 1.5, 1.75, 2.0 };
}

} // namespace StudioPatchbay::Design
