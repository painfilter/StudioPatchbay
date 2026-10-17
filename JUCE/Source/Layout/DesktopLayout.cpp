#include "DesktopLayout.h"
#include "Design/PlatinumTheme.h"

namespace StudioPatchbay
{

DesktopLayout DesktopLayout::compute (juce::Rectangle<int> desktop, int itemCount, double uiScale)
{
    using namespace Design::Metrics;
    const auto sc = [uiScale] (int v) { return juce::roundToInt (v * uiScale); };

    const auto stepX = sc (desktopTileHorizontalStep);
    const auto stepY = sc (desktopTileVerticalStep);
    const auto tileW = sc (desktopTileWidth);
    const auto tileH = sc (desktopTileHeight);
    const auto margin = sc (4);

    // Slots per column; at least two, so the trash never takes the only one.
    const auto rows = juce::jmax (2, (desktop.getHeight() - 2 * margin) / juce::jmax (1, stepY));
    const auto slotsFirstColumn = rows - 1;   // the trash takes the last

    auto columns = 1;
    if (itemCount > slotsFirstColumn)
        columns += (itemCount - slotsFirstColumn + rows - 1) / rows;
    columns = juce::jmin (columns, maxColumns);

    DesktopLayout layout;
    auto area = desktop;
    layout.shelf = area.removeFromRight (columns * stepX).reduced (0, margin);
    layout.window = area.withTrimmedLeft (sc (6)).reduced (0, margin).withTrimmedRight (sc (2));

    const auto slot = [&] (int column, int row)
    {
        const auto x = layout.shelf.getRight() - (column + 1) * stepX + (stepX - tileW) / 2;
        const auto y = layout.shelf.getY() + row * stepY + (stepY - tileH) / 2;
        return juce::Rectangle<int> (x, y, tileW, tileH);
    };

    // The trash sits at the very bottom, wherever the last row ends.
    layout.trash = slot (0, 0).withY (layout.shelf.getBottom() - stepY + (stepY - tileH) / 2);

    const auto capacity = slotsFirstColumn + (columns - 1) * rows;
    for (int i = 0; i < juce::jmin (itemCount, capacity); ++i)
    {
        const auto column = i < slotsFirstColumn ? 0 : 1 + (i - slotsFirstColumn) / rows;
        const auto row = i < slotsFirstColumn ? i : (i - slotsFirstColumn) % rows;
        layout.items.push_back (slot (column, row));
    }

    return layout;
}

} // namespace StudioPatchbay
