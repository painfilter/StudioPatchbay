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
                                         ViewSwitcher::View::controlRoom };
constexpr int viewCount = 3;

// Menu item ids.
enum : int
{
    reloadId = 1, exportPdfId, exportPngId, quitId, exportViewPdfId, exportViewPngId,
    viewFirstId = 10,
    backId = 20, forwardId, zoomInId, zoomOutId, zoomResetId, findId,
    zoomFirstId = 100,
    languageSystemId = 200, languageEnglishId, languageGermanId,
    appearanceSystemId = 210, appearanceLightId, appearanceDarkId,
    aboutId = 300, licensesId,
    devicesId = 400
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
    content->getWorkspace().setContentScale (settings.contentScale);
    content->getWorkspace().onContentScaleChanged = [this] (double) { saveSettings(); };
    content->getWorkspace().onOpenSetup = [this] { showDeviceManager(); };
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

    // Start with the desktop focused, not the search field.
    content->setWantsKeyboardFocus (true);
    content->grabKeyboardFocus();
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
    settings.contentScale = content->getWorkspace().getContentScale();
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
    if (key == juce::KeyPress (',', cmd, 0))
    {
        showDeviceManager();
        return true;
    }
    if (content->isShaded())
        return false;

    auto& workspace = content->getWorkspace();
    for (int i = 0; i < viewCount; ++i)
        if (key == juce::KeyPress ('1' + i, cmd, 0))
        {
            workspace.switchToView (views[i]);
            return true;
        }
    if (key == juce::KeyPress ('[', cmd, 0)) { workspace.goBack(); return true; }
    if (key == juce::KeyPress (']', cmd, 0)) { workspace.goForward(); return true; }
    if (key == juce::KeyPress ('f', cmd, 0)) { workspace.focusSearch(); return true; }
    if (key == juce::KeyPress ('=', cmd, 0) || key == juce::KeyPress ('+', cmd, 0))
    {
        workspace.setContentScale (workspace.getContentScale() + 0.1);
        return true;
    }
    if (key == juce::KeyPress ('-', cmd, 0)) { workspace.setContentScale (workspace.getContentScale() - 0.1); return true; }
    if (key == juce::KeyPress ('0', cmd, 0)) { workspace.setContentScale (1.0); return true; }
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

