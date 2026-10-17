#include "DeviceManager.h"
#include "DeviceEditor.h"
#include "ImportReview.h"
#include "UI/DeviceStyle.h"
#include "Design/PlatinumOverlayWindow.h"
#include "Localisation/Text.h"
#include "UI/Glyphs.h"

#include <algorithm>

namespace StudioPatchbay
{

namespace
{
using Text::tr;

constexpr int margin = 12, headerHeight = 34, leftWidth = 420;
constexpr int deviceRowHeight = 44, headerRowHeight = 26;
const juce::Rectangle<int> filterArea { margin, 42, leftWidth, 24 };
const juce::Rectangle<int> listArea { margin, 70, leftWidth, DeviceManager::height - 70 - margin };
const juce::Rectangle<int> portArea { margin * 2 + leftWidth, 42, DeviceManager::width - leftWidth - margin * 3,
                                      DeviceManager::height - 42 - margin };
} // namespace

// ============================================================================
// The device list
// ============================================================================

class DeviceManager::DeviceList : public juce::Component
{
public:
    explicit DeviceList (DeviceManager& o) : owner (o) {}

    int rowHeight (const Row& r) const { return r.header ? headerRowHeight : deviceRowHeight; }

    void updateSize (int w)
    {
        int h = 0;
        for (const auto& r : owner.rows)
            h += rowHeight (r);
        setSize (w, h);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& pal = owner.pal;
        const auto& devices = owner.appState.getPhysicalDevices();
        g.fillAll (pal.field);
        int y = 0;
        for (size_t i = 0; i < owner.rows.size(); ++i)
        {
            const auto& r = owner.rows[i];
            const juce::Rectangle<int> area (0, y, getWidth(), rowHeight (r));
            y += area.getHeight();
            if (r.header)
            {
                g.setColour (pal.inkSoft);
                g.setFont (Fonts::sans (11.0f, Fonts::Weight::semiBold));
                g.drawText (DeviceStyle::deviceTypePlural (r.type).toUpperCase(), area.reduced (8, 0).withTrimmedTop (8),
                            juce::Justification::centredLeft);
                continue;
            }
            if (r.device >= devices.size())
                continue;
            const auto& d = devices[r.device];
            if (static_cast<int> (i) == hovered)
            {
                g.setColour (pal.selection.withAlpha (0.12f));
                g.fillRect (area);
            }
            g.setColour (DeviceStyle::deviceColour (d, pal.inkSoft));
            g.fillRect (area.getX() + 8, area.getY() + 8, 8, area.getHeight() - 16);

            auto text = area.withTrimmedLeft (26).reduced (0, 5);
            g.setColour (pal.ink);
            g.setFont (Fonts::sans (13.0f, Fonts::Weight::semiBold));
            const auto nameRow = text.removeFromTop (18);
            g.drawText (d.name, nameRow, juce::Justification::centredLeft);
            if (d.deviceType != DeviceType::converter)
            {
                const auto nameWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), d.name));
                const auto badge = DeviceStyle::deviceTypeName (d.deviceType);
                g.setFont (Fonts::sans (10.0f, Fonts::Weight::medium));
                const auto badgeWidth = juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), badge)) + 10;
                const auto badgeArea = juce::Rectangle<int> (nameRow.getX() + nameWidth + 6, nameRow.getY() + 2, badgeWidth, 14);
                g.setColour (pal.controlSurface);
                g.fillRect (badgeArea);
                g.setColour (pal.shadowEdge);
                g.drawRect (badgeArea, 1);
                g.setColour (pal.inkSoft);
                g.drawText (badge, badgeArea, juce::Justification::centred);
            }
            g.setColour (pal.inkSoft);
            g.setFont (DeviceStyle::monoFont (11.0f));
            g.drawText (DeviceStyle::rangeText (d), text, juce::Justification::centredLeft);

            g.setColour (pal.shadowEdge.withAlpha (0.35f));
            g.fillRect (area.getX() + 26, area.getBottom() - 1, area.getWidth() - 26, 1);
        }
    }

    int rowAt (int y) const
    {
        int top = 0;
        for (size_t i = 0; i < owner.rows.size(); ++i)
        {
            top += rowHeight (owner.rows[i]);
            if (y < top)
                return owner.rows[i].header ? -1 : static_cast<int> (i);
        }
        return -1;
    }

    void mouseMove (const juce::MouseEvent& e) override { setHovered (rowAt (e.y)); }
    void mouseExit (const juce::MouseEvent&) override { setHovered (-1); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        const auto index = rowAt (e.y);
        const auto& devices = owner.appState.getPhysicalDevices();
        if (index < 0 || owner.rows[static_cast<size_t> (index)].device >= devices.size())
            return;
        const auto device = devices[owner.rows[static_cast<size_t> (index)].device];
        if (e.mods.isPopupMenu())
            owner.showRowMenu (device);
        else
            owner.openEditor (device, false);
    }

