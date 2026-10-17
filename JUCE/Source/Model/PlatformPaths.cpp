#include "PlatformPaths.h"

#include <algorithm>

namespace StudioPatchbay
{

juce::File PlatformPaths::getCubasePreferencesBase()
{
#if JUCE_MAC
    // ~/Library/Preferences
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
               .getChildFile ("Library")
               .getChildFile ("Preferences");
#elif JUCE_WINDOWS
    // %APPDATA%, i.e. C:\Users\<name>\AppData\Roaming, then Steinberg
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Steinberg");
#else
    // Cubase does not run here; nothing to find.
    return {};
#endif
}

std::optional<int> PlatformPaths::versionFromFolderName (const juce::String& folderName)
{
    const juce::String prefix ("Cubase ");
    if (! folderName.startsWith (prefix))
        return std::nullopt;

    auto versionText = folderName.substring (prefix.length());
    if (versionText.endsWith ("_64"))
        versionText = versionText.dropLastCharacters (3);

    if (versionText.isEmpty() || ! versionText.containsOnly ("0123456789"))
        return std::nullopt;

    const auto version = versionText.getIntValue();
    if (version < minimumVersion)
        return std::nullopt;

    return version;
}

std::vector<CubaseInstallation> PlatformPaths::detectInstallationsIn (const juce::File& baseDir)
{
    std::vector<CubaseInstallation> installations;

    if (! baseDir.isDirectory())
        return installations;

    for (auto& child : baseDir.findChildFiles (juce::File::findDirectories, false))
        if (auto version = versionFromFolderName (child.getFileName()))
            installations.push_back ({ *version, child });

    // Latest first; equal versions in a stable, path-based order.
    std::sort (installations.begin(), installations.end(),
               [] (const auto& a, const auto& b)
               {
                   if (a.version != b.version)
                       return a.version > b.version;
                   return a.prefsDir.getFullPathName() < b.prefsDir.getFullPathName();
               });

    return installations;
}

std::vector<CubaseInstallation> PlatformPaths::detectInstallations()
{
    return detectInstallationsIn (getCubasePreferencesBase());
}

juce::File PlatformPaths::getAppDataDirectory()
{
    // ~/Library/Application Support, %APPDATA%, or ~/.config elsewhere.
    auto appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                       .getChildFile ("StudioPatchbay");

    if (! appData.isDirectory())
        appData.createDirectory();

    return appData;
}

} // namespace StudioPatchbay
