#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "AppState.h"
#include "UI/OS9MenuBar.h"
#include "UI/SidebarPanel.h"
#include "UI/ViewSwitcher.h"
#include "UI/CardsView.h"
#include "UI/PatchbayGridView.h"
#include "UI/ControlRoomView.h"
#include "UI/SignalFlowView.h"
#include "UI/DeviceDetailPanel.h"
#include "Export/ViewExporter.h"

namespace StudioPatchbay
{

// Root content component for the StudioPatchbay application window.
// Owns the OS9 menu bar, sidebar, view tabs, and all view components.
class AppContentComponent : public juce::Component,
                            private juce::ChangeListener
{
public:
    AppContentComponent();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setDarkMode (bool shouldUseDarkMode);
    bool isDarkMode() const { return darkMode; }

    void setUiScale (double scale);

    OS9MenuBar& getMenuBar() { return menuBar; }
    AppState& getAppState() { return appState; }

    // View switching API for menus and keyboard shortcuts.
    void switchToView (ViewSwitcher::View view) { viewSwitcher.setActiveView (view); showActiveView(); }
    ViewSwitcher::View getCurrentView() const { return viewSwitcher.getActiveView(); }

    // The active view's complete content, for File > Export.
    Export::ExportSource activeExportSource();
    static juce::String viewName (ViewSwitcher::View view);

private:
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void onViewChanged (ViewSwitcher::View view);
    void updateStatusText();
    void showActiveView();

    int sc (int value) const;
    void paintContentFrame (juce::Graphics& g, juce::Rectangle<int> area);

    StudioPatchbay::Design::Palette pal = StudioPatchbay::Design::darkPalette();
    bool darkMode = true;
    double uiScale = 1.0;

    AppState appState;
    OS9MenuBar menuBar;
    SidebarPanel sidebar { appState };
    ViewSwitcher viewSwitcher;

    // View components.
    CardsView cardsView;
    PatchbayGridView patchbayView;
    ControlRoomView controlRoomView;
    SignalFlowView signalFlowView;
    DeviceDetailPanel detailPanel;

    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppContentComponent)
};

} // namespace StudioPatchbay
