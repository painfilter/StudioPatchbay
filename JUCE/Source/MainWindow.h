#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "AppContentComponent.h"
#include "Model/AppSettings.h"

namespace StudioPatchbay
{

// Main application window with OS9 title bar, keyboard shortcuts,
// and menu callbacks. Implements KeyListener for Cmd+R (Reload),
// Cmd+1-4 (view switching), and Cmd+Q (quit).
class MainWindow : public juce::DocumentWindow,
                   public juce::KeyListener,
                   private DesktopActions
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
    // DesktopActions; the menus use the same commands.
    ViewSwitcher::View currentView() const override;
    void exportView (Export::Format, ViewSwitcher::View) override;
    void reload() override;
    void showAbout() override;
    void forgetRecentExport (const juce::File&) override;

    void saveSettings();
    void applyTheme (bool dark);
    void applyZoom (double scale);

    StudioPatchbay::Design::PlatinumLookAndFeel os9Laf;
    AppContentComponent* content = nullptr;
    double uiScale = 1.0;
    AppSettings settings;
    std::unique_ptr<juce::FileChooser> exportChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

} // namespace StudioPatchbay
