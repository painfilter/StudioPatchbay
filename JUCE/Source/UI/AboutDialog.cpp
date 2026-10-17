#include "AboutDialog.h"
#include "Fonts.h"

namespace StudioPatchbay
{

AboutDialog::AboutDialog()
{
    setSize (360, 320);

    okButton.onClick = [this]
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (0);
    };
    addAndMakeVisible (okButton);

    licenseText.setReadOnly (true);
    licenseText.setMultiLine (true);
    licenseText.setScrollbarsShown (true);
    licenseText.setFont (Fonts::sans (11.0f, Fonts::Weight::regular));
    licenseText.setText (
        "StudioPatchbay is free software under the MIT licence.\n"
        "It uses the following software:\n\n"
        "JUCE Framework\n"
        "Copyright (c) Raw Material Software Limited\n"
        "Licensed under the AGPLv3 or a commercial JUCE licence\n\n"
        "IBM Plex Sans\n"
        "Copyright (c) IBM Corp.\n"
        "Licensed under the SIL Open Font License 1.1\n\n"
        "Catch2 (Tests)\n"
        "Licensed under the Boost Software License 1.0\n\n"
        "Classic Mac OS 9 / Platinum-inspired interface\n"
        "Copyright (c) Jan Nordus\n",
        false);
    addAndMakeVisible (licenseText);
}

void AboutDialog::setPalette (const StudioPatchbay::Design::Palette& newPalette)
{
    pal = newPalette;

    licenseText.setColour (juce::TextEditor::backgroundColourId, pal.field);
    licenseText.setColour (juce::TextEditor::textColourId, pal.ink);
    licenseText.setColour (juce::TextEditor::outlineColourId, pal.shadowEdge);

    repaint();
}

void AboutDialog::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);

    auto area = getLocalBounds().reduced (20, 16);

    // App icon placeholder (small diamond).
    auto iconArea = area.removeFromTop (30);
    g.setColour (pal.selection);
    auto cx = static_cast<float> (iconArea.getCentreX());
    auto cy = static_cast<float> (iconArea.getCentreY());
    juce::Path diamond;
    diamond.startNewSubPath (cx, cy - 12.0f);
    diamond.lineTo (cx + 12.0f, cy);
    diamond.lineTo (cx, cy + 12.0f);
    diamond.lineTo (cx - 12.0f, cy);
    diamond.closeSubPath();
    g.fillPath (diamond);

    area.removeFromTop (8);

    // App name.
    g.setFont (Fonts::sans (18.0f, Fonts::Weight::semiBold));
    g.setColour (pal.ink);
    g.drawText ("StudioPatchbay", area.removeFromTop (24),
                juce::Justification::centred);

    area.removeFromTop (4);

    // Version.
    g.setFont (Fonts::sans (12.0f, Fonts::Weight::regular));
    g.setColour (pal.ink.withAlpha (0.6f));
    g.drawText ("Version " + juce::JUCEApplication::getInstance()->getApplicationVersion(),
                area.removeFromTop (16), juce::Justification::centred);

    area.removeFromTop (2);

    // Author.
    g.setFont (Fonts::sans (11.0f, Fonts::Weight::regular));
    g.setColour (pal.ink.withAlpha (0.5f));
    g.drawText ("Jan Nordus", area.removeFromTop (14),
                juce::Justification::centred);

    area.removeFromTop (8);

    // Section header.
    g.setFont (Fonts::sans (10.0f, Fonts::Weight::semiBold));
    g.setColour (pal.ink.withAlpha (0.45f));
    g.drawText ("LICENSES", area.removeFromTop (16),
                juce::Justification::centredLeft);
}

void AboutDialog::resized()
{
    auto area = getLocalBounds().reduced (20, 16);
    area.removeFromTop (130); // space for painted content

    auto buttonArea = area.removeFromBottom (30);
    okButton.setBounds (buttonArea.withSizeKeepingCentre (80, 26));

    area.removeFromBottom (8);
    licenseText.setBounds (area);
}

void AboutDialog::show (const StudioPatchbay::Design::Palette& palette)
{
    auto* dialog = new AboutDialog();
    dialog->setPalette (palette);

    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned (dialog);
    opts.dialogTitle = "About StudioPatchbay";
    opts.dialogBackgroundColour = palette.windowPanel;
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar = false;
    opts.resizable = false;
    opts.launchAsync();
}

} // namespace StudioPatchbay