private:
    void setHovered (int index)
    {
        if (hovered == index)
            return;
        hovered = index;
        setMouseCursor (index >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }

    DeviceManager& owner;
    int hovered = -1;
};

// ============================================================================
// Detected ports, the Cubase version they come from, interface names
// ============================================================================

class DeviceManager::PortPanel : public juce::Component
{
public:
    explicit PortPanel (DeviceManager& o) : owner (o)
    {
        versionBox.onChange = [this]
        {
            const auto index = versionBox.getSelectedItemIndex();
            if (index >= 0 && index != owner.appState.getSelectedIndex())
                owner.appState.selectInstallation (index);
        };
        addAndMakeVisible (versionBox);
        labelsViewport.setViewedComponent (&labels, false);
        labelsViewport.setScrollBarsShown (true, false);
        addAndMakeVisible (labelsViewport);
        linesViewport.setViewedComponent (&lines, false);
        linesViewport.setScrollBarsShown (true, false);
        addAndMakeVisible (linesViewport);
    }

    void refresh()
    {
        const auto& installations = owner.appState.getInstallations();
        versionBox.clear (juce::dontSendNotification);
        for (size_t i = 0; i < installations.size(); ++i)
            versionBox.addItem (installations[i].displayName(), static_cast<int> (i) + 1);
        versionBox.setSelectedItemIndex (owner.appState.getSelectedIndex(), juce::dontSendNotification);
        versionBox.setTextWhenNothingSelected (installations.empty() ? tr ("devices.noInstallation") : tr ("devices.select"));
        versionBox.setEnabled (! installations.empty());

        // One name field per port; typing changes nothing until Return or
        // leaving the field, so the views do not rebuild per keystroke.
        const auto& audioPorts = owner.appState.getAudioPorts();
        editors.clear();
        for (size_t i = 0; i < audioPorts.size(); ++i)
        {
            auto editor = std::make_unique<juce::TextEditor>();
            const auto port = audioPorts[i];
            editor->setText (savedLabel (port), false);
            editor->setTextToShowWhenEmpty (port.name + " Interface", owner.pal.inkSoft);
            editor->setFont (Fonts::sans (12.0f, Fonts::Weight::regular));
            const auto commit = [this, port, e = editor.get()]
            {
                if (e->getText() != savedLabel (port))
                {
                    // Deferred: committing rebuilds this panel and its editors.
                    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<DeviceManager> (&owner), port,
                                                      text = e->getText()]
                    {
                        if (safe != nullptr)
                            safe->appState.setPortLabel (port, text);
                    });
                }
            };
            editor->onReturnKey = commit;
            editor->onFocusLost = commit;
            labels.addAndMakeVisible (*editor);
            editors.push_back (std::move (editor));
        }
        resized();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& pal = owner.pal;
        auto area = getLocalBounds();
        g.setColour (pal.ink);
        g.setFont (Fonts::sans (12.5f, Fonts::Weight::semiBold));
        g.drawText (tr ("devices.detectedPorts"), area.removeFromTop (18), juce::Justification::centredLeft);
        g.setColour (pal.inkSoft);
        g.setFont (Fonts::sans (11.0f, Fonts::Weight::regular));
        g.drawFittedText (tr ("devices.detectedPortsHelp"), area.removeFromTop (28), juce::Justification::topLeft, 2);
        g.drawText (tr ("devices.from"), area.removeFromTop (26).withWidth (44), juce::Justification::centredLeft);

        if (! owner.appState.getAudioPorts().empty())
        {
            area.removeFromTop (6);
            g.setColour (pal.ink);
            g.setFont (Fonts::sans (11.5f, Fonts::Weight::semiBold));
            g.drawText (tr ("devices.interfaceNames"), area.removeFromTop (16), juce::Justification::centredLeft);
            g.setColour (pal.inkSoft);
            g.setFont (Fonts::sans (10.5f, Fonts::Weight::regular));
            g.drawFittedText (tr ("devices.interfaceNamesHelp"), area.removeFromTop (28), juce::Justification::topLeft, 2);
        }

        if (owner.appState.getDetectedPorts().empty())
        {
            g.setColour (pal.inkSoft);
            g.setFont (Fonts::sans (11.5f, Fonts::Weight::regular));
            g.drawFittedText (tr ("devices.noPorts"), linesViewport.getBounds(), juce::Justification::centredTop, 3);
        }
        else if (hasApproximate())
        {
            g.setColour (pal.inkSoft);
            g.setFont (Fonts::sans (10.5f, Fonts::Weight::regular));
            Glyphs::line (g, Glyphs::Kind::warning, juce::Colour (0xffd08a00), pal.inkSoft, tr ("devices.approximate"),
                          approximateArea, 4);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromTop (18 + 28);
        versionBox.setBounds (area.removeFromTop (26).withTrimmedLeft (44).reduced (0, 2));

        const auto portCount = static_cast<int> (owner.appState.getAudioPorts().size());
        labelsViewport.setVisible (portCount > 0);
        if (portCount > 0)
        {
            area.removeFromTop (6 + 16 + 28);
            labelsViewport.setBounds (area.removeFromTop (juce::jmin (portCount, 4) * 26 + 2));
            labels.setSize (labelsViewport.getMaximumVisibleWidth(), portCount * 26);
            for (int i = 0; i < portCount; ++i)
                editors[static_cast<size_t> (i)]->setBounds (64, i * 26 + 1, labels.getWidth() - 66, 22);
        }
        area.removeFromTop (8);
        approximateArea = hasApproximate() ? area.removeFromTop (56) : juce::Rectangle<int>();
        linesViewport.setBounds (area);
        lines.setSize (linesViewport.getMaximumVisibleWidth(),
                       static_cast<int> (owner.appState.getDetectedPorts().size()) * 16 + 4);
        lines.setVisible (! owner.appState.getDetectedPorts().empty());
    }

