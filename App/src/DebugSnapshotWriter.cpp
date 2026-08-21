#include "DebugSnapshotWriter.h"

namespace NextStudio::Debug
{
bool writeValidatedPng(const juce::Image &image, const juce::File &file)
{
    if (!image.isValid() || image.getWidth() <= 0 || image.getHeight() <= 0 || file == juce::File())
        return false;

    auto stream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream());
    if (stream == nullptr || !stream->openedOk())
        return false;

    juce::PNGImageFormat png;
    const auto encoded = png.writeImageToStream(image, *stream);
    stream->flush();
    stream.reset();

    const auto decoded = encoded && file.existsAsFile() && file.getSize() > 0
                             ? juce::ImageFileFormat::loadFrom(file)
                             : juce::Image();
    if (!encoded || !decoded.isValid() || decoded.getWidth() != image.getWidth() || decoded.getHeight() != image.getHeight())
    {
        file.deleteFile();
        return false;
    }

    return true;
}
} // namespace NextStudio::Debug
