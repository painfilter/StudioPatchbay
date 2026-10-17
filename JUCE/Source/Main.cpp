#include <juce_gui_basics/juce_gui_basics.h>
#include "MainWindow.h"

class StudioPatchbayApplication : public juce::JUCEApplication
{
public:
    StudioPatchbayApplication() = default;

    const juce::String getApplicationName() override    { return "StudioPatchbay"; }
    const juce::String getApplicationVersion() override { return SP_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise (const juce::String& /*commandLine*/) override
    {
        mainWindow = std::make_unique<StudioPatchbay::MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& /*commandLine*/) override {}

private:
    std::unique_ptr<StudioPatchbay::MainWindow> mainWindow;
};

START_JUCE_APPLICATION (StudioPatchbayApplication)
