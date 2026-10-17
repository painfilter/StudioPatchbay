#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Model/DataModel.h"
#include "StudioPatchbayTokens.h"

namespace StudioPatchbay
{

// Patchbay matrix grid showing channels (rows) vs. devices (columns).
// Row headers display port families and channel ranges from Port Setup.
// Column headers display device names from External Plugins.
// Cells are filled where a device uses that channel, colour-coded by
// connector type via StudioPatchbayTokens::ConnectorTint.
//
// The entire grid lives inside a viewport for scrolling when the
// matrix is larger than the visible area.
class PatchbayGridView : public juce::Component
{
public:
    PatchbayGridView();

    void paint (juce::Graphics& g) override;
    void resized() override;

    // Update the displayed data (called when AppState changes).
    void setData (const ParsedDevices& devices, const std::vector<DetectedPort>& ports);

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);
    void setUiScale (double scale);

    // The whole scrolled content, for export.
    juce::Component& getExportContent() { return gridContent; }

    // Get the connector tint for a port family string.
    // Public so other views (SignalFlowView) can reuse it.
    static juce::Colour connectorTintForFamily (const juce::String& family);

private:
    int sc (int value) const;

    // Determines the max channel number across all devices and ports.
    int maxChannel() const;

    // Inner content component for the scrollable grid.
    class GridContent : public juce::Component
    {
    public:
        void paint (juce::Graphics& g) override;

        StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
        double uiScale = 1.0;

        ParsedDevices devices;
        std::vector<DetectedPort> ports;
        std::map<int, juce::String> sendMap;
        std::map<int, juce::String> recvMap;
        int totalChannels = 0;

        int sc (int v) const { return juce::roundToInt (v * uiScale); }
    };

    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;

    ParsedDevices currentDevices;
    std::vector<DetectedPort> currentPorts;

    GridContent gridContent;
    juce::Viewport viewport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PatchbayGridView)
};

} // namespace StudioPatchbay
