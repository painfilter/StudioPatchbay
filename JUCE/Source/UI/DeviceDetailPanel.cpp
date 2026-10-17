#include "DeviceDetailPanel.h"
#include "DeviceStyle.h"
#include "Localisation/Text.h"

#include <set>

namespace StudioPatchbay
{

namespace
{
using Text::tr;
using Type = ParsedDevice::Type;
using K = SignalFlow::Node::Kind;

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

// Small drawn symbols for the flow nodes (no emoji: the bundled font has none).
enum class Icon { computer, keys, plug, up, down, unit, warning, midi };

Icon iconFor (K kind)
{
    switch (kind)
    {
        case K::cubaseInstrumentTrack: case K::cubaseReceived: case K::cubaseSent:
        case K::cubaseFxSend: case K::cubaseFxReturn: return Icon::computer;
        case K::midiOut:          return Icon::midi;
        case K::instrument:       return Icon::keys;
        case K::noChannels: case K::noSendChannels: return Icon::warning;
        case K::analogOut: case K::analogIn: return Icon::plug;
        case K::interfaceReceive: return Icon::down;
        case K::interfaceSend:    return Icon::up;
        case K::hardware:         return Icon::unit;
    }
    return Icon::unit;
}

void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> r, juce::Colour c)
{
    const auto line = juce::jmax (1.0f, r.getWidth() / 12.0f);
    g.setColour (c);
    switch (icon)
    {
        case Icon::computer:
            g.drawRect (r.withTrimmedBottom (r.getHeight() * 0.3f), line);
            g.fillRect (r.getCentreX() - r.getWidth() * 0.2f, r.getBottom() - line * 2, r.getWidth() * 0.4f, line);
            break;
        case Icon::keys:
        case Icon::unit:
        {
            const auto body = r.withSizeKeepingCentre (r.getWidth(), r.getHeight() * 0.6f);
            g.drawRect (body, line);
            for (int i = 1; i < 4; ++i)
                if (icon == Icon::keys)
                    g.fillRect (body.getX() + body.getWidth() * i / 4.0f, body.getY(), line, body.getHeight() * 0.6f);
                else
                    g.drawEllipse (body.getX() + body.getWidth() * (i - 0.5f) / 3.5f, body.getCentreY() - line * 1.5f,
                                   line * 3, line * 3, line * 0.8f);
            break;
        }
        case Icon::plug:
            g.drawEllipse (r.reduced (r.getWidth() * 0.2f), line);
            g.fillEllipse (r.withSizeKeepingCentre (line * 2.5f, line * 2.5f));
            break;
        case Icon::up:
        case Icon::down:
        {
            juce::Path p;
            const auto cx = r.getCentreX(), top = r.getY() + line, bottom = r.getBottom() - line;
            p.startNewSubPath (cx, icon == Icon::up ? bottom : top);
            p.lineTo (cx, icon == Icon::up ? top : bottom);
            p.startNewSubPath (cx - r.getWidth() * 0.25f, icon == Icon::up ? top + r.getHeight() * 0.25f : bottom - r.getHeight() * 0.25f);
            p.lineTo (cx, icon == Icon::up ? top : bottom);
            p.lineTo (cx + r.getWidth() * 0.25f, icon == Icon::up ? top + r.getHeight() * 0.25f : bottom - r.getHeight() * 0.25f);
            g.strokePath (p, juce::PathStrokeType (line * 1.3f));
            break;
        }
        case Icon::warning:
        {
            juce::Path p;
            p.addTriangle (r.getCentreX(), r.getY() + line, r.getRight() - line, r.getBottom() - line, r.getX() + line, r.getBottom() - line);
            g.strokePath (p, juce::PathStrokeType (line));
            g.fillRect (r.getCentreX() - line * 0.5f, r.getY() + r.getHeight() * 0.38f, line, r.getHeight() * 0.28f);
            break;
        }
        case Icon::midi:
            g.drawEllipse (r.reduced (line), line);
            for (int i = 0; i < 5; ++i)
            {
                const auto a = juce::MathConstants<float>::pi * (0.15f + 0.175f * static_cast<float> (i)) + juce::MathConstants<float>::pi;
                g.fillEllipse (r.getCentreX() + std::cos (a) * r.getWidth() * 0.25f - line * 0.6f,
                               r.getCentreY() - std::sin (a) * r.getWidth() * 0.25f - line * 0.6f, line * 1.2f, line * 1.2f);
            }
            break;
    }
}
} // namespace

DeviceDetailPanel::DeviceDetailPanel (const ViewData& d) : data (d)
{
    viewport.setViewedComponent (&page, false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);
    closeButton.getProperties().set ("windowClose", true);
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);
}

