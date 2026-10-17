#include "SignalFlowView.h"
#include "Fonts.h"
#include "UI/PatchbayGridView.h" // for connectorTintForFamily

namespace StudioPatchbay
{

// ============================================================================
// Layout constants
// ============================================================================

static constexpr int kNodeW       = 160;
static constexpr int kNodeH       = 40;
static constexpr int kNodeGap     = 8;
static constexpr int kColumnGap   = 120; // gap between port and device columns
static constexpr int kTopPadding  = 20;
static constexpr int kLeftPadding = 16;

// ============================================================================
// SignalFlowView
// ============================================================================

SignalFlowView::SignalFlowView()
{
    flowContent.owner = this;
    viewport.setViewedComponent (&flowContent, false);
    viewport.setScrollBarsShown (true, true);
    addAndMakeVisible (viewport);
}

int SignalFlowView::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

void SignalFlowView::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;
    flowContent.repaint();
    repaint();
}

void SignalFlowView::setUiScale (double scale)
{
    uiScale = scale;
    rebuildLayout();
    resized();
}

void SignalFlowView::setData (const ParsedDevices& devices,
                               const std::vector<DetectedPort>& ports)
{
    currentDevices = devices;
    currentPorts = ports;
    rebuildLayout();
    resized();
}

// ============================================================================
// Layout
// ============================================================================

void SignalFlowView::rebuildLayout()
{
    deviceNodes.clear();
    sendPortNodes.clear();
    recvPortNodes.clear();

    const int nw = sc (kNodeW);
    const int nh = sc (kNodeH);
    const int ng = sc (kNodeGap);
    const int cg = sc (kColumnGap);
    const int lp = sc (kLeftPadding);
    const int tp = sc (kTopPadding);

    // --- Send ports (left column) ---
    int y = tp;
    for (auto& port : currentPorts)
    {
        if (port.direction == ChannelDirection::send)
        {
            PortNode pn;
            pn.port = port;
            pn.bounds = { lp, y, nw, nh };
            sendPortNodes.push_back (pn);
            y += nh + ng;
        }
    }

    // --- Devices (centre column) ---
    int deviceX = lp + nw + cg;
    y = tp;
    auto allDevices = currentDevices.all();
    for (auto& dev : allDevices)
    {
        DeviceNode dn;
        dn.device = dev;
        dn.bounds = { deviceX, y, nw, nh };
        deviceNodes.push_back (dn);
        y += nh + ng;
    }

    // --- Recv ports (right column) ---
    int recvX = deviceX + nw + cg;
    y = tp;
    for (auto& port : currentPorts)
    {
        if (port.direction == ChannelDirection::recv)
        {
            PortNode pn;
            pn.port = port;
            pn.bounds = { recvX, y, nw, nh };
            recvPortNodes.push_back (pn);
            y += nh + ng;
        }
    }
}

void SignalFlowView::resized()
{
    viewport.setBounds (getLocalBounds());

    // Compute content size from all nodes.
    int maxX = sc (kLeftPadding);
    int maxY = sc (kTopPadding);
    auto updateMax = [&] (const juce::Rectangle<int>& r)
    {
        maxX = juce::jmax (maxX, r.getRight());
        maxY = juce::jmax (maxY, r.getBottom());
    };

    for (auto& n : sendPortNodes)  updateMax (n.bounds);
    for (auto& n : deviceNodes)    updateMax (n.bounds);
    for (auto& n : recvPortNodes)  updateMax (n.bounds);

    maxX += sc (kLeftPadding);
    maxY += sc (kTopPadding);

    flowContent.setSize (juce::jmax (maxX, viewport.getMaximumVisibleWidth()),
                         juce::jmax (maxY, viewport.getMaximumVisibleHeight()));
}

// ============================================================================
// Paint
// ============================================================================

void SignalFlowView::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);

    if (currentDevices.all().empty())
    {
        g.setFont (Fonts::sans (static_cast<float> (sc (13)), Fonts::Weight::regular));
        g.setColour (pal.ink.withAlpha (0.4f));
        g.drawText ("No signal flow data available.",
                    getLocalBounds().reduced (sc (20)),
                    juce::Justification::centred);
    }
}

