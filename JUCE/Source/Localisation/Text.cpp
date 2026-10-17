#include "Text.h"
#include "TextCatalog.h"
#include "BinaryData.h"

namespace StudioPatchbay::Text
{

namespace
{
juce::String preference { "system" };

juce::var load (const char* resourceName)
{
    int size = 0;
    if (const auto* data = BinaryData::getNamedResource (resourceName, size))
        return juce::JSON::parse (juce::String::fromUTF8 (data, size));
    jassertfalse;   // the catalogue is not in the binary data
    return {};
}

struct Catalogs
{
    juce::var product, common;
};

const Catalogs& catalogs (bool inGerman)
{
    static const Catalogs english { load ("StudioPatchbay_en_json"), load ("common_en_json") };
    static const Catalogs deutsch { load ("StudioPatchbay_de_json"), load ("common_de_json") };
    return inGerman ? deutsch : english;
}

juce::String find (const Catalogs& c, const juce::String& key)
{
    auto text = lookup (c.product, key);
    return text.isNotEmpty() ? text : lookup (c.common, key);
}
} // namespace

void setLanguagePreference (const juce::String& value)
{
    preference = juce::StringArray { "en", "de" }.contains (value) ? value : juce::String ("system");
}

juce::String languagePreference() { return preference; }

bool german()
{
    if (preference == "system")
        return juce::SystemStats::getUserLanguage().startsWithIgnoreCase ("de");
    return preference == "de";
}

juce::String tr (const juce::String& key)
{
    auto text = find (catalogs (german()), key);
    if (text.isEmpty())
        text = find (catalogs (false), key);
    return text.isNotEmpty() ? text : key;
}

} // namespace StudioPatchbay::Text
