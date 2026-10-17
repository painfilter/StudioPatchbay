#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "AppState.h"
#include "Model/Browsing.h"
#include "UI/Sidebar.h"
#include "UI/Toolbar.h"
#include "UI/ViewSwitcher.h"
#include "UI/CardsView.h"
#include "UI/PatchbayView.h"
#include "UI/ControlRoomView.h"
#include "UI/DeviceDetailPanel.h"
#include "Export/ViewExporter.h"

namespace StudioPatchbay
{

// Everything inside the application window below its title bar, laid out as
// the Swift app's main window: the sidebar (Cubase version, filter, view),
// the toolbar (back/forward, counts, search), the view tabs with the watch
// status, Reload and the content zoom, the three views, and the detail
// panel over their right edge when a device is selected.
class Workspace : public juce::Component,
                  private juce::ChangeListener
{
public:
    explicit Workspace (AppState&);
    ~Workspace() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setPalette (const Design::Palette&, bool dark);
    void setUiScale (double);
    void refreshText();

    void switchToView (ViewSwitcher::View);
    ViewSwitcher::View getCurrentView() const { return viewSwitcher.getActiveView(); }

    double getContentScale() const { return style.contentScale; }
    void setContentScale (double);   // 0.6 .. 2.0, 10 % steps
    void goBack();
    void goForward();
    bool canGoBack() const { return history.canGoBack(); }
    bool canGoForward() const { return history.canGoForward(); }
    void focusSearch() { toolbar.focusSearch(); }
    void jumpToChannel (int channel);

    std::function<void()> onOpenSetup;
    std::function<void (double)> onContentScaleChanged;

    // A view's complete content, for export.
    Export::ExportSource exportSource (ViewSwitcher::View);

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void updateData();
    void applyFilter();
    void select (std::optional<ParsedDevice>);
    void showActiveView();
    void nextMatch();
    void restore (NavigationHistory::Point);
    void scrollWhenShown (int channel);
    ContentView& view (ViewSwitcher::View);
    int sc (int value) const { return juce::roundToInt (value * style.uiScale); }

    AppState& appState;
    ViewData data;
    ViewStyle style;
    FilterMode filter = FilterMode::all;
    juce::String query;
    NavigationHistory history;
    std::optional<int> currentChannel;
    int matchIndex = -1;

    Sidebar sidebar { appState };
    Toolbar toolbar;
    ViewSwitcher viewSwitcher;
    ViewBar viewBar;
    CardsView cardsView { data };
    PatchbayView patchbayView { data };
    ControlRoomView controlRoomView { data };
    DeviceDetailPanel detailPanel { data };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Workspace)
};

} // namespace StudioPatchbay
