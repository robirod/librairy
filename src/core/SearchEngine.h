#pragma once

#include "metadata/MetadataModel.h"
#include "core/Database.h"
#include <juce_core/juce_core.h>
#include <vector>

namespace librairy
{

/**
 * High-level search interface that wraps the database FTS5 engine.
 *
 * Provides text-based search, filtered search, and will later integrate
 * with AI embeddings for semantic search (Stage 2).
 */
class SearchEngine
{
public:
    explicit SearchEngine(Database& database);
    ~SearchEngine() = default;

    /** Simple text search across all metadata fields */
    std::vector<AudioFileInfo> search(
        const juce::String& query,
        int64_t libraryId = -1,
        int maxResults = 500) const;

    /** Search with filters */
    std::vector<AudioFileInfo> searchFiltered(
        const Database::SearchFilters& filters) const;

    /** Get all files for a library (no search filter) */
    std::vector<AudioFileInfo> getAllFiles(
        int64_t libraryId,
        int maxResults = 5000) const;

private:
    Database& db;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchEngine)
};

} // namespace librairy

