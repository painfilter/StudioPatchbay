#include <catch2/catch_test_macros.hpp>
#include "Export/PdfWriter.h"

using namespace StudioPatchbay::Export;

namespace
{
// Byte-exact: the file is binary, so no text decoding.
std::string asText (const juce::MemoryBlock& block)
{
    return { static_cast<const char*> (block.getData()), block.getSize() };
}

bool contains (const std::string& text, const std::string& part) { return text.find (part) != std::string::npos; }

// The byte range of the first stream after `marker`.
juce::MemoryBlock streamAfter (const juce::MemoryBlock& pdf, const char* marker)
{
    const auto* bytes = static_cast<const char*> (pdf.getData());
    const std::string all (bytes, pdf.getSize());
    const auto at = all.find (marker);
    const auto begin = all.find ("stream\n", at) + 7;
    const auto end = all.find ("\nendstream", begin);
    return { bytes + begin, end - begin };
}
} // namespace

TEST_CASE ("PDF writer rejects an empty page", "[export]")
{
    CHECK (writeImagePdf ({}).isEmpty());
}

TEST_CASE ("PDF writer produces a valid single-page image PDF", "[export]")
{
    const juce::uint8 rgb[] = { 255, 0, 0,   0, 255, 0,
                                0, 0, 255,   10, 20, 30 };
    PdfImagePage page;
    page.pixelWidth = 2;
    page.pixelHeight = 2;
    page.rgb = rgb;
    page.pageWidthPoints = 100;
    page.pageHeightPoints = 50;
    page.title = "Patchbay (Cubase 14) \\ test";

    const auto pdf = writeImagePdf (page);
    const auto text = asText (pdf);

    REQUIRE (text.rfind ("%PDF-1.4\n", 0) == 0);
    CHECK (text.substr (text.size() - 6) == "%%EOF\n");
    CHECK (contains (text, "/MediaBox [0 0 100 50]"));
    CHECK (contains (text, "/Width 2 /Height 2"));
    CHECK (contains (text, "/Title (Patchbay \\(Cubase 14\\) \\\\ test)"));

    SECTION ("every xref offset points at its object")
    {
        const auto& all = text;
        const auto startxref = all.rfind ("startxref\n");
        const auto xrefAt = std::stoul (all.substr (startxref + 10));
        REQUIRE (all.compare (xrefAt, 5, "xref\n") == 0);

        auto line = all.find ('\n', all.find ('\n', xrefAt) + 1) + 1;   // skip "xref", "0 7"
        line += 20;                                                     // skip the free entry
        for (int object = 1; object <= 6; ++object, line += 20)
        {
            const auto offset = std::stoul (all.substr (line, 10));
            CHECK (all.substr (offset, std::to_string (object).size() + 6) == std::to_string (object) + " 0 obj");
        }
    }

    SECTION ("the image stream inflates to the original pixels")
    {
        const auto compressed = streamAfter (pdf, "/Subtype /Image");
        juce::MemoryInputStream source (compressed, false);
        juce::GZIPDecompressorInputStream inflate (&source, false, juce::GZIPDecompressorInputStream::zlibFormat);
        juce::MemoryBlock pixels;
        inflate.readIntoMemoryBlock (pixels);
        REQUIRE (pixels.getSize() == sizeof (rgb));
        CHECK (std::memcmp (pixels.getData(), rgb, sizeof (rgb)) == 0);
    }
}
