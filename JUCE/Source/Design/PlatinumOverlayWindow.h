// Generated file. Do not edit by hand: changes are overwritten when the
// file is regenerated.

#pragma once

#include "PlatinumLookAndFeel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

namespace StudioPatchbay::Design
{

class OverlayWindow final : public juce::Component
{
public:
    OverlayWindow (juce::String title, std::unique_ptr<juce::Component> content,
                      int contentWidth, int contentHeight);

    std::function<void()> onClose;
    void setUiScale (double newScale);
    void setDarkMode (bool);
    void setCloseTooltip (const juce::String&);
    juce::Component* getContent() const noexcept { return content.get(); }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    int scaled (int v) const { return juce::roundToInt (v * uiScale); }
    int edge (int t = 1) const { return juce::jmax (1, scaled (t)); }
    juce::Rectangle<int> dialogBounds() const;
    juce::Rectangle<int> previewBounds() const;
    juce::Rectangle<int> titleBar (juce::Rectangle<int> dialog) const;
    void drawDragOutline (juce::Graphics&, juce::Rectangle<int>) const;

    juce::String title;
    std::unique_ptr<juce::Component> content;
    int contentWidth, contentHeight;
    double uiScale = 1.0;
    bool darkMode = false;
    juce::TextButton closeBox;
    juce::Point<double> posLogical, previewLogical;
    juce::Point<int> dragOffset;
    bool hasCustomPos = false, dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverlayWindow)
};

std::vector<juce::Rectangle<int>> drawTabStrip (juce::Graphics&, juce::Rectangle<int> strip,
                                                const juce::StringArray& names, int activeIndex,
                                                const Palette&, double uiScale);

} // namespace StudioPatchbay::Design
