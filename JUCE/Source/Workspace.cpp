#include "Workspace.h"
#include "UI/Fonts.h"
#include "Localisation/Text.h"

namespace StudioPatchbay
{

using Text::tr;

Workspace::Workspace (AppState& state) : appState (state)
{
    addAndMakeVisible (sidebar);
    addAndMakeVisible (viewSwitcher);
    addAndMakeVisible (cardsView);
    addAndMakeVisible (patchbayView);
    addAndMakeVisible (controlRoomView);
    addAndMakeVisible (signalFlowView);
    addAndMakeVisible (statusLabel);
    addChildComponent (detailPanel);
    detailPanel.onClose = [this] { resized(); };

    statusLabel.setJustificationType (juce::Justification::centredLeft);
    viewSwitcher.onViewChanged = [this] (ViewSwitcher::View) { showActiveView(); };
    cardsView.onDeviceClicked = [this] (const ParsedDevice& dev)
    {
        detailPanel.showDevice (&dev, appState.getDetectedPorts());
        resized();
    };

    setPalette (pal);
    setUiScale (uiScale);
    appState.addChangeListener (this);
    changeListenerCallback (&appState);
    showActiveView();
}

Workspace::~Workspace()
{
    appState.removeChangeListener (this);
}

void Workspace::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);
}

void Workspace::resized()
{
    auto area = getLocalBounds().reduced (sc (6), 0);
    area.removeFromTop (sc (4));

    statusLabel.setBounds (area.removeFromBottom (sc (22)).reduced (sc (2), 0));
    sidebar.setBounds (area.removeFromLeft (sc (180)));
    area.removeFromLeft (sc (4));
    viewSwitcher.setBounds (area.removeFromTop (sc (30)));

    if (detailPanel.isVisible())
        detailPanel.setBounds (area.removeFromRight (sc (DeviceDetailPanel::panelWidth)));

    for (juce::Component* view : { static_cast<juce::Component*> (&cardsView), static_cast<juce::Component*> (&patchbayView),
                                   static_cast<juce::Component*> (&controlRoomView), static_cast<juce::Component*> (&signalFlowView) })
        view->setBounds (area);
}

void Workspace::showActiveView()
{
    const auto view = viewSwitcher.getActiveView();
    cardsView.setVisible (view == ViewSwitcher::View::cards);
    patchbayView.setVisible (view == ViewSwitcher::View::patchbay);
    controlRoomView.setVisible (view == ViewSwitcher::View::controlRoom);
    signalFlowView.setVisible (view == ViewSwitcher::View::signalFlow);
}

Export::ExportSource Workspace::exportSource (ViewSwitcher::View view)
{
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

    juce::String title ("StudioPatchbay - " + ViewSwitcher::viewName (view));
    if (auto* inst = appState.getSelectedInstallation())
        title << " - " << inst->displayName();

    return { content, pal.windowPanel, uiScale, title };
}

void Workspace::setPalette (const Design::Palette& newPalette)
{
    pal = newPalette;
    sidebar.setPalette (pal);
    viewSwitcher.setPalette (pal);
    cardsView.setPalette (pal);
    patchbayView.setPalette (pal);
    controlRoomView.setPalette (pal);
    signalFlowView.setPalette (pal);
    detailPanel.setPalette (pal);
    statusLabel.setColour (juce::Label::textColourId, pal.inkSoft);
    repaint();
}

void Workspace::setUiScale (double scale)
{
    uiScale = scale;
    sidebar.setUiScale (scale);
    viewSwitcher.setUiScale (scale);
    cardsView.setUiScale (scale);
    patchbayView.setUiScale (scale);
    controlRoomView.setUiScale (scale);
    signalFlowView.setUiScale (scale);
    detailPanel.setUiScale (scale);
    statusLabel.setFont (Fonts::sans (static_cast<float> (11.0 * scale), Fonts::Weight::regular));
    resized();
    repaint();
}

void Workspace::refreshText()
{
    viewSwitcher.repaint();
    updateStatusText();
}

void Workspace::changeListenerCallback (juce::ChangeBroadcaster*)
{
    cardsView.setDevices (appState.getParsedDevices());
    patchbayView.setData (appState.getParsedDevices(), appState.getDetectedPorts());
    controlRoomView.setBuses (appState.getControlRoomBuses());
    signalFlowView.setData (appState.getParsedDevices(), appState.getDetectedPorts());
    updateStatusText();
}

void Workspace::updateStatusText()
{
    auto* inst = appState.getSelectedInstallation();
    if (inst == nullptr)
    {
        statusLabel.setText (tr ("status.noInstallation"), juce::dontSendNotification);
        return;
    }

    const auto& devices = appState.getParsedDevices();
    const auto& ports = appState.getDetectedPorts();
    const auto& buses = appState.getControlRoomBuses();

    juce::StringArray parts { inst->displayName() };
    const auto total = devices.instruments.size() + devices.fxDevices.size();
    if (total > 0)
        parts.add (tr ("status.devices").replace ("%COUNT%", juce::String (total))
                                        .replace ("%INSTRUMENTS%", juce::String (devices.instruments.size()))
                                        .replace ("%FX%", juce::String (devices.fxDevices.size())));
    if (! ports.empty())
        parts.add (tr ("status.ports").replace ("%COUNT%", juce::String (ports.size())));
    if (! buses.empty())
        parts.add (tr ("status.buses").replace ("%COUNT%", juce::String (buses.size())));
    if (appState.isWatching())
        parts.add (tr ("status.watching"));

    statusLabel.setText (parts.joinIntoString ("  |  "), juce::dontSendNotification);
}

} // namespace StudioPatchbay
