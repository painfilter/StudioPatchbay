#include "AppWindow.h"
#include "Fonts.h"

namespace StudioPatchbay
{

namespace
{
void frame (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour c, int thickness)
{
    g.setColour (c);
    for (int i = 0; i < thickness && ! r.isEmpty(); ++i, r = r.reduced (1))
    {
        g.fillRect (r.getX(), r.getY(), r.getWidth(), 1);
        g.fillRect (r.getX(), r.getBottom() - 1, r.getWidth(), 1);
        g.fillRect (r.getX(), r.getY(), 1, r.getHeight());
        g.fillRect (r.getRight() - 1, r.getY(), 1, r.getHeight());
    }
}
} // namespace

AppWindow::AppWindow (juce::Component& content) : body (content)
{
    collapseBox.getProperties().set ("windowShade", true);
    collapseBox.onClick = [this] { if (onCollapse) onCollapse(); };
    addAndMakeVisible (collapseBox);
    addAndMakeVisible (body);
}

void AppWindow::setPalette (const Design::Palette& p) { pal = p; repaint(); }

void AppWindow::setUiScale (double scale)
{
    uiScale = scale;
    resized();
    repaint();
}

void AppWindow::setTitle (const juce::String& text)
{
    if (title == text)
        return;
    title = text;
    repaint (titleRow());
}

void AppWindow::setCollapseTooltip (const juce::String& text) { collapseBox.setTooltip (text); }

juce::Rectangle<int> AppWindow::titleRow() const
{
    return getLocalBounds().removeFromTop (sc (titleBarHeight)).reduced (sc (2));
}

void AppWindow::paint (juce::Graphics& g)
{
    const auto edge = juce::jmax (1, sc (1));
    g.fillAll (pal.windowPanel);
    frame (g, getLocalBounds(), pal.ink, juce::jmax (1, sc (2)));

    const auto row = titleRow();
    const auto cutout = row.withSizeKeepingCentre (juce::jmin (sc (240), row.getWidth() - sc (120)), row.getHeight());
    const auto left = juce::Rectangle<int>::leftTopRightBottom (row.getX() + sc (23), row.getY(),
                                                                cutout.getX() - sc (4), row.getBottom());
    const auto right = juce::Rectangle<int>::leftTopRightBottom (cutout.getRight() + sc (4), row.getY(),
                                                                 row.getRight() - sc (43), row.getBottom());
    g.setColour (pal.ink);
    for (const auto stripes : { left, right })
        for (int y = stripes.getY() + sc (2); y < stripes.getBottom() - edge; y += juce::jmax (1, sc (3)))
            g.fillRect (stripes.getX(), y, stripes.getWidth(), edge);

    g.setFont (Fonts::sans (static_cast<float> (13.0 * uiScale), Fonts::Weight::semiBold));
    g.drawFittedText (title, cutout.reduced (sc (8), 0), juce::Justification::centred, 1);

    // Close box at the left, zoom box at the right: drawn for the look of the
    // window; the native window around the desktop does both jobs.
    const auto box = [&] (juce::Rectangle<int> r, bool zoom)
    {
        g.setColour (pal.field);
        g.fillRect (r);
        frame (g, r, pal.ink, edge);
        frame (g, zoom ? juce::Rectangle<int> (r.getX() + sc (3), r.getY() + sc (3), sc (8), sc (8))
                       : r.reduced (sc (3)), pal.ink, edge);
    };
    box ({ row.getX() + sc (3), row.getY() + sc (3), sc (16), sc (16) }, false);
    box ({ row.getRight() - sc (39), row.getY() + sc (3), sc (16), sc (16) }, true);
}

void AppWindow::resized()
{
    const auto row = titleRow();
    collapseBox.setBounds (row.getRight() - sc (20), row.getY() + sc (3), sc (16), sc (16));
    const auto stroke = juce::jmax (1, sc (2));
    body.setBounds (getLocalBounds().withTrimmedTop (sc (titleBarHeight)).reduced (stroke).withTrimmedTop (-stroke));
}

} // namespace StudioPatchbay
