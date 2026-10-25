#include "ResultsTable.h"

namespace librairy
{

ResultsTable::ResultsTable()
{
    setupColumns();

    table.setColour(juce::ListBox::backgroundColourId,
                    juce::Colour(0xFF1E1E1E));
    table.setColour(juce::ListBox::outlineColourId,
                    juce::Colour(0xFF3D3D3D));
    table.setHeaderHeight(28);
    table.setRowHeight(24);
    table.setMultipleSelectionEnabled(true);
    addAndMakeVisible(table);
}

void ResultsTable::setupColumns()
{
    auto& header = table.getHeader();

    header.addColumn("Name",        kName,        220, 100, -1,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Duration",    kDuration,     70,  50, -1,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Sample Rate", kSampleRate,   85,  60, -1,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Bit Depth",   kBitDepth,     65,  50, -1,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Ch",          kChannels,     40,  30, -1,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Category",    kCategory,    120,  80, -1,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Description", kDescription, 240, 100, -1,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Path",        kPath,        280, 100, -1,
                     juce::TableHeaderComponent::defaultFlags);
    header.addColumn("Date",        kDate,        120,  80, -1,
                     juce::TableHeaderComponent::defaultFlags);
}

void ResultsTable::resized()
{
    table.setBounds(getLocalBounds());
}

void ResultsTable::mouseDrag(const juce::MouseEvent& /*event*/)
{
}

int ResultsTable::getNumRows()
{
    return static_cast<int>(currentResults.size());
}

void ResultsTable::paintRowBackground(juce::Graphics& g, int rowNumber,
                                       int /*width*/, int /*height*/,
                                       bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xFF3A5FCD));  // Selection blue
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(0xFF222222));  // Alternating row
    else
        g.fillAll(juce::Colour(0xFF1E1E1E));
}

void ResultsTable::paintCell(juce::Graphics& g, int rowNumber,
                              int columnId, int width, int height,
                              bool /*rowIsSelected*/)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(currentResults.size()))
        return;

    const auto& info = currentResults[static_cast<size_t>(rowNumber)];

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(13.0f));

    juce::String text;

    switch (columnId)
    {
        case kName:        text = info.filename;                               break;
        case kDuration:    text = juce::String(info.durationSeconds, 2) + "s"; break;
        case kSampleRate:  text = juce::String(info.sampleRate) + " Hz";       break;
        case kBitDepth:    text = juce::String(info.bitDepth) + " bit";        break;
        case kChannels:    text = (info.numChannels == 2) ? "2 (Stereo)" :
                                  (info.numChannels == 1) ? "1 (Mono)" :
                                  juce::String(info.numChannels);              break;
        case kCategory:    text = info.category.isNotEmpty() ? info.category : "-"; break;
        case kDescription: text = info.description;                            break;
        case kPath:        text = info.filePath;                               break;
        case kDate:        text = info.dateModified;                           break;
        default: break;
    }

    g.drawText(text, 6, 0, width - 12, height,
               juce::Justification::centredLeft, true);
}

void ResultsTable::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    std::sort(currentResults.begin(), currentResults.end(),
              [newSortColumnId, isForwards](const AudioFileInfo& a, const AudioFileInfo& b)
              {
                  int comp = 0;
                  switch (newSortColumnId)
                  {
                      case kName:        comp = a.filename.compareIgnoreCase(b.filename); break;
                      case kDuration:    comp = (a.durationSeconds < b.durationSeconds) ? -1 : (a.durationSeconds > b.durationSeconds ? 1 : 0); break;
                      case kSampleRate:  comp = a.sampleRate - b.sampleRate; break;
                      case kBitDepth:    comp = a.bitDepth - b.bitDepth; break;
                      case kChannels:    comp = a.numChannels - b.numChannels; break;
                      case kCategory:    comp = a.category.compareIgnoreCase(b.category); break;
                      case kDescription: comp = a.description.compareIgnoreCase(b.description); break;
                      case kDate:        comp = a.dateModified.compareIgnoreCase(b.dateModified); break;
                      default:           comp = a.filePath.compareIgnoreCase(b.filePath); break;
                  }
                  return isForwards ? (comp < 0) : (comp > 0);
              });

    table.updateContent();
    table.repaint();
}

void ResultsTable::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 &&
        lastRowSelected < static_cast<int>(currentResults.size()))
    {
        const auto& info = currentResults[static_cast<size_t>(lastRowSelected)];
        listeners.call([&info](Listener& l) { l.fileSelected(info); });
    }
}

void ResultsTable::cellDoubleClicked(int rowNumber, int /*columnId*/,
                                     const juce::MouseEvent& /*event*/)
{
    if (rowNumber >= 0 && rowNumber < static_cast<int>(currentResults.size()))
    {
        const auto& info = currentResults[static_cast<size_t>(rowNumber)];
        listeners.call([&info](Listener& l) { l.fileDoubleClicked(info); });
    }
}

juce::var ResultsTable::getDragSourceDescription(const juce::SparseSet<int>& /*currentlySelectedRows*/)
{
    return "librairy_audio_files";
}

void ResultsTable::triggerExternalDrag()
{
    auto selectedRows = table.getSelectedRows();
    juce::StringArray filesToDrag;

    for (int i = 0; i < selectedRows.size(); ++i)
    {
        int row = selectedRows[i];
        if (row >= 0 && row < static_cast<int>(currentResults.size()))
        {
            filesToDrag.add(currentResults[static_cast<size_t>(row)].filePath);
        }
    }

    if (filesToDrag.size() > 0)
    {
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            filesToDrag, false, this);
    }
}

std::optional<AudioFileInfo> ResultsTable::getSelectedFile() const
{
    int row = table.getSelectedRow();
    if (row >= 0 && row < static_cast<int>(currentResults.size()))
        return currentResults[static_cast<size_t>(row)];
    return std::nullopt;
}

void ResultsTable::setResults(std::vector<AudioFileInfo> results)
{
    currentResults = std::move(results);
    table.updateContent();
    table.repaint();
}

void ResultsTable::addListener(Listener* listener)    { listeners.add(listener); }
void ResultsTable::removeListener(Listener* listener)  { listeners.remove(listener); }

} // namespace librairy
