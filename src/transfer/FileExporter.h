#pragma once

#include "metadata/MetadataModel.h"
#include <juce_core/juce_core.h>

#include <functional>
#include <vector>

namespace librairy
{

/**
 * Exports/transfers audio files to a destination folder.
 *
 * Supports:
 *   - Copy or Move
 *   - Flat or structured output (by category, by date)
 *   - Filename renaming templates
 *   - Duplicate handling
 */
class FileExporter
{
public:
    FileExporter() = default;
    ~FileExporter() = default;

    /** Output folder structure strategy */
    enum class FolderStructure
    {
        Flat,           // All files in one folder
        ByCategory,     // Subfolders by category
        BySubcategory,  // category/subcategory
        ByDate,         // YYYY/MM/DD
        ByFormat        // WAV/, AIFF/, FLAC/
    };

    /** Export configuration */
    struct ExportConfig
    {
        juce::File       destination;
        FolderStructure  structure     = FolderStructure::Flat;
        bool             copyNotMove   = true;   // true = copy, false = move
        bool             overwrite     = false;
        juce::String     renamingPattern;         // e.g., "{category}_{filename}"
    };

    /** Export result for each file */
    struct ExportResult
    {
        juce::String sourcePath;
        juce::String destPath;
        bool         success = false;
        juce::String errorMessage;
    };

    /**
     * Export files to the destination.
     *
     * @param files     Files to export
     * @param config    Export configuration
     * @param progress  Optional callback for progress (0.0 - 1.0)
     * @return          Results for each file
     */
    std::vector<ExportResult> exportFiles(
        const std::vector<AudioFileInfo>& files,
        const ExportConfig& config,
        std::function<void(float)> progress = nullptr) const;

private:
    /** Determine the output path for a file given the config */
    juce::File getOutputPath(const AudioFileInfo& info,
                             const ExportConfig& config) const;

    /** Apply renaming pattern */
    juce::String applyRenaming(const AudioFileInfo& info,
                                const juce::String& pattern) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileExporter)
};

} // namespace librairy

