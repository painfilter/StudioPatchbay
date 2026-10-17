#include "MainWindow.h"
#include "StudioPatchbayTokens.h"
#include "UI/Fonts.h"
#include "Localisation/Text.h"

namespace StudioPatchbay
{

namespace
{
using Text::tr;
using Menu = OS9MenuBar::Menu;

const juce::String commandKey (juce::CharPointer_UTF8 ("\xe2\x8c\x98"));

juce::PopupMenu::Item item (int id, const juce::String& text, const juce::String& shortcut = {},
                            bool ticked = false, bool enabled = true)
{
    juce::PopupMenu::Item i (text);
    i.itemID = id;
    i.isTicked = ticked;
    i.isEnabled = enabled;
    if (shortcut.isNotEmpty())
        i.shortcutKeyDescription = commandKey + shortcut;
    return i;
}

constexpr ViewSwitcher::View views[] = { ViewSwitcher::View::cards, ViewSwitcher::View::patchbay,
                                         ViewSwitcher::View::controlRoom, ViewSwitcher::View::signalFlow };

// Menu item ids.
enum : int
{
    reloadId = 1, exportPdfId, exportPngId, quitId,
    viewFirstId = 10,
    zoomFirstId = 100,
    languageSystemId = 200, languageEnglishId, languageGermanId,
    appearanceSystemId = 210, appearanceLightId, appearanceDarkId,
    aboutId = 300, licensesId
};
} // namespace

MainWindow::MainWindow (juce::String name)
    : juce::DocumentWindow (name, juce::Colours::black, juce::DocumentWindow::allButtons)
{
    Fonts::applyTo (os9Laf);
    setLookAndFeel (&os9Laf);
    juce::LookAndFeel::setDefaultLookAndFeel (&os9Laf);
    setUsingNativeTitleBar (true);

    settings = AppSettings::load();
    Text::setLanguagePreference (settings.language);

    content = new AppContentComponent (settings.lastCubaseVersion);
    content->getMenuBar().onMenu = [this] (Menu m) { showMenu (m); };
    content->getDesktop().setActions (this);
    content->getDesktop().setRecentExports (settings.recentExports);
    content->onShadedChanged = [this] (bool shaded)
    {
        // The desktop is navigation, not a second window: the menus that act
        // on the shown data are off while the window is collapsed.
        content->getMenuBar().setMenuEnabled (Menu::file, ! shaded);
        content->getMenuBar().setMenuEnabled (Menu::view, ! shaded);
    };

    const auto width = settings.windowWidth > 0 ? settings.windowWidth : Tokens::baseWidth;
    const auto height = settings.windowHeight > 0 ? settings.windowHeight : Tokens::baseHeight;
    content->setSize (width, height);   // logical, at zoom 1 until applyZoom
    setContentOwned (content, true);

    juce::Desktop::getInstance().addDarkModeSettingListener (this);
    applyAppearance();

    auto zoom = 1.0;
    for (auto level : Design::Scale::windowZoomLevels)
        if (juce::approximatelyEqual (level, settings.uiScale))
            zoom = level;
    setResizable (true, false);
    applyZoom (zoom);

    centreWithSize (getWidth(), getHeight());
    setVisible (true);
    addKeyListener (this);
}

MainWindow::~MainWindow()
{
    saveSettings();
    removeKeyListener (this);
    juce::Desktop::getInstance().removeDarkModeSettingListener (this);
    clearContentComponent();
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

// ============================================================================
// Size and zoom
// ============================================================================

juce::Point<int> MainWindow::logicalSize() const
{
    const auto scale = content->getUiScale();
    return { juce::roundToInt (content->getWidth() / scale), juce::roundToInt (content->getHeight() / scale) };
}

void MainWindow::applyZoom (double scale)
{
    auto logical = logicalSize();
    logical.x = juce::jmax (logical.x, AppContentComponent::minimumWidth);
    logical.y = juce::jmax (logical.y, AppContentComponent::minimumHeight);

    // A larger zoom must not push the window off the screen: give up desktop
    // area first, down to the minimum, then move the window back on screen.
    const auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds());
    if (display != nullptr)
    {
        const auto area = display->userBounds.toNearestInt();
        logical.x = juce::jmax (AppContentComponent::minimumWidth,
                                juce::jmin (logical.x, juce::roundToInt (area.getWidth() / scale)));
        logical.y = juce::jmax (AppContentComponent::minimumHeight,
                                juce::jmin (logical.y, juce::roundToInt ((area.getHeight() - 40) / scale)));
    }

    os9Laf.setUiScale (scale);
    content->setUiScale (scale);
    setResizeLimits (juce::roundToInt (AppContentComponent::minimumWidth * scale),
                     juce::roundToInt (AppContentComponent::minimumHeight * scale), 16384, 16384);
    setContentComponentSize (juce::roundToInt (logical.x * scale), juce::roundToInt (logical.y * scale));
    if (display != nullptr && isOnDesktop())
        setTopLeftPosition (getBounds().constrainedWithin (display->userBounds.toNearestInt()).getPosition());
    repaint();
}

// ============================================================================
// Appearance and language
// ============================================================================

void MainWindow::applyAppearance()
{
    const auto dark = settings.appearance == "dark"
                      || (settings.appearance == "system" && juce::Desktop::getInstance().isDarkModeActive());
    os9Laf.setDarkMode (dark);
    content->setDarkMode (dark);
    repaint();
}

void MainWindow::darkModeSettingChanged()
{
    if (settings.appearance == "system")
        applyAppearance();
}

void MainWindow::applyLanguage (const juce::String& language)
{
    settings.language = language;
    Text::setLanguagePreference (language);
    content->refreshText();
}

void MainWindow::saveSettings()
{
    const auto logical = logicalSize();
    settings.uiScale = content->getUiScale();
    settings.windowWidth = logical.x;
    settings.windowHeight = logical.y;
    settings.lastCubaseVersion = content->getAppState().getPreferredVersion();
    settings.save();
    content->getDesktop().setRecentExports (settings.recentExports);
}

// ============================================================================
// Keyboard shortcuts
// ============================================================================

bool MainWindow::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    const auto cmd = juce::ModifierKeys::commandModifier;

