#include "CardsView.h"
#include "Fonts.h"

namespace StudioPatchbay
{

CardsView::CardsView()
{
    viewport.setViewedComponent (&cardContainer, false);
    viewport.setScrollBarsShown (true, false); // vertical only
    addAndMakeVisible (viewport);
}

int CardsView::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

void CardsView::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;
    cardContainer.pal = newPalette;
    cardContainer.repaint();
    for (auto* card : cards)
        card->setPalette (pal);
    repaint();
}

void CardsView::setUiScale (double scale)
{
    uiScale = scale;
    cardContainer.uiScale = scale;
    for (auto* card : cards)
        card->setUiScale (scale);
    resized();
}

void CardsView::setDevices (const ParsedDevices& devices)
{
    currentDevices = devices;
    rebuildCards();
}

// ============================================================================
// Card management
// ============================================================================

void CardsView::rebuildCards()
{
    cards.clear();

    auto addCards = [&] (const std::vector<ParsedDevice>& devices)
    {
        for (auto& dev : devices)
        {
            auto* card = cards.add (new DeviceCardView (dev));
            card->setPalette (pal);
            card->setUiScale (uiScale);
            if (onDeviceClicked)
            {
                card->onClick = [this] (const ParsedDevice& d)
                {
                    if (onDeviceClicked)
                        onDeviceClicked (d);
                };
            }
            cardContainer.addAndMakeVisible (card);
        }
    };

    addCards (currentDevices.instruments);
    addCards (currentDevices.fxDevices);

    // Without cards the view itself shows the empty state.
    viewport.setVisible (! cards.isEmpty());

    resized();
}

// ============================================================================
// Layout
// ============================================================================

int CardsView::layoutCards()
{
    cardContainer.headers.clear();
    if (cards.isEmpty())
        return 0;

    const auto viewW = viewport.getMaximumVisibleWidth();
    const auto cardW = sc (220);
    const auto cardH = sc (140);
    const auto gap = sc (8);
    const auto padding = sc (10);
    const auto headerH = sc (24);

    const int cols = juce::jmax (1, (viewW - 2 * padding + gap) / (cardW + gap));

    int y = padding;
    int cardIdx = 0;

    // Helper to lay out a section.
    cardContainer.headers.clear();

    auto layoutSection = [&] (const juce::String& sectionTitle, size_t count)
    {
        if (count == 0)
            return;

        cardContainer.headers.push_back ({ sectionTitle + "  (" + juce::String (count) + ")",
                                           { padding, y, viewW - 2 * padding, headerH - sc (4) } });
        y += headerH;

        int col = 0;
        for (size_t i = 0; i < count; ++i)
        {
            if (cardIdx >= cards.size())
                break;

            int x = padding + col * (cardW + gap);
            cards[cardIdx]->setBounds (x, y, cardW, cardH);
            ++cardIdx;
            ++col;

            if (col >= cols)
            {
                col = 0;
                y += cardH + gap;
            }
        }

        // Close the last row.
        if (col > 0)
            y += cardH + gap;
    };

    layoutSection ("INSTRUMENTS", currentDevices.instruments.size());
    layoutSection ("EXTERNAL FX", currentDevices.fxDevices.size());

    return y + padding;
}

void CardsView::resized()
{
    viewport.setBounds (getLocalBounds());

    int totalH = layoutCards();
    cardContainer.setSize (viewport.getMaximumVisibleWidth(), juce::jmax (totalH, getHeight()));
}

// ============================================================================
// Paint
// ============================================================================

void CardsView::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);

    if (cards.isEmpty())
    {
        g.setFont (Fonts::sans (static_cast<float> (sc (13)), Fonts::Weight::regular));
        g.setColour (pal.ink.withAlpha (0.4f));
        g.drawText (juce::String (juce::CharPointer_UTF8 ("No devices found \xe2\x80\x93 select a Cubase installation with External Plugins configured.")),
                    getLocalBounds().reduced (sc (20)),
                    juce::Justification::centred);
    }
}

void CardsView::CardContainer::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);

    // Section headers sit between the cards, so they belong to the scrolled
    // content; the layout already placed them.
    const auto edge = juce::jmax (1, juce::roundToInt (uiScale));
    g.setFont (Fonts::sans (static_cast<float> (11.0 * uiScale), Fonts::Weight::semiBold));
    for (const auto& header : headers)
    {
        if (! g.clipRegionIntersects (header.bounds))
            continue;
        g.setColour (pal.ink.withAlpha (0.6f));
        g.drawText (header.title, header.bounds, juce::Justification::centredLeft, true);
        g.setColour (pal.shadowEdge);
        g.fillRect (header.bounds.getX(), header.bounds.getBottom() - edge, header.bounds.getWidth(), edge);
    }
}

} // namespace StudioPatchbay
