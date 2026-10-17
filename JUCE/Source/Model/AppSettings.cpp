#include "AppSettings.h"
#include "PlatformPaths.h"

namespace StudioPatchbay
{

namespace
{
std::unique_ptr<juce::PropertiesFile> openFile()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "StudioPatchbay";
    options.filenameSuffix = ".settings";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    auto file = PlatformPaths::getAppDataDirectory().getChildFile ("StudioPatchbay.settings");
    return std::make_unique<juce::PropertiesFile> (file, options);
}
} // namespace

AppSettings AppSettings::load()
{
    AppSettings settings;
    auto file = openFile();

    // A file from a newer version keeps its values for that version; this one
    // starts from its defaults rather than guessing what the keys now mean.
    if (file->getIntValue ("formatVersion", formatVersion) > formatVersion)
        return settings;

    settings.darkMode = file->getBoolValue ("darkMode", settings.darkMode);
    settings.uiScale = file->getDoubleValue ("uiScale", settings.uiScale);
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

void AppSettings::save() const
{
    auto file = openFile();
    if (file->getIntValue ("formatVersion", formatVersion) > formatVersion)
        return;   // never overwrite a newer file

    file->setValue ("formatVersion", formatVersion);
    file->setValue ("darkMode", darkMode);
    file->setValue ("uiScale", uiScale);
    file->setValue ("recentExports", recentExports.joinIntoString ("\n"));
    file->saveIfNeeded();
}

} // namespace StudioPatchbay
