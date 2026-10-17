#pragma once

#include "ViewData.h"

namespace StudioPatchbay
{

// The Control Room view of the Swift app: a card per monitor bus with its
// path from Cubase's Control Room to each channel's converter and physical
// output, or "direct hardware output" when no converter is involved.
class ControlRoomView : public ContentView
{
public:
    explicit ControlRoomView (const ViewData&);

    void refresh() override;
    juce::Component& getExportContent() override { return page; }

private:
    class Page : public juce::Component
    {
    public:
        explicit Page (ControlRoomView& o) : owner (o) {}
        void paint (juce::Graphics& g) override { owner.layout (&g); }
    private:
        ControlRoomView& owner;
    };

    int layout (juce::Graphics*);
    int busCard (juce::Graphics*, const ControlRoomBus&, juce::Rectangle<int>) const;
    juce::StringArray routes (const ControlRoomBus&) const;

    Page page { *this };
};

} // namespace StudioPatchbay
