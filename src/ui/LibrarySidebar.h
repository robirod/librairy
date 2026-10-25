#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "metadata/MetadataModel.h"
#include <vector>

namespace librairy
{

/**
 * Sidebar displaying the list of audio libraries, scan progress,
 * and button to import/scan new folders.
 */
class LibrarySidebar : public juce::Component,
                       public juce::ListBoxModel
{
public:
    LibrarySidebar();
    ~LibrarySidebar() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // ---- ListBoxModel ----
    int  getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                          int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

    /** Update the list of libraries displayed */
    void setLibraries(std::vector<LibraryInfo> libraries);

    /** Set scan progress or status text to display */
    void setScanStatus(const juce::String& status);

    /** Listener for sidebar events */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void librarySelected(int64_t libraryId) = 0;
        virtual void addLibraryRequested() = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    juce::Label         titleLabel   { {}, "LIBRARIES" };
    juce::TextButton    addButton    { "+ Add Folder" };
    juce::ListBox       listBox      { "LibrariesList", this };
    juce::Label         statusLabel  { {}, "" };

    std::vector<LibraryInfo> currentLibraries;
    int64_t selectedLibraryId = -1; // -1 means "All Audio"

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibrarySidebar)
};

} // namespace librairy
