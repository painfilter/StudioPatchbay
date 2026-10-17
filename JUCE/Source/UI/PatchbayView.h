#pragma once

#include "ViewData.h"
#include "Model/PatchbayLayout.h"

namespace StudioPatchbay
{

// The Patchbay view of the Swift app: a legend, then per interface port its
// physical devices, each a block with its OUT row above its IN row (cells
// show the physical I/O number, the Cubase channel below), then the audio
// interfaces, then the Control Room buses. Cells are coloured by what uses
// them; hover shows what is on a cell, click selects (several devices on one
// channel: click again for the next). Rows longer than the page wrap.
class PatchbayView : public ContentView
{
public:
    explicit PatchbayView (const ViewData&);

    void refresh() override;
    juce::Component& getExportContent() override { return page; }

    // For the export: list under each block what is connected to it.
    void setShowConnections (bool);
    // The page area of a channel's OUT cell, to scroll to; empty if not shown.
    juce::Rectangle<int> channelBounds (int channel) const;
    void scrollToChannel (int channel);

private:
    struct Cell
    {
        juce::Rectangle<int> bounds;
        int channel;
        ChannelDirection direction;
        std::vector<ParsedDevice> devices;
        std::vector<ControlRoomBus> buses;
        std::optional<int> physicalIO;
    };

    class Page : public juce::Component
    {
    public:
        explicit Page (PatchbayView& o) : owner (o) {}
        void paint (juce::Graphics& g) override { owner.layout (&g); owner.paintTooltip (g); }
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        int hovered = -1;
    private:
        PatchbayView& owner;
    };

    // Draws the page (or, with g null, only measures it and records the
    // cells); returns the height.
    int layout (juce::Graphics*);
    void paintTooltip (juce::Graphics&);
    int cellAt (juce::Point<int>) const;

    Page page { *this };
    juce::TextButton setupButton;
    std::vector<Cell> cells;
    bool showConnections = false;
    juce::Rectangle<int> bannerButtonArea;
};

} // namespace StudioPatchbay