void SignalFlowView::FlowContent::paint (juce::Graphics& g)
{
    if (owner == nullptr) return;

    g.fillAll (owner->pal.windowPanel);

    // Column headers.
    auto headerFont = Fonts::sans (static_cast<float> (owner->sc (11)), Fonts::Weight::semiBold);
    g.setFont (headerFont);
    g.setColour (owner->pal.ink.withAlpha (0.5f));

    if (! owner->sendPortNodes.empty())
        g.drawText ("SEND PORTS", owner->sendPortNodes[0].bounds.withHeight (0).translated (0, -owner->sc (4)),
                    juce::Justification::bottomLeft);
    if (! owner->deviceNodes.empty())
        g.drawText ("DEVICES", owner->deviceNodes[0].bounds.withHeight (0).translated (0, -owner->sc (4)),
                    juce::Justification::bottomLeft);
    if (! owner->recvPortNodes.empty())
        g.drawText ("RECV PORTS", owner->recvPortNodes[0].bounds.withHeight (0).translated (0, -owner->sc (4)),
                    juce::Justification::bottomLeft);

    // Draw connections first (behind nodes).
    owner->paintConnections (g);

    // Draw port nodes.
    for (auto& pn : owner->sendPortNodes)
    {
        auto tint = PatchbayGridView::connectorTintForFamily (pn.port.family);
        auto subtitle = juce::String (pn.port.channelStart) + "-" + juce::String (pn.port.channelEnd);
        owner->paintNode (g, pn.bounds, pn.port.family, subtitle, tint);
    }

    for (auto& pn : owner->recvPortNodes)
    {
        auto tint = PatchbayGridView::connectorTintForFamily (pn.port.family);
        auto subtitle = juce::String (pn.port.channelStart) + "-" + juce::String (pn.port.channelEnd);
        owner->paintNode (g, pn.bounds, pn.port.family, subtitle, tint);
    }

    // Draw device nodes.
    for (auto& dn : owner->deviceNodes)
    {
        juce::Colour tint;
        switch (dn.device.type)
        {
            case ParsedDevice::Type::instrument: tint = Tokens::DeviceTint::instrument(); break;
            case ParsedDevice::Type::fx:         tint = Tokens::DeviceTint::fx(); break;
            case ParsedDevice::Type::sendOnly:   tint = Tokens::DeviceTint::sendOnly(); break;
        }
        auto subtitle = juce::String (dn.device.channelCount()) + " ch";
        if (dn.device.isStereo()) subtitle = "Stereo";
        if (dn.device.isMono())   subtitle = "Mono";
        owner->paintNode (g, dn.bounds, dn.device.name, subtitle, tint);
    }
}

void SignalFlowView::paintNode (juce::Graphics& g, juce::Rectangle<int> bounds,
                                 const juce::String& label, const juce::String& subtitle,
                                 juce::Colour tint)
{
    const auto edge = juce::jmax (1, sc (1));

    // Background with tint.
    g.setColour (pal.controlSurface.overlaidWith (tint));
    g.fillRect (bounds);

    // OS9 bevel.
    g.setColour (pal.highlightEdge);
    g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), edge);
    g.fillRect (bounds.getX(), bounds.getY(), edge, bounds.getHeight());
    g.setColour (pal.shadowEdge);
    g.fillRect (bounds.getX(), bounds.getBottom() - edge, bounds.getWidth(), edge);
    g.fillRect (bounds.getRight() - edge, bounds.getY(), edge, bounds.getHeight());

    // Label.
    auto content = bounds.reduced (sc (6), sc (4));
    g.setFont (Fonts::sans (static_cast<float> (sc (11)), Fonts::Weight::semiBold));
    g.setColour (pal.ink);
    g.drawText (label, content.removeFromTop (sc (16)), juce::Justification::centredLeft, true);

    // Subtitle.
    g.setFont (Fonts::sans (static_cast<float> (sc (9)), Fonts::Weight::regular));
    g.setColour (pal.ink.withAlpha (0.5f));
    g.drawText (subtitle, content, juce::Justification::centredLeft);
}

void SignalFlowView::paintConnections (juce::Graphics& g)
{
    const auto sendMap = currentDevices.buildChannelMap (ChannelDirection::send);
    const auto recvMap = currentDevices.buildChannelMap (ChannelDirection::recv);

    // For each device, find which send ports connect to it, and draw curves.
    for (auto& dn : deviceNodes)
    {
        // Send connections: port → device (left to centre).
        for (auto ch : dn.device.sendChannels)
        {
            for (auto& pn : sendPortNodes)
            {
                if (ch >= pn.port.channelStart && ch <= pn.port.channelEnd)
                {
                    auto tint = PatchbayGridView::connectorTintForFamily (pn.port.family);
                    auto colour = tint.isTransparent() ? pal.ink.withAlpha (0.25f)
                                                       : tint.withAlpha (0.6f);
                    g.setColour (colour);

                    float x1 = static_cast<float> (pn.bounds.getRight());
                    float y1 = static_cast<float> (pn.bounds.getCentreY());
                    float x2 = static_cast<float> (dn.bounds.getX());
                    float y2 = static_cast<float> (dn.bounds.getCentreY());
                    float cpOffset = (x2 - x1) * 0.4f;

                    juce::Path curve;
                    curve.startNewSubPath (x1, y1);
                    curve.cubicTo (x1 + cpOffset, y1, x2 - cpOffset, y2, x2, y2);
                    g.strokePath (curve, juce::PathStrokeType (static_cast<float> (sc (1))));
                    break;
                }
            }
        }

        // Recv connections: device → port (centre to right).
        for (auto ch : dn.device.recvChannels)
        {
            for (auto& pn : recvPortNodes)
            {
                if (ch >= pn.port.channelStart && ch <= pn.port.channelEnd)
                {
                    auto tint = PatchbayGridView::connectorTintForFamily (pn.port.family);
                    auto colour = tint.isTransparent() ? pal.ink.withAlpha (0.25f)
                                                       : tint.withAlpha (0.6f);
                    g.setColour (colour);

                    float x1 = static_cast<float> (dn.bounds.getRight());
                    float y1 = static_cast<float> (dn.bounds.getCentreY());
                    float x2 = static_cast<float> (pn.bounds.getX());
                    float y2 = static_cast<float> (pn.bounds.getCentreY());
                    float cpOffset = (x2 - x1) * 0.4f;

                    juce::Path curve;
                    curve.startNewSubPath (x1, y1);
                    curve.cubicTo (x1 + cpOffset, y1, x2 - cpOffset, y2, x2, y2);
                    g.strokePath (curve, juce::PathStrokeType (static_cast<float> (sc (1))));
                    break;
                }
            }
        }
    }
}

} // namespace StudioPatchbay