void DeviceDetailPanel::setStyle (const ViewStyle& s)
{
    style = s;
    style.contentScale = 1.0;   // the panel follows the window zoom only
    resized();
}

void DeviceDetailPanel::refresh()
{
    nodes = data.selected ? SignalFlow::build (*data.selected, data.physical, data.ports) : std::vector<SignalFlow::Node>();
    closeButton.setTooltip (tr ("action.close"));
    resized();
    page.repaint();
}

void DeviceDetailPanel::paint (juce::Graphics& g)
{
    g.fillAll (style.pal.windowPanel);
    g.setColour (style.pal.ink);
    g.fillRect (0, 0, juce::jmax (1, sc (1)), getHeight());
}

void DeviceDetailPanel::resized()
{
    viewport.setBounds (getLocalBounds().withTrimmedLeft (juce::jmax (1, sc (1))));
    closeButton.setBounds (getWidth() - sc (26), sc (14), sc (16), sc (16));
    page.setSize (viewport.getMaximumVisibleWidth(), 10);
    page.setSize (viewport.getMaximumVisibleWidth(), juce::jmax (layout (nullptr), viewport.getMaximumVisibleHeight()));
}

int DeviceDetailPanel::layout (juce::Graphics* g)
{
    if (! data.selected)
        return 0;
    const auto& d = *data.selected;
    const auto& s = style;
    const auto width = page.getWidth();
    const auto accent = s.typeColour (d.type);
    auto inner = juce::Rectangle<int> (sc (16), sc (14), width - sc (32), 0);
    int y = inner.getY();
    if (g == nullptr)
        nodeBounds.clear();
    else
        g->fillAll (s.pal.windowPanel);

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
    const auto divider = [&]
    {
        y += sc (10);
        if (g != nullptr)
        {
            g->setColour (s.pal.shadowEdge.withAlpha (0.5f));
            g->fillRect (inner.getX(), y, inner.getWidth(), 1);
        }
        y += sc (10);
    };
    const auto sectionTitle = [&] (const juce::String& t)
    {
        text (t.toUpperCase(), { inner.getX(), y, inner.getWidth(), sc (14) }, s.pal.inkSoft,
              Fonts::sans (scf (10.5f), Fonts::Weight::semiBold));
        y += sc (20);
    };

    // Header: type, name, ports.
    text (typeName (d.type).toUpperCase(), { inner.getX(), y, inner.getWidth() - sc (24), sc (13) }, s.onSurface (accent),
          Fonts::sans (scf (10.0f), Fonts::Weight::semiBold));
    y += sc (15);
    if (g != nullptr)
    {
        g->setColour (s.pal.ink);
        g->setFont (Fonts::sans (scf (16.0f), Fonts::Weight::semiBold));
        g->drawFittedText (d.name, inner.getX(), y, inner.getWidth() - sc (24), sc (40), juce::Justification::topLeft, 2);
    }
    y += sc (24);
    {
        std::vector<const AudioPort*> ports;
        for (auto ch : d.allChannels())
            if (auto* p = AudioPorts::find (data.ports, ch); p != nullptr && std::find (ports.begin(), ports.end(), p) == ports.end())
                ports.push_back (p);
        std::sort (ports.begin(), ports.end(), [] (auto* a, auto* b) { return a->id < b->id; });
        auto x = inner.getX();
        const auto f = Fonts::sans (scf (10.0f));
        for (auto* p : ports)
        {
            const auto colour = DeviceStyle::connectorColour (p->color, s.pal.inkSoft);
            const auto w = juce::roundToInt (juce::GlyphArrangement::getStringWidth (f, p->name)) + sc (20);
            if (g != nullptr)
            {
                const auto chip = juce::Rectangle<int> (x, y, w, sc (16));
                g->setColour (colour.withAlpha (0.14f));
                g->fillRect (chip);
                g->setColour (colour);
                g->fillEllipse (static_cast<float> (chip.getX() + sc (5)), chip.getCentreY() - scf (3.0f), scf (6.0f), scf (6.0f));
                g->setColour (s.onSurface (colour));
                g->setFont (f);
                g->drawText (p->name, chip.withTrimmedLeft (sc (14)), juce::Justification::centredLeft);
            }
            x += w + sc (4);
        }
        if (! ports.empty())
            y += sc (18);
    }
    divider();

    // Signal flow.
    sectionTitle (tr ("detail.signalFlow"));
    const auto ioDetail = [] (const SignalFlow::Node& n)
    {
        return n.text + juce::String (juce::CharPointer_UTF8 ("  \xe2\x80\xa2  ")) + "Ch " + n.detail
               + (n.noConverter ? juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  ")) + tr ("detail.noConverter") : juce::String());
    };
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const auto& n = nodes[i];
        juce::String label;
        juce::Colour colour = s.pal.ink;
        switch (n.kind)
        {
            case K::cubaseInstrumentTrack: label = tr ("flow.instrumentTrack"); break;
            case K::midiOut:          label = tr ("flow.midiOut").replace ("%NAME%", n.text); colour = s.midi(); break;
            case K::instrument:
            case K::hardware:         label = n.text; colour = accent; break;
            case K::noChannels:       label = tr ("flow.noChannels"); colour = s.warning(); break;
            case K::noSendChannels:   label = tr ("flow.noSendChannels"); colour = s.warning(); break;
            case K::analogOut:        label = tr ("flow.analogOut").replace ("%NAME%", n.text); colour = s.pal.inkSoft; break;
            case K::analogIn:         label = tr ("flow.analogIn").replace ("%NAME%", n.text); colour = s.pal.inkSoft; break;
            case K::interfaceReceive: label = ioDetail (n); colour = s.typeColour (Type::instrument); break;
            case K::interfaceSend:    label = ioDetail (n); colour = d.type == Type::sendOnly ? s.typeColour (Type::sendOnly)
                                                                                                : s.typeColour (Type::fx); break;
            case K::cubaseReceived:   label = tr ("flow.cubaseReceived"); break;
            case K::cubaseSent:       label = tr ("flow.cubaseSent"); break;
            case K::cubaseFxSend:     label = tr ("flow.cubaseFxSend"); break;
            case K::cubaseFxReturn:   label = tr ("flow.cubaseFxReturn"); break;
        }

        const auto font = Fonts::sans (scf (11.5f));
        const auto textWidth = inner.getWidth() - sc (40);
        juce::GlyphArrangement wrapped;
        wrapped.addJustifiedText (font, label, 0.0f, 0.0f, static_cast<float> (textWidth), juce::Justification::left);
        const auto lines = juce::jlimit (1, 3, juce::roundToInt (wrapped.getBoundingBox (0, -1, true).getHeight() / font.getHeight()));
        const auto box = juce::Rectangle<int> (inner.getX(), y, inner.getWidth(), juce::roundToInt (font.getHeight() * static_cast<float> (lines)) + sc (14));
        if (g == nullptr)
            nodeBounds.push_back (box);
        else
        {
            const auto hover = static_cast<int> (i) == page.hovered && n.jumpChannel.has_value();
            g->setColour (colour.withAlpha (hover ? 0.18f : 0.08f));
            g->fillRect (box);
            g->setColour (colour.withAlpha (hover ? 0.5f : 0.25f));
            g->drawRect (box, 1);
            drawIcon (*g, iconFor (n.kind), box.withWidth (sc (28)).toFloat().withSizeKeepingCentre (scf (14.0f), scf (14.0f)),
                      s.onSurface (colour));
            g->setColour (s.pal.ink);
            g->setFont (font);
            g->drawFittedText (label, box.withTrimmedLeft (sc (30)).withTrimmedRight (sc (14)).reduced (0, sc (6)),
                               juce::Justification::centredLeft, lines);
            if (hover)
                drawIcon (*g, Icon::up, box.withLeft (box.getRight() - sc (14)).toFloat().withSizeKeepingCentre (scf (9.0f), scf (9.0f)),
                          s.onSurface (colour));
        }
        y = box.getBottom();
        if (i + 1 < nodes.size())
        {
            text (juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x93")), { inner.getX() + sc (8), y, sc (12), sc (13) },
                  s.pal.inkSoft, Fonts::sans (scf (10.0f)), juce::Justification::centred);
            y += sc (13);
        }
    }
    if (d.type == Type::fx && d.hasMismatchedChannels())
    {
        y += sc (6);
        const auto box = juce::Rectangle<int> (inner.getX(), y, inner.getWidth(), sc (26));
        if (g != nullptr)
        {
            g->setColour (s.warning().withAlpha (0.12f));
            g->fillRect (box);
            drawIcon (*g, Icon::warning, box.withWidth (sc (26)).toFloat().withSizeKeepingCentre (scf (13.0f), scf (13.0f)), s.warning());
        }
        text (tr ("detail.mismatch"), box.withTrimmedLeft (sc (28)), s.pal.ink, Fonts::sans (scf (11.0f)));
        y = box.getBottom();
    }
    divider();

    // Details.
    sectionTitle (tr ("detail.details"));
    const auto row = [&] (const juce::String& label, const juce::String& value)
    {
        const auto r = juce::Rectangle<int> (inner.getX(), y, inner.getWidth(), sc (26));
        if (g != nullptr)
        {
            g->setColour (s.pal.controlSurface.withAlpha (0.6f));
            g->fillRect (r);
        }
        const auto labelFont = Fonts::sans (scf (11.0f));
        const auto labelWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (labelFont, label)) + sc (16);
        text (label, r.reduced (sc (8), 0).withWidth (labelWidth), s.pal.inkSoft, labelFont);
        text (value, r.reduced (sc (8), 0).withTrimmedLeft (labelWidth), s.pal.ink, s.mono (11.0f).withHeight (scf (11.0f)),
              juce::Justification::centredRight);
        y += r.getHeight() + sc (4);
    };

    const auto count = d.channelCount();
    row (tr ("detail.format"), count > 2 ? tr ("detail.channels").replace ("%COUNT%", juce::String (count))
                              : d.isStereo() ? tr ("cards.stereo") : d.isMono() ? tr ("cards.mono") : tr ("detail.unknown"));
    if (d.midiDevice.isNotEmpty())
        row (tr ("detail.midi"), d.midiDevice);

    std::vector<const AudioPort*> ports;
    for (auto ch : d.allChannels())
        if (auto* p = AudioPorts::find (data.ports, ch); p != nullptr && std::find (ports.begin(), ports.end(), p) == ports.end())
            ports.push_back (p);
    std::sort (ports.begin(), ports.end(), [] (auto* a, auto* b) { return a->id < b->id; });
    if (! d.allChannels().empty())
    {
        std::set<ConnectorType> colours;
        for (auto* p : ports) colours.insert (p->color);
        const auto label = colours == std::set<ConnectorType> { ConnectorType::adat } ? tr ("detail.adatChannels")
                         : colours == std::set<ConnectorType> { ConnectorType::madi } ? tr ("detail.madiChannels")
                                                                                       : tr ("detail.channelList");
        juce::StringArray list;
        for (auto ch : d.allChannels()) list.add (juce::String (ch));
        row (label, list.joinIntoString (", "));
    }
    for (auto* p : ports)
        row (tr ("detail.port"), p->name);

    const auto physicalRows = [&] (const std::vector<int>& channels, ChannelDirection dir)
    {
        std::vector<std::pair<juce::String, juce::String>> rows;
        const auto dirName = dir == ChannelDirection::send ? juce::String ("OUT") : juce::String ("IN");
        for (const auto& io : SignalFlow::deviceIOs (channels, dir, data.physical))
        {
            rows.emplace_back (DeviceStyle::deviceTypeName (io.device.deviceType) + " (" + dirName + ")", io.device.name);
            const auto ioLabel = io.device.physicalRangeLabel (io.channels, dir);
            rows.emplace_back (tr ("detail.physicalIO") + " (" + dirName + ")", ioLabel.isEmpty() ? juce::String ("I/O ?") : ioLabel);
        }
        return rows;
    };
    const auto sendRows = physicalRows (d.sendChannels, ChannelDirection::send);
    const auto recvRows = physicalRows (d.recvChannels, ChannelDirection::recv);
    for (auto& [l, v] : sendRows) row (l, v);
    if (recvRows != sendRows)
        for (auto& [l, v] : recvRows) row (l, v);
    row (tr ("detail.type"), typeName (d.type));

    return y + sc (16);
}

int DeviceDetailPanel::nodeAt (juce::Point<int> p) const
{
    for (size_t i = 0; i < nodeBounds.size() && i < nodes.size(); ++i)
        if (nodeBounds[i].contains (p))
            return static_cast<int> (i);
    return -1;
}

void DeviceDetailPanel::Page::mouseMove (const juce::MouseEvent& e)
{
    const auto index = owner.nodeAt (e.getPosition());
    if (index == hovered)
        return;
    hovered = index;
    const auto clickable = index >= 0 && owner.nodes[static_cast<size_t> (index)].jumpChannel.has_value();
    setMouseCursor (clickable ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    setTooltip (clickable ? tr ("detail.jumpTooltip") : juce::String());
    repaint();
}

void DeviceDetailPanel::Page::mouseExit (const juce::MouseEvent&)
{
    hovered = -1;
    repaint();
}

void DeviceDetailPanel::Page::mouseUp (const juce::MouseEvent& e)
{
    const auto index = owner.nodeAt (e.getPosition());
    if (index < 0)
        return;
    if (auto ch = owner.nodes[static_cast<size_t> (index)].jumpChannel; ch && owner.data.jumpToChannel)
        owner.data.jumpToChannel (*ch);
}

} // namespace StudioPatchbay