private:
    bool hasApproximate() const
    {
        const auto& d = owner.appState.getDetectedPorts();
        return std::any_of (d.begin(), d.end(), [] (const DetectedPort& p) { return p.isApproximate; });
    }

    juce::String savedLabel (const AudioPort& port) const
    {
        const auto& saved = owner.appState.getDeviceStore().getLabels();
        for (const auto& key : { AudioPorts::stableLabelKey (port), "ch" + juce::String (port.channelStart), port.name })
            if (auto it = saved.find (key); it != saved.end())
                return it->second;
        return {};
    }

    struct Labels : juce::Component
    {
        explicit Labels (PortPanel& p) : panel (p) {}
        void paint (juce::Graphics& g) override
        {
            const auto& ports = panel.owner.appState.getAudioPorts();
            g.setColour (panel.owner.pal.ink);
            g.setFont (DeviceStyle::monoFont (11.0f));
            for (size_t i = 0; i < ports.size(); ++i)
                g.drawText (ports[i].name, 0, static_cast<int> (i) * 26 + 1, 60, 22, juce::Justification::centredLeft);
        }
        PortPanel& panel;
    } labels { *this };

    struct Lines : juce::Component
    {
        explicit Lines (PortPanel& p) : panel (p) {}
        void paint (juce::Graphics& g) override
        {
            auto ports = panel.owner.appState.getDetectedPorts();
            std::stable_sort (ports.begin(), ports.end(), [] (auto& a, auto& b) { return a.channelStart < b.channelStart; });
            g.fillAll (panel.owner.pal.field);
            g.setColour (panel.owner.pal.ink);
            g.setFont (DeviceStyle::monoFont (11.0f));
            int y = 2;
            for (const auto& p : ports)
            {
                const auto line = juce::String (p.direction == ChannelDirection::send ? "OUT" : "IN ")
                                  + (p.isApproximate ? "*" : " ") + " " + p.family.paddedRight (' ', 14).substring (0, 14)
                                  + "ch " + DeviceStyle::rangeOf ({ p.channelStart, p.channelEnd });
                g.drawText (line, 4, y, getWidth() - 4, 16, juce::Justification::centredLeft);
                y += 16;
            }
        }
        PortPanel& panel;
    } lines { *this };

    DeviceManager& owner;
    juce::ComboBox versionBox;
    juce::Viewport labelsViewport, linesViewport;
    std::vector<std::unique_ptr<juce::TextEditor>> editors;
    juce::Rectangle<int> approximateArea;
};

