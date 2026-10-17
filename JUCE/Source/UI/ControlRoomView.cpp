#include "ControlRoomView.h"
#include "Fonts.h"

namespace StudioPatchbay
{

ControlRoomView::ControlRoomView() = default;

int ControlRoomView::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

void ControlRoomView::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;
    repaint();
}

void ControlRoomView::setUiScale (double scale)
{
    uiScale = scale;
    repaint();
}

void ControlRoomView::setBuses (const std::vector<ControlRoomBus>& buses)
{
    currentBuses = buses;
    repaint();
}

juce::String ControlRoomView::kindLabel (ControlRoomBus::Kind kind)
{
    switch (kind)
    {
        case ControlRoomBus::Kind::studio:  return "Studio";
        case ControlRoomBus::Kind::phones:  return "Phones";
        case ControlRoomBus::Kind::speaker: return "Speaker";
    }
    return {};
}

// ============================================================================
// Paint
// ============================================================================

void ControlRoomView::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);

    if (currentBuses.empty())
    {
        g.setFont (Fonts::sans (static_cast<float> (sc (13)), Fonts::Weight::regular));
        g.setColour (pal.ink.withAlpha (0.4f));
        g.drawText ("No Control Room configuration found.",
                    getLocalBounds().reduced (sc (20)),
                    juce::Justification::centred);
        return;
    }

    const auto edge = juce::jmax (1, sc (1));
    const auto padding = sc (12);
    const auto busH = sc (70);
    const auto gap = sc (8);

    int y = padding;

    // Section header.
    g.setFont (Fonts::sans (static_cast<float> (sc (11)), Fonts::Weight::semiBold));
    g.setColour (pal.ink.withAlpha (0.6f));
    g.drawText ("CONTROL ROOM BUSES",
                juce::Rectangle<int> (padding, y, getWidth() - 2 * padding, sc (20)),
                juce::Justification::centredLeft);
    y += sc (24);

    // Separator.
    g.setColour (pal.shadowEdge);
    g.fillRect (padding, y, getWidth() - 2 * padding, edge);
    y += sc (8);

    for (auto& bus : currentBuses)
    {
        auto busRect = juce::Rectangle<int> (padding, y, getWidth() - 2 * padding, busH);

        // Inset panel background.
        g.setColour (pal.controlSurface);
        g.fillRect (busRect);

        // OS9 bevel (inset).
        g.setColour (pal.shadowEdge);
        g.fillRect (busRect.getX(), busRect.getY(), busRect.getWidth(), edge);
        g.fillRect (busRect.getX(), busRect.getY(), edge, busRect.getHeight());
        g.setColour (pal.highlightEdge);
        g.fillRect (busRect.getX(), busRect.getBottom() - edge, busRect.getWidth(), edge);
        g.fillRect (busRect.getRight() - edge, busRect.getY(), edge, busRect.getHeight());

        auto content = busRect.reduced (sc (10), sc (8));

        // Bus kind badge.
        auto kindStr = kindLabel (bus.kind);
        g.setFont (Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::semiBold));
        g.setColour (pal.ink.withAlpha (0.5f));
        g.drawText (kindStr.toUpperCase(),
                    content.removeFromTop (sc (14)),
                    juce::Justification::centredLeft);

        content.removeFromTop (sc (2));

        // Bus name.
        g.setFont (Fonts::sans (static_cast<float> (sc (13)), Fonts::Weight::semiBold));
        g.setColour (pal.ink);
        g.drawText (bus.name,
                    content.removeFromTop (sc (18)),
                    juce::Justification::centredLeft);

        content.removeFromTop (sc (4));

        // Channel assignment.
        if (! bus.channels.empty())
        {
            juce::String chStr = "Ch: ";
            for (size_t i = 0; i < bus.channels.size(); ++i)
            {
                if (i > 0) chStr += ", ";
                chStr += juce::String (bus.channels[i]);
            }

            g.setFont (Fonts::sans (static_cast<float> (sc (11)), Fonts::Weight::regular));
            g.setColour (pal.ink.withAlpha (0.6f));
            g.drawText (chStr, content, juce::Justification::centredLeft);
        }

        y += busH + gap;
    }
}

void ControlRoomView::resized()
{
    // All painting is done directly – no child components to lay out.
}

} // namespace StudioPatchbay
