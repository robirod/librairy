#include "MetadataReader.h"

namespace librairy
{

MetadataReader::MetadataReader()
{
    formatManager.registerBasicFormats();
}

AudioFileInfo MetadataReader::readFile(const juce::File& file) const
{
    AudioFileInfo info;
    info.filePath      = file.getFullPathName();
    info.filename      = file.getFileName();
    info.fileSizeBytes = file.getSize();
    info.dateModified  = file.getLastModificationTime()
                             .toString(true, true, false, true);
    info.formatName    = file.getFileExtension().toLowerCase()
                             .trimCharactersAtStart(".");

    // Read via JUCE AudioFormatReader
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return info;

    // Technical properties
    info.sampleRate      = static_cast<int>(reader->sampleRate);
    info.bitDepth        = static_cast<int>(reader->bitsPerSample);
    info.numChannels     = static_cast<int>(reader->numChannels);
    info.durationSeconds = (reader->sampleRate > 0)
        ? static_cast<double>(reader->lengthInSamples) / reader->sampleRate
        : 0.0;

    // Metadata from the format reader
    extractMetadata(info, reader->metadataValues);

    return info;
}

AudioFileInfo MetadataReader::readFileQuick(const juce::File& file) const
{
    AudioFileInfo info;
    info.filePath      = file.getFullPathName();
    info.filename      = file.getFileName();
    info.fileSizeBytes = file.getSize();
    info.dateModified  = file.getLastModificationTime()
                             .toString(true, true, false, true);
    info.formatName    = file.getFileExtension().toLowerCase()
                             .trimCharactersAtStart(".");

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return info;

    info.sampleRate      = static_cast<int>(reader->sampleRate);
    info.bitDepth        = static_cast<int>(reader->bitsPerSample);
    info.numChannels     = static_cast<int>(reader->numChannels);
    info.durationSeconds = (reader->sampleRate > 0)
        ? static_cast<double>(reader->lengthInSamples) / reader->sampleRate
        : 0.0;

    return info;
}

void MetadataReader::extractMetadata(
    AudioFileInfo& info,
    const juce::StringPairArray& meta) const
{
    // BWF / WAV metadata keys (JUCE uses these key names)
    if (meta.containsKey("bwav description"))
        info.description = meta.getValue("bwav description", "");
    else if (meta.containsKey("Description"))
        info.description = meta.getValue("Description", "");

    if (meta.containsKey("bwav originator"))
        info.originator = meta.getValue("bwav originator", "");

    if (meta.containsKey("bwav originator ref"))
        info.originatorRef = meta.getValue("bwav originator ref", "");

    if (meta.containsKey("bwav origination date"))
        info.originationDate = meta.getValue("bwav origination date", "");

    if (meta.containsKey("bwav origination time"))
        info.originationTime = meta.getValue("bwav origination time", "");

    if (meta.containsKey("bwav time reference"))
    {
        info.timeReference = meta.getValue("bwav time reference", "0")
                                 .getLargeIntValue();
    }

    if (meta.containsKey("bwav coding history"))
        info.codingHistory = meta.getValue("bwav coding history", "");

    // ID3 metadata (MP3)
    if (meta.containsKey("id3title"))
        info.description = meta.getValue("id3title", "");

    if (meta.containsKey("id3artist"))
        info.originator = meta.getValue("id3artist", "");

    if (meta.containsKey("id3genre"))
        info.category = meta.getValue("id3genre", "");

    if (meta.containsKey("id3comment"))
        info.notes = meta.getValue("id3comment", "");

    // Vorbis Comments (FLAC, OGG)
    if (meta.containsKey("TITLE"))
        info.description = meta.getValue("TITLE", "");

    if (meta.containsKey("ARTIST"))
        info.originator = meta.getValue("ARTIST", "");

    if (meta.containsKey("GENRE"))
        info.category = meta.getValue("GENRE", "");

    if (meta.containsKey("COMMENT"))
        info.notes = meta.getValue("COMMENT", "");

    if (meta.containsKey("DESCRIPTION"))
        info.description = meta.getValue("DESCRIPTION", "");

    // iXML chunk (if present as a single string)
    if (meta.containsKey("ixml"))
        parseIXML(info, meta.getValue("ixml", ""));
}

void MetadataReader::parseIXML(AudioFileInfo& info,
                                const juce::String& ixmlContent) const
{
    if (ixmlContent.isEmpty())
        return;

    // Parse iXML as XML
    auto xml = juce::parseXML(ixmlContent);
    if (xml == nullptr)
        return;

    // Standard iXML fields
    if (auto* cat = xml->getChildByName("CATEGORY"))
        info.category = cat->getAllSubText().trim();

    if (auto* subCat = xml->getChildByName("SUBCATEGORY"))
        info.subcategory = subCat->getAllSubText().trim();

    if (auto* desc = xml->getChildByName("NOTE"))
        info.notes = desc->getAllSubText().trim();

    if (auto* scene = xml->getChildByName("SCENE"))
        info.scene = scene->getAllSubText().trim();

    if (auto* take = xml->getChildByName("TAKE_TYPE"))
        info.takeType = take->getAllSubText().trim();

    // Some iXML implementations use BEXT sub-element
    if (auto* bext = xml->getChildByName("BEXT"))
    {
        if (auto* bwfDesc = bext->getChildByName("BWF_DESCRIPTION"))
        {
            if (info.description.isEmpty())
                info.description = bwfDesc->getAllSubText().trim();
        }

        if (auto* orig = bext->getChildByName("BWF_ORIGINATOR"))
        {
            if (info.originator.isEmpty())
                info.originator = orig->getAllSubText().trim();
        }
    }
}

} // namespace librairy

