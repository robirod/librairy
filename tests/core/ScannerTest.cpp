#include <catch2/catch_test_macros.hpp>
#include "core/Scanner.h"

/**
 * Unit tests for Scanner class.
 * Note: Full integration tests require actual audio files.
 */

TEST_CASE("Scanner - Supported file detection", "[scanner]")
{
    SECTION("WAV files are supported")
    {
        REQUIRE(librairy::isSupportedAudioFile(juce::File("/test/audio.wav")));
        REQUIRE(librairy::isSupportedAudioFile(juce::File("/test/audio.WAV")));
    }

    SECTION("AIFF files are supported")
    {
        REQUIRE(librairy::isSupportedAudioFile(juce::File("/test/audio.aiff")));
        REQUIRE(librairy::isSupportedAudioFile(juce::File("/test/audio.aif")));
    }

    SECTION("FLAC files are supported")
    {
        REQUIRE(librairy::isSupportedAudioFile(juce::File("/test/audio.flac")));
    }

    SECTION("MP3 files are supported")
    {
        REQUIRE(librairy::isSupportedAudioFile(juce::File("/test/audio.mp3")));
    }

    SECTION("Non-audio files are not supported")
    {
        REQUIRE_FALSE(librairy::isSupportedAudioFile(juce::File("/test/doc.pdf")));
        REQUIRE_FALSE(librairy::isSupportedAudioFile(juce::File("/test/image.png")));
        REQUIRE_FALSE(librairy::isSupportedAudioFile(juce::File("/test/video.mp4")));
    }
}

