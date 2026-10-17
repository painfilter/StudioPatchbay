#include <juce_gui_basics/juce_gui_basics.h>
#include "MainWindow.h"

#include <iostream>

class StudioPatchbayApplication : public juce::JUCEApplication
{
public:
    StudioPatchbayApplication() = default;

    const juce::String getApplicationName() override    { return "StudioPatchbay"; }
    const juce::String getApplicationVersion() override { return SP_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise (const juce::String& commandLine) override
    {
        mainWindow = std::make_unique<StudioPatchbay::MainWindow> (getApplicationName());

        // "--export <file.pdf|file.png>" writes the patchbay sheet and quits,
        // for scripts and printing without the window.
        const auto args = juce::StringArray::fromTokens (commandLine, true);
        const auto index = args.indexOf ("--export");
        if (index >= 0 && index + 1 < args.size())
        {
            const juce::File target (juce::File::getCurrentWorkingDirectory().getChildFile (args[index + 1].unquoted()));
            const auto result = mainWindow->exportPatchbayTo (target);
            if (result.failed())
                std::cerr << result.getErrorMessage() << std::endl;
            setApplicationReturnValue (result.wasOk() ? 0 : 1);
            quit();
        }
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
