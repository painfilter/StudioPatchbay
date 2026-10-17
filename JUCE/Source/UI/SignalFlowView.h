#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Model/DataModel.h"
#include "StudioPatchbayTokens.h"

namespace StudioPatchbay
{

// Signal Flow diagram showing devices as nodes with port connections
// drawn as curved lines. Physical ports appear on the left (Send) and
// right (Recv), with devices in the centre. Connector-type tints colour
// the connection lines.
//
// Layout:   [Send Ports] ──── [Devices] ──── [Recv Ports]
//
// The view is scrollable via an internal viewport.
class SignalFlowView : public juce::Component
{
public:
    SignalFlowView();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setData (const ParsedDevices& devices, const std::vector<DetectedPort>& ports);

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);
    void setUiScale (double scale);

    // The whole scrolled content, for export.
    juce::Component& getExportContent() { return flowContent; }

private:
    int sc (int value) const;

    // Node rectangles for layout.
    struct DeviceNode
    {
        ParsedDevice device;
        juce::Rectangle<int> bounds;
    };

    struct PortNode
    {
        DetectedPort port;
        juce::Rectangle<int> bounds;
    };

    void rebuildLayout();
    void paintConnections (juce::Graphics& g);
    void paintNode (juce::Graphics& g, juce::Rectangle<int> bounds,
                    const juce::String& label, const juce::String& subtitle,
                    juce::Colour tint);

    // Get y-centre for a given channel number in a port column.
    int channelYInPort (int channel, ChannelDirection dir) const;
    int channelYInDevice (int channel, const DeviceNode& node, ChannelDirection dir) const;

    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;

    ParsedDevices currentDevices;
    std::vector<DetectedPort> currentPorts;

    std::vector<DeviceNode> deviceNodes;
    std::vector<PortNode> sendPortNodes;
    std::vector<PortNode> recvPortNodes;

    // Inner content for scrolling.
    class FlowContent : public juce::Component
    {
    public:
        SignalFlowView* owner = nullptr;
        void paint (juce::Graphics& g) override;
    };

    FlowContent flowContent;
    juce::Viewport viewport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalFlowView)
};

} // namespace StudioPatchbay
