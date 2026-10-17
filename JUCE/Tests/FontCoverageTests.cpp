#include <catch2/catch_test_macros.hpp>
#include <juce_graphics/juce_graphics.h>

#include <regex>

// The interface draws everything in the bundled IBM Plex fonts. A character
// they do not carry would silently come from a system font instead, so every
// character in the texts and string literals must be in Plex Sans and Plex
// Mono, and no code outside the font module may ask for a system font.

namespace
{
const juce::File root (SP_SOURCE_DIR);

juce::Typeface::Ptr load (const char* name)
{
    juce::MemoryBlock data;
    root.getChildFile ("Resources/Fonts").getChildFile (name).loadFileAsData (data);
    return juce::Typeface::createSystemTypefaceFor (data.getData(), data.getSize());
}

void collectStrings (const juce::var& v, juce::StringArray& out)
{
    if (v.isString())
        out.add (v.toString());
    else if (auto* o = v.getDynamicObject())
        for (const auto& p : o->getProperties())
            collectStrings (p.value, out);
}

// The text of C++ string literals, with \x escapes decoded as UTF-8.
juce::StringArray literalsIn (const juce::String& source)
{
    juce::StringArray out;
    const std::string text = source.toStdString();
    static const std::regex literal (R"re("((?:[^"\\\n]|\\.)*)")re");
    for (std::sregex_iterator it (text.begin(), text.end(), literal), end; it != end; ++it)
    {
        const auto body = (*it)[1].str();
        std::string bytes;
        for (size_t i = 0; i < body.size(); ++i)
        {
            if (body[i] == '\\' && i + 1 < body.size())
            {
                if (body[i + 1] == 'x' && i + 3 < body.size())
                {
                    bytes += static_cast<char> (std::stoi (body.substr (i + 2, 2), nullptr, 16));
                    i += 3;
                }
                else
                    ++i;   // \n, \", \\ ... are ASCII
                continue;
            }
            bytes += body[i];
        }
        out.add (juce::String::fromUTF8 (bytes.data(), static_cast<int> (bytes.size())));
    }
    return out;
}
} // namespace

TEST_CASE ("Fonts: every character the interface shows is in Plex Sans and Plex Mono", "[fonts]")
{
    const auto sans = load ("IBMPlexSans-Regular.ttf");
    const auto mono = load ("IBMPlexMono-Regular.ttf");
    REQUIRE (sans != nullptr);
    REQUIRE (mono != nullptr);

    juce::StringArray texts;
    for (const auto& file : root.getChildFile ("Resources/Localization").findChildFiles (juce::File::findFiles, false, "*.json"))
        collectStrings (juce::JSON::parse (file), texts);
    for (const auto& file : root.getChildFile ("Source").findChildFiles (juce::File::findFiles, true, "*.cpp;*.h"))
        if (! file.getParentDirectory().getFileName().equalsIgnoreCase ("Design"))
            texts.addArray (literalsIn (file.loadFileAsString()));

    juce::StringArray missing;
    for (const auto& t : texts)
        for (auto p = t.getCharPointer(); ! p.isEmpty();)
        {
            const auto c = p.getAndAdvance();
            if (c < 0x20)
                continue;
            if (! sans->getNominalGlyphForCodepoint (c) || ! mono->getNominalGlyphForCodepoint (c))
                missing.addIfNotAlreadyThere (juce::String::charToString (c) + " U+"
                                              + juce::String::toHexString (static_cast<int> (c)).toUpperCase());
        }
    INFO (missing.joinIntoString (", "));
    CHECK (missing.isEmpty());
}

TEST_CASE ("Fonts: no system font is requested outside the font module", "[fonts]")
{
    juce::StringArray offenders;
    for (const auto& file : root.getChildFile ("Source").findChildFiles (juce::File::findFiles, true, "*.cpp;*.h"))
    {
        if (file.getParentDirectory().getFileName() == "Design" || file.getFileNameWithoutExtension() == "Fonts")
            continue;
        const auto text = file.loadFileAsString();
        for (auto call : { "getDefaultMonospacedFontName", "getDefaultSansSerifFontName", "getDefaultSerifFontName",
                           "FontOptions (", "juce::Font (" })
            if (text.contains (call))
                offenders.add (file.getFileName() + ": " + call);
    }
    INFO (offenders.joinIntoString ("\n"));
    CHECK (offenders.isEmpty());
}
