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

class AboutDialog : public juce::Component
{
public:
    struct Content
    {
        juce::String title;
        juce::String productName;
        juce::String tagline;
        juce::String versionLine;
        juce::String architectureLine;
        juce::String publisher;
        juce::String copyright;
        juce::String websiteLabel;
        juce::URL websiteUrl;
        juce::String emailLabel;
        juce::URL emailUrl;
        juce::String okText { "OK" };
        juce::String closeTooltip;
        juce::String licensingLine;
    };

    AboutDialog (std::function<void()> closeCallback,
                    std::unique_ptr<juce::Drawable> lightLogo,
                    std::unique_ptr<juce::Drawable> darkLogo);

    void setContent (Content);
    [[nodiscard]] const Content& getContent() const noexcept { return content; }
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
    int logicalDialogHeight() const;
    juce::Rectangle<int> dialogBounds() const;
    juce::Rectangle<int> previewDialogBounds() const;
    void drawDragOutline (juce::Graphics&, juce::Rectangle<int>) const;
    void applyAppearance();
    void closeDialog();

    std::function<void()> close;
    std::unique_ptr<juce::Drawable> lightLogo;
    std::unique_ptr<juce::Drawable> darkLogo;
    Content content;
    juce::TextButton closeButton;
    juce::TextButton okButton;
    juce::HyperlinkButton websiteButton;
    juce::HyperlinkButton emailButton;
    juce::Label contactSeparator;
    juce::Point<double> dialogPositionLogical;
    juce::Point<double> previewPositionLogical;
    juce::Point<int> dragOffset;
    bool darkMode = false;
    bool hasCustomDialogPosition = false;
    bool draggingWindow = false;
    double uiScale = 1.0;
};

} // namespace StudioPatchbay::Design
