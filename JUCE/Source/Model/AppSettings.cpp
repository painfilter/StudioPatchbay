#include "AppSettings.h"
#include "PlatformPaths.h"

namespace StudioPatchbay
{

namespace
{
juce::File defaultFile()
{
    return PlatformPaths::getAppDataDirectory().getChildFile ("StudioPatchbay.settings");
}

std::unique_ptr<juce::PropertiesFile> openFile (const juce::File& file)
{
    juce::PropertiesFile::Options options;
    options.applicationName = "StudioPatchbay";
    options.filenameSuffix = ".settings";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    return std::make_unique<juce::PropertiesFile> (file, options);
}

juce::String oneOf (const juce::String& value, const juce::StringArray& allowed, const juce::String& fallback)
{
    return allowed.contains (value) ? value : fallback;
}
} // namespace

AppSettings AppSettings::load() { return load (defaultFile()); }
void AppSettings::save() const { save (defaultFile()); }

AppSettings AppSettings::load (const juce::File& path)
{
    AppSettings settings;
    auto file = openFile (path);
    const auto version = file->getIntValue ("formatVersion", formatVersion);

    // A file from a newer version keeps its values for that version; this one
    // starts from its defaults rather than guessing what the keys now mean.
    if (version > formatVersion)
        return settings;

    if (version < 2)
    {
        if (file->containsKey ("darkMode"))
            settings.appearance = file->getBoolValue ("darkMode") ? "dark" : "light";
    }
    else
    {
        settings.appearance = oneOf (file->getValue ("appearance"), { "system", "light", "dark" }, "system");
        settings.language = oneOf (file->getValue ("language"), { "system", "en", "de" }, "system");
        settings.windowWidth = juce::jmax (0, file->getIntValue ("windowWidth"));
        settings.windowHeight = juce::jmax (0, file->getIntValue ("windowHeight"));
        settings.lastCubaseVersion = juce::jmax (0, file->getIntValue ("lastCubaseVersion"));
    }

    settings.uiScale = file->getDoubleValue ("uiScale", settings.uiScale);
    settings.contentScale = juce::jlimit (0.6, 2.0, file->getDoubleValue ("contentScale", settings.contentScale));
    settings.recentExports.addLines (file->getValue ("recentExports"));
    settings.recentExports.removeEmptyStrings();
    return settings;
}

void AppSettings::rememberExport (const juce::File& file)
{
    recentExports.removeString (file.getFullPathName());
    recentExports.insert (0, file.getFullPathName());
    while (recentExports.size() > maxRecentExports)
        recentExports.remove (recentExports.size() - 1);
}

void AppSettings::save (const juce::File& path) const
{
    auto file = openFile (path);
    if (file->getIntValue ("formatVersion", formatVersion) > formatVersion)
        return;   // never overwrite a newer file

    file->removeValue ("darkMode");
    file->setValue ("formatVersion", formatVersion);
    file->setValue ("appearance", appearance);
    file->setValue ("language", language);
    file->setValue ("uiScale", uiScale);
    file->setValue ("contentScale", contentScale);
    file->setValue ("windowWidth", windowWidth);
    file->setValue ("windowHeight", windowHeight);
    file->setValue ("lastCubaseVersion", lastCubaseVersion);
    file->setValue ("recentExports", recentExports.joinIntoString ("\n"));
    file->saveIfNeeded();
}

} // namespace StudioPatchbay
