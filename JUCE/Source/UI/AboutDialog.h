#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"

namespace StudioPatchbay
{

// OS9-styled About dialog showing application name, version,
// build info, and third-party license credits.
class AboutDialog : public juce::Component
{
public:
    AboutDialog();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);

    // Show as a modal dialog centred on screen.
    static void show (const StudioPatchbay::Design::Palette& pal);

private:
    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();

    juce::TextButton okButton { "OK" };
    juce::TextEditor licenseText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutDialog)
};

} // namespace StudioPatchbay
