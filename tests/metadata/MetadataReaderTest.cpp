#include <catch2/catch_test_macros.hpp>
#include "metadata/MetadataModel.h"

/**
 * Unit tests for MetadataReader.
 * Full tests require actual audio files with metadata.
 */

TEST_CASE("MetadataModel - Supported extensions", "[metadata]")
{
    SECTION("Standard extensions are recognized")
    {
        auto& exts = librairy::getSupportedExtensions();
        REQUIRE(exts.contains(".wav"));
        REQUIRE(exts.contains(".aiff"));
        REQUIRE(exts.contains(".flac"));
        REQUIRE(exts.contains(".mp3"));
        REQUIRE(exts.contains(".ogg"));
        REQUIRE(exts.contains(".bwf"));
        REQUIRE(exts.contains(".caf"));
    }

    SECTION("AudioFileInfo defaults are sensible")
    {
        librairy::AudioFileInfo info;
        REQUIRE(info.id == -1);
        REQUIRE(info.sampleRate == 0);
        REQUIRE(info.durationSeconds == 0.0);
        REQUIRE(info.userRating == 0);
        REQUIRE(info.aiConfidence == 0.0f);
        REQUIRE_FALSE(info.aiReviewed);
    }
}