        auto& workspace = content->getWorkspace();
        if (result >= viewFirstId && result < viewFirstId + viewCount)
            workspace.switchToView (views[result - viewFirstId]);
        else if (result >= zoomFirstId && result < zoomFirstId + static_cast<int> (Design::Scale::windowZoomLevels.size()))
            applyZoom (Design::Scale::windowZoomLevels[static_cast<size_t> (result - zoomFirstId)]);
        else switch (result)
        {
            case backId:             workspace.goBack(); return;
            case forwardId:          workspace.goForward(); return;
            case findId:             workspace.focusSearch(); return;
            case zoomInId:           workspace.setContentScale (workspace.getContentScale() + 0.1); return;
            case zoomOutId:          workspace.setContentScale (workspace.getContentScale() - 0.1); return;
            case zoomResetId:        workspace.setContentScale (1.0); return;
            case reloadId:           reload(); break;
            case exportPdfId:        exportView (Export::Format::pdf, std::nullopt); return;
            case exportPngId:        exportView (Export::Format::png, std::nullopt); return;
            case exportViewPdfId:    exportView (Export::Format::pdf, currentView()); return;
            case exportViewPngId:    exportView (Export::Format::png, currentView()); return;
            case quitId:             juce::JUCEApplication::getInstance()->systemRequestedQuit(); return;
            case languageSystemId:   applyLanguage ("system"); break;
            case languageEnglishId:  applyLanguage ("en"); break;
            case languageGermanId:   applyLanguage ("de"); break;
            case appearanceSystemId: settings.appearance = "system"; applyAppearance(); break;
            case appearanceLightId:  settings.appearance = "light"; applyAppearance(); break;
            case appearanceDarkId:   settings.appearance = "dark"; applyAppearance(); break;
            case aboutId:            showAbout(); return;
            case licensesId:         content->showLicenses(); return;
            case devicesId:          showDeviceManager(); return;
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
    const auto hasData = content->getAppState().hasParsedDevices();
    menu.addItem (item (exportPdfId, tr ("menu.exportPdf"), {}, false, hasData));
    menu.addItem (item (exportPngId, tr ("menu.exportPng"), {}, false, hasData));
    const auto viewName = ViewSwitcher::viewName (currentView());
    menu.addItem (item (exportViewPdfId, tr ("menu.exportViewPdf").replace ("%VIEW%", viewName), {}, false, hasData));
    menu.addItem (item (exportViewPngId, tr ("menu.exportViewPng").replace ("%VIEW%", viewName), {}, false, hasData));
    menu.addSeparator();
    menu.addItem (item (quitId, tr ("menu.quit"), "Q"));
    return menu;
}

juce::PopupMenu MainWindow::viewMenu()
{
    auto& workspace = content->getWorkspace();
    juce::PopupMenu menu;
    for (int i = 0; i < viewCount; ++i)
        menu.addItem (item (viewFirstId + i, ViewSwitcher::viewName (views[i]), juce::String (i + 1),
                            currentView() == views[i]));
    menu.addSeparator();
    menu.addItem (item (backId, tr ("menu.back"), "[", false, workspace.canGoBack()));
    menu.addItem (item (forwardId, tr ("menu.forward"), "]", false, workspace.canGoForward()));
    menu.addItem (item (findId, tr ("menu.find"), "F"));
    menu.addSeparator();
    const auto scale = workspace.getContentScale();
    menu.addItem (item (zoomInId, tr ("menu.zoomIn"), "+", false, scale < 2.0 - 1e-6));
    menu.addItem (item (zoomOutId, tr ("menu.zoomOut"), "-", false, scale > 0.6 + 1e-6));
    menu.addItem (item (zoomResetId, tr ("menu.actualSize"), "0", false, std::abs (scale - 1.0) > 1e-6));
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
    menu.addSeparator();
    menu.addItem (item (devicesId, tr ("menu.devices"), ","));
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

void MainWindow::exportView (Export::Format format, std::optional<ViewSwitcher::View> view)
{
    if (! content->getAppState().hasParsedDevices())
        return;

    const auto extension = format == Export::Format::pdf ? juce::String (".pdf") : juce::String (".png");
    const auto fileName = view ? "StudioPatchbay " + ViewSwitcher::viewName (*view) : juce::String ("StudioPatchbay");
    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile (fileName + extension);
    const auto title = view ? tr ("desktop.exportView").replace ("%VIEW%", ViewSwitcher::viewName (*view))
                            : tr ("menu.exportPatchbay");

    exportChooser = std::make_unique<juce::FileChooser> (title, suggested, "*" + extension);
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

        auto& workspace = content->getWorkspace();
        juce::Result result = juce::Result::ok();
        if (view)
            result = Export::exportTo (workspace.exportSource (*view), format, target);
        else
        {
            const auto sheet = workspace.makePatchbaySheet();
            result = Export::exportTo (sheet->source(), format, target);
        }
        if (result.failed())
        {
            content->showMessage (tr ("message.exportFailed"), result.getErrorMessage());
            return;
        }
        settings.rememberExport (target);
        saveSettings();
    });
}

juce::Result MainWindow::exportPatchbayTo (const juce::File& target)
{
    if (! content->getAppState().hasParsedDevices())
        return juce::Result::fail ("No External Plugins.xml was read; there is nothing to export.");
    const auto format = target.hasFileExtension (".png") ? Export::Format::png : Export::Format::pdf;
    const auto sheet = content->getWorkspace().makePatchbaySheet();
    return Export::exportTo (sheet->source(), format, target);
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

void MainWindow::showDeviceManager()
{
    if (content->isShaded())
        content->setShaded (false);
    content->showDeviceManager();
}

void MainWindow::restoreWindow (juce::Rectangle<int> fromIcon)
{
    content->setShaded (false, fromIcon);
}

} // namespace StudioPatchbay
