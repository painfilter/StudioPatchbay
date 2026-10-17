#include <catch2/catch_test_macros.hpp>
#include "Model/PlatformPaths.h"

using namespace StudioPatchbay;

TEST_CASE ("Cubase preference folder names", "[platform]")
{
    CHECK (PlatformPaths::versionFromFolderName ("Cubase 14") == 14);       // macOS
    CHECK (PlatformPaths::versionFromFolderName ("Cubase 14_64") == 14);    // Windows
    CHECK (PlatformPaths::versionFromFolderName ("Cubase 12") == 12);
    CHECK (PlatformPaths::versionFromFolderName ("Cubase 15_64") == 15);

    CHECK_FALSE (PlatformPaths::versionFromFolderName ("Cubase 11"));
    CHECK_FALSE (PlatformPaths::versionFromFolderName ("Cubase 11_64"));
    CHECK_FALSE (PlatformPaths::versionFromFolderName ("Cubase"));
    CHECK_FALSE (PlatformPaths::versionFromFolderName ("Cubase _64"));
    CHECK_FALSE (PlatformPaths::versionFromFolderName ("Cubase 14_32"));
    CHECK_FALSE (PlatformPaths::versionFromFolderName ("Cubase 14 backup"));
    CHECK_FALSE (PlatformPaths::versionFromFolderName ("Nuendo 14_64"));
    CHECK_FALSE (PlatformPaths::versionFromFolderName ("cubase 14"));
}

TEST_CASE ("Installations are found in a base folder, latest first", "[platform]")
{
    juce::TemporaryFile temp;
    const auto base = temp.getFile();
    REQUIRE (base.createDirectory());

    for (auto name : { "Cubase 13_64", "Cubase 15_64", "Cubase 11_64", "Steinberg Hub", "Cubase 14" })
        REQUIRE (base.getChildFile (name).createDirectory());
    REQUIRE (base.getChildFile ("Cubase 16_64").create());   // a file, not a folder

    const auto found = PlatformPaths::detectInstallationsIn (base);
    base.deleteRecursively();

    REQUIRE (found.size() == 3);
    CHECK (found[0].version == 15);
    CHECK (found[0].prefsDir.getFileName() == "Cubase 15_64");
    CHECK (found[1].version == 14);
    CHECK (found[2].version == 13);
    CHECK (found[0].externalPluginsXml().getParentDirectory() == found[0].prefsDir);
}

TEST_CASE ("A missing base folder yields no installations", "[platform]")
{
    CHECK (PlatformPaths::detectInstallationsIn ({}).empty());
    CHECK (PlatformPaths::detectInstallationsIn (juce::File::getCurrentWorkingDirectory()
                                                     .getChildFile ("does-not-exist")).empty());
}
