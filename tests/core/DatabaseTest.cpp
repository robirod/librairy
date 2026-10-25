#include <catch2/catch_test_macros.hpp>
#include "core/Database.h"
#include <juce_core/juce_core.h>

/**
 * Unit tests for Database class.
 */

static juce::String getTestDbPath()
{
    auto tempDir = juce::File::getSpecialLocation(
        juce::File::tempDirectory);
    return tempDir.getChildFile("librairy_test.db").getFullPathName();
}

static void cleanupTestDb()
{
    juce::File(getTestDbPath()).deleteFile();
    juce::File(getTestDbPath() + "-wal").deleteFile();
    juce::File(getTestDbPath() + "-shm").deleteFile();
}

TEST_CASE("Database opens and creates schema", "[database]")
{
    cleanupTestDb();
    librairy::Database db(getTestDbPath());

    REQUIRE(db.isOpen());

    cleanupTestDb();
}

TEST_CASE("Database CRUD - Libraries", "[database]")
{
    cleanupTestDb();
    librairy::Database db(getTestDbPath());

    SECTION("Create and retrieve library")
    {
        auto id = db.createLibrary("Test SFX Library");
        REQUIRE(id > 0);

        auto lib = db.getLibrary(id);
        REQUIRE(lib.has_value());
        REQUIRE(lib->name == "Test SFX Library");
    }

    SECTION("List libraries")
    {
        db.createLibrary("Library A");
        db.createLibrary("Library B");

        auto libs = db.getLibraries();
        REQUIRE(libs.size() >= 2);
    }

    SECTION("Rename library")
    {
        auto id = db.createLibrary("Old Name");
        REQUIRE(db.renameLibrary(id, "New Name"));

        auto lib = db.getLibrary(id);
        REQUIRE(lib->name == "New Name");
    }

    SECTION("Delete library")
    {
        auto id = db.createLibrary("To Delete");
        REQUIRE(db.deleteLibrary(id));

        auto lib = db.getLibrary(id);
        REQUIRE_FALSE(lib.has_value());
    }

    cleanupTestDb();
}

TEST_CASE("Database CRUD - Files", "[database]")
{
    cleanupTestDb();
    librairy::Database db(getTestDbPath());

    auto libId = db.createLibrary("Test Library");

    SECTION("Insert and retrieve file")
    {
        librairy::AudioFileInfo info;
        info.libraryId       = libId;
        info.filePath        = "/test/audio/explosion.wav";
        info.filename        = "explosion.wav";
        info.sampleRate      = 48000;
        info.bitDepth        = 24;
        info.numChannels     = 2;
        info.durationSeconds = 3.5;
        info.category        = "SFX";
        info.description     = "Large explosion with debris";

        auto fileId = db.upsertFile(info);
        REQUIRE(fileId > 0);

        auto retrieved = db.getFile(fileId);
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->filename == "explosion.wav");
        REQUIRE(retrieved->sampleRate == 48000);
        REQUIRE(retrieved->category == "SFX");
    }

    SECTION("Upsert updates existing file")
    {
        librairy::AudioFileInfo info;
        info.libraryId  = libId;
        info.filePath   = "/test/audio/rain.wav";
        info.filename   = "rain.wav";
        info.category   = "Ambience";

        auto id1 = db.upsertFile(info);
        info.category = "Weather";
        auto id2 = db.upsertFile(info);

        // Should update, not create new
        auto retrieved = db.getFileByPath("/test/audio/rain.wav");
        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->category == "Weather");
    }

    SECTION("File count")
    {
        librairy::AudioFileInfo info;
        info.libraryId = libId;

        info.filePath = "/test/1.wav"; info.filename = "1.wav";
        db.upsertFile(info);
        info.filePath = "/test/2.wav"; info.filename = "2.wav";
        db.upsertFile(info);
        info.filePath = "/test/3.wav"; info.filename = "3.wav";
        db.upsertFile(info);

        REQUIRE(db.getFileCount(libId) == 3);
    }

    cleanupTestDb();
}

TEST_CASE("Database Full-Text Search", "[database][search]")
{
    cleanupTestDb();
    librairy::Database db(getTestDbPath());

    auto libId = db.createLibrary("Search Test");

    // Insert test files
    auto insertFile = [&](const juce::String& name,
                          const juce::String& desc,
                          const juce::String& cat)
    {
        librairy::AudioFileInfo info;
        info.libraryId   = libId;
        info.filePath    = "/test/" + name;
        info.filename    = name;
        info.description = desc;
        info.category    = cat;
        db.upsertFile(info);
    };

    insertFile("rain_heavy.wav",   "Heavy rain on metal roof",  "Weather");
    insertFile("rain_light.wav",   "Light rain drizzle",        "Weather");
    insertFile("explosion_big.wav","Large cinematic explosion",  "SFX");
    insertFile("birds_forest.wav", "Forest birds morning chorus","Ambience");
    insertFile("footsteps.wav",    "Footsteps on gravel",       "Foley");

    SECTION("Search by keyword")
    {
        auto results = db.searchFullText("rain", libId);
        REQUIRE(results.size() == 2);
    }

    SECTION("Search by category")
    {
        auto results = db.searchFullText("Weather", libId);
        REQUIRE(results.size() == 2);
    }

    SECTION("Search by description word")
    {
        auto results = db.searchFullText("cinematic", libId);
        REQUIRE(results.size() == 1);
        REQUIRE(results[0].filename == "explosion_big.wav");
    }

    SECTION("Search with no results")
    {
        auto results = db.searchFullText("nonexistent", libId);
        REQUIRE(results.empty());
    }

    cleanupTestDb();
}

