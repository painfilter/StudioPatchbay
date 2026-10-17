#include "AppContentComponent.h"
#include "UI/Fonts.h"
#include "Localisation/Text.h"
#include "BinaryData.h"

namespace StudioPatchbay
{

namespace
{
using Text::tr;

constexpr double shadeAnimationMs = 220.0;

std::unique_ptr<juce::Drawable> logo (const char* data, int size)
{
    return juce::Drawable::createFromSVGString (juce::String::fromUTF8 (data, size));
}

void frame (juce::Graphics& g, juce::Rectangle<int> r, juce::Colour c, int thickness)
{
    g.setColour (c);
    for (int i = 0; i < thickness && ! r.isEmpty(); ++i, r = r.reduced (1))
    {
        g.fillRect (r.getX(), r.getY(), r.getWidth(), 1);
        g.fillRect (r.getX(), r.getBottom() - 1, r.getWidth(), 1);
        g.fillRect (r.getX(), r.getY(), 1, r.getHeight());
        g.fillRect (r.getRight() - 1, r.getY(), 1, r.getHeight());
    }
}

juce::Rectangle<int> interpolate (juce::Rectangle<int> from, juce::Rectangle<int> to, double amount)
{
    const auto mix = [amount] (int a, int b) { return juce::roundToInt (a + (b - a) * amount); };
    return { mix (from.getX(), to.getX()), mix (from.getY(), to.getY()),
             mix (from.getWidth(), to.getWidth()), mix (from.getHeight(), to.getHeight()) };
}

// The third-party notices, readable and scrollable inside an overlay window.
class LicenseText : public juce::Component
{
public:
    LicenseText()
    {
        text.setMultiLine (true);
        text.setReadOnly (true);
        text.setScrollbarsShown (true);
        text.setCaretVisible (false);
        text.setBorder (juce::BorderSize<int> (1, 1, 1, 0));
        text.setFont (Fonts::sans (12.0f, Fonts::Weight::regular));
        text.setText (juce::String::fromUTF8 (BinaryData::THIRD_PARTY_LICENSES_txt,
                                              BinaryData::THIRD_PARTY_LICENSES_txtSize), false);
        text.moveCaretToTop (false);
        addAndMakeVisible (text);
    }
    void resized() override { text.setBounds (getLocalBounds().reduced (12)); }
    juce::TextEditor text;
};
} // namespace

AppContentComponent::AppContentComponent (int preferredCubaseVersion)
    : appState (preferredCubaseVersion),
      about ([this] { about.setVisible (false); },
             logo (BinaryData::PainfilterStackedLight_svg, BinaryData::PainfilterStackedLight_svgSize),
             logo (BinaryData::PainfilterStackedDark_svg, BinaryData::PainfilterStackedDark_svgSize)),
      message ([this] { message.setVisible (false); })
{
    addAndMakeVisible (appWindow);
    addAndMakeVisible (menuBar);
    addChildComponent (desktop);
    addChildComponent (about);
    addChildComponent (message);

    appWindow.onCollapse = [this] { setShaded (true); };
    appState.addChangeListener (&titleWatcher);

    refreshText();
}

AppContentComponent::~AppContentComponent()
{
    appState.removeChangeListener (&titleWatcher);
}

juce::Rectangle<int> AppContentComponent::windowBounds() const
{
    // Below the menu bar with a 6 px gap, 8 px from the other edges.
    const auto top = sc (Design::Metrics::menuBarHeight + 6);
    return { sc (8), top, getWidth() - sc (16), getHeight() - top - sc (8) };
}

// ============================================================================
// Paint
// ============================================================================

void AppContentComponent::paint (juce::Graphics& g)
{
    const auto window = windowBounds();
    const auto windowShown = appWindow.isVisible();

    // The desktop shows only around the window: paint it there and nowhere
    // else, so the clock's small repaint stays small.
    juce::Graphics::ScopedSaveState state (g);
    if (windowShown)
        g.excludeClipRegion (window);
    g.fillAll (pal.desktop);

    Design::DesktopPattern pattern;
    pattern.top = sc (Design::Metrics::menuBarHeight);
    pattern.step = juce::jmax (1, sc (4));
    pattern.rowOffset = sc (2);
    pattern.dotSize = juce::jmax (1, sc (1));
    Design::paintDesktopPattern (g, pal.desktop.brighter (darkMode ? 0.08f : 0.12f), pattern,
                                 windowShown ? window : juce::Rectangle<int>());

    if (windowShown)
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRect (window.translated (sc (4), sc (4)).getIntersection (getLocalBounds()));
    }
}

void AppContentComponent::paintOverChildren (juce::Graphics& g)
{
    if (! animating)
        return;

    // ZoomRects: the outline travels between window and icon, trailed by two
    // earlier outlines. Only outlines; nothing is filled.
    const auto elapsed = juce::Time::getMillisecondCounterHiRes() - animationStartMs;
    const auto progress = juce::jlimit (0.0, 1.0, elapsed / shadeAnimationMs);
    const auto edge = juce::jmax (1, sc (1));
    for (int trail = 0; trail < 3; ++trail)
    {
        const auto step = juce::jlimit (0.0, 1.0, progress - trail * 0.12);
        const auto amount = animatingToShaded ? step : 1.0 - step;
        frame (g, interpolate (windowBounds(), animationIcon, amount), pal.ink, edge);
    }
}

// ============================================================================
// Layout
// ============================================================================

void AppContentComponent::resized()
{
    menuBar.setBounds (0, 0, getWidth(), sc (Design::Metrics::menuBarHeight));
    appWindow.setBounds (windowBounds());
    desktop.setBounds (getLocalBounds());
    for (juce::Component* overlay : { static_cast<juce::Component*> (&about),
                                      static_cast<juce::Component*> (&message),
                                      static_cast<juce::Component*> (licenses.get()) })
        overlay->setBounds (getLocalBounds());
}

