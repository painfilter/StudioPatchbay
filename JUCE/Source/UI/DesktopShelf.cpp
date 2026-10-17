#include "DesktopShelf.h"
#include "DesktopIcons.h"

namespace StudioPatchbay
{

namespace
{
const juce::Identifier viewDragKey ("studioPatchbayView");

juce::String revealLabel()
{
   #if JUCE_MAC
    return "Show in Finder";
   #elif JUCE_WINDOWS
    return "Show in Explorer";
   #else
    return "Show in File Manager";
   #endif
}

juce::String clockText (const juce::Time& t)
{
    const auto h24 = t.getHours();
    return juce::String (h24 % 12 == 0 ? 12 : h24 % 12) + ":"
           + juce::String (t.getMinutes()).paddedLeft ('0', 2) + (h24 < 12 ? " AM" : " PM");
}
} // namespace

DesktopShelf::DesktopShelf (AppState& state) : appState (state)
{
    setInterceptsMouseClicks (false, true);   // only the icons take clicks
    trash = std::make_unique<Design::DesktopTile> ("Trash", "", DesktopIcons::trash,
                                                   [this] { if (actions) actions->showAbout(); });
    addAndMakeVisible (*trash);
    appState.addChangeListener (this);
    lastRead = clockText (juce::Time::getCurrentTime());   // the files were read just now
    rebuild();
}

DesktopShelf::~DesktopShelf() { appState.removeChangeListener (this); }

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
    trash->setDarkMode (dark);
    for (auto& item : items)
        item.tile->setDarkMode (dark);
}

void DesktopShelf::setUiScale (double scale)
{
    uiScale = scale;
    trash->setUiScale (scale);
    for (auto& item : items)
        item.tile->setUiScale (scale);
}

// ============================================================================
// Icons
// ============================================================================

Design::DesktopTile& DesktopShelf::addTile (Item&& item, juce::String title, juce::String detail,
                                            Design::DesktopTile::IconPainter painter)
{
    const auto index = items.size();
    item.tile = std::make_unique<Design::DesktopTile> (std::move (title), std::move (detail),
                                                       std::move (painter),
                                                       [this, index] { activate (items[index]); });
    item.tile->onPopupMenu = [this, index] { showMenu (items[index]); };
    item.tile->setDarkMode (darkMode);
    item.tile->setUiScale (uiScale);
    addAndMakeVisible (*item.tile);
    items.push_back (std::move (item));
    return *items.back().tile;
}

void DesktopShelf::rebuild()
{
    dropTarget = nullptr;
    items.clear();
    items.reserve (appState.getInstallations().size() + 3 + static_cast<size_t> (recentExports.size()));

    const auto& installations = appState.getInstallations();
    for (size_t i = 0; i < installations.size(); ++i)
        addTile ({ Kind::installation, static_cast<int> (i), installations[i].prefsDir, {} },
                 installations[i].displayName(), {}, DesktopIcons::volume);

    addTile ({ Kind::exportPdf, -1, {}, {} }, "PDF Export", "Click, or drop a view tab here",
             DesktopIcons::printer);
    addTile ({ Kind::exportPng, -1, {}, {} }, "PNG Export", "Click, or drop a view tab here",
             DesktopIcons::camera);
    addTile ({ Kind::watcher, -1, {}, {} }, "Watcher", {},
             [this] (juce::Graphics& g, juce::Rectangle<int> r, bool dark)
             { DesktopIcons::watcher (g, r, dark, appState.isWatching()); });

    for (const auto& path : recentExports)
    {
        const juce::File file (path);
        if (! file.existsAsFile())
            continue;
        const auto kind = file.getFileExtension().trimCharactersAtStart (".");
        addTile ({ Kind::recent, -1, file, {} }, file.getFileNameWithoutExtension(),
                 kind.toUpperCase() + "  " + file.getLastModificationTime().toString (true, false),
                 [kind] (juce::Graphics& g, juce::Rectangle<int> r, bool dark)
                 { DesktopIcons::document (g, r, dark, kind); });
    }

    updateStates();
    if (auto* parent = getParentComponent())
        parent->resized();   // the number of icons decides the layout
}