    if (key == juce::KeyPress ('r', cmd, 0))
    {
        reload();
        return true;
    }
    if (key == juce::KeyPress ('q', cmd, 0))
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
        return true;
    }
    if (content->isShaded())
        return false;

    for (int i = 0; i < 4; ++i)
        if (key == juce::KeyPress ('1' + i, cmd, 0))
        {
            content->getWorkspace().switchToView (views[i]);
            return true;
        }
    return false;
}

// ============================================================================
// Menus
// ============================================================================

void MainWindow::showMenu (Menu which)
{
    auto menu = [&]
    {
        switch (which)
        {
            case Menu::file:    return fileMenu();
            case Menu::view:    return viewMenu();
            case Menu::options: return optionsMenu();
            case Menu::help:    break;
        }
        return helpMenu();
    }();

    // Detached popups do not reliably inherit the look and feel.
    menu.setLookAndFeel (&os9Laf);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&content->getMenuBar().head (which)),
                        [this] (int result)
    {
        content->getMenuBar().menuClosed();

        if (result >= viewFirstId && result < viewFirstId + 4)
            content->getWorkspace().switchToView (views[result - viewFirstId]);
        else if (result >= zoomFirstId && result < zoomFirstId + static_cast<int> (Design::Scale::windowZoomLevels.size()))
            applyZoom (Design::Scale::windowZoomLevels[static_cast<size_t> (result - zoomFirstId)]);
        else switch (result)
        {
            case reloadId:           reload(); break;
            case exportPdfId:        exportView (Export::Format::pdf, currentView()); break;
            case exportPngId:        exportView (Export::Format::png, currentView()); break;
            case quitId:             juce::JUCEApplication::getInstance()->systemRequestedQuit(); return;
            case languageSystemId:   applyLanguage ("system"); break;
            case languageEnglishId:  applyLanguage ("en"); break;
            case languageGermanId:   applyLanguage ("de"); break;
            case appearanceSystemId: settings.appearance = "system"; applyAppearance(); break;
            case appearanceLightId:  settings.appearance = "light"; applyAppearance(); break;
            case appearanceDarkId:   settings.appearance = "dark"; applyAppearance(); break;
            case aboutId:            showAbout(); return;
            case licensesId:         content->showLicenses(); return;
            default:                 return;
        }
        saveSettings();
    });
}

