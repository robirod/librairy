#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "metadata/MetadataModel.h"

namespace librairy
{

/**
 * Sortable, filterable table displaying audio file search results.
 * Supports row selection, double-click to play, and external drag & drop to DAWs.
 */
class ResultsTable : public juce::Component,
                     public juce::TableListBoxModel
{
public:
    ResultsTable();
    ~ResultsTable() override = default;

    // ---- TableListBoxModel ----
    int  getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber,
                            int width, int height,
                            bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId,
                   int width, int height,
                   bool rowIsSelected) override;
    void sortOrderChanged(int newSortColumnId,
                          bool isForwards) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void cellDoubleClicked(int rowNumber, int columnId,
                           const juce::MouseEvent& event) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int>& currentlySelectedRows) override;

    // ---- Component ----
    void resized() override;
    void mouseDrag(const juce::MouseEvent& event) override;

    /** Replace the data displayed in the table */
    void setResults(std::vector<AudioFileInfo> results);

    /** Get current results list */
    const std::vector<AudioFileInfo>& getResults() const { return currentResults; }

    /** Get selected file info (if any) */
    std::optional<AudioFileInfo> getSelectedFile() const;

    /** Listener for selection events */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void fileSelected(const AudioFileInfo& info) = 0;
        virtual void fileDoubleClicked(const AudioFileInfo& info) = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    class DragTableListBox : public juce::TableListBox
    {
    public:
        DragTableListBox(const juce::String& name, TableListBoxModel* model)
            : TableListBox(name, model) {}

        void mouseDrag(const juce::MouseEvent& event) override
        {
            TableListBox::mouseDrag(event);
            if (event.getDistanceFromDragStart() > 6)
            {
                auto selectedRows = getSelectedRows();
                if (selectedRows.size() > 0)
                {
                    if (auto* parent = findParentComponentOfClass<ResultsTable>())
                    {
                        parent->triggerExternalDrag();
                    }
                }
            }
        }
    };

    friend class DragTableListBox;
    void triggerExternalDrag();

    DragTableListBox table { "Results", this };
    std::vector<AudioFileInfo> currentResults;

    juce::ListenerList<Listener> listeners;

    // Column IDs
    enum ColumnIds
    {
        kName       = 1,
        kDuration   = 2,
        kSampleRate = 3,
        kBitDepth   = 4,
        kChannels   = 5,
        kCategory   = 6,
        kDescription= 7,
        kPath       = 8,
        kDate       = 9
    };

    void setupColumns();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResultsTable)
};

} // namespace librairy
