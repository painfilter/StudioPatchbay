#pragma once

#include "ViewData.h"

namespace StudioPatchbay
{

// The Cards view of the Swift app: Instruments, External FX and FX Sends
// (synth routing), each a grid of cards (200–360 px wide at 100 %). A card
// shows name, type, ports, format, its channel rows, the physical devices
// on its channels, its MIDI device and a warning when send and return
// differ. Click selects (and opens the detail panel); again deselects.
class CardsView : public ContentView
{
public:
    explicit CardsView (const ViewData&);

    void refresh() override;
    juce::Component& getExportContent() override { return page; }

private:
    struct Placed
    {
        ParsedDevice device;
        juce::Rectangle<int> bounds;
    };
    struct Section
    {
        juce::String title;
        juce::Colour colour;
        int count;
        int y;
    };

    class Page : public juce::Component
    {
    public:
        explicit Page (CardsView& o) : owner (o) {}
        void paint (juce::Graphics&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        int hovered = -1;
    private:
        int cardAt (juce::Point<int>) const;
        CardsView& owner;
    };

    // Draws a card into `area` (or only measures it when g is null) and
    // returns its height.
    int card (juce::Graphics*, const ParsedDevice&, juce::Rectangle<int> area, bool hover) const;

    Page page { *this };
    std::vector<Placed> cards;
    std::vector<Section> sections;
};

} // namespace StudioPatchbay
