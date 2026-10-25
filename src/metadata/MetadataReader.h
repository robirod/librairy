#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "MetadataModel.h"

namespace librairy
{

/**
 * Reads metadata from audio files.
 *
 * Supports:
 *   - WAV/BWF: Description, Originator, Date, TimeReference, iXML chunk
 *   - AIFF: Common chunk, ANNO chunks
 *   - FLAC: Vorbis Comments
 *   - MP3: ID3v2 tags
 *   - OGG: Vorbis Comments
 *
 * Uses JUCE's AudioFormatReader for technical properties and
 * extracts metadata from format-specific headers.
 */
class MetadataReader
{
public:
    MetadataReader();
    ~MetadataReader() = default;

    /**
     * Read all metadata from the given audio file.
     *
     * @param file      The audio file to read
     * @return          AudioFileInfo populated with file props + metadata.
     *                  id and libraryId are left at -1.
     */
    AudioFileInfo readFile(const juce::File& file) const;

    /**
     * Read only technical properties (sample rate, bit depth, etc.)
     * without parsing extended metadata. Faster for initial scan.
     */
    AudioFileInfo readFileQuick(const juce::File& file) const;

private:
    mutable juce::AudioFormatManager formatManager;

    /** Extract metadata key-value pairs from a format reader */
    void extractMetadata(AudioFileInfo& info,
                         const juce::StringPairArray& metadataPairs) const;

    /** Parse iXML chunk content if present */
    void parseIXML(AudioFileInfo& info,
                   const juce::String& ixmlContent) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetadataReader)
};

} // namespace librairy

