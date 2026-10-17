#include "DesktopShelf.h"
#include "DesktopIcons.h"
#include "Localisation/Text.h"
#include "BinaryData.h"

namespace StudioPatchbay
{

namespace
{
using Text::tr;

juce::String revealLabel()
{
   #if JUCE_MAC
    return tr ("desktop.showInFinder");
   #elif JUCE_WINDOWS
    return tr ("desktop.showInExplorer");
   #else
    return tr ("desktop.showInFileManager");
   #endif
}

juce::String clockText (const juce::Time& t)
{
    if (Text::german())
        return t.formatted ("%H:%M");
    const auto h24 = t.getHours();
    return juce::String (h24 % 12 == 0 ? 12 : h24 % 12) + ":"
           + juce::String (t.getMinutes()).paddedLeft ('0', 2) + (h24 < 12 ? " AM" : " PM");
}

constexpr int tileW = Design::Metrics::desktopTileWidth;
constexpr int tileH = Design::Metrics::desktopTileHeight;
constexpr int margin = 32;
constexpr int bottomInset = 24;
} // namespace

DesktopShelf::DesktopShelf (AppState& state) : appState (state)
{
    setInterceptsMouseClicks (false, true);   // only the icons take clicks
    appIcon = juce::ImageCache::getFromMemory (BinaryData::StudioPatchbayIcon_png,
                                                BinaryData::StudioPatchbayIcon_pngSize);

    appTile = std::make_unique<Design::DesktopTile> ("StudioPatchbay", "",
        [this] (juce::Graphics& g, juce::Rectangle<int> r, bool)
        {
            if (appIcon.isValid())
                g.drawImageWithin (appIcon, r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                                   juce::RectanglePlacement::centred);
        },
        [this] { if (actions) actions->restoreWindow (appIconBounds()); });
    trash = std::make_unique<Design::DesktopTile> ("", "", DesktopIcons::trash,
                                                   [this] { if (actions) actions->showAbout(); });
    for (auto* tile : { appTile.get(), trash.get() })
        prepare (*tile);

    appState.addChangeListener (this);
    lastRead = clockText (juce::Time::getCurrentTime());   // the files were read just now
    rebuild();
}

DesktopShelf::~DesktopShelf() { appState.removeChangeListener (this); }

void DesktopShelf::prepare (Design::DesktopTile& tile)
{
    tile.setDarkMode (darkMode);
    tile.setUiScale (uiScale);
    addAndMakeVisible (tile);
}

void DesktopShelf::setRecentExports (const juce::StringArray& paths)
{
    if (paths == recentExports)
        return;
    recentExports = paths;
    rebuild();
}

void DesktopShelf::setDarkMode (bool dark)
{
    darkMode = dark;
    appTile->setDarkMode (dark);
    trash->setDarkMode (dark);
    for (auto& item : items)
        item.tile->setDarkMode (dark);
}

void DesktopShelf::setUiScale (double scale)
{
    uiScale = scale;
    appTile->setUiScale (scale);
    trash->setUiScale (scale);
    for (auto& item : items)
        item.tile->setUiScale (scale);
    resized();
}

void DesktopShelf::refreshText()
{
    rebuild();
}

// ============================================================================
// Icons
// ============================================================================

void DesktopShelf::addTile (Item&& item, Design::DesktopTile::IconPainter painter)
{
    const auto index = items.size();
    item.tile = std::make_unique<Design::DesktopTile> ("", "", std::move (painter),
                                                       [this, index] { activate (items[index]); });
    item.tile->onPopupMenu = [this, index] { showMenu (items[index]); };
    prepare (*item.tile);
    items.push_back (std::move (item));
}

void DesktopShelf::rebuild()
{
    items.clear();
    items.reserve (appState.getInstallations().size() + 3 + static_cast<size_t> (recentExports.size()));

    const auto& installations = appState.getInstallations();
    for (size_t i = 0; i < installations.size(); ++i)
        addTile ({ Kind::installation, static_cast<int> (i), installations[i].prefsDir, {} }, DesktopIcons::volume);

    addTile ({ Kind::devices, -1, {}, {} }, DesktopIcons::converter);
    addTile ({ Kind::exportPdf, -1, {}, {} }, DesktopIcons::printer);
    addTile ({ Kind::exportPng, -1, {}, {} }, DesktopIcons::camera);
    addTile ({ Kind::watcher, -1, {}, {} },
             [this] (juce::Graphics& g, juce::Rectangle<int> r, bool dark)
             { DesktopIcons::watcher (g, r, dark, appState.isWatching()); });

    for (const auto& path : recentExports)
    {
        const juce::File file (path);
        if (! file.existsAsFile())
            continue;
        const auto kind = file.getFileExtension().trimCharactersAtStart (".");
        addTile ({ Kind::recent, -1, file, {} },
                 [kind] (juce::Graphics& g, juce::Rectangle<int> r, bool dark)
                 { DesktopIcons::document (g, r, dark, kind); });
    }

    updateStates();
    resized();
}

void DesktopShelf::updateStates()
{
    trash->setText (tr ("ui.trash"), {});
    const auto* selected = appState.getSelectedInstallation();
    appTile->setText ("StudioPatchbay", selected != nullptr ? selected->displayName() : tr ("desktop.noInstallation"));
    appTile->setSelected (true);

    for (auto& item : items)
    {
        switch (item.kind)
        {
            case Kind::installation:
            {
                const auto& inst = appState.getInstallations()[static_cast<size_t> (item.installation)];
                item.tile->setSelected (item.installation == appState.getSelectedIndex());
                item.tile->setText (inst.displayName(), inst.xmlExists() ? "" : tr ("desktop.noExternalPlugins"));
                break;
            }
            case Kind::devices:
                item.tile->setText (tr ("devices.title"),
                                    tr ("desktop.devicesCount").replace ("%COUNT%",
                                        juce::String (static_cast<int> (appState.getPhysicalDevices().size()))));
                break;
            case Kind::exportPdf: item.tile->setText (tr ("desktop.pdfExport"), tr ("desktop.exportDetail")); break;
            case Kind::exportPng: item.tile->setText (tr ("desktop.pngExport"), tr ("desktop.exportDetail")); break;
            case Kind::watcher:
                item.tile->setText (tr ("desktop.watcher"),
                                    appState.isWatching() ? tr ("desktop.watcherRead").replace ("%TIME%", lastRead)
                                                          : tr ("desktop.watcherOff"));
                break;
            case Kind::recent:
                item.tile->setText (item.file.getFileNameWithoutExtension(),
                                    item.file.getFileExtension().trimCharactersAtStart (".").toUpperCase() + "  "
                                        + item.file.getLastModificationTime().toString (true, false));
                break;
        }
    }
}

void DesktopShelf::changeListenerCallback (juce::ChangeBroadcaster*)
{
    lastRead = clockText (juce::Time::getCurrentTime());

    auto count = 0;
    for (const auto& item : items)
        count += item.kind == Kind::installation ? 1 : 0;

    if (count != static_cast<int> (appState.getInstallations().size()))
        rebuild();
    else
        updateStates();
}

// ============================================================================
// Layout
// ============================================================================

juce::Rectangle<int> DesktopShelf::gridSlot (int index, int logicalWidth)
{
    const auto step = Design::Metrics::desktopTileHorizontalStep;
    const auto columns = juce::jmax (1, (logicalWidth - 2 * margin - tileW) / step + 1);
    return { margin + (index % columns) * step,
             54 + (index / columns) * Design::Metrics::desktopTileVerticalStep, tileW, tileH };
}

void DesktopShelf::resized()
{
    const auto logicalW = juce::roundToInt (getWidth() / uiScale);
    const auto logicalH = juce::roundToInt (getHeight() / uiScale);
    const auto scaled = [this] (juce::Rectangle<int> r)
    { return juce::Rectangle<int> (sc (r.getX()), sc (r.getY()), sc (r.getWidth()), sc (r.getHeight())); };

    appTile->setBounds (scaled ({ margin, logicalH - bottomInset - tileH, tileW, tileH }));
    trash->setBounds (scaled ({ logicalW - margin - tileW, logicalH - bottomInset - tileH, tileW, tileH }));

    // Grid tiles stop above the bottom row; what does not fit is not shown.
    const auto lastTop = logicalH - bottomInset - tileH - Design::Metrics::desktopTileVerticalStep;
    for (size_t i = 0; i < items.size(); ++i)
    {
        const auto slot = gridSlot (static_cast<int> (i), logicalW);
        items[i].tile->setVisible (slot.getY() <= lastTop);
        items[i].tile->setBounds (scaled (slot));
    }
}

juce::Rectangle<int> DesktopShelf::appIconBounds() const
{
    return appTile->getIconBoundsInParent();
}

// ============================================================================
// Actions
// ============================================================================

void DesktopShelf::activate (const Item& item)
{
    switch (item.kind)
    {
        case Kind::installation:
        {
            // Read first: selecting may rebuild the tiles.
            const auto from = item.tile->getIconBoundsInParent();
            appState.selectInstallation (item.installation);
            if (actions) actions->restoreWindow (from);
            break;
        }
        case Kind::devices:   if (actions) actions->showDeviceManager(); break;
        case Kind::exportPdf: if (actions) actions->exportView (Export::Format::pdf, actions->currentView()); break;
        case Kind::exportPng: if (actions) actions->exportView (Export::Format::png, actions->currentView()); break;
        case Kind::watcher:   if (actions) actions->reload(); break;
        case Kind::recent:    item.file.startAsProcess(); break;
    }
}

void DesktopShelf::showMenu (const Item& item)
{
    juce::PopupMenu menu;
    switch (item.kind)
    {
        case Kind::installation:
            menu.addItem (tr ("desktop.open"), [this, i = item.installation, from = item.tile->getIconBoundsInParent()]
            {
                appState.selectInstallation (i);
                if (actions) actions->restoreWindow (from);
            });
            menu.addItem (tr ("desktop.showPreferencesFolder"), [f = item.file] { f.revealToUser(); });
            break;
        case Kind::devices:
            menu.addItem (tr ("desktop.open"), [this] { if (actions) actions->showDeviceManager(); });
            break;
        case Kind::exportPdf:
        case Kind::exportPng:
        {
            const auto format = item.kind == Kind::exportPdf ? Export::Format::pdf : Export::Format::png;
            for (auto view : { ViewSwitcher::View::cards, ViewSwitcher::View::patchbay,
                               ViewSwitcher::View::controlRoom, ViewSwitcher::View::signalFlow })
                menu.addItem (tr ("desktop.exportView").replace ("%VIEW%", ViewSwitcher::viewName (view)),
                              [this, format, view] { if (actions) actions->exportView (format, view); });
            break;
        }
        case Kind::watcher:
            menu.addItem (tr ("desktop.reloadNow"), [this] { if (actions) actions->reload(); });
            menu.addItem (tr ("desktop.watchForChanges"), true, appState.isWatching(), [this]
            {
                if (appState.isWatching()) appState.stopWatching(); else appState.startWatching();
                updateStates();
            });
            break;
        case Kind::recent:
            menu.addItem (tr ("desktop.open"), [f = item.file] { f.startAsProcess(); });
            menu.addItem (revealLabel(), [f = item.file] { f.revealToUser(); });
            menu.addSeparator();
            menu.addItem (tr ("desktop.removeFromDesktop"), [this, f = item.file]
            {
                // Deferred: removing the icon rebuilds the tiles, and the
                // menu's own callback must not run inside a destroyed tile.
                juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<DesktopShelf> (this), f]
                {
                    if (safe != nullptr && safe->actions != nullptr)
                        safe->actions->forgetRecentExport (f);
                });
            });
            break;
    }
    menu.setLookAndFeel (&getLookAndFeel());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (item.tile.get()));
}

} // namespace StudioPatchbay
