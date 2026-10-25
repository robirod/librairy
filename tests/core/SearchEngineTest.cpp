#include <catch2/catch_test_macros.hpp>
#include "core/SearchEngine.h"
#include "core/Database.h"

/**
 * Unit tests for SearchEngine class.
 */

TEST_CASE("SearchEngine - basic search", "[search]")
{
    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto dbPath = tempDir.getChildFile("librairy_search_test.db").getFullPathName();

    // Cleanup before test
    juce::File(dbPath).deleteFile();

    librairy::Database db(dbPath);
    librairy::SearchEngine engine(db);

    auto libId = db.createLibrary("Search Test");

    // Insert test data
    librairy::AudioFileInfo info;
    info.libraryId   = libId;
    info.filePath    = "/test/thunder.wav";
    info.filename    = "thunder.wav";
    info.description = "Distant thunder rumble";
    info.category    = "Weather";
    db.upsertFile(info);

    SECTION("Search returns matching file")
    {
        auto results = engine.search("thunder", libId);
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].filename == "thunder.wav");
    }

    SECTION("Empty search returns all files")
    {
        auto results = engine.search("", libId);
        REQUIRE(results.size() >= 1);
    }

    // Cleanup
    juce::File(dbPath).deleteFile();
}

