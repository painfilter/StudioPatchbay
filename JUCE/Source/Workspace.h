#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Design/PlatinumLookAndFeel.h"
#include "AppState.h"
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

// Everything inside the application window below its title bar: the
// sidebar, the view tabs, the views, the detail panel and the status line.
class Workspace : public juce::Component,
                  private juce::ChangeListener
{
public:
    explicit Workspace (AppState&);
    ~Workspace() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setPalette (const Design::Palette&);
    void setUiScale (double);
    void refreshText();

    void switchToView (ViewSwitcher::View view) { viewSwitcher.setActiveView (view); showActiveView(); }
    ViewSwitcher::View getCurrentView() const { return viewSwitcher.getActiveView(); }

    // A view's complete content, for export.
    Export::ExportSource exportSource (ViewSwitcher::View);

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void updateStatusText();
    void showActiveView();
    int sc (int value) const { return juce::roundToInt (value * uiScale); }

    AppState& appState;
    Design::Palette pal = Design::darkPalette();
    double uiScale = 1.0;

    SidebarPanel sidebar { appState };
    ViewSwitcher viewSwitcher;
    CardsView cardsView;
    PatchbayGridView patchbayView;
    ControlRoomView controlRoomView;
    SignalFlowView signalFlowView;
    DeviceDetailPanel detailPanel;
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Workspace)
};

} // namespace StudioPatchbay
