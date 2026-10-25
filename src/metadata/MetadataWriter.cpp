#include "MetadataWriter.h"

namespace librairy
{

MetadataWriter::MetadataWriter()
{
    formatManager.registerBasicFormats();
}

bool MetadataWriter::writeMetadata(const AudioFileInfo& info) const
{
    juce::File file(info.filePath);
    if (!file.existsAsFile())
        return false;

    // Read original audio data
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    // Prepare temp file
    juce::File tempFile = file.getSiblingFile(
        file.getFileNameWithoutExtension() + "_tmp" + file.getFileExtension());

    // Get format for writing
    auto* format = formatManager.findFormatForFileExtension(
        file.getFileExtension());
    if (format == nullptr)
        return false;

    // Build metadata
    auto metadata = buildBWFMetadata(info);

    // Add iXML if WAV/BWF
    auto ext = file.getFileExtension().toLowerCase();
    if (ext == ".wav" || ext == ".wave" || ext == ".bwf")
    {
        metadata.set("ixml", buildIXML(info));
    }

    // Write to temp file with updated metadata
    {
        std::unique_ptr<juce::FileOutputStream> outStream(
            tempFile.createOutputStream());
        if (outStream == nullptr)
            return false;

        std::unique_ptr<juce::AudioFormatWriter> writer(
            format->createWriterFor(
                outStream.get(),
                reader->sampleRate,
                reader->numChannels,
                static_cast<int>(reader->bitsPerSample),
                metadata,
                0));

        if (writer == nullptr)
            return false;

        outStream.release(); // Writer takes ownership

        // Copy audio data
        writer->writeFromAudioReader(*reader, 0, reader->lengthInSamples);
    }

    // Replace original with temp
    reader.reset(); // Release read lock on original file

    if (!tempFile.moveFileTo(file))
    {
        tempFile.deleteFile();
        return false;
    }

    return true;
}

bool MetadataWriter::writeField(const juce::String& filePath,
                                 const juce::String& /*fieldName*/,
                                 const juce::String& /*value*/) const
{
    // TODO: Implement single-field write (read current metadata, update
    //       one field, write back). For now, use writeMetadata().
    juce::File file(filePath);
    if (!file.existsAsFile())
        return false;

    // Placeholder — single field write will be optimized later
    return false;
}

juce::StringPairArray MetadataWriter::buildBWFMetadata(
    const AudioFileInfo& info) const
{
    juce::StringPairArray meta;

    if (info.description.isNotEmpty())
        meta.set("bwav description", info.description);

    if (info.originator.isNotEmpty())
        meta.set("bwav originator", info.originator);

    if (info.originatorRef.isNotEmpty())
        meta.set("bwav originator ref", info.originatorRef);

    if (info.originationDate.isNotEmpty())
        meta.set("bwav origination date", info.originationDate);

    if (info.originationTime.isNotEmpty())
        meta.set("bwav origination time", info.originationTime);

    if (info.timeReference != 0)
        meta.set("bwav time reference",
                 juce::String(info.timeReference));

    if (info.codingHistory.isNotEmpty())
        meta.set("bwav coding history", info.codingHistory);

    return meta;
}

juce::String MetadataWriter::buildIXML(const AudioFileInfo& info) const
{
    juce::String xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<BWFXML>\n";
    xml << "  <IXML_VERSION>1.52</IXML_VERSION>\n";

    if (info.category.isNotEmpty())
        xml << "  <CATEGORY>" << info.category << "</CATEGORY>\n";

    if (info.subcategory.isNotEmpty())
        xml << "  <SUBCATEGORY>" << info.subcategory << "</SUBCATEGORY>\n";

    if (info.scene.isNotEmpty())
        xml << "  <SCENE>" << info.scene << "</SCENE>\n";

    if (info.takeType.isNotEmpty())
        xml << "  <TAKE_TYPE>" << info.takeType << "</TAKE_TYPE>\n";

    if (info.notes.isNotEmpty())
        xml << "  <NOTE>" << info.notes << "</NOTE>\n";

    xml << "</BWFXML>\n";
    return xml;
}

} // namespace librairy