// ============================================================================
// The window
// ============================================================================

DeviceManager::DeviceManager (AppState& state, DesktopWindows& windows)
    : appState (state), desktop (windows)
{
    list = std::make_unique<DeviceList> (*this);
    ports = std::make_unique<PortPanel> (*this);
    listViewport.setViewedComponent (list.get(), false);
    listViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (listViewport);
    addAndMakeVisible (*ports);

    addButton.setButtonText (tr ("devices.add"));
    actionsButton.setButtonText (tr ("devices.actions"));
    emptyAddButton.setButtonText (tr ("devices.add"));
    emptyImportButton.setButtonText (tr ("devices.import"));
    addButton.onClick = [this] { addDevice(); };
    emptyAddButton.onClick = [this] { addDevice(); };
    emptyImportButton.onClick = [this] { importDevices(); };
    actionsButton.onClick = [this] { showActions(); };
    for (auto* b : { &addButton, &actionsButton, &emptyAddButton, &emptyImportButton })
        addAndMakeVisible (*b);

    appState.addChangeListener (this);
    setSize (width, height);
    rebuild();

    if (appState.getDeviceStore().getError().isNotEmpty())
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<DeviceManager> (this)]
        {
            if (safe != nullptr)
                safe->desktop.showMessage (tr ("devices.saveProblem"), safe->appState.getDeviceStore().getError());
        });
}

DeviceManager::~DeviceManager()
{
    appState.removeChangeListener (this);
}

void DeviceManager::changeListenerCallback (juce::ChangeBroadcaster*)
{
    rebuild();
}

void DeviceManager::rebuild()
{
    const auto& devices = appState.getPhysicalDevices();
    rows.clear();
    for (auto type : { DeviceType::converter, DeviceType::audioInterface, DeviceType::other })
    {
        if (filter && *filter != type)
            continue;
        std::vector<size_t> ofType;
        for (size_t i = 0; i < devices.size(); ++i)
            if (devices[i].deviceType == type)
                ofType.push_back (i);
        std::stable_sort (ofType.begin(), ofType.end(),
                          [&] (size_t a, size_t b) { return devices[a].sendChannelStart < devices[b].sendChannelStart; });
        if (! filter && ! ofType.empty())
            rows.push_back ({ true, type, 0 });
        for (auto i : ofType)
            rows.push_back ({ false, type, i });
    }

    const auto empty = devices.empty();
    listViewport.setVisible (! empty);
    emptyAddButton.setVisible (empty);
    emptyImportButton.setVisible (empty);
    list->updateSize (listViewport.getMaximumVisibleWidth());
    ports->refresh();
    repaint();
}

