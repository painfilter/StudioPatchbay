// Generated file. Do not edit by hand: changes are overwritten when the
// file is regenerated.

#pragma once

#include "PlatinumLookAndFeel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace StudioPatchbay::Design
{

class DesktopTile : public juce::Component
{
public:
    using IconPainter = std::function<void (juce::Graphics&, juce::Rectangle<int>, bool)>;

    DesktopTile (juce::String title, juce::String detail, IconPainter, std::function<void()> onClick);

    void setDarkMode (bool);
    void setUiScale (double);
    void setSelected (bool);
    void setText (juce::String title, juce::String detail);
    [[nodiscard]] juce::Rectangle<int> getIconBoundsInParent() const;

    std::function<void()> onPopupMenu;

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    int scaled (int) const;
    juce::Rectangle<int> iconBounds() const;

    juce::String title;
    juce::String detail;
    IconPainter paintIcon;
    std::function<void()> click;
    bool darkMode = false;
    bool hovered = false;
    bool selected = false;
    double uiScale = 1.0;
};

} // namespace StudioPatchbay::Design
