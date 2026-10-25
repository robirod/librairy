#include "LibrarySidebar.h"

namespace librairy
{

LibrarySidebar::LibrarySidebar()
{
    titleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    addAndMakeVisible(titleLabel);

    addButton.onClick = [this]()
    {
        listeners.call([](Listener& l) { l.addLibraryRequested(); });
    };
    addButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A2A2A));
    addButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(addButton);

    listBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xFF181818));
    listBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xFF2C2C2C));
    listBox.setRowHeight(32);
    addAndMakeVisible(listBox);

    statusLabel.setFont(juce::FontOptions(11.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF4CAF50)); // Green accent
    addAndMakeVisible(statusLabel);
}

void LibrarySidebar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF181818));

    // Right dividing border
    g.setColour(juce::Colour(0xFF333333));
    g.drawLine(static_cast<float>(getWidth() - 1), 0.f,
               static_cast<float>(getWidth() - 1),
               static_cast<float>(getHeight()));
}

void LibrarySidebar::resized()
{
    auto bounds = getLocalBounds().reduced(8, 8);

    titleLabel.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(4);

    addButton.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(8);

    if (statusLabel.getText().isNotEmpty())
    {
        statusLabel.setBounds(bounds.removeFromBottom(24));
        bounds.removeFromBottom(4);
    }

    listBox.setBounds(bounds);
}

int LibrarySidebar::getNumRows()
{
    // Row 0 is always "All Audio"
    return 1 + static_cast<int>(currentLibraries.size());
}

void LibrarySidebar::paintListBoxItem(int rowNumber, juce::Graphics& g,
                                      int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xFF3A5FCD)); // Active selection blue
    else
        g.fillAll(rowNumber % 2 == 0 ? juce::Colour(0xFF1E1E1E) : juce::Colour(0xFF181818));

    g.setFont(juce::FontOptions(13.0f));
    g.setColour(juce::Colours::white);

    if (rowNumber == 0)
    {
        g.drawText("All Audio", 10, 0, width - 20, height,
                   juce::Justification::centredLeft, true);
    }
    else
    {
        size_t idx = static_cast<size_t>(rowNumber - 1);
        if (idx < currentLibraries.size())
        {
            auto name = currentLibraries[idx].name;
            g.drawText(name, 10, 0, width - 60, height,
                       juce::Justification::centredLeft, true);

            // Draw file count on the right
            g.setColour(juce::Colour(0xFF888888));
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(juce::String(currentLibraries[idx].fileCount),
                       width - 55, 0, 45, height,
                       juce::Justification::centredRight, true);
        }
    }
}

void LibrarySidebar::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected == 0)
    {
        selectedLibraryId = -1; // All libraries
        listeners.call([](Listener& l) { l.librarySelected(-1); });
    }
    else if (lastRowSelected > 0)
    {
        size_t idx = static_cast<size_t>(lastRowSelected - 1);
        if (idx < currentLibraries.size())
        {
            selectedLibraryId = currentLibraries[idx].id;
            listeners.call([this](Listener& l) { l.librarySelected(selectedLibraryId); });
        }
    }
}

void LibrarySidebar::setLibraries(std::vector<LibraryInfo> libraries)
{
    currentLibraries = std::move(libraries);
    listBox.updateContent();
    listBox.repaint();
}

void LibrarySidebar::setScanStatus(const juce::String& status)
{
    statusLabel.setText(status, juce::dontSendNotification);
    resized();
    repaint();
}

void LibrarySidebar::addListener(Listener* listener)
{
    listeners.add(listener);
}

void LibrarySidebar::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

} // namespace librairy
