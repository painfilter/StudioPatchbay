#include "PatchbayGridView.h"
#include "Fonts.h"

namespace StudioPatchbay
{

// ============================================================================
// Layout constants (logical pixels, scaled by uiScale)
// ============================================================================

static constexpr int kRowHeaderW     = 140; // port family + channel range
static constexpr int kColHeaderH     = 80;  // rotated device names
static constexpr int kCellW          = 28;  // cell width per device column
static constexpr int kCellH          = 18;  // cell height per channel row
static constexpr int kDirectionLabelH = 20; // SEND / RECV section header

// ============================================================================
// Connector tint mapping
// ============================================================================

juce::Colour PatchbayGridView::connectorTintForFamily (const juce::String& family)
{
    auto f = family.toLowerCase();
    if (f.contains ("madi"))        return Tokens::ConnectorTint::madi();
    if (f.contains ("adat"))        return Tokens::ConnectorTint::adat();
    if (f.contains ("aes"))         return Tokens::ConnectorTint::aes();
    if (f.contains ("spdif")
        || f.contains ("s/pdif"))   return Tokens::ConnectorTint::spdif();
    if (f.contains ("dante"))       return Tokens::ConnectorTint::dante();
    if (f.contains ("usb"))         return Tokens::ConnectorTint::usb();
    if (f.contains ("thunder"))     return Tokens::ConnectorTint::thunderbolt();
    if (f.contains ("mic")
        || f.contains ("preamp"))   return Tokens::ConnectorTint::analogMic();
    if (f.contains ("line")
        || f.contains ("analog"))   return Tokens::ConnectorTint::analogLine();
    return juce::Colours::transparentBlack;
}

// ============================================================================
// PatchbayGridView
// ============================================================================

PatchbayGridView::PatchbayGridView()
{
    viewport.setViewedComponent (&gridContent, false);
    viewport.setScrollBarsShown (true, true);
    addAndMakeVisible (viewport);
}

int PatchbayGridView::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

void PatchbayGridView::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;
    gridContent.pal = pal;
    gridContent.repaint();
    repaint();
}

void PatchbayGridView::setUiScale (double scale)
{
    uiScale = scale;
    gridContent.uiScale = scale;
    resized();
}

int PatchbayGridView::maxChannel() const
{
    int maxCh = 0;
    for (auto& d : currentDevices.all())
    {
        for (auto ch : d.sendChannels) maxCh = juce::jmax (maxCh, ch);
        for (auto ch : d.recvChannels) maxCh = juce::jmax (maxCh, ch);
    }
    for (auto& p : currentPorts)
        maxCh = juce::jmax (maxCh, p.channelEnd);
    return maxCh;
}

void PatchbayGridView::setData (const ParsedDevices& devices,
                                 const std::vector<DetectedPort>& ports)
{
    currentDevices = devices;
    currentPorts = ports;

    gridContent.devices = devices;
    gridContent.ports = ports;
    gridContent.sendMap = devices.buildChannelMap (ChannelDirection::send);
    gridContent.recvMap = devices.buildChannelMap (ChannelDirection::recv);
    gridContent.totalChannels = maxChannel();

    resized();
    gridContent.repaint();
}

void PatchbayGridView::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);

    if (currentDevices.all().empty() && currentPorts.empty())
    {
        g.setFont (Fonts::sans (static_cast<float> (sc (13)), Fonts::Weight::regular));
        g.setColour (pal.ink.withAlpha (0.4f));
        g.drawText (juce::String (juce::CharPointer_UTF8 ("No port or device data available \xe2\x80\x93 select a Cubase installation.")),
                    getLocalBounds().reduced (sc (20)),
                    juce::Justification::centred);
    }
}

void PatchbayGridView::resized()
{
    viewport.setBounds (getLocalBounds());

    auto allDevices = currentDevices.all();
    int numDevices = static_cast<int> (allDevices.size());
    int numChannels = gridContent.totalChannels;

    int contentW = sc (kRowHeaderW) + numDevices * sc (kCellW) + sc (20);
    int contentH = sc (kColHeaderH) + sc (kDirectionLabelH) * 2
                   + numChannels * sc (kCellH) * 2 + sc (20);

    contentW = juce::jmax (contentW, viewport.getMaximumVisibleWidth());
    contentH = juce::jmax (contentH, viewport.getMaximumVisibleHeight());

    gridContent.setSize (contentW, contentH);
}

// ============================================================================
// GridContent::paint  – the actual matrix rendering
// ============================================================================

