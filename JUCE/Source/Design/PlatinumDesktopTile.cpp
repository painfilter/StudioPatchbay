// Generated file. Do not edit by hand: changes are overwritten when the
// file is regenerated.

#include "PlatinumDesktopTile.h"

namespace StudioPatchbay::Design
{

DesktopTile::DesktopTile (juce::String titleToShow, juce::String detailToShow,
                                IconPainter iconPainter, std::function<void()> onClick)
    : title (std::move (titleToShow)), detail (std::move (detailToShow)),
      paintIcon (std::move (iconPainter)), click (std::move (onClick))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (true);
}

void DesktopTile::setDarkMode (bool value) { darkMode = value; repaint(); }

void DesktopTile::setUiScale (double value) { uiScale = value > 0.0 ? value : 1.0; repaint(); }

void DesktopTile::setSelected (bool value) { selected = value; repaint(); }

void DesktopTile::setText (juce::String newTitle, juce::String newDetail)
{
    title = std::move (newTitle);
    detail = std::move (newDetail);
    repaint();
}

int DesktopTile::scaled (int value) const { return juce::roundToInt (uiScale * static_cast<double> (value)); }

juce::Rectangle<int> DesktopTile::iconBounds() const
{
    const auto width = scaled (Metrics::desktopIconWidth);
    return { (getWidth() - width) / 2, scaled (5), width, scaled (Metrics::desktopIconHeight) };
}

juce::Rectangle<int> DesktopTile::getIconBoundsInParent() const
{
    return iconBounds().translated (getX(), getY());
}

void DesktopTile::paint (juce::Graphics& g)
{
    const auto palette = darkMode ? darkPalette() : lightPalette();
    const auto frame = getLocalBounds().reduced (scaled (2)).toFloat();
    const auto corner = static_cast<float> (scaled (Metrics::desktopTileCornerRadius));
    const auto line = static_cast<float> (juce::jmax (1, scaled (1)));

    if (selected)
    {
        g.setColour (palette.selection.withAlpha (Interaction::desktopTileSelectedFillAlpha));
        g.fillRoundedRectangle (frame, corner);
        g.setColour ((darkMode ? palette.highlightEdge : palette.shadowEdge)
                         .withAlpha (Interaction::desktopTileSelectedOutlineAlpha));
        g.drawRoundedRectangle (frame, corner, line);
    }

    if (hovered)
    {
        const auto fill = darkMode ? palette.controlSurface : palette.windowPanel;
        const auto alpha = darkMode ? Interaction::desktopTileHoverFillAlphaDark
                                    : Interaction::desktopTileHoverFillAlphaLight;
        g.setColour (fill.withAlpha (alpha));
        g.fillRoundedRectangle (frame, corner);
        g.setColour (darkMode ? palette.highlightEdge : palette.shadowEdge);
        g.drawRoundedRectangle (frame, corner, line);
    }

    if (hasKeyboardFocus (false))
    {
        g.setColour (palette.selection);
        g.drawRoundedRectangle (getLocalBounds().reduced (scaled (1)).toFloat(), corner, line);
    }

    if (paintIcon)
        paintIcon (g, iconBounds(), darkMode);

    g.setColour (palette.ink);
    g.setFont (chromeFont (10.0 * uiScale, juce::Font::bold));
    g.drawFittedText (title, getLocalBounds().withTop (scaled (36)).withHeight (scaled (14)),
                      juce::Justification::centred, 1);
    g.setFont (chromeFont (8.5 * uiScale));
    g.drawFittedText (detail, getLocalBounds().withTop (scaled (51)).withHeight (scaled (17)),
                      juce::Justification::centred, 2);
}

void DesktopTile::mouseUp (const juce::MouseEvent& event)
{
    if (! event.mouseWasClicked())
        return;

    if (event.mods.isPopupMenu())
    {
        if (onPopupMenu)
            onPopupMenu();
        return;
    }
    if (click)
        click();
}

void DesktopTile::mouseEnter (const juce::MouseEvent&) { hovered = true; repaint(); }

void DesktopTile::mouseExit (const juce::MouseEvent&) { hovered = false; repaint(); }

bool DesktopTile::keyPressed (const juce::KeyPress& key)
{
    if ((key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey) && click)
    {
        click();
        return true;
    }
    return false;
}

} // namespace StudioPatchbay::Design
