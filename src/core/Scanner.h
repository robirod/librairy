#pragma once

#include <juce_core/juce_core.h>
#include "metadata/MetadataModel.h"
#include "metadata/MetadataReader.h"

#include <atomic>
#include <functional>
#include <vector>

namespace librairy
{

class Database;

/**
 * Recursive file system scanner for audio libraries.
 *
 * Scans root directories asynchronously, extracts metadata for each
 * supported audio file, and inserts/updates records in the database.
 *
 * Supports:
 *   - Async scanning on a background thread
 *   - Progress reporting
 *   - Cancellation
 *   - Incremental scans (skip unchanged files)
 */
class Scanner : public juce::Thread
{
public:
    explicit Scanner(Database& database);
    ~Scanner() override;

    /**
     * Start scanning the given root paths for a library.
     * Runs asynchronously — returns immediately.
     */
    void startScan(int64_t libraryId,
                   const juce::StringArray& rootPaths,
                   bool incrementalOnly = true);

    /** Cancel the current scan */
    void cancelScan();

    /** Is a scan currently running? */
    bool isScanning() const;

    // ---- Progress ----
    struct Progress
    {
        int    filesFound     = 0;
        int    filesProcessed = 0;
        int    filesSkipped   = 0;
        int    filesErrored   = 0;
        juce::String currentFile;
    };

    Progress getProgress() const;

    // ---- Listener ----
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void scanProgressUpdated(const Progress& progress) = 0;
        virtual void scanCompleted(const Progress& finalProgress) = 0;
        virtual void scanCancelled() = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    // Thread::run()
    void run() override;

    /** Recursively discover all supported audio files */
    std::vector<juce::File> discoverFiles(
        const juce::StringArray& rootPaths);

    /** Process a single file: read metadata and insert into DB */
    void processFile(const juce::File& file, int64_t libraryId,
                     bool incrementalOnly);

    /** Check if a file needs re-indexing (by size + modification time) */
    bool fileNeedsUpdate(const juce::File& file) const;

    Database&        db;
    MetadataReader   metadataReader;

    // Scan parameters (set before thread starts)
    int64_t          scanLibraryId = -1;
    juce::StringArray scanRootPaths;
    bool             scanIncremental = true;

    // Progress tracking (atomic for thread safety)
    std::atomic<int> filesFound     { 0 };
    std::atomic<int> filesProcessed { 0 };
    std::atomic<int> filesSkipped   { 0 };
    std::atomic<int> filesErrored   { 0 };
    juce::String     currentFile;
    juce::CriticalSection progressLock;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Scanner)
};

} // namespace librairy

