#include "Workspace.h"
#include "UI/Fonts.h"
#include "Localisation/Text.h"

namespace StudioPatchbay
{

using Text::tr;

Workspace::Workspace (AppState& state) : appState (state)
{
    data.select = [this] (std::optional<ParsedDevice> d) { select (std::move (d)); };
    data.jumpToChannel = [this] (int ch) { jumpToChannel (ch); };
    data.openSetup = [this] { if (onOpenSetup) onOpenSetup(); };

    for (juce::Component* c : { static_cast<juce::Component*> (&sidebar), static_cast<juce::Component*> (&toolbar),
                                static_cast<juce::Component*> (&viewSwitcher), static_cast<juce::Component*> (&viewBar),
                                static_cast<juce::Component*> (&cardsView), static_cast<juce::Component*> (&patchbayView),
                                static_cast<juce::Component*> (&controlRoomView) })
        addAndMakeVisible (*c);
    addChildComponent (detailPanel);

    sidebar.onFilter = [this] (FilterMode m) { filter = m; applyFilter(); };
    sidebar.onView = [this] (ViewSwitcher::View v) { switchToView (v); };
    sidebar.onSetup = [this] { if (onOpenSetup) onOpenSetup(); };
    viewSwitcher.onViewChanged = [this] (ViewSwitcher::View) { showActiveView(); };
    toolbar.onSearch = [this] (const juce::String& q) { query = q; matchIndex = -1; applyFilter(); };
    toolbar.onNextMatch = [this] { nextMatch(); };
    toolbar.onBack = [this] { goBack(); };
    toolbar.onForward = [this] { goForward(); };
    viewBar.onReload = [this] { appState.rescanInstallations(); };
    viewBar.onContentScale = [this] (double s) { setContentScale (s); };
    detailPanel.onClose = [this] { select (std::nullopt); };

    appState.addChangeListener (this);
    setUiScale (1.0);
    updateData();
    showActiveView();
}

Workspace::~Workspace()
{
    appState.removeChangeListener (this);
}

// ============================================================================
// Data
// ============================================================================

void Workspace::changeListenerCallback (juce::ChangeBroadcaster*)
{
    updateData();
}

void Workspace::updateData()
{
    data.devices = appState.getParsedDevices();
    data.physical = appState.getPhysicalDevices();
    data.ports = appState.getAudioPorts();
    data.buses = appState.getControlRoomBuses();

    // A device that is gone after a reload is no longer selected.
    if (data.selected)
    {
        const auto all = data.devices.all();
        if (std::find (all.begin(), all.end(), *data.selected) == all.end())
            data.selected.reset();
    }

    const auto& d = data.devices;
    toolbar.setCounts (static_cast<int> (d.instruments.size()), static_cast<int> (d.fxDevices.size()),
                       d.usedChannelCount(), appState.hasParsedDevices());
    viewBar.setWatching (appState.isWatching(), appState.getLastReadTime());
    sidebar.repaint();
    applyFilter();
}

void Workspace::applyFilter()
{
    data.filtered = Browsing::filter (data.devices, filter, query, data.physical);
    data.searchActive = query.isNotEmpty();
    data.highlightedChannels.clear();
    for (const auto& d : data.filtered)
        for (auto ch : d.allChannels())
            data.highlightedChannels.insert (ch);
    toolbar.setMatches (static_cast<int> (data.filtered.size()));

    cardsView.refresh();
    patchbayView.refresh();
    controlRoomView.refresh();
    detailPanel.refresh();
    repaint();
}

void Workspace::select (std::optional<ParsedDevice> device)
{
    data.selected = std::move (device);
    detailPanel.setVisible (data.selected.has_value());
    detailPanel.refresh();
    resized();
    cardsView.getExportContent().repaint();
    patchbayView.getExportContent().repaint();
}

// ============================================================================
// Views, jumps and history
// ============================================================================

ContentView& Workspace::view (ViewSwitcher::View v)
{
    switch (v)
    {
        case ViewSwitcher::View::patchbay:    return patchbayView;
        case ViewSwitcher::View::controlRoom: return controlRoomView;
        case ViewSwitcher::View::cards:       break;
    }
    return cardsView;
}

void Workspace::switchToView (ViewSwitcher::View v)
{
    viewSwitcher.setActiveView (v);
    showActiveView();
}

void Workspace::showActiveView()
{
    const auto v = viewSwitcher.getActiveView();
    cardsView.setVisible (v == ViewSwitcher::View::cards);
    patchbayView.setVisible (v == ViewSwitcher::View::patchbay);
    controlRoomView.setVisible (v == ViewSwitcher::View::controlRoom);
    sidebar.setView (v);
    detailPanel.toFront (false);
}

void Workspace::scrollWhenShown (int channel)
{
    switchToView (ViewSwitcher::View::patchbay);
    patchbayView.scrollToChannel (channel);
}

void Workspace::jumpToChannel (int channel)
{
    history.jumped ({ static_cast<int> (getCurrentView()), currentChannel });
    currentChannel = channel;
    scrollWhenShown (channel);
    toolbar.setHistory (history.canGoBack(), history.canGoForward());
}

void Workspace::restore (NavigationHistory::Point point)
{
    currentChannel = point.channel;
    if (point.channel)
        scrollWhenShown (*point.channel);
    else
        switchToView (static_cast<ViewSwitcher::View> (point.view));
    toolbar.setHistory (history.canGoBack(), history.canGoForward());
}

void Workspace::goBack()
{
    if (auto point = history.back ({ static_cast<int> (getCurrentView()), currentChannel }))
        restore (*point);
}

void Workspace::goForward()
{
    if (auto point = history.forward ({ static_cast<int> (getCurrentView()), currentChannel }))
        restore (*point);
}

void Workspace::nextMatch()
{
    const auto matches = Browsing::inPatchbayOrder (data.filtered);
    if (matches.empty())
        return;
    matchIndex = (matchIndex + 1) % static_cast<int> (matches.size());
    const auto channels = matches[static_cast<size_t> (matchIndex)].allChannels();
    if (! channels.empty())
        jumpToChannel (channels.front());
}

void Workspace::setContentScale (double scale)
{
    style.contentScale = juce::jlimit (0.6, 2.0, std::round (scale * 10.0) / 10.0);
    viewBar.setContentScale (style.contentScale);
    for (auto* v : { static_cast<ContentView*> (&cardsView), static_cast<ContentView*> (&patchbayView),
                     static_cast<ContentView*> (&controlRoomView) })
        v->setStyle (style);
    if (onContentScaleChanged)
        onContentScaleChanged (style.contentScale);
}

// ============================================================================
// Layout and look
// ============================================================================

void Workspace::paint (juce::Graphics& g)
{
    g.fillAll (style.pal.windowPanel);
    if (appState.hasParsedDevices())
        return;

    // No data: why, as in the Swift app's empty and error states.
    auto area = cardsView.getBounds().withSizeKeepingCentre (juce::jmin (cardsView.getWidth() - sc (40), sc (460)), sc (120));
    juce::String title, detail;
    if (appState.getErrorMessage().isNotEmpty())
    {
        title = tr ("empty.couldNotLoad");
        detail = appState.getErrorMessage();
    }
    else
    {
        title = tr ("empty.noData");
        juce::StringArray names;
        for (const auto& inst : appState.getInstallations())
            names.add (inst.displayName());
        detail = names.isEmpty() ? tr ("empty.noInstallations")
                                 : tr ("empty.xmlMissing").replace ("%NAMES%", names.joinIntoString (", "));
    }
    g.setColour (style.pal.ink);
    g.setFont (Fonts::sans (static_cast<float> (15.0 * style.uiScale), Fonts::Weight::semiBold));
    g.drawText (title, area.removeFromTop (sc (26)), juce::Justification::centred);
    g.setColour (style.pal.inkSoft);
    g.setFont (Fonts::sans (static_cast<float> (12.0 * style.uiScale)));
    g.drawFittedText (detail, area, juce::Justification::centredTop, 4);
}

void Workspace::resized()
{
    auto area = getLocalBounds();
    sidebar.setBounds (area.removeFromLeft (sc (200)));
    toolbar.setBounds (area.removeFromTop (sc (38)));
    auto tabs = area.removeFromTop (sc (32)).reduced (sc (6), 0);
    viewSwitcher.setBounds (tabs.removeFromLeft (juce::jmin (viewSwitcher.preferredWidth(), tabs.getWidth() / 2)).withTrimmedTop (sc (4)));
    viewBar.setBounds (tabs.withTrimmedLeft (sc (8)));

    const auto hasData = appState.hasParsedDevices();
    for (auto* v : { static_cast<ContentView*> (&cardsView), static_cast<ContentView*> (&patchbayView),
                     static_cast<ContentView*> (&controlRoomView) })
    {
        v->setBounds (area);
        v->setAlpha (hasData ? 1.0f : 0.0f);
        v->setInterceptsMouseClicks (hasData, hasData);
    }
    detailPanel.setBounds (area.removeFromRight (sc (DeviceDetailPanel::panelWidth)));
}

void Workspace::setPalette (const Design::Palette& p, bool dark)
{
    style.pal = p;
    style.dark = dark;
    sidebar.setPalette (p);
    toolbar.setPalette (p);
    viewSwitcher.setPalette (p);
    viewBar.setPalette (p);
    detailPanel.setStyle (style);
    for (auto* v : { static_cast<ContentView*> (&cardsView), static_cast<ContentView*> (&patchbayView),
                     static_cast<ContentView*> (&controlRoomView) })
        v->setStyle (style);
    repaint();
}

void Workspace::setUiScale (double scale)
{
    style.uiScale = scale;
    sidebar.setUiScale (scale);
    toolbar.setUiScale (scale);
    viewSwitcher.setUiScale (scale);
    viewBar.setUiScale (scale);
    viewBar.setContentScale (style.contentScale);
    detailPanel.setStyle (style);
    for (auto* v : { static_cast<ContentView*> (&cardsView), static_cast<ContentView*> (&patchbayView),
                     static_cast<ContentView*> (&controlRoomView) })
        v->setStyle (style);
    resized();
    repaint();
}

void Workspace::refreshText()
{
    toolbar.refreshText();
    viewBar.refreshText();
    viewSwitcher.repaint();
    sidebar.repaint();
    applyFilter();
}

Export::ExportSource Workspace::exportSource (ViewSwitcher::View v)
{
    juce::String title ("StudioPatchbay - " + ViewSwitcher::viewName (v));
    if (auto* inst = appState.getSelectedInstallation())
        title << " - " << inst->displayName();
    return { view (v).getExportContent(), style.pal.windowPanel, style.uiScale, title };
}

Workspace::PatchbaySheet::PatchbaySheet (const ViewData& live, const ViewStyle& liveStyle, const juce::String& t)
    : data (live), title (t), background (liveStyle.pal.windowPanel)
{
    data.filtered = data.devices.all();
    data.highlightedChannels.clear();
    for (const auto& d : data.filtered)
        for (auto ch : d.allChannels())
            data.highlightedChannels.insert (ch);
    data.searchActive = false;
    data.selected.reset();
    data.select = nullptr;
    data.jumpToChannel = nullptr;
    data.openSetup = nullptr;

    auto style = liveStyle;
    style.uiScale = 1.0;
    view.setShowConnections (true);
    view.setBounds (0, 0, 1400 + view.getViewport().getScrollBarThickness(), 800);
    view.setStyle (style);
}

Export::ExportSource Workspace::PatchbaySheet::source()
{
    return { view.getExportContent(), background, 1.0, title };
}

std::unique_ptr<Workspace::PatchbaySheet> Workspace::makePatchbaySheet() const
{
    juce::String title ("StudioPatchbay - " + ViewSwitcher::viewName (ViewSwitcher::View::patchbay));
    if (auto* inst = appState.getSelectedInstallation())
        title << " - " << inst->displayName();
    return std::make_unique<PatchbaySheet> (data, style, title);
}

} // namespace StudioPatchbay