void DeviceManager::paint (juce::Graphics& g)
{
    g.fillAll (pal.windowPanel);
    g.setColour (pal.ink);

    if (appState.getPhysicalDevices().empty())
    {
        auto area = listArea.reduced (20, 80);
        g.setFont (Fonts::sans (13.0f, Fonts::Weight::semiBold));
        g.drawText (tr ("devices.emptyTitle"), area.removeFromTop (22), juce::Justification::centred);
        g.setColour (pal.inkSoft);
        g.setFont (Fonts::sans (12.0f, Fonts::Weight::regular));
        g.drawFittedText (tr ("devices.emptyHelp"), area.removeFromTop (48), juce::Justification::centredTop, 3);
    }
    else
    {
        filterTabs = Design::drawTabStrip (g, filterArea,
                                           { tr ("devices.filterAll"), DeviceStyle::deviceTypeName (DeviceType::converter),
                                             DeviceStyle::deviceTypeName (DeviceType::audioInterface),
                                             DeviceStyle::deviceTypeName (DeviceType::other) },
                                           filter ? static_cast<int> (*filter) + 1 : 0, pal, 1.0);
        g.setColour (pal.shadowEdge);
        g.drawRect (listArea.expanded (1), 1);
    }

    g.setColour (pal.shadowEdge);
    g.fillRect (portArea.getX() - margin / 2, portArea.getY(), 1, portArea.getHeight());
}

void DeviceManager::resized()
{
    auto header = juce::Rectangle<int> (margin, 6, width - 2 * margin, headerHeight - 10);
    actionsButton.setBounds (header.removeFromRight (90));
    header.removeFromRight (6);
    addButton.setBounds (header.removeFromRight (80));

    listViewport.setBounds (listArea);
    list->updateSize (listViewport.getMaximumVisibleWidth());
    ports->setBounds (portArea);

    const auto centre = juce::Point<int> (listArea.getCentreX(), listArea.getY() + 160);
    emptyAddButton.setBounds (centre.x - 106, centre.y, 100, 24);
    emptyImportButton.setBounds (centre.x + 6, centre.y, 100, 24);
}

void DeviceManager::mouseUp (const juce::MouseEvent& e)
{
    for (size_t i = 0; i < filterTabs.size(); ++i)
        if (filterTabs[i].contains (e.getPosition()))
        {
            filter = i == 0 ? std::nullopt : std::optional<DeviceType> (static_cast<DeviceType> (i - 1));
            rebuild();
            return;
        }
}

void DeviceManager::setPalette (const Design::Palette& p)
{
    OverlayContent::setPalette (p);
    ports->refresh();
}

// ============================================================================
// Actions
// ============================================================================

void DeviceManager::openEditor (const PhysicalDevice& device, bool isNew)
{
    auto editor = std::make_unique<DeviceEditor> (appState, device, isNew, [this] (std::function<void()> discard)
    {
        desktop.confirm (tr ("devices.discardTitle"), tr ("devices.discardDetail"), tr ("devices.discard"), std::move (discard));
    });
    desktop.openWindow (isNew ? tr ("devices.addTitle") : tr ("devices.editTitle"), std::move (editor),
                        DeviceEditor::width, DeviceEditor::height);
}

