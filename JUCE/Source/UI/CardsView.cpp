#include "CardsView.h"
#include "DeviceStyle.h"
#include "Localisation/Text.h"
#include "Glyphs.h"

#include <set>

namespace StudioPatchbay
{

namespace
{
using Text::tr;
using Type = ParsedDevice::Type;

juce::String typeName (Type t)
{
    switch (t)
    {
        case Type::instrument: return tr ("cards.typeInstrument");
        case Type::sendOnly:   return tr ("cards.typeSendOnly");
        case Type::fx:         break;
    }
    return tr ("cards.typeFx");
}

juce::String formatLabel (const ParsedDevice& d)
{
    const auto count = d.channelCount();
    if (count > 2) return juce::String (count) + "ch";
    if (d.isStereo()) return tr ("cards.stereo");
    if (d.isMono()) return tr ("cards.mono");
    return {};
}

float textWidth (const juce::Font& f, const juce::String& s)
{
    return juce::GlyphArrangement::getStringWidth (f, s);
}
} // namespace

CardsView::CardsView (const ViewData& d) : ContentView (d)
{
    viewport.setViewedComponent (&page, false);
}

int CardsView::card (juce::Graphics* g, const ParsedDevice& d, juce::Rectangle<int> area, bool hover) const
{
    const auto& s = style;
    const auto accent = s.typeColour (d.type);
    const auto accentText = s.onSurface (accent);
    const auto selected = data.isSelected (d);
    const auto pad = s.px (12), gap = s.px (8);
    auto inner = area.reduced (pad);
    int y = inner.getY();

    // Background, border by state, the accent stripe on the left.
    if (g != nullptr)
    {
        g->setColour (s.pal.field);
        g->fillRect (area);
        g->setColour (selected ? accent : (hover ? accent.withAlpha (0.5f) : s.pal.shadowEdge.withAlpha (0.6f)));
        g->drawRect (area, selected ? juce::jmax (2, s.px (1.5f)) : 1);
        g->setColour (accent);
        g->fillRect (area.getX() + 1, area.getY() + s.px (8), juce::jmax (2, s.px (3)), area.getHeight() - s.px (16));
    }

    // Header: name (up to two lines) and type with port badges; format badge right.
    const auto format = formatLabel (d);
    const auto badgeFont = s.mono (9.0f);
    const auto formatWidth = format.isEmpty() ? 0 : juce::roundToInt (textWidth (badgeFont, format)) + s.px (10);
    const auto nameFont = s.font (14.0f, Fonts::Weight::semiBold);
    juce::GlyphArrangement nameLines;
    nameLines.addJustifiedText (nameFont, d.name, 0, 0, static_cast<float> (inner.getWidth() - formatWidth - s.px (6)),
                                juce::Justification::left);
    const auto lineCount = juce::jlimit (1, 2, nameLines.getNumGlyphs() > 0
                                                   ? juce::roundToInt ((nameLines.getBoundingBox (0, -1, true).getHeight()) / nameFont.getHeight())
                                                   : 1);
    const auto nameHeight = juce::roundToInt (nameFont.getHeight() * static_cast<float> (lineCount));
    if (g != nullptr)
    {
        g->setColour (s.pal.ink);
        g->setFont (nameFont);
        g->drawFittedText (d.name, inner.getX(), y, inner.getWidth() - formatWidth - s.px (6), nameHeight,
                           juce::Justification::topLeft, 2);
        if (formatWidth > 0)
        {
            const auto badge = juce::Rectangle<int> (inner.getRight() - formatWidth, y, formatWidth, s.px (16));
            g->setColour (accent.withAlpha (0.14f));
            g->fillRect (badge);
            g->setColour (accentText);
            g->setFont (badgeFont);
            g->drawText (format, badge, juce::Justification::centred);
        }
    }
    y += nameHeight + s.px (3);

    const auto typeFont = s.font (11.0f);
    const auto typeRow = juce::Rectangle<int> (inner.getX(), y, inner.getWidth(), s.px (16));
    if (g != nullptr)
    {
        g->setColour (accentText);
        g->setFont (typeFont);
        const auto label = typeName (d.type);
        g->drawText (label, typeRow, juce::Justification::centredLeft);
        auto x = typeRow.getX() + juce::roundToInt (textWidth (typeFont, label)) + s.px (6);

        // The ports this device's channels lie on.
        std::vector<const AudioPort*> ports;
        for (auto ch : d.allChannels())
            if (auto* p = AudioPorts::find (data.ports, ch); p != nullptr && std::find (ports.begin(), ports.end(), p) == ports.end())
                ports.push_back (p);
        std::sort (ports.begin(), ports.end(), [] (auto* a, auto* b) { return a->id < b->id; });
        g->setFont (badgeFont);
        for (auto* p : ports)
        {
            const auto colour = DeviceStyle::connectorColour (p->color, s.pal.inkSoft);
            const auto w = juce::roundToInt (textWidth (badgeFont, p->name)) + s.px (8);
            if (x + w > typeRow.getRight())
                break;
            const auto badge = juce::Rectangle<int> (x, typeRow.getY() + s.px (1), w, typeRow.getHeight() - s.px (2));
            g->setColour (colour.withAlpha (0.18f));
            g->fillRect (badge);
            g->setColour (s.onSurface (colour));
            g->drawText (p->name, badge, juce::Justification::centred);
            x += w + s.px (4);
        }
    }
    y += typeRow.getHeight() + gap;
    if (g != nullptr)
    {
        g->setColour (s.pal.shadowEdge.withAlpha (0.5f));
        g->fillRect (inner.getX(), y, inner.getWidth(), 1);
    }
    y += 1 + gap;

    // Channel rows: "↓ IN" for an instrument, "↑ SEND" and "↓ RECV" otherwise.
    const auto rowLabelWidth = s.px (38);
    const auto numberFont = s.mono (11.0f);
    const auto badgeHeight = s.px (17), spacing = s.px (3);
    const auto channelRow = [&] (const juce::String& label, const std::vector<int>& channels, juce::Colour colour)
    {
        auto x = inner.getX() + rowLabelWidth;
        int rowY = y;
        if (g != nullptr)
        {
            g->setColour (s.pal.inkSoft);
            g->setFont (s.mono (10.0f));
            g->drawText (label, inner.getX(), y, rowLabelWidth, badgeHeight, juce::Justification::centredLeft);
            g->setFont (numberFont);
        }
        for (auto ch : channels)
        {
            const auto text = juce::String (ch);
            const auto w = juce::roundToInt (textWidth (numberFont, text)) + s.px (10);
            if (x > inner.getX() + rowLabelWidth && x + w > inner.getRight())
            {
                x = inner.getX() + rowLabelWidth;
                rowY += badgeHeight + spacing;
            }
            if (g != nullptr)
            {
                const auto badge = juce::Rectangle<int> (x, rowY, w, badgeHeight);
                g->setColour (colour.withAlpha (0.14f));
                g->fillRect (badge);
                g->setColour (s.onSurface (colour));
                g->drawText (text, badge, juce::Justification::centred);
            }
            x += w + spacing;
        }
        y = rowY + badgeHeight + s.px (4);
    };
    const auto line = [&] (const juce::String& text, juce::Colour colour, const juce::Font& font, int height)
    {
        if (g != nullptr)
        {
            g->setColour (colour);
            g->setFont (font);
            g->drawText (text, inner.getX(), y, inner.getWidth(), height, juce::Justification::centredLeft, true);
        }
        y += height;
    };

    const auto warning = [&] (const juce::String& text)
    {
        if (g != nullptr)
        {
            g->setColour (s.warning());
            g->setFont (typeFont);
            Glyphs::line (*g, Glyphs::Kind::warning, s.warning(), s.warning(), text, { inner.getX(), y, inner.getWidth(), s.px (16) });
        }
        y += s.px (16);
    };
    if (d.allChannels().empty())
        warning (tr ("cards.noChannels"));
    else if (d.type == Type::instrument)
    {
        if (! d.recvChannels.empty())
            channelRow (juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x93 IN")), d.recvChannels, s.typeColour (Type::instrument));
    }
    else
    {
        if (! d.sendChannels.empty())
            channelRow (juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x91 SEND")), d.sendChannels, accent);
        if (! d.recvChannels.empty())
            channelRow (juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x93 RECV")), d.recvChannels, s.typeColour (Type::instrument));
    }

    // The physical devices on its channels, per direction.
    const auto namesOn = [&] (const std::vector<int>& channels, ChannelDirection dir)
    {
        std::set<juce::String> names;
        for (auto ch : channels)
            if (auto* p = PhysicalDevices::find (data.physical, ch, dir))
                names.insert (p->name);
        juce::StringArray list;
        for (auto& n : names) list.add (n);
        return list.joinIntoString (", ");
    };
    const auto outNames = namesOn (d.sendChannels, ChannelDirection::send);
    const auto inNames = namesOn (d.recvChannels, ChannelDirection::recv);
    if (outNames.isNotEmpty() || inNames.isNotEmpty())
    {
        y += s.px (4);
        if (outNames.isNotEmpty()) line ("OUT: " + outNames, s.pal.inkSoft, s.font (9.5f), s.px (13));
        if (inNames.isNotEmpty())  line ("IN: " + inNames, s.pal.inkSoft, s.font (9.5f), s.px (13));
    }

    if (d.midiDevice.isNotEmpty())
    {
        y += s.px (4);
        if (g != nullptr)
        {
            g->setColour (s.midi());
            g->fillEllipse (static_cast<float> (inner.getX()), static_cast<float> (y + s.px (5)),
                            static_cast<float> (s.px (5)), static_cast<float> (s.px (5)));
        }
        const auto saved = inner;
        inner = inner.withTrimmedLeft (s.px (9));
        line (d.midiDevice, s.pal.inkSoft, s.mono (11.0f), s.px (15));
        inner = saved;
    }
    if (d.hasMismatchedChannels())
    {
        y += s.px (4);
        warning (tr ("cards.mismatch"));
    }
    return y + pad - area.getY();
}

void CardsView::refresh()
{
    const auto width = pageWidth();
    const auto margin = style.px (16), gap = style.px (10);
    const auto inner = width - 2 * margin;
    const auto cardMin = style.px (200), cardMax = style.px (360);
    const auto columns = juce::jmax (1, (inner + gap) / (cardMin + gap));
    const auto cardWidth = juce::jmin (cardMax, (inner - gap * (columns - 1)) / columns);

    cards.clear();
    sections.clear();
    int y = margin + style.px (18);   // under the source caption

    std::vector<ParsedDevice> instruments, fx, sends;
    for (const auto& d : data.filtered)
        (d.type == Type::instrument ? instruments : d.type == Type::fx ? fx : sends).push_back (d);

    const auto section = [&] (const juce::String& title, juce::Colour colour, const std::vector<ParsedDevice>& list)
    {
        if (list.empty())
            return;
        y += style.px (6);
        sections.push_back ({ title, colour, static_cast<int> (list.size()), y });
        y += style.px (24);
        int rowTop = y, rowHeight = 0;
        for (size_t i = 0; i < list.size(); ++i)
        {
            const auto column = static_cast<int> (i) % columns;
            if (column == 0 && i > 0)
            {
                rowTop += rowHeight + gap;
                rowHeight = 0;
            }
            const auto x = margin + column * (cardWidth + gap);
            const auto height = card (nullptr, list[i], { x, rowTop, cardWidth, 1000 }, false);
            cards.push_back ({ list[i], { x, rowTop, cardWidth, height } });
            rowHeight = juce::jmax (rowHeight, height);
        }
        y = rowTop + rowHeight + style.px (18);
    };
    section (tr ("cards.sectionInstruments"), style.typeColour (Type::instrument), instruments);
    section (tr ("cards.sectionFx"), style.typeColour (Type::fx), fx);
    section (tr ("cards.sectionSends"), style.typeColour (Type::sendOnly), sends);

    const auto height = data.filtered.empty() ? viewport.getMaximumVisibleHeight() : y + margin;
    page.setSize (width, juce::jmax (height, viewport.getMaximumVisibleHeight()));
    page.repaint();
}

void CardsView::Page::paint (juce::Graphics& g)
{
    const auto& s = owner.style;
    g.fillAll (s.pal.windowPanel);
    const auto margin = s.px (16);

    g.setColour (s.pal.inkSoft);
    g.setFont (s.font (10.0f));
    g.drawText (tr ("cards.source"), margin, margin, getWidth() - 2 * margin, s.px (14), juce::Justification::centredLeft);

    for (const auto& section : owner.sections)
    {
        const auto row = juce::Rectangle<int> (margin, section.y, getWidth() - 2 * margin, s.px (18));
        g.setColour (s.onSurface (section.colour));
        g.setFont (s.font (11.0f, Fonts::Weight::semiBold));
        const auto title = section.title.toUpperCase();
        g.drawText (title, row, juce::Justification::centredLeft);
        auto x = row.getX() + juce::roundToInt (textWidth (g.getCurrentFont(), title)) + s.px (8);
        g.setFont (s.mono (10.0f));
        const auto count = juce::String (section.count);
        const auto chip = juce::Rectangle<int> (x, row.getY() + s.px (2), juce::roundToInt (textWidth (g.getCurrentFont(), count)) + s.px (12), row.getHeight() - s.px (4));
        g.setColour (s.pal.controlSurface);
        g.fillRect (chip);
        g.setColour (s.pal.inkSoft);
        g.drawText (count, chip, juce::Justification::centred);
        g.setColour (s.pal.shadowEdge.withAlpha (0.5f));
        g.fillRect (chip.getRight() + s.px (8), row.getCentreY(), row.getRight() - chip.getRight() - s.px (8), 1);
    }

    for (size_t i = 0; i < owner.cards.size(); ++i)
        if (g.clipRegionIntersects (owner.cards[i].bounds))
            owner.card (&g, owner.cards[i].device, owner.cards[i].bounds, static_cast<int> (i) == hovered);

    if (owner.data.filtered.empty())
    {
        auto area = getLocalBounds().withSizeKeepingCentre (getWidth(), s.px (60));
        g.setColour (s.pal.ink);
        g.setFont (s.font (14.0f, Fonts::Weight::semiBold));
        g.drawText (tr ("cards.noMatches"), area.removeFromTop (s.px (24)), juce::Justification::centred);
        g.setColour (s.pal.inkSoft);
        g.setFont (s.font (12.0f));
        g.drawText (tr ("cards.noMatchesHelp"), area, juce::Justification::centred);
    }
}

int CardsView::Page::cardAt (juce::Point<int> p) const
{
    for (size_t i = 0; i < owner.cards.size(); ++i)
        if (owner.cards[i].bounds.contains (p))
            return static_cast<int> (i);
    return -1;
}

void CardsView::Page::mouseMove (const juce::MouseEvent& e)
{
    const auto index = cardAt (e.getPosition());
    if (index == hovered)
        return;
    hovered = index;
    setMouseCursor (index >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    repaint();
}

void CardsView::Page::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void CardsView::Page::mouseUp (const juce::MouseEvent& e)
{
    const auto index = cardAt (e.getPosition());
    if (index < 0 || ! owner.data.select)
        return;
    const auto& d = owner.cards[static_cast<size_t> (index)].device;
    owner.data.select (owner.data.isSelected (d) ? std::nullopt : std::optional<ParsedDevice> (d));
}

} // namespace StudioPatchbay
