#include "MainWindow.h"
#include "StudioPatchbayTokens.h"
#include "UI/Fonts.h"
#include "UI/AboutDialog.h"
#include "Model/AppSettings.h"

namespace StudioPatchbay
{

MainWindow::MainWindow (juce::String name)
    : juce::DocumentWindow (name,
                            juce::Colours::darkgrey,
                            juce::DocumentWindow::allButtons)
{
    // Apply bundled IBM Plex Sans to the LookAndFeel before any UI is drawn.
    Fonts::applyTo (os9Laf);

    // Set as both this window's and the global default LookAndFeel.
    setLookAndFeel (&os9Laf);
    juce::LookAndFeel::setDefaultLookAndFeel (&os9Laf);

    // Disable native title bar – the OS9LookAndFeel draws the Platinum
    // striped title bar via drawDocumentWindowTitleBar().
    setUsingNativeTitleBar (false);

    // Create the application content component.
    content = new AppContentComponent();

    // Wire up menu callbacks.
    content->getMenuBar().onFileMenu    = [this] { showFileMenu(); };
    content->getMenuBar().onViewMenu    = [this] { showViewMenu(); };
    content->getMenuBar().onOptionsMenu = [this] { showOptionsMenu(); };
    content->getMenuBar().onHelpMenu    = [this] { showHelpMenu(); };

    setContentOwned (content, false);

    // The last theme and zoom; dark on first launch.
    const auto settings = AppSettings::load();
    applyTheme (settings.darkMode);
    for (auto level : Design::Scale::windowZoomLevels)
        if (juce::approximatelyEqual (level, settings.uiScale))
            applyZoom (level);

    setResizable (true, true);
    centreWithSize (Tokens::baseWidth, Tokens::baseHeight);
    setVisible (true);

    // Register keyboard shortcuts.
    addKeyListener (this);
}

MainWindow::~MainWindow()
{
    removeKeyListener (this);
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

// ============================================================================
// Keyboard shortcuts
// ============================================================================

bool MainWindow::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    auto cmd = juce::ModifierKeys::commandModifier;

    // Cmd+R → Reload.
    if (key == juce::KeyPress ('r', cmd, 0))
    {
        content->getAppState().rescanInstallations();
        return true;
    }

    // Cmd+1 through Cmd+4 → switch views.
    if (key == juce::KeyPress ('1', cmd, 0))
    {
        content->switchToView (ViewSwitcher::View::cards);
        return true;
    }
    if (key == juce::KeyPress ('2', cmd, 0))
    {
        content->switchToView (ViewSwitcher::View::patchbay);
        return true;
    }
    if (key == juce::KeyPress ('3', cmd, 0))
    {
        content->switchToView (ViewSwitcher::View::controlRoom);
        return true;
    }
    if (key == juce::KeyPress ('4', cmd, 0))
    {
        content->switchToView (ViewSwitcher::View::signalFlow);
        return true;
    }

    return false;
}

// ============================================================================
// Menu implementations
// ============================================================================

void MainWindow::showFileMenu()
{
    juce::PopupMenu menu;
    
    juce::PopupMenu::Item reloadItem ("Reload Configuration");
    reloadItem.itemID = 1;
    reloadItem.shortcutKeyDescription = juce::String (juce::CharPointer_UTF8 ("\xe2\x8c\x98")) + "R";
    menu.addItem (reloadItem);
    
    menu.addSeparator();
    menu.addItem (2, "Export as PDF...");
    menu.addItem (3, "Export as PNG...");
    menu.addSeparator();
    
    juce::PopupMenu::Item quitItem ("Quit StudioPatchbay");
    quitItem.itemID = 99;
    quitItem.shortcutKeyDescription = juce::String (juce::CharPointer_UTF8 ("\xe2\x8c\x98")) + "Q";
    menu.addItem (quitItem);

    menu.showMenuAsync (juce::PopupMenu::Options(),
                        [this] (int result)
    {
        if (result == 1)
            content->getAppState().rescanInstallations();
        else if (result == 2)
            exportActiveView (Export::Format::pdf);
        else if (result == 3)
            exportActiveView (Export::Format::png);
        else if (result == 99)
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
    });
}

void MainWindow::exportActiveView (Export::Format format)
{
    const auto extension = format == Export::Format::pdf ? juce::String (".pdf") : juce::String (".png");
    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile ("StudioPatchbay " + AppContentComponent::viewName (content->getCurrentView())
                                              + extension);

    exportChooser = std::make_unique<juce::FileChooser> ("Export " + AppContentComponent::viewName (content->getCurrentView()),
                                                         suggested, "*" + extension);
    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;

    exportChooser->launchAsync (flags, [this, format, extension] (const juce::FileChooser& chooser)
    {
        auto target = chooser.getResult();
        if (target == juce::File())
            return;   // cancelled
        if (! target.hasFileExtension (extension))
            target = target.withFileExtension (extension);

        const auto result = Export::exportTo (content->activeExportSource(), format, target);
        if (result.failed())
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "Export failed", result.getErrorMessage());
    });
}