void PatchbayGridView::GridContent::paint (juce::Graphics& g)
{
    if (devices.all().empty())
        return;

    const auto edge = juce::jmax (1, sc (1));
    auto allDevices = devices.all();
    int numDevices = static_cast<int> (allDevices.size());

    const auto headerFont = Fonts::sans (static_cast<float> (sc (10)), Fonts::Weight::semiBold);
    const auto cellFont   = Fonts::sans (static_cast<float> (sc (9)), Fonts::Weight::regular);
    const auto labelFont  = Fonts::sans (static_cast<float> (sc (11)), Fonts::Weight::semiBold);

    g.fillAll (pal.windowPanel);

    const int rhW = sc (kRowHeaderW);
    const int chH = sc (kColHeaderH);
    const int cW  = sc (kCellW);
    const int cH  = sc (kCellH);
    const int dlH = sc (kDirectionLabelH);

    // --- Column headers (device names, rotated) ---
    g.setFont (headerFont);
    for (int col = 0; col < numDevices; ++col)
    {
        auto colRect = juce::Rectangle<int> (rhW + col * cW, 0, cW, chH);

        // Alternating column tint.
        if (col % 2 == 0)
        {
            g.setColour (pal.controlSurface.withAlpha (0.3f));
            g.fillRect (colRect);
        }

        // Rotated text.
        g.saveState();
        g.setColour (pal.ink);
        auto transform = juce::AffineTransform::rotation (
            -juce::MathConstants<float>::halfPi,
            static_cast<float> (colRect.getCentreX()),
            static_cast<float> (colRect.getCentreY()));
        g.addTransform (transform);

        auto textRect = juce::Rectangle<int> (
            colRect.getCentreX() - chH / 2,
            colRect.getCentreY() - cW / 2,
            chH, cW);
        g.drawText (allDevices[static_cast<size_t> (col)].name,
                    textRect, juce::Justification::centredLeft, true);
        g.restoreState();
    }

    // --- Helper to draw one direction section ---
    auto drawSection = [&] (const juce::String& dirLabel,
                            const std::map<int, juce::String>& channelMap,
                            int yStart)
    {
        // Section label.
        g.setFont (labelFont);
        g.setColour (pal.ink.withAlpha (0.6f));
        g.drawText (dirLabel,
                    juce::Rectangle<int> (sc (6), yStart, rhW - sc (12), dlH),
                    juce::Justification::centredLeft);

        // Separator under label.
        g.setColour (pal.shadowEdge);
        g.fillRect (sc (4), yStart + dlH - edge, rhW + numDevices * cW, edge);

        int y = yStart + dlH;

        for (int ch = 1; ch <= totalChannels; ++ch)
        {
            auto rowRect = juce::Rectangle<int> (0, y, rhW + numDevices * cW, cH);

            // Row background – find port family for this channel.
            juce::Colour rowTint = juce::Colours::transparentBlack;
            for (auto& port : ports)
            {
                bool dirMatch = (dirLabel == "SEND" && port.direction == ChannelDirection::send)
                             || (dirLabel == "RECV" && port.direction == ChannelDirection::recv);
                if (dirMatch && ch >= port.channelStart && ch <= port.channelEnd)
                {
                    rowTint = PatchbayGridView::connectorTintForFamily (port.family);
                    break;
                }
            }

            // Alternating row tint.
            if (ch % 2 == 0)
            {
                g.setColour (pal.controlSurface.withAlpha (0.15f));
                g.fillRect (rowRect);
            }

            // Apply connector tint.
            if (! rowTint.isTransparent())
            {
                g.setColour (rowTint);
                g.fillRect (rowRect);
            }

            // Row header (channel number).
            g.setFont (cellFont);
            g.setColour (pal.ink.withAlpha (0.7f));
            g.drawText (juce::String (ch),
                        juce::Rectangle<int> (sc (4), y, sc (24), cH),
                        juce::Justification::centredRight);

            // Port family label (only on first channel of a port range).
            for (auto& port : ports)
            {
                bool dirMatch = (dirLabel == "SEND" && port.direction == ChannelDirection::send)
                             || (dirLabel == "RECV" && port.direction == ChannelDirection::recv);
                if (dirMatch && ch == port.channelStart)
                {
                    g.setColour (pal.ink.withAlpha (0.5f));
                    g.drawText (port.family,
                                juce::Rectangle<int> (sc (30), y, rhW - sc (34), cH),
                                juce::Justification::centredLeft, true);
                }
            }

            // Grid cells.
            for (int col = 0; col < numDevices; ++col)
            {
                auto cellRect = juce::Rectangle<int> (rhW + col * cW, y, cW, cH);

                // Cell border.
                g.setColour (pal.shadowEdge.withAlpha (0.2f));
                g.drawRect (cellRect, edge);

                // Check if this device uses this channel.
                auto it = channelMap.find (ch);
                if (it != channelMap.end()
                    && it->second == allDevices[static_cast<size_t> (col)].name)
                {
                    // Filled cell – connection exists.
                    auto fillRect = cellRect.reduced (sc (3), sc (2));
                    g.setColour (pal.selection.withAlpha (0.7f));
                    g.fillRect (fillRect);

                    // Small diamond marker.
                    g.setColour (pal.ink);
                    auto cx = static_cast<float> (cellRect.getCentreX());
                    auto cy = static_cast<float> (cellRect.getCentreY());
                    auto ds = static_cast<float> (sc (3));
                    juce::Path diamond;
                    diamond.startNewSubPath (cx, cy - ds);
                    diamond.lineTo (cx + ds, cy);
                    diamond.lineTo (cx, cy + ds);
                    diamond.lineTo (cx - ds, cy);
                    diamond.closeSubPath();
                    g.fillPath (diamond);
                }
            }

            // Row grid line.
            g.setColour (pal.shadowEdge.withAlpha (0.1f));
            g.fillRect (rhW, y + cH - edge, numDevices * cW, edge);

            y += cH;
        }

        return y;
    };

    // Draw SEND section.
    int sendEnd = drawSection ("SEND", sendMap, chH);

    // Gap between sections.
    int recvStart = sendEnd + sc (8);

    // Draw RECV section.
    drawSection ("RECV", recvMap, recvStart);

    // Column grid lines.
    g.setColour (pal.shadowEdge.withAlpha (0.15f));
    for (int col = 0; col <= numDevices; ++col)
    {
        int x = rhW + col * cW;
        g.fillRect (x, chH, edge, getHeight() - chH);
    }
}

} // namespace StudioPatchbay
