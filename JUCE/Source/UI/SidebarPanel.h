#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "AppState.h"

namespace StudioPatchbay
{

// Left sidebar panel showing detected Cubase installations as a selectable list.
// Painted in the OS9 Platinum style with inset field background and
// row highlighting in the palette selection colour.
class SidebarPanel : public juce::Component,
                     public juce::ListBoxModel,
                     private juce::ChangeListener
{
public:
    explicit SidebarPanel (AppState& state);
    ~SidebarPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setPalette (const StudioPatchbay::Design::Palette& newPalette);
    void setUiScale (double scale);

    // --- ListBoxModel ---
    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics& g,
                           int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent& e) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    int sc (int value) const;

    AppState& appState;
    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    double uiScale = 1.0;

    juce::ListBox listBox { "Installations" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SidebarPanel)
};

} // namespace StudioPatchbay
