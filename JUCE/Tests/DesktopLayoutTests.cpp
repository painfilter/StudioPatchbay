#include <catch2/catch_test_macros.hpp>
#include "Layout/DesktopLayout.h"

using namespace StudioPatchbay;

namespace
{
bool overlapsAny (const juce::Rectangle<int>& r, const std::vector<juce::Rectangle<int>>& others)
{
    for (auto& o : others)
        if (&o != &r && o.intersects (r))
            return true;
    return false;
}
} // namespace

TEST_CASE ("Desktop icons stand right of the window, trash at the bottom", "[layout]")
{
    const juce::Rectangle<int> desktop (0, 27, 960, 673);
    const auto layout = DesktopLayout::compute (desktop, 5, 1.0);

    REQUIRE (layout.items.size() == 5);
    CHECK (layout.shelf.getRight() == desktop.getRight());
    CHECK (layout.shelf.getWidth() == 112);              // one column
    CHECK (layout.window.getRight() <= layout.shelf.getX());
    CHECK (desktop.contains (layout.window));

    for (auto& item : layout.items)
    {
        CHECK (layout.shelf.contains (item));
        CHECK_FALSE (item.intersects (layout.trash));
        CHECK_FALSE (overlapsAny (item, layout.items));
    }
    CHECK (layout.shelf.contains (layout.trash));
    CHECK (layout.trash.getBottom() > layout.items.back().getBottom());
    CHECK (layout.items[0].getY() < layout.items[1].getY());   // top to bottom
}

TEST_CASE ("More icons open more columns, up to a limit", "[layout]")
{
    const juce::Rectangle<int> desktop (0, 27, 1200, 400);   // 4 rows: 3 slots + trash
    CHECK (DesktopLayout::compute (desktop, 3, 1.0).shelf.getWidth() == 112);

    const auto two = DesktopLayout::compute (desktop, 4, 1.0);
    CHECK (two.shelf.getWidth() == 224);
    REQUIRE (two.items.size() == 4);
    CHECK (two.items[3].getX() < two.items[0].getX());          // second column is to the left

    const auto many = DesktopLayout::compute (desktop, 100, 1.0);
    CHECK (many.shelf.getWidth() == DesktopLayout::maxColumns * 112);
    CHECK (many.items.size() == 3 + 2 * 4);                       // what fits
}

TEST_CASE ("Desktop layout follows the zoom", "[layout]")
{
    const juce::Rectangle<int> desktop (0, 40, 1600, 1000);
    const auto layout = DesktopLayout::compute (desktop, 2, 1.5);
    CHECK (layout.shelf.getWidth() == 168);
    CHECK (layout.items[0].getWidth() == 144);
    CHECK (layout.items[0].getHeight() == 108);
}
