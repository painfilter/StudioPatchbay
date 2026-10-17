#include "ControlRoomView.h"
#include "Localisation/Text.h"

namespace StudioPatchbay
{

namespace
{
using Text::tr;
const juce::String dot (juce::CharPointer_UTF8 ("  \xc2\xb7  "));
const juce::String arrow (juce::CharPointer_UTF8 ("  \xe2\x86\x92  "));

juce::String kindName (ControlRoomBus::Kind k)
{
    switch (k)
    {
        case ControlRoomBus::Kind::phones:  return tr ("controlRoom.kindPhones");
        case ControlRoomBus::Kind::speaker: return tr ("controlRoom.kindSpeaker");
        case ControlRoomBus::Kind::studio:  break;
    }
    return tr ("controlRoom.kindStudio");
}
} // namespace

ControlRoomView::ControlRoomView (const ViewData& d) : ContentView (d)
{
    viewport.setViewedComponent (&page, false);
}

juce::StringArray ControlRoomView::routes (const ControlRoomBus& bus) const
{
    juce::StringArray result;
    for (auto ch : bus.channels)
    {
        const auto head = "ch " + juce::String (ch);
        if (auto* dev = PhysicalDevices::find (data.physical, ch, ChannelDirection::send))
            result.add (head + arrow + dev->name + dot + "I/O " + juce::String (*dev->physicalIO (ch, ChannelDirection::send)));
        else if (auto* port = AudioPorts::find (data.ports, ch))
            result.add (head + arrow + port->interfaceName + dot + tr ("controlRoom.direct"));
        else
            result.add (head);
    }
    return result;
}

int ControlRoomView::busCard (juce::Graphics* g, const ControlRoomBus& bus, juce::Rectangle<int> area) const
{
    const auto& s = style;
    const auto teal = s.controlRoom();
    auto inner = area.reduced (s.px (12));
    int y = inner.getY();

    if (g != nullptr)
    {
        g->setColour (s.pal.field);
        g->fillRect (area);
        g->setColour (s.pal.shadowEdge.withAlpha (0.6f));
        g->drawRect (area, 1);
        g->setColour (teal);
        g->fillRect (area.getX() + 1, area.getY() + s.px (4), juce::jmax (2, s.px (3)), area.getHeight() - s.px (8));
        g->setColour (s.pal.ink);
        g->setFont (s.font (14.0f, Fonts::Weight::semiBold));
        g->drawText (bus.name, inner.getX(), y, inner.getWidth(), s.px (18), juce::Justification::centredLeft, true);
        g->setColour (s.onSurface (teal));
        g->setFont (s.font (11.0f));
        g->drawText (kindName (bus.kind), inner.getX(), y + s.px (18), inner.getWidth(), s.px (15), juce::Justification::centredLeft);
    }
    y += s.px (33) + s.px (8);
    if (g != nullptr)
    {
        g->setColour (s.pal.shadowEdge.withAlpha (0.5f));
        g->fillRect (inner.getX(), y, inner.getWidth(), 1);
    }
    y += s.px (8);

    const auto node = [&] (const juce::String& icon, const juce::String& text, juce::Colour colour)
    {
        const auto r = juce::Rectangle<int> (inner.getX(), y, inner.getWidth(), s.px (26));
        if (g != nullptr)
        {
            g->setColour (colour.withAlpha (0.08f));
            g->fillRect (r);
            g->setColour (colour.withAlpha (0.25f));
            g->drawRect (r, 1);
            g->setColour (s.pal.ink);
            g->setFont (s.font (11.0f));
            g->drawText (icon, r.withWidth (s.px (24)), juce::Justification::centred);
            g->drawText (text, r.withTrimmedLeft (s.px (26)).withTrimmedRight (s.px (4)), juce::Justification::centredLeft, true);
        }
        y += r.getHeight();
    };
    const auto flowArrow = [&]
    {
        if (g != nullptr)
        {
            g->setColour (s.pal.inkSoft);
            g->setFont (s.font (10.0f));
            g->drawText (juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x93")), inner.getX() + s.px (6), y, s.px (12), s.px (13),
                         juce::Justification::centred);
        }
        y += s.px (13);
    };

    node (juce::String (juce::CharPointer_UTF8 ("\xe2\x97\x8e")), tr ("controlRoom.cubase"), s.pal.ink);
    for (const auto& route : routes (bus))
    {
        flowArrow();
        node (juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x91")), route, teal);
    }
    return y + s.px (12) - area.getY();
}

int ControlRoomView::layout (juce::Graphics* g)
{
    const auto& s = style;
    const auto width = page.getWidth();
    const auto margin = s.px (16), gap = s.px (10);
    if (g != nullptr)
        g->fillAll (s.pal.windowPanel);

    // Header: "Control Room", count, hairline.
    int y = margin;
    if (g != nullptr)
    {
        const auto title = tr ("patchbay.legendControlRoom");
        const auto f = s.font (12.5f, Fonts::Weight::semiBold);
        g->setColour (s.onSurface (s.controlRoom()));
        g->setFont (f);
        g->drawText (title, margin, y, width, s.px (18), juce::Justification::centredLeft);
        auto x = margin + juce::roundToInt (juce::GlyphArrangement::getStringWidth (f, title)) + s.px (8);
        const auto count = juce::String (static_cast<int> (data.buses.size()));
        const auto chip = juce::Rectangle<int> (x, y + s.px (2), s.px (24), s.px (14));
        g->setColour (s.pal.controlSurface);
        g->fillRect (chip);
        g->setColour (s.pal.inkSoft);
        g->setFont (s.mono (10.0f));
        g->drawText (count, chip, juce::Justification::centred);
        g->setColour (s.pal.shadowEdge.withAlpha (0.5f));
        g->fillRect (chip.getRight() + s.px (8), y + s.px (9), width - margin - chip.getRight() - s.px (8), 1);
    }
    y += s.px (34);

    if (data.buses.empty())
    {
        if (g != nullptr)
        {
            auto area = juce::Rectangle<int> (margin, y + s.px (40), width - 2 * margin, s.px (60));
            g->setColour (s.pal.ink);
            g->setFont (s.font (14.0f, Fonts::Weight::semiBold));
            g->drawText (tr ("controlRoom.emptyTitle"), area.removeFromTop (s.px (24)), juce::Justification::centred);
            g->setColour (s.pal.inkSoft);
            g->setFont (s.font (12.0f));
            g->drawFittedText (tr ("controlRoom.emptyHelp"), area, juce::Justification::centredTop, 2);
        }
        return y + s.px (120);
    }

    const auto inner = width - 2 * margin;
    const auto cardMin = s.px (280), cardMax = s.px (420);
    const auto columns = juce::jmax (1, (inner + gap) / (cardMin + gap));
    const auto cardWidth = juce::jmin (cardMax, (inner - gap * (columns - 1)) / columns);
    int rowHeight = 0;
    for (size_t i = 0; i < data.buses.size(); ++i)
    {
        const auto column = static_cast<int> (i) % columns;
        if (column == 0 && i > 0)
        {
            y += rowHeight + gap;
            rowHeight = 0;
        }
        const auto x = margin + column * (cardWidth + gap);
        const auto height = busCard (nullptr, data.buses[i], { x, y, cardWidth, 1000 });
        if (g != nullptr)
            busCard (g, data.buses[i], { x, y, cardWidth, height });
        rowHeight = juce::jmax (rowHeight, height);
    }
    return y + rowHeight + margin;
}

void ControlRoomView::refresh()
{
    page.setSize (pageWidth(), 10);
    const auto height = layout (nullptr);
    page.setSize (pageWidth(), juce::jmax (height, viewport.getMaximumVisibleHeight()));
    page.repaint();
}

} // namespace StudioPatchbay
