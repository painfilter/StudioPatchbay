#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Fonts.h"

namespace StudioPatchbay
{

// The content of a window opened on the desktop (Design::OverlayWindow). It
// is laid out in logical pixels; the window scales it with the zoom.
class OverlayContent : public juce::Component
{
public:
    virtual void setPalette (const Design::Palette& p) { pal = p; repaint(); }

    // The close box or Escape. Content with unsaved work asks first.
    virtual void closeRequested() { close(); }

    // Set by the host: closes the window.
    std::function<void()> close = [] {};

protected:
    Design::Palette pal = Design::lightPalette();
};

// What content can open on the desktop: more windows, a question, a message.
class DesktopWindows
{
public:
    virtual ~DesktopWindows() = default;
    virtual OverlayContent& openWindow (const juce::String& title, std::unique_ptr<OverlayContent>, int w, int h) = 0;
    virtual void confirm (const juce::String& headline, const juce::String& detail,
                          const juce::String& actionText, std::function<void()> action) = 0;
    virtual void showMessage (const juce::String& title, const juce::String& message) = 0;
};

// A question with a cancel button and one action, e.g. "Delete this device?".
class ConfirmContent : public OverlayContent
{
public:
    static constexpr int width = 440, height = 160;

    ConfirmContent (juce::String headline, juce::String detail, juce::String cancelText,
                    juce::String actionText, std::function<void()> action)
        : heading (std::move (headline)), text (std::move (detail)), onAction (std::move (action))
    {
        cancelButton.setButtonText (cancelText);
        actionButton.setButtonText (actionText);
        cancelButton.onClick = [this] { close(); };
        actionButton.onClick = [this]
        {
            auto run = onAction;   // closing deletes this
            close();
            if (run) run();
        };
        addAndMakeVisible (cancelButton);
        addAndMakeVisible (actionButton);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (pal.windowPanel);
        auto area = getLocalBounds().reduced (18, 14).withTrimmedBottom (40);
        g.setColour (pal.ink);
        g.setFont (Fonts::sans (13.0f, Fonts::Weight::semiBold));
        g.drawFittedText (heading, area.removeFromTop (20), juce::Justification::centredLeft, 1);
        area.removeFromTop (6);
        g.setFont (Fonts::sans (12.0f, Fonts::Weight::regular));
        g.drawFittedText (text, area, juce::Justification::topLeft, 4);
    }

    void resized() override
    {
        auto row = getLocalBounds().reduced (18, 14).removeFromBottom (24);
        actionButton.setBounds (row.removeFromRight (120));
        row.removeFromRight (8);
        cancelButton.setBounds (row.removeFromRight (100));
    }

private:
    juce::String heading, text;
    std::function<void()> onAction;
    juce::TextButton cancelButton, actionButton;
};

} // namespace StudioPatchbay