// ============================================================================
// WindowShade
// ============================================================================

void AppContentComponent::setShaded (bool shouldBeShaded, juce::Rectangle<int> icon)
{
    if (shaded == shouldBeShaded && ! animating)
        return;

    animationIcon = icon.isEmpty() ? desktop.appIconBounds() : icon;
    animatingToShaded = shouldBeShaded;
    animationStartMs = juce::Time::getMillisecondCounterHiRes();
    animating = true;
    startTimerHz (60);

    if (shouldBeShaded)
    {
        // The window goes at once and the outline shrinks into the icon.
        about.setVisible (false);
        message.setVisible (false);
        licenses->setVisible (false);
        shaded = true;
        appWindow.setVisible (false);
        desktop.setVisible (true);
        menuBar.setShaded (true);
        if (onShadedChanged)
            onShadedChanged (true);
        animationIcon = icon.isEmpty() ? desktop.appIconBounds() : icon;
    }
    // Restoring keeps the desktop until the outline has reached the window,
    // so the window never appears ahead of its animation.
    repaint();
}

void AppContentComponent::timerCallback()
{
    if (juce::Time::getMillisecondCounterHiRes() - animationStartMs >= shadeAnimationMs)
        finishAnimation();
    repaint();
}

void AppContentComponent::finishAnimation()
{
    stopTimer();
    animating = false;
    if (! animatingToShaded && shaded)
    {
        shaded = false;
        desktop.setVisible (false);
        appWindow.setVisible (true);
        menuBar.setShaded (false);
        if (onShadedChanged)
            onShadedChanged (false);
    }
}

// ============================================================================
// Overlays
// ============================================================================

void AppContentComponent::showAbout()
{
    Design::AboutDialog::Content c;
    c.title = tr ("ui.aboutTitle");
    c.productName = "StudioPatchbay";
    c.tagline = tr ("ui.aboutTagline");
    c.versionLine = "Version " + juce::String (SP_VERSION_STRING);
    c.architectureLine = tr ("ui.runningOn") + " " + juce::SystemStats::getOperatingSystemName()
                         + (juce::SystemStats::getCpuModel().containsIgnoreCase ("Apple") ? " (Apple Silicon)" : "");
    c.publisher = "Painfilter Software & Music Production";
    c.copyright = "Copyright 2026 Jan Nordus";
    c.websiteLabel = "painfilter.de";
    c.websiteUrl = juce::URL ("https://www.painfilter.de");
    c.emailLabel = "info@painfilter.de";
    c.emailUrl = juce::URL ("mailto:info@painfilter.de");
    c.okText = tr ("action.ok");
    c.closeTooltip = tr ("action.close");
    c.licensingLine = tr ("ui.licensing");
    about.setContent (std::move (c));
    about.setVisible (true);
    about.toFront (true);
    about.grabKeyboardFocus();
}

void AppContentComponent::showLicenses()
{
    licenses->setVisible (true);
    licenses->toFront (true);
    licenses->grabKeyboardFocus();
}

void AppContentComponent::showMessage (const juce::String& title, const juce::String& text)
{
    message.setContent (title, text, tr ("action.ok"), tr ("action.close"));
    message.setVisible (true);
    message.toFront (true);
    message.grabKeyboardFocus();
}

// ============================================================================
// Appearance
// ============================================================================

void AppContentComponent::setDarkMode (bool dark)
{
    darkMode = dark;
    pal = dark ? Design::darkPalette() : Design::lightPalette();
    menuBar.setPalette (pal);
    appWindow.setPalette (pal);
    workspace.setPalette (pal);
    desktop.setDarkMode (dark);
    about.setDarkMode (dark);
    message.setDarkMode (dark);
    licenses->setDarkMode (dark);
    if (auto* text = dynamic_cast<LicenseText*> (licenses->getContent()))
    {
        text->text.setColour (juce::TextEditor::backgroundColourId, pal.field);
        text->text.setColour (juce::TextEditor::textColourId, pal.ink);
        text->text.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        text->text.applyColourToAllText (pal.ink, false);
    }
    repaint();
}

void AppContentComponent::setUiScale (double scale)
{
    uiScale = scale;
    menuBar.setUiScale (scale);
    appWindow.setUiScale (scale);
    workspace.setUiScale (scale);
    desktop.setUiScale (scale);
    about.setUiScale (scale);
    message.setUiScale (scale);
    licenses->setUiScale (scale);
    resized();
    repaint();
}

void AppContentComponent::refreshText()
{
    menuBar.refreshText();
    workspace.refreshText();
    desktop.refreshText();
    appWindow.setCollapseTooltip (tr ("ui.collapseWindow"));
    licenses = [this]
    {
        // The title is fixed at construction; rebuild keeps the window simple.
        const auto wasVisible = licenses != nullptr && licenses->isVisible();
        auto window = std::make_unique<Design::OverlayWindow> (tr ("label.thirdPartyLicenses"),
                                                               std::make_unique<LicenseText>(), 560, 380);
        window->onClose = [this] { licenses->setVisible (false); };
        window->setCloseTooltip (tr ("action.close"));
        window->setVisible (wasVisible);
        return window;
    }();
    addChildComponent (*licenses);
    licenses->setUiScale (uiScale);
    licenses->setBounds (getLocalBounds());
    setDarkMode (darkMode);
    if (about.isVisible())
        showAbout();
    updateTitle();
}

void AppContentComponent::updateTitle()
{
    juce::String title ("StudioPatchbay");
    if (auto* inst = appState.getSelectedInstallation())
        title << " - " << inst->displayName();
    appWindow.setTitle (title);
}

} // namespace StudioPatchbay
