#pragma once

#include <juce_graphics/juce_graphics.h>

#include <vector>

namespace StudioPatchbay
{

// Where the application window and the desktop icons go, for a given size.
//
// Icons stand in columns at the right edge of the desktop, like volumes on a
// classic Mac desktop: filled top to bottom, the rightmost column first. The
// trash always takes the bottom slot of the rightmost column. The window gets
// whatever is left to the left of the columns.
struct DesktopLayout
{
    static constexpr int maxColumns = 3;

    juce::Rectangle<int> window;                 // the content frame
    juce::Rectangle<int> shelf;                  // all icon columns together
    std::vector<juce::Rectangle<int>> items;     // in order; may hold fewer than asked
    juce::Rectangle<int> trash;

    // `desktop` is everything below the menu bar.
    static DesktopLayout compute (juce::Rectangle<int> desktop, int itemCount, double uiScale);
};

} // namespace StudioPatchbay
