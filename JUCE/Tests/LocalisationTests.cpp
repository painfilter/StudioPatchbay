#include <catch2/catch_test_macros.hpp>
#include "Localisation/TextCatalog.h"

#include <regex>

namespace
{
const juce::File root (SP_SOURCE_DIR);

juce::var catalog (const juce::String& name)
{
    return juce::JSON::parse (root.getChildFile ("Resources/Localization").getChildFile (name));
}

juce::StringArray keysOf (const juce::String& name)
{
    juce::StringArray keys;
    StudioPatchbay::Text::collectKeys (catalog (name), {}, keys);
    keys.sort (false);
    return keys;
}
} // namespace

TEST_CASE ("Localisation: English and German have the same keys", "[text]")
{
    for (auto base : { "StudioPatchbay", "common" })
    {
        const auto en = keysOf (juce::String (base) + ".en.json");
        const auto de = keysOf (juce::String (base) + ".de.json");
        INFO (base);
        CHECK (! en.isEmpty());
        CHECK (en == de);
    }
}

TEST_CASE ("Localisation: every key the code asks for exists", "[text]")
{
    const auto product = catalog ("StudioPatchbay.en.json");
    const auto common = catalog ("common.en.json");
    const std::regex call (R"re(tr \("([A-Za-z0-9_.]+)"\))re");

    juce::StringArray missing;
    for (const auto& file : root.getChildFile ("Source").findChildFiles (juce::File::findFiles, true, "*.cpp;*.h"))
    {
        const auto text = file.loadFileAsString().toStdString();
        for (std::sregex_iterator it (text.begin(), text.end(), call), end; it != end; ++it)
        {
            const juce::String key ((*it)[1].str());
            if (StudioPatchbay::Text::lookup (product, key).isEmpty()
                && StudioPatchbay::Text::lookup (common, key).isEmpty())
                missing.addIfNotAlreadyThere (file.getFileName() + ": " + key);
        }
    }
    INFO (missing.joinIntoString ("\n"));
    CHECK (missing.isEmpty());
}