juce::PopupMenu MainWindow::fileMenu()
{
    juce::PopupMenu menu;
    menu.addItem (item (reloadId, tr ("menu.reload"), "R"));
    menu.addSeparator();
    menu.addItem (item (exportPdfId, tr ("menu.exportPdf")));
    menu.addItem (item (exportPngId, tr ("menu.exportPng")));
    menu.addSeparator();
    menu.addItem (item (quitId, tr ("menu.quit"), "Q"));
    return menu;
}

juce::PopupMenu MainWindow::viewMenu()
{
    juce::PopupMenu menu;
    for (int i = 0; i < 4; ++i)
        menu.addItem (item (viewFirstId + i, ViewSwitcher::viewName (views[i]), juce::String (i + 1),
                            currentView() == views[i]));
    return menu;
}

juce::PopupMenu MainWindow::optionsMenu()
{
    juce::PopupMenu zoom;
    for (size_t i = 0; i < Design::Scale::windowZoomLevels.size(); ++i)
    {
        const auto level = Design::Scale::windowZoomLevels[i];
        zoom.addItem (item (zoomFirstId + static_cast<int> (i), juce::String (juce::roundToInt (level * 100)) + " %",
                            {}, juce::approximatelyEqual (level, content->getUiScale())));
    }

    const auto language = Text::languagePreference();
    juce::PopupMenu languages;
    languages.addItem (item (languageSystemId, tr ("menu.languageSystem"), {}, language == "system"));
    languages.addItem (item (languageEnglishId, tr ("menu.languageEnglish"), {}, language == "en"));
    languages.addItem (item (languageGermanId, tr ("menu.languageGerman"), {}, language == "de"));

    juce::PopupMenu appearance;
    appearance.addItem (item (appearanceSystemId, tr ("menu.appearanceSystem"), {}, settings.appearance == "system"));
    appearance.addItem (item (appearanceLightId, tr ("menu.appearanceLight"), {}, settings.appearance == "light"));
    appearance.addItem (item (appearanceDarkId, tr ("menu.appearanceDark"), {}, settings.appearance == "dark"));

    juce::PopupMenu menu;
    menu.addSubMenu (tr ("menu.windowZoom"), zoom);
    menu.addSubMenu (tr ("menu.language"), languages);
    menu.addSubMenu (tr ("menu.appearance"), appearance);
    return menu;
}

juce::PopupMenu MainWindow::helpMenu()
{
    juce::PopupMenu menu;
    menu.addItem (item (aboutId, tr ("menu.about")));
    menu.addSeparator();
    menu.addItem (item (licensesId, tr ("menu.licenses")));
    return menu;
}

// ============================================================================
// Actions
// ============================================================================

void MainWindow::exportView (Export::Format format, ViewSwitcher::View view)
{
    const auto extension = format == Export::Format::pdf ? juce::String (".pdf") : juce::String (".png");
    const auto name = ViewSwitcher::viewName (view);
    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile ("StudioPatchbay " + name + extension);

    exportChooser = std::make_unique<juce::FileChooser> (tr ("desktop.exportView").replace ("%VIEW%", name),
                                                         suggested, "*" + extension);
    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;

    exportChooser->launchAsync (flags, [this, format, view, extension] (const juce::FileChooser& chooser)
    {
        auto target = chooser.getResult();
        if (target == juce::File())
            return;   // cancelled
        if (! target.hasFileExtension (extension))
            target = target.withFileExtension (extension);

        const auto result = Export::exportTo (content->getWorkspace().exportSource (view), format, target);
        if (result.failed())
        {
            content->showMessage (tr ("message.exportFailed"), result.getErrorMessage());
            return;
        }
        settings.rememberExport (target);
        saveSettings();
    });
}

ViewSwitcher::View MainWindow::currentView() const
{
    return content->getWorkspace().getCurrentView();
}

void MainWindow::reload()
{
    content->getAppState().rescanInstallations();
}

void MainWindow::showAbout()
{
    content->showAbout();
}

void MainWindow::forgetRecentExport (const juce::File& file)
{
    settings.recentExports.removeString (file.getFullPathName());
    saveSettings();
}

void MainWindow::restoreWindow (juce::Rectangle<int> fromIcon)
{
    content->setShaded (false, fromIcon);
}

} // namespace StudioPatchbay