void DeviceManager::addDevice()
{
    PhysicalDevice d;
    d.name = tr ("devices.newName");
    d.shortName = tr ("devices.newShortName");
    d.portColor = ConnectorType::other;
    d.physicalChannelCount = 8;
    d.sendChannelStart = 1;
    d.recvChannelStart = 1;

    // Unlike the Swift app, a new device starts on the first free channels.
    for (auto direction : { ChannelDirection::send, ChannelDirection::recv })
        for (auto start : PhysicalDevices::freeStartChannels (d, appState.getPhysicalDevices(), direction))
        {
            (direction == ChannelDirection::send ? d.sendChannelStart : d.recvChannelStart) = start;
            const auto clash = PhysicalDevices::conflicts (d, appState.getPhysicalDevices());
            if (std::none_of (clash.begin(), clash.end(), [direction] (auto& c) { return c.direction == direction; }))
                break;
        }
    openEditor (d, true);
}

void DeviceManager::showRowMenu (const PhysicalDevice& device)
{
    juce::PopupMenu menu;
    menu.addItem (tr ("devices.edit"), [this, device] { openEditor (device, false); });
    menu.addItem (tr ("devices.duplicate"), [this, device] { openEditor (appState.duplicatePhysicalDevice (device), false); });
    menu.addSeparator();
    menu.addItem (tr ("devices.delete"), [this, device]
    {
        desktop.confirm (tr ("devices.deleteTitle"), tr ("devices.deleteDetail"), tr ("devices.delete"),
                         [this, id = device.id] { appState.deletePhysicalDevice (id); });
    });
    menu.setLookAndFeel (&getLookAndFeel());
    menu.showMenuAsync (juce::PopupMenu::Options());
}

void DeviceManager::showActions()
{
    juce::PopupMenu menu;
    menu.addItem (tr ("devices.export"), [this] { exportDevices(); });
    menu.addItem (tr ("devices.import"), [this] { importDevices(); });
    menu.addSeparator();
    menu.addItem (tr ("devices.revealFile"), [this] { appState.getDeviceStore().devicesFile().revealToUser(); });
    menu.addItem (tr ("devices.copyPath"), [this]
    {
        juce::SystemClipboard::copyTextToClipboard (appState.getDeviceStore().devicesFile().getFullPathName());
    });
    menu.addSeparator();
    menu.addItem (tr ("devices.removeAll"), ! appState.getPhysicalDevices().empty(), false, [this]
    {
        desktop.confirm (tr ("devices.removeAllTitle"), tr ("devices.removeAllDetail"), tr ("devices.removeAll"),
                         [this] { appState.replaceAllPhysicalDevices ({}); });
    });
    menu.setLookAndFeel (&getLookAndFeel());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&actionsButton));
}

void DeviceManager::exportDevices()
{
    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile ("StudioPatchbay Physical Devices.json");
    chooser = std::make_unique<juce::FileChooser> (tr ("devices.export"), suggested, "*.json");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
    {
        auto target = fc.getResult();
        if (target == juce::File())
            return;
        if (! target.hasFileExtension (".json"))
            target = target.withFileExtension (".json");
        if (! target.replaceWithText (PhysicalDevices::toJson (appState.getPhysicalDevices()), false, false, "\n"))
            desktop.showMessage (tr ("devices.importFailed"), tr ("devices.couldNotSave").replace ("%PATH%", target.getFullPathName()));
    });
}

void DeviceManager::importDevices()
{
    chooser = std::make_unique<juce::FileChooser> (tr ("devices.import"), juce::File(), "*.json");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File())
            return;
        std::vector<PhysicalDevice> imported;
        const auto result = PhysicalDevices::fromJson (file.loadFileAsString(), imported);
        if (result.failed())
        {
            desktop.showMessage (tr ("devices.importFailed"), tr ("devices.couldNotRead") + "\n" + result.getErrorMessage());
            return;
        }
        if (imported.empty())
        {
            desktop.showMessage (tr ("devices.importFailed"), tr ("devices.importEmpty"));
            return;
        }
        desktop.openWindow (tr ("devices.reviewTitle"), std::make_unique<ImportReview> (appState, std::move (imported)),
                            ImportReview::width, ImportReview::height);
    });
}

} // namespace StudioPatchbay
