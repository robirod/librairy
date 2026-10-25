#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "MetadataModel.h"

namespace librairy
{

/**
 * Writes metadata back to audio files without re-encoding the audio data.
 *
 * Current support:
 *   - WAV/BWF: Updates BWF description chunk, iXML chunk
 *   - AIFF: Updates metadata chunks
 *   - FLAC: Updates Vorbis Comments
 *
 * For formats where in-place editing is not possible (MP3 ID3 tags),
 * the file is rewritten with updated metadata.
 */
class MetadataWriter
{
public:
    MetadataWriter();
    ~MetadataWriter() = default;

    /**
     * Write metadata fields from `info` back to the audio file.
     *
     * @param info  The AudioFileInfo with updated metadata fields.
     *              filePath must point to a valid audio file.
     * @return      true if metadata was written successfully
     */
    bool writeMetadata(const AudioFileInfo& info) const;

    /**
     * Write a single metadata field to a file.
     *
     * @param filePath   Path to the audio file
     * @param fieldName  Metadata field name (e.g., "description", "category")
     * @param value      New value for the field
     * @return           true if successful
     */
    bool writeField(const juce::String& filePath,
                    const juce::String& fieldName,
                    const juce::String& value) const;

private:
    mutable juce::AudioFormatManager formatManager;

    /** Build BWF metadata pairs from AudioFileInfo */
    juce::StringPairArray buildBWFMetadata(const AudioFileInfo& info) const;

    /** Generate iXML string from AudioFileInfo */
    juce::String buildIXML(const AudioFileInfo& info) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetadataWriter)
};

} // namespace librairy

