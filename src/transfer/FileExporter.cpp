#include "FileExporter.h"

namespace librairy
{

std::vector<FileExporter::ExportResult> FileExporter::exportFiles(
    const std::vector<AudioFileInfo>& files,
    const ExportConfig& config,
    std::function<void(float)> progress) const
{
    std::vector<ExportResult> results;
    results.reserve(files.size());

    if (!config.destination.isDirectory())
        config.destination.createDirectory();

    for (size_t i = 0; i < files.size(); ++i)
    {
        ExportResult result;
        result.sourcePath = files[i].filePath;

        juce::File sourceFile(files[i].filePath);
        if (!sourceFile.existsAsFile())
        {
            result.success = false;
            result.errorMessage = "Source file not found";
            results.push_back(std::move(result));
            continue;
        }

        auto destFile = getOutputPath(files[i], config);

        // Create parent directories
        destFile.getParentDirectory().createDirectory();

        // Handle duplicates
        if (destFile.existsAsFile() && !config.overwrite)
        {
            // Generate unique name
            int counter = 1;
            auto baseName = destFile.getFileNameWithoutExtension();
            auto ext = destFile.getFileExtension();
            auto parent = destFile.getParentDirectory();

            while (destFile.existsAsFile())
            {
                destFile = parent.getChildFile(
                    baseName + "_" + juce::String(counter++) + ext);
            }
        }

        result.destPath = destFile.getFullPathName();

        // Copy or move
        bool ok = false;
        if (config.copyNotMove)
            ok = sourceFile.copyFileTo(destFile);
        else
            ok = sourceFile.moveFileTo(destFile);

        result.success = ok;
        if (!ok)
            result.errorMessage = "Failed to " +
                juce::String(config.copyNotMove ? "copy" : "move") +
                " file";

        results.push_back(std::move(result));

        // Report progress
        if (progress)
            progress(static_cast<float>(i + 1) /
                     static_cast<float>(files.size()));
    }

    return results;
}

juce::File FileExporter::getOutputPath(
    const AudioFileInfo& info,
    const ExportConfig& config) const
{
    juce::String filename = info.filename;

    // Apply renaming if pattern is set
    if (config.renamingPattern.isNotEmpty())
    {
        auto newName = applyRenaming(info, config.renamingPattern);
        if (newName.isNotEmpty())
        {
            // Preserve extension
            auto ext = juce::File(info.filePath).getFileExtension();
            filename = newName + ext;
        }
    }

    switch (config.structure)
    {
        case FolderStructure::ByCategory:
        {
            auto cat = info.category.isNotEmpty() ? info.category : "Uncategorized";
            return config.destination.getChildFile(cat).getChildFile(filename);
        }
        case FolderStructure::BySubcategory:
        {
            auto cat = info.category.isNotEmpty() ? info.category : "Uncategorized";
            auto sub = info.subcategory.isNotEmpty() ? info.subcategory : "General";
            return config.destination.getChildFile(cat)
                                     .getChildFile(sub)
                                     .getChildFile(filename);
        }
        case FolderStructure::ByDate:
        {
            auto date = info.dateModified.isNotEmpty() ? info.dateModified : "Unknown";
            // Use just the date part (first 10 chars of ISO date)
            auto dateFolder = date.substring(0, 10).replaceCharacter('-', '/');
            return config.destination.getChildFile(dateFolder)
                                     .getChildFile(filename);
        }
        case FolderStructure::ByFormat:
        {
            auto fmt = info.formatName.isNotEmpty()
                ? info.formatName.toUpperCase() : "OTHER";
            return config.destination.getChildFile(fmt).getChildFile(filename);
        }
        case FolderStructure::Flat:
        default:
            return config.destination.getChildFile(filename);
    }
}

juce::String FileExporter::applyRenaming(
    const AudioFileInfo& info,
    const juce::String& pattern) const
{
    auto result = pattern;

    result = result.replace("{filename}",
        juce::File(info.filePath).getFileNameWithoutExtension());
    result = result.replace("{category}",
        info.category.isNotEmpty() ? info.category : "Uncategorized");
    result = result.replace("{subcategory}",
        info.subcategory.isNotEmpty() ? info.subcategory : "General");
    result = result.replace("{originator}",
        info.originator.isNotEmpty() ? info.originator : "Unknown");
    result = result.replace("{samplerate}",
        juce::String(info.sampleRate));
    result = result.replace("{bitdepth}",
        juce::String(info.bitDepth));
    result = result.replace("{channels}",
        juce::String(info.numChannels));

    return result;
}

} // namespace librairy

