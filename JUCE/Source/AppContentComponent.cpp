#include "AppContentComponent.h"
#include "UI/Fonts.h"

namespace StudioPatchbay
{

AppContentComponent::AppContentComponent()
{
    addAndMakeVisible (menuBar);
    addAndMakeVisible (sidebar);
    addAndMakeVisible (viewSwitcher);
    addAndMakeVisible (cardsView);
    addAndMakeVisible (patchbayView);
    addAndMakeVisible (controlRoomView);
    addAndMakeVisible (signalFlowView);
    addAndMakeVisible (statusLabel);

    menuBar.setPalette (pal);
    sidebar.setPalette (pal);
    viewSwitcher.setPalette (pal);
    cardsView.setPalette (pal);
    patchbayView.setPalette (pal);
    controlRoomView.setPalette (pal);
    signalFlowView.setPalette (pal);
    detailPanel.setPalette (pal);

    // Detail panel starts hidden, on top of other views.
    addChildComponent (detailPanel);
    detailPanel.onClose = [this] { resized(); };

    statusLabel.setFont (Fonts::sans (11.0f, Fonts::Weight::regular));
    statusLabel.setColour (juce::Label::textColourId, pal.ink.withAlpha (0.5f));
    statusLabel.setJustificationType (juce::Justification::centredLeft);

    // Wire view switching.
    viewSwitcher.onViewChanged = [this] (ViewSwitcher::View v) { onViewChanged (v); };

    // Wire card click → detail panel.
    cardsView.onDeviceClicked = [this] (const ParsedDevice& dev)
    {
        detailPanel.showDevice (&dev, appState.getDetectedPorts());
        resized();
    };

    appState.addChangeListener (this);

    // Trigger initial data load.
    changeListenerCallback (&appState);
    showActiveView();
}

int AppContentComponent::sc (int value) const
{
    return juce::roundToInt (value * uiScale);
}

// ============================================================================
// Paint
// ============================================================================

void AppContentComponent::paint (juce::Graphics& g)
{
    const auto barH = sc (StudioPatchbay::Design::Metrics::menuBarHeight);

    auto contentArea = getLocalBounds();
    contentArea.removeFromTop (barH + sc (4));
    contentArea.reduce (sc (6), sc (4));

    // The desktop shows only around the content frame: paint it there and
    // nowhere else, so a small repaint (the menu clock) stays small.
    {
        juce::Graphics::ScopedSaveState state (g);
        g.excludeClipRegion (contentArea);
        g.fillAll (pal.desktop);
        Design::DesktopPattern pattern;
        pattern.top = barH;
        pattern.step = juce::jmax (1, sc (4));
        pattern.rowOffset = sc (2);
        pattern.dotSize = juce::jmax (1, sc (1));
        Design::paintDesktopPattern (g, pal.desktop.brighter (0.12f), pattern, contentArea);
    }

    paintContentFrame (g, contentArea);
}

void AppContentComponent::paintContentFrame (juce::Graphics& g, juce::Rectangle<int> area)
{
    const auto edge = juce::jmax (1, sc (1));

    g.setColour (pal.windowPanel);
    g.fillRect (area);

    g.setColour (pal.shadowEdge);
    g.fillRect (area.getX(), area.getY(), area.getWidth(), edge);
    g.fillRect (area.getX(), area.getY(), edge, area.getHeight());

    g.setColour (pal.highlightEdge);
    g.fillRect (area.getX(), area.getBottom() - edge, area.getWidth(), edge);
    g.fillRect (area.getRight() - edge, area.getY(), edge, area.getHeight());
}

// ============================================================================
// Layout
// ============================================================================

void AppContentComponent::resized()
{
    auto area = getLocalBounds();
    const auto barH = sc (StudioPatchbay::Design::Metrics::menuBarHeight);

    menuBar.setBounds (area.removeFromTop (barH));

    area.removeFromTop (sc (4));
    area.reduce (sc (6), sc (4));

    // Status bar at the bottom.
    auto statusArea = area.removeFromBottom (sc (22));
    statusLabel.setBounds (statusArea.reduced (sc (8), 0));

    // Sidebar.
    sidebar.setBounds (area.removeFromLeft (sc (180)));
    area.removeFromLeft (sc (4));

    // View tabs.
    viewSwitcher.setBounds (area.removeFromTop (sc (30)));

    // Detail panel on the right (if visible).
    if (detailPanel.isShowing())
    {
        auto detailArea = area.removeFromRight (sc (DeviceDetailPanel::panelWidth));
        detailPanel.setBounds (detailArea);
    }

    // All views share the remaining bounds.
    cardsView.setBounds (area);
    patchbayView.setBounds (area);
    controlRoomView.setBounds (area);
    signalFlowView.setBounds (area);
}

// ============================================================================
// View switching
// ============================================================================

void AppContentComponent::onViewChanged (ViewSwitcher::View)
{
    showActiveView();
}

void AppContentComponent::showActiveView()
{
    auto view = viewSwitcher.getActiveView();

    cardsView.setVisible (view == ViewSwitcher::View::cards);
    patchbayView.setVisible (view == ViewSwitcher::View::patchbay);
    controlRoomView.setVisible (view == ViewSwitcher::View::controlRoom);
    signalFlowView.setVisible (view == ViewSwitcher::View::signalFlow);
}

// ============================================================================
// Export
// ============================================================================

juce::String AppContentComponent::viewName (ViewSwitcher::View view)
{
    switch (view)
    {
        case ViewSwitcher::View::cards:       return "Cards";
        case ViewSwitcher::View::patchbay:    return "Patchbay";
        case ViewSwitcher::View::controlRoom: return "Control Room";
        case ViewSwitcher::View::signalFlow:  return "Signal Flow";
    }
    return "View";
}

Export::ExportSource AppContentComponent::activeExportSource()
{
    const auto view = viewSwitcher.getActiveView();
    auto& content = [&]() -> juce::Component&
    {
        switch (view)
        {
            case ViewSwitcher::View::patchbay:    return patchbayView.getExportContent();
            case ViewSwitcher::View::controlRoom: return controlRoomView.getExportContent();
            case ViewSwitcher::View::signalFlow:  return signalFlowView.getExportContent();
            case ViewSwitcher::View::cards:       break;
        }
        return cardsView.getExportContent();
    }();

    juce::String title ("StudioPatchbay - " + viewName (view));
    if (auto* inst = appState.getSelectedInstallation())
        title << " - " << inst->displayName();

    return { content, pal.windowPanel, uiScale, title };
}

// ============================================================================
// Theme
// ============================================================================

void AppContentComponent::setDarkMode (bool shouldUseDarkMode)
{
    darkMode = shouldUseDarkMode;
    pal = darkMode ? StudioPatchbay::Design::darkPalette()
                   : StudioPatchbay::Design::lightPalette();

    menuBar.setPalette (pal);
    sidebar.setPalette (pal);
    viewSwitcher.setPalette (pal);
    cardsView.setPalette (pal);
    patchbayView.setPalette (pal);
    controlRoomView.setPalette (pal);
    signalFlowView.setPalette (pal);
    detailPanel.setPalette (pal);

    statusLabel.setColour (juce::Label::textColourId, pal.ink.withAlpha (0.5f));

    repaint();
}

void AppContentComponent::setUiScale (double scale)
{
    uiScale = scale;
    menuBar.setUiScale (scale);
    sidebar.setUiScale (scale);
    viewSwitcher.setUiScale (scale);
    cardsView.setUiScale (scale);
    patchbayView.setUiScale (scale);
    controlRoomView.setUiScale (scale);
    signalFlowView.setUiScale (scale);
    detailPanel.setUiScale (scale);
    resized();
    repaint();
}

// ============================================================================
// State change
// ============================================================================

void AppContentComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    cardsView.setDevices (appState.getParsedDevices());
    patchbayView.setData (appState.getParsedDevices(), appState.getDetectedPorts());
    controlRoomView.setBuses (appState.getControlRoomBuses());
    signalFlowView.setData (appState.getParsedDevices(), appState.getDetectedPorts());

    updateStatusText();
}

void AppContentComponent::updateStatusText()
{
    auto* inst = appState.getSelectedInstallation();
    if (inst == nullptr)
    {
        statusLabel.setText ("No Cubase installation found", juce::dontSendNotification);
        return;
    }

    auto& devices = appState.getParsedDevices();
    auto& ports = appState.getDetectedPorts();
    auto& buses = appState.getControlRoomBuses();

    juce::String status;
    status << inst->displayName();

    auto totalDevices = devices.instruments.size() + devices.fxDevices.size();
    if (totalDevices > 0)
        status << "  |  " << juce::String (totalDevices) << " devices ("
               << juce::String (devices.instruments.size()) << " instr, "
               << juce::String (devices.fxDevices.size()) << " FX)";

    if (! ports.empty())
        status << "  |  " << juce::String (ports.size()) << " ports";

    if (! buses.empty())
        status << "  |  " << juce::String (buses.size()) << " CR buses";

    if (appState.isWatching())
        status << "  |  Watching for changes";

    statusLabel.setText (status, juce::dontSendNotification);
}

} // namespace StudioPatchbay
