#include "PdfWriter.h"

namespace StudioPatchbay::Export
{

namespace
{
// PDF literal strings: ASCII only here, with the three reserved characters escaped.
juce::String pdfText (const juce::String& text)
{
    juce::String out;
    for (auto c : text)
    {
        if (c == '\\' || c == '(' || c == ')')
            out << '\\' << juce::String::charToString (c);
        else if (c >= 32 && c < 127)
            out << juce::String::charToString (c);
        else
            out << '?';
    }
    return out;
}

struct Writer
{
    juce::MemoryOutputStream out;
    juce::Array<juce::int64> offsets;

    void text (const juce::String& s) { out.write (s.toRawUTF8(), s.getNumBytesAsUTF8()); }

    void beginObject()
    {
        offsets.add (static_cast<juce::int64> (out.getPosition()));
        text (juce::String (offsets.size()) + " 0 obj\n");
    }

    void object (const juce::String& dictionary)
    {
        beginObject();
        text (dictionary + "\nendobj\n");
    }

    void streamObject (const juce::String& dictionaryEntries, const juce::MemoryBlock& data)
    {
        beginObject();
        text ("<< " + dictionaryEntries + " /Length " + juce::String (static_cast<juce::int64> (data.getSize()))
              + " >>\nstream\n");
        out.write (data.getData(), data.getSize());
        text ("\nendstream\nendobj\n");
    }
};
} // namespace

juce::MemoryBlock writeImagePdf (const PdfImagePage& page)
{
    if (page.pixelWidth <= 0 || page.pixelHeight <= 0 || page.rgb == nullptr
        || page.pageWidthPoints <= 0 || page.pageHeightPoints <= 0)
        return {};

    juce::MemoryBlock pixels;
    {
        juce::MemoryOutputStream compressed (pixels, false);
        // windowBits 0 = zlib framing, which is what /FlateDecode expects.
        juce::GZIPCompressorOutputStream zip (compressed, 9, 0);
        zip.write (page.rgb, static_cast<size_t> (page.pixelWidth) * static_cast<size_t> (page.pixelHeight) * 3);
    }

    const auto w = juce::String (page.pageWidthPoints);
    const auto h = juce::String (page.pageHeightPoints);
    const auto drawing = "q " + w + " 0 0 " + h + " 0 0 cm /Im0 Do Q";

    Writer pdf;
    // Version line, then four high bytes that mark the file as binary.
    static const char header[] = "%PDF-1.4\n%\xe2\xe3\xcf\xd3\n";
    pdf.out.write (header, sizeof (header) - 1);
    pdf.object ("<< /Type /Catalog /Pages 2 0 R >>");
    pdf.object ("<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
    pdf.object ("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " + w + " " + h + "]"
                " /Resources << /XObject << /Im0 4 0 R >> >> /Contents 5 0 R >>");
    pdf.streamObject ("/Type /XObject /Subtype /Image /Width " + juce::String (page.pixelWidth)
                      + " /Height " + juce::String (page.pixelHeight)
                      + " /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /FlateDecode",
                      pixels);
    pdf.streamObject ("", juce::MemoryBlock (drawing.toRawUTF8(), drawing.getNumBytesAsUTF8()));
    pdf.object ("<< /Title (" + pdfText (page.title) + ") /Producer (StudioPatchbay) >>");

    const auto xrefAt = static_cast<juce::int64> (pdf.out.getPosition());
    pdf.text ("xref\n0 " + juce::String (pdf.offsets.size() + 1) + "\n0000000000 65535 f \n");
    for (auto offset : pdf.offsets)
        pdf.text (juce::String (offset).paddedLeft ('0', 10) + " 00000 n \n");
    pdf.text ("trailer\n<< /Size " + juce::String (pdf.offsets.size() + 1)
              + " /Root 1 0 R /Info " + juce::String (pdf.offsets.size()) + " 0 R >>\n"
              "startxref\n" + juce::String (xrefAt) + "\n%%EOF\n");

    return pdf.out.getMemoryBlock();
}

} // namespace StudioPatchbay::Export
