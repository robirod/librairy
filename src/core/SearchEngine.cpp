#include "SearchEngine.h"
#include "Database.h"

namespace librairy
{

SearchEngine::SearchEngine(Database& database)
    : db(database)
{
}

std::vector<AudioFileInfo> SearchEngine::search(
    const juce::String& query,
    int64_t libraryId,
    int maxResults) const
{
    if (query.isEmpty())
        return getAllFiles(libraryId, maxResults);

    return db.searchFullText(query, libraryId, maxResults);
}

std::vector<AudioFileInfo> SearchEngine::searchFiltered(
    const Database::SearchFilters& filters) const
{
    return db.searchWithFilters(filters);
}

std::vector<AudioFileInfo> SearchEngine::getAllFiles(
    int64_t libraryId,
    int maxResults) const
{
    Database::SearchFilters filters;
    filters.libraryId = libraryId;
    filters.limit     = maxResults;
    return db.searchWithFilters(filters);
}

} // namespace librairy

