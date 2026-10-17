#include "PatchbayView.h"
#include "DeviceStyle.h"
#include "Localisation/Text.h"

namespace StudioPatchbay
{

namespace
{
using Text::tr;
using Type = ParsedDevice::Type;

const juce::String dot (juce::CharPointer_UTF8 ("  \xc2\xb7  "));
const juce::String dash (juce::CharPointer_UTF8 ("\xe2\x80\x93"));

float textWidth (const juce::Font& f, const juce::String& s) { return juce::GlyphArrangement::getStringWidth (f, s); }

juce::String busKindName (ControlRoomBus::Kind k)
{
    switch (k)
    {
        case ControlRoomBus::Kind::phones:  return tr ("controlRoom.kindPhones");
        case ControlRoomBus::Kind::speaker: return tr ("controlRoom.kindSpeaker");
        case ControlRoomBus::Kind::studio:  break;
    }
    return tr ("controlRoom.kindStudio");
}

template <typename Map>
auto lookup (const Map& map, int ch) -> typename Map::mapped_type
{
    if (auto it = map.find (ch); it != map.end())
        return it->second;
    return {};
}
} // namespace

PatchbayView::PatchbayView (const ViewData& d) : ContentView (d)
{
    viewport.setViewedComponent (&page, false);
    setupButton.onClick = [this] { if (data.openSetup) data.openSetup(); };
    page.addChildComponent (setupButton);
}

void PatchbayView::setShowConnections (bool show)
{
    showConnections = show;
    refresh();
}

void PatchbayView::refresh()
{
    page.setSize (pageWidth(), 10);
    const auto height = layout (nullptr);
    page.setSize (pageWidth(), juce::jmax (height, viewport.getMaximumVisibleHeight()));
    setupButton.setButtonText (tr ("patchbay.openSetup"));
    setupButton.setVisible (data.physical.empty() && ! showConnections);
    setupButton.setBounds (bannerButtonArea);
    page.repaint();
}

juce::Rectangle<int> PatchbayView::channelBounds (int channel) const
{
    for (const auto& c : cells)
        if (c.channel == channel && c.direction == ChannelDirection::send)
            return c.bounds;
    for (const auto& c : cells)
        if (c.channel == channel)
            return c.bounds;
    return {};
}

void PatchbayView::scrollToChannel (int channel)
{
    const auto target = channelBounds (channel);
    if (target.isEmpty())
        return;
    viewport.setViewPosition (0, juce::jmax (0, target.getCentreY() - viewport.getMaximumVisibleHeight() / 2));
}

int PatchbayView::layout (juce::Graphics* g)
{
    const auto& s = style;
    const auto width = page.getWidth();
    const auto ui = static_cast<float> (s.uiScale);
    const auto cellSize = juce::roundToInt (juce::jmax (36.0f * static_cast<float> (s.contentScale), 26.0f) * ui);
    const auto cellGap = s.px (3);
    const auto labelFont = s.mono (juce::jmax (9.0f, 8.5f / static_cast<float> (s.contentScale)));
    const auto subFont = s.mono (juce::jmax (9.0f, 8.0f / static_cast<float> (s.contentScale)));
    const auto headerFont = s.font (juce::jmax (12.0f, 10.5f / static_cast<float> (s.contentScale)), Fonts::Weight::bold);
    int y = 0;

    if (g == nullptr)
        cells.clear();
    if (g != nullptr)
        g->fillAll (s.pal.windowPanel);

    const auto hairline = [&] (int lineY, float alpha)
    {
        if (g != nullptr)
        {
            g->setColour (s.pal.shadowEdge.withAlpha (alpha));
            g->fillRect (0, lineY, width, 1);
        }
    };
    const auto text = [&] (const juce::String& t, juce::Rectangle<int> r, juce::Colour c, const juce::Font& f,
                           juce::Justification j = juce::Justification::centredLeft)
    {
        if (g != nullptr)
        {
            g->setColour (c);
            g->setFont (f);
            g->drawText (t, r, j, true);
        }
    };

    // No converters yet: say why every block reads "Unknown".
    if (data.physical.empty() && ! showConnections)
    {
        const auto banner = juce::Rectangle<int> (0, y, width, s.px (44));
        if (g != nullptr)
        {
            g->setColour (s.warning().withAlpha (0.10f));
            g->fillRect (banner);
            g->setColour (s.pal.inkSoft);
            g->setFont (s.font (11.0f));
            g->drawFittedText (tr ("patchbay.noDevicesBanner"), banner.reduced (s.px (16), s.px (4)).withTrimmedRight (s.px (150)),
                               juce::Justification::centredLeft, 2);
        }
        bannerButtonArea = juce::Rectangle<int> (width - s.px (150), banner.getCentreY() - s.px (11), s.px (134), s.px (22));
        y += banner.getHeight();
        hairline (y++, 0.6f);
    }

    // Legend.
    {
        auto row = juce::Rectangle<int> (s.px (16), y + s.px (10), width - s.px (32), s.px (16));
        const std::pair<juce::Colour, juce::String> items[] = {
            { s.typeColour (Type::instrument), tr ("patchbay.legendInstrument") },
            { s.typeColour (Type::fx), tr ("patchbay.legendFx") },
            { s.typeColour (Type::sendOnly), tr ("patchbay.legendSendOnly") },
            { s.controlRoom(), tr ("patchbay.legendControlRoom") } };
        auto x = row.getX();
        const auto f = s.font (10.0f);
        for (auto& [colour, label] : items)
        {
            if (g != nullptr)
            {
                g->setColour (colour.withAlpha (0.25f));
                g->fillRect (x, row.getY() + s.px (1), s.px (14), s.px (14));
                g->setColour (colour.withAlpha (0.7f));
                g->drawRect (x, row.getY() + s.px (1), s.px (14), s.px (14), 1);
            }
            text (label, { x + s.px (19), row.getY(), s.px (110), row.getHeight() }, s.pal.inkSoft, f);
            x += s.px (19) + juce::roundToInt (textWidth (f, label)) + s.px (16);
        }
        text (tr ("patchbay.legendHint"), { x + s.px (8), row.getY(), row.getRight() - x - s.px (8), row.getHeight() },
              s.pal.inkSoft, s.font (9.5f));
        y += s.px (36);
        hairline (y++, 0.6f);
    }

    const auto send = data.devices.devicesByChannel (ChannelDirection::send);
    const auto recv = data.devices.devicesByChannel (ChannelDirection::recv);
    const auto buses = Patchbay::busesByChannel (data.buses);
    const auto highlighted = [&] (int ch) { return data.highlightedChannels.count (ch) > 0; };

    // One row of cells (wrapping) with the Cubase channel under each.
    const auto cellRow = [&] (const std::vector<int>& channels, ChannelDirection direction,
                              const std::optional<PhysicalDevice>& device, juce::Colour accent,
                              std::function<std::vector<ControlRoomBus> (int)> busesOn)
    {
        const auto left = s.px (12) + s.px (12);
        auto x = left;
        const auto lineHeight = cellSize + s.px (14);
        for (auto ch : channels)
        {
            if (x > left && x + cellSize > width - s.px (12))
            {
                x = left;
                y += lineHeight;
            }
            Cell cell { { x, y, cellSize, cellSize }, ch, direction,
                        lookup (direction == ChannelDirection::send ? send : recv, ch),
                        busesOn (ch),
                        device ? device->physicalIO (ch, direction) : std::nullopt };
            if (g == nullptr)
                cells.push_back (cell);
            else if (g->clipRegionIntersects (cell.bounds.expanded (2, s.px (14))))
            {
                const auto used = ! cell.devices.empty();
                const auto busOnly = ! used && ! cell.buses.empty();
                const auto selected = used && data.selected
                                      && std::find (cell.devices.begin(), cell.devices.end(), *data.selected) != cell.devices.end();
                const auto isHovered = page.hovered >= 0 && static_cast<size_t> (page.hovered) < cells.size()
                                       && cells[static_cast<size_t> (page.hovered)].bounds == cell.bounds;
                const auto isMatch = highlighted (ch);
                const auto colour = used ? s.cellColour (cell.devices) : (busOnly ? s.controlRoom() : s.pal.inkSoft);
                const auto outDim = direction == ChannelDirection::send ? 0.82f : 1.0f;

                // Not a match while filtering or searching: faded, as in the Swift app.
                const auto fade = used && ! isMatch ? 0.15f : 1.0f;
                const auto r = cell.bounds.toFloat();
                g->setColour ((used || busOnly ? colour.withAlpha ((s.dark ? 0.32f : 0.20f) * outDim)
                                               : (direction == ChannelDirection::send ? s.pal.controlSurface : s.pal.field))
                                  .withMultipliedAlpha (fade));
                g->fillRoundedRectangle (r, static_cast<float> (s.px (4)));
                if ((isHovered || selected) && used)
                {
                    g->setColour (s.pal.ink.withAlpha (0.08f * fade));
                    g->fillRoundedRectangle (r, static_cast<float> (s.px (4)));
                }
                const auto searchRing = data.searchActive && isMatch && used;
                const auto strong = selected || searchRing || (isHovered && used);
                g->setColour ((strong ? s.pal.ink : (used || busOnly ? colour.withAlpha (0.8f) : s.pal.shadowEdge.withAlpha (0.6f)))
                                  .withMultipliedAlpha (fade));
                g->drawRoundedRectangle (r.reduced (0.5f), static_cast<float> (s.px (4)),
                                         selected || searchRing ? 2.0f : 1.0f);

                const auto number = cell.physicalIO ? juce::String (*cell.physicalIO) : juce::String (ch);
                g->setColour ((used || busOnly ? s.onSurface (colour) : s.pal.inkSoft).withMultipliedAlpha (fade));
                g->setFont (s.mono (1.0f).withHeight (juce::jlimit (8.0f, 16.0f, static_cast<float> (cellSize) * 0.28f)));
                g->drawText (number, cell.bounds, juce::Justification::centred);
                if (! cell.buses.empty())
                {
                    g->setColour (s.controlRoom().withMultipliedAlpha (fade));
                    const auto d = juce::jmax (4.0f, static_cast<float> (cellSize) * 0.16f);
                    g->fillEllipse (r.getRight() - d - 3.0f, r.getY() + 3.0f, d, d);
                }
                g->setColour (accent.withAlpha (0.6f));
                g->setFont (subFont);
                g->drawText ("ch" + juce::String (ch), cell.bounds.getX() - s.px (4), cell.bounds.getBottom() + 1,
                             cell.bounds.getWidth() + s.px (8), s.px (12), juce::Justification::centred);
            }
            x += cellSize + cellGap;
        }
        y += lineHeight + s.px (3);
    };

    const auto dirLabel = [&] (const juce::String& t, juce::Colour c)
    {
        text (t, { s.px (24), y, width, s.px (13) }, c, labelFont.boldened());
        y += s.px (15);
    };

    // A device block: header, OUT row, IN row (and for the export, what is connected).
    const auto block = [&] (const Patchbay::Block& b, juce::Colour portColour, const juce::String& interfaceName)
    {
        const auto accent = b.device ? DeviceStyle::colourFromHex (b.device->customColorHex).value_or (portColour) : s.pal.inkSoft;
        y += s.px (6);
        const auto header = juce::Rectangle<int> (s.px (12), y, width - s.px (24), s.px (b.device ? 34 : 24));
        if (g != nullptr)
        {
            g->setColour (accent.withAlpha (0.09f));
            g->fillRect (header);
            g->setColour (accent.withAlpha (0.35f));
            g->fillRect (header.getX(), header.getY(), header.getWidth(), 1);
            g->setColour (accent.withAlpha (0.25f));
            g->fillRect (header.getX(), header.getBottom() - 1, header.getWidth(), 1);
            g->setColour (accent);
            g->fillRect (header.getX() + s.px (8), header.getY() + s.px (5), juce::jmax (2, s.px (3)), header.getHeight() - s.px (10));
        }
        auto textArea = header.withTrimmedLeft (s.px (19)).reduced (0, s.px (5));
        auto name = b.device ? b.device->name : tr ("patchbay.unknown");
        if (b.device && b.device->deviceType != DeviceType::converter)
            name = DeviceStyle::deviceTypeName (b.device->deviceType) + ": " + name;
        text (name, textArea.removeFromTop (headerFont.getHeight() > 0 ? juce::roundToInt (headerFont.getHeight()) : s.px (14)),
              s.onSurface (accent), headerFont);
        if (b.device)
            text (interfaceName + dot + Patchbay::headerChannelLabel (*b.device) + dot + tr ("patchbay.physicalIO")
                      + " 1" + dash + juce::String (b.device->physicalChannelCount),
                  textArea, s.onSurface (accent).withAlpha (0.75f), subFont);
        y = header.getBottom() + s.px (5);

        const auto busesOut = [&] (int ch) { return lookup (buses, ch); };
        const auto none = [] (int) { return std::vector<ControlRoomBus>(); };
        dirLabel (juce::String (juce::CharPointer_UTF8 ("OUT \xe2\x86\x92 ")) + tr ("patchbay.hardware"), s.onSurface (accent).withAlpha (0.7f));
        cellRow (b.rowChannels (ChannelDirection::send), ChannelDirection::send, b.device, accent, busesOut);
        dirLabel (juce::String (juce::CharPointer_UTF8 ("IN \xe2\x86\x90 ")) + tr ("patchbay.hardware"), s.onSurface (accent));
        cellRow (b.rowChannels (ChannelDirection::recv), ChannelDirection::recv, b.device, accent, none);

        if (showConnections)
        {
            const auto connected = Patchbay::connections (b, send, recv);
            const auto busList = Patchbay::busConnections (b, buses);
            if (! connected.empty() || ! busList.empty())
            {
                const auto columns = std::array<int, 4> { s.px (24), s.px (24 + 222), s.px (24 + 334), s.px (24 + 426) };
                dirLabel (tr ("patchbay.connected"), accent.withAlpha (0.6f));
                const auto row = [&] (const juce::String& a, const juce::String& b2, const juce::String& c, const juce::String& d,
                                      juce::Colour colour, const juce::Font& f)
                {
                    const juce::String values[] = { a, b2, c, d };
                    for (size_t i = 0; i < 4; ++i)
                        text (values[i], { columns[i], y, (i < 3 ? columns[i + 1] - columns[i] - s.px (12) : s.px (200)), s.px (13) },
                              colour, f);
                    y += s.px (14);
                };
                row (tr ("patchbay.columnDevice"), tr ("patchbay.columnType"), "OUT", "IN", s.pal.inkSoft, subFont.boldened());
                hairline (y, 0.3f);
                y += 2;
                for (const auto& c : connected)
                {
                    const auto type = c.device.type == Type::instrument ? tr ("cards.typeInstrument")
                                    : c.device.type == Type::fx ? tr ("cards.typeFx") : tr ("cards.typeSendOnly");
                    row (c.device.name, type, c.out, c.in, s.pal.inkSoft, subFont);
                }
                for (const auto& [bus, channels] : busList)
                    row (bus.name, tr ("patchbay.legendControlRoom"), channels, {}, s.onSurface (s.controlRoom()), subFont);
                y += s.px (4);
            }
        }
        y += s.px (6);
        hairline (y++, 0.35f);
    };

    const auto sectionHeader = [&] (juce::Colour tint, const juce::String& title, const juce::String& detail,
                                    const juce::String& right, juce::Colour titleColour)
    {
        const auto area = juce::Rectangle<int> (0, y, width, s.px (30));
        if (g != nullptr)
        {
            g->setColour (tint);
            g->fillRect (area);
        }
        auto row = area.reduced (s.px (16), 0);
        const auto titleFont = s.font (11.0f, Fonts::Weight::semiBold);
        const auto titleWidth = juce::roundToInt (textWidth (titleFont, title)) + s.px (8);
        text (title, row.removeFromLeft (titleWidth), titleColour, titleFont);
        const auto rightFont = s.font (10.0f);
        const auto rightWidth = juce::roundToInt (textWidth (rightFont, right)) + s.px (4);
        text (right, row.removeFromRight (rightWidth), s.pal.inkSoft, rightFont, juce::Justification::centredRight);
        text (detail, row, s.pal.inkSoft, s.font (10.0f));
        y = area.getBottom();
        hairline (y++, 0.6f);
    };

    const auto layoutModel = Patchbay::build (data.devices, data.buses, data.ports, data.physical);
    for (const auto& section : layoutModel.ports)
    {
        const auto colour = DeviceStyle::connectorColour (section.port.color, s.pal.inkSoft);
        sectionHeader (colour.withAlpha (0.08f), section.port.name,
                       dot.trimStart() + section.port.interfaceName + "   ch " + juce::String (section.port.channelStart) + dash
                           + juce::String (section.port.channelEnd),
                       tr ("patchbay.channelsInUse").replace ("%COUNT%", juce::String (section.usedCount)),
                       s.onSurface (colour));
        for (const auto& b : section.blocks)
            block (b, colour, section.port.interfaceName);
        hairline (y++, 0.6f);
    }

    if (! layoutModel.audioInterfaces.empty())
    {
        sectionHeader (s.pal.ink.withAlpha (0.05f), tr ("patchbay.audioInterfaces").toUpperCase(),
                       tr ("patchbay.audioInterfacesDetail"), {}, s.pal.ink);
        for (const auto& d : layoutModel.audioInterfaces)
        {
            const auto r = d.channelRange();
            block ({ d, r.first, r.last }, DeviceStyle::connectorColour (d.portColor, s.pal.inkSoft),
                   DeviceStyle::deviceTypeName (DeviceType::audioInterface));
        }
    }

    if (! data.buses.empty())
    {
        sectionHeader (s.controlRoom().withAlpha (0.08f), tr ("patchbay.legendControlRoom").toUpperCase(),
                       tr ("patchbay.controlRoomDetail"), {}, s.pal.ink);
        for (const auto& bus : data.buses)
        {
            auto channels = bus.channels;
            std::sort (channels.begin(), channels.end());
            juce::StringArray via;
            for (auto ch : channels)
                if (auto* p = PhysicalDevices::find (data.physical, ch, ChannelDirection::send))
                    via.addIfNotAlreadyThere (p->name);
            via.sort (false);
            const auto routing = via.isEmpty() ? tr ("patchbay.directOutput") : tr ("patchbay.via") + " " + via.joinIntoString (" + ");

            y += s.px (6);
            const auto header = juce::Rectangle<int> (s.px (12), y, width - s.px (24), s.px (34));
            const auto teal = s.controlRoom();
            if (g != nullptr)
            {
                g->setColour (teal.withAlpha (0.09f));
                g->fillRect (header);
                g->setColour (teal.withAlpha (0.35f));
                g->fillRect (header.getX(), header.getY(), header.getWidth(), 1);
                g->setColour (teal);
                g->fillRect (header.getX() + s.px (8), header.getY() + s.px (5), juce::jmax (2, s.px (3)), header.getHeight() - s.px (10));
            }
            auto textArea = header.withTrimmedLeft (s.px (19)).reduced (0, s.px (5));
            text (bus.name, textArea.removeFromTop (juce::roundToInt (headerFont.getHeight())), s.onSurface (teal), headerFont);
            text (busKindName (bus.kind) + dot + "OUT ch " + Patchbay::rangeString (channels) + dot + routing, textArea,
                  s.onSurface (teal).withAlpha (0.75f), subFont);
            y = header.getBottom() + s.px (5);
            dirLabel (juce::String (juce::CharPointer_UTF8 ("OUT \xe2\x86\x92 ")) + tr ("patchbay.hardware"), s.onSurface (teal).withAlpha (0.7f));
            const auto thisBus = [&bus] (int) { return std::vector<ControlRoomBus> { bus }; };
            std::optional<PhysicalDevice> none;
            // Physical I/O per channel comes from whichever device sends on it.
            const auto left = s.px (24);
            juce::ignoreUnused (left);
            cellRow (channels, ChannelDirection::send, none, teal, thisBus);
            if (g == nullptr)
                for (auto& c : cells)
                    if (c.direction == ChannelDirection::send && ! c.physicalIO && c.buses.size() == 1 && c.buses[0].index == bus.index)
                        if (auto* p = PhysicalDevices::find (data.physical, c.channel, ChannelDirection::send))
                            c.physicalIO = p->physicalIO (c.channel, ChannelDirection::send);
            y += s.px (6);
            hairline (y++, 0.35f);
        }
    }

    return y + s.px (20);
}

int PatchbayView::cellAt (juce::Point<int> p) const
{
    for (size_t i = 0; i < cells.size(); ++i)
        if (cells[i].bounds.contains (p))
            return static_cast<int> (i);
    return -1;
}

void PatchbayView::paintTooltip (juce::Graphics& g)
{
    if (page.hovered < 0 || static_cast<size_t> (page.hovered) >= cells.size())
        return;
    const auto& c = cells[static_cast<size_t> (page.hovered)];
    if (c.devices.empty() && c.buses.empty())
        return;

    const auto dir = c.direction == ChannelDirection::send ? juce::String ("OUT") : juce::String ("IN");
    const auto where = c.physicalIO ? "I/O " + juce::String (*c.physicalIO) + dot + tr ("patchbay.cubaseChannel") + " " + juce::String (c.channel)
                                    : "ch " + juce::String (c.channel);
    juce::StringArray lines { "[" + dir + "]  " + where };
    if (c.devices.size() == 1)
        lines.add (c.devices[0].name);
    else if (! c.devices.empty())
    {
        for (const auto& d : c.devices)
            lines.add (juce::String (juce::CharPointer_UTF8 ("\xe2\x80\xa2 ")) + d.name);
        lines.add (tr ("patchbay.clickToCycle"));
    }
    for (const auto& bus : c.buses)
        lines.add (tr ("patchbay.legendControlRoom") + ": " + bus.name);

    const auto& s = style;
    const auto font = Fonts::sans (static_cast<float> (11.0 * s.uiScale));
    int w = 0;
    for (const auto& l : lines)
        w = juce::jmax (w, juce::roundToInt (textWidth (font, l)));
    const auto lineHeight = juce::roundToInt (font.getHeight() * 1.25f);
    auto box = juce::Rectangle<int> (c.bounds.getX(), c.bounds.getY() - lineHeight * lines.size() - s.px (16),
                                     w + s.px (16) + 6, lineHeight * lines.size() + s.px (10));
    box = box.constrainedWithin (page.getLocalBounds());
    if (box.getBottom() > c.bounds.getY())
        box.setY (c.bounds.getBottom() + s.px (6));

    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRect (box.translated (2, 2));
    g.setColour (s.pal.field);
    g.fillRect (box);
    g.setColour (s.pal.ink);
    g.drawRect (box, 1);
    g.setFont (font);
    auto textArea = box.reduced (s.px (8), s.px (5));
    for (const auto& l : lines)
        g.drawText (l, textArea.removeFromTop (lineHeight), juce::Justification::centredLeft);
}

void PatchbayView::Page::mouseMove (const juce::MouseEvent& e)
{
    const auto index = owner.cellAt (e.getPosition());
    if (index == hovered)
        return;
    hovered = index;
    const auto clickable = index >= 0 && ! owner.cells[static_cast<size_t> (index)].devices.empty();
    setMouseCursor (clickable ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    repaint();
}

void PatchbayView::Page::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void PatchbayView::Page::mouseUp (const juce::MouseEvent& e)
{
    const auto index = owner.cellAt (e.getPosition());
    if (index < 0 || ! owner.data.select)
        return;
    const auto& devices = owner.cells[static_cast<size_t> (index)].devices;
    if (devices.empty())
        return;

    // One device: click toggles it. Several: each click selects the next.
    const auto& selected = owner.data.selected;
    if (devices.size() == 1)
    {
        owner.data.select (owner.data.isSelected (devices[0]) ? std::nullopt : std::optional<ParsedDevice> (devices[0]));
        return;
    }
    auto it = selected ? std::find (devices.begin(), devices.end(), *selected) : devices.end();
    if (it == devices.end())
        owner.data.select (devices.front());
    else
        owner.data.select (*(std::next (it) == devices.end() ? devices.begin() : std::next (it)));
}

} // namespace StudioPatchbay
