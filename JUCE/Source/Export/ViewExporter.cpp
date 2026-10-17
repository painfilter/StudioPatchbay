#include "ViewExporter.h"
#include "PdfWriter.h"

namespace StudioPatchbay::Export
{

namespace
{
constexpr double outputPixelsPerLogicalPixel = 2.0;
constexpr double maxOutputPixels = 40.0e6;   // ~160 MB as ARGB while rendering

std::vector<juce::uint8> rgbBytes (const juce::Image& image)
{
    const juce::Image::BitmapData data (image, juce::Image::BitmapData::readOnly);
    std::vector<juce::uint8> rgb;
    rgb.reserve (static_cast<size_t> (image.getWidth()) * static_cast<size_t> (image.getHeight()) * 3);
    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto c = data.getPixelColour (x, y);
            rgb.push_back (c.getRed());
            rgb.push_back (c.getGreen());
            rgb.push_back (c.getBlue());
        }
    return rgb;
}
} // namespace

juce::Image renderForExport (const ExportSource& source)
{
    const auto bounds = source.content.getLocalBounds();
    if (bounds.isEmpty())
        return {};

    const auto uiScale = source.uiScale > 0.0 ? source.uiScale : 1.0;
    auto factor = outputPixelsPerLogicalPixel / uiScale;
    const auto pixels = bounds.getWidth() * factor * bounds.getHeight() * factor;
    if (pixels > maxOutputPixels)
        factor *= std::sqrt (maxOutputPixels / pixels);

    const auto w = juce::jmax (1, juce::roundToInt (bounds.getWidth() * factor));
    const auto h = juce::jmax (1, juce::roundToInt (bounds.getHeight() * factor));
    juce::Image image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (image);
    g.fillAll (source.background.withAlpha (1.0f));
    g.addTransform (juce::AffineTransform::scale (static_cast<float> (w) / static_cast<float> (bounds.getWidth()),
                                                  static_cast<float> (h) / static_cast<float> (bounds.getHeight())));
    source.content.paintEntireComponent (g, true);
    return image;
}

juce::Result exportTo (const ExportSource& source, Format format, const juce::File& target)
{
    const auto image = renderForExport (source);
    if (! image.isValid())
        return juce::Result::fail ("There is nothing to export in this view.");

    juce::TemporaryFile temp (target);
    {
        juce::FileOutputStream stream (temp.getFile());
        if (! stream.openedOk())
            return juce::Result::fail ("Could not write to " + target.getParentDirectory().getFullPathName());

        if (format == Format::png)
        {
            juce::PNGImageFormat png;
            if (! png.writeImageToStream (image, stream))
                return juce::Result::fail ("The PNG image could not be encoded.");
        }
        else
        {
            // One logical pixel is one point, so the page has the view's proportions
            // and the raster carries twice the resolution for printing.
            const auto uiScale = source.uiScale > 0.0 ? source.uiScale : 1.0;
            const auto rgb = rgbBytes (image);
            PdfImagePage page;
            page.pixelWidth = image.getWidth();
            page.pixelHeight = image.getHeight();
            page.rgb = rgb.data();
            page.pageWidthPoints = juce::jmax (1, juce::roundToInt (source.content.getWidth() / uiScale));
            page.pageHeightPoints = juce::jmax (1, juce::roundToInt (source.content.getHeight() / uiScale));
            page.title = source.title;
            const auto pdf = writeImagePdf (page);
            if (pdf.isEmpty() || ! stream.write (pdf.getData(), pdf.getSize()))
                return juce::Result::fail ("The PDF could not be written.");
        }

        stream.flush();
        if (stream.getStatus().failed())
            return stream.getStatus();
    }

    if (! temp.overwriteTargetFileWithTemporary())
        return juce::Result::fail ("Could not replace " + target.getFullPathName());

    return juce::Result::ok();
}

} // namespace StudioPatchbay::Export
