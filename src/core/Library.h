#pragma once

#include "metadata/MetadataModel.h"
#include <juce_core/juce_core.h>

namespace librairy
{

/**
 * Placeholder for Library management logic.
 * Coordinates Scanner, Database, and SearchEngine for a given library.
 */
class Library
{
public:
    Library() = default;
    ~Library() = default;

    // TODO: High-level library management methods
    // - createLibrary(name, rootPaths)
    // - openLibrary(id)
    // - scanLibrary(id)
    // - getStats(id)
};

} // namespace librairy

