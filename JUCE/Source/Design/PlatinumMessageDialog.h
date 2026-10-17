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

class MessageDialog final : public juce::Component
{
public:
    explicit MessageDialog (std::function<void()> closeCallback);

    void setContent (juce::String title, juce::String message,
                     juce::String okText, juce::String closeTooltip);
    void setDarkMode (bool);
    void setUiScale (double);

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    int scaled (int) const;
    int edge (int logicalThickness = 1) const;
    juce::Rectangle<int> dialogBounds() const;
    juce::Rectangle<int> previewDialogBounds() const;
    void drawDragOutline (juce::Graphics&, juce::Rectangle<int>) const;
    void closeDialog();

    std::function<void()> close;
    juce::String title;
    juce::String message;
    juce::TextButton closeButton;
    juce::TextButton okButton;
    juce::Point<double> dialogPositionLogical;
    juce::Point<double> previewPositionLogical;
    juce::Point<int> dragOffset;
    bool darkMode = false;
    bool hasCustomDialogPosition = false;
    bool draggingWindow = false;
    double uiScale = 1.0;
};

} // namespace StudioPatchbay::Design
