#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

namespace librairy
{

/**
 * Generates waveform thumbnail data for audio files.
 * Waveforms are cached for fast redraw.
 */
class WaveformGenerator
{
public:
    WaveformGenerator();
    ~WaveformGenerator() = default;

    // TODO: Generate waveform peaks data for a file
    // TODO: Cache management

private:
    juce::AudioFormatManager formatManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformGenerator)
};

} // namespace librairy

