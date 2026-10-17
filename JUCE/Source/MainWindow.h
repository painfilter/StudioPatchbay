#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "AppContentComponent.h"
#include "Model/AppSettings.h"

namespace StudioPatchbay
{

// The native window. Its title bar and frame are the system's, so it moves
// and resizes like any other window; inside it the desktop is drawn.
//
// Size and zoom fit together: the desktop has a logical size, and the window
// is that size times the zoom. Choosing a zoom step keeps the logical size and
// resizes the window; dragging the window edge keeps the zoom and changes the
// logical size. The smallest window is the logical minimum times the zoom.
class MainWindow : public juce::DocumentWindow,
                   public juce::KeyListener,
                   private juce::DarkModeSettingListener,
                   private DesktopActions
{
public:
    explicit MainWindow (juce::String name);
    ~MainWindow() override;

    void closeButtonPressed() override;

    using juce::DocumentWindow::keyPressed;
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;

    AppContentComponent& getContent() { return *content; }

private:
    void showMenu (OS9MenuBar::Menu);
    juce::PopupMenu fileMenu();
    juce::PopupMenu viewMenu();
    juce::PopupMenu optionsMenu();
    juce::PopupMenu helpMenu();

    // DesktopActions; the menus use the same commands.
    ViewSwitcher::View currentView() const override;
    void exportView (Export::Format, ViewSwitcher::View) override;
    void reload() override;
    void showAbout() override;
    void forgetRecentExport (const juce::File&) override;
    void restoreWindow (juce::Rectangle<int> fromIcon) override;
    void showDeviceManager() override;

    void darkModeSettingChanged() override;

    void saveSettings();
    void applyAppearance();
    void applyLanguage (const juce::String&);
    void applyZoom (double scale);
    juce::Point<int> logicalSize() const;

    Design::PlatinumLookAndFeel os9Laf;
    AppContentComponent* content = nullptr;
    AppSettings settings;
    std::unique_ptr<juce::FileChooser> exportChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

} // namespace StudioPatchbay
