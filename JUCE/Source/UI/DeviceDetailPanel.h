#pragma once

#include "ViewData.h"
#include "Model/SignalFlow.h"

namespace StudioPatchbay
{

// The detail panel of the Swift app, over the right edge of the views: the
// selected device's type, name and ports, its signal flow (nodes on a channel
// jump to it in the patchbay) and a table of details.
class DeviceDetailPanel : public juce::Component
{
public:
    static constexpr int panelWidth = 280;

    explicit DeviceDetailPanel (const ViewData&);

    void setStyle (const ViewStyle&);
    void refresh();   // after the selection or data changed

    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Page : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit Page (DeviceDetailPanel& o) : owner (o) {}
        void paint (juce::Graphics& g) override { owner.layout (&g); }
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        int hovered = -1;
    private:
        DeviceDetailPanel& owner;
    };

    int layout (juce::Graphics*);
    int nodeAt (juce::Point<int>) const;
    int sc (int v) const { return juce::roundToInt (v * style.uiScale); }
    float scf (float v) const { return static_cast<float> (v * style.uiScale); }

    const ViewData& data;
    ViewStyle style;
    std::vector<SignalFlow::Node> nodes;
    std::vector<juce::Rectangle<int>> nodeBounds;
    Page page { *this };
    juce::Viewport viewport;
    juce::TextButton closeButton;
};

} // namespace StudioPatchbay