void MainWindow::showViewMenu()
{
    auto currentView = content->getCurrentView();

    juce::PopupMenu menu;
    
    juce::PopupMenu::Item cardsItem ("Cards");
    cardsItem.itemID = 1;
    cardsItem.isTicked = (currentView == ViewSwitcher::View::cards);
    cardsItem.shortcutKeyDescription = juce::String (juce::CharPointer_UTF8 ("\xe2\x8c\x98")) + "1";
    menu.addItem (cardsItem);
    
    juce::PopupMenu::Item patchbayItem ("Patchbay Grid");
    patchbayItem.itemID = 2;
    patchbayItem.isTicked = (currentView == ViewSwitcher::View::patchbay);
    patchbayItem.shortcutKeyDescription = juce::String (juce::CharPointer_UTF8 ("\xe2\x8c\x98")) + "2";
    menu.addItem (patchbayItem);
    
    juce::PopupMenu::Item controlRoomItem ("Control Room");
    controlRoomItem.itemID = 3;
    controlRoomItem.isTicked = (currentView == ViewSwitcher::View::controlRoom);
    controlRoomItem.shortcutKeyDescription = juce::String (juce::CharPointer_UTF8 ("\xe2\x8c\x98")) + "3";
    menu.addItem (controlRoomItem);
    
    juce::PopupMenu::Item signalFlowItem ("Signal Flow");
    signalFlowItem.itemID = 4;
    signalFlowItem.isTicked = (currentView == ViewSwitcher::View::signalFlow);
    signalFlowItem.shortcutKeyDescription = juce::String (juce::CharPointer_UTF8 ("\xe2\x8c\x98")) + "4";
    menu.addItem (signalFlowItem);

    menu.showMenuAsync (juce::PopupMenu::Options(),
                        [this] (int result)
    {
        switch (result)
        {
            case 1: content->switchToView (ViewSwitcher::View::cards); break;
            case 2: content->switchToView (ViewSwitcher::View::patchbay); break;
            case 3: content->switchToView (ViewSwitcher::View::controlRoom); break;
            case 4: content->switchToView (ViewSwitcher::View::signalFlow); break;
            default: break;
        }
    });
}

void MainWindow::showOptionsMenu()
{
    juce::PopupMenu zoomMenu;
    for (size_t i = 0; i < StudioPatchbay::Design::Scale::windowZoomLevels.size(); ++i)
    {
        auto level = StudioPatchbay::Design::Scale::windowZoomLevels[i];
        auto label = juce::String (juce::roundToInt (level * 100)) + "%";
        bool isCurrent = juce::approximatelyEqual (level, uiScale);
        zoomMenu.addItem (static_cast<int> (100 + i), label, true, isCurrent);
    }

    juce::PopupMenu themeMenu;
    themeMenu.addItem (200, "Dark Mode", true, content->isDarkMode());
    themeMenu.addItem (201, "Light Mode", true, ! content->isDarkMode());

    juce::PopupMenu menu;
    menu.addSubMenu ("Window Zoom", zoomMenu);
    menu.addSubMenu ("Theme", themeMenu);

    menu.showMenuAsync (juce::PopupMenu::Options(),
                        [this] (int result)
    {
        // Zoom levels: IDs 100–107
        if (result >= 100 && result < 200)
        {
            auto idx = static_cast<size_t> (result - 100);
            if (idx < StudioPatchbay::Design::Scale::windowZoomLevels.size())
                applyZoom (StudioPatchbay::Design::Scale::windowZoomLevels[idx]);
        }
        else if (result == 200 || result == 201)
        {
            applyTheme (result == 200);
        }
        else
        {
            return;
        }
        AppSettings { content->isDarkMode(), uiScale }.save();
    });
}

void MainWindow::applyTheme (bool dark)
{
    os9Laf.setDarkMode (dark);
    content->setDarkMode (dark);
    repaint();
}

void MainWindow::applyZoom (double scale)
{
    uiScale = scale;
    os9Laf.setUiScale (scale);
    content->setUiScale (scale);
    repaint();
}

void MainWindow::showHelpMenu()
{
    auto currentPal = content->isDarkMode()
        ? StudioPatchbay::Design::darkPalette()
        : StudioPatchbay::Design::lightPalette();

    juce::PopupMenu menu;
    menu.addItem (1, "About StudioPatchbay...");
    menu.addSeparator();
    menu.addItem (2, "Third-Party Licenses...");

    menu.showMenuAsync (juce::PopupMenu::Options(),
                        [currentPal] (int result)
    {
        if (result == 1 || result == 2)
            AboutDialog::show (currentPal);
    });
}

} // namespace StudioPatchbay
