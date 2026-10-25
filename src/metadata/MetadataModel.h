#pragma once

#include <juce_core/juce_core.h>
#include <string>
#include <vector>

namespace librairy
{

/**
 * Data model for audio file information.
 * Represents both technical properties and embedded metadata.
 */
struct AudioFileInfo
{
    // --- Database ID ---
    int64_t id = -1;
    int64_t libraryId = -1;

    // --- File Properties ---
    juce::String filePath;
    juce::String filename;
    int64_t      fileSizeBytes = 0;
    juce::String dateModified;
    juce::String fileHash;        // Partial hash for move detection

    // --- Audio Properties ---
    double  durationSeconds = 0.0;
    int     sampleRate      = 0;
    int     bitDepth        = 0;
    int     numChannels     = 0;
    juce::String formatName;      // WAV, AIFF, FLAC, etc.

    // --- BWF Metadata ---
    juce::String description;
    juce::String originator;
    juce::String originatorRef;
    juce::String originationDate;
    juce::String originationTime;
    int64_t      timeReference = 0;  // Sample position for BWF
    juce::String codingHistory;

    // --- iXML / Categorical Metadata ---
    juce::String category;
    juce::String subcategory;
    juce::String takeType;
    juce::String scene;
    juce::String notes;
    juce::String keywords;        // Comma-separated tags

    // --- User / Custom ---
    int          userRating = 0;  // 1-5 stars, 0 = unrated
    juce::String userNotes;

    // --- AI (Stage 2) ---
    juce::String aiSuggestedCategory;
    float        aiConfidence = 0.0f;
    bool         aiReviewed   = false;
};

/**
 * Library descriptor — a named collection of root paths.
 */
struct LibraryInfo
{
    int64_t      id = -1;
    juce::String name;
    juce::StringArray rootPaths;
    juce::String dateCreated;
    int64_t      fileCount = 0;
};

/**
 * Supported audio file extensions.
 */
inline const juce::StringArray& getSupportedExtensions()
{
    static const juce::StringArray extensions {
        ".wav", ".wave", ".bwf",
        ".aif", ".aiff", ".aifc",
        ".flac",
        ".mp3",
        ".ogg", ".oga",
        ".caf",
        ".w64",           // Sony Wave64
        ".rf64"           // EBU RF64
    };
    return extensions;
}

/** Check if a file extension is a supported audio format */
inline bool isSupportedAudioFile(const juce::File& file)
{
    auto ext = file.getFileExtension().toLowerCase();
    return getSupportedExtensions().contains(ext);
}

} // namespace librairy

