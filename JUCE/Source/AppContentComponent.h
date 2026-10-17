#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "Design/PlatinumAboutDialog.h"
#include "Design/PlatinumMessageDialog.h"
#include "Design/PlatinumOverlayWindow.h"
#include "AppState.h"
#include "Workspace.h"
#include "UI/OS9MenuBar.h"
#include "UI/AppWindow.h"
#include "UI/DesktopShelf.h"

namespace StudioPatchbay
{

// The simulated desktop that fills the native window: the menu bar, the
// application window floating on the desktop pattern, and, while that window
// is collapsed (WindowShade), the desktop icons. About, the licences and
// messages open as overlays above everything.
//
// Every measure is logical and multiplied by the window zoom, so a zoom step
// scales the whole desktop; resizing the native window changes the logical
// desktop size instead.
class AppContentComponent : public juce::Component,
                            private juce::Timer
{
public:
    explicit AppContentComponent (int preferredCubaseVersion);
    ~AppContentComponent() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    void setDarkMode (bool);
    bool isDarkMode() const { return darkMode; }
    void setUiScale (double);
    double getUiScale() const { return uiScale; }
    void refreshText();

    OS9MenuBar& getMenuBar() { return menuBar; }
    AppState& getAppState() { return appState; }
    Workspace& getWorkspace() { return workspace; }
    DesktopShelf& getDesktop() { return desktop; }

    bool isShaded() const { return shaded; }
    void setShaded (bool shouldBeShaded, juce::Rectangle<int> icon = {});
    std::function<void (bool)> onShadedChanged;

    void showAbout();
    void showLicenses();
    void showMessage (const juce::String& title, const juce::String& message);

    // The smallest logical desktop the window layout works in.
    static constexpr int minimumWidth = 760;
    static constexpr int minimumHeight = 560;

private:
    // zero-footprint: runs only for the 220 ms of a WindowShade animation.
    void timerCallback() override;
    int sc (int value) const { return juce::roundToInt (value * uiScale); }
    juce::Rectangle<int> windowBounds() const;
    void updateTitle();
    void finishAnimation();

    AppState appState;
    Workspace workspace { appState };
    AppWindow appWindow { workspace };
    OS9MenuBar menuBar;
    DesktopShelf desktop { appState };

    Design::AboutDialog about;
    Design::MessageDialog message;
    std::unique_ptr<Design::OverlayWindow> licenses;

    Design::Palette pal = Design::darkPalette();
    bool darkMode = true;
    double uiScale = 1.0;

    bool shaded = false;
    bool animating = false;
    bool animatingToShaded = false;
    double animationStartMs = 0.0;
    juce::Rectangle<int> animationIcon;

    struct TitleWatcher : juce::ChangeListener
    {
        explicit TitleWatcher (AppContentComponent& o) : owner (o) {}
        void changeListenerCallback (juce::ChangeBroadcaster*) override { owner.updateTitle(); }
        AppContentComponent& owner;
    } titleWatcher { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppContentComponent)
};

} // namespace StudioPatchbay
