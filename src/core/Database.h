#pragma once

#include <juce_core/juce_core.h>
#include "metadata/MetadataModel.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <optional>

// Forward declare — we don't expose sqlite3 in the header
struct sqlite3;
struct sqlite3_stmt;

namespace librairy
{

/**
 * SQLite database wrapper for the audio catalog.
 *
 * Schema:
 *   libraries    — Named collections of root paths
 *   files        — Audio file records with metadata
 *   root_paths   — Root folders for each library
 *   files_fts    — FTS5 virtual table for full-text search
 *
 * Thread safety: All methods lock internally. Safe to call from any thread.
 */
class Database
{
public:
    /** Open (or create) a database at the given path */
    explicit Database(const juce::String& dbPath);
    ~Database();

    /** Is the database open and ready? */
    bool isOpen() const;

    // =====================================================================
    // Libraries
    // =====================================================================

    /** Create a new library. Returns the new library ID. */
    int64_t createLibrary(const juce::String& name);

    /** Delete a library and all its file records */
    bool deleteLibrary(int64_t libraryId);

    /** Rename a library */
    bool renameLibrary(int64_t libraryId, const juce::String& newName);

    /** Get all libraries */
    std::vector<LibraryInfo> getLibraries() const;

    /** Get a library by ID */
    std::optional<LibraryInfo> getLibrary(int64_t libraryId) const;

    // =====================================================================
    // Root Paths
    // =====================================================================

    /** Add a root path to a library */
    bool addRootPath(int64_t libraryId, const juce::String& path);

    /** Remove a root path */
    bool removeRootPath(int64_t libraryId, const juce::String& path);

    /** Get root paths for a library */
    juce::StringArray getRootPaths(int64_t libraryId) const;

    // =====================================================================
    // Files
    // =====================================================================

    /** Insert or update a file record. Returns the file ID. */
    int64_t upsertFile(const AudioFileInfo& info);

    /** Get a file by ID */
    std::optional<AudioFileInfo> getFile(int64_t fileId) const;

    /** Get a file by path */
    std::optional<AudioFileInfo> getFileByPath(
        const juce::String& filePath) const;

    /** Delete a file record */
    bool deleteFile(int64_t fileId);

    /** Delete all files for a library */
    bool deleteFilesForLibrary(int64_t libraryId);

    /** Mark files as missing if they no longer exist on disk */
    int markMissingFiles(int64_t libraryId);

    /** Get the count of files in a library */
    int64_t getFileCount(int64_t libraryId) const;

    // =====================================================================
    // Search
    // =====================================================================

    /** Full-text search across all indexed fields */
    std::vector<AudioFileInfo> searchFullText(
        const juce::String& query,
        int64_t libraryId = -1,
        int limit = 500) const;

    /** Filtered search with specific criteria */
    struct SearchFilters
    {
        juce::String textQuery;
        int64_t      libraryId    = -1;
        juce::String formatFilter;          // e.g., "wav"
        int          minSampleRate = 0;
        int          maxSampleRate = 0;
        int          minBitDepth   = 0;
        int          maxBitDepth   = 0;
        int          minChannels   = 0;
        int          maxChannels   = 0;
        double       minDuration   = 0.0;
        double       maxDuration   = 0.0;
        juce::String category;
        int          limit         = 500;
    };

    std::vector<AudioFileInfo> searchWithFilters(
        const SearchFilters& filters) const;

    // =====================================================================
    // Maintenance
    // =====================================================================

    /** Run VACUUM to compact the database */
    void vacuum();

    /** Get the database file size in bytes */
    int64_t getDatabaseSize() const;

private:
    sqlite3* db = nullptr;
    juce::String databasePath;
    mutable juce::CriticalSection dbLock;

    /** Create schema (tables, indexes, FTS) */
    void createSchema();

    /** Execute a SQL statement (no results) */
    bool execute(const juce::String& sql);

    /** Helper to prepare a statement */
    sqlite3_stmt* prepare(const juce::String& sql) const;

    /** Build AudioFileInfo from a SELECT statement row */
    AudioFileInfo rowToFileInfo(sqlite3_stmt* stmt) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Database)
};

} // namespace librairy

