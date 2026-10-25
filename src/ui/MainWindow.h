#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "LibrarySidebar.h"
#include "SearchBar.h"
#include "ResultsTable.h"
#include "MetadataPanel.h"
#include "WaveformView.h"

#include "core/Database.h"
#include "core/SearchEngine.h"
#include "core/Scanner.h"
#include "audio/AudioEngine.h"
#include "metadata/MetadataWriter.h"

namespace librairy
{

//==============================================================================
/**
 * Main application window for Librairy.
 */
class MainWindow : public juce::DocumentWindow
{
public:
    explicit MainWindow(const juce::String& name);
    ~MainWindow() override = default;

    void closeButtonPressed() override;

private:
    //==========================================================================
    /** Content component that owns the full business logic and layout */
    class MainContent : public juce::Component,
                        public LibrarySidebar::Listener,
                        public SearchBar::Listener,
                        public ResultsTable::Listener,
                        public WaveformView::Listener,
                        public MetadataPanel::Listener,
                        public Scanner::Listener,
                        public juce::Timer
    {
    public:
        MainContent();
        ~MainContent() override;

        void resized() override;
        void paint(juce::Graphics& g) override;
        bool keyPressed(const juce::KeyPress& key) override;

        // ---- Timer ----
        void timerCallback() override;

        // ---- LibrarySidebar::Listener ----
        void librarySelected(int64_t libraryId) override;
        void addLibraryRequested() override;

        // ---- SearchBar::Listener ----
        void searchTextChanged(const juce::String& query) override;
        void searchSubmitted(const juce::String& query) override;

        // ---- ResultsTable::Listener ----
        void fileSelected(const AudioFileInfo& info) override;
        void fileDoubleClicked(const AudioFileInfo& info) override;

        // ---- WaveformView::Listener ----
        void positionClicked(double normalizedPosition) override;

        // ---- MetadataPanel::Listener ----
        void metadataChanged(const juce::String& field,
                             const juce::String& newValue) override;
        void metadataSaveRequested() override;

        // ---- Scanner::Listener ----
        void scanProgressUpdated(const Scanner::Progress& progress) override;
        void scanCompleted(const Scanner::Progress& finalProgress) override;
        void scanCancelled() override;

    private:
        // Core subsystems
        std::unique_ptr<Database>       db;
        std::unique_ptr<SearchEngine>   searchEngine;
        std::unique_ptr<Scanner>        scanner;
        AudioEngine                     audioEngine;
        MetadataWriter                  metadataWriter;

        // UI Components
        SearchBar           searchBar;
        LibrarySidebar      sidebar;
        ResultsTable        resultsTable;
        WaveformView        waveformView;
        MetadataPanel       metadataPanel;

        // State
        int64_t                      activeLibraryId = -1; // -1 = All
        juce::String                 activeQuery;
        std::optional<AudioFileInfo> activeFileInfo;
        std::unique_ptr<juce::FileChooser> fileChooser;

        void refreshLibraries();
        void performSearch();
        static juce::File getDefaultDatabaseFile();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContent)
    };

    std::unique_ptr<MainContent> contentComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace librairy
