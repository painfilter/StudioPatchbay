#include "ViewSwitcher.h"
#include "DesktopShelf.h"
#include "Fonts.h"

namespace StudioPatchbay
{

ViewSwitcher::ViewSwitcher()
{
    tabs = {
        { "Cards",        View::cards },
        { "Patchbay",     View::patchbay },
        { "Control Room", View::controlRoom },
        { "Signal Flow",  View::signalFlow }
    };
}

int ViewSwitcher::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

void ViewSwitcher::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;
    repaint();
}

void ViewSwitcher::setUiScale (double scale)
{
    uiScale = scale;
    repaint();
}

void ViewSwitcher::setActiveView (View view)
{
    if (activeView != view)
    {
        activeView = view;
        repaint();
        if (onViewChanged)
            onViewChanged (view);
    }
}

// ============================================================================
// Tab geometry
// ============================================================================

juce::Rectangle<int> ViewSwitcher::tabBounds (int index) const
{
    const auto tabW = sc (110);
    const auto tabH = getHeight();
    const auto slant = sc (8);  // trapezoid slant width
    const auto gap = sc (2);
    const auto x = sc (4) + index * (tabW + gap);
    juce::ignoreUnused (slant);
    return { x, 0, tabW, tabH };
}

int ViewSwitcher::tabIndexAtX (int x) const
{
    for (int i = 0; i < static_cast<int> (tabs.size()); ++i)
    {
        if (tabBounds (i).contains (x, getHeight() / 2))
            return i;
    }
    return -1;
}

// ============================================================================
// Paint
// ============================================================================

void ViewSwitcher::paint (juce::Graphics& g)
{
    const auto edge = juce::jmax (1, sc (1));
    const auto slant = sc (8);
    const auto font = Fonts::sans (static_cast<float> (sc (12)), Fonts::Weight::medium);

    // Bottom line spanning the full width.
    g.setColour (pal.shadowEdge);
    g.fillRect (0, getHeight() - edge, getWidth(), edge);

    for (int i = 0; i < static_cast<int> (tabs.size()); ++i)
    {
        auto bounds = tabBounds (i);
        bool isActive = (tabs[static_cast<size_t> (i)].view == activeView);

        // Build the trapezoid path.
        juce::Path tabPath;
        const float l = static_cast<float> (bounds.getX());
        const float r = static_cast<float> (bounds.getRight());
        const float t = static_cast<float> (isActive ? bounds.getY() : bounds.getY() + sc (3));
        const float b = static_cast<float> (bounds.getBottom());
        const float s = static_cast<float> (slant);

        tabPath.startNewSubPath (l, b);
        tabPath.lineTo (l + s, t);
        tabPath.lineTo (r - s, t);
        tabPath.lineTo (r, b);
        tabPath.closeSubPath();

        // Fill.
        g.setColour (isActive ? pal.windowPanel : pal.controlSurface);
        g.fillPath (tabPath);

        // Stroke.
        g.setColour (isActive ? pal.ink : pal.shadowEdge);
        g.strokePath (tabPath, juce::PathStrokeType (static_cast<float> (edge)));

        // If active, erase the bottom line under this tab.
        if (isActive)
        {
            g.setColour (pal.windowPanel);
            g.fillRect (bounds.getX() + 1, getHeight() - edge, bounds.getWidth() - 2, edge);
        }

        // Label.
        g.setFont (font);
        g.setColour (isActive ? pal.ink : pal.ink.withAlpha (0.6f));
        g.drawText (tabs[static_cast<size_t> (i)].label,
                    bounds.reduced (slant, 0).withTrimmedTop (isActive ? 0 : sc (3)),
                    juce::Justification::centred);
    }
}

// ============================================================================
// Mouse
// ============================================================================

void ViewSwitcher::mouseDown (const juce::MouseEvent& e)
{
    int idx = tabIndexAtX (e.x);
    if (idx >= 0 && idx < static_cast<int> (tabs.size()))
        setActiveView (tabs[static_cast<size_t> (idx)].view);
}

void ViewSwitcher::mouseDrag (const juce::MouseEvent& e)
{
    // Pulling a tab away from the bar drags that view, e.g. onto an export
    // program on the desktop.
    if (e.getDistanceFromDragStart() < sc (6))
        return;
    const auto idx = tabIndexAtX (e.getMouseDownX());
    auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this);
    if (container == nullptr || container->isDragAndDropActive()
        || idx < 0 || idx >= static_cast<int> (tabs.size()))
        return;
    const auto bounds = tabBounds (idx);
    container->startDragging (DesktopShelf::describeViewDrag (tabs[static_cast<size_t> (idx)].view), this,
                              juce::ScaledImage (createComponentSnapshot (bounds)));
}

juce::String ViewSwitcher::viewName (View view)
{
    switch (view)
    {
        case View::cards:       return "Cards";
        case View::patchbay:    return "Patchbay";
        case View::controlRoom: return "Control Room";
        case View::signalFlow:  return "Signal Flow";
    }
    return "View";
}

} // namespace StudioPatchbay
