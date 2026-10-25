#include "Database.h"
#include <sqlite3.h>

namespace librairy
{

// =============================================================================
// Construction / Destruction
// =============================================================================

Database::Database(const juce::String& dbPath)
    : databasePath(dbPath)
{
    int rc = sqlite3_open(dbPath.toRawUTF8(), &db);
    if (rc != SQLITE_OK)
    {
        DBG("Failed to open database: " << sqlite3_errmsg(db));
        sqlite3_close(db);
        db = nullptr;
        return;
    }

    // Enable WAL mode for better concurrent read performance
    execute("PRAGMA journal_mode=WAL;");
    execute("PRAGMA foreign_keys=ON;");
    execute("PRAGMA synchronous=NORMAL;");

    createSchema();
}

Database::~Database()
{
    if (db != nullptr)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Database::isOpen() const
{
    return db != nullptr;
}

// =============================================================================
// Schema
// =============================================================================

void Database::createSchema()
{
    // Libraries table
    execute(R"SQL(
        CREATE TABLE IF NOT EXISTS libraries (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            name         TEXT NOT NULL,
            date_created TEXT DEFAULT (datetime('now'))
        );
    )SQL");

    // Root paths table
    execute(R"SQL(
        CREATE TABLE IF NOT EXISTS root_paths (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            library_id INTEGER NOT NULL,
            path       TEXT NOT NULL,
            FOREIGN KEY (library_id) REFERENCES libraries(id) ON DELETE CASCADE,
            UNIQUE(library_id, path)
        );
    )SQL");

    // Files table
    execute(R"SQL(
        CREATE TABLE IF NOT EXISTS files (
            id                INTEGER PRIMARY KEY AUTOINCREMENT,
            library_id        INTEGER NOT NULL,
            file_path         TEXT NOT NULL UNIQUE,
            filename          TEXT NOT NULL,
            file_size_bytes   INTEGER DEFAULT 0,
            date_modified     TEXT,
            file_hash         TEXT,

            -- Audio properties
            duration_seconds  REAL DEFAULT 0.0,
            sample_rate       INTEGER DEFAULT 0,
            bit_depth         INTEGER DEFAULT 0,
            num_channels      INTEGER DEFAULT 0,
            format_name       TEXT,

            -- BWF metadata
            description       TEXT DEFAULT '',
            originator        TEXT DEFAULT '',
            originator_ref    TEXT DEFAULT '',
            origination_date  TEXT DEFAULT '',
            origination_time  TEXT DEFAULT '',
            time_reference    INTEGER DEFAULT 0,
            coding_history    TEXT DEFAULT '',

            -- Categorical metadata
            category          TEXT DEFAULT '',
            subcategory       TEXT DEFAULT '',
            take_type         TEXT DEFAULT '',
            scene             TEXT DEFAULT '',
            notes             TEXT DEFAULT '',
            keywords          TEXT DEFAULT '',

            -- User metadata
            user_rating       INTEGER DEFAULT 0,
            user_notes        TEXT DEFAULT '',

            -- AI metadata (Stage 2)
            ai_suggested_cat  TEXT DEFAULT '',
            ai_confidence     REAL DEFAULT 0.0,
            ai_reviewed       INTEGER DEFAULT 0,

            -- Tracking
            indexed_at        TEXT DEFAULT (datetime('now')),
            is_missing        INTEGER DEFAULT 0,

            FOREIGN KEY (library_id) REFERENCES libraries(id) ON DELETE CASCADE
        );
    )SQL");

    // Indexes for common queries
    execute("CREATE INDEX IF NOT EXISTS idx_files_library "
            "ON files(library_id);");
    execute("CREATE INDEX IF NOT EXISTS idx_files_category "
            "ON files(category);");
    execute("CREATE INDEX IF NOT EXISTS idx_files_format "
            "ON files(format_name);");
    execute("CREATE INDEX IF NOT EXISTS idx_files_sample_rate "
            "ON files(sample_rate);");

    // FTS5 virtual table for full-text search
    execute(R"SQL(
        CREATE VIRTUAL TABLE IF NOT EXISTS files_fts USING fts5(
            filename,
            description,
            category,
            subcategory,
            keywords,
            notes,
            originator,
            content='files',
            content_rowid='id',
            tokenize='porter unicode61'
        );
    )SQL");

    // Triggers to keep FTS in sync
    execute(R"SQL(
        CREATE TRIGGER IF NOT EXISTS files_ai AFTER INSERT ON files BEGIN
            INSERT INTO files_fts(rowid, filename, description, category,
                                  subcategory, keywords, notes, originator)
            VALUES (new.id, new.filename, new.description, new.category,
                    new.subcategory, new.keywords, new.notes, new.originator);
        END;
    )SQL");

    execute(R"SQL(
        CREATE TRIGGER IF NOT EXISTS files_ad AFTER DELETE ON files BEGIN
            INSERT INTO files_fts(files_fts, rowid, filename, description,
                                  category, subcategory, keywords, notes,
                                  originator)
            VALUES ('delete', old.id, old.filename, old.description,
                    old.category, old.subcategory, old.keywords, old.notes,
                    old.originator);
        END;
    )SQL");

    execute(R"SQL(
        CREATE TRIGGER IF NOT EXISTS files_au AFTER UPDATE ON files BEGIN
            INSERT INTO files_fts(files_fts, rowid, filename, description,
                                  category, subcategory, keywords, notes,
                                  originator)
            VALUES ('delete', old.id, old.filename, old.description,
                    old.category, old.subcategory, old.keywords, old.notes,
                    old.originator);
            INSERT INTO files_fts(rowid, filename, description, category,
                                  subcategory, keywords, notes, originator)
            VALUES (new.id, new.filename, new.description, new.category,
                    new.subcategory, new.keywords, new.notes, new.originator);
        END;
    )SQL");
}

// =============================================================================
// Libraries
// =============================================================================

int64_t Database::createLibrary(const juce::String& name)
{
    juce::ScopedLock lock(dbLock);

    auto* stmt = prepare(
        "INSERT INTO libraries (name) VALUES (?);");
    if (stmt == nullptr) return -1;

    sqlite3_bind_text(stmt, 1, name.toRawUTF8(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    int64_t id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(db) : -1;
    sqlite3_finalize(stmt);
    return id;
}

bool Database::deleteLibrary(int64_t libraryId)
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare("DELETE FROM libraries WHERE id = ?;");
    if (stmt == nullptr) return false;

    sqlite3_bind_int64(stmt, 1, libraryId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::renameLibrary(int64_t libraryId, const juce::String& newName)
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare("UPDATE libraries SET name = ? WHERE id = ?;");
    if (stmt == nullptr) return false;

    sqlite3_bind_text(stmt, 1, newName.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, libraryId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<LibraryInfo> Database::getLibraries() const
{
    juce::ScopedLock lock(dbLock);
    std::vector<LibraryInfo> result;

    auto* stmt = prepare(
        "SELECT id, name, date_created FROM libraries ORDER BY name;");
    if (stmt == nullptr) return result;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        LibraryInfo lib;
        lib.id          = sqlite3_column_int64(stmt, 0);
        lib.name        = reinterpret_cast<const char*>(
                              sqlite3_column_text(stmt, 1));
        lib.dateCreated = reinterpret_cast<const char*>(
                              sqlite3_column_text(stmt, 2));
        result.push_back(std::move(lib));
    }

    sqlite3_finalize(stmt);
    return result;
}

std::optional<LibraryInfo> Database::getLibrary(int64_t libraryId) const
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare(
        "SELECT id, name, date_created FROM libraries WHERE id = ?;");
    if (stmt == nullptr) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, libraryId);

    std::optional<LibraryInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        LibraryInfo lib;
        lib.id          = sqlite3_column_int64(stmt, 0);
        lib.name        = reinterpret_cast<const char*>(
                              sqlite3_column_text(stmt, 1));
        lib.dateCreated = reinterpret_cast<const char*>(
                              sqlite3_column_text(stmt, 2));
        result = std::move(lib);
    }

    sqlite3_finalize(stmt);
    return result;
}

// =============================================================================
// Root Paths
// =============================================================================

bool Database::addRootPath(int64_t libraryId, const juce::String& path)
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare(
        "INSERT OR IGNORE INTO root_paths (library_id, path) VALUES (?, ?);");
    if (stmt == nullptr) return false;

    sqlite3_bind_int64(stmt, 1, libraryId);
    sqlite3_bind_text(stmt, 2, path.toRawUTF8(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::removeRootPath(int64_t libraryId, const juce::String& path)
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare(
        "DELETE FROM root_paths WHERE library_id = ? AND path = ?;");
    if (stmt == nullptr) return false;

    sqlite3_bind_int64(stmt, 1, libraryId);
    sqlite3_bind_text(stmt, 2, path.toRawUTF8(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

juce::StringArray Database::getRootPaths(int64_t libraryId) const
{
    juce::ScopedLock lock(dbLock);
    juce::StringArray paths;

    auto* stmt = prepare(
        "SELECT path FROM root_paths WHERE library_id = ?;");
    if (stmt == nullptr) return paths;

    sqlite3_bind_int64(stmt, 1, libraryId);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        paths.add(reinterpret_cast<const char*>(
                      sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    return paths;
}

// =============================================================================
// Files
// =============================================================================

int64_t Database::upsertFile(const AudioFileInfo& info)
{
    juce::ScopedLock lock(dbLock);

    auto* stmt = prepare(R"SQL(
        INSERT INTO files (
            library_id, file_path, filename, file_size_bytes, date_modified,
            file_hash, duration_seconds, sample_rate, bit_depth, num_channels,
            format_name, description, originator, originator_ref,
            origination_date, origination_time, time_reference,
            coding_history, category, subcategory, take_type, scene,
            notes, keywords, user_rating, user_notes,
            ai_suggested_cat, ai_confidence, ai_reviewed, is_missing
        ) VALUES (
            ?, ?, ?, ?, ?,
            ?, ?, ?, ?, ?,
            ?, ?, ?, ?,
            ?, ?, ?,
            ?, ?, ?, ?, ?,
            ?, ?, ?, ?,
            ?, ?, ?, 0
        )
        ON CONFLICT(file_path) DO UPDATE SET
            filename         = excluded.filename,
            file_size_bytes  = excluded.file_size_bytes,
            date_modified    = excluded.date_modified,
            duration_seconds = excluded.duration_seconds,
            sample_rate      = excluded.sample_rate,
            bit_depth        = excluded.bit_depth,
            num_channels     = excluded.num_channels,
            format_name      = excluded.format_name,
            description      = excluded.description,
            originator       = excluded.originator,
            originator_ref   = excluded.originator_ref,
            origination_date = excluded.origination_date,
            origination_time = excluded.origination_time,
            time_reference   = excluded.time_reference,
            coding_history   = excluded.coding_history,
            category         = excluded.category,
            subcategory      = excluded.subcategory,
            take_type        = excluded.take_type,
            scene            = excluded.scene,
            notes            = excluded.notes,
            keywords         = excluded.keywords,
            is_missing       = 0;
    )SQL");

    if (stmt == nullptr) return -1;

    int i = 1;
    sqlite3_bind_int64(stmt, i++, info.libraryId);
    sqlite3_bind_text(stmt, i++, info.filePath.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.filename.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, i++, info.fileSizeBytes);
    sqlite3_bind_text(stmt, i++, info.dateModified.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.fileHash.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, i++, info.durationSeconds);
    sqlite3_bind_int(stmt, i++, info.sampleRate);
    sqlite3_bind_int(stmt, i++, info.bitDepth);
    sqlite3_bind_int(stmt, i++, info.numChannels);
    sqlite3_bind_text(stmt, i++, info.formatName.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.description.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.originator.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.originatorRef.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.originationDate.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.originationTime.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, i++, info.timeReference);
    sqlite3_bind_text(stmt, i++, info.codingHistory.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.category.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.subcategory.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.takeType.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.scene.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.notes.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.keywords.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, i++, info.userRating);
    sqlite3_bind_text(stmt, i++, info.userNotes.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, info.aiSuggestedCategory.toRawUTF8(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, i++, static_cast<double>(info.aiConfidence));
    sqlite3_bind_int(stmt, i++, info.aiReviewed ? 1 : 0);

    int rc = sqlite3_step(stmt);
    int64_t id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(db) : -1;
    sqlite3_finalize(stmt);
    return id;
}

std::optional<AudioFileInfo> Database::getFile(int64_t fileId) const
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare("SELECT * FROM files WHERE id = ?;");
    if (stmt == nullptr) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, fileId);

    std::optional<AudioFileInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToFileInfo(stmt);

    sqlite3_finalize(stmt);
    return result;
}

std::optional<AudioFileInfo> Database::getFileByPath(
    const juce::String& filePath) const
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare("SELECT * FROM files WHERE file_path = ?;");
    if (stmt == nullptr) return std::nullopt;

    sqlite3_bind_text(stmt, 1, filePath.toRawUTF8(), -1, SQLITE_TRANSIENT);

    std::optional<AudioFileInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = rowToFileInfo(stmt);

    sqlite3_finalize(stmt);
    return result;
}

bool Database::deleteFile(int64_t fileId)
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare("DELETE FROM files WHERE id = ?;");
    if (stmt == nullptr) return false;

    sqlite3_bind_int64(stmt, 1, fileId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::deleteFilesForLibrary(int64_t libraryId)
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare("DELETE FROM files WHERE library_id = ?;");
    if (stmt == nullptr) return false;

    sqlite3_bind_int64(stmt, 1, libraryId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int Database::markMissingFiles(int64_t libraryId)
{
    juce::ScopedLock lock(dbLock);

    // Get all file paths for this library
    auto* stmt = prepare(
        "SELECT id, file_path FROM files "
        "WHERE library_id = ? AND is_missing = 0;");
    if (stmt == nullptr) return 0;

    sqlite3_bind_int64(stmt, 1, libraryId);

    std::vector<int64_t> missingIds;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        auto id = sqlite3_column_int64(stmt, 0);
        auto path = juce::String(reinterpret_cast<const char*>(
                                     sqlite3_column_text(stmt, 1)));

        if (!juce::File(path).existsAsFile())
            missingIds.push_back(id);
    }
    sqlite3_finalize(stmt);

    // Mark missing
    for (auto id : missingIds)
    {
        auto* update = prepare(
            "UPDATE files SET is_missing = 1 WHERE id = ?;");
        if (update != nullptr)
        {
            sqlite3_bind_int64(update, 1, id);
            sqlite3_step(update);
            sqlite3_finalize(update);
        }
    }

    return static_cast<int>(missingIds.size());
}

int64_t Database::getFileCount(int64_t libraryId) const
{
    juce::ScopedLock lock(dbLock);
    auto* stmt = prepare(
        "SELECT COUNT(*) FROM files WHERE library_id = ?;");
    if (stmt == nullptr) return 0;

    sqlite3_bind_int64(stmt, 1, libraryId);

    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int64(stmt, 0);

    sqlite3_finalize(stmt);
    return count;
}

// =============================================================================
// Search
// =============================================================================

std::vector<AudioFileInfo> Database::searchFullText(
    const juce::String& query,
    int64_t libraryId,
    int limit) const
{
    juce::ScopedLock lock(dbLock);
    std::vector<AudioFileInfo> results;

    juce::String sql =
        "SELECT f.* FROM files f "
        "JOIN files_fts fts ON f.id = fts.rowid "
        "WHERE files_fts MATCH ?";

    if (libraryId > 0)
        sql += " AND f.library_id = " + juce::String(libraryId);

    sql += " ORDER BY rank LIMIT " + juce::String(limit) + ";";

    auto* stmt = prepare(sql);
    if (stmt == nullptr) return results;

    // Prepare FTS query: add * for prefix matching
    auto ftsQuery = query.trim();
    if (ftsQuery.isNotEmpty() && !ftsQuery.containsChar('*'))
        ftsQuery += "*";

    sqlite3_bind_text(stmt, 1, ftsQuery.toRawUTF8(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW)
        results.push_back(rowToFileInfo(stmt));

    sqlite3_finalize(stmt);
    return results;
}

std::vector<AudioFileInfo> Database::searchWithFilters(
    const SearchFilters& filters) const
{
    juce::ScopedLock lock(dbLock);
    std::vector<AudioFileInfo> results;

    juce::String sql = "SELECT f.* FROM files f ";
    juce::String where = "WHERE 1=1 ";
    std::vector<std::pair<int, juce::String>> textBindings;
    int paramIdx = 1;

    // FTS query
    if (filters.textQuery.isNotEmpty())
    {
        sql += "JOIN files_fts fts ON f.id = fts.rowid ";
        where += "AND files_fts MATCH ? ";
        auto ftsQ = filters.textQuery.trim() + "*";
        textBindings.push_back({paramIdx++, ftsQ});
    }

    if (filters.libraryId > 0)
        where += "AND f.library_id = " + juce::String(filters.libraryId) + " ";

    if (filters.formatFilter.isNotEmpty())
    {
        where += "AND f.format_name = ? ";
        textBindings.push_back({paramIdx++, filters.formatFilter});
    }

    if (filters.minSampleRate > 0)
        where += "AND f.sample_rate >= " + juce::String(filters.minSampleRate) + " ";
    if (filters.maxSampleRate > 0)
        where += "AND f.sample_rate <= " + juce::String(filters.maxSampleRate) + " ";

    if (filters.minBitDepth > 0)
        where += "AND f.bit_depth >= " + juce::String(filters.minBitDepth) + " ";
    if (filters.maxBitDepth > 0)
        where += "AND f.bit_depth <= " + juce::String(filters.maxBitDepth) + " ";

    if (filters.minChannels > 0)
        where += "AND f.num_channels >= " + juce::String(filters.minChannels) + " ";
    if (filters.maxChannels > 0)
        where += "AND f.num_channels <= " + juce::String(filters.maxChannels) + " ";

    if (filters.minDuration > 0.0)
        where += "AND f.duration_seconds >= " + juce::String(filters.minDuration) + " ";
    if (filters.maxDuration > 0.0)
        where += "AND f.duration_seconds <= " + juce::String(filters.maxDuration) + " ";

    if (filters.category.isNotEmpty())
    {
        where += "AND f.category = ? ";
        textBindings.push_back({paramIdx++, filters.category});
    }

    sql += where;

    if (filters.textQuery.isNotEmpty())
        sql += "ORDER BY rank ";
    else
        sql += "ORDER BY f.filename ";

    sql += "LIMIT " + juce::String(filters.limit) + ";";

    auto* stmt = prepare(sql);
    if (stmt == nullptr) return results;

    for (auto& [idx, value] : textBindings)
        sqlite3_bind_text(stmt, idx, value.toRawUTF8(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW)
        results.push_back(rowToFileInfo(stmt));

    sqlite3_finalize(stmt);
    return results;
}

// =============================================================================
// Maintenance
// =============================================================================

void Database::vacuum()
{
    juce::ScopedLock lock(dbLock);
    execute("VACUUM;");
}

int64_t Database::getDatabaseSize() const
{
    return juce::File(databasePath).getSize();
}

// =============================================================================
// Private Helpers
// =============================================================================

bool Database::execute(const juce::String& sql)
{
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.toRawUTF8(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        DBG("SQL Error: " << (errMsg ? errMsg : "unknown"));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

sqlite3_stmt* Database::prepare(const juce::String& sql) const
{
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.toRawUTF8(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        DBG("Prepare error: " << sqlite3_errmsg(db));
        return nullptr;
    }
    return stmt;
}

AudioFileInfo Database::rowToFileInfo(sqlite3_stmt* stmt) const
{
    AudioFileInfo info;
    int col = 0;

    auto getText = [&](int c) -> juce::String {
        auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, c));
        return text ? juce::String(text) : juce::String();
    };

    info.id              = sqlite3_column_int64(stmt, col++);
    info.libraryId       = sqlite3_column_int64(stmt, col++);
    info.filePath        = getText(col++);
    info.filename        = getText(col++);
    info.fileSizeBytes   = sqlite3_column_int64(stmt, col++);
    info.dateModified    = getText(col++);
    info.fileHash        = getText(col++);
    info.durationSeconds = sqlite3_column_double(stmt, col++);
    info.sampleRate      = sqlite3_column_int(stmt, col++);
    info.bitDepth        = sqlite3_column_int(stmt, col++);
    info.numChannels     = sqlite3_column_int(stmt, col++);
    info.formatName      = getText(col++);
    info.description     = getText(col++);
    info.originator      = getText(col++);
    info.originatorRef   = getText(col++);
    info.originationDate = getText(col++);
    info.originationTime = getText(col++);
    info.timeReference   = sqlite3_column_int64(stmt, col++);
    info.codingHistory   = getText(col++);
    info.category        = getText(col++);
    info.subcategory     = getText(col++);
    info.takeType        = getText(col++);
    info.scene           = getText(col++);
    info.notes           = getText(col++);
    info.keywords        = getText(col++);
    info.userRating      = sqlite3_column_int(stmt, col++);
    info.userNotes       = getText(col++);
    info.aiSuggestedCategory = getText(col++);
    info.aiConfidence    = static_cast<float>(sqlite3_column_double(stmt, col++));
    info.aiReviewed      = sqlite3_column_int(stmt, col++) != 0;

    return info;
}

} // namespace librairy

