#include "Fonts.h"

#include "BinaryData.h"
#include "Design/PlatinumLookAndFeel.h"

namespace StudioPatchbay
{

namespace
{
// The registered Plex weights, cached for the process. DeletedAtShutdown frees
// the cache during JUCE's message-system teardown – before static destructors
// and the leak detector run – so the Typefaces never outlive JUCE (which would
// trip the leak detector and crash at exit).
struct FontCache : public juce::DeletedAtShutdown
{
    juce::Typeface::Ptr regular, medium, semiBold, bold;

    FontCache()
    {
        regular  = juce::Typeface::createSystemTypefaceFor (BinaryData::IBMPlexSansRegular_ttf,
                                                            BinaryData::IBMPlexSansRegular_ttfSize);
        medium   = juce::Typeface::createSystemTypefaceFor (BinaryData::IBMPlexSansMedium_ttf,
                                                            BinaryData::IBMPlexSansMedium_ttfSize);
        semiBold = juce::Typeface::createSystemTypefaceFor (BinaryData::IBMPlexSansSemiBold_ttf,
                                                            BinaryData::IBMPlexSansSemiBold_ttfSize);
        bold     = juce::Typeface::createSystemTypefaceFor (BinaryData::IBMPlexSansBold_ttf,
                                                            BinaryData::IBMPlexSansBold_ttfSize);
    }

    ~FontCache() override { clearSingletonInstance(); }

    juce::Typeface::Ptr get (Fonts::Weight w) const
    {
        switch (w)
        {
            case Fonts::Weight::medium:   return medium;
            case Fonts::Weight::semiBold: return semiBold;
            case Fonts::Weight::bold:     return bold;
            case Fonts::Weight::regular:
            default:                      return regular;
        }
    }

    JUCE_DECLARE_SINGLETON_SINGLETHREADED (FontCache, false)
};

JUCE_IMPLEMENT_SINGLETON (FontCache)
} // namespace

juce::Typeface::Ptr Fonts::typeface (Weight w)
{
    return FontCache::getInstance()->get (w);
}

juce::Font Fonts::sans (float height, Weight w)
{
    return juce::Font (juce::FontOptions (typeface (w)).withHeight (height));
}

juce::Font Fonts::sans (float height, juce::Font::FontStyleFlags style)
{
    const auto w = (style & juce::Font::bold) != 0 ? Weight::semiBold : Weight::regular;
    auto f = sans (height, w);
    if ((style & juce::Font::italic) != 0)
        f = f.italicised();
    return f;
}

void Fonts::applyTo (juce::LookAndFeel& laf)
{
    laf.setDefaultSansSerifTypeface (typeface (Weight::regular));
    // A function, not a captured Typeface: nothing JUCE owns may outlive its shutdown.
    Design::setChromeFontProvider ([] (double height, int styleFlags)
    {
        return sans (static_cast<float> (height), static_cast<juce::Font::FontStyleFlags> (styleFlags));
    });
}

} // namespace StudioPatchbay
