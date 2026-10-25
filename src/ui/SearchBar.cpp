#include "SearchBar.h"

namespace librairy
{

SearchBar::SearchBar()
{
    // Search input
    searchInput.setMultiLine(false);
    searchInput.setTextToShowWhenEmpty("Search library... (Cmd+F)",
                                       juce::Colours::grey);
    searchInput.setColour(juce::TextEditor::backgroundColourId,
                          juce::Colour(0xFF2D2D2D));
    searchInput.setColour(juce::TextEditor::textColourId,
                          juce::Colours::white);
    searchInput.setColour(juce::TextEditor::outlineColourId,
                          juce::Colour(0xFF3D3D3D));
    searchInput.onTextChange = [this]()
    {
        listeners.call([this](Listener& l)
        {
            l.searchTextChanged(searchInput.getText());
        });
    };
    searchInput.onReturnKey = [this]()
    {
        listeners.call([this](Listener& l)
        {
            l.searchSubmitted(searchInput.getText());
        });
    };

    addAndMakeVisible(searchInput);

    // Clear button
    clearButton.onClick = [this]() { clear(); };
    addAndMakeVisible(clearButton);

    // Filter button
    addAndMakeVisible(filterButton);
}

void SearchBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF252525));

    // Bottom border
    g.setColour(juce::Colour(0xFF3D3D3D));
    g.drawLine(0.f,
               static_cast<float>(getHeight() - 1),
               static_cast<float>(getWidth()),
               static_cast<float>(getHeight() - 1));
}

void SearchBar::resized()
{
    auto bounds = getLocalBounds().reduced(8, 8);

    filterButton.setBounds(bounds.removeFromRight(80));
    bounds.removeFromRight(4);
    clearButton.setBounds(bounds.removeFromRight(32));
    bounds.removeFromRight(4);
    searchInput.setBounds(bounds);
}

void SearchBar::addListener(Listener* listener)
{
    listeners.add(listener);
}

void SearchBar::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

juce::String SearchBar::getSearchText() const
{
    return searchInput.getText();
}

void SearchBar::clear()
{
    searchInput.clear();
    listeners.call([](Listener& l)
    {
        l.searchTextChanged("");
    });
}

} // namespace librairy