void DesktopShelf::updateStates()
{
    for (auto& item : items)
    {
        if (item.kind == Kind::installation)
        {
            const auto& inst = appState.getInstallations()[static_cast<size_t> (item.installation)];
            item.tile->setSelected (item.installation == appState.getSelectedIndex());
            item.tile->setText (inst.displayName(), inst.xmlExists() ? "" : "No External Plugins");
        }
        else if (item.kind == Kind::watcher)
        {
            item.tile->setText ("Watcher", appState.isWatching() ? "Read " + lastRead : "Off");
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

void DesktopShelf::place (const DesktopLayout& layout)
{
    setBounds (layout.shelf);
    const auto origin = layout.shelf.getPosition();
    trash->setBounds (layout.trash - origin);
    for (size_t i = 0; i < items.size(); ++i)
    {
        const auto shown = i < layout.items.size();
        items[i].tile->setVisible (shown);
        if (shown)
            items[i].tile->setBounds (layout.items[i] - origin);
    }
}

// ============================================================================
// Actions
// ============================================================================

void DesktopShelf::activate (const Item& item)
{
    switch (item.kind)
    {
        case Kind::installation: appState.selectInstallation (item.installation); break;
        case Kind::exportPdf:    if (actions) actions->exportView (Export::Format::pdf, actions->currentView()); break;
        case Kind::exportPng:    if (actions) actions->exportView (Export::Format::png, actions->currentView()); break;
        case Kind::watcher:      if (actions) actions->reload(); break;
        case Kind::recent:       item.file.startAsProcess(); break;
    }
}

void DesktopShelf::showMenu (const Item& item)
{
    juce::PopupMenu menu;
    switch (item.kind)
    {
        case Kind::installation:
            menu.addItem ("Open", [this, i = item.installation] { appState.selectInstallation (i); });
            menu.addItem ("Show Preferences Folder", [f = item.file] { f.revealToUser(); });
            break;
        case Kind::exportPdf:
        case Kind::exportPng:
        {
            const auto format = item.kind == Kind::exportPdf ? Export::Format::pdf : Export::Format::png;
            for (auto view : { ViewSwitcher::View::cards, ViewSwitcher::View::patchbay,
                               ViewSwitcher::View::controlRoom, ViewSwitcher::View::signalFlow })
                menu.addItem ("Export " + ViewSwitcher::viewName (view),
                              [this, format, view] { if (actions) actions->exportView (format, view); });
            break;
        }
        case Kind::watcher:
            menu.addItem ("Reload Now", [this] { if (actions) actions->reload(); });
            menu.addItem ("Watch for Changes", true, appState.isWatching(), [this]
            {
                if (appState.isWatching()) appState.stopWatching(); else appState.startWatching();
                updateStates();
                repaint();
            });
            break;
        case Kind::recent:
            menu.addItem ("Open", [f = item.file] { f.startAsProcess(); });
            menu.addItem (revealLabel(), [f = item.file] { f.revealToUser(); });
            menu.addSeparator();
            menu.addItem ("Remove from Desktop", [this, f = item.file]
            {
                // Deferred: removing the icon rebuilds the shelf, and the
                // menu's own callback must not run inside a destroyed tile.
                juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<DesktopShelf> (this), f]
                {
                    if (safe != nullptr && safe->actions != nullptr)
                        safe->actions->forgetRecentExport (f);
                });
            });
            break;
    }
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (item.tile.get()));
}

// ============================================================================
// Drag and drop: a view tab onto an export program
// ============================================================================

juce::var DesktopShelf::describeViewDrag (ViewSwitcher::View view)
{
    auto* object = new juce::DynamicObject();
    object->setProperty (viewDragKey, static_cast<int> (view));
    return juce::var (object);
}

std::optional<ViewSwitcher::View> DesktopShelf::viewFromDrag (const juce::var& description)
{
    if (auto* object = description.getDynamicObject(); object != nullptr && object->hasProperty (viewDragKey))
        return static_cast<ViewSwitcher::View> (static_cast<int> (object->getProperty (viewDragKey)));
    return std::nullopt;
}

DesktopShelf::Item* DesktopShelf::exportItemAt (juce::Point<int> position)
{
    for (auto& item : items)
        if ((item.kind == Kind::exportPdf || item.kind == Kind::exportPng)
            && item.tile->isVisible() && item.tile->getBounds().contains (position))
            return &item;
    return nullptr;
}

void DesktopShelf::setDropHighlight (Item* target)
{
    if (dropTarget == target)
        return;
    if (dropTarget != nullptr)
        dropTarget->tile->setSelected (false);
    dropTarget = target;
    if (dropTarget != nullptr)
        dropTarget->tile->setSelected (true);
}

bool DesktopShelf::isInterestedInDragSource (const SourceDetails& details)
{
    return viewFromDrag (details.description).has_value();
}

void DesktopShelf::itemDragMove (const SourceDetails& details)
{
    setDropHighlight (exportItemAt (details.localPosition));
}

void DesktopShelf::itemDragExit (const SourceDetails&)
{
    setDropHighlight (nullptr);
}

void DesktopShelf::itemDropped (const SourceDetails& details)
{
    auto* target = exportItemAt (details.localPosition);
    setDropHighlight (nullptr);
    const auto view = viewFromDrag (details.description);
    if (target == nullptr || ! view || actions == nullptr)
        return;
    actions->exportView (target->kind == Kind::exportPdf ? Export::Format::pdf : Export::Format::png, *view);
}

} // namespace StudioPatchbay
