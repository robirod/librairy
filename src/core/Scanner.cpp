#include "Scanner.h"
#include "Database.h"

namespace librairy
{

Scanner::Scanner(Database& database)
    : juce::Thread("LibrairyScannerThread"),
      db(database)
{
}

Scanner::~Scanner()
{
    cancelScan();
}

void Scanner::startScan(int64_t libraryId,
                         const juce::StringArray& rootPaths,
                         bool incrementalOnly)
{
    if (isThreadRunning())
    {
        DBG("Scanner: Already scanning. Cancel first.");
        return;
    }

    scanLibraryId    = libraryId;
    scanRootPaths    = rootPaths;
    scanIncremental  = incrementalOnly;

    filesFound.store(0);
    filesProcessed.store(0);
    filesSkipped.store(0);
    filesErrored.store(0);
    currentFile.clear();

    startThread(juce::Thread::Priority::background);
}

void Scanner::cancelScan()
{
    signalThreadShouldExit();
    stopThread(5000);
}

bool Scanner::isScanning() const
{
    return isThreadRunning();
}

Scanner::Progress Scanner::getProgress() const
{
    Progress p;
    p.filesFound     = filesFound.load();
    p.filesProcessed = filesProcessed.load();
    p.filesSkipped   = filesSkipped.load();
    p.filesErrored   = filesErrored.load();

    {
        juce::ScopedLock lock(progressLock);
        p.currentFile = currentFile;
    }

    return p;
}

void Scanner::run()
{
    DBG("Scanner: Starting scan for library " << scanLibraryId);

    // Phase 1: Discover all audio files
    auto files = discoverFiles(scanRootPaths);
    filesFound.store(static_cast<int>(files.size()));

    DBG("Scanner: Found " << files.size() << " audio files");

    // Phase 2: Process each file
    for (const auto& file : files)
    {
        if (threadShouldExit())
        {
            listeners.call([](Listener& l) { l.scanCancelled(); });
            return;
        }

        processFile(file, scanLibraryId, scanIncremental);

        // Notify progress periodically
        if (filesProcessed.load() % 50 == 0)
        {
            auto progress = getProgress();
            listeners.call([&progress](Listener& l)
            {
                l.scanProgressUpdated(progress);
            });
        }
    }

    auto finalProgress = getProgress();
    DBG("Scanner: Completed. Processed=" << finalProgress.filesProcessed
        << " Skipped=" << finalProgress.filesSkipped
        << " Errors=" << finalProgress.filesErrored);

    listeners.call([&finalProgress](Listener& l)
    {
        l.scanCompleted(finalProgress);
    });
}

std::vector<juce::File> Scanner::discoverFiles(
    const juce::StringArray& rootPaths)
{
    std::vector<juce::File> result;

    for (const auto& rootPath : rootPaths)
    {
        if (threadShouldExit())
            break;

        juce::File root(rootPath);
        if (!root.isDirectory())
        {
            DBG("Scanner: Root path not a directory: " << rootPath);
            continue;
        }

        // Recursively find all files
        for (const auto& entry :
             juce::RangedDirectoryIterator(root, true, "*",
                                            juce::File::findFiles))
        {
            if (threadShouldExit())
                break;

            const auto& file = entry.getFile();
            if (isSupportedAudioFile(file))
            {
                result.push_back(file);
            }
        }
    }

    return result;
}

void Scanner::processFile(const juce::File& file,
                           int64_t libraryId,
                           bool incrementalOnly)
{
    {
        juce::ScopedLock lock(progressLock);
        currentFile = file.getFileName();
    }

    // Check if we can skip (incremental mode)
    if (incrementalOnly && !fileNeedsUpdate(file))
    {
        filesSkipped.fetch_add(1);
        filesProcessed.fetch_add(1);
        return;
    }

    // Read metadata
    try
    {
        auto info = metadataReader.readFile(file);
        info.libraryId = libraryId;

        db.upsertFile(info);
        filesProcessed.fetch_add(1);
    }
    catch (...)
    {
        DBG("Scanner: Error processing file: " << file.getFullPathName());
        filesErrored.fetch_add(1);
        filesProcessed.fetch_add(1);
    }
}

bool Scanner::fileNeedsUpdate(const juce::File& file) const
{
    auto existing = db.getFileByPath(file.getFullPathName());
    if (!existing.has_value())
        return true; // New file

    // Check if modified since last index
    auto currentModTime = file.getLastModificationTime()
                              .toString(true, true, false, true);
    auto currentSize    = file.getSize();

    return (existing->dateModified != currentModTime ||
            existing->fileSizeBytes != currentSize);
}

void Scanner::addListener(Listener* listener)    { listeners.add(listener); }
void Scanner::removeListener(Listener* listener)  { listeners.remove(listener); }

} // namespace librairy

