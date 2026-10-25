#pragma once

/**
 * Auto-Tagger — Stage 2 stub.
 *
 * Orchestrates the auto-tagging workflow:
 * 1. Scan for untagged files
 * 2. Run inference on each
 * 3. Store suggestions in the database
 * 4. Present to user for review
 */

#ifdef LIBRAIRY_AI_ENABLED

namespace librairy
{

class AutoTagger
{
public:
    AutoTagger() = default;
    ~AutoTagger() = default;

    // TODO: auto-tag workflow orchestration
};

} // namespace librairy

#endif

