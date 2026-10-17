#include <catch2/catch_test_macros.hpp>
#include "Model/AppSettings.h"

using StudioPatchbay::AppSettings;

namespace
{
struct TempSettings
{
    juce::File file = juce::File::createTempFile (".settings");
    ~TempSettings() { file.deleteFile(); }

    void write (const juce::String& xml) { file.replaceWithText (xml); }
};
} // namespace

TEST_CASE ("AppSettings: version 1 darkMode becomes appearance", "[settings]")
{
    TempSettings temp;
    temp.write ("<?xml version=\"1.0\"?><PROPERTIES>"
                "<VALUE name=\"formatVersion\" val=\"1\"/>"
                "<VALUE name=\"darkMode\" val=\"0\"/>"
                "<VALUE name=\"uiScale\" val=\"1.25\"/>"
                "</PROPERTIES>");

    const auto settings = AppSettings::load (temp.file);
    CHECK (settings.appearance == "light");
    CHECK (settings.language == "system");
    CHECK (settings.uiScale == 1.25);

    settings.save (temp.file);
    const auto xml = temp.file.loadFileAsString();
    CHECK (! xml.contains ("darkMode"));
    CHECK (xml.contains ("\"formatVersion\" val=\"2\""));
}

TEST_CASE ("AppSettings: round trip", "[settings]")
{
    TempSettings temp;
    AppSettings settings;
    settings.appearance = "dark";
    settings.language = "de";
    settings.uiScale = 1.5;
    settings.windowWidth = 1000;
    settings.windowHeight = 720;
    settings.lastCubaseVersion = 14;
    settings.rememberExport (juce::File::getCurrentWorkingDirectory().getChildFile ("a.pdf"));
    settings.save (temp.file);

    const auto loaded = AppSettings::load (temp.file);
    CHECK (loaded.appearance == "dark");
    CHECK (loaded.language == "de");
    CHECK (loaded.uiScale == 1.5);
    CHECK (loaded.windowWidth == 1000);
    CHECK (loaded.windowHeight == 720);
    CHECK (loaded.lastCubaseVersion == 14);
    CHECK (loaded.recentExports.size() == 1);
}

TEST_CASE ("AppSettings: unknown values fall back, newer files are left alone", "[settings]")
{
    TempSettings temp;
    temp.write ("<?xml version=\"1.0\"?><PROPERTIES>"
                "<VALUE name=\"formatVersion\" val=\"2\"/>"
                "<VALUE name=\"appearance\" val=\"purple\"/>"
                "<VALUE name=\"language\" val=\"fr\"/>"
                "</PROPERTIES>");
    const auto settings = AppSettings::load (temp.file);
    CHECK (settings.appearance == "system");
    CHECK (settings.language == "system");

    temp.write ("<?xml version=\"1.0\"?><PROPERTIES>"
                "<VALUE name=\"formatVersion\" val=\"99\"/>"
                "<VALUE name=\"appearance\" val=\"dark\"/>"
                "</PROPERTIES>");
    const auto before = temp.file.loadFileAsString();
    AppSettings::load (temp.file).save (temp.file);
    CHECK (temp.file.loadFileAsString() == before);
}
