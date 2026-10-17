#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "AppContentComponent.h"

namespace StudioPatchbay
{

// Main application window with OS9 title bar, keyboard shortcuts,
// and menu callbacks. Implements KeyListener for Cmd+R (Reload),
// Cmd+1-4 (view switching), and Cmd+Q (quit).
class MainWindow : public juce::DocumentWindow,
                   public juce::KeyListener
{
public:
    MainWindow (juce::String name);
    ~MainWindow() override;

    void closeButtonPressed() override;

    // KeyListener.
    using juce::DocumentWindow::keyPressed;
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;

    AppContentComponent& getContent() { return *content; }

private:
    void showFileMenu();
    void showViewMenu();
    void showOptionsMenu();
    void showHelpMenu();
    void exportActiveView (Export::Format format);
    void applyTheme (bool dark);
    void applyZoom (double scale);

    StudioPatchbay::Design::PlatinumLookAndFeel os9Laf;
    AppContentComponent* content = nullptr;
    double uiScale = 1.0;
    std::unique_ptr<juce::FileChooser> exportChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

} // namespace StudioPatchbay
